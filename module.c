#include <Python.h>
#include <datetime.h>

#include "timezone.h"

/* ########################################################### */

static inline PyObject *
tzinfo_from_isoformat_results(int rv, int tzoffset, int tz_useconds)
{
    PyObject *tzinfo;
    if (rv == 1) {
        tzinfo = new_fixed_offset(tzoffset);
    }
    else {
        tzinfo = Py_None;
        Py_INCREF(Py_None);
    }

    return tzinfo;
}

static const char *
parse_digits(const char *ptr, int *var, size_t num_digits)
{
    size_t i = 0;
    for (i = 0; i < num_digits; ++i) {
        unsigned int tmp = (unsigned int)(*(ptr++) - '0');
        if (tmp > 9) {
            return NULL;
        }
        *var *= 10;
        *var += (signed int)tmp;
    }

    return ptr;
}

static int
parse_isoformat_date(const char *dtstr, int *year, int *month, int *day)
{
    /* Parse the date components of the result of date.isoformat()
     *
     *  Return codes:
     *       0:  Success
     *      -1:  Failed to parse date component
     *      -2:  Failed to parse dateseparator
     */
    const char *p = dtstr;
    p = parse_digits(p, year, 4);
    if (NULL == p) {
        return -1;
    }

    if (*(p++) != '-') {
        return -2;
    }

    p = parse_digits(p, month, 2);
    if (NULL == p) {
        return -1;
    }

    if (*(p++) != '-') {
        return -2;
    }

    p = parse_digits(p, day, 2);
    if (p == NULL) {
        return -1;
    }

    return 0;
}

static int
parse_hh_mm_ss_ff(const char *tstr, const char *tstr_end, int *hour,
                  int *minute, int *second, int *microsecond)
{
    const char *p = tstr;
    const char *p_end = tstr_end;
    int *vals[3] = {hour, minute, second};
    size_t i = 0;

    // Parse [HH[:MM[:SS]]]
    for (i = 0; i < 3; ++i) {
        p = parse_digits(p, vals[i], 2);
        if (NULL == p) {
            return -3;
        }

        char c = *(p++);
        if (p >= p_end) {
            return c != '\0';
        }
        else if (c == ':') {
            continue;
        }
        else if (c == '.') {
            break;
        }
        else {
            return -4;  // Malformed time separator
        }
    }

    // Parse .fff[fff]
    size_t len_remains = p_end - p;
    if (!(len_remains == 6 || len_remains == 3)) {
        return -3;
    }

    p = parse_digits(p, microsecond, len_remains);
    if (NULL == p) {
        return -3;
    }

    if (len_remains == 3) {
        *microsecond *= 1000;
    }

    // Return 1 if it's not the end of the string
    return *p != '\0';
}

static int
parse_isoformat_time(const char *dtstr, size_t dtlen, int *hour, int *minute,
                     int *second, int *microsecond, int *tzoffset,
                     int *tzmicrosecond)
{
    // Parse the time portion of a datetime.isoformat() string
    //
    // Return codes:
    //      0:  Success (no tzoffset)
    //      1:  Success (with tzoffset)
    //     -3:  Failed to parse time component
    //     -4:  Failed to parse time separator
    //     -5:  Malformed timezone string

    const char *p = dtstr;
    const char *p_end = dtstr + dtlen;

    const char *tzinfo_pos = p;
    do {
        if (*tzinfo_pos == '+' || *tzinfo_pos == '-') {
            break;
        }
    } while (++tzinfo_pos < p_end);

    int rv = parse_hh_mm_ss_ff(dtstr, tzinfo_pos, hour, minute, second,
                               microsecond);

    if (rv < 0) {
        return rv;
    }
    else if (tzinfo_pos == p_end) {
        // We know that there's no time zone, so if there's stuff at the
        // end of the string it's an error.
        if (rv == 1) {
            return -5;
        }
        else {
            return 0;
        }
    }

    // Parse time zone component
    // Valid formats are:
    //    - +HH:MM           (len  6)
    //    - +HH:MM:SS        (len  9)
    //    - +HH:MM:SS.ffffff (len 16)
    size_t tzlen = p_end - tzinfo_pos;
    if (!(tzlen == 6 || tzlen == 9 || tzlen == 16)) {
        return -5;
    }

    int tzsign = (*tzinfo_pos == '-') ? -1 : 1;
    tzinfo_pos++;
    int tzhour = 0, tzminute = 0, tzsecond = 0;
    rv = parse_hh_mm_ss_ff(tzinfo_pos, p_end, &tzhour, &tzminute, &tzsecond,
                           tzmicrosecond);

    *tzoffset = tzsign * ((tzhour * 3600) + (tzminute * 60) + tzsecond);
    *tzmicrosecond *= tzsign;

    return rv ? -5 : 1;
}

static PyObject *
datetime_fromisoformat(PyObject *dtstr)
{
    assert(dtstr != NULL);

    if (!PyUnicode_Check(dtstr)) {
        PyErr_SetString(PyExc_TypeError,
                        "fromisoformat: argument must be str");
        return NULL;
    }

    Py_ssize_t len;
    const char *dt_ptr = PyUnicode_AsUTF8AndSize(dtstr, &len);
    const char *p = dt_ptr;

    int year = 0, month = 0, day = 0;
    int hour = 0, minute = 0, second = 0, microsecond = 0;
    int tzoffset = 0, tzusec = 0;

    // date has a fixed length of 10
    int rv = parse_isoformat_date(p, &year, &month, &day);

    if (!rv && len > 10) {
        // In UTF-8, the length of multi-byte characters is encoded in the MSB
        if ((p[10] & 0x80) == 0) {
            p += 11;
        }
        else {
            switch (p[10] & 0xf0) {
                case 0xe0:
                    p += 13;
                    break;
                case 0xf0:
                    p += 14;
                    break;
                default:
                    p += 12;
                    break;
            }
        }

        len -= (p - dt_ptr);
        rv = parse_isoformat_time(p, len, &hour, &minute, &second,
                                  &microsecond, &tzoffset, &tzusec);
    }
    if (rv < 0) {
        PyErr_Format(PyExc_ValueError, "Invalid isoformat string: %s", dt_ptr);
        return NULL;
    }

    PyObject *tzinfo = tzinfo_from_isoformat_results(rv, tzoffset, tzusec);
    if (tzinfo == NULL) {
        return NULL;
    }

    PyObject *dt = PyDateTimeAPI->DateTime_FromDateAndTime(
        year, month, day, hour, minute, second, microsecond, tzinfo,
        PyDateTimeAPI->DateTimeType);

    Py_DECREF(tzinfo);
    return dt;
}

/* ########################################################### */

static PyObject *
fromisoformat(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *tzinfo = Py_None;

    obj = datetime_fromisoformat(PyTuple_GetItem(args, 0));

    // obj = PyDateTimeAPI->DateTime_FromDateAndTime(
    //     2014, 2, 5, 23, 45, 0, 0, tzinfo, PyDateTimeAPI->DateTimeType);

    return obj;
}

static PyMethodDef FromISOFormatMethods[] = {
    {"fromisoformat", fromisoformat, METH_VARARGS,
     "Return a datetime corresponding to a date_string in one of the formats "
     "emitted by date.isoformat() and datetime.isoformat()"},
    {NULL, NULL, 0, NULL}};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "datetime_fromisoformat",
    NULL,
    -1,
    FromISOFormatMethods,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_datetime_fromisoformat(void)
#else
initdatetime_fromisoformat(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    (void)Py_InitModule("datetime_fromisoformat", FromISOFormatMethods);
#endif
    PyDateTime_IMPORT;

#if PY_MAJOR_VERSION >= 3
    if (initialize_timezone_code() < 0)
        return NULL;
#else
    initialize_timezone_code();
#endif

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

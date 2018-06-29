/* This file was originally taken from cPython 3.7's code base
 * (`Modules/_datetimemodule.c`). (2018-06-10 18:02:24, commit SHA-1:
 * 037e9125527d4a55af566f161c96a61b3c3fd998)
 *
 * Since then, I have:
 *   - torn out all the functionality that doesn't matter to
 *     `backports.datetime_fromisoformat`
 *   - switched calls to datetime creation to use the versions found in
 *     `PyDateTimeAPI`
 *   - made minor changes to make it compilable for older versions of Python.
 *
 * Below is a copy of the Python 3.7 code license
 * (from https://docs.python.org/3/license.html):
 *
 * PSF LICENSE AGREEMENT FOR PYTHON 3.7.0
 *
 * 1. This LICENSE AGREEMENT is between the Python Software Foundation ("PSF"),
 *    and the Individual or Organization ("Licensee") accessing and otherwise
 *    using Python 3.7.0 software in source or binary form and its associated
 *    documentation.
 *
 * 2. Subject to the terms and conditions of this License Agreement, PSF hereby
 *    grants Licensee a nonexclusive, royalty-free, world-wide license to
 *    reproduce, analyze, test, perform and/or display publicly, prepare
 *    derivative works, distribute, and otherwise use Python 3.7.0 alone or in
 *    any derivative version, provided, however, that PSF's License Agreement
 *    and PSF's notice of copyright, i.e., "Copyright © 2001-2018 Python
 *    Software Foundation; All Rights Reserved" are retained in Python 3.7.0
 *    alone or in any derivative version prepared by Licensee.
 *
 * 3. In the event Licensee prepares a derivative work that is based on or
 *    incorporates Python 3.7.0 or any part thereof, and wants to make the
 *    derivative work available to others as provided herein, then Licensee
 *    hereby agrees to include in any such work a brief summary of the changes
 *    made to Python 3.7.0.
 *
 * 4. PSF is making Python 3.7.0 available to Licensee on an "AS IS" basis.
 *    PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  BY WAY
 *    OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND DISCLAIMS ANY
 *    REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY
 *    PARTICULAR PURPOSE OR THAT THE USE OF PYTHON 3.7.0 WILL NOT INFRINGE ANY
 *    THIRD PARTY RIGHTS.
 *
 * 5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON 3.7.0
 *    FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS A RESULT
 *    OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON 3.7.0, OR ANY
 *    DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 *
 * 6. This License Agreement will automatically terminate upon a material
 *    breach of its terms and conditions.
 *
 * 7. Nothing in this License Agreement shall be deemed to create any
 *    relationship of agency, partnership, or joint venture between PSF and
 *    Licensee.  This License Agreement does not grant permission to use PSF
 *    trademarks or trade name in a trademark sense to endorse or promote
 *    products or services of Licensee, or any third party.
 *
 * 8. By copying, installing or otherwise using Python 3.7.0, Licensee agrees
 *    to be bound by the terms and conditions of this License Agreement.
 */

/*  C implementation for the date/time type documented at
 *  http://www.zope.org/Members/fdrake/DateTimeWiki/FrontPage
 */

#include "_datetimemodule.h"
#include <datetime.h>
#include "Python.h"
#include "timezone.h"

/* ---------------------------------------------------------------------------
 * String parsing utilities and helper functions
 */

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

PyObject *
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

void
initialize_datetime_code(void)
{
    PyDateTime_IMPORT;
}
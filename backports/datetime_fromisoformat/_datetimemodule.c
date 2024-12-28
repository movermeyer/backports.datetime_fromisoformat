/* This file was originally taken from cPython's code base
 * (`Modules/_datetimemodule.c`). (2018-06-10 18:02:24, commit SHA-1:
 * 037e9125527d4a55af566f161c96a61b3c3fd998)
 * It was then refreshed using the Python 3.11 contents present at
 * 27d8dc2c9d3de886a884f79f0621d4586c0e0f7a
 * Finally, the `_PyUnicode_Copy` implementation was copied from
 * c8749b578324ad4089c8d014d9136bc42b065343
 *
 * Since then, I have:
 *   - torn out all the functionality that doesn't matter to
 *     `backports.datetime_fromisoformat`
 *   - switched calls to datetime creation to use the versions found in
 *     `PyDateTimeAPI`
 *   - made minor changes to make it compilable for older versions of Python.
 *     - Including in-lining a copy of _PyUnicode_Copy
 *
 * Below is a copy of the Python 3.11 code license
 * (from https://docs.python.org/3/license.html):
 *
 * PSF LICENSE AGREEMENT FOR PYTHON 3.11.0
 *
 * 1. This LICENSE AGREEMENT is between the Python Software Foundation ("PSF"), and
 *    the Individual or Organization ("Licensee") accessing and otherwise using Python
 *    3.11.0 software in source or binary form and its associated documentation.
 *
 * 2. Subject to the terms and conditions of this License Agreement, PSF hereby
 *    grants Licensee a nonexclusive, royalty-free, world-wide license to reproduce,
 *    analyze, test, perform and/or display publicly, prepare derivative works,
 *    distribute, and otherwise use Python 3.11.0 alone or in any derivative
 *    version, provided, however, that PSF's License Agreement and PSF's notice of
 *    copyright, i.e., "Copyright © 2001-2022 Python Software Foundation; All Rights
 *    Reserved" are retained in Python 3.11.0 alone or in any derivative version
 *    prepared by Licensee.
 *
 * 3. In the event Licensee prepares a derivative work that is based on or
 *    incorporates Python 3.11.0 or any part thereof, and wants to make the
 *    derivative work available to others as provided herein, then Licensee hereby
 *    agrees to include in any such work a brief summary of the changes made to Python
 *    3.11.0.
 *
 * 4. PSF is making Python 3.11.0 available to Licensee on an "AS IS" basis.
 *    PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  BY WAY OF
 *    EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND DISCLAIMS ANY REPRESENTATION OR
 *    WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE
 *    USE OF PYTHON 3.11.0 WILL NOT INFRINGE ANY THIRD PARTY RIGHTS.
 *
 * 5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON 3.11.0
 *    FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS A RESULT OF
 *    MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON 3.11.0, OR ANY DERIVATIVE
 *    THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 *
 * 6. This License Agreement will automatically terminate upon a material breach of
 *    its terms and conditions.
 *
 * 7. Nothing in this License Agreement shall be deemed to create any relationship
 *    of agency, partnership, or joint venture between PSF and Licensee.  This License
 *    Agreement does not grant permission to use PSF trademarks or trade name in a
 *    trademark sense to endorse or promote products or services of Licensee, or any
 *    third party.
 *
 * 8. By copying, installing or otherwise using Python 3.11.0, Licensee agrees
 *    to be bound by the terms and conditions of this License Agreement.
 */

/*  C implementation for the date/time type documented at
 *  http://www.zope.org/Members/fdrake/DateTimeWiki/FrontPage
 */

#include "_datetimemodule.h"
#include <datetime.h>
#include "Python.h"
#include "timezone.h"

#define PY_VERSION_AT_LEAST_36 \
    ((PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6) || PY_MAJOR_VERSION > 3)

#define MINYEAR 1
#define MAXYEAR 9999

/* ---------------------------------------------------------------------------
 * General calendrical helper functions
 */

/* For each month ordinal in 1..12, the number of days in that month,
 * and the number of days before that month in the same year.  These
 * are correct for non-leap years only.
 */
static const int _days_in_month[] = {
    0, /* unused; this vector uses 1-based indexing */
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const int _days_before_month[] = {
    0, /* unused; this vector uses 1-based indexing */
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/* year -> 1 if leap year, else 0. */
static int
is_leap(int year)
{
    /* Cast year to unsigned.  The result is the same either way, but
     * C can generate faster code for unsigned mod than for signed
     * mod (especially for % 4 -- a good compiler should just grab
     * the last 2 bits when the LHS is unsigned).
     */
    const unsigned int ayear = (unsigned int)year;
    return ayear % 4 == 0 && (ayear % 100 != 0 || ayear % 400 == 0);
}

/* year, month -> number of days in that month in that year */
static int
days_in_month(int year, int month)
{
    assert(month >= 1);
    assert(month <= 12);
    if (month == 2 && is_leap(year))
        return 29;
    else
        return _days_in_month[month];
}

/* year, month -> number of days in year preceding first day of month */
static int
days_before_month(int year, int month)
{
    int days;

    assert(month >= 1);
    assert(month <= 12);
    days = _days_before_month[month];
    if (month > 2 && is_leap(year))
        ++days;
    return days;
}

/* year -> number of days before January 1st of year.  Remember that we
 * start with year 1, so days_before_year(1) == 0.
 */
static int
days_before_year(int year)
{
    int y = year - 1;
    /* This is incorrect if year <= 0; we really want the floor
     * here.  But so long as MINYEAR is 1, the smallest year this
     * can see is 1.
     */
    assert (year >= 1);
    return y*365 + y/4 - y/100 + y/400;
}

/* Number of days in 4, 100, and 400 year cycles.  That these have
 * the correct values is asserted in the module init function.
 */
#define DI4Y    1461    /* days_before_year(5); days in 4 years */
#define DI100Y  36524   /* days_before_year(101); days in 100 years */
#define DI400Y  146097  /* days_before_year(401); days in 400 years  */

/* ordinal -> year, month, day, considering 01-Jan-0001 as day 1. */
static void
ord_to_ymd(int ordinal, int *year, int *month, int *day)
{
    int n, n1, n4, n100, n400, leapyear, preceding;

    /* ordinal is a 1-based index, starting at 1-Jan-1.  The pattern of
     * leap years repeats exactly every 400 years.  The basic strategy is
     * to find the closest 400-year boundary at or before ordinal, then
     * work with the offset from that boundary to ordinal.  Life is much
     * clearer if we subtract 1 from ordinal first -- then the values
     * of ordinal at 400-year boundaries are exactly those divisible
     * by DI400Y:
     *
     *    D  M   Y            n              n-1
     *    -- --- ----        ----------     ----------------
     *    31 Dec -400        -DI400Y       -DI400Y -1
     *     1 Jan -399         -DI400Y +1   -DI400Y      400-year boundary
     *    ...
     *    30 Dec  000        -1             -2
     *    31 Dec  000         0             -1
     *     1 Jan  001         1              0          400-year boundary
     *     2 Jan  001         2              1
     *     3 Jan  001         3              2
     *    ...
     *    31 Dec  400         DI400Y        DI400Y -1
     *     1 Jan  401         DI400Y +1     DI400Y      400-year boundary
     */
    assert(ordinal >= 1);
    --ordinal;
    n400 = ordinal / DI400Y;
    n = ordinal % DI400Y;
    *year = n400 * 400 + 1;

    /* Now n is the (non-negative) offset, in days, from January 1 of
     * year, to the desired date.  Now compute how many 100-year cycles
     * precede n.
     * Note that it's possible for n100 to equal 4!  In that case 4 full
     * 100-year cycles precede the desired day, which implies the
     * desired day is December 31 at the end of a 400-year cycle.
     */
    n100 = n / DI100Y;
    n = n % DI100Y;

    /* Now compute how many 4-year cycles precede it. */
    n4 = n / DI4Y;
    n = n % DI4Y;

    /* And now how many single years.  Again n1 can be 4, and again
     * meaning that the desired day is December 31 at the end of the
     * 4-year cycle.
     */
    n1 = n / 365;
    n = n % 365;

    *year += n100 * 100 + n4 * 4 + n1;
    if (n1 == 4 || n100 == 4) {
        assert(n == 0);
        *year -= 1;
        *month = 12;
        *day = 31;
        return;
    }

    /* Now the year is correct, and n is the offset from January 1.  We
     * find the month via an estimate that's either exact or one too
     * large.
     */
    leapyear = n1 == 3 && (n4 != 24 || n100 == 3);
    assert(leapyear == is_leap(*year));
    *month = (n + 50) >> 5;
    preceding = (_days_before_month[*month] + (*month > 2 && leapyear));
    if (preceding > n) {
        /* estimate is too large */
        *month -= 1;
        preceding -= days_in_month(*year, *month);
    }
    n -= preceding;
    assert(0 <= n);
    assert(n < days_in_month(*year, *month));

    *day = n + 1;
}

/* year, month, day -> ordinal, considering 01-Jan-0001 as day 1. */
static int
ymd_to_ord(int year, int month, int day)
{
    return days_before_year(year) + days_before_month(year, month) + day;
}

/* Day of week, where Monday==0, ..., Sunday==6.  1/1/1 was a Monday. */
static int
weekday(int year, int month, int day)
{
    return (ymd_to_ord(year, month, day) + 6) % 7;
}

/* Ordinal of the Monday starting week 1 of the ISO year.  Week 1 is the
 * first calendar week containing a Thursday.
 */
static int
iso_week1_monday(int year)
{
    int first_day = ymd_to_ord(year, 1, 1);     /* ord of 1/1 */
    /* 0 if 1/1 is a Monday, 1 if a Tue, etc. */
    int first_weekday = (first_day + 6) % 7;
    /* ordinal of closest Monday at or before 1/1 */
    int week1_monday  = first_day - first_weekday;

    if (first_weekday > 3)      /* if 1/1 was Fri, Sat, Sun */
        week1_monday += 7;
    return week1_monday;
}

static int
iso_to_ymd(const int iso_year, const int iso_week, const int iso_day,
           int *year, int *month, int *day) {
    if (iso_week <= 0 || iso_week >= 53) {
        int out_of_range = 1;
        if (iso_week == 53) {
            // ISO years have 53 weeks in it on years starting with a Thursday
            // and on leap years starting on Wednesday
            int first_weekday = weekday(iso_year, 1, 1);
            if (first_weekday == 3 || (first_weekday == 2 && is_leap(iso_year))) {
                out_of_range = 0;
            }
        }

        if (out_of_range) {
            return -2;
        }
    }

    if (iso_day <= 0 || iso_day >= 8) {
        return -3;
    }

    // Convert (Y, W, D) to (Y, M, D) in-place
    int day_1 = iso_week1_monday(iso_year);

    int day_offset = (iso_week - 1)*7 + iso_day - 1;

    ord_to_ymd(day_1 + day_offset, year, month, day);
    return 0;
}

/* ---------------------------------------------------------------------------
 * Range checkers.
 */

#if !PY_VERSION_AT_LEAST_36
/* Check that date arguments are in range.  Return 0 if they are.  If they
 * aren't, raise ValueError and return -1.
 */
static int
check_date_args(int year, int month, int day)
{
    if (year < MINYEAR || year > MAXYEAR) {
        PyErr_Format(PyExc_ValueError, "year %i is out of range", year);
        return -1;
    }
    if (month < 1 || month > 12) {
        PyErr_SetString(PyExc_ValueError, "month must be in 1..12");
        return -1;
    }
    if (day < 1 || day > days_in_month(year, month)) {
        PyErr_SetString(PyExc_ValueError, "day is out of range for month");
        return -1;
    }
    return 0;
}

/* Check that time arguments are in range.  Return 0 if they are.  If they
 * aren't, raise ValueError and return -1.
 */
static int
check_time_args(int h, int m, int s, int us, int fold)
{
    if (h < 0 || h > 23) {
        PyErr_SetString(PyExc_ValueError, "hour must be in 0..23");
        return -1;
    }
    if (m < 0 || m > 59) {
        PyErr_SetString(PyExc_ValueError, "minute must be in 0..59");
        return -1;
    }
    if (s < 0 || s > 59) {
        PyErr_SetString(PyExc_ValueError, "second must be in 0..59");
        return -1;
    }
    if (us < 0 || us > 999999) {
        PyErr_SetString(PyExc_ValueError, "microsecond must be in 0..999999");
        return -1;
    }
    if (fold != 0 && fold != 1) {
        PyErr_SetString(PyExc_ValueError, "fold must be either 0 or 1");
        return -1;
    }
    return 0;
}
#endif

/* ---------------------------------------------------------------------------
 * String parsing utilities and helper functions
 */

static unsigned char
is_digit(const char c) {
    return ((unsigned int)(c - '0')) < 10;
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
parse_isoformat_date(const char *dtstr, const size_t len, int *year, int *month, int *day)
{
    /* Parse the date components of the result of date.isoformat()
     *
     *  Return codes:
     *       0:  Success
     *      -1:  Failed to parse date component
     *      -2:  Inconsistent date separator usage
     *      -3:  Failed to parse ISO week.
     *      -4:  Failed to parse ISO day.
     *      -5, -6: Failure in iso_to_ymd
     */
    const char *p = dtstr;
    p = parse_digits(p, year, 4);
    if (NULL == p) {
        return -1;
    }

    const unsigned char uses_separator = (*p == '-');
    if (uses_separator) {
        ++p;
    }

    if(*p == 'W') {
        // This is an isocalendar-style date string
        p++;
        int iso_week = 0;
        int iso_day = 0;

        p = parse_digits(p, &iso_week, 2);
        if (NULL == p) {
            return -3;
        }

        assert(p > dtstr);
        if ((size_t)(p - dtstr) < len) {
            if (uses_separator && *(p++) != '-') {
                return -2;
            }

            p = parse_digits(p, &iso_day, 1);
            if (NULL == p) {
                return -4;
            }
        } else {
            iso_day = 1;
        }

        int rv = iso_to_ymd(*year, iso_week, iso_day, year, month, day);
        if (rv) {
            return -3 + rv;
        } else {
            return 0;
        }
    }

    p = parse_digits(p, month, 2);
    if (NULL == p) {
        return -1;
    }

    if (uses_separator && *(p++) != '-') {
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
    *hour = *minute = *second = *microsecond = 0;
    const char *p = tstr;
    const char *p_end = tstr_end;
    int *vals[3] = {hour, minute, second};
    size_t i = 0;
    // This is initialized to satisfy an erroneous compiler warning.
    unsigned char has_separator = 1;

    // Parse [HH[:?MM[:?SS]]]
    for (i = 0; i < 3; ++i) {
        p = parse_digits(p, vals[i], 2);
        if (NULL == p) {
            return -3;
        }

        char c = *(p++);
        if (i == 0) {
            has_separator = (c == ':');
        }

        if (p >= p_end) {
            return c != '\0';
        }
        else if (has_separator && (c == ':')) {
            continue;
        }
        else if (c == '.' || c == ',') {
            break;
        } else if (!has_separator) {
            --p;
        } else {
            return -4;  // Malformed time separator
        }
    }

    // Parse fractional components
    size_t len_remains = p_end - p;
    size_t to_parse = len_remains;
    if (len_remains >= 6) {
        to_parse = 6;
    }

    p = parse_digits(p, microsecond, to_parse);
    if (NULL == p) {
        return -3;
    }

    static int correction[] = {
        100000, 10000, 1000, 100, 10
    };

    if (to_parse < 6) {
        *microsecond *= correction[to_parse-1];
    }

    while (is_digit(*p)){
        ++p; // skip truncated digits
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
        if (*tzinfo_pos == 'Z' || *tzinfo_pos == '+' || *tzinfo_pos == '-') {
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

    // Special case UTC / Zulu time.
    if (*tzinfo_pos == 'Z') {
        *tzoffset = 0;
        *tzmicrosecond = 0;

        if (*(tzinfo_pos + 1) != '\0') {
            return -5;
        } else {
            return 1;
        }
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

/* ---------------------------------------------------------------------------
 * tzinfo helpers.
 */

#if !PY_VERSION_AT_LEAST_36
/* Ensure that p is None or of a tzinfo subclass.  Return 0 if OK; if not
 * raise TypeError and return -1.
 */
static int
check_tzinfo_subclass(PyObject *p)
{
    if (p == Py_None || PyTZInfo_Check(p))
        return 0;
    PyErr_Format(PyExc_TypeError,
                 "tzinfo argument must be None or of a tzinfo subclass, "
                 "not type '%s'",
                 Py_TYPE(p)->tp_name);
    return -1;
}
#endif

static inline PyObject *
tzinfo_from_isoformat_results(int rv, int tzoffset, int tz_useconds)
{
    PyObject *tzinfo;
    if (rv == 1) {
        if (abs(tzoffset) >= 86400){
            PyObject *delta;

            delta = PyDelta_FromDSU(0, tzoffset, 0);
            PyErr_Format(PyExc_ValueError, "offset must be a timedelta"
                            " strictly between -timedelta(hours=24) and"
                            " timedelta(hours=24),"
                            " not %R.", delta);
            Py_DECREF(delta);
            return NULL;
        }

        tzinfo = new_fixed_offset(tzoffset);
    }
    else {
        tzinfo = Py_None;
        Py_INCREF(Py_None);
    }

    return tzinfo;
}

/* Return the new date from a string as generated by date.isoformat() */
PyObject *
date_fromisoformat(PyObject *dtstr)
{
    assert(dtstr != NULL);

    if (!PyUnicode_Check(dtstr)) {
        PyErr_SetString(PyExc_TypeError,
                        "fromisoformat: argument must be str");
        return NULL;
    }

    Py_ssize_t len;

    const char *dt_ptr = PyUnicode_AsUTF8AndSize(dtstr, &len);
    if (dt_ptr == NULL) {
        goto invalid_string_error;
    }

    int year = 0, month = 0, day = 0;

    int rv;
    if (len == 7 || len == 8 || len == 10) {
        rv = parse_isoformat_date(dt_ptr, len, &year, &month, &day);
    }
    else {
        rv = -1;
    }

    if (rv < 0) {
        goto invalid_string_error;
    }

#if !PY_VERSION_AT_LEAST_36
    /* Python 3.6+ does this validation as part of date's C API
     * constructor. See
     * https://github.com/python/cpython/commit/b67f0967386a9c9041166d2bbe0a421bd81e10bc
     */
    if (check_date_args(year, month, day) < 0) {
        return NULL;
    }
#endif

    return PyDateTimeAPI->Date_FromDate(year, month, day,
                                        PyDateTimeAPI->DateType);
invalid_string_error:
    PyErr_Format(PyExc_ValueError, "Invalid isoformat string: %R", dtstr);
    return NULL;
}

PyObject *
time_fromisoformat(PyObject *tstr)
{
    assert(tstr != NULL);

    if (!PyUnicode_Check(tstr)) {
        PyErr_SetString(PyExc_TypeError,
                        "fromisoformat: argument must be str");
        return NULL;
    }

    Py_ssize_t len;
    const char *p = PyUnicode_AsUTF8AndSize(tstr, &len);

    if (p == NULL) {
        goto invalid_string_error;
    }

    // The spec actually requires that time-only ISO 8601 strings start with
    // T, but the extended format allows this to be omitted as long as there
    // is no ambiguity with date strings.
    if (*p == 'T') {
        ++p;
        len -= 1;
    }

    int hour = 0, minute = 0, second = 0, microsecond = 0;
    int tzoffset, tzimicrosecond = 0;
    int rv = parse_isoformat_time(p, len, &hour, &minute, &second,
                                  &microsecond, &tzoffset, &tzimicrosecond);

    if (rv < 0) {
        goto invalid_string_error;
    }

    PyObject *tzinfo =
        tzinfo_from_isoformat_results(rv, tzoffset, tzimicrosecond);

    if (tzinfo == NULL) {
        return NULL;
    }

#if !PY_VERSION_AT_LEAST_36
    /* Python 3.6+ does this validation as part of time's C API
     * constructor. See
     * https://github.com/python/cpython/commit/b67f0967386a9c9041166d2bbe0a421bd81e10bc
     */
    if (check_time_args(hour, minute, second, microsecond, 0) < 0) {
        return NULL;
    }
    if (check_tzinfo_subclass(tzinfo) < 0) {
        return NULL;
    }
#endif

    PyObject *t = PyDateTimeAPI->Time_FromTime(
        hour, minute, second, microsecond, tzinfo, PyDateTimeAPI->TimeType);

    Py_DECREF(tzinfo);
    return t;

invalid_string_error:
    PyErr_Format(PyExc_ValueError, "Invalid isoformat string: %R", tstr);
    return NULL;
}

PyObject *
_PyUnicode_Copy(PyObject *unicode)
{
    Py_ssize_t length;
    PyObject *copy;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    length = PyUnicode_GET_LENGTH(unicode);
    copy = PyUnicode_New(length, PyUnicode_MAX_CHAR_VALUE(unicode));
    if (!copy)
        return NULL;
    assert(PyUnicode_KIND(copy) == PyUnicode_KIND(unicode));

    memcpy(PyUnicode_DATA(copy), PyUnicode_DATA(unicode),
           length * PyUnicode_KIND(unicode));
    assert(_PyUnicode_CheckConsistency(copy, 1));
    return copy;
}

static PyObject *
_sanitize_isoformat_str(PyObject *dtstr)
{
    Py_ssize_t len = PyUnicode_GetLength(dtstr);
    if (len < 7) {  // All valid ISO 8601 strings are at least 7 characters long
        return NULL;
    }

    // `fromisoformat` allows surrogate characters in exactly one position,
    // the separator; to allow datetime_fromisoformat to make the simplifying
    // assumption that all valid strings can be encoded in UTF-8, this function
    // replaces any surrogate character separators with `T`.
    //
    // The result of this, if not NULL, returns a new reference
    const void* const unicode_data = PyUnicode_DATA(dtstr);
    const int kind = PyUnicode_KIND(dtstr);

    // Depending on the format of the string, the separator can only ever be
    // in positions 7, 8 or 10. We'll check each of these for a surrogate and
    // if we find one, replace it with `T`. If there is more than one surrogate,
    // we don't have to bother sanitizing it, because the function will later
    // fail when we try to encode the string as ASCII.
    static const size_t potential_separators[3] = {7, 8, 10};
    size_t surrogate_separator = 0;
    for(size_t idx = 0;
         idx < sizeof(potential_separators) / sizeof(*potential_separators);
         ++idx) {
        size_t pos = potential_separators[idx];
        if (pos > (size_t)len) {
            break;
        }

        if(Py_UNICODE_IS_SURROGATE(PyUnicode_READ(kind, unicode_data, pos))) {
            surrogate_separator = pos;
            break;
        }
    }

    if (surrogate_separator == 0) {
        Py_INCREF(dtstr);
        return dtstr;
    }

    PyObject *str_out = _PyUnicode_Copy(dtstr);
    if (str_out == NULL) {
        return NULL;
    }

    if (PyUnicode_WriteChar(str_out, surrogate_separator, (Py_UCS4)'T')) {
        Py_DECREF(str_out);
        return NULL;
    }

    return str_out;
}


static Py_ssize_t
_find_isoformat_datetime_separator(const char *dtstr, Py_ssize_t len) {
    // The valid date formats can all be distinguished by characters 4 and 5
    // and further narrowed down by character
    // which tells us where to look for the separator character.
    // Format    |  As-rendered |   Position
    // ---------------------------------------
    // %Y-%m-%d  |  YYYY-MM-DD  |    10
    // %Y%m%d    |  YYYYMMDD    |     8
    // %Y-W%V    |  YYYY-Www    |     8
    // %YW%V     |  YYYYWww     |     7
    // %Y-W%V-%u |  YYYY-Www-d  |    10
    // %YW%V%u   |  YYYYWwwd    |     8
    // %Y-%j     |  YYYY-DDD    |     8
    // %Y%j      |  YYYYDDD     |     7
    //
    // Note that because we allow *any* character for the separator, in the
    // case where character 4 is W, it's not straightforward to determine where
    // the separator is — in the case of YYYY-Www-d, you have actual ambiguity,
    // e.g. 2020-W01-0000 could be YYYY-Www-D0HH or YYYY-Www-HHMM, when the
    // separator character is a number in the former case or a hyphen in the
    // latter case.
    //
    // The case of YYYYWww can be distinguished from YYYYWwwd by tracking ahead
    // to either the end of the string or the first non-numeric character —
    // since the time components all come in pairs YYYYWww#HH can be
    // distinguished from YYYYWwwd#HH by the fact that there will always be an
    // odd number of digits before the first non-digit character in the former
    // case.
    static const char date_separator = '-';
    static const char week_indicator = 'W';

    if (len == 7) {
        return 7;
    }

    if (dtstr[4] == date_separator) {
        // YYYY-???

        if (dtstr[5] == week_indicator) {
            // YYYY-W??

            if (len < 8) {
                return -1;
            }

            if (len > 8 && dtstr[8] == date_separator) {
                // YYYY-Www-D (10) or YYYY-Www-HH (8)
                if (len == 9) { return -1; }
                if (len > 10 && is_digit(dtstr[10])) {
                    // This is as far as we'll try to go to resolve the
                    // ambiguity for the moment — if we have YYYY-Www-##, the
                    // separator is either a hyphen at 8 or a number at 10.
                    //
                    // We'll assume it's a hyphen at 8 because it's way more
                    // likely that someone will use a hyphen as a separator
                    // than a number, but at this point it's really best effort
                    // because this is an extension of the spec anyway.
                    return 8;
                }

                return 10;
            } else {
                // YYYY-Www (8)
                return 8;
            }
        } else {
            // YYYY-MM-DD (10)
            return 10;
        }
    } else {
        // YYYY???
        if (dtstr[4] == week_indicator) {
            // YYYYWww (7) or YYYYWwwd (8)
            size_t idx = 7;
            for (; idx < (size_t)len; ++idx) {
                // Keep going until we run out of digits.
                if (!is_digit(dtstr[idx])) {
                    break;
                }
            }

            if (idx < 9) {
                return idx;
            }

            if (idx % 2 == 0) {
                // If the index of the last number is even, it's YYYYWww
                return 7;
            } else {
                return 8;
            }
        } else {
            // YYYYMMDD (8)
            return 8;
        }
    }
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

    // We only need to sanitize this string if the separator is a surrogate
    // character. In the situation where the separator location is ambiguous,
    // we don't have to sanitize it anything because that can only happen when
    // the separator is either '-' or a number. This should mostly be a noop
    // but it makes the reference counting easier if we still sanitize.
    PyObject *dtstr_clean = _sanitize_isoformat_str(dtstr);
    if (dtstr_clean == NULL) {
        goto invalid_string_error;
    }

    Py_ssize_t len;
    const char *dt_ptr = PyUnicode_AsUTF8AndSize(dtstr_clean, &len);

    if (dt_ptr == NULL) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeEncodeError)) {
            // Encoding errors are invalid string errors at this point
            goto invalid_string_error;
        }
        else {
            goto error;
        }
    }

    const Py_ssize_t separator_location = _find_isoformat_datetime_separator(
            dt_ptr, len);


    const char *p = dt_ptr;

    int year = 0, month = 0, day = 0;
    int hour = 0, minute = 0, second = 0, microsecond = 0;
    int tzoffset = 0, tzusec = 0;

    // date runs up to separator_location
    int rv = parse_isoformat_date(p, separator_location, &year, &month, &day);

    if (!rv && len > separator_location) {
        // In UTF-8, the length of multi-byte characters is encoded in the MSB
        p += separator_location;
        if ((p[0] & 0x80) == 0) {
            p += 1;
        }
        else {
            switch (p[0] & 0xf0) {
                case 0xe0:
                    p += 3;
                    break;
                case 0xf0:
                    p += 4;
                    break;
                default:
                    p += 2;
                    break;
            }
        }

        len -= (p - dt_ptr);
        rv = parse_isoformat_time(p, len, &hour, &minute, &second,
                                  &microsecond, &tzoffset, &tzusec);
    }
    if (rv < 0) {
        goto invalid_string_error;
    }

    PyObject *tzinfo = tzinfo_from_isoformat_results(rv, tzoffset, tzusec);
    if (tzinfo == NULL) {
        goto error;
    }

#if !PY_VERSION_AT_LEAST_36
    /* Python 3.6+ does this validation as part of datetime's C API
     * constructor. See
     * https://github.com/python/cpython/commit/b67f0967386a9c9041166d2bbe0a421bd81e10bc
     */
    if (check_date_args(year, month, day) < 0) {
        return NULL;
    }
    if (check_time_args(hour, minute, second, microsecond, 0) < 0) {
        return NULL;
    }
    if (check_tzinfo_subclass(tzinfo) < 0) {
        return NULL;
    }
#endif

    PyObject *dt = PyDateTimeAPI->DateTime_FromDateAndTime(
        year, month, day, hour, minute, second, microsecond, tzinfo,
        PyDateTimeAPI->DateTimeType);

    Py_DECREF(tzinfo);
    Py_DECREF(dtstr_clean);
    return dt;

invalid_string_error:
    PyErr_Format(PyExc_ValueError, "Invalid isoformat string: %R", dtstr);

error:
    Py_XDECREF(dtstr_clean);

    return NULL;
}

void
initialize_datetime_code(void)
{
    PyDateTime_IMPORT;
}

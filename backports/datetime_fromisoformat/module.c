#include <Python.h>
#include <datetime.h>

#include "_datetimemodule.h"
#include "timezone.h"

static PyObject *
fromisoformat_date(PyObject *self, PyObject *dtstr)
{
    PyObject *obj;
    obj = date_fromisoformat(dtstr);
    return obj;
}

static PyObject *
fromisoformat_time(PyObject *self, PyObject *dtstr)
{
    PyObject *obj;
    obj = time_fromisoformat(dtstr);
    return obj;
}

static PyObject *
fromisoformat_datetime(PyObject *self, PyObject *dtstr)
{
    PyObject *obj;
    obj = datetime_fromisoformat(dtstr);
    return obj;
}

static PyMethodDef FromISOFormatMethods[] = {
    {"date_fromisoformat", fromisoformat_date, METH_O,
     "Return a date corresponding to a date_string in one of the formats "
     "emitted by date.isoformat()"},
    {"time_fromisoformat", fromisoformat_time, METH_O,
     "Return a time corresponding to a date_string in one of the formats "
     "emitted by time.isoformat()"},
    {"datetime_fromisoformat", fromisoformat_datetime, METH_O,
     "Return a datetime corresponding to a date_string in one of the formats "
     "emitted by datetime.isoformat()"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_datetime_fromisoformat",
    NULL,
    -1,
    FromISOFormatMethods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit__datetime_fromisoformat(void)
{
    PyObject *module = PyModule_Create(&moduledef);
    PyDateTime_IMPORT;

    if (initialize_timezone_code(module) < 0)
        return NULL;

    initialize_datetime_code();
    return module;
}

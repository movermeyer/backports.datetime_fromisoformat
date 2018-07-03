#include <Python.h>
#include <datetime.h>

#include "_datetimemodule.h"
#include "timezone.h"

static PyObject *
fromisoformat(PyObject *self, PyObject *args)
{
    PyObject *obj;
    obj = datetime_fromisoformat(PyTuple_GetItem(args, 0));
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
    "_datetime_fromisoformat",
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
PyInit__datetime_fromisoformat(void)
#else
init_datetime_fromisoformat(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    (void)Py_InitModule("_datetime_fromisoformat", FromISOFormatMethods);
#endif
    PyDateTime_IMPORT;

#if PY_MAJOR_VERSION >= 3
    if (initialize_timezone_code() < 0)
        return NULL;
#else
    initialize_timezone_code();
#endif

    initialize_datetime_code();

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

#include <Python.h>

PyObject *
date_fromisoformat(PyObject *dtstr);

PyObject *
time_fromisoformat(PyObject *tstr);

PyObject *
datetime_fromisoformat(PyObject *dtstr);

void
initialize_datetime_code(void);
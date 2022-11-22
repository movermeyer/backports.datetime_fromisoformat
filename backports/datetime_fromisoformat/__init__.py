import os
import sys

from backports._datetime_fromisoformat import date_fromisoformat, datetime_fromisoformat, time_fromisoformat, FixedOffset


class MonkeyPatch(object):
    @staticmethod
    def patch_fromisoformat():
        import ctypes as c

        def flush_mro_cache():
            # From https://stackoverflow.com/questions/24497316/set-a-read-only-attribute-in-python/24498525#24498525

            if os.name == "nt":
                pythonapi = c.PyDLL("python dll", None, sys.dllhandle)
            elif sys.platform == "cygwin":
                pythonapi = c.PyDLL("libpython%d.%d.dll" % sys.version_info[:2])
            else:
                pythonapi = c.PyDLL(None)

            pythonapi.PyType_Modified(c.py_object(object))

        _get_dict = c.pythonapi._PyObject_GetDictPtr
        _get_dict.restype = c.POINTER(c.py_object)
        _get_dict.argtypes = [c.py_object]

        from datetime import date, datetime, time

        if sys.version_info.major >= 3 and sys.version_info < (3, 11):
            d = _get_dict(datetime)[0]
            d['fromisoformat'] = datetime_fromisoformat

            d = _get_dict(date)[0]
            d['fromisoformat'] = date_fromisoformat

            d = _get_dict(time)[0]
            d['fromisoformat'] = time_fromisoformat

            flush_mro_cache()

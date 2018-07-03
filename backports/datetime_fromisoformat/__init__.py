from backports._datetime_fromisoformat import fromisoformat

class MonkeyPatch(object):
    @staticmethod 
    def patch_fromisoformat():
        import ctypes as c

        def flush_mro_cache():
            # From https://stackoverflow.com/questions/24497316/set-a-read-only-attribute-in-python/24498525#24498525
            c.PyDLL(None).PyType_Modified(c.py_object(object))

        _get_dict = c.pythonapi._PyObject_GetDictPtr
        _get_dict.restype = c.POINTER(c.py_object)
        _get_dict.argtypes = [c.py_object]

        from datetime import datetime

        try:
            _ = datetime.fromisoformat
        except AttributeError:
            d = _get_dict(datetime)[0]
            d['fromisoformat'] = fromisoformat

        flush_mro_cache()

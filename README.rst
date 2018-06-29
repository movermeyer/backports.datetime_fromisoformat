================================
backports.datetime_fromisoformat
================================

A backport of Python 3.7's `datetime.fromisoformat` to earlier version of Python 3. 
Tested against Python 3.4, 3.5 and 3.6.

Current Status
--------------

`backports.datetime_fromisoformat` is under active development, and should be considered pre-alpha.

The interface is still being worked out. Some open questions include:

* What should usage look like?
    * ex. Should we monkey-patch `datetime` objects?
* What should we do when this package is installed on Python 3.7+?

If you have thoughts on how this ought to be done, come chat with us in [the Issues](https://github.com/movermeyer/backports.datetime_fromisoformat/issues).

Quick Start
-----------

As mentioned, the interface is not worked out yet. However, this is how you use it in its current state:

```python
>>> from backports.datetime_fromisoformat import fromisoformat
>>> fromisoformat("2014-01-09T21:48:00-05:30")
datetime.datetime(2014, 1, 9, 21, 48, tzinfo=-05:30)
```

Explanation
-----------
In Python 3.7, [`datetime.fromisoformat`](https://docs.python.org/3/library/datetime.html#datetime.datetime.fromisoformat) was added. It is the inverse of the [`datetime.isoformat`](https://docs.python.org/3/library/datetime.html#datetime.date.isoformat) method.
For those who need to support earlier versions of Python, a backport was needed. 

`backports.datetime_fromisoformat` is a C implementation of `fromisoformat` based on the upstream cPython 3.7 code.
For timezone objects, it uses the a custom `timezone` C implementation (originally from [Pendulum](https://pendulum.eustace.io/)).

Goal / Project Scope
--------------------

The purpose of this project is to provide a perfect backport of `datetime.fromisoformat` and `date.fromisoformat` to earlier versions of Python, while still providing comparable performance.
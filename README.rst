================================
backports.datetime_fromisoformat
================================

.. image:: https://circleci.com/gh/movermeyer/backports.datetime_fromisoformat.svg?style=svg
    :target: https://circleci.com/gh/movermeyer/backports.datetime_fromisoformat

A backport of Python 3.7's ``datetime.fromisoformat`` methods to earlier versions of Python 3. 
Tested against Python 3.4, 3.5 and 3.6.

Current Status
--------------

``backports.datetime_fromisoformat`` is under active development, and should be considered pre-alpha.

Quick Start
-----------

.. code:: python

  >>> from datetime import date, datetime, time
  >>> from backports.datetime_fromisoformat import MonkeyPatch
  >>> MonkeyPatch.patch_fromisoformat()

  >>> datetime.fromisoformat("2014-01-09T21:48:00-05:30")
  datetime.datetime(2014, 1, 9, 21, 48, tzinfo=-05:30)

  >>> date.fromisoformat("2014-01-09")
  datetime.date(2014, 1, 9)

  >>> time.fromisoformat("21:48:00-05:30")
  datetime.time(21, 48, tzinfo=-05:30)

Explanation
-----------
In Python 3.7, `datetime.fromisoformat`_ was added. It is the inverse of `datetime.isoformat`_.
Similar methods were added to the ``date`` and ``time`` types as well. 
For those who need to support earlier versions of Python, a backport of these methods was needed.

.. _`datetime.fromisoformat`: https://docs.python.org/3/library/datetime.html#datetime.datetime.fromisoformat

.. _`datetime.isoformat`: https://docs.python.org/3/library/datetime.html#datetime.date.isoformat

``backports.datetime_fromisoformat`` is a C implementation of ``fromisoformat`` based on the upstream cPython 3.7 code.
For timezone objects, it uses the a custom ``timezone`` C implementation (originally from `Pendulum`_).

.. _`Pendulum`: https://pendulum.eustace.io/

Usage in Python 3.7+
--------------------

NOTE: in Python 3.7 and later, the ``fromisoformat`` methods exist in the stdlib, and installing this package has NO EFFECT.

Goal / Project Scope
--------------------

The purpose of this project is to provide a perfect backport of the ``fromisoformat`` methods to earlier versions of Python, while still providing comparable performance.

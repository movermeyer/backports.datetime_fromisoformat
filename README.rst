================================
backports.datetime_fromisoformat
================================

.. image:: https://github.com/movermeyer/backports.datetime_fromisoformat/workflows/Tests/badge.svg
    :target: https://github.com/movermeyer/backports.datetime_fromisoformat/workflows/Tests

A backport of Python 3.11's ``datetime.fromisoformat`` methods to earlier versions of Python 3.
Tested against Python 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 3.10 and 3.11

Current Status
--------------

Development of ``backports.datetime_fromisoformat`` is "complete". Outside of potential minor bug fixes, do not expect new development here.

Quick Start
-----------

**Installation:**

.. code:: bash

  pip install backports-datetime-fromisoformat

**Usage:**

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

In Python 3.11, `datetime.fromisoformat`_ was extended to cover (almost) all of the ISO 8601 specification, making it generally useful.

For those who need to support earlier versions of Python, a backport of these methods was needed.

.. _`datetime.fromisoformat`: https://docs.python.org/3/library/datetime.html#datetime.datetime.fromisoformat

.. _`datetime.isoformat`: https://docs.python.org/3/library/datetime.html#datetime.date.isoformat

``backports.datetime_fromisoformat`` is a C implementation of ``fromisoformat`` based on the upstream cPython 3.11 code.
For timezone objects, it uses a custom ``timezone`` C implementation (originally from `Pendulum`_).

.. _`Pendulum`: https://pendulum.eustace.io/

Usage in Python 3.11+
---------------------

NOTE: in Python 3.11 and later, compatible versions of ``fromisoformat`` methods exist in the stdlib, and installing this package has NO EFFECT.

Goal / Project Scope
--------------------

The purpose of this project is to provide a perfect backport of the ``fromisoformat`` methods to earlier versions of Python, while still providing comparable performance.

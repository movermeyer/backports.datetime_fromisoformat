import os

from setuptools import setup, Extension
# workaround for open() with encoding='' python2/3 compability
from io import open

with open('README.rst', encoding='utf-8') as file:
    long_description = file.read()

VERSION = "1.0.0"

setup(
    name="backports-datetime-fromisoformat",
    version=VERSION,
    description="Backport of Python 3.7's datetime.fromisoformat",
    long_description=long_description,
    license="MIT",
    author="Michael Overmeyer",
    author_email="backports@movermeyer.com",
    url="https://github.com/movermeyer/backports.datetime_fromisoformat",

    packages=["backports", "backports.datetime_fromisoformat"],

    ext_modules=[Extension("backports._datetime_fromisoformat", [
        os.path.join("backports", "datetime_fromisoformat", "module.c"),
        os.path.join("backports", "datetime_fromisoformat", "_datetimemodule.c"),
        os.path.join("backports", "datetime_fromisoformat", "timezone.c")
    ])],

    test_suite='tests',
    tests_require=[
        'pytz'
    ],
    classifiers=[
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ]
)

import os

from setuptools import setup, Extension
# workaround for open() with encoding='' python2/3 compability
from io import open

with open('README.rst', encoding='utf-8') as file:
    long_description = file.read()

setup(
    name="backports-datetime-fromisoformat",
    version="0.0.1",
    description="Backport of Python 3.7's datetime.fromisoformat",
    long_description=long_description,
    license="MIT",
    ext_package="backports",
    ext_modules=[Extension("datetime_fromisoformat", [
        os.path.join("backports", "datetime_fromisoformat", "module.c"), 
        os.path.join("backports", "datetime_fromisoformat", "timezone.c")
    ])],
    test_suite='tests',
    tests_require=[
        'pytz',
        "unittest2 ; python_version < '3'"
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

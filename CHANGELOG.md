<!-- Generated with "Markdown T​O​C" extension for Visual Studio Code -->
<!-- TOC anchorMode:github.com -->

- [2.x.x](#2xx)
  - [Migration from 1.x.x](#migration-from-1xx)
  - [Version 2.0.1](#version-201)
  - [Version 2.0.0](#version-200)
- [1.x.x](#1xx)
  - [Version 1.0.0](#version-100)
- [0.x.x](#0xx)
  - [Version 0.0.2](#version-002)
  - [Version 0.0.1](#version-001)


# 2.x.x

## Migration from 1.x.x

No migration is needed.

However, starting in version 2, `backports.datetime_fromisoformat` will apply its changes to Python < 3.11, whereas v1 only applied changes to Python < 3.7. If you happened to be using `backports.datetime_fromisoformat` v1 on Python 3.7 through Python 3.10 and then upgrade to v2, it will patch the `fromisoformat` methods, whereas in v1 it did not. The `fromisoformat` methods will be able to parse timestamps from a wider portion of the ISO 8601 specification. This is unlikely to be a problem, but for completeness it is mentioned here.

## Version 2.0.1

* Switched to `pyproject.toml`
* Switched to `pytest` as a test runner
* Added wheel builds

## Version 2.0.0

* Backport the logic from Python 3.11's `fromisoformat` methods, instead of Python 3.7's, [#22](https://github.com/movermeyer/backports.datetime_fromisoformat/pull/22)
  * Adds support for a wider portion of the ISO 8601 specification
  * Backports are now applied to Python < 3.11, instead of only Python < 3.7

# 1.x.x

## Version 1.0.0

* Made FixedOffset objects picklable/copyable (Solves #12, Thanks @jappievw)

# 0.x.x

## Version 0.0.2

* Fixed build issue that impacted certain installations (#10, Thanks @rlyons)

## Version 0.0.1

* Initial release

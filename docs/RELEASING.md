# Release Process

When you think you are ready to make a release of `backports.datetime_fromisoformat` follow this process.

- [Release Process](#release-process)
  - [Update `pyproject.toml` and `CHANGELOG.md`](#update-pyprojecttoml-and-changelogmd)
  - [Create a developmental release](#create-a-developmental-release)
  - [Test the developmental release](#test-the-developmental-release)
  - [What if something went wrong with the developmental release?](#what-if-something-went-wrong-with-the-developmental-release)
  - [Create a GitHub Release](#create-a-github-release)
  - [What if something went wrong with the final release?](#what-if-something-went-wrong-with-the-final-release)

## Update `pyproject.toml` and `CHANGELOG.md`

Modify the `project.version` key in `pyproject.toml` to be the version you want to release.
Make sure that there is a corresponding entry in `CHANGELOG.md`

## Create a developmental release

The first step to *any* release is to excercise our build pipeline to make sure that our systems are still working as expected.
(We release so infrequently, that there is often something that has broken due to "bit rot". You know how it is.)

This is done with a ["developmental release"](https://peps.python.org/pep-0440) that executes the entire publishing process, including being uploaded to the [Test PyPI server](https://test.pypi.org/).

**Note:** Developmental releases are the first step to doing *any* sort of public release.  Developmental releases are private, used purely for internal testing of our CI systems. **They aren't for external users at all!**
Public "beta"/"pre-release" releases for external users use this same process as "final"/"production" releases: Start by creating a developmental release to make sure things are still working.

Steps to creating a developmental release:

1. Manually trigger the [`publish` GitHub Action workflow](.github/workflows/publish.yml) with the developmental release version number
2. Verify that the workflow completes successfully

If everything went well, your developmental release will be present in the [Test PyPI server](https://test.pypi.org/project/backports-datetime-fromisoformat/#history)

## Test the developmental release

```
TODO: How to download a version from the Test PyPI server
```

TODO: What things to test.

## What if something went wrong with the developmental release?

🎉 This is why we have developmental releases! 🎉

1. Debug and correct the failure of the [`publish` GitHub Action workflow](https://github.com/movermeyer/backports.datetime_fromisoformat/actions/workflows/publish.yml)
2. Try again by manually triggering the [`publish` GitHub Action workflow](https://github.com/movermeyer/backports.datetime_fromisoformat/actions/workflows/publish.yml)

Only continue once you have a successful developmental release.

## Create a GitHub Release

Once you have sucessfully uploaded and tested a developmental release, it's time for the real thing!

1. Create a new [GitHub release](https://github.com/movermeyer/backports.datetime_fromisoformat/releases/new)
2. Click "Publish Release"
3. Verify that the [`publish` GitHub Action workflow](.github/workflows/publish.yml) completes successfully

## What if something went wrong with the final release?

1. Debug and correct the problem.
2. Publish a new release with an incremented version number

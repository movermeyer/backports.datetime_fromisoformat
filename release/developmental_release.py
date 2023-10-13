import argparse
import json
import sys

from packaging.version import parse, Version
from typing import Iterator

def _releases(contents) -> Iterator[Version]:
    parsed = json.loads(contents)
    existing_releases = [parse(x) for x in parsed["releases"].keys()]
    return existing_releases

def non_developmental_version(version: Version):
    if not version.is_devrelease:
        return version

    epoch = f"{version.epoch}!" if version.epoch else ""
    release = ".".join([str(x) for x in version.release])
    pre = f"{version.pre[0]}{version.pre[1]}" if version.pre else ""
    post = f".post{version.post}" if version.post else ""
    return parse(f"{epoch}{release}{pre}{post}")

def existing_developmental_release_version(new: Version, existing_releases: Iterator[Version]) -> Version:
    return max([x for x in existing_releases if x.is_devrelease and non_developmental_version(x) == new], default=None)

def new_developmental_release_version(new: Version, existing_releases: Iterator[Version]) -> Version:
    existing = existing_developmental_release_version(new, existing_releases)
    dev_number = existing.dev + 1 if existing is not None else 1

    epoch = f"{new.epoch}!" if new.epoch else ""
    release = ".".join([str(x) for x in new.release])
    pre = f"{new.pre[0]}{new.pre[1]}" if new.pre else ""
    post = f".post{new.post}" if new.post else ""
    dev = f".dev{dev_number}"
    return parse(f"{epoch}{release}{pre}{post}{dev}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generates the next developmental release version number based on existing releases'
    )

    parser.add_argument('new_version', help='The new version number you want to use (without any `.dev` suffix)')
    args = parser.parse_args()

    contents = sys.stdin.read()
    existing_releases = _releases(contents)

    existing = existing_developmental_release_version(parse(args.new_version), existing_releases) or "0.0.0"
    print(existing)

    dev_version = new_developmental_release_version(parse(args.new_version), existing_releases)
    print(dev_version)

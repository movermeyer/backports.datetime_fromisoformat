import itertools
import unittest

from packaging.version import parse, Version
from typing import Iterator

from developmental_release import new_developmental_release_version

def generate_versions() -> Iterator[Version]:
    # https://peps.python.org/pep-0440/
    epoch_options = ["", "1!"]
    release_options = ['2', '2.0', '2.0.0', '2.0.0.0', '2.0.0.0.0']
    pre_release_options = ["", 'a1', 'b1', 'rc1']
    post_release_options = ["", ".post1"]
    dev_options = ["", ".dev1"]
    for v in (parse(f"{epoch}{release}{pre_release}{post_release}{dev}") for (epoch, release, pre_release, post_release, dev) in itertools.product(epoch_options, release_options, pre_release_options, post_release_options, dev_options)):
        yield v

class TestDevelopmentalVersionGeneration(unittest.TestCase):
    def test_no_releases(self):
        for version in generate_versions():
            if version.is_devrelease:
                continue
            new_dev_version = new_developmental_release_version(version, [])
            self.assertRegex(f"{new_dev_version}", r"\.dev1$", f"Input: {version}, Output: {new_dev_version}")

    def test_existing_dev_release(self):
        for version in generate_versions():
            if version.is_devrelease:
                continue
            new_dev_version = new_developmental_release_version(version, [parse(f"{version}.dev1")])
            self.assertRegex(f"{new_dev_version}", r"\.dev2$", f"Input: {version}, Output: {new_dev_version}")

    def test_dev_release_exists_for_more_specific_version(self):
        version = parse("2.0.0")
        new_dev_version = new_developmental_release_version(version, [parse("2.0.0b1.dev1")])
        self.assertRegex(f"{new_dev_version}", r"\.dev1$", f"Input: {version}, Output: {new_dev_version}")

if __name__ == '__main__':
    unittest.main()

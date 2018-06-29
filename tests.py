import unittest

from backports.datetime_fromisoformat import fromisoformat
from datetime import datetime


class TestFromIsoFormat(unittest.TestCase):
    def test_basic_naive_parse(self):
        expected = datetime(2014, 2, 5, 23, 45)
        self.assertEqual(expected, fromisoformat(expected.isoformat()))


if __name__ == '__main__':
    unittest.main()

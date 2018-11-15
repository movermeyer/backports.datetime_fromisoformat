import copy
import pickle
import pytz
import sys
import unittest

from datetime import date, datetime, time, timedelta

from backports.datetime_fromisoformat import MonkeyPatch
MonkeyPatch.patch_fromisoformat()


class TestFromIsoFormat(unittest.TestCase):
    def test_basic_naive_parse(self):
        expected = datetime(2014, 2, 5, 23, 45)
        self.assertEqual(expected, datetime.fromisoformat(expected.isoformat()))


class TestsFromCPython(unittest.TestCase):
    # These test cases are taken from cPython's `Lib/test/datetimetester.py`

    def test_fromisoformat(self):
        # Test that isoformat() is reversible
        base_dates = [
            (1, 1, 1),
            (1000, 2, 14),
            (1900, 1, 1),
            (2000, 2, 29),
            (2004, 11, 12),
            (2004, 4, 3),
            (2017, 5, 30)
        ]

        for date_cls in [datetime, date]:
            for dt_tuple in base_dates:
                dt = date_cls(*dt_tuple)
                dt_str = dt.isoformat()
                with self.subTest(dt_str=dt_str):
                    dt_rt = date_cls.fromisoformat(dt.isoformat())

                    self.assertEqual(dt, dt_rt)
                    self.assertIsInstance(dt_rt, date_cls)

    def test_fromisoformat_fails(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                 # Empty string
            '009-03-04',        # Not 10 characters
            '123456789',        # Not a date
            '200a-12-04',       # Invalid character in year
            '2009-1a-04',       # Invalid character in month
            '2009-12-0a',       # Invalid character in day
            '2009-01-32',       # Invalid day
            '2009-02-29',       # Invalid leap day
            '20090228',         # Valid ISO8601 output not from isoformat()
        ]

        for bad_str in bad_strs:
            with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                datetime.fromisoformat(bad_str)

            with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                date.fromisoformat(bad_str)

    def test_fromisoformat_fails_typeerror(self):
        # Test that fromisoformat fails when passed the wrong type
        import io

        bad_types = [b'2009-03-01', None, io.StringIO('2009-03-01')]
        for bad_type in bad_types:
            with self.assertRaises(TypeError, msg="Did not fail on '{0}'".format(bad_type)):
                datetime.fromisoformat(bad_type)

            with self.assertRaises(TypeError, msg="Did not fail on '{0}'".format(bad_type)):
                date.fromisoformat(bad_type)

    def test_fromisoformat_datetime(self):
        # Test that isoformat() is reversible
        base_dates = [
            (1, 1, 1),
            (1900, 1, 1),
            (2004, 11, 12),
            (2017, 5, 30)
        ]

        base_times = [
            (0, 0, 0, 0),
            (0, 0, 0, 241000),
            (0, 0, 0, 234567),
            (12, 30, 45, 234567)
        ]

        separators = [' ', 'T']

        tzinfos = [None, pytz.utc,
                   pytz.FixedOffset(-5 * 60),
                   pytz.FixedOffset(2 * 60)]

        dts = [datetime(*(date_tuple + time_tuple), tzinfo=tzi)
               for date_tuple in base_dates
               for time_tuple in base_times
               for tzi in tzinfos]

        for dt in dts:
            for sep in separators:
                dtstr = dt.isoformat(sep=sep)

                with self.subTest(dtstr=dtstr):
                    dt_rt = datetime.fromisoformat(dtstr)
                    self.assertEqual(dt, dt_rt)

    def test_fromisoformat_timezone(self):
        base_dt = datetime(2014, 12, 30, 12, 30, 45, 217456)

        tzoffsets = [
            timedelta(hours=5),
            timedelta(hours=2),
            timedelta(hours=6, minutes=27),

            # Our timezone implementation doesn't handle sub-minute offsets.
            # timedelta(hours=12, minutes=32, seconds=30),
            # timedelta(hours=2, minutes=4, seconds=9, microseconds=123456)
        ]

        tzoffsets += [-1 * td for td in tzoffsets]

        tzinfos = [None, pytz.utc,
                   pytz.FixedOffset(0)]

        tzinfos += [pytz.FixedOffset(td.total_seconds() / 60) for td in tzoffsets]

        for tzi in tzinfos:
            dt = base_dt.replace(tzinfo=tzi)
            dtstr = dt.isoformat()

            with self.subTest(tstr=dtstr):
                dt_rt = datetime.fromisoformat(dtstr)
                assert dt == dt_rt, dt_rt

    def test_fromisoformat_separators(self):
        separators = [
            ' ', 'T', '\u007f',     # 1-bit widths
            '\u0080', 'ʁ',          # 2-bit widths
            'ᛇ', '時',               # 3-bit widths
            '🐍'                     # 4-bit widths
        ]

        for sep in separators:
            dt = datetime(2018, 1, 31, 23, 59, 47, 124789)
            dtstr = dt.isoformat(sep=sep)

            with self.subTest(dtstr=dtstr):
                dt_rt = datetime.fromisoformat(dtstr)
                self.assertEqual(dt, dt_rt)

    def test_fromisoformat_ambiguous(self):
        # Test strings like 2018-01-31+12:15 (where +12:15 is not a time zone)
        separators = ['+', '-']
        for sep in separators:
            dt = datetime(2018, 1, 31, 12, 15)
            dtstr = dt.isoformat(sep=sep)

            with self.subTest(dtstr=dtstr):
                dt_rt = datetime.fromisoformat(dtstr)
                self.assertEqual(dt, dt_rt)

    def test_fromisoformat_timespecs(self):
        if sys.version_info >= (3, 6):
            datetime_bases = [
                (2009, 12, 4, 8, 17, 45, 123456),
                (2009, 12, 4, 8, 17, 45, 0)]

            tzinfos = [None, pytz.utc,
                       pytz.FixedOffset(-5 * 60),
                       pytz.FixedOffset(2 * 60),
                       pytz.FixedOffset(6 * 60 + 27)]

            timespecs = ['hours', 'minutes', 'seconds', 'milliseconds', 'microseconds']

            for ip, ts in enumerate(timespecs):
                for tzi in tzinfos:
                    for dt_tuple in datetime_bases:
                        if ts == 'milliseconds':
                            new_microseconds = 1000 * (dt_tuple[6] // 1000)
                            dt_tuple = dt_tuple[0:6] + (new_microseconds,)

                        dt = datetime(*(dt_tuple[0:(4 + ip)]), tzinfo=tzi)
                        dtstr = dt.isoformat(timespec=ts)
                        with self.subTest(dtstr=dtstr):
                            dt_rt = datetime.fromisoformat(dtstr)
                            self.assertEqual(dt, dt_rt)

    def test_fromisoformat_fails_datetime(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                             # Empty string
            '2009.04-19T03',                # Wrong first separator
            '2009-04.19T03',                # Wrong second separator
            '2009-04-19T0a',                # Invalid hours
            '2009-04-19T03:1a:45',          # Invalid minutes
            '2009-04-19T03:15:4a',          # Invalid seconds
            '2009-04-19T03;15:45',          # Bad first time separator
            '2009-04-19T03:15;45',          # Bad second time separator
            '2009-04-19T03:15:4500:00',     # Bad time zone separator
            '2009-04-19T03:15:45.2345',     # Too many digits for milliseconds
            '2009-04-19T03:15:45.1234567',  # Too many digits for microseconds

            # Our timezone implementation doesn't mind > 24h offsets
            # '2009-04-19T03:15:45.123456+24:30',    # Invalid time zone offset
            # '2009-04-19T03:15:45.123456-24:30',    # Invalid negative offset

            '2009-04-10ᛇᛇᛇᛇᛇ12:15',         # Too many unicode separators
            '2009-04-19T1',                 # Incomplete hours
            '2009-04-19T12:3',              # Incomplete minutes
            '2009-04-19T12:30:4',           # Incomplete seconds
            '2009-04-19T12:',               # Ends with time separator
            '2009-04-19T12:30:',            # Ends with time separator
            '2009-04-19T12:30:45.',         # Ends with time separator
            '2009-04-19T12:30:45.123456+',  # Ends with timzone separator
            '2009-04-19T12:30:45.123456-',  # Ends with timzone separator
            '2009-04-19T12:30:45.123456-05:00a',    # Extra text
            '2009-04-19T12:30:45.123-05:00a',       # Extra text
            '2009-04-19T12:30:45-05:00a',           # Extra text
        ]

        for bad_str in bad_strs:
            with self.subTest(bad_str=bad_str):
                with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                    datetime.fromisoformat(bad_str)

    # Our timezone implementation doesn't have a concept of UTC being special
    # def test_fromisoformat_utc(self):
    #     dt_str = '2014-04-19T13:21:13+00:00'
    #     dt = datetime.fromisoformat(dt_str)
    #     self.assertIs(dt.tzinfo, pytz.utc)

    def test_time_fromisoformat(self):
        tsc = time(12, 14, 45, 203745, tzinfo=pytz.utc)
        tsc_rt = time.fromisoformat(tsc.isoformat())

        self.assertEqual(tsc, tsc_rt)
        self.assertIsInstance(tsc_rt, time)


class TestCopy(unittest.TestCase):
    def test_basic_pickle_and_copy(self):
        dt = datetime.fromisoformat('2018-11-01 20:42:09.058000')
        dt2 = pickle.loads(pickle.dumps(dt))
        self.assertEqual(dt, dt2)
        dt3 = copy.deepcopy(dt)
        self.assertEqual(dt, dt3)

        # FixedOffset
        dt = datetime.fromisoformat('2018-11-01 20:42:09.058000+01:30')
        dt2 = pickle.loads(pickle.dumps(dt))
        self.assertEqual(dt, dt2)
        dt3 = copy.deepcopy(dt)
        self.assertEqual(dt, dt3)


if __name__ == '__main__':
    unittest.main()

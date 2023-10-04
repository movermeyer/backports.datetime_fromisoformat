import copy
import io
import itertools
import pickle
import pytz
import re
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

                    self.assertEqual(dt, dt_rt)
                    self.assertIsInstance(dt_rt, date_cls)

    def test_fromisoformat_examples(self):
        examples = [
            ('00010101', (1, 1, 1)),
            ('20000101', (2000, 1, 1)),
            ('20250102', (2025, 1, 2)),
            ('99991231', (9999, 12, 31)),
            ('0001-01-01', (1, 1, 1)),
            ('2000-01-01', (2000, 1, 1)),
            ('2025-01-02', (2025, 1, 2)),
            ('9999-12-31', (9999, 12, 31)),
            ('2025W01', (2024, 12, 30)),
            ('2025-W01', (2024, 12, 30)),
            ('2025W014', (2025, 1, 2)),
            ('2025-W01-4', (2025, 1, 2)),
            ('2026W01', (2025, 12, 29)),
            ('2026-W01', (2025, 12, 29)),
            ('2026W013', (2025, 12, 31)),
            ('2026-W01-3', (2025, 12, 31)),
            ('2022W52', (2022, 12, 26)),
            ('2022-W52', (2022, 12, 26)),
            ('2022W527', (2023, 1, 1)),
            ('2022-W52-7', (2023, 1, 1)),
            ('2015W534', (2015, 12, 31)),      # Has week 53
            ('2015-W53-4', (2015, 12, 31)),    # Has week 53
            ('2015-W53-5', (2016, 1, 1)),
            ('2020W531', (2020, 12, 28)),      # Leap year
            ('2020-W53-1', (2020, 12, 28)),    # Leap year
            ('2020-W53-6', (2021, 1, 2)),
        ]

        for date_cls in [datetime, date]:
            for input_str, expected_values in examples:
                expected = date_cls(*expected_values)
                with self.subTest(input_str=input_str):
                    actual = date_cls.fromisoformat(input_str)
                    self.assertEqual(actual, expected)

    def test_fromisoformat_fails(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                 # Empty string
            '\ud800',           # bpo-34454: Surrogate code point
            '009-03-04',        # Not 10 characters
            '123456789',        # Not a date
            '200a-12-04',       # Invalid character in year
            '2009-1a-04',       # Invalid character in month
            '2009-12-0a',       # Invalid character in day
            '2009-01-32',       # Invalid day
            '2009-02-29',       # Invalid leap day
            '2019-W53-1',       # No week 53 in 2019
            '2020-W54-1',       # No week 54
            '2009\ud80002\ud80028',     # Separators are surrogate codepoints
        ]

        for bad_str in bad_strs:
            with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                datetime.fromisoformat(bad_str)

            with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                date.fromisoformat(bad_str)

    def test_fromisoformat_fails_typeerror(self):
        # Test that fromisoformat fails when passed the wrong type
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

    def test_datetime_fromisoformat_timezone(self):
        base_dt = datetime(2014, 12, 30, 12, 30, 45, 217456)

        tzoffsets = [
            timedelta(hours=5), timedelta(hours=2),
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
            '🐍',                    # 4-bit widths
            '\ud800',               # bpo-34454: Surrogate code point
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
        if sys.version_info >= (3, 6): # timespec parameter was added in Python 3.6
            datetime_bases = [
                (2009, 12, 4, 8, 17, 45, 123456),
                (2009, 12, 4, 8, 17, 45, 0)]

            tzinfos = [None, pytz.utc,
                       pytz.FixedOffset(-5 * 60),
                       pytz.FixedOffset(2 * 60),
                       pytz.FixedOffset(6 * 60 + 27)]

            timespecs = ['hours', 'minutes', 'seconds',
                         'milliseconds', 'microseconds']

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

    def test_fromisoformat_datetime_examples(self):
        UTC = pytz.utc
        BST = pytz.FixedOffset(1 * 60)
        EST = pytz.FixedOffset(-5 * 60)
        EDT = pytz.FixedOffset(-4 * 60)
        examples = [
            ('2025-01-02', datetime(2025, 1, 2, 0, 0)),
            ('2025-01-02T03', datetime(2025, 1, 2, 3, 0)),
            ('2025-01-02T03:04', datetime(2025, 1, 2, 3, 4)),
            ('2025-01-02T0304', datetime(2025, 1, 2, 3, 4)),
            ('2025-01-02T03:04:05', datetime(2025, 1, 2, 3, 4, 5)),
            ('2025-01-02T030405', datetime(2025, 1, 2, 3, 4, 5)),
            ('2025-01-02T03:04:05.6',
             datetime(2025, 1, 2, 3, 4, 5, 600000)),
            ('2025-01-02T03:04:05,6',
             datetime(2025, 1, 2, 3, 4, 5, 600000)),
            ('2025-01-02T03:04:05.678',
             datetime(2025, 1, 2, 3, 4, 5, 678000)),
            ('2025-01-02T03:04:05.678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2025-01-02T03:04:05,678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2025-01-02T030405.678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2025-01-02T030405,678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2025-01-02T03:04:05.6789010',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2009-04-19T03:15:45.2345',
             datetime(2009, 4, 19, 3, 15, 45, 234500)),
            ('2009-04-19T03:15:45.1234567',
             datetime(2009, 4, 19, 3, 15, 45, 123456)),
            ('2025-01-02T03:04:05,678',
             datetime(2025, 1, 2, 3, 4, 5, 678000)),
            ('20250102', datetime(2025, 1, 2, 0, 0)),
            ('20250102T03', datetime(2025, 1, 2, 3, 0)),
            ('20250102T03:04', datetime(2025, 1, 2, 3, 4)),
            ('20250102T03:04:05', datetime(2025, 1, 2, 3, 4, 5)),
            ('20250102T030405', datetime(2025, 1, 2, 3, 4, 5)),
            ('20250102T03:04:05.6',
             datetime(2025, 1, 2, 3, 4, 5, 600000)),
            ('20250102T03:04:05,6',
             datetime(2025, 1, 2, 3, 4, 5, 600000)),
            ('20250102T03:04:05.678',
             datetime(2025, 1, 2, 3, 4, 5, 678000)),
            ('20250102T03:04:05,678',
             datetime(2025, 1, 2, 3, 4, 5, 678000)),
            ('20250102T03:04:05.678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('20250102T030405.678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('20250102T030405,678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('20250102T030405.6789010',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2022W01', datetime(2022, 1, 3)),
            ('2022W52520', datetime(2022, 12, 26, 20, 0)),
            ('2022W527520', datetime(2023, 1, 1, 20, 0)),
            ('2026W01516', datetime(2025, 12, 29, 16, 0)),
            ('2026W013516', datetime(2025, 12, 31, 16, 0)),
            ('2025W01503', datetime(2024, 12, 30, 3, 0)),
            ('2025W014503', datetime(2025, 1, 2, 3, 0)),
            ('2025W01512', datetime(2024, 12, 30, 12, 0)),
            ('2025W014512', datetime(2025, 1, 2, 12, 0)),
            ('2025W014T121431', datetime(2025, 1, 2, 12, 14, 31)),
            ('2026W013T162100', datetime(2025, 12, 31, 16, 21)),
            ('2026W013 162100', datetime(2025, 12, 31, 16, 21)),
            ('2022W527T202159', datetime(2023, 1, 1, 20, 21, 59)),
            ('2022W527 202159', datetime(2023, 1, 1, 20, 21, 59)),
            ('2025W014 121431', datetime(2025, 1, 2, 12, 14, 31)),
            ('2025W014T030405', datetime(2025, 1, 2, 3, 4, 5)),
            ('2025W014 030405', datetime(2025, 1, 2, 3, 4, 5)),
            ('2020-W53-6T03:04:05', datetime(2021, 1, 2, 3, 4, 5)),
            ('2020W537 03:04:05', datetime(2021, 1, 3, 3, 4, 5)),
            ('2025-W01-4T03:04:05', datetime(2025, 1, 2, 3, 4, 5)),
            ('2025-W01-4T03:04:05.678901',
             datetime(2025, 1, 2, 3, 4, 5, 678901)),
            ('2025-W01-4T12:14:31', datetime(2025, 1, 2, 12, 14, 31)),
            ('2025-W01-4T12:14:31.012345',
             datetime(2025, 1, 2, 12, 14, 31, 12345)),
            ('2026-W01-3T16:21:00', datetime(2025, 12, 31, 16, 21)),
            ('2026-W01-3T16:21:00.000000', datetime(2025, 12, 31, 16, 21)),
            ('2022-W52-7T20:21:59',
             datetime(2023, 1, 1, 20, 21, 59)),
            ('2022-W52-7T20:21:59.999999',
             datetime(2023, 1, 1, 20, 21, 59, 999999)),
            ('2025-W01003+00',
             datetime(2024, 12, 30, 3, 0, tzinfo=UTC)),
            ('2025-01-02T03:04:05+00',
             datetime(2025, 1, 2, 3, 4, 5, tzinfo=UTC)),
            ('2025-01-02T03:04:05Z',
             datetime(2025, 1, 2, 3, 4, 5, tzinfo=UTC)),
            ('2025-01-02003:04:05,6+00:00:00.00',
             datetime(2025, 1, 2, 3, 4, 5, 600000, tzinfo=UTC)),
            ('2000-01-01T00+21',
             datetime(2000, 1, 1, 0, 0, tzinfo=pytz.FixedOffset(21 * 60))),
            ('2025-01-02T03:05:06+0300',
             datetime(2025, 1, 2, 3, 5, 6,
                           tzinfo=pytz.FixedOffset(3 * 60))),
            ('2025-01-02T03:05:06-0300',
             datetime(2025, 1, 2, 3, 5, 6,
                           tzinfo=pytz.FixedOffset(-3 * 60))),
            ('2025-01-02T03:04:05+0000',
             datetime(2025, 1, 2, 3, 4, 5, tzinfo=UTC)),
            ('2025-01-02T03:05:06+03',
             datetime(2025, 1, 2, 3, 5, 6,
                           tzinfo=pytz.FixedOffset(3 * 60))),
            ('2025-01-02T03:05:06-03',
             datetime(2025, 1, 2, 3, 5, 6,
                           tzinfo=pytz.FixedOffset(-3 * 60))),
            ('2020-01-01T03:05:07.123457-05:00',
             datetime(2020, 1, 1, 3, 5, 7, 123457, tzinfo=EST)),
            ('2020-01-01T03:05:07.123457-0500',
             datetime(2020, 1, 1, 3, 5, 7, 123457, tzinfo=EST)),
            ('2020-06-01T04:05:06.111111-04:00',
             datetime(2020, 6, 1, 4, 5, 6, 111111, tzinfo=EDT)),
            ('2020-06-01T04:05:06.111111-0400',
             datetime(2020, 6, 1, 4, 5, 6, 111111, tzinfo=EDT)),
            ('2021-10-31T01:30:00.000000+01:00',
             datetime(2021, 10, 31, 1, 30, tzinfo=BST)),
            ('2021-10-31T01:30:00.000000+0100',
             datetime(2021, 10, 31, 1, 30, tzinfo=BST)),
            ('2025-01-02T03:04:05,6+000000.00',
             datetime(2025, 1, 2, 3, 4, 5, 600000, tzinfo=UTC)),
            # Our timezone implementation doesn't handle sub-minute offsets.
            # ('2025-01-02T03:04:05,678+00:00:10',
            #  datetime(2025, 1, 2, 3, 4, 5, 678000,
            #                tzinfo=timezone(timedelta(seconds=10)))),
        ]

        for input_str, expected in examples:
            with self.subTest(input_str=input_str):
                actual = datetime.fromisoformat(input_str)
                self.assertEqual(actual, expected)

    def test_fromisoformat_fails_datetime(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                             # Empty string
            '\ud800',                       # bpo-34454: Surrogate code point
            '2009.04-19T03',                # Wrong first separator
            '2009-04.19T03',                # Wrong second separator
            '2009-04-19T0a',                # Invalid hours
            '2009-04-19T03:1a:45',          # Invalid minutes
            '2009-04-19T03:15:4a',          # Invalid seconds
            '2009-04-19T03;15:45',          # Bad first time separator
            '2009-04-19T03:15;45',          # Bad second time separator
            '2009-04-19T03:15:4500:00',     # Bad time zone separator
            '2009-04-19T03:15:45.123456+24:30',    # Invalid time zone offset
            '2009-04-19T03:15:45.123456-24:30',    # Invalid negative offset
            '2009-04-10ᛇᛇᛇᛇᛇ12:15',         # Too many unicode separators
            '2009-04\ud80010T12:15',        # Surrogate char in date
            '2009-04-10T12\ud80015',        # Surrogate char in time
            '2009-04-19T1',                 # Incomplete hours
            '2009-04-19T12:3',              # Incomplete minutes
            '2009-04-19T12:30:4',           # Incomplete seconds
            '2009-04-19T12:',               # Ends with time separator
            '2009-04-19T12:30:',            # Ends with time separator
            '2009-04-19T12:30:45.',         # Ends with time separator
            '2009-04-19T12:30:45.123456+',  # Ends with timezone separator
            '2009-04-19T12:30:45.123456-',  # Ends with timezone separator
            '2009-04-19T12:30:45.123456-05:00a',    # Extra text
            '2009-04-19T12:30:45.123-05:00a',       # Extra text
            '2009-04-19T12:30:45-05:00a',           # Extra text
        ]

        for bad_str in bad_strs:
            with self.subTest(bad_str=bad_str):
                with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                    datetime.fromisoformat(bad_str)

    def test_datetime_fromisoformat_fails_surrogate(self):
        # Test that when fromisoformat() fails with a surrogate character as
        # the separator, the error message contains the original string
        dtstr = "2018-01-03\ud80001:0113"

        with self.assertRaisesRegex(ValueError, re.escape(repr(dtstr))):
            datetime.fromisoformat(dtstr)

    # Our timezone implementation doesn't have a concept of UTC being special
    # def test_fromisoformat_utc(self):
    #     dt_str = '2014-04-19T13:21:13+00:00'
    #     dt = datetime.fromisoformat(dt_str)
    #     self.assertIs(dt.tzinfo, pytz.utc)

    def test_time_fromisoformat(self):
        time_examples = [
            (0, 0, 0, 0),
            (23, 59, 59, 999999),
        ]

        hh = (9, 12, 20)
        mm = (5, 30)
        ss = (4, 45)
        usec = (0, 245000, 678901)

        time_examples += list(itertools.product(hh, mm, ss, usec))

        tzinfos = [None, pytz.utc,
                       pytz.FixedOffset(2 * 60),
                       pytz.FixedOffset(6 * 60 + 27)]

        for ttup in time_examples:
            for tzi in tzinfos:
                t = time(*ttup, tzinfo=tzi)
                tstr = t.isoformat()

                with self.subTest(tstr=tstr):
                    t_rt = time.fromisoformat(tstr)
                    self.assertEqual(t, t_rt)

    def test_time_fromisoformat_timezone(self):
        base_time = time(12, 30, 45, 217456)

        tzoffsets = [
            5 * 60,
            2 * 60,
            (6 * 60) + 27,

            # Our timezone implementation doesn't handle sub-minute offsets.
            # timedelta(hours=12, minutes=32, seconds=30),
            # timedelta(hours=2, minutes=4, seconds=9, microseconds=123456)
        ]

        tzoffsets += [-1 * offset for offset in tzoffsets]

        tzinfos = [None, pytz.utc,
                   pytz.FixedOffset(0)]

        tzinfos += [pytz.FixedOffset(offset) for offset in tzoffsets]

        for tzi in tzinfos:
            t = base_time.replace(tzinfo=tzi)
            tstr = t.isoformat()

            with self.subTest(tstr=tstr):
                t_rt = time.fromisoformat(tstr)
                assert t == t_rt, t_rt

    def test_time_fromisoformat_timespecs(self):
        if sys.version_info >= (3, 6): # timespec parameter was added in Python 3.6
            time_bases = [
                (8, 17, 45, 123456),
                (8, 17, 45, 0)
            ]

            tzinfos = [None, pytz.utc,
                    pytz.FixedOffset(-5 * 60),
                    pytz.FixedOffset(2 * 60),
                    pytz.FixedOffset((6 * 60) +27)]

            timespecs = ['hours', 'minutes', 'seconds',
                        'milliseconds', 'microseconds']

            for ip, ts in enumerate(timespecs):
                for tzi in tzinfos:
                    for t_tuple in time_bases:
                        if ts == 'milliseconds':
                            new_microseconds = 1000 * (t_tuple[-1] // 1000)
                            t_tuple = t_tuple[0:-1] + (new_microseconds,)

                        t = time(*(t_tuple[0:(1 + ip)]), tzinfo=tzi)
                        tstr = t.isoformat(timespec=ts)
                        with self.subTest(tstr=tstr):
                            t_rt = time.fromisoformat(tstr)
                            self.assertEqual(t, t_rt)

    def test_time_fromisoformat_fractions(self):
        strs = [
            ('12:30:45.1', (12, 30, 45, 100000)),
            ('12:30:45.12', (12, 30, 45, 120000)),
            ('12:30:45.123', (12, 30, 45, 123000)),
            ('12:30:45.1234', (12, 30, 45, 123400)),
            ('12:30:45.12345', (12, 30, 45, 123450)),
            ('12:30:45.123456', (12, 30, 45, 123456)),
            ('12:30:45.1234567', (12, 30, 45, 123456)),
            ('12:30:45.12345678', (12, 30, 45, 123456)),
        ]

        for time_str, time_comps in strs:
            expected = time(*time_comps)
            actual = time.fromisoformat(time_str)

            self.assertEqual(actual, expected)

    def test_fromisoformat_time_examples(self):
        examples = [
            ('0000', time(0, 0)),
            ('00:00', time(0, 0)),
            ('000000', time(0, 0)),
            ('00:00:00', time(0, 0)),
            ('000000.0', time(0, 0)),
            ('00:00:00.0', time(0, 0)),
            ('000000.000', time(0, 0)),
            ('00:00:00.000', time(0, 0)),
            ('000000.000000', time(0, 0)),
            ('00:00:00.000000', time(0, 0)),
            ('1200', time(12, 0)),
            ('12:00', time(12, 0)),
            ('120000', time(12, 0)),
            ('12:00:00', time(12, 0)),
            ('120000.0', time(12, 0)),
            ('12:00:00.0', time(12, 0)),
            ('120000.000', time(12, 0)),
            ('12:00:00.000', time(12, 0)),
            ('120000.000000', time(12, 0)),
            ('12:00:00.000000', time(12, 0)),
            ('2359', time(23, 59)),
            ('23:59', time(23, 59)),
            ('235959', time(23, 59, 59)),
            ('23:59:59', time(23, 59, 59)),
            ('235959.9', time(23, 59, 59, 900000)),
            ('23:59:59.9', time(23, 59, 59, 900000)),
            ('235959.999', time(23, 59, 59, 999000)),
            ('23:59:59.999', time(23, 59, 59, 999000)),
            ('235959.999999', time(23, 59, 59, 999999)),
            ('23:59:59.999999', time(23, 59, 59, 999999)),
            ('00:00:00Z', time(0, 0, tzinfo=pytz.utc)),
            ('12:00:00+0000', time(12, 0, tzinfo=pytz.utc)),
            ('12:00:00+00:00', time(12, 0, tzinfo=pytz.utc)),
            ('00:00:00+05',
             time(0, 0, tzinfo=pytz.FixedOffset(5 * 60))),
            ('00:00:00+05:30',
             time(0, 0, tzinfo=pytz.FixedOffset((5 * 60) + 30))),
            ('12:00:00-05:00',
             time(12, 0, tzinfo=pytz.FixedOffset(-5 * 60))),
            ('12:00:00-0500',
             time(12, 0, tzinfo=pytz.FixedOffset(-5 * 60))),
            # Our timezone implementation doesn't handle sub-minute offsets.
            # ('00:00:00,000-23:59:59.999999',
            #  time(0, 0, tzinfo=timezone(-timedelta(hours=23, minutes=59, seconds=59, microseconds=999999)))),
        ]

        for input_str, expected in examples:
            with self.subTest(input_str=input_str):
                actual = time.fromisoformat(input_str)
                self.assertEqual(actual, expected)

    def test_time_fromisoformat_fails(self):
        bad_strs = [
            '',                         # Empty string
            '12\ud80000',               # Invalid separator - surrogate char
            '12:',                      # Ends on a separator
            '12:30:',                   # Ends on a separator
            '12:30:15.',                # Ends on a separator
            '1',                        # Incomplete hours
            '12:3',                     # Incomplete minutes
            '12:30:1',                  # Incomplete seconds
            '1a:30:45.334034',          # Invalid character in hours
            '12:a0:45.334034',          # Invalid character in minutes
            '12:30:a5.334034',          # Invalid character in seconds
            '12:30:45.123456+24:30',    # Invalid time zone offset
            '12:30:45.123456-24:30',    # Invalid negative offset
            '12：30：45',                 # Uses full-width unicode colons
            '12:30:45.123456a',         # Non-numeric data after 6 components
            '12:30:45.123456789a',      # Non-numeric data after 9 components
            '12:30:45․123456',          # Uses \u2024 in place of decimal point
            '12:30:45a',                # Extra at tend of basic time
            '12:30:45.123a',            # Extra at end of millisecond time
            '12:30:45.123456a',         # Extra at end of microsecond time
            '12:30:45.123456-',         # Extra at end of microsecond time
            '12:30:45.123456+',         # Extra at end of microsecond time
            '12:30:45.123456+12:00:30a',    # Extra at end of full time
        ]

        for bad_str in bad_strs:
            with self.subTest(bad_str=bad_str):
                with self.assertRaises(ValueError, msg="Did not fail on '{0}'".format(bad_str)):
                    time.fromisoformat(bad_str)

    def test_time_fromisoformat_fails_typeerror(self):
        # Test the fromisoformat fails when passed the wrong type
        bad_types = [b'12:30:45', None, io.StringIO('12:30:45')]

        for bad_type in bad_types:
            with self.assertRaises(TypeError, msg="Did not fail on '{0}'".format(bad_type)):
                time.fromisoformat(bad_type)

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

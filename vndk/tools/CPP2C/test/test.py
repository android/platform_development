#!/usr/bin/env python3

import os
import unittest
import sys
import tempfile

import_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '.'))
import_path = os.path.abspath(os.path.join(import_path, 'utils'))
sys.path.insert(1, import_path)

from utils import (AOSP_DIR, read_content, run_header_converter)


SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
INPUT_DIR = os.path.join(SCRIPT_DIR, 'input')
EXPECTED_DIR = os.path.join(SCRIPT_DIR, 'expected')


class HeaderCheckerTest(unittest.TestCase):

    # Setup

    @classmethod
    def setUpClass(cls):
        cls.maxDiff = None

    def setUp(self):
        self.tmp_dir = None

    def tearDown(self):
        if self.tmp_dir:
            self.tmp_dir.cleanup()
            self.tmp_dir = None

    # Helpers

    def run_and_compare(self, input_path, expected_path, flags=[], cflags=[]):
        actual_output = run_header_converter(input_path, flags, cflags)

        expected_no_ext = os.path.splitext(expected_path)[0]

        expected_h = read_content(expected_no_ext + ".h")
        expected_cpp = read_content(expected_no_ext + ".cpp")

        self.assertEqual(actual_output[0], expected_h)
        self.assertEqual(actual_output[1], expected_cpp)

    def run_and_compare_name(self, name, flags=[], cflags=[]):
        input_path = os.path.join(INPUT_DIR, name)
        expected_path = os.path.join(EXPECTED_DIR, name)
        self.run_and_compare(input_path, expected_path, flags, cflags)

    # Tests begin here

    def test_example1(self):
        self.run_and_compare_name('CppClass.h', ['-wrap', './test/classesToWrap.txt'])


if __name__ == '__main__':
    unittest.main()

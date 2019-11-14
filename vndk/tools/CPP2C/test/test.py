#!/usr/bin/env python3

import os
import unittest
import sys
import tempfile

import_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '.'))
sys.path.insert(1, import_path)

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
INPUT_DIR = os.path.join(SCRIPT_DIR, 'input')
OUTPUT_DIR = os.path.join(SCRIPT_DIR, 'output')
EXPECTED_DIR = os.path.join(SCRIPT_DIR, 'expected')

from utils import (AOSP_DIR, read_content, run_header_converter)

HWUI_HEADERS_DIR = (
    os.path.join(AOSP_DIR, 'system', 'core', 'libutils',
                 'include'),
)

SKIA_HEADERS_DIR = (
    os.path.join(AOSP_DIR, 'external', 'skia'),
    os.path.join(AOSP_DIR, 'external', 'skia', 'include',
                 'config', 'linux'),
    os.path.join(AOSP_DIR, 'external', 'skia', 'include',
                 'config'),
)

class CppConverterTests(unittest.TestCase):

    # Helpers

    def run_and_compare(self, input_path, output_path, expected_path, flags=[], cflags=[]):
        actual_output = run_header_converter(input_path, output_path, flags, cflags)

        expected_no_ext = os.path.splitext(expected_path)[0]

        expected_h = read_content(expected_no_ext + ".h")
        expected_cpp = read_content(expected_no_ext + ".cpp")

        self.assertEqual(actual_output[0], expected_h)
        self.assertEqual(actual_output[1], expected_cpp)


    def run_and_compare_name(self, name, flags=[], cflags=[]):
        input_path = os.path.join(INPUT_DIR, name)
        output_path = OUTPUT_DIR
        expected_path = os.path.join(EXPECTED_DIR, name)
        self.run_and_compare(input_path, output_path, expected_path, flags, cflags)

    # Tests begin here

    def test_basic_functionality(self):
        self.run_and_compare_name('CppTest.h', ['-wrap', SCRIPT_DIR + '/classesForBasicTest.txt'])

    def test_stdlib(self):
        self.run_and_compare_name('CppStdTest.h', ['-wrap', SCRIPT_DIR + '/stdClasses.txt'])

    def test_hwui(self):
        cflags = []
        for dir in HWUI_HEADERS_DIR:
            cflags += ['-I', dir]

        self.run_and_compare_name('HwuiPointers.h', ['-wrap', SCRIPT_DIR + '/hwuiClasses.txt'], cflags)

    def test_skia(self):
        cflags = []
        for dir in SKIA_HEADERS_DIR:
            cflags += ['-I', dir]

        self.run_and_compare_name('SkiaPointers.h', ['-wrap', SCRIPT_DIR + '/skiaClasses.txt'], cflags)


if __name__ == '__main__':
    unittest.main()

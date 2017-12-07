#!/usr/bin/env python3

import os
import sys
import unittest

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import compat
from vndk_definition_tool import VNDKLibDir


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


class VNDKLibDirTest(unittest.TestCase):
    def test_is_vndk_sp_dir_name(self):
        self.assertTrue(VNDKLibDir.is_vndk_sp_dir_name('vndk-sp'))
        self.assertTrue(VNDKLibDir.is_vndk_sp_dir_name('vndk-sp-28'))

        self.assertFalse(VNDKLibDir.is_vndk_sp_dir_name('vndk'))
        self.assertFalse(VNDKLibDir.is_vndk_sp_dir_name('vndk-28'))


    def test_is_vndk_dir_name(self):
        self.assertTrue(VNDKLibDir.is_vndk_dir_name('vndk'))
        self.assertTrue(VNDKLibDir.is_vndk_dir_name('vndk-28'))

        self.assertFalse(VNDKLibDir.is_vndk_dir_name('vndk-sp'))
        self.assertFalse(VNDKLibDir.is_vndk_dir_name('vndk-sp-28'))


    def test_create_from_dirs_unversioned(self):
        input_dir = os.path.join(
                SCRIPT_DIR, 'testdata', 'test_vndk_lib_dir',
                'vndk_unversioned')

        vndk_lib_dirs = VNDKLibDir.create_from_dirs(
                [os.path.join(input_dir, 'system')],
                [os.path.join(input_dir, 'vendor')])

        def test(lib_dir, vndk_sp_dirs, vndk_dirs):
            expected_vndk_sp_dirs = [
                '/vendor/' + lib_dir + '/vndk-sp',
                '/system/' + lib_dir + '/vndk-sp',
            ]

            expected_vndk_dirs = [
                '/vendor/' + lib_dir + '/vndk',
                '/system/' + lib_dir + '/vndk',
            ]

            self.assertEqual(expected_vndk_sp_dirs, vndk_sp_dirs)
            self.assertEqual(expected_vndk_dirs, vndk_dirs)

            # To make sure the libraries in the vendor partition  are found
            # before the one in the system partition, the order of the answer
            # matters.
            self.assertLessEqual(
                    vndk_sp_dirs.index('/vendor/' + lib_dir + '/vndk-sp'),
                    vndk_sp_dirs.index('/system/' + lib_dir + '/vndk-sp'))
            self.assertLessEqual(
                    vndk_dirs.index('/vendor/' + lib_dir + '/vndk'),
                    vndk_dirs.index('/system/' + lib_dir + '/vndk'))

        test('lib', vndk_lib_dirs.vndk_sp_lib32, vndk_lib_dirs.vndk_lib32)
        test('lib64', vndk_lib_dirs.vndk_sp_lib64, vndk_lib_dirs.vndk_lib64)


    def test_create_from_dirs_versioned(self):
        input_dir = os.path.join(
                SCRIPT_DIR, 'testdata', 'test_vndk_lib_dir', 'vndk_versioned')

        vndk_lib_dirs = VNDKLibDir.create_from_dirs(
                [os.path.join(input_dir, 'system')],
                [os.path.join(input_dir, 'vendor')])

        def test(lib_dir, vndk_sp_dirs, vndk_dirs):
            expected_vndk_sp_dirs = [
                '/vendor/' + lib_dir + '/vndk-sp-28',
                '/system/' + lib_dir + '/vndk-sp-28',
            ]

            expected_vndk_dirs = [
                '/vendor/' + lib_dir + '/vndk-28',
                '/system/' + lib_dir + '/vndk-28',
            ]

            self.assertEqual(expected_vndk_sp_dirs, vndk_sp_dirs)
            self.assertEqual(expected_vndk_dirs, vndk_dirs)

            # To make sure the libraries in the vendor partition  are found
            # before the one in the system partition, the order of the answer
            # matters.
            self.assertLessEqual(
                    vndk_sp_dirs.index('/vendor/' + lib_dir + '/vndk-sp-28'),
                    vndk_sp_dirs.index('/system/' + lib_dir + '/vndk-sp-28'))
            self.assertLessEqual(
                    vndk_dirs.index('/vendor/' + lib_dir + '/vndk-28'),
                    vndk_dirs.index('/system/' + lib_dir + '/vndk-28'))

        test('lib', vndk_lib_dirs.vndk_sp_lib32, vndk_lib_dirs.vndk_lib32)
        test('lib64', vndk_lib_dirs.vndk_sp_lib64, vndk_lib_dirs.vndk_lib64)


    def test_create_from_dirs_versioned_multiple(self):
        input_dir = os.path.join(
                SCRIPT_DIR, 'testdata', 'test_vndk_lib_dir',
                'vndk_versioned_multiple')

        vndk_lib_dirs = VNDKLibDir.create_from_dirs(
                [os.path.join(input_dir, 'system')],
                [os.path.join(input_dir, 'vendor')])

        def test(lib_dir, vndk_sp_dirs, vndk_dirs):
            expected_vndk_sp_dirs = [
                '/vendor/' + lib_dir + '/vndk-sp-29',
                '/vendor/' + lib_dir + '/vndk-sp-28',
                '/system/' + lib_dir + '/vndk-sp-29',
                '/system/' + lib_dir + '/vndk-sp-28',
            ]

            expected_vndk_dirs = [
                '/vendor/' + lib_dir + '/vndk-29',
                '/vendor/' + lib_dir + '/vndk-28',
                '/system/' + lib_dir + '/vndk-29',
                '/system/' + lib_dir + '/vndk-28',
            ]

            self.assertEqual(expected_vndk_sp_dirs, vndk_sp_dirs)
            self.assertEqual(expected_vndk_dirs, vndk_dirs)

            # To make sure the libraries in the vendor partition  are found
            # before the one in the system partition, the order of the answer
            # matters.
            self.assertLessEqual(
                    vndk_sp_dirs.index('/vendor/' + lib_dir + '/vndk-sp-28'),
                    vndk_sp_dirs.index('/system/' + lib_dir + '/vndk-sp-28'))
            self.assertLessEqual(
                    vndk_dirs.index('/vendor/' + lib_dir + '/vndk-28'),
                    vndk_dirs.index('/system/' + lib_dir + '/vndk-28'))

        test('lib', vndk_lib_dirs.vndk_sp_lib32, vndk_lib_dirs.vndk_lib32)
        test('lib64', vndk_lib_dirs.vndk_sp_lib64, vndk_lib_dirs.vndk_lib64)


if __name__ == '__main__':
    unittest.main()

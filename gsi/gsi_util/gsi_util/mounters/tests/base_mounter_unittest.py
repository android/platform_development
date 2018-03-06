#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Unit test for gsi_util.mounters.base.base_mounter."""

import unittest

from gsi_util.mounters import base_mounter


class _MockFileAccessor(base_mounter.BaseFileAccessor):
  _PATH_PREFIX = '/system/'
  _FAKE_FILES = (
      'exist.a',
      'exist.c',
      'folder/exist.d',
      'folder/exist.f',
  )

  def __init__(self):
    super(_MockFileAccessor, self).__init__(self._PATH_PREFIX)

  # override
  def _handle_prepare_file(self, filename_in_storage):
    if filename_in_storage not in self._FAKE_FILES:
      return None

    prepared_filename = '/prepared/' + filename_in_storage
    return base_mounter.MounterFile(prepared_filename)


class _MockMounter(base_mounter.BaseMounter):

  def __init__(self, serial_num=None):
    super(_MockMounter, self).__init__()

  def _handle_mount(self):
    return _MockFileAccessor()

  # override
  def _handle_unmount(self):
    # do nothing
    pass


class BaseMounterTest(unittest.TestCase):
  """Unit test for BaseMounterTest."""

  def test_prepare_file_with_diff_styles(self):
    mounter = _MockMounter()

    # By a string
    with mounter as file_accessor:
      RULE = '/system/folder/exist.f'
      with file_accessor.prepare_file(RULE) as filename:
        self.assertEqual(filename, '/prepared/folder/exist.f')

    # By a list
    with mounter as file_accessor:
      RULE = ['/system/folder/lake.e', '/system/folder/exist.f']
      with file_accessor.prepare_file(RULE) as filename:
        self.assertEqual(filename, '/prepared/folder/exist.f')

    # By a dict, 'fallbacks' is a string
    with mounter as file_accessor:
      RULE = {'fallbacks': '/system/folder/exist.f'}
      with file_accessor.prepare_file(RULE) as filename:
        self.assertEqual(filename, '/prepared/folder/exist.f')

    # By a dict, 'fallbacks' is a list
    with mounter as file_accessor:
      RULE = {'fallbacks': ['/system/folder/lake.e', '/system/folder/exist.f']}
      with file_accessor.prepare_file(RULE) as filename:
        self.assertEqual(filename, '/prepared/folder/exist.f')

  def test_prepare_file_assert(self):
    mounter = _MockMounter()
    with mounter as file_accessor:
      # the storage is not mounted on 'vendor', should raise an exception
      with self.assertRaises(RuntimeError):
        with file_accessor.prepare_file('/vendor/folder/exist.d') as _:
          self.fail()

      # the file is not in storage, should raise an exception
      with self.assertRaises(RuntimeError):
        with file_accessor.prepare_file('/system/folder/lake.e') as _:
          self.fail()

  def test_prepare_file_by_rule_with_fallback(self):
    # All test cases should return the first existing file in fallback_list
    mounter = _MockMounter()

    with mounter as file_accessor:
      # Should return the first exist file in the storage
      RULE = ['/system/lake.b', '/system/exist.c', '/system/folder/exist.d']
      with file_accessor.prepare_file(RULE) as filename:
        self.assertEqual(filename, '/prepared/exist.c')

      LAKE_FALLBACKS = [
          '/system/lake', '/system/lake.b', '/system/folder/lake.e'
      ]

      # Should return None if all files in fallback_list are not exist
      RULE = {'fallbacks': LAKE_FALLBACKS, 'optional': True}
      with file_accessor.prepare_file(RULE) as filename:
        self.assertIsNone(filename)

      # Should raise an exception if all files in fallback_list are not exist
      with self.assertRaises(RuntimeError):
        RULE = {'fallbacks': LAKE_FALLBACKS, 'optional': False}
        with file_accessor.prepare_file(RULE) as _:
          self.fail()

      # the default 'optional' should False
      with self.assertRaises(RuntimeError):
        RULE = {'fallbacks': LAKE_FALLBACKS}
        with file_accessor.prepare_file(RULE) as _:
          self.fail()

  def test_prepare_multi_files_with_diff_styles(self):
    mounter = _MockMounter()

    # By a dict
    with mounter as file_accessor:
      RULES = {
          'file1': '/system/exist.c',
          'file2': ['/system/folder/lake.e', '/system/folder/exist.f'],
          'file3': {'fallbacks': '/system/lake.b', 'optional': True}
      }
      EXPECT_PREPARED = {
          'file1': '/prepared/exist.c',
          'file2': '/prepared/folder/exist.f',
          'file3': None
      }
      with file_accessor.prepare_multi_files(RULES) as prepared_files:
        self.assertEqual(prepared_files, EXPECT_PREPARED)

    # By a tuple
    with mounter as file_accessor:
      RULES = (
          '/system/exist.c',
          ['/system/folder/lake.e', '/system/folder/exist.f'],
          {'fallbacks': '/system/lake.b', 'optional': True}
      )
      EXPECT_PREPARED = ('/prepared/exist.c', '/prepared/folder/exist.f', None)
      with file_accessor.prepare_multi_files(RULES) as prepared_files:
        self.assertEqual(prepared_files, EXPECT_PREPARED)

    # By a list
    with mounter as file_accessor:
      RULES = [
          '/system/exist.c',
          ['/system/folder/lake.e', '/system/folder/exist.f'],
          {'fallbacks': '/system/lake.b', 'optional': True}
      ]
      EXPECT_PREPARED = ['/prepared/exist.c', '/prepared/folder/exist.f', None]
      with file_accessor.prepare_multi_files(RULES) as prepared_files:
        self.assertEqual(prepared_files, EXPECT_PREPARED)


  def test_prepare_multi_files_with_dict_and_fallback(self):
    mounter = _MockMounter()

    # Should prepare 'file1' and 'file2'; lake for 'file3'
    with mounter as file_accessor:
      RULES = {
          'file1': '/system/exist.c',
          'file2': ['/system/lack.f', '/system/folder/exist.f'],
          'file3': {
              'fallbacks': ['/system/lack.a', '/system/folder/lack.e'],
              'optional': True
          }
      }
      EXPECT_PREPARED = {
          'file1': '/prepared/exist.c',
          'file2': '/prepared/folder/exist.f',
          'file3': None
      }
      with file_accessor.prepare_multi_files(RULES) as prepared_files:
        self.assertEqual(prepared_files, EXPECT_PREPARED)

    # Should be assert by 'file3'
    with mounter as file_accessor:
      RULES = {
          'file1': '/system/exist.c',
          'file2': {
              'fallbacks': ['/system/lack.f' '/system/folder/exist.f'],
              'optional': True
          },
          'file3': ['/system/lack.a', '/system/lack.b']
      }
      with self.assertRaises(RuntimeError):
        with file_accessor.prepare_multi_files(RULES) as _:
          self.fail()

if __name__ == '__main__':
  unittest.main()

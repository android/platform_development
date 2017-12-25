#!/usr/bin/env python
#
# Copyright 2017 - The Android Open Source Project
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
"""Provides class AdbMounter."""

import errno
import logging
import os
import shutil
import tempfile

import gsi_util.mounters.base_mounter as base_mounter
import gsi_util.utils.adb_utils as adb_utils


class _AdbFileAccessor(base_mounter.BaseFileAccessor):

  def __init__(self, temp_dir):
    super(_AdbFileAccessor, self).__init__()
    self._temp_dir = temp_dir

  @staticmethod
  def _prepare_dirs_for_file(filename):
    dir_path = os.path.dirname(filename)
    try:
      os.makedirs(dir_path)
    except OSError as exc:
      if exc.errno != errno.EEXIST:
        raise

  def _handle_prepare_file(self, pathfile_in_mount):
    filename = os.path.join(self._temp_dir, pathfile_in_mount)
    logging.info('Prepare file %s -> %s', pathfile_in_mount, filename)

    self._prepare_dirs_for_file(filename)
    if not adb_utils.pull(filename, pathfile_in_mount):
      logging.error('Fail to prepare file: %s', pathfile_in_mount)
      return None

    def cleanup():
      logging.debug('Delete temp file: %s', filename)
      os.remove(filename)

    return (filename, cleanup)


class AdbMounter(base_mounter.BaseMounter):
  """Provides a file accessor which can access files by adb."""

  def __init__(self, device_name):
    super(AdbMounter, self).__init__()
    self._device_name = device_name

  def _handle_mount(self):
    adb_utils.root()

    self._temp_dir = tempfile.mkdtemp()
    logging.debug('Created temp dir: %s', self._temp_dir)

    return _AdbFileAccessor(self._temp_dir)

  def _handle_unmount(self):
    logging.debug('Remove temp dir: %s', self._temp_dir)
    shutil.rmtree(self._temp_dir)
    del self._temp_dir

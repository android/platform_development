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

"""CompositeMounter implements a BaseMounter which can add multiple mounters."""

import logging
import os

from adb_mounter import AdbMounter
import base_mounter
from folder_mounter import FolderMounter
from image_mounter import ImageMounter


class _CompositeFileAccessor(base_mounter.BaseFileAccessor):

  def __init__(self, file_accessors):
    super(_CompositeFileAccessor, self).__init__()
    self._file_accessors = file_accessors

  def _handle_prepare_file(self, pathfile_in_mount):
    logging.debug('_CompositeFileAccessor._handle_prepare_file(%s)',
                  pathfile_in_mount)

    pathfile_in_mount = '/' + pathfile_in_mount
    for (prefix_path, file_accessor) in self._file_accessors:
      if pathfile_in_mount.startswith(prefix_path):
        return file_accessor.prepare_file(pathfile_in_mount)

    logging.debug('  Not found')
    return None


class CompositeMounter(base_mounter.BaseMounter):
  """Implements a BaseMounter which can add multiple mounters."""

  _SUPPORT_PARTITION = ['system', 'vendor']

  @classmethod
  def _create_mounter(cls, partition, name_to_mount):
    """Create a proper Mounter instance by the given name.

    Args:
      partiton: the partition to be mounted as
      name_to_mount: see Returns

    Returns:
      Return a AdbMounter if name_to_mount is 'adb'
      Return a FolderMounter if name_to_mount is a folder
      Return a ImageMounter if name_to_mount is a file

    Raises:
      ValueError: partiton is not support or name_to_mount is not exist
    """
    if partition not in cls._SUPPORT_PARTITION:
      raise ValueError('Wrong partition name "{}"'.format(partition))

    if name_to_mount == 'adb':
      # TODO(szuweilin): handle device name
      return AdbMounter('device')

    path_prefix = '/{}/'.format(partition)

    if os.path.isdir(name_to_mount):
      return FolderMounter(name_to_mount, path_prefix)

    if os.path.isfile(name_to_mount):
      if partition == 'system':
        path_prefix = 'detect-system-as-root'
      return ImageMounter(name_to_mount, path_prefix)

    raise ValueError('Unknown target "{}"'.format(name_to_mount))

  def __init__(self):
    super(CompositeMounter, self).__init__()
    self._mounters = []

  def _handle_mount(self):
    file_accessors = [(path_prefix, mounter.mount())
                      for (path_prefix, mounter) in self._mounters]
    return _CompositeFileAccessor(file_accessors)

  def _handle_unmount(self):
    for (_, mounter) in reversed(self._mounters):
      mounter.unmount()

  def add(self, partition, name_to_mount):
    logging.debug('CompositeMounter.add(%s, %s)', partition, name_to_mount)
    path_prefix = '/{}/'.format(partition)
    mounter = self._create_mounter(partition, name_to_mount)
    self._mounters.append((path_prefix, mounter))

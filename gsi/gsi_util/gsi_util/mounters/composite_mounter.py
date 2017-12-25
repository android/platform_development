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

  # override
  def _handle_prepare_file(self, file_in_storage):
    logging.debug('_CompositeFileAccessor._handle_prepare_file(%s)',
                  file_in_storage)

    pathfile_to_prepare = '/' + file_in_storage
    for (prefix_path, file_accessor) in self._file_accessors:
      if pathfile_to_prepare.startswith(prefix_path):
        return file_accessor.prepare_file(pathfile_to_prepare)

    logging.debug('  Not found')
    return None


class CompositeMounter(base_mounter.BaseMounter):
  """Implements a BaseMounter which can add multiple mounters."""

  _SUPPORTED_PARTITION = ['system', 'vendor']

  @classmethod
  def _create_mounter(cls, partition, mount_target):
    """Create a proper Mounter instance by the given name.

    Args:
      partition: the partition to be mounted as
      mount_target: 'adb', a folder name or an image file name to mount.
        see Returns for the detail.

    Returns:
      Returns an AdbMounter if mount_target is 'adb'
      Returns a FolderMounter if mount_target is a folder name
      Returns an ImageMounter if mount_target is a image file name

    Raises:
      ValueError: partiton is not support or mount_target is not exist.
    """
    if partition not in cls._SUPPORTED_PARTITION:
      raise ValueError('Wrong partition name "{}"'.format(partition))

    if mount_target == 'adb':
      # TODO(szuweilin): handle device name
      return AdbMounter('device')

    path_prefix = '/{}/'.format(partition)

    if os.path.isdir(mount_target):
      return FolderMounter(mount_target, path_prefix)

    if os.path.isfile(mount_target):
      if partition == 'system':
        path_prefix = ImageMounter.DETECT_SYSTEM_AS_ROOT
      return ImageMounter(mount_target, path_prefix)

    raise ValueError('Unknown target "{}"'.format(mount_target))

  def __init__(self):
    super(CompositeMounter, self).__init__()
    self._mounters = []

  # override
  def _handle_mount(self):
    file_accessors = [(path_prefix, mounter.mount())
                      for (path_prefix, mounter) in self._mounters]
    return _CompositeFileAccessor(file_accessors)

  # override
  def _handle_unmount(self):
    for (_, mounter) in reversed(self._mounters):
      mounter.unmount()

  def add(self, partition, mount_target):
    logging.debug('CompositeMounter.add(%s, %s)', partition, mount_target)
    path_prefix = '/{}/'.format(partition)
    mounter = self._create_mounter(partition, mount_target)
    self._mounters.append((path_prefix, mounter))

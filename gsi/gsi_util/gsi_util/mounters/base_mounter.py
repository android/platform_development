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

"""Base classes to implement Mounter classes."""

import logging


class MounterFile(object):

  def __init__(self, filename, cleanup_func=None):
    self._filename = filename
    self._clean_up_func = cleanup_func

  def _handle_clean_up(self):
    if self._clean_up_func:
      self._clean_up_func()

  def __enter__(self):
    return self._filename

  def __exit__(self, exc_type, exc_val, exc_tb):
    self._handle_clean_up()

  def clean_up(self):
    self._handle_clean_up()


class BaseFileAccessor(object):
  """An abstract class to implement the file accessors.

  A mounter returns a file accessor when it is mounted. A file accessor must
  override the method  _handle_prepare_file() to return the file name of
  the requested file in the storage. However, files in some mounter storages
  couldn't be access directly, ex. the file accessor of AdbMounter, which
  accesses the file in a device by adb. In this case, file accessor could
  return a temp file which contains the content. A file accessor could give the
  cleanup_func when creating MounterFile instance to cleanup the temp file.
  """

  def __init__(self, path_prefix='/'):
    logging.debug('BaseFileAccessor(path_prefix=%s)', path_prefix)
    self._path_prefix = path_prefix

  def _get_pathfile_to_access(self, file_to_map):
    path_prefix = self._path_prefix

    if not file_to_map.startswith(path_prefix):
      raise RuntimeError('"%s" does not start with "%s"', file_to_map,
                         path_prefix)

    return file_to_map[len(path_prefix):]

  def _handle_prepare_file(self, filename_in_storage):
    """Override this method to prepare the given file in the storage.

    Args:
      filename_in_storage: the file in the storage to be prepared

    Returns:
      Return an MounterFile instance. Return None if the request file is not
      in the mount.
    """
    raise NotImplementedError()

  def prepare_file(self, filename_in_mount):
    """Return the accessable file name in the storage.

    The function prepares a accessable file which contains the content of the
    filename_in_mount.

    See BaseFileAccessor for the detail.

    Args:
      filename_in_mount: the file to map.
        filename_in_mount should be a full path file as the path in a real
        device, and must start with a '/'. For example: '/system/build.prop',
        '/vendor/default.prop', '/init.rc', etc.

    Returns:
      A MounterFile instance. Return None if the file is not exit in the
      storage.
    """
    filename_in_storage = self._get_pathfile_to_access(filename_in_mount)
    ret = self._handle_prepare_file(filename_in_storage)
    return ret if ret else MounterFile(None)


class BaseMounter(object):

  def _handle_mount(self):
    """Override this method to handle mounting and return a file accessor.

    File accessor must inherit from  BaseFileAccessor.
    """
    raise NotImplementedError()

  def _handle_unmount(self):
    """Override this method to handle cleanup this mounting."""
    # default is do nothing
    return

  def _process_mount(self):
    if self._mounted:
      raise RuntimeError('The mounter had been mounted.')

    file_accessor = self._handle_mount()
    self._mounted = True

    return file_accessor

  def _process_unmount(self):
    if self._mounted:
      self._handle_unmount()
      self._mounted = False

  def __init__(self):
    self._mounted = False

  def __enter__(self):
    return self._process_mount()

  def __exit__(self, exc_type, exc_val, exc_tb):
    self._process_unmount()

  def mount(self):
    return self._process_mount()

  def unmount(self):
    self._process_unmount()

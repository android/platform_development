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

"""Base classes to implement Mounter classes."""

import logging


class MounterFile(object):

  def __init__(self, filename, cleanup_func):
    self._filename = filename
    self._clean_up_func = cleanup_func

  def _handle_clean_up(self):
    if self._clean_up_func:
      self._clean_up_func()

  def __enter__(self):
    return self._filename

  def __exit__(self, exc_type, exc_val, exc_tb):
    self._handle_clean_up()


class BaseFileAccessor(object):

  def __init__(self, path_prefix='/'):
    logging.debug('BaseFileAccessor(path_prefix=%s)', path_prefix)
    self._path_prefix = path_prefix

  def _get_pathfile_to_access(self, file_to_map):
    path_prefix = self._path_prefix

    if not file_to_map.startswith(path_prefix):
      raise RuntimeError('"%s" does not start with "%s"', file_to_map,
                         path_prefix)

    return file_to_map[len(path_prefix):]

  def _handle_prepare_file(self, pathfile_in_mount):
    """Prepare a mapping file for the given file in the storage.

    Args:
      pathfile_in_mount: the file in the storage to be prepared
    Returns:
      Could be an MounterFile instance, filename or a
      tuple(filename, cleanup_func). See prepare_file() for the detail
      filename: the accessable file name which map to pathfile_in_mount
      cleanup_func: a function to clean up the file after accessed
    """

    raise NotImplementedError()

  def prepare_file(self, file_to_map):
    """Prepare a mapping file for the file-to-map in the image or device.

    Args:
      file_to_map: the file to map.
        file-to-map should be a full path file as the path in a real device,
        and must start with a '/'. For example: '/system/build.prop',
        '/vendor/default.prop', '/init.rc', etc.

    Returns:
      The function prepares a accessable file and return its file by
      a _MappingFile. The file could be a copy of the file-to-map.
    """
    pathfile_in_mount = self._get_pathfile_to_access(file_to_map)
    ret = self._handle_prepare_file(pathfile_in_mount)
    if isinstance(ret, MounterFile):
      return ret
    elif isinstance(ret, tuple):
      (filename, cleanup_func) = ret
      return MounterFile(filename, cleanup_func)
    else:
      filename = ret
      return MounterFile(filename, None)


class BaseMounter(object):

  def _handle_mount(self):
    """Return an instance which inherits from BaseFileAccessor."""
    raise NotImplementedError()

  def _handle_unmount(self):
    """Handles cleanup for this mounting."""
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

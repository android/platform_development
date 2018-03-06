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
"""Base classes to implement Mounter classes."""

import abc
import logging


class MounterFile(object):

  def __init__(self, filename, cleanup_func=None):
    self._filename = filename
    self._clean_up_func = cleanup_func

  def _handle_get_filename(self):
    return self._filename

  def _handle_clean_up(self):
    if self._clean_up_func:
      self._clean_up_func()

  def __enter__(self):
    return self._handle_get_filename()

  def __exit__(self, exc_type, exc_val, exc_tb):
    self._handle_clean_up()

  def get_filename(self):
    return self._handle_get_filename()

  def clean_up(self):
    self._handle_clean_up()


class _MounterMultiFiles(object):

  def __init__(self, prepared_files):
    assert isinstance(prepared_files, (dict, tuple, list))
    self._prepared_files = prepared_files

  def _handle_get_filenames(self):
    prepared_files = self._prepared_files
    if isinstance(prepared_files, dict):
      return dict((k, v.get_filename()) for k, v in prepared_files.iteritems())
    elif isinstance(prepared_files, tuple):
      return tuple(x.get_filename() for x in prepared_files)
    elif isinstance(prepared_files, list):
      return [x.get_filename() for x in prepared_files]
    else:
      assert False

  def _handle_clean_up(self):
    prepared_files = self._prepared_files
    if isinstance(prepared_files, dict):
      clean_up_list = prepared_files.values()
    elif isinstance(prepared_files, (tuple, list)):
      clean_up_list = prepared_files
    else:
      assert False

    # Cleanup all files in reversed order
    for x in reversed(clean_up_list):
      x.clean_up()

  def __enter__(self):
    return self._handle_get_filenames()

  def __exit__(self, exc_type, exc_val, exc_tb):
    self._handle_clean_up()

  def get_filenames(self):
    return self._handle_get_filenames()

  def clean_up(self):
    self._handle_clean_up()


class BaseFileAccessor(object):
  """An abstract class to implement the file accessors.

  A mounter returns a file accessor when it is mounted. A file accessor must
  override the method  _handle_prepare_file() to return the file name of
  the requested file in the storage. However, files in some mounter storages
  couldn't be access directly, e.g. the file accessor of AdbMounter, which
  accesses the file in a device by adb. In this case, file accessor could
  return a temp file which contains the content. A file accessor could give the
  cleanup_func when creating MounterFile to cleanup the temp file.
  """

  __metaclass__ = abc.ABCMeta

  def __init__(self, path_prefix='/'):
    logging.debug('BaseFileAccessor(path_prefix=%s)', path_prefix)
    self._path_prefix = path_prefix

  def _get_pathfile_to_access(self, file_to_map):
    path_prefix = self._path_prefix

    if not file_to_map.startswith(path_prefix):
      raise RuntimeError('"{}" does not start with "{}"'.format(file_to_map,
                                                                path_prefix))

    return file_to_map[len(path_prefix):]

  @abc.abstractmethod
  def _handle_prepare_file(self, filename_in_storage):
    """Override this method to prepare the given file in the storage.

    Args:
      filename_in_storage: the file in the storage to be prepared

    Returns:
      Return an MounterFile instance. Return None if the request file is not
      in the mount.
    """

  def prepare_file(self, file_to_prepare):
    """Return the file from mount source to the temp folder.

    This method will prepare the given file by files_to_prepare.
    which can be different types. For example:

      '/system/manifest.xml' (string)

      ['/system/manifest.xml', '/system/etc/vintf/manifest.xml'] (list)

      {'fallbacks': ['/system/manifest.xml'], optional: True}

    With different types, file_to_prepare will be:

      string: the only one filename to prepare in the fallback list
      list: a fallback list with filenames to prepare in order
      dict: with item by following keys:
        'fallbacks': same as a string or a list
        'optional': describ the file is optional

    The filename(s) in file_to_prepare should be a full path file as the path in
    a real device, and must start with a '/'. For example: '/system/build.prop',
    '/vendor/default.prop', '/init.rc', etc.

    The method Mounter will look the filenames in the fallback list in order,
    and prepare the first existing file in the list.

    When the method can not prepare the file by an any filename in the fallback
    list, it will raise an exception if the file is not optional, otherwise
    return None.

    For more examples, see test_prepare_file_with_diff_styles() and
    test_prepare_file_by_rule_with_fallback() in base_mounter_unittest.py

    Args:
      file_to_prepare: the file to be prepared

    Returns:
      A MounterFile instance which contains the prepared filename.
    """
    if isinstance(file_to_prepare, dict):
      fallbacks = file_to_prepare.get('fallbacks')
      optional = file_to_prepare.get('optional')
    else:
      fallbacks = file_to_prepare
      optional = False

    # Make sure fallback_list is a list
    if isinstance(fallbacks, basestring):
      fallbacks = [fallbacks]
    elif isinstance(fallbacks, list):
      pass
    else:
      raise ValueError("Unsupported type.")

    if not fallbacks:
      raise ValueError("No fallbacks.")

    for filename_in_mount in fallbacks:
      filename_in_storage = self._get_pathfile_to_access(filename_in_mount)
      ret = self._handle_prepare_file(filename_in_storage)
      if ret:
        return ret

    # Can not find any file in fallbacks
    if optional:
      return MounterFile(None)

    raise RuntimeError('Cannot prepare file: {}.'.format(fallbacks))

  def prepare_multi_files(self, files_to_prepare):
    """ Prepare multiple files from mount source to the temp folder.

      This method will prepare all files which in the files_to_prepare.
      files_to_prepare can be a dict, a tuple or a list. The return value will
      be the same type as the input.

      If the files_to_prepare is a dict, the return dict contains the prepared
      filenames with same key in the given dict. This style is more clear for
      preparing a lot of files.

      For example:

        RULES = { 'file1': '/system/the_file1', 'file2': '/system/the_file2' }
        with file_accessor.prepare_multi_files(RULES) as prepared

      And prepared will be similar to:

        { 'file1': '/prepared/the_file1', 'file2': '/prepared/the_file2'}

      Each file in files_to_prepare could be any type supported as the parameter
      file_to_prepare of method prepare_file(). See prepare_file() for the
      detail.

      For more examples, see test_prepare_multi_files_with_diff_styles() in
      base_mounter_unittest.py

      Args:
        files_to_prepare: The files in mount to be prepared.

      Return:
        A MounterMultiFiles contains the prepared filenames.
    """
    if isinstance(files_to_prepare, dict):
      prepared_files = dict(
          (k, self.prepare_file(v)) for k, v in files_to_prepare.iteritems())
    elif isinstance(files_to_prepare, tuple):
      prepared_files = tuple(self.prepare_file(x) for x in files_to_prepare)
    elif isinstance(files_to_prepare, list):
      prepared_files = [self.prepare_file(x) for x in files_to_prepare]
    else:
      raise ValueError("Unsupported type.")

    return _MounterMultiFiles(prepared_files)


class BaseMounter(object):

  __metaclass__ = abc.ABCMeta

  @abc.abstractmethod
  def _handle_mount(self):
    """Override this method to handle mounting and return a file accessor.

    File accessor must inherit from  BaseFileAccessor.
    """

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

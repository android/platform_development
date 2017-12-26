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

"""Provides class XmlDumper."""

import logging
import xml.etree.ElementTree as ET


class XmlDumper(object):

  def __init__(self, file_accessor, args):
    filename_in_mount = args[0]
    logging.debug('Parse %s...', filename_in_mount)
    with file_accessor.prepare_file(filename_in_mount) as filename:
      if filename:
        self._tree = ET.parse(filename)

  def __enter__(self):
    # do nothing
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    if hasattr(self, '_tree'):
      del self._tree

  def dump(self, index):
    if not hasattr(self, '_tree'):
      return None

    xpath = index
    results = self._tree.findall(xpath)
    return ', '.join([e.text for e in results]) if results else None

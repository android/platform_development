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

"""Script to run */tests/*_unittest.py files."""

import os
import runpy


def get_unittest_files():
  matches = []
  for dirpath, _, filenames in os.walk('.'):
    if os.path.basename(dirpath) == 'tests':
      matches.extend(os.path.join(dirpath, f)
                     for f in filenames if f.endswith('_unittest.py'))
  return matches


if __name__ == '__main__':
  for test_case in get_unittest_files():
    runpy.run_path(test_case, run_name='__main__')

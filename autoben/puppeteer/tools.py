# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import subprocess


def execute_command(command):
  """Runs command and returns its output.

  Args:
    command: sequence of program arguments

  Returns:
    Output from stdout
  """
  process = subprocess.Popen(command, stdout=subprocess.PIPE)
  out = process.communicate()[0]
  return out.strip()


def mean(vals):
  """Calculates mean of the values.

  Args:
    vals: list of values

  Returns:
    Mean of the values
  """
  return sum(vals) / float(len(vals))


def adb_rmdir(path):
  """Removes file or directory through adb.

  Args:
    path: path of the file or directory
  """
  execute_command(['adb', 'shell', 'rm', '-rf', path])


def init_output_if_not_exist(output_file):
  """Initializes the file if it's not existing.

  Args:
    output_file: path of the output file
  """
  if not os.path.isfile(output_file) or os.stat(output_file).st_size == 0:
    with open(output_file, 'w', 0) as fp:
      fp.write('record_id')


def start_uiauto(case):
  """Runs the automator with specific testcase.

  Args:
    case: name of the testcase
  """
  execute_command(['adb', 'shell', 'am', 'instrument', '-e', 'test', case,
                   '-w', ('com.google.android.apps.puppeteer.test/'
                          'android.support.test.runner.AndroidJUnitRunner')])

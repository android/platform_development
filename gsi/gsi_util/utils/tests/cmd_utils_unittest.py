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

from StringIO import StringIO
import subprocess
import shutil
import tempfile
import unittest

from gsi_util.utils import cmd_utils

class RunCommandTest(unittest.TestCase):

  def setUp(self):
    self.outstream = StringIO()

  def tearDown(self):
    self.outstream.close()

  def test_verbose_output(self):
    result = cmd_utils.run_command(['echo', '123'], verbose=True,
                                   output=self.outstream)
    self.assertEqual((0, None, None), result)
    self.assertEqual("Command executed successfully: 'echo 123'\n",
                      self.outstream.getvalue())

  def test_no_verbose_output(self):
    result = cmd_utils.run_command(['echo', 'foo'], verbose=False,
                                   output=self.outstream)
    self.assertEqual((0, None, None), result)
    self.assertEqual('', self.outstream.getvalue())

  def test_shell_command(self):
    result = cmd_utils.run_command('echo uses shell', verbose=True,
                                   shell=True, output=self.outstream)
    self.assertEqual((0, None, None), result)
    self.assertEqual("Command executed successfully: 'echo uses shell'\n",
                      self.outstream.getvalue())

  def test_read_stdout(self):
    result = cmd_utils.run_command('echo 123; echo 456; exit 5', verbose=False,
                                   shell=True, read_stdout=True,
                                   raise_on_error=False, output=self.outstream)
    self.assertEqual(5, result.returncode)
    self.assertEqual('123\n456\n', result.stdoutdata)
    self.assertEqual(None, result.stderrdata)

  def test_read_stderr(self):
    result = cmd_utils.run_command('(echo error1; echo error2)>&2; exit 3',
                                   verbose=False, shell=True, read_stderr=True,
                                   raise_on_error=False, output=self.outstream)
    self.assertEqual(3, result.returncode)
    self.assertEqual(None, result.stdoutdata)
    self.assertEqual('error1\nerror2\n', result.stderrdata)

  def test_read_stdout_and_stderr(self):
    result = cmd_utils.run_command('echo foo; echo bar>&2; exit 1',
                                   verbose=False, shell=True,
                                   read_stdout=True, read_stderr=True,
                                   raise_on_error=False, output=self.outstream)
    self.assertEqual(1, result.returncode)
    self.assertEqual('foo\n', result.stdoutdata)
    self.assertEqual('bar\n', result.stderrdata)

  def test_raise_on_error(self):
    _CMD_FAILED = 'echo foo; exit 1'
    with self.assertRaises(subprocess.CalledProcessError) as context_manager:
      cmd_utils.run_command(_CMD_FAILED, verbose=False, shell=True,
                            raise_on_error=True, output=self.outstream)
    proc_err = context_manager.exception
    self.assertEqual(1, proc_err.returncode)
    self.assertEqual(_CMD_FAILED, proc_err.cmd)

  def test_change_working_directory(self):
    """Tests that cwd argument can be passed to subprocess.Popen()."""
    tmp_dir = tempfile.mkdtemp(prefix='cmd_utils_test')
    result = cmd_utils.run_command('pwd', verbose=False, shell=True,
                                   read_stdout=True, raise_on_error=False,
                                   cwd=tmp_dir, output=self.outstream)
    self.assertEqual('%s\n' % tmp_dir, result.stdoutdata)
    shutil.rmtree(tmp_dir)

if __name__ == '__main__':
  unittest.main()

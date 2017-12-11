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

"""Unit test for gsi_util.utils.cmd_utils."""

import logging
from logging import handlers
import shutil
import subprocess
import tempfile
import unittest

from gsi_util.utils import cmd_utils


class RunCommandTest(unittest.TestCase):

  def setUp(self):
    """Sets up logging output for assert checks."""
    log_entries = self._log_entries = []

    class Target(object):
      """The target handler to store log output."""

      def handle(self, record):
        log_entries.append((record.levelname, record.msg % record.args))

    self._handler = handlers.MemoryHandler(capacity=0, target=Target())
    logging.getLogger().addHandler(self._handler)

  def tearDown(self):
    """Removes logging handler."""
    logging.getLogger().removeHandler(self._handler)

  def test_command_sequence(self):
    result = cmd_utils.run_command(['echo', '123'])
    self.assertEqual((0, None, None), result)
    self.assertEqual(('INFO', "Command executed successfully: 'echo 123'"),
                     self._log_entries[0])

  def test_shell_command(self):
    result = cmd_utils.run_command('echo uses shell', shell=True)
    self.assertEqual((0, None, None), result)
    self.assertEqual(('INFO',
                      "Command executed successfully: 'echo uses shell'"),
                     self._log_entries[0])

  def test_error_logging(self):
    error_cmd = 'echo foo; echo bar; (echo 123; echo 456;)>&2; exit 1'
    cmd_utils.run_command(error_cmd, shell=True, raise_on_error=False)
    self.assertEqual(('ERROR', 'Failed to execute command: %r' % error_cmd),
                     self._log_entries[0])
    self.assertEqual(('ERROR', '  Return code: 1'), self._log_entries[1])
    self.assertEqual(('ERROR', "  stdout: 'foo\\nbar\\n'"),
                     self._log_entries[2])
    self.assertEqual(('ERROR', "  stderr: '123\\n456\\n'"),
                     self._log_entries[3])

  def test_read_stdout(self):
    result = cmd_utils.run_command('echo 123; echo 456; exit 5', shell=True,
                                   read_stdout=True, raise_on_error=False)
    self.assertEqual(5, result.returncode)
    self.assertEqual('123\n456\n', result.stdoutdata)
    self.assertEqual(None, result.stderrdata)

  def test_read_stderr(self):
    result = cmd_utils.run_command('(echo error1; echo error2)>&2; exit 3',
                                   shell=True, read_stderr=True,
                                   raise_on_error=False)
    self.assertEqual(3, result.returncode)
    self.assertEqual(None, result.stdoutdata)
    self.assertEqual('error1\nerror2\n', result.stderrdata)

  def test_read_stdout_and_stderr(self):
    result = cmd_utils.run_command('echo foo; echo bar>&2; exit 1',
                                   shell=True, read_stdout=True,
                                   read_stderr=True, raise_on_error=False)
    self.assertEqual(1, result.returncode)
    self.assertEqual('foo\n', result.stdoutdata)
    self.assertEqual('bar\n', result.stderrdata)

  def test_raise_on_error(self):
    error_cmd = 'echo foo; exit 1'
    with self.assertRaises(subprocess.CalledProcessError) as context_manager:
      cmd_utils.run_command(error_cmd, shell=True, raise_on_error=True)
    proc_err = context_manager.exception
    self.assertEqual(1, proc_err.returncode)
    self.assertEqual(error_cmd, proc_err.cmd)

  def test_change_working_directory(self):
    """Tests that cwd argument can be passed to subprocess.Popen()."""
    tmp_dir = tempfile.mkdtemp(prefix='cmd_utils_test')
    result = cmd_utils.run_command('pwd', shell=True,
                                   read_stdout=True, raise_on_error=False,
                                   cwd=tmp_dir)
    self.assertEqual('%s\n' % tmp_dir, result.stdoutdata)
    shutil.rmtree(tmp_dir)

if __name__ == '__main__':
  logging.basicConfig(format='%(message)s', level=logging.INFO)
  unittest.main()

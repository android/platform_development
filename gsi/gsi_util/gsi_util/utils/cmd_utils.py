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

"""Command-related utilities."""

from collections import namedtuple
import getpass
import logging
import subprocess


CommandResult = namedtuple('CommandResult', 'returncode stdoutdata, stderrdata')
PIPE = subprocess.PIPE


def run_command(command, read_stdout=False, read_stderr=False,
                log_stdout=False, log_stderr_on_error=False,
                raise_on_error=True, sudo=False, **kwargs):
  """Runs a command and returns the results.

  Args:
    command: A sequence of command arguments or else a single string.
    read_stdout: If True, includes stdout data in the returned tuple.
      Otherwise includes None in the returned tuple.
    read_stderr: If True, includes stderr data in the returned tuple.
      Otherwise includes None in the returned tuple.
    log_stdout: If True, logs stdout data.
    log_stderr_on_error: If true, logs stderro data when return code is nonzero.
    raise_on_error: If True, raise exception if return code is nonzero.
    sudo: Prepends 'sudo' to command if user is not root.
    **kwargs: the keyword arguments passed to subprocess.Popen().

  Returns:
    A namedtuple CommandResult(returncode, stdoutdata, stderrdata).
    The latter two fields will be set only when read_stdout/read_stderr
    is True, respectively. Otherwise, they will be None.

  Raises:
    OSError: Not such a command to execute, raised by subprocess.Popen().
    subprocess.CalledProcessError: The return code of the command is nonzero.
  """
  if sudo and getpass.getuser() != 'root':
    if kwargs.pop('shell', False):
      command = ['sudo', 'sh', '-c', command]
    else:
      command = ['sudo'] + command

  if kwargs.get('shell'):
    command_in_log = command
  else:
    command_in_log = ' '.join(arg for arg in command)

  if read_stdout or log_stdout:
    assert kwargs.get('stdout') in [None, PIPE]
    kwargs['stdout'] = PIPE
  if read_stderr or log_stderr_on_error:
    assert kwargs.get('stderr') in [None, PIPE]
    kwargs['stderr'] = PIPE

  need_communicate = (read_stdout or read_stderr or
                      log_stdout or log_stderr_on_error)
  proc = subprocess.Popen(command, **kwargs)
  if need_communicate:
    stdout, stderr = proc.communicate()
  else:
    proc.wait()  # no need to communicate; just wait.

  if proc.returncode != 0:
    logging.error('Failed to execute command: %r', command_in_log)
    logging.error('  Return code: %d', proc.returncode)
    if log_stdout:
      logging.error('  stdout: %r', stdout)
    if log_stderr_on_error:
      logging.error('  stderr: %r', stderr)
    if raise_on_error:
      raise subprocess.CalledProcessError(proc.returncode, command)
  else:
    logging.info('Command executed successfully: %r', command_in_log)
    if log_stdout:
      logging.info('  stdout: %r', stdout)

  return CommandResult(proc.returncode,
                       stdout if read_stdout else None,
                       stderr if read_stderr else None)

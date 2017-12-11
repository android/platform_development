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
import pipes
import subprocess
import sys


CommandResult = namedtuple('CommandResult', 'returncode stdoutdata, stderrdata')


def run_command(command, verbose=True, output=sys.stdout, raise_on_error=True,
                read_stdout=False, read_stderr=False, sudo=False, **kwargs):
  """Runs a command and returns the results.

  Args:
    command: A sequence of command arguments or else a single string.
    verbose: If True, writes verbose logs to output.
    output: The output stream to write logs.
    raise_on_error: If True, raise exception if return code is not zero.
    read_stdout: If True, includes stdout data in the returned tuple.
      Otherwise includes None in the returned tuple.
    read_stderr: If True, includes stderr data in the returned tuple.
      Otherwise includes None in the returned tuple.
    sudo: Prepends 'sudo' to command if user is not root.
    **kwargs: the keyword arguments passed to subprocess.Popen().

  Returns:
    A namedtuple CommandResult(returncode, stdoutdata, stderrdata).
    The latter two fields will be set only when read_stdout/read_stderr
    is True, respectively. Otherwise, they will be None.

  Raises:
    OSError: Not such a command to execute, raised by subprocess.Popen().
    subprocess.CalledProcessError: The return code of the command is not zero.
  """
  if sudo and getpass.getuser() != 'root':
    if kwargs.pop('shell', False):
      command = ['sudo', 'sh', '-c', command]
    else:
      command = ['sudo'] + command

  if kwargs.get('shell'):
    command_in_log = pipes.quote(command)
  else:
    command_in_log = pipes.quote(' '.join(arg for arg in command))

  kwargs['stdout'] = subprocess.PIPE
  kwargs['stderr'] = subprocess.PIPE
  proc = subprocess.Popen(command, **kwargs)
  stdout, stderr = proc.communicate()

  if proc.returncode != 0:
    output.write('Failed to execute command: {})\n'.format(command_in_log))
    output.write('  Return code: {})\n'.format(proc.returncode))
    output.write('  stdout: {})\n'.format(stdout))
    output.write('  stderr: {})\n'.format(stderr))
    if raise_on_error:
      raise subprocess.CalledProcessError(proc.returncode, command)
  elif verbose:
    output.write('Command executed successfully: {}\n'.format(command_in_log))

  return CommandResult(proc.returncode,
                       stdout if read_stdout else None,
                       stderr if read_stderr else None)

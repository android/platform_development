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


def fastboot_flash(partition_name, image_name=None, allow_error=False):
  """fastboot flash a partition with a given image."""

  cmd = ['fastboot', 'flash', partition_name]

  # image_name can be None, for `fastboot` to flash
  # ${ANDROID_PRODUCT_OUT}/{partition_name}.img.
  if image_name is not None:
    cmd.append(image_name)

  check_call(cmd, allow_error=allow_error)


def fastboot_erase(partition_name=None, allow_error=False):
  """fastboot erase a partition."""

  if partition_name is None:
    check_call(['fastboot', '-w'], allow_error=allow_error)
  else:
    check_call(['fastboot', 'erase', partition_name], allow_error=allow_error)


def fastboot_reboot():
  check_call(['fastboot', 'reboot'], allow_error=True)


def check_call(args, allow_error=False, **kwargs):
  """Executes subprocess.check_call().

  Args:
    args: the command sequence for subprocess.check_call().
    allow_error: won't raise CalledProcessError if True.
    **kwargs: the keyword arguments passed to subprocess.check_call().

  Raises:
    subprocess.CalledProcessError: The return code of the command is not zero.
  """

  try:
    subprocess.check_call(args, *kwargs)
  except subprocess.CalledProcessError as e:
    logging.critical('Failed to run %r, return code: %d', e.cmd, e.returncode)
    logging.critical('  Error output: %r', e.output)
    if not allow_error:
      raise


def run_command(command, raise_on_error=True, read_stdout=False,
                read_stderr=False, sudo=False, **kwargs):
  """Runs a command and returns the results.

  Args:
    command: A sequence of command arguments or else a single string.
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
    command_in_log = command
  else:
    command_in_log = ' '.join(arg for arg in command)

  kwargs['stdout'] = subprocess.PIPE
  kwargs['stderr'] = subprocess.PIPE
  proc = subprocess.Popen(command, **kwargs)
  stdout, stderr = proc.communicate()

  if proc.returncode != 0:
    logging.error('Failed to execute command: %r', command_in_log)
    logging.error('  Return code: %d', proc.returncode)
    logging.error('  stdout: %r', stdout)
    logging.error('  stderr: %r', stderr)
    if raise_on_error:
      raise subprocess.CalledProcessError(proc.returncode, command)
  else:
    logging.info('Command executed successfully: %r', command_in_log)

  return CommandResult(proc.returncode,
                       stdout if read_stdout else None,
                       stderr if read_stderr else None)

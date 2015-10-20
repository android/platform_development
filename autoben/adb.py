# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to communicate with Android device using adb command."""

import logging
import subprocess


class AdbCommand(object):
  """Executes command on remote Android device."""

  def __init__(self, serial_number=None, verbose=False):
    """Constructor.

    Args:
      serial_number: If specified, connect to the device.
      verbose: True to print out command.
    """
    self._serial_number = serial_number
    self._verbose = verbose

  def Run(self, shell_command):
    """Executes "adb shell shell_command".

    Args:
      shell_command: command to execute remotely.

    Returns:
      adb shell output.
    """
    if self._serial_number:
      command = ['adb', '-s', self._serial_number, 'shell']
    else:
      command = ['adb', 'shell']
    command.append('%s' % shell_command)
    if self._verbose:
      logging.debug('AdbCommand: %s', ' '.join(command))

    return subprocess.check_output(command)

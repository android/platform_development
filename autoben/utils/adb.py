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
"""Simple ADB interface for running commands in adb shell.

Upon instantiation, a connection to adb shell is established, and used by all
following commands. This prevents the overhead of reconnecting each time a
command is executed.

  Usage example:

  >>> adb = ADB()
  >>> lines = adb.run("echo Hello world")
  >>> lines[0]
  'Hello world'
"""
import pexpect


class ADB(object):
  """Simple ADB interface."""

  def __init__(self, adb_path="adb", adb_args="",
               prompt=ur"(\d+\|)?[a-z0-9_-]+@\w+:/ [#$]"):
    """Connects to adb shell upon instantiation.

    Args:
      adb_path: Path to adb executable.
      adb_args: Additional arguments passing to adb.
      prompt: Expected adb shell prompt.
    """
    self.conn = pexpect.spawnu("%s %s shell" % (adb_path, adb_args))
    self.conn.expect(prompt)
    self.prompt = prompt

  def run(self, cmd):
    """Runs command in adb shell.

    Args:
      cmd: The command.

    Returns:
      A list of resulting lines of output.
    """
    self.conn.sendline(cmd)
    self.conn.expect(self.prompt)
    return self.conn.before.strip().split("\r\r\n")[1:]

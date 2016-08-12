#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#            http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Provides functionality to interact with a device via `fastboot`."""

import os
import re
import subprocess


class FastbootError(Exception):
    """Something went wrong interacting with fastboot."""


class FastbootDevice(object):
    """Class to interact with a fastboot device."""

    # Prefix for INFO-type messages when printed by fastboot. If we want
    # to parse the output from an INFO message we need to strip this off.
    INFO_PREFIX = '(bootloader) '

    def __init__(self, path='fastboot'):
        """Initialization.

        Args:
            path: path to the fastboot executable to test with.

        Raises:
            FastbootError: Failed to find a device in fastboot mode.
        """
        self.path = path

        # Make sure the fastboot executable is available.
        try:
            subprocess.check_output([self.path, '--version'])
        except OSError:
            raise FastbootError('Could not execute `{}`'.format(self.path))

        # Make sure exactly 1 fastboot device is available if <specific device>
        # was not given as an argument. Do not try to find an adb device and
        # put it in fastboot mode, it would be too easy to accidentally
        # download to the wrong device.
        if not self._check_single_device():
            raise FastbootError('Requires exactly 1 device in fastboot mode')

    def _check_single_device(self):
        """Returns True if there is exactly one fastboot device attached.
           When ANDROID_SERIAL is set it checks that the device is available.
        """

        if 'ANDROID_SERIAL' in os.environ:
            try:
                self.getvar('product')
                return True
            except subprocess.CalledProcessError:
                return False
        devices = subprocess.check_output([self.path, 'devices']).splitlines()
        return len(devices) == 1 and devices[0].split()[1] == 'fastboot'

    def getvar(self, name):
        """Calls `fastboot getvar`.

        To query all variables (fastboot getvar all) use getvar_all()
        instead.

        Args:
            name: variable name to access.

        Returns:
            String value of variable |name| or None if not found.
        """
        try:
            output = subprocess.check_output([self.path, 'getvar', name],
                                             stderr=subprocess.STDOUT).splitlines()
        except subprocess.CalledProcessError:
            return None
        # Output format is <name>:<whitespace><value>.
        out = 0
        if output[0] == "< waiting for any device >":
            out = 1
        result = re.search(r'{}:\s*(.*)'.format(name), output[out])
        if result:
            return result.group(1)
        else:
            return None

    def getvar_all(self):
        """Calls `fastboot getvar all`.

        Returns:
            A {name, value} dictionary of variables.
        """
        output = subprocess.check_output([self.path, 'getvar', 'all'],
                                         stderr=subprocess.STDOUT).splitlines()
        all_vars = {}
        for line in output:
            result = re.search(r'(.*):\s*(.*)', line)
            if result:
                var_name = result.group(1)

                # `getvar all` works by sending one INFO message per variable
                # so we need to strip out the info prefix string.
                if var_name.startswith(self.INFO_PREFIX):
                    var_name = var_name[len(self.INFO_PREFIX):]

                # In addition to returning all variables the bootloader may
                # also think it's supposed to query a return a variable named
                # "all", so ignore this line if so. Fastboot also prints a
                # summary line that we want to ignore.
                if var_name != 'all' and 'total time' not in var_name:
                    all_vars[var_name] = result.group(2)
        return all_vars

    def flashall(self, wipe_user=True, slot=None, skip_secondary=False, quiet=True):
        """Calls `fastboot [-w] flashall`.

        Args:
            wipe_user: whether to set the -w flag or not.
            slot: slot to flash if device supports A/B, otherwise default will be used.
            skip_secondary: on A/B devices, flashes only the primary images if true.
            quiet: True to hide output, false to send it to stdout.
        """
        func = (subprocess.check_output if quiet else subprocess.check_call)
        command = [self.path, 'flashall']
        if slot:
            command.extend(['--slot', slot])
        if skip_secondary:
            command.append("--skip-secondary")
        if wipe_user:
            command.append('-w')
        func(command, stderr=subprocess.STDOUT)

    def flash(self, partition='cache', img=None, slot=None, quiet=True):
        """Calls `fastboot flash`.

        Args:
            partition: which partition to flash.
            img: path to .img file, otherwise the default will be used.
            slot: slot to flash if device supports A/B, otherwise default will be used.
            quiet: True to hide output, false to send it to stdout.
        """
        func = (subprocess.check_output if quiet else subprocess.check_call)
        command = [self.path, 'flash', partition]
        if img:
            command.append(img)
        if slot:
            command.extend(['--slot', slot])
        if skip_secondary:
            command.append("--skip-secondary")
        func(command, stderr=subprocess.STDOUT)

    def reboot(self, bootloader=False):
        """Calls `fastboot reboot [bootloader]`.

        Args:
            bootloader: True to reboot back to the bootloader.
        """
        command = [self.path, 'reboot']
        if bootloader:
            command.append('bootloader')
        subprocess.check_output(command, stderr=subprocess.STDOUT)

    def set_active(self, slot):
        """Calls `fastboot set_active <slot>`.

        Args:
            slot: The slot to set as the current slot."""
        command = [self.path, 'set_active', slot]
        subprocess.check_output(command, stderr=subprocess.STDOUT)

#!/usr/bin/env python3
#
# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""This module implements an Android.mk rewriter which fixes errors that are
caught by prebuilt ELF checker."""

from __future__ import print_function

import os.path
import re
import sys

from .android import find_android_build_top
from .readobj import readobj


class ErrorHandler(object):  # pylint: disable=too-few-public-methods
    """This class handles the errors that may occur during the rewriting
    work."""

    @staticmethod
    def error(line, fmt, *args):
        """This function prints an error message."""
        fmt = '{}: error: ' + fmt
        print(fmt.format(line, *args), file=sys.stderr)


class ModuleNameConverter(object):
    """This class converts DT_NEEDED names into build system module names."""

    @staticmethod
    def get_module_name(dt_needed):
        """Convert a DT_NEEDED name to the build system module name."""
        return re.sub('\\.so$', '', dt_needed)


    def get_module_names(self, dt_needed_entries):
        """Convert DT_NEEDED names into build system module names."""
        return set(self.get_module_name(name) for name in dt_needed_entries)


class Rewriter(object):  # pylint: disable=too-few-public-methods
    """This class rewrites the input Android.mk file and adds missing
    LOCAL_SHARED_LIBRARIES, LOCAL_MULTILIB, or LOCAL_CHECK_ELF_FILES."""


    _INCLUDE = re.compile('\\s*include\\s+\\$\\(([A-Za-z0-9_]*)\\)')
    _VAR = re.compile('([A-Za-z_][A-Za-z0-9_-]*)\\s*:?=\\s*(.*)$')


    def __init__(self, mk_path, variables=None, android_build_top=None):
        self._error_handler = ErrorHandler()

        self._mk_path = mk_path
        self._mk_dirname = os.path.dirname(mk_path)
        self._module_name_converter = ModuleNameConverter()

        self._variables = {}
        if variables:
            for key, value in variables.items():
                self._add_var(key, value, -1, -1)

        if android_build_top is None:
            self._android_build_top = find_android_build_top(mk_path)
        else:
            self._android_build_top = android_build_top


    def _read_prebuilt_file_path(self):
        file_var = self._variables.get('LOCAL_SRC_FILES')
        if file_var is not None:
            return os.path.join(self._mk_dirname, file_var[0])

        file_var = self._variables.get('LOCAL_PREBUILT_MODULE_FILE')
        if file_var is not None:
            return os.path.join(self._android_build_top, file_var[0])

        return None


    def _rewrite_build_prebuilt(self, stashed_lines, line_no):
        check_elf_files_var = self._variables.get('LOCAL_CHECK_ELF_FILES')
        if check_elf_files_var is not None and \
                check_elf_files_var[0] == 'false':
            return

        # Read the prebuilt ELF file
        prebuilt_file = self._read_prebuilt_file_path()
        if prebuilt_file is None:
            self._error_handler.error(
                line_no,
                'LOCAL_SRC_FILES and LOCAL_PREBUILT_MODULE_FILE are not '
                'defined')
            return

        if not os.path.exists(prebuilt_file):
            self._error_handler.error(
                line_no, 'Prebuilt file does not exist: "{}"', prebuilt_file)

        is_32bit, dt_soname, dt_needed = readobj(prebuilt_file)

        # Check whether LOCAL_MULTILIB is missing for 32-bit executables
        multilib_var = self._variables.get('LOCAL_MULTILIB')
        if not multilib_var and is_32bit:
            stashed_lines.append('LOCAL_MULTILIB := 32')

        # Check whether DT_SONAME matches with the file name
        filename = os.path.basename(prebuilt_file)
        if dt_soname and dt_soname != filename:
            stashed_lines.extend([
                '# Bypass prebuilt ELF check due to mismatched DT_SONAME',
                'LOCAL_CHECK_ELF_FILES := false',])
            return

        # Check LOCAL_SHARED_LIBRARIES
        shared_libs = self._module_name_converter.get_module_names(dt_needed)
        shared_libs_var = self._variables.get('LOCAL_SHARED_LIBRARIES')

        if not shared_libs_var:
            if shared_libs:
                stashed_lines.append('LOCAL_SHARED_LIBRARIES := ' +
                                     ' '.join(sorted(shared_libs)))
            return

        shared_libs_specified = set(re.split('[ \t\n]', shared_libs_var[0]))
        if shared_libs != shared_libs_specified:
            # Replace LOCAL_SHARED_LIBRARIES
            stashed_lines[shared_libs_var[1]:shared_libs_var[2]] = [
                'LOCAL_SHARED_LIBRARIES := ' + ' '.join(sorted(shared_libs))]


    def _add_var(self, key, value, start, end):
        self._variables[key] = (self._expand_vars(value), start, end)


    def _expand_vars(self, string):
        def _lookup_variable(match):
            key = match.group(1)
            try:
                return self._variables[key][0]
            except KeyError:
                # If we cannot find the variable, leave it as-is.
                return match.group(0)

        old_string = None
        while old_string != string:
            old_string = string
            string = re.sub('\\$\\(([A-Za-z][A-Za-z0-9_-]*)\\)',
                            _lookup_variable, old_string)
        return string


    def _clear_vars(self):
        self._variables = {key: value
                           for key, value in self._variables.items()
                           if not key.startswith('LOCAL_')}


    def _rewrite_lines(self, lines, out_file):
        stashed_lines = []

        def _flush_stashed_lines():
            for line in stashed_lines:
                print(line, file=out_file)
            stashed_lines[:] = []

        line_iter = enumerate(lines)
        for line_no, line in line_iter:
            match = self._INCLUDE.match(line)
            if match:
                command = match.group(1)
                if command == 'CLEAR_VARS':
                    self._clear_vars()
                elif command == 'BUILD_PREBUILT':
                    self._rewrite_build_prebuilt(stashed_lines, line_no)
                stashed_lines.append(line)
                _flush_stashed_lines()
                continue

            match = self._VAR.match(line)
            if match:
                start = len(stashed_lines)
                stashed_lines.append(line)

                key = match.group(1).strip()
                value = match.group(2).strip()

                while value.endswith('\\'):
                    line_no, line = next(line_iter, (-1, None))
                    if line is None:
                        value = value[:-1]
                        break
                    stashed_lines.append(line)
                    value = value[:-1] + line.strip()
                end = len(stashed_lines)

                self._add_var(key, value, start, end)
                continue

            stashed_lines.append(line)

        _flush_stashed_lines()


    def rewrite(self, out_file=sys.stdout):
        """This function reads the content of `self._mk_path`, rewrites build
        rules, and prints the rewritten build rules to `out_file`."""

        with open(self._mk_path, 'r') as input_file:
            lines = input_file.read().splitlines()

        self._rewrite_lines(lines, out_file)

#!/usr/bin/env python3

"""Fix prebuilt ELF check errors.

This script fixes prebuilt ELF check errors by updating LOCAL_SHARED_LIBRARIES,
adding LOCAL_MULTILIB, or adding LOCAL_CHECK_ELF_FILES.
"""

import argparse
import os
import os.path
import re
import subprocess
import sys


_INCLUDE = re.compile('\\s*include\\s+\\$\\(([A-Za-z0-9_]*)\\)')
_VAR = re.compile('([A-Za-z_][A-Za-z0-9_-]*)\\s*:?=\\s*(.*)$')

_ELF_CLASS = re.compile(
    '\\s*Class:\\s*(.*)$')
_DT_NEEDED = re.compile(
    '\\s*0x[0-9a-fA-F]+\\s+\\(NEEDED\\)\\s+Shared library: \\[(.*)\\]$')
_DT_SONAME = re.compile(
    '\\s*0x[0-9a-fA-F]+\\s+\\(SONAME\\)\\s+Library soname: \\[(.*)\\]$')


def find_android_build_top(path):
    path = os.path.dirname(os.path.abspath(path))
    prev = None
    while prev != path:
        if os.path.exists(os.path.join(path, '.repo', 'manifest.xml')):
            return path
        prev = path
        path = os.path.dirname(path)
    raise ValueError('failed to find ANDROID_BUILD_TOP')


def readelf(path):
    # Read ELF class (32-bit / 64-bit)
    proc = subprocess.Popen(['readelf', '-h', path], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    is_32bit = False
    for line in stdout.decode('utf-8').splitlines():
        match = _ELF_CLASS.match(line)
        if match:
            if match.group(1) == 'ELF32':
                is_32bit = True

    # Read DT_SONAME and DT_NEEDED
    proc = subprocess.Popen(['readelf', '-d', path], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    dt_soname = None
    dt_needed = set()
    for line in stdout.decode('utf-8').splitlines():
        match = _DT_NEEDED.match(line)
        if match:
            dt_needed.add(match.group(1))
            continue
        match = _DT_SONAME.match(line)
        if match:
            dt_soname = match.group(1)
            continue

    return is_32bit, dt_soname, dt_needed


def convert_dt_needed_to_module_name(dt_needed):
    return set(re.sub('\\.so$', '', name) for name in dt_needed)


def read_prebuilt_file_path(android_build_top, mk_path, variables):
    file_var = variables.get('LOCAL_SRC_FILES')
    if file_var is not None:
        return os.path.join(os.path.dirname(mk_path), file_var[0])
    file_var = variables.get('LOCAL_PREBUILT_MODULE_FILE')
    if file_var is not None:
        return os.path.join(android_build_top, file_var[0])
    return None


def rewrite_build_prebuilt(android_build_top, mk_path, stashed_lines,
                           variables, line_no):
    check_elf_files_var = variables.get('LOCAL_CHECK_ELF_FILES')
    if check_elf_files_var is not None and check_elf_files_var[0] == 'false':
        return

    # Read the prebuilt ELF file
    prebuilt_file = read_prebuilt_file_path(
        android_build_top, mk_path, variables)
    if prebuilt_file is None:
        print('error: Failed to read ELF file for $(BUILD_PREBUILT) at line {0}'
              .format(line_no), file=sys.stderr)
        return

    is_32bit, dt_soname, dt_needed = readelf(prebuilt_file)

    # Check whether LOCAL_MULTILIB is missing for 32-bit executables
    multilib_var = variables.get('LOCAL_MULTILIB')
    if not multilib_var and is_32bit:
        stashed_lines.append('LOCAL_MULTILIB := 32')

    # Check whether DT_SONAME matches with the file name
    filename = os.path.basename(prebuilt_file)
    if dt_soname and dt_soname != filename:
        stashed_lines.append(
            '# Bypass prebuilt ELF check due to '
            'mismatched sonames')
        stashed_lines.append(
            'LOCAL_CHECK_ELF_FILES := false')
        return

    # Check LOCAL_SHARED_LIBRARIES
    shared_libs = convert_dt_needed_to_module_name(dt_needed)
    shared_libs_var = variables.get('LOCAL_SHARED_LIBRARIES')

    if not shared_libs_var:
        if shared_libs:
            stashed_lines.append(
                'LOCAL_SHARED_LIBRARIES := ' + ' '.join(sorted(shared_libs)))
        return

    shared_libs_spec = set(re.split('[ \t\n]', shared_libs_var[0]))
    if shared_libs != shared_libs_spec:
        # Replace LOCAL_SHARED_LIBRARIES
        stashed_lines[shared_libs_var[1]:shared_libs_var[2]] = [
            'LOCAL_SHARED_LIBRARIES := ' + ' '.join(sorted(shared_libs))]


def rewrite_lines(mk_path, lines, variables=None):
    if variables is None:
        variables = {}

    stashed_lines = []
    android_build_top = find_android_build_top(mk_path)

    def flush_stashed_lines():
        nonlocal stashed_lines
        for line in stashed_lines:
            print(line)
        stashed_lines = []

    def expand(string):
        def lookup_variable(match):
            key = match.group(1)
            try:
                return variables[key][0]
            except KeyError:
                # If we cannot find the variable, leave it as-is.
                return match.group(0)

        old_string = None
        while old_string != string:
            old_string = string
            string = re.sub('\\$\\(([A-Za-z][A-Za-z0-9_-]*)\\)',
                            lookup_variable, old_string)
        return string

    line_iter = enumerate(lines)
    while True:
        line_no, line = next(line_iter, (-1, None))
        if line is None:
            break

        match = _INCLUDE.match(line)
        if match:
            command = match.group(1)
            if command == 'CLEAR_VARS':
                variables = { key: value
                              for key, value in variables.items()
                              if not key.startswith('LOCAL_') }
            elif command == 'BUILD_PREBUILT':
                rewrite_build_prebuilt(android_build_top, mk_path,
                                       stashed_lines, variables, line_no)
            stashed_lines.append(line)
            flush_stashed_lines()
            continue

        match = _VAR.match(line)
        if match:
            start = len(stashed_lines)
            stashed_lines.append(line)

            key = match.group(1).strip()
            value = match.group(2).strip()

            while value.endswith('\\'):
                line_no, line = next(line_iter, (-1, None))
                if line == None:
                    value = value[:-1]
                    break
                stashed_lines.append(line)
                value = value[:-1] + line.strip()
            end = len(stashed_lines)

            variables[key] = (expand(value), start, end)
            continue

        stashed_lines.append(line)

    flush_stashed_lines()



def rewrite(mk_path):
    with open(mk_path, 'r') as input_file:
        lines = input_file.read().splitlines()
    rewrite_lines(mk_path, lines)


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('android_mk', help='Path to Android.mk')
    parser.add_argument('--var', action='append', default=[])
    return parser.parse_args()


def main():
    args = _parse_args()
    variables = {}
    for var in args.var:
        if '=' in var:
            key, value = var.split('=', 1)
            key = key.strip()
            value = value.strip()
            variables[key] = value

    rewrite(args.android_mk)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3

import gzip
import os
import subprocess
import sys
import tempfile
import collections

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

try:
    AOSP_DIR = os.environ['ANDROID_BUILD_TOP']
except KeyError:
    print('error: ANDROID_BUILD_TOP environment variable is not set.',
          file=sys.stderr)
    sys.exit(1)

PROJECT_DIR = os.path.join(AOSP_DIR, 'development', 'vndk', 'tools', 'CPP2C',)

DEFAULT_CPPFLAGS = ['-std=c++17', '-x', 'c++'] # -x tells clang the input file is C++

BUILTIN_HEADERS_DIR = (
    os.path.join(AOSP_DIR, 'external', 'libcxx'),
    os.path.join(AOSP_DIR, 'prebuilts', 'clang-tools', 'linux-x86',
                 'clang-headers'),
)

EXE_NAME = 'cpp2c'


def read_content(output_path):
    with open(output_path, 'r') as f:
        return f.read().replace(PROJECT_DIR + '/', '') # TODO: remove replace command usage


def run_header_converter_on_file(input_path, output_path,
                                  flags=tuple(),
                                  cflags=tuple()):
    cmd = [EXE_NAME, input_path, '-o', output_path]

    cmd += flags
    cmd += ['--']

    if cflags:
        cmd += cflags

    for dir in BUILTIN_HEADERS_DIR:
        cmd += ['-I', dir]

    cmd += DEFAULT_CPPFLAGS

    subprocess.check_call(cmd)


def run_header_converter(input_path, 
                            output_path,
                            flags=tuple(),
                            cflags=tuple()):
    wrapperName = os.path.splitext(os.path.basename(input_path))[0] + 'Wrapper'
    wrapper_path = os.path.join(output_path, wrapperName)

    run_header_converter_on_file(input_path, output_path, flags, cflags)
    return [read_content(wrapper_path + ".h"), read_content(wrapper_path + ".cpp")]

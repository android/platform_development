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

DEFAULT_CPPFLAGS = ['-std=c++17']

EXE_NAME = 'cpp2c'


def read_content(output_path):
    with open(output_path, 'r') as f:
        return f.read().replace(PROJECT_DIR + '/', '') # TODO: remove replace command usage


def run_header_converter_on_file(input_path,
                                  flags=tuple(),
                                  cflags=tuple()):
    cmd = [EXE_NAME, input_path]

    cmd += flags
    cmd += ['--']

    if not cflags:
        cmd += DEFAULT_CPPFLAGS
    else:
        cmd += cflags

    subprocess.check_call(cmd)


def run_header_converter(input_path, 
                            flags=tuple(),
                            cflags=tuple()):
    with tempfile.TemporaryDirectory() as tmp:
        # TODO add --output param to the converted in order to actually write to a tmp file
        # and to avoid hardcoded paths
        run_header_converter_on_file(input_path, flags, cflags)
        return [read_content("./cwrapper.h"), read_content("./cwrapper.cpp")]
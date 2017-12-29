#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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
#
"""Utility functions for VNDK snapshot."""

import glob
import os
import re
import sys


def get_android_build_top():
    ANDROID_BUILD_TOP = os.getenv('ANDROID_BUILD_TOP')
    if not ANDROID_BUILD_TOP:
        print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
              '\'. build/envsetup.sh; lunch <build target>\'. Exiting script.')
        sys.exit(1)
    return ANDROID_BUILD_TOP


def join_realpath(root, *args):
    return os.path.realpath(os.path.join(root, *args))


def _get_dir_from_env(env_var, default):
    return os.path.realpath(os.getenv(env_var, default))


def get_out_dir(android_build_top):
    return _get_dir_from_env('OUT_DIR', join_realpath(android_build_top,
                                                      'out'))


def get_dist_dir(out_dir):
    return _get_dir_from_env('DIST_DIR', join_realpath(out_dir, 'dist'))


def get_snapshot_archs(install_dir):
    """Returns a list of VNDK snapshot target_archs under install_dir.

    Args:
      install_dir: string, absolute path of prebuilts/vndk/v{version}
    """
    target_archs = []
    for file in glob.glob('{}/*'.format(install_dir)):
        basename = os.path.basename(file)
        if os.path.isdir(file) and basename != 'NOTICE_FILES':
            target_archs.append(basename)
    return target_archs


def arch_from_path(path):
    """Extracts arch from given VNDK snapshot path.

    Args:
      path: string, path that starts with 'arch-{arch}-*/...'

    Returns:
      arch string, (e.g., 'arm' or 'arm64' or 'x86' or 'x86_64')
    """
    return path.split('/')[0].split('-')[1]


def target_arch_from_path(path):
    """Extracts target_arch from given VNDK snapshot path.

    Args:
      path: string, path relative to prebuilts/vndk/v{version}

    Returns:
      target_arch string, (e.g. 'arm64')
    """
    return path.split('/')[0]


def find(path, names):
    """Returns a list of files in a directory that match the given names.

    Args:
      path: string, absolute path of directory from which to find files
      names: list of strings, names of the files to find
    """
    found = []
    for root, _, files in os.walk(path):
        for file_name in sorted(files):
            if file_name in names:
                abspath = os.path.abspath(os.path.join(root, file_name))
                rel_to_root = abspath.replace(os.path.abspath(path), '')
                found.append(rel_to_root[1:])  # strip leading /
    return found

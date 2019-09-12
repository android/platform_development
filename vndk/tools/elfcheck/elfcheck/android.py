#!/usr/bin/env python3

"""This module contains utilities to extract Android build system variables."""

import os.path


def find_android_build_top(path):
    """This function finds ANDROID_BUILD_TOP by searching parent directories of
    `path`."""
    path = os.path.dirname(os.path.abspath(path))
    prev = None
    while prev != path:
        if os.path.exists(os.path.join(path, '.repo', 'manifest.xml')):
            return path
        prev = path
        path = os.path.dirname(path)
    raise ValueError('failed to find ANDROID_BUILD_TOP')

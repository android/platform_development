#!/usr/bin/env python3

"""Fix prebuilt ELF check errors.

This script fixes prebuilt ELF check errors by updating LOCAL_SHARED_LIBRARIES,
adding LOCAL_MULTILIB, or adding LOCAL_CHECK_ELF_FILES.
"""

import argparse

from elfcheck.rewriter import Rewriter


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('android_mk', help='Path to Android.mk')
    parser.add_argument('--var', action='append', default=[])
    return parser.parse_args()


def _parse_arg_var(args_var):
    variables = {}
    for var in args_var:
        if '=' in var:
            key, value = var.split('=', 1)
            key = key.strip()
            value = value.strip()
            variables[key] = value
    return variables


def main():
    """Main function"""
    args = _parse_args()
    rewriter = Rewriter(args.android_mk, _parse_arg_var(args.var))
    rewriter.rewrite()


if __name__ == '__main__':
    main()

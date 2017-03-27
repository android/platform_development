#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
import shutil
import re
import subprocess
import sys
import traceback

def makedirs(path, exist_ok):
    if exist_ok and os.path.isdir(path):
        return
    return os.makedirs(path)

# Path constants.
SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
AOSP_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, *['..'] * 4))
SOURCE_ABI_DUMP_EXT = '.lsdump'

def create_vndk_lib_name_filter(file_list_path):
    if not file_list_path:
        def accept_all_filenames(name):
            return True
        return accept_all_filenames

    with open(file_list_path, 'r') as f:
        lines = f.read().splitlines()

    patt = re.compile('^(?:' +
                      '|'.join('(?:' + re.escape(x) + ')' for x in lines) +
                      ')$')
    def accept_matched_filenames(name):
        return patt.match(name)
    return accept_matched_filenames


def create_source_abi_reference_dump(soong_dir, is_vndk_lib_name, args):
    target_arch =  get_build_var('TARGET_ARCH', args)
    target_arch_variant =  get_build_var('TARGET_ARCH_VARIANT', args)
    target_cpu_variant =  get_build_var('TARGET_CPU_VARIANT', args)

    target_2nd_arch =  get_build_var('TARGET_2ND_ARCH', args)
    target_2nd_arch_variant =  get_build_var('TARGET_2ND_ARCH_VARIANT', args)
    target_2nd_cpu_variant =  get_build_var('TARGET_2ND_CPU_VARIANT', args)

    target_dir = 'android_'+ target_arch + '_' + target_arch_variant + '_' + \
        target_cpu_variant
    target_2nd_arch_dir = 'android_'+ target_2nd_arch + '_' + \
        target_2nd_arch_variant + '_' + target_2nd_cpu_variant
    print('target_dir: ', target_dir)
    print('target_2nd_arch_dir', target_2nd_arch_dir)
    num_copied = 0
    for base, dirnames, filenames in os.walk(soong_dir):
        for filename in filenames:
          if not is_vndk_lib_name(filename):
              continue
          path = os.path.join(base, filename)
          source_dump_path = path + SOURCE_ABI_DUMP_EXT
          arch = None
          if target_dir in os.path.dirname(path):
              if os.path.exists(source_dump_path):
                  arch = target_arch + '_' + target_arch_variant + '_' +\
                      target_cpu_variant
          elif target_2nd_arch_dir in os.path.dirname(path):
              if os.path.exists(source_dump_path):
                  arch = target_2nd_arch + '_' + target_2nd_arch_variant + '_' + \
                  target_2nd_cpu_variant
          if arch is not None:
              copy_src_dump_to_dst(source_dump_path, arch)
              num_copied += 1
    return num_copied

def copy_src_dump_to_dst(source_dump_path, arch):
    source_dump_dir = os.path.join(AOSP_DIR, 'development', 'vndk', 'dumps', \
                                   arch)
    makedirs(source_dump_dir, exist_ok=True)
    dst = os.path.join(source_dump_dir, os.path.basename(source_dump_path))
    shutil.copyfile(source_dump_path, dst)
    print('msg: Source Based Reference dump created at : ', dst)

def get_build_var_from_build_system(name):
    """Get build system variable for the launched target."""
    if 'ANDROID_PRODUCT_OUT' not in os.environ:
        return None

    cmd = ['make', '--no-print-directory', '-f', 'build/core/config.mk',
           'dumpvar-' + name]

    environ = dict(os.environ)
    environ['CALLED_FROM_SETUP'] = 'true'
    environ['BUILD_SYSTEM'] = 'build/core'

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, env=environ,
                            cwd=AOSP_DIR)
    out, err = proc.communicate()
    return out.decode('utf-8').strip()

def get_build_var(name, args):
    """Get build system variable either from command line option or build
    system."""
    value = getattr(args, name.lower(), None)
    return value if value else get_build_var_from_build_system(name)

def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--vndk-list', help='VNDK library list')
    args = parser.parse_args()
    num_processed = 0
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    num_processed += create_source_abi_reference_dump(soong_dir,\
          create_vndk_lib_name_filter(args.vndk_list), args)
    print()
    print('msg: Processed', num_processed, 'libraries')
if __name__ == '__main__':
    main()

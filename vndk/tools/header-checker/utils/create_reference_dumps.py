#!/usr/bin/env python3

import os
import re
import sys
import argparse

from utils import make_library
from utils import find_lib_lsdump
from utils import get_build_var
from utils import AOSP_DIR
from utils import read_output_content
from utils import copy_reference_dump

def get_vndk_libs(vndk_list_path):
    with open(vndk_list_path, 'r') as f:
        lines = f.read().splitlines()
    return lines

def create_source_abi_reference_dumps(soong_dir, vndk_libs, args):
    target_arch =  get_build_var('TARGET_ARCH')
    target_arch_variant =  get_build_var('TARGET_ARCH_VARIANT')
    target_cpu_variant =  get_build_var('TARGET_CPU_VARIANT')

    target_2nd_arch =  get_build_var('TARGET_2ND_ARCH')
    target_2nd_arch_variant =  get_build_var('TARGET_2ND_ARCH_VARIANT')
    target_2nd_cpu_variant =  get_build_var('TARGET_2ND_CPU_VARIANT')
    ref_dump_dir_stem = os.path.join(args.ref_dump_dir, args.version)
    ref_dump_dir_insertion = 'source-based'
    num_libs_copied = 0
    for vndk_lib in vndk_libs:
        if args.make_libs:
            make_library(vndk_lib)
        arch_lsdump_path = find_lib_lsdump(vndk_lib, target_arch,
                                           target_arch_variant,
                                           target_cpu_variant)
        # Copy the contents of the lsdump into it's corresponding
        # reference  directory.
        num_libs_copied += copy_reference_dump(arch_lsdump_path,
                                               ref_dump_dir_stem,
                                               ref_dump_dir_insertion,
                                               target_arch)

        second_arch_lsdump_path = find_lib_lsdump(
            vndk_lib, target_2nd_arch, target_2nd_arch_variant,
            target_2nd_cpu_variant)

        num_libs_copied += copy_reference_dump(
            second_arch_lsdump_path, ref_dump_dir_stem,
            ref_dump_dir_insertion, target_2nd_arch)

    return num_libs_copied


def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', help='VNDK version')
    parser.add_argument('--vndk-list', help='VNDK version')
    parser.add_argument('-ref-dump-dir', help='VNDK reference abi dump dir')
    parser.add_argument('-make-libs', action ="store_true", default = False,
                        help='make libraries before copying dumps')
    args = parser.parse_args()
    num_processed = 0
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    num_processed += create_source_abi_reference_dumps(soong_dir,\
          get_vndk_libs(args.vndk_list), args)
    print()
    print('msg: Processed', num_processed, 'libraries')
if __name__ == '__main__':
    main()

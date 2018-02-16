#!/usr/bin/env python3

import os
import re
import sys
import argparse

from utils import (make_library, find_lib_lsdumps, get_build_var, AOSP_DIR,
                   read_output_content, copy_reference_dumps)

class Target(object):
    def __init__(self, has_2nd):
        extra = '_2ND' if has_2nd else ''
        self.arch = get_build_var('TARGET{}_ARCH'.format(extra))
        self.arch_variant = get_build_var('TARGET{}_ARCH_VARIANT'.format(extra))
        self.cpu_variant = \
            get_build_var('TARGET{}_CPU_VARIANT'.format(extra))

def find_and_copy_lib_lsdumps(arch, arch_variant, cpu_variant, soong_dir,
                              ref_dump_dir_stem, ref_dump_dir_insertion,
                              core_or_vendor_shared_str):
    arch_lsdump_paths = find_lib_lsdumps(arch, arch_variant, cpu_variant,
                                         soong_dir, core_or_vendor_shared_str)
    # Copy the contents of the lsdump into it's corresponding
    # reference  directory.
    return copy_reference_dumps(arch_lsdump_paths, ref_dump_dir_stem,
                                ref_dump_dir_insertion, arch)


def create_source_abi_reference_dumps(soong_dir, args):
    # TODO: make this a function
    ref_dump_dir_stem_vndk = os.path.join(args.ref_dump_dir, 'vndk')
    ref_dump_dir_stem_vndk = os.path.join(ref_dump_dir_stem_vndk, args.version)
    ref_dump_dir_stem_ndk = os.path.join(args.ref_dump_dir, 'ndk')
    ref_dump_dir_stem_ndk = os.path.join(ref_dump_dir_stem_ndk, args.version)
    ref_dump_dir_insertion = 'source-based'
    num_libs_copied = 0
    for target in [Target(True), Target(False)]:
        num_libs_copied += find_and_copy_lib_lsdumps(
            target.arch, target.arch_variant, target.cpu_variant, soong_dir,
            ref_dump_dir_stem_vndk, ref_dump_dir_insertion, '_vendor_shared')

        num_libs_copied += find_and_copy_lib_lsdumps(
            target.arch, target.arch_variant, target.cpu_variant, soong_dir,
            ref_dump_dir_stem_ndk, ref_dump_dir_insertion, '_core_shared')

    return num_libs_copied


def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', help='VNDK version')
    parser.add_argument('-ref-dump-dir',
                        help='directory to copy reference abi dumps into')
    args = parser.parse_args()
    num_processed = 0
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    num_processed += create_source_abi_reference_dumps(soong_dir, args)
    print()
    print('msg: Processed', num_processed, 'libraries')
if __name__ == '__main__':
    main()

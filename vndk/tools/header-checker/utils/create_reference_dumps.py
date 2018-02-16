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
                              core_or_vendor_shared_str, libs):
    arch_lsdump_paths = find_lib_lsdumps(arch, arch_variant, cpu_variant,
                                         soong_dir, core_or_vendor_shared_str,
                                         libs)
    # Copy the contents of the lsdump into it's corresponding
    # reference  directory.
    cpu_variant_str = ''
    if cpu_variant != 'generic':
        cpu_variant_str = '-' + cpu_variant
    return copy_reference_dumps(arch_lsdump_paths, ref_dump_dir_stem,
                                ref_dump_dir_insertion,
                                arch + '-' + arch_variant + cpu_variant_str)

def get_ref_dump_dir_stem(args, vndk_or_ndk):
    version = args.version
    if version != '' and version[0].isdigit() == False :
        version = 'current'
    bitness_str = '32' if get_build_var('BINDER32BIT') == 'true' else '64'
    ref_dump_dir_stem = os.path.join(args.ref_dump_dir, vndk_or_ndk)
    ref_dump_dir_stem = os.path.join(ref_dump_dir_stem, version)
    ref_dump_dir_stem = os.path.join(ref_dump_dir_stem, bitness_str)

    return ref_dump_dir_stem

def create_source_abi_reference_dumps(soong_dir, args):
    ref_dump_dir_stem_vndk = get_ref_dump_dir_stem(args, 'vndk')
    ref_dump_dir_stem_ndk = get_ref_dump_dir_stem(args, 'ndk')
    ref_dump_dir_insertion = 'source-based'
    num_libs_copied = 0
    for target in [Target(True), Target(False)]:
        num_libs_copied += find_and_copy_lib_lsdumps(
            target.arch, target.arch_variant, target.cpu_variant, soong_dir,
            ref_dump_dir_stem_vndk, ref_dump_dir_insertion, '_vendor_shared',
            args.libs)

        num_libs_copied += find_and_copy_lib_lsdumps(
            target.arch, target.arch_variant, target.cpu_variant, soong_dir,
            ref_dump_dir_stem_ndk, ref_dump_dir_insertion, '_core_shared',
            args.libs)

    return num_libs_copied


def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', help='VNDK version',
                        default=get_build_var('PLATFORM_VNDK_VERSION'))
    parser.add_argument('-libs', help='libs to create references for',
                        action='append')
    parser.add_argument('-ref-dump-dir',
                        help='directory to copy reference abi dumps into',
                        default=AOSP_DIR + '/prebuilts/abi-dumps')
    args = parser.parse_args()
    num_processed = 0
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    num_processed += create_source_abi_reference_dumps(soong_dir, args)
    print()
    print('msg: Processed', num_processed, 'libraries')
if __name__ == '__main__':
    main()

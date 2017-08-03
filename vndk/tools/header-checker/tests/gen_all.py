#!/usr/bin/env python3

import os
import re
import sys

import_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
import_path = os.path.abspath(os.path.join(import_path, 'utils'))
sys.path.insert(1, import_path)

from utils import run_header_checker
from utils import make_library
from utils import find_lib_lsdump
from utils import get_build_var
from utils import AOSP_DIR
from utils import read_output_content
from utils import copy_reference_dump

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
INPUT_DIR = os.path.join(SCRIPT_DIR, 'input')
EXPECTED_DIR = os.path.join(SCRIPT_DIR, 'expected')
REFERENCE_DUMP_DIR = os.path.join(SCRIPT_DIR, 'reference_dumps')

DEFAULT_CFLAGS = ['-x', 'c++', '-std=c++11']

FILE_EXTENSIONS = ['h', 'hpp', 'hxx', 'cpp', 'cc', 'c']

TEST_GOLDEN_LIBS = ['libc_and_cpp',
                    'libc_and_cpp_whole_static',
                    'libc_and_cpp_without_Cfunction',
                    'libc_and_cpp_with_unused_struct',
                    'libgolden_cpp']

TEST_DIFF_LIBS = ['libgoldencpp_vtable_diff',
                  'libgoldencpp_member_diff',
                  'libgoldencpp_enum_extended',
                  'libgoldencpp_enum_diff']

def main():
    patt = re.compile(
        '^.*\\.(?:' + \
        '|'.join('(?:' + re.escape(ext) + ')' for ext in FILE_EXTENSIONS) + \
        ')$')

    input_dir_prefix_len = len(INPUT_DIR) + 1
    for base, dirnames, filenames in os.walk(INPUT_DIR):
        for filename in filenames:
            if not patt.match(filename):
                print('ignore:', filename)
                continue

            input_path = os.path.join(base, filename)
            input_rel_path = input_path[input_dir_prefix_len:]
            output_path = os.path.join(EXPECTED_DIR, input_rel_path)

            print('generating', output_path, '...')
            output_content = run_header_checker(input_path, DEFAULT_CFLAGS)

            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            with open(output_path, 'w') as f:
                f.write(output_content)
     #TODO: Make the test frame-work support multiple arch configurations not
     # just 2

    target_arch =  get_build_var('TARGET_ARCH')
    target_arch_variant =  get_build_var('TARGET_ARCH_VARIANT')
    target_cpu_variant =  get_build_var('TARGET_CPU_VARIANT')

    target_2nd_arch =  get_build_var('TARGET_2ND_ARCH')
    target_2nd_arch_variant =  get_build_var('TARGET_2ND_ARCH_VARIANT')
    target_2nd_cpu_variant =  get_build_var('TARGET_2ND_CPU_VARIANT')

    for test_golden_lib in TEST_GOLDEN_LIBS:
        make_library(test_golden_lib)
        arch_lsdump_path = find_lib_lsdump(test_golden_lib, target_arch,
                                           target_arch_variant,
                                           target_cpu_variant)
        # Copy the contents of the lsdump into it's corresponding reference
        # directory.
        copy_reference_dump(arch_lsdump_path, REFERENCE_DUMP_DIR,
                            '', target_arch)
        second_arch_lsdump_path = find_lib_lsdump(test_golden_lib,
                                                  target_2nd_arch,
                                                  target_2nd_arch_variant,
                                                  target_2nd_cpu_variant)
        copy_reference_dump(second_arch_lsdump_path, REFERENCE_DUMP_DIR,
                            '', target_2nd_arch)

    return 0

if __name__ == '__main__':
    sys.exit(main())

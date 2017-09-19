#!/usr/bin/env python3

import os
import sys
import csv
import argparse
from utils import Artifact
from utils import find_lib_artifact
from utils import get_build_var
from utils import AOSP_DIR
from subprocess import Popen, PIPE

FUNCTIONS_REMOVED_STR='functions_removed'
FUNCTIONS_ADDED_STR='functions_added'
FUNCTIONS_CHANGED_STR='function_diffs'

GLOBAL_VARS_REMOVED_STR='global_vars_removed'
GLOBAL_VARS_ADDED_STR='global_var_added'
GLOBAL_VARS_CHANGED_STR='global_var_diffs'

ENUM_TYPES_CHANGED_STR='enum_type_diffs'

RECORD_TYPES_CHANGED_STR='record_type_diffs'

class Target(object):
    def __init__(self, has_2nd):
        extra = '_2ND' if has_2nd else ''
        self.arch = get_build_var('TARGET{}_ARCH'.format(extra))
        self.arch_variant = get_build_var('TARGET{}_ARCH_VARIANT'.format(extra))
        self.cpu_variant = \
            get_build_var('TARGET{}_CPU_VARIANT'.format(extra))


def grep_count_from_file(attr, tf):
    proc_grep = Popen(['grep', attr, tf], stdout=PIPE)
    proc_wc = Popen(['wc', '-l' ], stdin=proc_grep.stdout, stdout=PIPE)
    return proc_wc.communicate()[0].decode("utf-8").strip()

def parse_tf(lib_name, tf, arch):
    return {'Library' : lib_name,
            'Functions Removed' : grep_count_from_file(FUNCTIONS_REMOVED_STR, tf),
            'Functions Added' : grep_count_from_file(FUNCTIONS_ADDED_STR, tf),
            'Functions Changed' : grep_count_from_file(FUNCTIONS_CHANGED_STR, tf),
            'Global Vars Removed' : grep_count_from_file(GLOBAL_VARS_REMOVED_STR, tf),
            'Global Vars Added' : grep_count_from_file(GLOBAL_VARS_ADDED_STR, tf),
            'Global Vars Changed' : grep_count_from_file(GLOBAL_VARS_CHANGED_STR, tf),
            'Record Types Changed' : grep_count_from_file(RECORD_TYPES_CHANGED_STR, tf),
            'Enum Types Changed' : grep_count_from_file(ENUM_TYPES_CHANGED_STR, tf)}

def generate_csv_file(args):
    """ Go throught the out dir and search for all abidiff files. For each
        abidiff file, print statistics out into a csv file"""
    output_csv = args.output_csv
    soong_dir = os.path.join(AOSP_DIR, 'out', 'soong', '.intermediates')
    if args.out_dir is not None:
        soong_dir = os.path.join(args.out_dir, 'soong', '.intermediates')
    for target in [Target(True), Target(False)]:
        fields = ['Library', 'Functions Removed', 'Functions Added',
                  'Functions Changed', 'Global Vars Removed',
                  'Global Vars Added', 'Global Vars Changed',
                  'Record Types Changed', 'Enum Types Changed']

        with open(output_csv + '_' + target.arch + '.csv', 'w') as f:
            csv_writer = csv.DictWriter(f, fieldnames=fields)
            csv_writer.writeheader()
            arch_abidiff_paths = find_lib_artifact(soong_dir, target.arch,
                                                   target.arch_variant,
                                                   target.cpu_variant,
                                                   Artifact.ABI_DIFF)
            for lib in arch_abidiff_paths.keys():
                csv_writer.writerow(parse_tf(lib, arch_abidiff_paths[lib],
                                             target.arch))

            print(len(arch_abidiff_paths), 'diff reports found for ', target.arch)
    return

def main():
    # Parse command line options.
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-csv', help='Output csv file stem')
    parser.add_argument('--out-dir', help='Path to out dir')
    args = parser.parse_args()
    generate_csv_file(args)

if __name__ == '__main__':
    main()

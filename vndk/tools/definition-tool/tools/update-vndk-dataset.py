#!/usr/bin/env python3

import argparse
import csv
import posixpath
import sys

PATH_COL = 0
TAG_COL = 1

def derive_vndk_path(path):
    dirname, basename = posixpath.split(path)
    if posixpath.basename(dirname) != '${LIB}':
        raise ValueError('dirname not end with ${LIB}')
    return posixpath.join(dirname, 'vndk', basename)

def duplicate_vndk_paths(data):
    paths = set(row[PATH_COL] for row in data)
    result = list(data)

    for row in data:
        # Ignore non-VNDK libraries.
        if row[TAG_COL] != 'VNDK':
            continue

        # Derive VNDK path from old path.
        path = row[PATH_COL]
        try:
            vndk_path = derive_vndk_path(path)
        except ValueError:
            print('error: {} does not have vndk path'.format(path))
            continue

        # Don't add new entries if the vndk path exists.
        if vndk_path in paths:
            continue

        # Duplicate the entry and append it to the end.
        row = list(row)
        row[PATH_COL] = vndk_path
        result.append(row)

    return result

def main():
    parser =argparse.ArgumentParser()
    parser.add_argument('tag_file')
    parser.add_argument('-o', '--output')
    args = parser.parse_args()

    with open(args.tag_file, 'r') as fp:
        data = list(csv.reader(fp))

    if data:
        data = duplicate_vndk_paths(data)

    with open(args.output, 'w') as fp:
        writer = csv.writer(fp, lineterminator='\n')
        writer.writerows(data)

if __name__ == '__main__':
    sys.exit(main())

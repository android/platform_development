#!/usr/bin/env python3

import argparse
import csv
import posixpath
import sys

VNDK_SP_HW = {
    'android.hidl.memory@1.0-impl.so',
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    args = parser.parse_args()

    with open(args.input, 'r') as fp:
        data = list(csv.reader(fp))

    soname_tag = dict()
    path_tag = dict()

    for path, tag, comments in data[1:]:
        # Check duplicated path.
        if path in path_tag:
            print('error: duplicated path:', path)
            sys.exit(1)
        path_tag[path] = tag

        # Check mismatched sonames.
        soname = posixpath.basename(path)
        if soname != 'libz.so':
            try:
                orig = soname_tag[soname]
                if orig != tag:
                    print('error: tag mismatch:', path)
                    sys.exit(1)
            except KeyError:
                soname_tag[soname] = tag

    # Check missing vndk.
    for path, tag, comments in data[1:]:
        soname = posixpath.basename(path)

        if soname in VNDK_SP_HW:
            vndk_path = posixpath.join('/system/${LIB}/vndk-sp/hw', soname)
            fwk_path = posixpath.join('/system/${LIB}/hw', soname)
            if vndk_path not in path_tag:
                print('error: missing vndk-sp-hw path:', vndk_path)
            if fwk_path not in path_tag:
                print('error: missing fwk-hw path:', fwk_path)
            continue

        if tag in {'VNDK', 'VNDK-Indirect'}:
            vndk_path = posixpath.join('/system/${LIB}/vndk', soname)
            fwk_path = posixpath.join('/system/${LIB}', soname)
            if vndk_path not in path_tag:
                print('error: missing vndk path:', vndk_path)
            if fwk_path not in path_tag:
                print('error: missing fwk path:', fwk_path)

        if tag in {'VNDK-SP', 'VNDK-SP-Indirect', 'VNDK-SP-Indirect-Private'}:
            vndk_sp_path = posixpath.join('/system/${LIB}/vndk-sp', soname)
            fwk_path = posixpath.join('/system/${LIB}', soname)
            if vndk_sp_path not in path_tag:
                print('error: missing vndk-sp path:', vndk_sp_path)
            if fwk_path not in path_tag:
                print('error: missing fwk path:', fwk_path)

if __name__ == '__main__':
    main()

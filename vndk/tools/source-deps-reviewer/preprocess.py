#!/usr/bin/env python3
import json
from utils import *
import argparse
import os
import re
import subprocess
import sys
import tempfile
import hashlib
import collections
from subprocess import call

def to_json(args, processed_file):
    # Load all matched grep.

    data = {}
    patt = re.compile('([^:]+):(\\d+):(.*)$')
    suspect = set()
    with open(processed_file, 'r') as fp:
        for line in fp:
            match = patt.match(line)
            if not match:
                continue

            file_path = match.group(1)
            line_no = int(match.group(2))
            data[file_path + ':' + str(line_no)] = ([], [])

    with open(args.output, 'w') as f:
        json.dump(data, f)

def process_grep(args, raw_grep_file):
    patt = re.compile(b'([^:]+):(\\d+):(.*)$')

    kw_patt = args.pattern.encode('utf-8')
    if not args.regex:
        kw_patt = b'\\b' + re.escape(kw_patt) + b'\\b'
    kw_patt = re.compile(kw_patt)

    suspect = collections.defaultdict(list)

    with open(raw_grep_file, 'rb') as fp:
        for line in fp:
            match = patt.match(line)
            if not match:
                continue

            file_path = match.group(1)
            line_no = match.group(2)
            code = match.group(3)

            file_name = os.path.basename(file_path)
            file_name_root, file_ext = os.path.splitext(file_name)


            # Check file name.
            if file_ext.lower() in FILE_EXT_BLACK_LIST:
                continue
            if file_name in FILE_NAME_BLACK_LIST:
                continue
            if any(patt in file_path for patt in PATH_PATTERN_BLACK_LIST):
                continue

            # Check matched line (quick filter).
            if not kw_patt.search(sanitize_code(code)):
                continue

            suspect[file_path].append((file_path, line_no, code))

    suspect = sorted(suspect.items())

    with open('processed.txt', 'wb') as out:
        for file_path, entries in suspect:
            # be careful here
            # path = os.path.join(args.android_root, file_path.decode('utf-8'))
            path = file_path.decode('utf-8')
            with open(path, 'rb') as f:
                code = sanitize_code_matched(f.read())
                ''' why ?????
                if file_path == b'hardware/libhardware/hardware.c':
                    with open('debug.txt', 'wb') as xx:
                        xx.write(code)
                '''
                if not kw_patt.search(sanitize_code(code)):
                    print('deep-filter:', file_path.decode('utf-8'))
                    continue

            for ent in entries:
                out.write(ent[0] + b':' + ent[1] + b':' + ent[2] + b'\n')

def prepare(android_root):
    parser = argparse.ArgumentParser()
    #parser.add_argument('--android-root', required=True)
    parser.add_argument('--android-root', default=android_root)
    parser.add_argument('-o', '--output', default='data.json')
    # grep pattern
    parser.add_argument('-p', '--pattern', default='dlopen')
    # defaults to False
    parser.add_argument('--regex', action='store_true')
    args = parser.parse_args()

    if not os.path.exists('raw_grep.txt') or is_empty_file('raw_grep.txt'):
        with open('raw_grep.txt', 'w') as f:
            call(['grep', args.pattern, '-RHn', args.android_root], stdout=f)
            process_grep(args, 'raw_grep.txt')


    if not os.path.exists(args.output) or is_empty_file(args.output):
        to_json(args, 'processed.txt')

if __name__ == '__main__':
    prepare('../test')

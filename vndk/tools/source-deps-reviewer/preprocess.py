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

raw_grep = 'raw_grep.txt'
processed = 'processed.txt'
data_path = 'data.json'
pattern = 'dlopen'
is_regex = False

def to_json():
    # Load all matched grep.
    data = {}
    patt = re.compile('([^:]+):(\\d+):(.*)$')
    suspect = set()
    with open(processed, 'r') as fp:
        for line in fp:
            match = patt.match(line)
            if not match:
                continue

            file_path = match.group(1)
            line_no = int(match.group(2))
            data[file_path + ':' + str(line_no)] = ([], [])

    with open(data_path, 'w') as f:
        json.dump(data, f)

def process_grep(android_root):
    patt = re.compile(b'([^:]+):(\\d+):(.*)$')

    kw_patt = pattern.encode('utf-8')
    if not is_regex:
        kw_patt = b'\\b' + re.escape(kw_patt) + b'\\b'
    kw_patt = re.compile(kw_patt)

    suspect = collections.defaultdict(list)

    with open(raw_grep, 'rb') as fp:
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

    with open(processed, 'wb') as out:
        for file_path, entries in suspect:
            # be careful here
            path = os.path.join(android_root, file_path.decode('utf-8'))
            with open(path, 'rb') as f:
                code = sanitize_code_matched(f.read())
                if not kw_patt.search(sanitize_code(code)):
                    print('deep-filter:', file_path.decode('utf-8'))
                    continue

            for ent in entries:
                out.write(ent[0] + b':' + ent[1] + b':' + ent[2] + b'\n')

def prepare(android_root):
    android_root = os.path.expanduser(android_root)
    if not os.path.exists(data_path) or is_empty_file(data_path):
        with open(raw_grep, 'w') as f:
            call([ 'grep', pattern, '-RHn'], cwd=os.path.expanduser(android_root), stdout=f)
        process_grep(android_root)
        to_json()


if __name__ == '__main__':
    prepare('~/mtk')

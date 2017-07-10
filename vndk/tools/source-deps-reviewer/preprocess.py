#!/usr/bin/env python3
import argparse
import bisect
import collections
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from subprocess import call

csearchindex = '.csearchindex'
data_path = 'data.json'

FILE_EXT_BLACK_LIST = {
    b'.1',
    b'.ac',
    b'.cmake',
    b'.html',
    b'.info',
    b'.la',
    b'.m4',
    b'.map',
    b'.md',
    b'.py',
    b'.rst',
    b'.sh',
    b'.sym',
    b'.txt',
    b'.xml',
}

FILE_NAME_BLACK_LIST = {
    b'CHANGES.0',
    b'ChangeLog',
    b'config.h.in',
    b'configure',
    b'configure.in',
    b'configure.linaro',
    b'libtool',
}

PATH_PATTERN_BLACK_LIST = (
    b'autom4te.cache',
    b'dejagnu',
    b'llvm/Config/Config',
)
def is_empty_file(file_path):
    return os.stat(file_path).st_size == 0

def sanitize_code_unmatched(code):
    # Remove unmatched quotes until EOL.
    code = re.sub(b'"[^"]*$', b'', code)
    # Remove unmatched C comments.
    code = re.sub(b'/\\*.*$', b'', code)
    return code

def sanitize_code_matched(code):
    # Remove matching quotes.
    code = re.sub(b'"(?:[^"\\\\]|(?:\\\\.))*"', b'', code)
    # Remove C++ comments.
    code = re.sub(b'//[^\\r\\n]*$', b'', code)
    # Remove matched C comments.
    code = re.sub(b'/\\*(?:[^*]|(?:\\*+[^*/]))*\\*+/', b'', code)
    return code

def sanitize_code(code):
    return sanitize_code_unmatched(sanitize_code_matched(code))

def to_json(processed):
    # Load all matched grep.
    data = {}
    patt = re.compile('([^:]+):(\\d+):(.*)$')
    suspect = set()
    for line in processed.decode('utf-8').split('\n'):
        match = patt.match(line)
        if not match:
            continue

        file_path = match.group(1)
        line_no = int(match.group(2))
        data[file_path + ':' + str(line_no)] = ([], [])

    with open(data_path, 'w') as f:
        json.dump(data, f, sort_keys=True, indent=4)

def add_to_json(processed):
    # Load all matched grep.
    with open(data_path, 'r') as f:
        data = json.load(f)

    patt = re.compile('([^:]+):(\\d+):(.*)$')
    suspect = set()
    for line in processed.decode('utf-8').split('\n'):
        match = patt.match(line)
        if not match:
            continue

        file_path = match.group(1)
        line_no = int(match.group(2))
        data[file_path + ':' + str(line_no)] = ([], [])


    with open(data_path, 'w') as f:
        json.dump(data, f, sort_keys=True, indent=4)

def process_grep(raw_grep, android_root, pattern, is_regex):
    patt = re.compile(b'([^:]+):(\\d+):(.*)$')

    kw_patt = pattern.encode('utf-8')
    if not is_regex:
        kw_patt = b'\\b' + re.escape(kw_patt) + b'\\b'
    kw_patt = re.compile(kw_patt)

    suspect = collections.defaultdict(list)

    for line in raw_grep.split(b'\n'):

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

    processed = b''
    for file_path, entries in suspect:
        # be careful here
        path = os.path.join(android_root, file_path.decode('utf-8'))
        with open(path, 'rb') as f:
            code = sanitize_code_matched(f.read())
            if not kw_patt.search(sanitize_code(code)):
                print('deep-filter:', file_path.decode('utf-8'))
                continue

        for ent in entries:
            processed += (ent[0] + b':' + ent[1] + b':' + ent[2] + b'\n')

    return processed

def prepare(android_root, pattern, is_regex):
    android_root = os.path.expanduser(android_root)
    print('building csearchindex ...')
    subprocess.call(['cindex', android_root])
    raw_grep = subprocess.check_output([ 'csearch', '-n', pattern], cwd=os.path.expanduser(android_root))
    to_json( process_grep(raw_grep, android_root, pattern, is_regex) )

def add_pattern(android_root, pattern, is_regex):
    raw_grep = subprocess.check_output([ 'csearch', '-n', pattern], cwd=os.path.expanduser(android_root))
    add_to_json( process_grep(raw_grep, android_root, pattern, is_regex) )

if __name__ == '__main__':
    prepare(android_root='test', pattern='dlopen', is_regex=False)
    add_pattern(android_root='test', pattern='dlopen', is_regex=False)


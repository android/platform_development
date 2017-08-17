#!/usr/bin/env python3

from sourcedr.config import *
from sourcedr.data_utils import data_exist
from sourcedr.data_utils import init_pattern
from sourcedr.data_utils import load_data
from sourcedr.data_utils import merge
from sourcedr.data_utils import save_data

from subprocess import call
import collections
import json
import os
import re
import subprocess

class ClikeFilter(object):
    def __init__(self, skip_literals=True, skip_comments=True):
        self.skip_literals = skip_literals
        self.skip_comments = skip_comments

    def process(self, code):
        if self.skip_comments:
            # Remove // comments.
            code = re.sub(b'//[^\\r\\n]*$', b'', code)
            # Remove matched /* */ comments.
            code = re.sub(b'/\\*(?:[^*]|(?:\\*+[^*/]))*\\*+/', b'', code)
        if self.skip_literals:
            # Remove matching quotes.
            code = re.sub(b'"(?:[^"\\\\]|(?:\\\\.))*"', b'', code)
            code = re.sub(b'\'(?:[^\'\\\\]|(?:\\\\.))*\'', b'', code)
        return code

class PyFilter(object):
    def __init__(self, skip_literals=True, skip_comments=True):
        self.skip_literals = skip_literals
        self.skip_comments = skip_comments

    def process(self, code):
        if self.skip_comments:
            # Remove # comments
            code = re.sub(b'#[^\\r\\n]*$')
        if self.skip_literals:
            # Remove matching quotes.
            code = re.sub(b'"(?:[^"\\\\]|(?:\\\\.))*"', b'', code)
            code = re.sub(b'\'(?:[^\'\\\\]|(?:\\\\.))*\'', b'', code)
        return code

class AssemblyFilter(object):
    def __init__(self, skip_literals=True, skip_comments=True):
        self.skip_literals = skip_literals
        self.skip_comments = skip_comments

    def process(self, code):
        if self.skip_comments:
            # Remove @ comments
            code = re.sub(b'@[^\\r\\n]*$', b'', code)
            # Remove // comments.
            code = re.sub(b'//[^\\r\\n]*$', b'', code)
            # Remove matched /* */ comments.
            code = re.sub(b'/\\*(?:[^*]|(?:\\*+[^*/]))*\\*+/', b'', code)
        return code

class MkFilter(object):
    def __init__(self, skip_literals=True, skip_comments=True):
        self.skip_literals = skip_literals
        self.skip_comments = skip_comments

    def process(self, code):
        if self.skip_comments:
            # Remove # comments
            code = re.sub(b'#[^\\r\\n]*$')
        return code

class CodeSearch(object):
    @staticmethod
    def create_default(android_root, index_path='csearchindex'):
        clike = ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', '.java']
        assembly = ['.s', '.S']
        python = ['.py']
        mk = ['.mk']
        cs = CodeSearch(android_root, index_path)
        cs.add_filter(clike, ClikeFilter())
        cs.add_filter(assembly, AssemblyFilter())
        cs.add_filter(python, PyFilter())
        cs.add_filter(mk, MkFilter())
        return cs

    def __init__(self, android_root, index_path):
        self.android_root = android_root
        self.env = dict(os.environ)
        self.env["CSEARCHINDEX"] = os.path.abspath(index_path)
        self.filters = {}

    def add_filter(self, exts, Filter):
        for ext in exts:
            self.filters[ext] = Filter

    def build_index(self):
        android_root = os.path.expanduser(self.android_root)
        print('building csearchindex for the directory ' + android_root + '...')
        subprocess.call(['cindex', android_root], env=self.env)

    def sanitize_code(self, file_path):
        with open(file_path, 'rb') as f:
            code = f.read()

        f, ext = os.path.splitext(file_path)
        code = self.filters[ext].process(code)

        if file_path == 'Android.bp':
            if skip_comments:
                # Remove // comments.
                code = re.sub(b'//[^\\r\\n]*$', b'', code)

        return code

    def process_grep(self, raw_grep, pattern, is_regex):
        pattern = pattern.encode('utf-8')
        if not is_regex:
            pattern = re.escape(pattern)
        pattern = re.compile(pattern)

        patt = re.compile(b'([^:]+):(\\d+):(.*)$')
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
            path = os.path.join(self.android_root, file_path.decode('utf-8'))
            if not pattern.search(self.sanitize_code(path)):
                continue

            suspect[file_path].append((file_path, line_no, code))

        suspect = sorted(suspect.items())

        processed = b''
        for file_path, entries in suspect:
            for ent in entries:
                processed += (ent[0] + b':' + ent[1] + b':' + ent[2] + b'\n')

        return processed

    # patterns and is_regexs are lists
    def find(self, patterns, is_regexs):
        # they shouldn't be empty
        assert patterns and is_regexs
        processed = b''
        for pattern, is_regex in zip(patterns, is_regexs):
            if not is_regex:
                pattern = re.escape(pattern)
            try:
                raw_grep = subprocess.check_output(
                    ['csearch', '-n', pattern],
                    cwd=os.path.expanduser(self.android_root),
                    env=self.env)
            except subprocess.CalledProcessError as e:
                if e.output == b'':
                    print('nothing found')
                    return
            processed += self.process_grep(raw_grep, pattern, is_regex)
        self.to_json(processed)

    def add_pattern(self, pattern, is_regex):
        if not is_regex:
            pattern = re.escape(pattern)
        try:
            raw_grep = subprocess.check_output(
                ['csearch', '-n', pattern],
                cwd=os.path.expanduser(self.android_root),
                env=self.env)
        except subprocess.CalledProcessError as e:
            if e.output == b'':
                print('nothing found')
                return
        processed = self.process_grep(raw_grep, pattern, is_regex)
        self.add_to_json(processed)

    def to_json(self, processed):
        data = {}
        suspect = set()
        patt = re.compile('([^:]+):(\\d+):(.*)$')
        for line in processed.decode('utf-8').split('\n'):
            match = patt.match(line)
            if not match:
                continue
            data[line] = ([], [])

        # if old data exists, perform merge
        if data_exist():
            old_data = load_data()
            data = merge(old_data, data)

        save_data(data)

    def add_to_json(self,processed):
        # Load all matched grep.
        data = load_data()
        patt = re.compile('([^:]+):(\\d+):(.*)$')
        for line in processed.decode('utf-8').split('\n'):
            match = patt.match(line)
            if not match:
                continue
            data[line] = ([], [])

        save_data(data)

if __name__ == '__main__':
    # Initialize a codeSearch engine for the directory 'test'
    engine = CodeSearch.create_default('test', 'csearchindex')
    # Build the index file for the directory
    engine.build_index()
    # This sets up the search engine and save it to database
    engine.find(patterns=['dlopen', 'cosine'], is_regexs=[False, False])

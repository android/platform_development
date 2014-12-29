#!/usr/bin/python
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#            http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Generate ICU stable C API wrapper source.

This script parses all the header files specified by the ICU module names. For
each function marked as ICU stable, it generates a wrapper function to be
called by NDK, which in turn calls the available function at runtime. The tool
relies on libclang to parse header files, which is a component provided by
Clang.

The generated library code will be dumped to standard output, which can be
redirected to a C file, e.g. `./gen-ndk-icu.py > icundk.c`.

Reference to ICU4C stable C APIs:
http://icu-project.org/apiref/icu4c/files.html
"""
from __future__ import absolute_import
from __future__ import print_function

import os
import site

import jinja2


THIS_DIR = os.path.dirname(os.path.realpath(__file__))
ANDROID_TOP = os.path.realpath(os.path.join(THIS_DIR, '../../..'))


def android_path(*args):
    """Returns the absolute path to a directory within the Android tree."""
    return os.path.join(ANDROID_TOP, *args)


def strip_space(from_str):
    """Strip out spaces from string f."""
    return from_str.replace(' ,', ',').replace(' *', '*').replace(' ]', ']')


def parse_parameters(tokens, name):
    """Parse the parameters from given tokens."""
    start = 0
    param_types = []
    param_vars = []
    has_va = False
    last_param = None
    errorcode_var = None
    i = 0
    tokens.append(',')
    for token in tokens:
        if token == ',':
            strs = tokens[start:i]
            # no argument, either f() or f(void)
            if not strs or ('void' in strs and len(strs) == 1):
                param_types.append('void')
                param_vars.append('')
            # variable argument list
            elif '...' in strs and len(strs) == 1:
                has_va = True
                last_param = param_vars[-1]  # must have at least one fixed arg
                va_idx = VA_FUNCTIONS[name][1]
                param_types.insert(va_idx, 'va_list')
                param_vars.insert(va_idx, 'args')
            # argument with array type
            elif '[' in strs:
                idx = strs.index('[')
                param_types.append(
                    strip_space(' '.join(strs[:idx-1])) + ' ' +
                    strip_space(' '.join(strs[idx:])))
                param_vars.append(strs[idx-1])
            else:
                param_type = strip_space(' '.join(tokens[start:i-1]))
                if param_type == 'UErrorCode*':
                    errorcode_var = tokens[i-1]
                param_types.append(param_type)
                param_vars.append(tokens[i-1])
            start = i + 1
        i += 1
    tokens = tokens[:-1]
    param_list = strip_space(' '.join(tokens))

    return (', '.join(param_types), ', '.join(param_vars), param_list, has_va,
            last_param, errorcode_var)


def short_name(name):
    """Trim the given file name to 'unicode/xyz.h'."""
    return name[name.rfind('unicode/'):]


def is_valid_function(tokens):
    """Check if the given tokens look like a function."""
    set_tokens = set(tokens)
    if '(' not in set_tokens or ')' not in set_tokens:
        return False
    if 'U_EXPORT2' not in set_tokens:
        return False
    return True


class Function(object):
    """Hold the information for a parsed function."""

    def __init__(self, tokens, file_name, module):
        left_paren = tokens.index('(')
        right_paren = tokens.index(')')
        self.name = tokens[left_paren - 1]
        # display will be used in dlsym and may be identical to others
        # for functions with variable argument lists.
        self.display = self.name
        if self.name in VA_FUNCTIONS:
            self.display = VA_FUNCTIONS[self.name][0]
        self.file_name = short_name(file_name)
        self.module = module
        self.handle = 'handle_' + module
        self.result_type = strip_space(' '.join(tokens[:left_paren-2]))
        (self.param_types, self.param_vars, self.param_list, self.has_va,
         self.last_param, self.errorcode_var) = parse_parameters(
             tokens[left_paren+1:right_paren], self.name)
        self.return_void = self.result_type == 'void'


class Header(object):
    """Parse all the functions from a header file."""

    def __init__(self, file_name, cursor, module):
        self.functions = []
        self.file_name = file_name
        self.cursor = cursor
        self.module = module

    def build_functions(self, processed_functions):
        """Parse all the functions in current header."""
        import clang.cindex  # pylint: disable=import-error
        visible_functions = set()
        for child in self.cursor.get_children():
            if (child.kind == clang.cindex.CursorKind.FUNCTION_DECL and
                    child.location.file.name == self.file_name):
                visible_functions.add(child.spelling)

        token_buffer = []
        is_recording = False

        for token in self.cursor.get_tokens():
            if (token.kind == clang.cindex.TokenKind.IDENTIFIER and
                    token.spelling == 'U_STABLE'):
                is_recording = True
                token_buffer = []
            elif (token.kind == clang.cindex.TokenKind.PUNCTUATION and
                  token.spelling == ';'):
                if is_recording:
                    if is_valid_function(token_buffer):
                        func = Function(token_buffer, self.file_name,
                                        self.module)
                        # Only process functions that are visible to the
                        # preprocessor (i.e. clang) and hasn't been processed
                        # yet (a counter case is u_strlen in ICU 46).
                        if (func.name in visible_functions and
                                func.name not in processed_functions):
                            self.functions.append(func)
                    is_recording = False
                token_buffer = []
            elif token.kind == clang.cindex.TokenKind.COMMENT:
                continue
            elif is_recording:
                token_buffer.append(token.spelling)

        return self.functions


# Module handles are hard-coded in the tool, e.g. handle_common.
MODULES = {
    'common': 'libicuuc.so',
    'i18n': 'libicui18n.so',
}


# Functions w/ variable argument lists (...) need special care to call
# their corresponding v- versions that accept a va_list argument. Note that
# although '...' will always appear as the last parameter, its v- version
# may put the va_list arg in a different place. Hence we provide an index
# to indicate the position.
#
# e.g. 'umsg_format': ('umsg_vformat', 3) means in the wrapper function of
# 'umsg_format', it will call 'umsg_vformat' instead, with the va_list arg
# inserted as the 3rd argument.
VA_FUNCTIONS = {
    'u_formatMessage': ('u_vformatMessage', 5),
    'u_parseMessage': ('u_vparseMessage', 5),
    'u_formatMessageWithError': ('u_vformatMessageWithError', 6),
    'u_parseMessageWithError': ('u_vparseMessageWithError', 5),
    'umsg_format': ('umsg_vformat', 3),
    'umsg_parse': ('umsg_vparse', 4),
    'utrace_format': ('utrace_vformat', 4),
}


def main():
    """Program entry point."""
    # Load clang.cindex
    site.addsitedir(android_path('external/clang/bindings/python'))
    import clang.cindex  # pylint: disable=import-error

    # Set up LD_LIBRARY_PATH to include libclang.so, libLLVM.so, and etc.  Note
    # that setting LD_LIBRARY_PATH with os.putenv() sometimes doesn't help.
    clang.cindex.Config.set_library_path(
        android_path('prebuilts/clang/host/linux-x86/clang-4053586/lib64'))
    index = clang.cindex.Index.create()

    icu_path = android_path('external/icu/icu4c/source')

    clang_flags = [
        '-x',
        'c',
        '-std=c99',
        '-DUCONFIG_USE_LOCAL',
        '-DU_DISABLE_RENAMING=1',
        '-DU_SHOW_CPLUSPLUS_API=0',
        '-DU_HIDE_DRAFT_API',
        '-DU_HIDE_DEPRECATED_API',
        '-DU_HIDE_INTERNAL_API',
    ]
    clang_flags += ['-I{}/{}'.format(icu_path, m) for m in MODULES]

    includes = []
    all_functions = []
    processed = set()

    for module in MODULES:
        path = '{}/{}/unicode'.format(icu_path, module)
        files = [os.path.join(path, f)
                 for f in os.listdir(path) if f.endswith('.h')]

        for file_path in files:
            tunit = index.parse(file_path, clang_flags)
            header = Header(file_path, tunit.cursor, module)
            functions = header.build_functions(processed)

            if functions:
                all_functions += functions
                processed |= set([function.name for function in functions])
                includes.append(short_name(file_path))

    # Dump the generated file
    jinja = jinja2.Environment(loader=jinja2.FileSystemLoader(
        os.path.join(THIS_DIR, 'templates')))
    jinja.trim_blocks = True
    jinja.lstrip_blocks = True
    data = {
        'functions': all_functions,
        'icu_headers': includes,
        'icu_modules': MODULES,
    }
    print(jinja.get_template('icundk.cpp').render(data))


if __name__ == '__main__':
    main()

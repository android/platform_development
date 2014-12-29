#!/usr/bin/python
"""Generate ICU stable C API wrapper library 'libndk_icu.a'.

It parses all the header files specified by the ICU module names. For each
function marked as ICU stable, it generates a wrapper function to be called
by NDK, which in turn calls the available function at runtime. The tool
relies on libclang to parse header files, and mako template to generate
wrapper functions. PYTHONPATH needs to include the path to clang and mako,
such as .../clang/bindings/python.

The script should be under 'development/ndk/sources/android/ndk_icu' for
correct path resolution.

The generated library will be dumped to stardard output, which can be
redirected to a C file, e.g. './gen-ndk-icu.py > ndk_icu.c'.

Reference to ICU C stable API headers:
http://icu-project.org/apiref/icu4c/files.html
"""

import os
import clang.cindex

from mako.template import Template


def strip_space(s):
    """Strip out spaces from string s."""
    return s.replace(' ,', ',').replace(' *', '*').replace(' ]', ']')


def parse_parameters(tokens):
    """Parse the parameters from given tokens."""
    start = 0
    param_types = []
    param_vars = []
    has_va = False
    last_param = None
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
                param_types.append('...')
            # argument with array type
            elif '[' in strs:
                idx = strs.index('[')
                param_types.append(strip_space(' '.join(strs[:idx-1])) + ' ' +
                                   strip_space(' '.join(strs[idx:])))
                param_vars.append(strs[idx-1])
            else:
                param_types.append(strip_space(' '.join(tokens[start:i-1])))
                param_vars.append(tokens[i-1])
            start = i + 1
        i += 1
    tokens = tokens[:-1]
    param_list = strip_space(' '.join(tokens))

    return (', '.join(param_types), ', '.join(param_vars),
            param_list, has_va, last_param)


def short_name(name):
    """Trim the given file name to 'unicode/xyz.h'."""
    return name[name.rfind('unicode/'):]


def is_valid_function(tokens):
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
        self.file_name = short_name(file_name)
        self.module = module
        self.handle = 'handle_' + module
        self.result_type = ' '.join(tokens[:left_paren - 2])
        (self.param_types, self.param_vars, self.param_list, self.has_va,
         self.last_param) = parse_parameters(tokens[left_paren+1:right_paren])
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
        visible_functions = set()
        for c in self.cursor.get_children():
            if (c.kind == clang.cindex.CursorKind.FUNCTION_DECL and
                c.location.file.name == self.file_name):
                visible_functions.add(c.spelling)

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


# Presumably current pwd is 'development/ndk/sources/android/ndk_icu'
base_path = '../../../../../external/icu/icu4c/source'
modules = {'common': 'libicuuc.so',
           'i18n': 'libicui18n.so'}

if __name__ == '__main__':
    # Load clang.cindex
    try:
        index = clang.cindex.Index.create()
    except clang.cindex.LibclangError:
        print 'ERROR: Please set LD_LIBRARY_PATH for libclang.so.'
        exit(-1)

    clang_flags = ['-x', 'c', '-std=c99',
                   '-DUCONFIG_USE_LOCAL',
                   '-DU_DISABLE_RENAMING=1',
                   '-DU_SHOW_CPLUSPLUS_API=0',
                   '-DU_HIDE_DRAFT_API',
                   '-DU_HIDE_DEPRECATED_API',
                   '-DU_HIDE_INTERNAL_API',
                   '-DU_HIDE_SYSTEM_API',
                  ]
    clang_flags += ['-I%s/%s' % (base_path, m) for m in modules]

    includes = []
    collection = []
    processed = set()

    for m in modules:
        path = '%s/%s/unicode' % (base_path, m)
        files = [os.path.join(path, f)
                 for f in os.listdir(path) if f.endswith('.h')]

        for f in files:
            tu = index.parse(f, clang_flags)
            header = Header(f, tu.cursor, m)
            functions = header.build_functions(processed)

            if functions:
                collection += functions
                processed |= set([function.name for function in functions])
                includes.append(short_name(f))

    # Dump the generated file
    tpl = Template(filename='ndk_icu_tmpl.mako')
    print tpl.render(functions=collection, includes=includes, modules=modules)


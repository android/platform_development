#!/usr/bin/python
"""Generate ICU4C stable C API changes matrix.

The tool processes each ICU version and then dumps the matrix. The tool
relies on libclang to parse header files, which is a component provided by
Clang.

Clang can be downloaded at http://llvm.org/releases/download.html. There is an
INSTALL.TXT for installation instructions after unpacking the source. This tool
requires two environment variables properly set for libclang. The first one is
PYTHONPATH to include the path for Clang python bindings, which resides under
</path/to/clang/source>/bindings/python. The other is LD_LIBRARY_PATH to
include the path for runtime library libclang.so.

The ICU sources to be compared should be placed in sub-directories, with their
names defined in 'versions' variable.

The generated matrix will be dumped to stardard output, which can be redirected
to a TXT file, e.g. './gen-icu4c-api-matrix.py > icu4c_api_matrix.txt'.

Reference to ICU4C stable C APIs:
http://icu-project.org/apiref/icu4c/files.html
"""

import os
import sys
import clang.cindex


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
            if len(strs) == 0 or ('void' in strs and len(strs) == 1):
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
                param_types.append(' '.join(strs[:idx-1]) + ' ' +
                                   ' '.join(strs[idx:]))
                param_vars.append(strs[idx-1])
            else:
                param_types.append(' '.join(tokens[start:i-1]))
                param_vars.append(tokens[i-1])
            start = i + 1
        i += 1
    tokens = tokens[:-1]
    param_list = ' '.join(tokens).replace(' ,', ',')

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
        self.name = tokens[left_paren-1]
        self.file_name = short_name(file_name)
        self.module = module
        self.handle = 'handle_' + module
        self.result_type = ' '.join(tokens[:left_paren-2])
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

    def build_functions(self, processed):
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
                        short = short_name(self.file_name)
                        # Only process functions that are visible to the
                        # preprocessor (i.e. clang) and hasn't been processed
                        # yet (a counter case is u_strlen in ICU 46).
                        if (func.name in visible_functions and
                            func.name not in processed):
                            self.functions.append(func.name)
                            processed.add(func.name)
                            # Also add the function to the global collection
                            if short not in all_functions:
                                all_functions[short] = ([func.name],
                                                        set([func.name]))
                            elif func.name not in all_functions[short][1]:
                                all_functions[short][0].append(func.name)
                                all_functions[short][1].add(func.name)
                    is_recording = False
                token_buffer = []
            elif token.kind == clang.cindex.TokenKind.COMMENT:
                continue
            elif is_recording:
                token_buffer.append(token.spelling)

        return self.functions


def dump_all_functions():
    """Dump all the functions."""
    for func in all_functions:
        print func
        print '[%d]:' % len(all_functions[func][0]), all_functions[func][0]


def dump_matrix(lists):
    """Dump the matrix of stable APIs."""
    n_lists = len(lists)
    print '%-40s|  %s' % ('ICU VERSIONS', '  '.join(versions))
    print '-'*80

    sets = [set(l) for l in lists]

    for header in all_functions:
        print '  %-38s|' % ('<' + header + '>')
        for func in all_functions[header][0]:
            flags = []
            changed = False
            for i in range(n_lists):
                if func in sets[i]:
                    flags.append('+')
                else:
                    flags.append('-')
                    changed = True
            if not changed:
                print '%-40s|' % (func)
            else:
                print '%-40s|  %s' % (func, '   '.join(flags))
        print '%-40s|' % ''

    print '-'*80
    print '%-40s| %s' % ('TOTAL', ' '.join(['%3d' % len(l) for l in lists]))
    print '-'*80
    print '%-40s|  %s' % ('ICU VERSIONS', '  '.join(versions))

# ICU source codes should be placed under 44/, 46/, and etc.
versions = ['44', '46', '48', '49', '50', '51', '52', '53', '54']
modules = {'common': 'libicuuc.so',
           'i18n': 'libicui18n.so'}

all_functions = {}

if __name__ == '__main__':
    """Main entry of the tool."""
    # Load clang.cindex
    try:
        index = clang.cindex.Index.create()
    except clang.cindex.LibclangError:
        print 'ERROR: Please set LD_LIBRARY_PATH for libclang.so.'
        exit(-1)

    collections = []
    for ver in versions:
        print >>sys.stderr, 'Processing %s' % ver
        base_path = '%s/source' % ver
        clang_flags = ['-x', 'c', '-std=c99',
                       '-DUCONFIG_USE_LOCAL',
                       '-DU_DISABLE_RENAMING=1',
                       '-DU_SHOW_CPLUSPLUS_API=0',
                       '-DU_HIDE_DRAFT_API',
                       '-DU_HIDE_DEPRECATED_API',
                       '-DU_HIDE_INTERNAL_API',
                      ]
        clang_flags += ['-I%s/%s' % (base_path, m) for m in modules]

        includes = []
        collection = []

        for m in modules:
            path = '%s/%s/unicode' % (base_path, m)
            files = [os.path.join(path, f)
                     for f in os.listdir(path) if f.endswith('.h')]

            for f in files:
                tu = index.parse(f, clang_flags)
                header = Header(f, tu.cursor, m)
                functions = header.build_functions(set(collection))

                if functions:
                    collection += functions
                    includes.append(short_name(f))

        collections.append(collection)

    dump_matrix(collections)
#    dump_all_functions()


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

The generated library code is written to icundk.cpp in the same directory as
this file.

Reference to ICU4C stable C APIs:
http://icu-project.org/apiref/icu4c/files.html
"""
from __future__ import absolute_import
from __future__ import print_function

import logging
import os
import site
import sys
import textwrap

import jinja2


THIS_DIR = os.path.dirname(os.path.realpath(__file__))
ANDROID_TOP = os.path.realpath(os.path.join(THIS_DIR, '../../..'))


def android_path(*args):
    """Returns the absolute path to a directory within the Android tree."""
    return os.path.join(ANDROID_TOP, *args)


site.addsitedir(android_path('external/clang/bindings/python'))
import clang.cindex  # pylint: disable=import-error,wrong-import-position


CLANG_PATH = android_path('prebuilts/clang/host/linux-x86/clang-r328903')


def logger():
    """Returns the module level logger."""
    return logging.getLogger(__name__)


def short_header_path(name):
    """Trim the given file name to 'unicode/xyz.h'."""
    return name[name.rfind('unicode/'):]


def get_children_by_kind(cursor, kind):
    """Returns a generator of cursor's children of a specific kind."""
    for child in cursor.get_children():
        if child.kind == kind:
            yield child


def is_function_visible(decl):
    """Returns True if the function has default visibility."""
    visible = False
    vis_attrs = get_children_by_kind(
        decl, clang.cindex.CursorKind.VISIBILITY_ATTR)
    for child in vis_attrs:
        visible = child.spelling == 'default'
    return visible


def should_process_function(decl, file_name, seen_functions):
    """Returns True if this function needs to be processed."""
    if decl.kind != clang.cindex.CursorKind.FUNCTION_DECL:
        return False
    if decl.location.file.name != file_name:
        return False
    if decl.spelling in seen_functions:
        return False
    if not is_function_visible(decl):
        return False
    return True


def decl_has_stable_tag(decl):
    """Returns True if the given decl has a doxygen stable tag."""
    if not decl.raw_comment:
        return False
    if '@stable' in decl.raw_comment:
        return True
    if '\\stable' in decl.raw_comment:
        return True
    return False


def func_is_whitelisted(decl):
    """Returns True if this function is stable despite the lack of @stable."""
    whitelist = (
        # This is marked with @stable, but since the #ifdef comes after the
        # doxygen comment libclang doesn't expose it as part of the decl.
        'uregex_openC',

        # Not intended to be called directly, but are used by @stable macros.
        'utf8_nextCharSafeBody',
        'utf8_appendCharSafeBody',
        'utf8_prevCharSafeBody',
        'utf8_back1SafeBody',
    )

    return decl.spelling in whitelist


class Function(object):
    """A visible function found in an ICU header."""

    # Functions w/ variable argument lists (...) need special care to call
    # their corresponding v- versions that accept a va_list argument. Note that
    # although '...' will always appear as the last parameter, its v- version
    # may put the va_list arg in a different place. Hence we provide an index
    # to indicate the position.
    #
    # e.g. 'umsg_format': ('umsg_vformat', 3) means in the wrapper function of
    # 'umsg_format', it will call 'umsg_vformat' instead, with the va_list arg
    # inserted as the 3rd argument.
    KNOWN_VA_FUNCTIONS = {
        'u_formatMessage': ('u_vformatMessage', 5),
        'u_parseMessage': ('u_vparseMessage', 5),
        'u_formatMessageWithError': ('u_vformatMessageWithError', 6),
        'u_parseMessageWithError': ('u_vparseMessageWithError', 5),
        'umsg_format': ('umsg_vformat', 3),
        'umsg_parse': ('umsg_vparse', 4),
        'utrace_format': ('utrace_vformat', 4),
    }

    def __init__(self, name, result_type, params, is_variadic, is_stable,
                 module):
        self.name = name
        self.result_type = result_type
        self.params = params
        self.is_variadic = is_variadic
        self.is_stable = is_stable

        # callee will be used in dlsym and may be identical to others for
        # functions with variable argument lists.
        self.callee = self.name
        if self.is_variadic:
            self.callee = Function.KNOWN_VA_FUNCTIONS[self.name][0]
            self.last_param = self.params[-1][1]
        self.handle = 'handle_' + module
        self.return_void = self.result_type == 'void'

    @classmethod
    def from_cursor(cls, cursor, module):
        """Creates a Function object from the decl at the cursor."""
        if cursor.type.kind != clang.cindex.TypeKind.FUNCTIONPROTO:
            raise ValueError(textwrap.dedent("""\
                {}'s type kind is {}, expected TypeKind.FUNCTIONPROTO.
                {} Line {} Column {}""".format(
                    cursor.spelling,
                    cursor.type.kind,
                    cursor.location.file,
                    cursor.location.line,
                    cursor.location.column)))

        name = cursor.spelling
        result_type = cursor.result_type.spelling
        is_variadic = cursor.type.is_function_variadic()
        is_stable = decl_has_stable_tag(cursor) or func_is_whitelisted(cursor)
        params = []
        for arg in cursor.get_arguments():
            params.append((arg.type.spelling, arg.spelling))
        return cls(name, result_type, params, is_variadic, is_stable, module)

    @property
    def param_str(self):
        """Returns a string usable as a parameter list in a function decl."""
        params = []
        for param_type, param_name in self.params:
            if '[' in param_type:
                # `int foo[42]` will be a param_type of `int [42]` and a
                # param_name of `foo`. We need to put these back in the right
                # order.
                param_name += param_type[param_type.find('['):]
                param_type = param_type[:param_type.find('[')]
            params.append('{} {}'.format(param_type, param_name))
        if self.is_variadic:
            params.append('...')
        return ', '.join(params)

    @property
    def arg_str(self):
        """Returns a string usable as an argument list in a function call."""
        args = []
        for _, param_name in self.params:
            args.append(param_name)
        if self.is_variadic:
            # We need to insert the va_list (named args) at the position
            # indicated by the KNOWN_VA_FUNCTIONS map.
            insert_pos = Function.KNOWN_VA_FUNCTIONS[self.name][1]
            args.insert(insert_pos, 'args')
        return ', '.join(args)


def get_visible_functions(cursor, module, file_name, seen_functions):
    """Returns a list of all visible functions in a header file."""
    functions = []
    for child in cursor.get_children():
        if should_process_function(child, file_name, seen_functions):
            functions.append(Function.from_cursor(child, module))
    return functions


def generate_file(functions, includes, modules):
    """Generates the library source file from the given functions."""
    jinja = jinja2.Environment(loader=jinja2.FileSystemLoader(
        os.path.join(THIS_DIR, 'templates')))
    jinja.trim_blocks = True
    jinja.lstrip_blocks = True
    data = {
        'functions': functions,
        'icu_headers': includes,
        'icu_modules': modules,
    }
    return jinja.get_template('icundk.cpp').render(data)


def setup_libclang():
    """Configures libclang to load in our environment."""
    # Set up LD_LIBRARY_PATH to include libclang.so, libLLVM.so, etc.  Note
    # that setting LD_LIBRARY_PATH with os.putenv() sometimes doesn't help.
    # clang.cindex.Config.set_library_path(os.path.join(CLANG_PATH, 'lib64'))
    clang.cindex.Config.set_library_file(
        os.path.join(CLANG_PATH, 'lib64', 'libclang.so.7'))


def is_ignored_header(file_name):
    """Returns True if the current header should be ignored."""
    cpp_headers = [
        'alphaindex.h',
        'appendable.h',
        'basictz.h',
        'brkiter.h',
        'bytestream.h',
        'bytestrie.h',
        'bytestriebuilder.h',
        'calendar.h',
        'caniter.h',
        'casemap.h',
        'char16ptr.h',
        'chariter.h',
        'choicfmt.h',
        'coleitr.h',
        'coll.h',
        'compactdecimalformat.h',
        'curramt.h',
        'currpinf.h',
        'currunit.h',
        'datefmt.h',
        'dbbi.h',
        'dcfmtsym.h',
        'decimfmt.h',
        'dtfmtsym.h',
        'dtintrv.h',
        'dtitvfmt.h',
        'dtitvinf.h',
        'dtptngen.h',
        'dtrule.h',
        'edits.h',
        'errorcode.h',
        'fieldpos.h',
        'filteredbrk.h',
        'fmtable.h',
        'format.h',
        'fpositer.h',
        'gender.h',
        'gregocal.h',
        'idna.h',
        'listformatter.h',
        'locdspnm.h',
        'locid.h',
        'measfmt.h',
        'measunit.h',
        'measure.h',
        'messagepattern.h',
        'msgfmt.h',
        'normalizer2.h',
        'normlzr.h',
        'nounit.h',
        'numberformatter.h',
        'numfmt.h',
        'numsys.h',
        'parsepos.h',
        'plurfmt.h',
        'plurrule.h',
        'rbbi.h',
        'rbnf.h',
        'rbtz.h',
        'regex.h',
        'region.h',
        'reldatefmt.h',
        'rep.h',
        'resbund.h',
        'schriter.h',
        'scientificnumberformatter.h',
        'search.h',
        'selfmt.h',
        'simpleformatter.h',
        'simpletz.h',
        'smpdtfmt.h',
        'sortkey.h',
        'std_string.h',
        'strenum.h',
        'stringpiece.h',
        'stringtriebuilder.h',
        'stsearch.h',
        'symtable.h',
        'tblcoll.h',
        'timezone.h',
        'tmunit.h',
        'tmutamt.h',
        'tmutfmt.h',
        'translit.h',
        'tzfmt.h',
        'tznames.h',
        'tzrule.h',
        'tztrans.h',
        'ucharstrie.h',
        'ucharstriebuilder.h',
        'uchriter.h',
        'unifilt.h',
        'unifunct.h',
        'unimatch.h',
        'unirepl.h',
        'uniset.h',
        'unistr.h',
        'uobject.h',
        'usetiter.h',
        'vtzone.h',
    ]

    return os.path.basename(file_name) in cpp_headers


def get_cflags(icu_path, icu_header_dirs):
    """Returns the cflags that should be used for parsing."""
    clang_flags = [
        '-x',
        'c',
        '-std=c99',
        '-DU_DISABLE_RENAMING=1',
        '-DU_SHOW_CPLUSPLUS_API=0',
        '-DU_HIDE_DRAFT_API',
        '-DU_HIDE_DEPRECATED_API',
        '-DU_HIDE_INTERNAL_API',
    ]

    include_dirs = []

    include_dirs.append(os.path.join(CLANG_PATH, 'lib64/clang/7.0.2/include'))
    include_dirs.append(android_path('bionic/libc/include'))
    include_dirs.extend([os.path.join(icu_path, m) for m in icu_header_dirs])

    for include_dir in include_dirs:
        clang_flags.append('-I' + include_dir)
    return clang_flags


def handle_diagnostics(tunit):
    """Prints compiler diagnostics to stdout. Exits if errors occurred."""
    errors = 0
    for diag in tunit.diagnostics:
        if diag.severity == clang.cindex.Diagnostic.Fatal:
            level = logging.CRITICAL
            errors += 1
        elif diag.severity == clang.cindex.Diagnostic.Error:
            level = logging.ERROR
            errors += 1
        elif diag.severity == clang.cindex.Diagnostic.Warning:
            level = logging.WARNING
        elif diag.severity == clang.cindex.Diagnostic.Note:
            level = logging.INFO
        logger().log(
            level, '%s:%s:%s %s', diag.location.file, diag.location.line,
            diag.location.column, diag.spelling)
    if errors:
        sys.exit('Errors occurred during parsing. Exiting.')


def main():
    """Program entry point."""
    logging.basicConfig(level=logging.DEBUG)

    setup_libclang()
    index = clang.cindex.Index.create()

    icu_path = android_path('external/icu/icu4c/source')
    icu_modules = (
        'common',
        'i18n',
    )

    includes = []
    functions = []
    seen = set()

    clang_flags = get_cflags(icu_path, icu_modules)
    for module in icu_modules:
        path = '{}/{}/unicode'.format(icu_path, module)
        files = [os.path.join(path, f)
                 for f in os.listdir(path) if f.endswith('.h')]

        for file_path in files:
            # We ignore C++ headers.
            if is_ignored_header(file_path):
                continue

            tunit = index.parse(file_path, clang_flags)
            handle_diagnostics(tunit)
            has_stable_api = False
            visible_functions = get_visible_functions(
                tunit.cursor, module, file_path, seen)
            for function in visible_functions:
                seen.add(function.name)
                if function.is_stable:
                    has_stable_api = True
                    functions.append(function)
            if has_stable_api:
                includes.append(short_header_path(file_path))

    with open(os.path.join(THIS_DIR, 'icundk.cpp'), 'w') as out_file:
        out_file.write(generate_file(functions, includes, icu_modules))

if __name__ == '__main__':
    main()

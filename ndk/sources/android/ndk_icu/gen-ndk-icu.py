#!/usr/bin/python
"""Generate ICU stable C API wrapper library 'libndk_icu.a'.

It parses all the header files specified by the ICU module names. For each
function marked as ICU stable, it generates a wrapper function to be called
by NDK, which in turn calls the available function at runtime. The tool
relies on libclang to parse header files, which is a component provided by
Clang.

Clang can be downloaded at http://llvm.org/releases/download.html. There is an
INSTALL.TXT for installation instructions after unpacking the source. This tool
requires two environment variables properly set for libclang. The first one is
PYTHONPATH to include the path for Clang python bindings, which resides under
</path/to/clang/source>/bindings/python. The other is LD_LIBRARY_PATH to
include the path for runtime library libclang.so.

The tool should be put under 'development/ndk/sources/android/ndk_icu' for
correct path resolution. The generated library code will be dumped to stardard
output, which can be redirected to a C file, e.g.
'./gen-ndk-icu.py > ndk_icu.c'.

Reference to ICU4C stable C APIs:
http://icu-project.org/apiref/icu4c/files.html
"""

import os
import clang.cindex


def strip_space(s):
    """Strip out spaces from string s."""
    return s.replace(' ,', ',').replace(' *', '*').replace(' ]', ']')


def parse_parameters(tokens, name):
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
                va_idx = va_functions[name][1]
                param_types.insert(va_idx, 'va_list')
                param_vars.insert(va_idx, 'args')
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
        # display will be used in dlsym and may be identical to others
        # for functions with variable argument lists.
        self.display = self.name
        if self.name in va_functions:
            self.display = va_functions[self.name][0]
        self.file_name = short_name(file_name)
        self.module = module
        self.handle = 'handle_' + module
        self.result_type = strip_space(' '.join(tokens[:left_paren-2]))
        (self.param_types, self.param_vars, self.param_list, self.has_va,
         self.last_param) = parse_parameters(tokens[left_paren+1:right_paren],
                                             self.name)
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
                            func.name not in processed_functions and
                            func.name not in deprecated_functions):
                            self.functions.append(func)
                    is_recording = False
                token_buffer = []
            elif token.kind == clang.cindex.TokenKind.COMMENT:
                continue
            elif is_recording:
                token_buffer.append(token.spelling)

        return self.functions


class Template(object):
    """Template class to generate library codes."""

    header = """\
/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlfcn.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <android/log.h>
"""

    macros = """
/* Allowed version number ranges between [44, 999].
 * 44 is the minimum supported ICU version that was shipped in
 * Gingerbread (2.3.3) devices.
 */
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3
#define ICUDATA_VERSION_MIN        44

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

/* status indicates the NDK ICU initialization result, which can be queried by
 * calling ndk_icu_status().
 * Return values:
 *   positive: ICU version successfully loaded;
 *    0: uninitialized;
 *   -1: invalid/missing ICU data file;
 *   -2: failure to load ICU libraries;
 */
enum {
  NDK_ICU_UNINITIALIZED = 0,
  NDK_ICU_INVALID_ICUDATA = -1,
  NDK_ICU_INVALID_LIBRARY = -2,
};
static int status = NDK_ICU_UNINITIALIZED;
"""
    inits = """
/* ICU data filename on Android is like 'icudt49l.dat'.
 *
 * The following is from ICU code: source/common/unicode/utypes.h:
 * #define U_ICUDATA_NAME "icudt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER
 *
 * U_ICUDATA_TYPE_LETTER needs to be 'l' as it's always little-endian on
 * Android devices.
 *
 * U_ICU_VERSION_SHORT is a decimal number between [44, 999].
 */
static int filter(const struct dirent* dirp) {
  const char* name = dirp->d_name;
  const int len = strlen(name);

  // Valid length of the filename 'icudt...l.dat'
  if (len < 10 + ICUDATA_VERSION_MIN_LENGTH ||
      len > 10 + ICUDATA_VERSION_MAX_LENGTH) {
    return 0;
  }

  // Valid decimal number in between
  int i;
  for (i = 5; i < len - 5; i++) {
    if (!isdigit(name[i])) {
      return 0;
    }
  }

  return !strncmp(name, "icudt", 5) && !strncmp(&name[len - 5], "l.dat", 5);
}

static void init_icudata_version() {
  memset(icudata_version, 0, ICUDATA_VERSION_MAX_LENGTH + 1);
  memset(syms, 0, sizeof(syms));

  struct dirent** namelist = NULL;
  int n = scandir("/system/usr/icu", &namelist, &filter, alphasort);
  int max_version = -1;
  while (n--) {
    char* name = namelist[n]->d_name;
    const int len = strlen(name);
    const char* verp = &name[5];
    name[len - 5] = '\\0';

    char* endptr;
    int ver = (int)strtol(verp, &endptr, 10);

    // We prefer the latest version available.
    if (ver > max_version) {
      max_version = ver;
      icudata_version[0] = '_';
      strcpy(icudata_version + 1, verp);
    }

    free(namelist[n]);
  }
  free(namelist);

  if (max_version == -1 || max_version < ICUDATA_VERSION_MIN) {
    __android_log_print(ANDROID_LOG_ERROR, "NDKICU",
        "Cannot locate ICU data file at /system/usr/icu.");
    status = NDK_ICU_INVALID_ICUDATA;
    return;
  }

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);

  if (!handle_i18n || !handle_common) {
    __android_log_print(ANDROID_LOG_ERROR, "NDKICU",
        "Cannot open ICU libraries.");
    status = NDK_ICU_INVALID_LIBRARY;
    return;
  }
"""

    def render(self, functions, includes, modules):
        """Render the templates with incoming arguments."""
        # header lines
        print self.header

        # includes
        for i in includes:
            print '#include <%s>' % i

        # macros
        print self.macros

        # init variables
        for m in modules:
            print 'static void* handle_%s = NULL;' % m
        print 'static void* syms[%d];' % len(functions)

        # init functions and variables
        print self.inits
        idx = 0
        print '  char func_name[128];'
        for f in functions:
            print '  strcpy(func_name, "%s");' % f.display
            print '  strcat(func_name, icudata_version);'
            print '  syms[%d] = dlsym(%s, func_name);' % (idx, f.handle)
            print
            idx += 1
        print '  status = max_version;'
        print '}\n'

        # generate android NDK specific function
        print '/* ndkicu.h */'
        print 'int ndk_icu_status(void) {'
        print '  return status;'
        print '}\n'

        # generate wrapper functions
        last_file_name = ''
        idx = 0
        for f in functions:
            if f.file_name != last_file_name:
                print '/* %s */' % (f.file_name)
                last_file_name = f.file_name

            print '%s %s(%s) {' % (f.result_type, f.name, f.param_list)
            print '  pthread_once(&once_control, &init_icudata_version);'
            print '  %s (*ptr)(%s);' % (f.result_type, f.param_types)
            print '  ptr = (%s(*)(%s))syms[%d];' % (f.result_type,
                                                    f.param_types, idx)

            if f.has_va and f.return_void:
                print '  va_list args;'
                print '  va_start(args, %s);' % (f.last_param)
                print '  ptr(%s);' % (f.param_vars)
                print '  va_end(args);'
                print '  return;'
            elif f.has_va:
                print '  va_list args;'
                print '  va_start(args, %s);' % (f.last_param)
                print '  %s ret = ptr(%s);' % (f.result_type, f.param_vars)
                print '  va_end(args);'
                print '  return ret;'
            elif f.return_void:
                print '  ptr(%s);' % (f.param_vars)
                print '  return;'
            else:
                print '  return ptr(%s);' % (f.param_vars)
            print '}\n'

            idx += 1


# Presumably current pwd is 'development/ndk/sources/android/ndk_icu'
base_path = '../../../../../external/icu/icu4c/source'

# Module handles are hard-coded in the tool, e.g. handle_common.
modules = {'common': 'libicuuc.so',
           'i18n': 'libicui18n.so'}

# Functions w/ variable argument lists (...) need special care to call
# their corresponding v- versions that accept a va_list argument. Note that
# although '...' will always appear as the last parameter, its v- version
# may put the va_list arg in a different place. Hence we provide an index
# to indicate the position.
#
# e.g. 'umsg_format': ('umsg_vformat', 3) means in the wrapper function of
# 'umsg_format', it will call 'umsg_vformat' instead, with the va_list arg
# inserted as the 3rd argument.
va_functions = {'u_formatMessage': ('u_vformatMessage', 5),
                'u_parseMessage': ('u_vparseMessage', 5),
                'u_formatMessageWithError': ('u_vformatMessageWithError', 6),
                'u_parseMessageWithError': ('u_vparseMessageWithError', 5),
                'umsg_format': ('umsg_vformat', 3),
                'umsg_parse': ('umsg_vparse', 4),
                'utrace_format': ('utrace_vformat', 4)}

# Functions that exist in ICU 44 but become deprecated in the latest ICU
# (currently 54 as of 1/10/2015).
deprecated_functions = set(['ucol_openFromShortString',
                            'ucol_getShortDefinitionString',
                            'ucol_normalizeShortDefinitionString',
                            'ucol_setVariableTop',
                            'ucol_restoreVariableTop',
                            'u_getISOComment',
                            'u_setMutexFunctions',
                            'u_setAtomicIncDecFunctions'])


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
                   '-DU_HIDE_INTERNAL_API']
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
    tpl = Template()
    tpl.render(functions=collection, includes=includes, modules=modules)


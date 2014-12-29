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
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <android/log.h>

% for i in includes:
#include <${i}>
% endfor

// Allowed version number ranges between [10, 999]
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

% for m in modules:
static void* handle_${m} = NULL;
% endfor

/* ICU data filename on Android is like 'icudt49l.dat'.
 *
 * The following is from ICU code: source/common/unicode/utypes.h:
 * #define U_ICUDATA_NAME "icudt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER
 *
 * U_ICUDATA_TYPE_LETTER is one of 'e', 'x', 'b', 'l'.
 */
static int filter(const struct dirent *dirp) {
  const char *name = dirp->d_name;
  const int len = strlen(name);

  // Valid length of the filename 'icudt...l.dat'
  if (len < 10 + ICUDATA_VERSION_MIN_LENGTH ||
      len > 10 + ICUDATA_VERSION_MAX_LENGTH) {
    return 0;
  }

  const char endian = name[len - 5];
  return !strncmp(name, "icudt", 5) && !strncmp(&name[len - 4], ".dat", 4) &&
    (endian == 'e' || endian == 'x' || endian == 'b' || endian == 'l');
}

static void init_icudata_version() {
  memset(icudata_version, 0, ICUDATA_VERSION_MAX_LENGTH + 1);

  struct dirent **namelist;
  int n = scandir("/system/usr/icu", &namelist, &filter, alphasort);
  if (n <= 0) {
    __android_log_print(ANDROID_LOG_ERROR, "NDKICU",
        "Cannot locate ICU data file at /system/usr/icu.");
  } else {
    int max_version = -1;
    while (n--) {
      char *name = namelist[n]->d_name;
      const int len = strlen(name);
      const char *verp = &name[5];
      name[len - 5] = '\0';

      int ver = atoi(verp);

      // We prefer the latest version available.
      if (ver > max_version) {
        max_version = ver;
        icudata_version[0] = '_';
        strcpy(icudata_version + 1, verp);
      }

      free(namelist[n]);
    }
    free(namelist);

    if (max_version == -1) {
      __android_log_print(ANDROID_LOG_ERROR, "NDKICU",
          "Cannot locate ICU data file at /system/usr/icu.");
    } else {
% for m in modules:
      handle_${m} = dlopen("${modules[m]}", RTLD_LOCAL);
% endfor
    }
  }
}

<% last_file_name = ""
%>
% for f in functions:
%   if f.file_name != last_file_name:
/* ${f.file_name} */
<%    last_file_name = f.file_name
%>
%   endif
${f.result_type} ${f.name}_android(${f.param_list}) {
  pthread_once(&once_control, &init_icudata_version);
  ${f.result_type} (*ptr)(${f.param_types});
  char func_name[128];
  strcpy(func_name, "${f.name}");
  strcat(func_name, icudata_version);
  ptr = (${f.result_type}(*)(${f.param_types}))dlsym(${f.handle}, func_name);
  % if f.has_va and f.return_void:
  va_list args;
  va_start(args, ${f.last_param});
  ptr(${f.param_vars}, args);
  va_end(args);
  return;
  % elif f.has_va:
  va_list args;
  va_start(args, ${f.last_param});
  ${f.result_type} ret = ptr(${f.param_vars});
  va_end(args);
  return ret;
  % elif f.return_void:
  ptr(${f.param_vars});
  return;
  % else:
  return ptr(${f.param_vars});
  % endif
}

% endfor

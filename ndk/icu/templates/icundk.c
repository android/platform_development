/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <android/log.h>

{% for header in icu_headers %}
#include <{{ header }}>
{% endfor %}

/* Allowed version number ranges between [44, 999].
 * 44 is the minimum supported ICU version that was shipped in
 * Gingerbread (2.3.3) devices.
 */
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3
#define ICUDATA_VERSION_MIN 44

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

{% for module in icu_modules %}
static void* handle_{{ module }} = NULL;
{% endfor %}

static void* syms[{{ functions|length }}];

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
  for (int i = 5; i < len - 5; i++) {
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
    name[len - 5] = '\0';

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
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Cannot locate ICU data file at /system/usr/icu.");
    abort();
  }

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);

  if (!handle_i18n || !handle_common) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Cannot open ICU libraries.");
    abort();
  }

  char func_name[128];
  {% for func in functions %}

  strcpy(func_name, "{{ func.display }}");
  strcat(func_name, icudata_version);
  syms[{{ loop.index0 }}] = dlsym({{ func.handle }}, func_name);
  {% endfor %}
}

{# Generates a wrapper function for each exposed ICU function. #}
{% for func in functions %}

{{ func.result_type }} {{ func.name }}({{ func.param_list }}) {
  pthread_once(&once_control, &init_icudata_version);
  {{ func.result_type }} (*ptr)({{ func.param_types }});
  {% if func.errorcode_var is not none %}
  if (syms[{{ loop.index0 }}] == NULL) {
    *{{ func.errorcode_var }} = U_UNSUPPORTED_ERROR;
    {% if func.return_void %}
    return;
    {% else %}
    return ({{ func.result_type }})0;
    {% endif %}
  }
  {% endif %}
  ptr = ({{ func.result_type }}(*)({{ func.param_types }}))syms[{{ loop.index0 }}];
  {% if func.has_va and func.return_void %}
  va_list args;
  va_start(args, {{ func.last_param }});
  ptr({{ func.param_vars }});
  va_end(args);
  return;
  {% elif func.has_va %}
  va_list args;
  va_start(args, {{ func.last_param }});
  {{ func.result_type }} ret = ptr({{ func.param_vars }});
  va_end(args);
  return ret;
  {% elif func.return_void %}
  ptr({{ func.param_vars }});
  return;
  {% else %}
  return ptr({{ func.param_vars }});
  {% endif %}
}
{% endfor %}

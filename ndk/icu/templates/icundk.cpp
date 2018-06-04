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

#include <android/icundk.h>
#include <android/log.h>

{#
  Jinja2 is used for templating throughout this file. {% and {{ indicate the
  beginning of a Jinja2 templated section, and {# begins a comment.

  See http://jinja.pocoo.org/ for more information
#}

{% for header in icu_headers|sort %}
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
static char g_icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

static const char kLogTag[] = "NDKICU";
static const char kUnavailableFunctionErrorFmt[] =
    "Attempted to call unavailable ICU function %s.";

{% for module in icu_modules %}
static void* handle_{{ module }} = nullptr;
{% endfor %}

static int __icu_dat_file_filter(const dirent* dirp) {
  const char* name = dirp->d_name;

  // Is the name the right length to match 'icudt(\d\d\d)l.dat'?
  const size_t len = strlen(name);
  if (len < 10 + ICUDATA_VERSION_MIN_LENGTH ||
      len > 10 + ICUDATA_VERSION_MAX_LENGTH) {
    return 0;
  }

  // Check that the version is a valid decimal number.
  for (int i = 5; i < len - 5; i++) {
    if (!isdigit(name[i])) {
      return 0;
    }
  }

  return !strncmp(name, "icudt", 5) && !strncmp(&name[len - 5], "l.dat", 5);
}

static void init_icudata_version() {
  dirent** namelist = nullptr;
  int n =
      scandir("/system/usr/icu", &namelist, &__icu_dat_file_filter, alphasort);
  int max_version = -1;
  while (n--) {
    // We prefer the latest version available.
    int version = atoi(&namelist[n]->d_name[strlen("icudt")]);
    if (version != 0 && version > max_version) {
      max_version = version;
    }
    free(namelist[n]);
  }
  free(namelist);

  if (max_version < ICUDATA_VERSION_MIN) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Cannot locate ICU data file at /system/usr/icu.");
    abort();
  }

  snprintf(g_icudata_version, sizeof(g_icudata_version), "_%d", max_version);

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  if (handle_i18n == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Could not open libicui18n: %s", dlerror());
    abort();
  }

  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);
  if (handle_common == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Could not open libicuuc: %s", dlerror());
    abort();
  }
}

bool ndk_is_icu_function_available(const char* name) {
  pthread_once(&once_control, &init_icudata_version);

  char versioned_symbol_name[strlen(name) +
                             sizeof(g_icudata_version)];
  snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s", name,
           g_icudata_version);

  if (dlsym(handle_common, versioned_symbol_name) != nullptr) {
    return true;
  }

  if (dlsym(handle_i18n, versioned_symbol_name) != nullptr) {
    return true;
  }

  return false;
}

{# Generates a wrapper function for each exposed ICU function. #}
{% for func in functions %}
{{ func.result_type }} {{ func.name }}({{ func.param_str }}) {
  pthread_once(&once_control, &init_icudata_version);

  typedef decltype(&{{func.callee}}) FuncPtr;
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("{{ func.callee }}") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "{{ func.callee }}", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym({{func.handle}}, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "{{ func.name }}");
    abort();
  }

  {% if func.is_variadic and func.return_void %}
  va_list args;
  va_start(args, {{ func.last_param }});
  ptr({{ func.arg_str }});
  va_end(args);
  return;
  {% elif func.is_variadic %}
  va_list args;
  va_start(args, {{ func.last_param }});
  {{ func.result_type }} ret = ptr({{ func.arg_str }});
  va_end(args);
  return ret;
  {% elif func.return_void %}
  ptr({{ func.arg_str }});
  {% else %}
  return ptr({{ func.arg_str }});
  {% endif %}
}
{% endfor %}

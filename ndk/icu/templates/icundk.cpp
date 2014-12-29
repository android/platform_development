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

{% for module in icu_modules %}
static void* handle_{{ module }} = nullptr;
{% endfor %}

struct sym_tab {
  const char* name;
  void** handle;
  void* addr;
  bool initialized;
};

{# TODO(danalbert): Don't waste all this space somehow. #}
static sym_tab syms[{{ functions|length }}] = {
  {% for func in functions %}
  {
    .name = "{{ func.name }}",
    .handle = &{{ func.handle }},
    .addr = nullptr,
    .initialized = false,
  },
  {% endfor %}
};

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
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Cannot locate ICU data file at /system/usr/icu.");
    abort();
  }

  snprintf(g_icudata_version, sizeof(g_icudata_version), "_%d", max_version);

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  if (handle_i18n == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Could not open libicui18n: %s", dlerror());
    abort();
  }

  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);
  if (handle_common == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Could not open libicuuc: %s", dlerror());
    abort();
  }
}

static sym_tab* get_sym_tab_entry(const char* name) {
  for (size_t i = 0; i < sizeof(syms); i++) {
    sym_tab* entry = &syms[i];
    if (strcmp(entry->name, name) == 0) {
      return entry;
    }
  }

  return nullptr;
}

static void* get_icu_wrapper_addr(const char* symbol_name) {
  sym_tab* entry = get_sym_tab_entry(symbol_name);
  if (entry == nullptr) {
    return nullptr;
  }

  if (!entry->initialized) {
    pthread_once(&once_control, &init_icudata_version);

    char versioned_symbol_name[strlen(symbol_name) + sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             symbol_name, g_icudata_version);

    entry->addr = dlsym(*entry->handle, versioned_symbol_name);
    entry->initialized = true;
  }

  return entry->addr;
}

bool ndk_icu_available(const char* name) {
  return get_icu_wrapper_addr(name) != nullptr;
}

{# Generates a wrapper function for each exposed ICU function. #}
{% for func in functions %}
{{ func.result_type }} {{ func.name }}({{ func.param_list }}) {
  typedef decltype(&{{ func.display }}) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("{{ func.display }}"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "{{ func.name }}");
    abort();
  }

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
  {% else %}
  return ptr({{ func.param_vars }});
  {% endif %}
}
{% endfor %}

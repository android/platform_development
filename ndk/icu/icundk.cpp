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


#include <unicode/icudataver.h>
#include <unicode/putil.h>
#include <unicode/ubidi.h>
#include <unicode/ubiditransform.h>
#include <unicode/ubrk.h>
#include <unicode/ucal.h>
#include <unicode/ucasemap.h>
#include <unicode/ucat.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <unicode/ucnv_err.h>
#include <unicode/ucnvsel.h>
#include <unicode/ucol.h>
#include <unicode/ucoleitr.h>
#include <unicode/ucsdet.h>
#include <unicode/ucurr.h>
#include <unicode/udat.h>
#include <unicode/udata.h>
#include <unicode/udateintervalformat.h>
#include <unicode/udatpg.h>
#include <unicode/uenum.h>
#include <unicode/ufieldpositer.h>
#include <unicode/uformattable.h>
#include <unicode/ugender.h>
#include <unicode/uidna.h>
#include <unicode/uiter.h>
#include <unicode/uldnames.h>
#include <unicode/ulistformatter.h>
#include <unicode/uloc.h>
#include <unicode/ulocdata.h>
#include <unicode/umsg.h>
#include <unicode/unorm2.h>
#include <unicode/unum.h>
#include <unicode/unumsys.h>
#include <unicode/upluralrules.h>
#include <unicode/uregex.h>
#include <unicode/uregion.h>
#include <unicode/ureldatefmt.h>
#include <unicode/ures.h>
#include <unicode/uscript.h>
#include <unicode/usearch.h>
#include <unicode/uset.h>
#include <unicode/ushape.h>
#include <unicode/uspoof.h>
#include <unicode/usprep.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>
#include <unicode/utmscale.h>
#include <unicode/utrace.h>
#include <unicode/utrans.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

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

static void* handle_common = nullptr;
static void* handle_i18n = nullptr;

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

const char * uloc_getDefault() {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDefault") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDefault", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDefault");
    abort();
  }

  return ptr();
}
void uloc_setDefault(const char * localeID, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * localeID, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_setDefault") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_setDefault", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_setDefault");
    abort();
  }

  ptr(localeID, status);
}
int32_t uloc_getLanguage(const char * localeID, char * language, int32_t languageCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * language, int32_t languageCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getLanguage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getLanguage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getLanguage");
    abort();
  }

  return ptr(localeID, language, languageCapacity, err);
}
int32_t uloc_getScript(const char * localeID, char * script, int32_t scriptCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * script, int32_t scriptCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getScript") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getScript", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getScript");
    abort();
  }

  return ptr(localeID, script, scriptCapacity, err);
}
int32_t uloc_getCountry(const char * localeID, char * country, int32_t countryCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * country, int32_t countryCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getCountry") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getCountry", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getCountry");
    abort();
  }

  return ptr(localeID, country, countryCapacity, err);
}
int32_t uloc_getVariant(const char * localeID, char * variant, int32_t variantCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * variant, int32_t variantCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getVariant") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getVariant", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getVariant");
    abort();
  }

  return ptr(localeID, variant, variantCapacity, err);
}
int32_t uloc_getName(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getName");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
int32_t uloc_canonicalize(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_canonicalize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_canonicalize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_canonicalize");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
const char * uloc_getISO3Language(const char * localeID) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * localeID);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getISO3Language") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getISO3Language", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getISO3Language");
    abort();
  }

  return ptr(localeID);
}
const char * uloc_getISO3Country(const char * localeID) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * localeID);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getISO3Country") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getISO3Country", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getISO3Country");
    abort();
  }

  return ptr(localeID);
}
uint32_t uloc_getLCID(const char * localeID) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(const char * localeID);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getLCID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getLCID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getLCID");
    abort();
  }

  return ptr(localeID);
}
int32_t uloc_getDisplayLanguage(const char * locale, const char * displayLocale, UChar * language, int32_t languageCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const char * displayLocale, UChar * language, int32_t languageCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayLanguage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayLanguage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayLanguage");
    abort();
  }

  return ptr(locale, displayLocale, language, languageCapacity, status);
}
int32_t uloc_getDisplayScript(const char * locale, const char * displayLocale, UChar * script, int32_t scriptCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const char * displayLocale, UChar * script, int32_t scriptCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayScript") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayScript", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayScript");
    abort();
  }

  return ptr(locale, displayLocale, script, scriptCapacity, status);
}
int32_t uloc_getDisplayCountry(const char * locale, const char * displayLocale, UChar * country, int32_t countryCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const char * displayLocale, UChar * country, int32_t countryCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayCountry") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayCountry", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayCountry");
    abort();
  }

  return ptr(locale, displayLocale, country, countryCapacity, status);
}
int32_t uloc_getDisplayVariant(const char * locale, const char * displayLocale, UChar * variant, int32_t variantCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const char * displayLocale, UChar * variant, int32_t variantCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayVariant") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayVariant", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayVariant");
    abort();
  }

  return ptr(locale, displayLocale, variant, variantCapacity, status);
}
int32_t uloc_getDisplayKeyword(const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayKeyword") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayKeyword", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayKeyword");
    abort();
  }

  return ptr(keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayKeywordValue(const char * locale, const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayKeywordValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayKeywordValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayKeywordValue");
    abort();
  }

  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayName(const char * localeID, const char * inLocaleID, UChar * result, int32_t maxResultSize, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, const char * inLocaleID, UChar * result, int32_t maxResultSize, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getDisplayName");
    abort();
  }

  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}
const char * uloc_getAvailable(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getAvailable");
    abort();
  }

  return ptr(n);
}
int32_t uloc_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_countAvailable");
    abort();
  }

  return ptr();
}
const char *const * uloc_getISOLanguages() {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char *const * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getISOLanguages") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getISOLanguages", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getISOLanguages");
    abort();
  }

  return ptr();
}
const char *const * uloc_getISOCountries() {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char *const * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getISOCountries") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getISOCountries", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getISOCountries");
    abort();
  }

  return ptr();
}
int32_t uloc_getParent(const char * localeID, char * parent, int32_t parentCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * parent, int32_t parentCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getParent") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getParent", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getParent");
    abort();
  }

  return ptr(localeID, parent, parentCapacity, err);
}
int32_t uloc_getBaseName(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getBaseName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getBaseName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getBaseName");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
UEnumeration * uloc_openKeywords(const char * localeID, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * localeID, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_openKeywords") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_openKeywords", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_openKeywords");
    abort();
  }

  return ptr(localeID, status);
}
int32_t uloc_getKeywordValue(const char * localeID, const char * keywordName, char * buffer, int32_t bufferCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, const char * keywordName, char * buffer, int32_t bufferCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getKeywordValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getKeywordValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getKeywordValue");
    abort();
  }

  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}
int32_t uloc_setKeywordValue(const char * keywordName, const char * keywordValue, char * buffer, int32_t bufferCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * keywordName, const char * keywordValue, char * buffer, int32_t bufferCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_setKeywordValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_setKeywordValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_setKeywordValue");
    abort();
  }

  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}
UBool uloc_isRightToLeft(const char * locale) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const char * locale);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_isRightToLeft") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_isRightToLeft", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_isRightToLeft");
    abort();
  }

  return ptr(locale);
}
ULayoutType uloc_getCharacterOrientation(const char * localeId, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef ULayoutType (*FuncPtr)(const char * localeId, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getCharacterOrientation") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getCharacterOrientation", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getCharacterOrientation");
    abort();
  }

  return ptr(localeId, status);
}
ULayoutType uloc_getLineOrientation(const char * localeId, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef ULayoutType (*FuncPtr)(const char * localeId, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getLineOrientation") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getLineOrientation", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getLineOrientation");
    abort();
  }

  return ptr(localeId, status);
}
int32_t uloc_acceptLanguageFromHTTP(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char * httpAcceptLanguage, UEnumeration * availableLocales, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char * httpAcceptLanguage, UEnumeration * availableLocales, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_acceptLanguageFromHTTP") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_acceptLanguageFromHTTP", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_acceptLanguageFromHTTP");
    abort();
  }

  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}
int32_t uloc_acceptLanguage(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char ** acceptList, int32_t acceptListCount, UEnumeration * availableLocales, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char ** acceptList, int32_t acceptListCount, UEnumeration * availableLocales, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_acceptLanguage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_acceptLanguage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_acceptLanguage");
    abort();
  }

  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}
int32_t uloc_getLocaleForLCID(uint32_t hostID, char * locale, int32_t localeCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(uint32_t hostID, char * locale, int32_t localeCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_getLocaleForLCID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_getLocaleForLCID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_getLocaleForLCID");
    abort();
  }

  return ptr(hostID, locale, localeCapacity, status);
}
int32_t uloc_addLikelySubtags(const char * localeID, char * maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_addLikelySubtags") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_addLikelySubtags", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_addLikelySubtags");
    abort();
  }

  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}
int32_t uloc_minimizeSubtags(const char * localeID, char * minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_minimizeSubtags") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_minimizeSubtags", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_minimizeSubtags");
    abort();
  }

  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}
int32_t uloc_forLanguageTag(const char * langtag, char * localeID, int32_t localeIDCapacity, int32_t * parsedLength, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * langtag, char * localeID, int32_t localeIDCapacity, int32_t * parsedLength, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_forLanguageTag") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_forLanguageTag", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_forLanguageTag");
    abort();
  }

  return ptr(langtag, localeID, localeIDCapacity, parsedLength, err);
}
int32_t uloc_toLanguageTag(const char * localeID, char * langtag, int32_t langtagCapacity, UBool strict, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * localeID, char * langtag, int32_t langtagCapacity, UBool strict, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_toLanguageTag") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_toLanguageTag", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_toLanguageTag");
    abort();
  }

  return ptr(localeID, langtag, langtagCapacity, strict, err);
}
const char * uloc_toUnicodeLocaleKey(const char * keyword) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * keyword);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_toUnicodeLocaleKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_toUnicodeLocaleKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_toUnicodeLocaleKey");
    abort();
  }

  return ptr(keyword);
}
const char * uloc_toUnicodeLocaleType(const char * keyword, const char * value) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * keyword, const char * value);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_toUnicodeLocaleType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_toUnicodeLocaleType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_toUnicodeLocaleType");
    abort();
  }

  return ptr(keyword, value);
}
const char * uloc_toLegacyKey(const char * keyword) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * keyword);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_toLegacyKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_toLegacyKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_toLegacyKey");
    abort();
  }

  return ptr(keyword);
}
const char * uloc_toLegacyType(const char * keyword, const char * value) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * keyword, const char * value);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uloc_toLegacyType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uloc_toLegacyType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uloc_toLegacyType");
    abort();
  }

  return ptr(keyword, value);
}
void u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo dataVersionFillin, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getDataVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getDataVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getDataVersion");
    abort();
  }

  ptr(dataVersionFillin, status);
}
UBool u_hasBinaryProperty(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c, UProperty which);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_hasBinaryProperty") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_hasBinaryProperty", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_hasBinaryProperty");
    abort();
  }

  return ptr(c, which);
}
UBool u_isUAlphabetic(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isUAlphabetic") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isUAlphabetic", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isUAlphabetic");
    abort();
  }

  return ptr(c);
}
UBool u_isULowercase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isULowercase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isULowercase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isULowercase");
    abort();
  }

  return ptr(c);
}
UBool u_isUUppercase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isUUppercase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isUUppercase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isUUppercase");
    abort();
  }

  return ptr(c);
}
UBool u_isUWhiteSpace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isUWhiteSpace") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isUWhiteSpace", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isUWhiteSpace");
    abort();
  }

  return ptr(c);
}
int32_t u_getIntPropertyValue(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 c, UProperty which);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getIntPropertyValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getIntPropertyValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getIntPropertyValue");
    abort();
  }

  return ptr(c, which);
}
int32_t u_getIntPropertyMinValue(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UProperty which);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getIntPropertyMinValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getIntPropertyMinValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getIntPropertyMinValue");
    abort();
  }

  return ptr(which);
}
int32_t u_getIntPropertyMaxValue(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UProperty which);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getIntPropertyMaxValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getIntPropertyMaxValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getIntPropertyMaxValue");
    abort();
  }

  return ptr(which);
}
double u_getNumericValue(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getNumericValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getNumericValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getNumericValue");
    abort();
  }

  return ptr(c);
}
UBool u_islower(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_islower") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_islower", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_islower");
    abort();
  }

  return ptr(c);
}
UBool u_isupper(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isupper") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isupper", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isupper");
    abort();
  }

  return ptr(c);
}
UBool u_istitle(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_istitle") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_istitle", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_istitle");
    abort();
  }

  return ptr(c);
}
UBool u_isdigit(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isdigit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isdigit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isdigit");
    abort();
  }

  return ptr(c);
}
UBool u_isalpha(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isalpha") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isalpha", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isalpha");
    abort();
  }

  return ptr(c);
}
UBool u_isalnum(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isalnum") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isalnum", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isalnum");
    abort();
  }

  return ptr(c);
}
UBool u_isxdigit(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isxdigit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isxdigit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isxdigit");
    abort();
  }

  return ptr(c);
}
UBool u_ispunct(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_ispunct") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_ispunct", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_ispunct");
    abort();
  }

  return ptr(c);
}
UBool u_isgraph(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isgraph") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isgraph", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isgraph");
    abort();
  }

  return ptr(c);
}
UBool u_isblank(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isblank") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isblank", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isblank");
    abort();
  }

  return ptr(c);
}
UBool u_isdefined(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isdefined") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isdefined", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isdefined");
    abort();
  }

  return ptr(c);
}
UBool u_isspace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isspace") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isspace", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isspace");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaSpaceChar(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isJavaSpaceChar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isJavaSpaceChar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isJavaSpaceChar");
    abort();
  }

  return ptr(c);
}
UBool u_isWhitespace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isWhitespace") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isWhitespace", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isWhitespace");
    abort();
  }

  return ptr(c);
}
UBool u_iscntrl(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_iscntrl") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_iscntrl", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_iscntrl");
    abort();
  }

  return ptr(c);
}
UBool u_isISOControl(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isISOControl") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isISOControl", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isISOControl");
    abort();
  }

  return ptr(c);
}
UBool u_isprint(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isprint") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isprint", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isprint");
    abort();
  }

  return ptr(c);
}
UBool u_isbase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isbase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isbase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isbase");
    abort();
  }

  return ptr(c);
}
UCharDirection u_charDirection(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCharDirection (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charDirection") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charDirection", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charDirection");
    abort();
  }

  return ptr(c);
}
UBool u_isMirrored(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isMirrored") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isMirrored", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isMirrored");
    abort();
  }

  return ptr(c);
}
UChar32 u_charMirror(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charMirror") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charMirror", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charMirror");
    abort();
  }

  return ptr(c);
}
UChar32 u_getBidiPairedBracket(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getBidiPairedBracket") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getBidiPairedBracket", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getBidiPairedBracket");
    abort();
  }

  return ptr(c);
}
int8_t u_charType(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int8_t (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charType");
    abort();
  }

  return ptr(c);
}
void u_enumCharTypes(UCharEnumTypeRange * enumRange, const void * context) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharEnumTypeRange * enumRange, const void * context);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_enumCharTypes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_enumCharTypes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_enumCharTypes");
    abort();
  }

  ptr(enumRange, context);
}
uint8_t u_getCombiningClass(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint8_t (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getCombiningClass") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getCombiningClass", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getCombiningClass");
    abort();
  }

  return ptr(c);
}
int32_t u_charDigitValue(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charDigitValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charDigitValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charDigitValue");
    abort();
  }

  return ptr(c);
}
UBlockCode ublock_getCode(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBlockCode (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ublock_getCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ublock_getCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ublock_getCode");
    abort();
  }

  return ptr(c);
}
int32_t u_charName(UChar32 code, UCharNameChoice nameChoice, char * buffer, int32_t bufferLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 code, UCharNameChoice nameChoice, char * buffer, int32_t bufferLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charName");
    abort();
  }

  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}
UChar32 u_charFromName(UCharNameChoice nameChoice, const char * name, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UCharNameChoice nameChoice, const char * name, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charFromName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charFromName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charFromName");
    abort();
  }

  return ptr(nameChoice, name, pErrorCode);
}
void u_enumCharNames(UChar32 start, UChar32 limit, UEnumCharNamesFn * fn, void * context, UCharNameChoice nameChoice, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UChar32 start, UChar32 limit, UEnumCharNamesFn * fn, void * context, UCharNameChoice nameChoice, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_enumCharNames") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_enumCharNames", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_enumCharNames");
    abort();
  }

  ptr(start, limit, fn, context, nameChoice, pErrorCode);
}
const char * u_getPropertyName(UProperty property, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UProperty property, UPropertyNameChoice nameChoice);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getPropertyName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getPropertyName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getPropertyName");
    abort();
  }

  return ptr(property, nameChoice);
}
UProperty u_getPropertyEnum(const char * alias) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UProperty (*FuncPtr)(const char * alias);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getPropertyEnum") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getPropertyEnum", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getPropertyEnum");
    abort();
  }

  return ptr(alias);
}
const char * u_getPropertyValueName(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UProperty property, int32_t value, UPropertyNameChoice nameChoice);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getPropertyValueName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getPropertyValueName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getPropertyValueName");
    abort();
  }

  return ptr(property, value, nameChoice);
}
int32_t u_getPropertyValueEnum(UProperty property, const char * alias) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UProperty property, const char * alias);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getPropertyValueEnum") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getPropertyValueEnum", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getPropertyValueEnum");
    abort();
  }

  return ptr(property, alias);
}
UBool u_isIDStart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isIDStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isIDStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isIDStart");
    abort();
  }

  return ptr(c);
}
UBool u_isIDPart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isIDPart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isIDPart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isIDPart");
    abort();
  }

  return ptr(c);
}
UBool u_isIDIgnorable(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isIDIgnorable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isIDIgnorable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isIDIgnorable");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaIDStart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isJavaIDStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isJavaIDStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isJavaIDStart");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaIDPart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_isJavaIDPart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_isJavaIDPart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_isJavaIDPart");
    abort();
  }

  return ptr(c);
}
UChar32 u_tolower(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_tolower") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_tolower", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_tolower");
    abort();
  }

  return ptr(c);
}
UChar32 u_toupper(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_toupper") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_toupper", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_toupper");
    abort();
  }

  return ptr(c);
}
UChar32 u_totitle(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_totitle") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_totitle", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_totitle");
    abort();
  }

  return ptr(c);
}
UChar32 u_foldCase(UChar32 c, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UChar32 c, uint32_t options);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_foldCase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_foldCase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_foldCase");
    abort();
  }

  return ptr(c, options);
}
int32_t u_digit(UChar32 ch, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 ch, int8_t radix);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_digit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_digit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_digit");
    abort();
  }

  return ptr(ch, radix);
}
UChar32 u_forDigit(int32_t digit, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(int32_t digit, int8_t radix);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_forDigit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_forDigit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_forDigit");
    abort();
  }

  return ptr(digit, radix);
}
void u_charAge(UChar32 c, UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UChar32 c, UVersionInfo versionArray);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charAge") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charAge", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charAge");
    abort();
  }

  ptr(c, versionArray);
}
void u_getUnicodeVersion(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo versionArray);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getUnicodeVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getUnicodeVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getUnicodeVersion");
    abort();
  }

  ptr(versionArray);
}
int32_t u_getFC_NFKC_Closure(UChar32 c, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 c, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getFC_NFKC_Closure") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getFC_NFKC_Closure", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getFC_NFKC_Closure");
    abort();
  }

  return ptr(c, dest, destCapacity, pErrorCode);
}
void UCNV_FROM_U_CALLBACK_STOP(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_FROM_U_CALLBACK_STOP") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_FROM_U_CALLBACK_STOP", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_FROM_U_CALLBACK_STOP");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_STOP(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_TO_U_CALLBACK_STOP") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_TO_U_CALLBACK_STOP", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_TO_U_CALLBACK_STOP");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_FROM_U_CALLBACK_SKIP(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_FROM_U_CALLBACK_SKIP") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_FROM_U_CALLBACK_SKIP", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_FROM_U_CALLBACK_SKIP");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_SUBSTITUTE(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_FROM_U_CALLBACK_SUBSTITUTE") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_FROM_U_CALLBACK_SUBSTITUTE", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_FROM_U_CALLBACK_SUBSTITUTE");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_ESCAPE(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_FROM_U_CALLBACK_ESCAPE") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_FROM_U_CALLBACK_ESCAPE", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_FROM_U_CALLBACK_ESCAPE");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_SKIP(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_TO_U_CALLBACK_SKIP") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_TO_U_CALLBACK_SKIP", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_TO_U_CALLBACK_SKIP");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_SUBSTITUTE(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_TO_U_CALLBACK_SUBSTITUTE") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_TO_U_CALLBACK_SUBSTITUTE", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_TO_U_CALLBACK_SUBSTITUTE");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_ESCAPE(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("UCNV_TO_U_CALLBACK_ESCAPE") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "UCNV_TO_U_CALLBACK_ESCAPE", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "UCNV_TO_U_CALLBACK_ESCAPE");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
UDataMemory * udata_open(const char * path, const char * type, const char * name, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDataMemory * (*FuncPtr)(const char * path, const char * type, const char * name, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_open");
    abort();
  }

  return ptr(path, type, name, pErrorCode);
}
UDataMemory * udata_openChoice(const char * path, const char * type, const char * name, UDataMemoryIsAcceptable * isAcceptable, void * context, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDataMemory * (*FuncPtr)(const char * path, const char * type, const char * name, UDataMemoryIsAcceptable * isAcceptable, void * context, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_openChoice") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_openChoice", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_openChoice");
    abort();
  }

  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}
void udata_close(UDataMemory * pData) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDataMemory * pData);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_close");
    abort();
  }

  ptr(pData);
}
const void * udata_getMemory(UDataMemory * pData) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const void * (*FuncPtr)(UDataMemory * pData);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_getMemory") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_getMemory", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_getMemory");
    abort();
  }

  return ptr(pData);
}
void udata_getInfo(UDataMemory * pData, UDataInfo * pInfo) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDataMemory * pData, UDataInfo * pInfo);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_getInfo") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_getInfo", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_getInfo");
    abort();
  }

  ptr(pData, pInfo);
}
void udata_setCommonData(const void * data, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * data, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_setCommonData") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_setCommonData", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_setCommonData");
    abort();
  }

  ptr(data, err);
}
void udata_setAppData(const char * packageName, const void * data, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * packageName, const void * data, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_setAppData") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_setAppData", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_setAppData");
    abort();
  }

  ptr(packageName, data, err);
}
void udata_setFileAccess(UDataFileAccess access, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDataFileAccess access, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udata_setFileAccess") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udata_setFileAccess", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udata_setFileAccess");
    abort();
  }

  ptr(access, status);
}
int ucnv_compareNames(const char * name1, const char * name2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int (*FuncPtr)(const char * name1, const char * name2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_compareNames") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_compareNames", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_compareNames");
    abort();
  }

  return ptr(name1, name2);
}
UConverter * ucnv_open(const char * converterName, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverter * (*FuncPtr)(const char * converterName, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_open");
    abort();
  }

  return ptr(converterName, err);
}
UConverter * ucnv_openU(const UChar * name, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverter * (*FuncPtr)(const UChar * name, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_openU") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_openU", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_openU");
    abort();
  }

  return ptr(name, err);
}
UConverter * ucnv_openCCSID(int32_t codepage, UConverterPlatform platform, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverter * (*FuncPtr)(int32_t codepage, UConverterPlatform platform, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_openCCSID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_openCCSID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_openCCSID");
    abort();
  }

  return ptr(codepage, platform, err);
}
UConverter * ucnv_openPackage(const char * packageName, const char * converterName, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverter * (*FuncPtr)(const char * packageName, const char * converterName, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_openPackage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_openPackage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_openPackage");
    abort();
  }

  return ptr(packageName, converterName, err);
}
UConverter * ucnv_safeClone(const UConverter * cnv, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverter * (*FuncPtr)(const UConverter * cnv, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_safeClone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_safeClone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_safeClone");
    abort();
  }

  return ptr(cnv, stackBuffer, pBufferSize, status);
}
void ucnv_close(UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_close");
    abort();
  }

  ptr(converter);
}
void ucnv_getSubstChars(const UConverter * converter, char * subChars, int8_t * len, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, char * subChars, int8_t * len, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getSubstChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getSubstChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getSubstChars");
    abort();
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstChars(UConverter * converter, const char * subChars, int8_t len, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter, const char * subChars, int8_t len, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setSubstChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setSubstChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setSubstChars");
    abort();
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstString(UConverter * cnv, const UChar * s, int32_t length, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * cnv, const UChar * s, int32_t length, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setSubstString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setSubstString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setSubstString");
    abort();
  }

  ptr(cnv, s, length, err);
}
void ucnv_getInvalidChars(const UConverter * converter, char * errBytes, int8_t * len, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, char * errBytes, int8_t * len, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getInvalidChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getInvalidChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getInvalidChars");
    abort();
  }

  ptr(converter, errBytes, len, err);
}
void ucnv_getInvalidUChars(const UConverter * converter, UChar * errUChars, int8_t * len, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, UChar * errUChars, int8_t * len, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getInvalidUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getInvalidUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getInvalidUChars");
    abort();
  }

  ptr(converter, errUChars, len, err);
}
void ucnv_reset(UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_reset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_reset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_reset");
    abort();
  }

  ptr(converter);
}
void ucnv_resetToUnicode(UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_resetToUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_resetToUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_resetToUnicode");
    abort();
  }

  ptr(converter);
}
void ucnv_resetFromUnicode(UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_resetFromUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_resetFromUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_resetFromUnicode");
    abort();
  }

  ptr(converter);
}
int8_t ucnv_getMaxCharSize(const UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int8_t (*FuncPtr)(const UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getMaxCharSize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getMaxCharSize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getMaxCharSize");
    abort();
  }

  return ptr(converter);
}
int8_t ucnv_getMinCharSize(const UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int8_t (*FuncPtr)(const UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getMinCharSize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getMinCharSize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getMinCharSize");
    abort();
  }

  return ptr(converter);
}
int32_t ucnv_getDisplayName(const UConverter * converter, const char * displayLocale, UChar * displayName, int32_t displayNameCapacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UConverter * converter, const char * displayLocale, UChar * displayName, int32_t displayNameCapacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getDisplayName");
    abort();
  }

  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}
const char * ucnv_getName(const UConverter * converter, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UConverter * converter, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getName");
    abort();
  }

  return ptr(converter, err);
}
int32_t ucnv_getCCSID(const UConverter * converter, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UConverter * converter, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getCCSID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getCCSID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getCCSID");
    abort();
  }

  return ptr(converter, err);
}
UConverterPlatform ucnv_getPlatform(const UConverter * converter, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverterPlatform (*FuncPtr)(const UConverter * converter, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getPlatform") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getPlatform", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getPlatform");
    abort();
  }

  return ptr(converter, err);
}
UConverterType ucnv_getType(const UConverter * converter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverterType (*FuncPtr)(const UConverter * converter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getType");
    abort();
  }

  return ptr(converter);
}
void ucnv_getStarters(const UConverter * converter, UBool  starters[256], UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, UBool  starters[256], UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getStarters") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getStarters", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getStarters");
    abort();
  }

  ptr(converter, starters, err);
}
void ucnv_getUnicodeSet(const UConverter * cnv, USet * setFillIn, UConverterUnicodeSet whichSet, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * cnv, USet * setFillIn, UConverterUnicodeSet whichSet, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getUnicodeSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getUnicodeSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getUnicodeSet");
    abort();
  }

  ptr(cnv, setFillIn, whichSet, pErrorCode);
}
void ucnv_getToUCallBack(const UConverter * converter, UConverterToUCallback * action, const void ** context) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, UConverterToUCallback * action, const void ** context);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getToUCallBack") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getToUCallBack", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getToUCallBack");
    abort();
  }

  ptr(converter, action, context);
}
void ucnv_getFromUCallBack(const UConverter * converter, UConverterFromUCallback * action, const void ** context) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * converter, UConverterFromUCallback * action, const void ** context);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getFromUCallBack") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getFromUCallBack", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getFromUCallBack");
    abort();
  }

  ptr(converter, action, context);
}
void ucnv_setToUCallBack(UConverter * converter, UConverterToUCallback newAction, const void * newContext, UConverterToUCallback * oldAction, const void ** oldContext, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter, UConverterToUCallback newAction, const void * newContext, UConverterToUCallback * oldAction, const void ** oldContext, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setToUCallBack") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setToUCallBack", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setToUCallBack");
    abort();
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_setFromUCallBack(UConverter * converter, UConverterFromUCallback newAction, const void * newContext, UConverterFromUCallback * oldAction, const void ** oldContext, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter, UConverterFromUCallback newAction, const void * newContext, UConverterFromUCallback * oldAction, const void ** oldContext, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setFromUCallBack") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setFromUCallBack", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setFromUCallBack");
    abort();
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_fromUnicode(UConverter * converter, char ** target, const char * targetLimit, const UChar ** source, const UChar * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter, char ** target, const char * targetLimit, const UChar ** source, const UChar * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_fromUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_fromUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_fromUnicode");
    abort();
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
void ucnv_toUnicode(UConverter * converter, UChar ** target, const UChar * targetLimit, const char ** source, const char * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * converter, UChar ** target, const UChar * targetLimit, const char ** source, const char * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_toUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_toUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_toUnicode");
    abort();
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
int32_t ucnv_fromUChars(UConverter * cnv, char * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UConverter * cnv, char * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_fromUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_fromUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_fromUChars");
    abort();
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucnv_toUChars(UConverter * cnv, UChar * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UConverter * cnv, UChar * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_toUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_toUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_toUChars");
    abort();
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
UChar32 ucnv_getNextUChar(UConverter * converter, const char ** source, const char * sourceLimit, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UConverter * converter, const char ** source, const char * sourceLimit, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getNextUChar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getNextUChar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getNextUChar");
    abort();
  }

  return ptr(converter, source, sourceLimit, err);
}
void ucnv_convertEx(UConverter * targetCnv, UConverter * sourceCnv, char ** target, const char * targetLimit, const char ** source, const char * sourceLimit, UChar * pivotStart, UChar ** pivotSource, UChar ** pivotTarget, const UChar * pivotLimit, UBool reset, UBool flush, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * targetCnv, UConverter * sourceCnv, char ** target, const char * targetLimit, const char ** source, const char * sourceLimit, UChar * pivotStart, UChar ** pivotSource, UChar ** pivotTarget, const UChar * pivotLimit, UBool reset, UBool flush, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_convertEx") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_convertEx", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_convertEx");
    abort();
  }

  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
}
int32_t ucnv_convert(const char * toConverterName, const char * fromConverterName, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * toConverterName, const char * fromConverterName, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_convert") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_convert", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_convert");
    abort();
  }

  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_toAlgorithmic(UConverterType algorithmicType, UConverter * cnv, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UConverterType algorithmicType, UConverter * cnv, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_toAlgorithmic") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_toAlgorithmic", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_toAlgorithmic");
    abort();
  }

  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_fromAlgorithmic(UConverter * cnv, UConverterType algorithmicType, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UConverter * cnv, UConverterType algorithmicType, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_fromAlgorithmic") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_fromAlgorithmic", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_fromAlgorithmic");
    abort();
  }

  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_flushCache() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_flushCache") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_flushCache", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_flushCache");
    abort();
  }

  return ptr();
}
int32_t ucnv_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_countAvailable");
    abort();
  }

  return ptr();
}
const char * ucnv_getAvailableName(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getAvailableName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getAvailableName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getAvailableName");
    abort();
  }

  return ptr(n);
}
UEnumeration * ucnv_openAllNames(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_openAllNames") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_openAllNames", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_openAllNames");
    abort();
  }

  return ptr(pErrorCode);
}
uint16_t ucnv_countAliases(const char * alias, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint16_t (*FuncPtr)(const char * alias, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_countAliases") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_countAliases", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_countAliases");
    abort();
  }

  return ptr(alias, pErrorCode);
}
const char * ucnv_getAlias(const char * alias, uint16_t n, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * alias, uint16_t n, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getAlias") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getAlias", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getAlias");
    abort();
  }

  return ptr(alias, n, pErrorCode);
}
void ucnv_getAliases(const char * alias, const char ** aliases, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * alias, const char ** aliases, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getAliases") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getAliases", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getAliases");
    abort();
  }

  ptr(alias, aliases, pErrorCode);
}
UEnumeration * ucnv_openStandardNames(const char * convName, const char * standard, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * convName, const char * standard, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_openStandardNames") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_openStandardNames", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_openStandardNames");
    abort();
  }

  return ptr(convName, standard, pErrorCode);
}
uint16_t ucnv_countStandards() {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint16_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_countStandards") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_countStandards", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_countStandards");
    abort();
  }

  return ptr();
}
const char * ucnv_getStandard(uint16_t n, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(uint16_t n, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getStandard") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getStandard", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getStandard");
    abort();
  }

  return ptr(n, pErrorCode);
}
const char * ucnv_getStandardName(const char * name, const char * standard, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * name, const char * standard, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getStandardName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getStandardName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getStandardName");
    abort();
  }

  return ptr(name, standard, pErrorCode);
}
const char * ucnv_getCanonicalName(const char * alias, const char * standard, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * alias, const char * standard, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getCanonicalName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getCanonicalName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getCanonicalName");
    abort();
  }

  return ptr(alias, standard, pErrorCode);
}
const char * ucnv_getDefaultName() {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_getDefaultName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_getDefaultName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_getDefaultName");
    abort();
  }

  return ptr();
}
void ucnv_setDefaultName(const char * name) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * name);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setDefaultName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setDefaultName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setDefaultName");
    abort();
  }

  ptr(name);
}
void ucnv_fixFileSeparator(const UConverter * cnv, UChar * source, int32_t sourceLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UConverter * cnv, UChar * source, int32_t sourceLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_fixFileSeparator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_fixFileSeparator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_fixFileSeparator");
    abort();
  }

  ptr(cnv, source, sourceLen);
}
UBool ucnv_isAmbiguous(const UConverter * cnv) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UConverter * cnv);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_isAmbiguous") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_isAmbiguous", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_isAmbiguous");
    abort();
  }

  return ptr(cnv);
}
void ucnv_setFallback(UConverter * cnv, UBool usesFallback) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverter * cnv, UBool usesFallback);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_setFallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_setFallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_setFallback");
    abort();
  }

  ptr(cnv, usesFallback);
}
UBool ucnv_usesFallback(const UConverter * cnv) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UConverter * cnv);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_usesFallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_usesFallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_usesFallback");
    abort();
  }

  return ptr(cnv);
}
const char * ucnv_detectUnicodeSignature(const char * source, int32_t sourceLength, int32_t * signatureLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const char * source, int32_t sourceLength, int32_t * signatureLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_detectUnicodeSignature") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_detectUnicodeSignature", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_detectUnicodeSignature");
    abort();
  }

  return ptr(source, sourceLength, signatureLength, pErrorCode);
}
int32_t ucnv_fromUCountPending(const UConverter * cnv, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UConverter * cnv, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_fromUCountPending") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_fromUCountPending", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_fromUCountPending");
    abort();
  }

  return ptr(cnv, status);
}
int32_t ucnv_toUCountPending(const UConverter * cnv, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UConverter * cnv, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_toUCountPending") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_toUCountPending", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_toUCountPending");
    abort();
  }

  return ptr(cnv, status);
}
UBool ucnv_isFixedWidth(UConverter * cnv, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UConverter * cnv, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_isFixedWidth") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_isFixedWidth", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_isFixedWidth");
    abort();
  }

  return ptr(cnv, status);
}
UChar32 utf8_nextCharSafeBody(const uint8_t * s, int32_t * pi, int32_t length, UChar32 c, UBool strict) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(const uint8_t * s, int32_t * pi, int32_t length, UChar32 c, UBool strict);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utf8_nextCharSafeBody") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utf8_nextCharSafeBody", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utf8_nextCharSafeBody");
    abort();
  }

  return ptr(s, pi, length, c, strict);
}
int32_t utf8_appendCharSafeBody(uint8_t * s, int32_t i, int32_t length, UChar32 c, UBool * pIsError) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(uint8_t * s, int32_t i, int32_t length, UChar32 c, UBool * pIsError);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utf8_appendCharSafeBody") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utf8_appendCharSafeBody", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utf8_appendCharSafeBody");
    abort();
  }

  return ptr(s, i, length, c, pIsError);
}
UChar32 utf8_prevCharSafeBody(const uint8_t * s, int32_t start, int32_t * pi, UChar32 c, UBool strict) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(const uint8_t * s, int32_t start, int32_t * pi, UChar32 c, UBool strict);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utf8_prevCharSafeBody") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utf8_prevCharSafeBody", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utf8_prevCharSafeBody");
    abort();
  }

  return ptr(s, start, pi, c, strict);
}
int32_t utf8_back1SafeBody(const uint8_t * s, int32_t start, int32_t i) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const uint8_t * s, int32_t start, int32_t i);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utf8_back1SafeBody") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utf8_back1SafeBody", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utf8_back1SafeBody");
    abort();
  }

  return ptr(s, start, i);
}
UBiDi * ubidi_open() {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDi * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_open");
    abort();
  }

  return ptr();
}
UBiDi * ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDi * (*FuncPtr)(int32_t maxLength, int32_t maxRunCount, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_openSized") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_openSized", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_openSized");
    abort();
  }

  return ptr(maxLength, maxRunCount, pErrorCode);
}
void ubidi_close(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_close");
    abort();
  }

  ptr(pBiDi);
}
void ubidi_setInverse(UBiDi * pBiDi, UBool isInverse) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, UBool isInverse);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setInverse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setInverse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setInverse");
    abort();
  }

  ptr(pBiDi, isInverse);
}
UBool ubidi_isInverse(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_isInverse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_isInverse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_isInverse");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_orderParagraphsLTR(UBiDi * pBiDi, UBool orderParagraphsLTR) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, UBool orderParagraphsLTR);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_orderParagraphsLTR") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_orderParagraphsLTR", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_orderParagraphsLTR");
    abort();
  }

  ptr(pBiDi, orderParagraphsLTR);
}
UBool ubidi_isOrderParagraphsLTR(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_isOrderParagraphsLTR") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_isOrderParagraphsLTR", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_isOrderParagraphsLTR");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingMode(UBiDi * pBiDi, UBiDiReorderingMode reorderingMode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, UBiDiReorderingMode reorderingMode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setReorderingMode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setReorderingMode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setReorderingMode");
    abort();
  }

  ptr(pBiDi, reorderingMode);
}
UBiDiReorderingMode ubidi_getReorderingMode(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiReorderingMode (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getReorderingMode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getReorderingMode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getReorderingMode");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingOptions(UBiDi * pBiDi, uint32_t reorderingOptions) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, uint32_t reorderingOptions);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setReorderingOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setReorderingOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setReorderingOptions");
    abort();
  }

  ptr(pBiDi, reorderingOptions);
}
uint32_t ubidi_getReorderingOptions(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getReorderingOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getReorderingOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getReorderingOptions");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setContext(UBiDi * pBiDi, const UChar * prologue, int32_t proLength, const UChar * epilogue, int32_t epiLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, const UChar * prologue, int32_t proLength, const UChar * epilogue, int32_t epiLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setContext");
    abort();
  }

  ptr(pBiDi, prologue, proLength, epilogue, epiLength, pErrorCode);
}
void ubidi_setPara(UBiDi * pBiDi, const UChar * text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel * embeddingLevels, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, const UChar * text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel * embeddingLevels, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setPara") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setPara", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setPara");
    abort();
  }

  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
}
void ubidi_setLine(const UBiDi * pParaBiDi, int32_t start, int32_t limit, UBiDi * pLineBiDi, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UBiDi * pParaBiDi, int32_t start, int32_t limit, UBiDi * pLineBiDi, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setLine") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setLine", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setLine");
    abort();
  }

  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
}
UBiDiDirection ubidi_getDirection(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiDirection (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getDirection") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getDirection", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getDirection");
    abort();
  }

  return ptr(pBiDi);
}
UBiDiDirection ubidi_getBaseDirection(const UChar * text, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiDirection (*FuncPtr)(const UChar * text, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getBaseDirection") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getBaseDirection", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getBaseDirection");
    abort();
  }

  return ptr(text, length);
}
const UChar * ubidi_getText(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getText");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getLength(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLength");
    abort();
  }

  return ptr(pBiDi);
}
UBiDiLevel ubidi_getParaLevel(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiLevel (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getParaLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getParaLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getParaLevel");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_countParagraphs(UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_countParagraphs") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_countParagraphs", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_countParagraphs");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getParagraph(const UBiDi * pBiDi, int32_t charIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UBiDi * pBiDi, int32_t charIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getParagraph") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getParagraph", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getParagraph");
    abort();
  }

  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
void ubidi_getParagraphByIndex(const UBiDi * pBiDi, int32_t paraIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UBiDi * pBiDi, int32_t paraIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getParagraphByIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getParagraphByIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getParagraphByIndex");
    abort();
  }

  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
UBiDiLevel ubidi_getLevelAt(const UBiDi * pBiDi, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiLevel (*FuncPtr)(const UBiDi * pBiDi, int32_t charIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLevelAt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLevelAt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLevelAt");
    abort();
  }

  return ptr(pBiDi, charIndex);
}
const UBiDiLevel * ubidi_getLevels(UBiDi * pBiDi, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UBiDiLevel * (*FuncPtr)(UBiDi * pBiDi, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLevels") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLevels", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLevels");
    abort();
  }

  return ptr(pBiDi, pErrorCode);
}
void ubidi_getLogicalRun(const UBiDi * pBiDi, int32_t logicalPosition, int32_t * pLogicalLimit, UBiDiLevel * pLevel) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UBiDi * pBiDi, int32_t logicalPosition, int32_t * pLogicalLimit, UBiDiLevel * pLevel);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLogicalRun") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLogicalRun", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLogicalRun");
    abort();
  }

  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
}
int32_t ubidi_countRuns(UBiDi * pBiDi, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBiDi * pBiDi, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_countRuns") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_countRuns", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_countRuns");
    abort();
  }

  return ptr(pBiDi, pErrorCode);
}
UBiDiDirection ubidi_getVisualRun(UBiDi * pBiDi, int32_t runIndex, int32_t * pLogicalStart, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiDirection (*FuncPtr)(UBiDi * pBiDi, int32_t runIndex, int32_t * pLogicalStart, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getVisualRun") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getVisualRun", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getVisualRun");
    abort();
  }

  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}
int32_t ubidi_getVisualIndex(UBiDi * pBiDi, int32_t logicalIndex, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBiDi * pBiDi, int32_t logicalIndex, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getVisualIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getVisualIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getVisualIndex");
    abort();
  }

  return ptr(pBiDi, logicalIndex, pErrorCode);
}
int32_t ubidi_getLogicalIndex(UBiDi * pBiDi, int32_t visualIndex, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBiDi * pBiDi, int32_t visualIndex, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLogicalIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLogicalIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLogicalIndex");
    abort();
  }

  return ptr(pBiDi, visualIndex, pErrorCode);
}
void ubidi_getLogicalMap(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getLogicalMap") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getLogicalMap", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getLogicalMap");
    abort();
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_getVisualMap(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getVisualMap") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getVisualMap", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getVisualMap");
    abort();
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_reorderLogical(const UBiDiLevel * levels, int32_t length, int32_t * indexMap) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UBiDiLevel * levels, int32_t length, int32_t * indexMap);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_reorderLogical") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_reorderLogical", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_reorderLogical");
    abort();
  }

  ptr(levels, length, indexMap);
}
void ubidi_reorderVisual(const UBiDiLevel * levels, int32_t length, int32_t * indexMap) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UBiDiLevel * levels, int32_t length, int32_t * indexMap);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_reorderVisual") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_reorderVisual", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_reorderVisual");
    abort();
  }

  ptr(levels, length, indexMap);
}
void ubidi_invertMap(const int32_t * srcMap, int32_t * destMap, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const int32_t * srcMap, int32_t * destMap, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_invertMap") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_invertMap", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_invertMap");
    abort();
  }

  ptr(srcMap, destMap, length);
}
int32_t ubidi_getProcessedLength(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getProcessedLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getProcessedLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getProcessedLength");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getResultLength(const UBiDi * pBiDi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UBiDi * pBiDi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getResultLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getResultLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getResultLength");
    abort();
  }

  return ptr(pBiDi);
}
UCharDirection ubidi_getCustomizedClass(UBiDi * pBiDi, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCharDirection (*FuncPtr)(UBiDi * pBiDi, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getCustomizedClass") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getCustomizedClass", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getCustomizedClass");
    abort();
  }

  return ptr(pBiDi, c);
}
void ubidi_setClassCallback(UBiDi * pBiDi, UBiDiClassCallback * newFn, const void * newContext, UBiDiClassCallback ** oldFn, const void ** oldContext, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, UBiDiClassCallback * newFn, const void * newContext, UBiDiClassCallback ** oldFn, const void ** oldContext, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_setClassCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_setClassCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_setClassCallback");
    abort();
  }

  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
}
void ubidi_getClassCallback(UBiDi * pBiDi, UBiDiClassCallback ** fn, const void ** context) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDi * pBiDi, UBiDiClassCallback ** fn, const void ** context);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_getClassCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_getClassCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_getClassCallback");
    abort();
  }

  ptr(pBiDi, fn, context);
}
int32_t ubidi_writeReordered(UBiDi * pBiDi, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBiDi * pBiDi, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_writeReordered") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_writeReordered", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_writeReordered");
    abort();
  }

  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}
int32_t ubidi_writeReverse(const UChar * src, int32_t srcLength, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * src, int32_t srcLength, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubidi_writeReverse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubidi_writeReverse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubidi_writeReverse");
    abort();
  }

  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}
int32_t u_strlen(const UChar * s) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strlen") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strlen", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strlen");
    abort();
  }

  return ptr(s);
}
int32_t u_countChar32(const UChar * s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_countChar32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_countChar32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_countChar32");
    abort();
  }

  return ptr(s, length);
}
UBool u_strHasMoreChar32Than(const UChar * s, int32_t length, int32_t number) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UChar * s, int32_t length, int32_t number);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strHasMoreChar32Than") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strHasMoreChar32Than", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strHasMoreChar32Than");
    abort();
  }

  return ptr(s, length, number);
}
UChar * u_strcat(UChar * dst, const UChar * src) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const UChar * src);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcat");
    abort();
  }

  return ptr(dst, src);
}
UChar * u_strncat(UChar * dst, const UChar * src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const UChar * src, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strncat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strncat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strncat");
    abort();
  }

  return ptr(dst, src, n);
}
UChar * u_strstr(const UChar * s, const UChar * substring) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, const UChar * substring);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strstr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strstr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strstr");
    abort();
  }

  return ptr(s, substring);
}
UChar * u_strFindFirst(const UChar * s, int32_t length, const UChar * substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, int32_t length, const UChar * substring, int32_t subLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFindFirst") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFindFirst", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFindFirst");
    abort();
  }

  return ptr(s, length, substring, subLength);
}
UChar * u_strchr(const UChar * s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strchr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strchr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strchr");
    abort();
  }

  return ptr(s, c);
}
UChar * u_strchr32(const UChar * s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strchr32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strchr32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strchr32");
    abort();
  }

  return ptr(s, c);
}
UChar * u_strrstr(const UChar * s, const UChar * substring) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, const UChar * substring);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strrstr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strrstr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strrstr");
    abort();
  }

  return ptr(s, substring);
}
UChar * u_strFindLast(const UChar * s, int32_t length, const UChar * substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, int32_t length, const UChar * substring, int32_t subLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFindLast") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFindLast", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFindLast");
    abort();
  }

  return ptr(s, length, substring, subLength);
}
UChar * u_strrchr(const UChar * s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strrchr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strrchr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strrchr");
    abort();
  }

  return ptr(s, c);
}
UChar * u_strrchr32(const UChar * s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strrchr32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strrchr32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strrchr32");
    abort();
  }

  return ptr(s, c);
}
UChar * u_strpbrk(const UChar * string, const UChar * matchSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * string, const UChar * matchSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strpbrk") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strpbrk", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strpbrk");
    abort();
  }

  return ptr(string, matchSet);
}
int32_t u_strcspn(const UChar * string, const UChar * matchSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * string, const UChar * matchSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcspn") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcspn", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcspn");
    abort();
  }

  return ptr(string, matchSet);
}
int32_t u_strspn(const UChar * string, const UChar * matchSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * string, const UChar * matchSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strspn") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strspn", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strspn");
    abort();
  }

  return ptr(string, matchSet);
}
UChar * u_strtok_r(UChar * src, const UChar * delim, UChar ** saveState) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * src, const UChar * delim, UChar ** saveState);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strtok_r") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strtok_r", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strtok_r");
    abort();
  }

  return ptr(src, delim, saveState);
}
int32_t u_strcmp(const UChar * s1, const UChar * s2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcmp");
    abort();
  }

  return ptr(s1, s2);
}
int32_t u_strcmpCodePointOrder(const UChar * s1, const UChar * s2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcmpCodePointOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcmpCodePointOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2);
}
int32_t u_strCompare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, UBool codePointOrder);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strCompare") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strCompare", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strCompare");
    abort();
  }

  return ptr(s1, length1, s2, length2, codePointOrder);
}
int32_t u_strCompareIter(UCharIterator * iter1, UCharIterator * iter2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCharIterator * iter1, UCharIterator * iter2, UBool codePointOrder);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strCompareIter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strCompareIter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strCompareIter");
    abort();
  }

  return ptr(iter1, iter2, codePointOrder);
}
int32_t u_strCaseCompare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strCaseCompare") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strCaseCompare", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strCaseCompare");
    abort();
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
int32_t u_strncmp(const UChar * ucs1, const UChar * ucs2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * ucs1, const UChar * ucs2, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strncmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strncmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strncmp");
    abort();
  }

  return ptr(ucs1, ucs2, n);
}
int32_t u_strncmpCodePointOrder(const UChar * s1, const UChar * s2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strncmpCodePointOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strncmpCodePointOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strncmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2, n);
}
int32_t u_strcasecmp(const UChar * s1, const UChar * s2, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2, uint32_t options);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcasecmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcasecmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcasecmp");
    abort();
  }

  return ptr(s1, s2, options);
}
int32_t u_strncasecmp(const UChar * s1, const UChar * s2, int32_t n, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2, int32_t n, uint32_t options);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strncasecmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strncasecmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strncasecmp");
    abort();
  }

  return ptr(s1, s2, n, options);
}
int32_t u_memcasecmp(const UChar * s1, const UChar * s2, int32_t length, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2, int32_t length, uint32_t options);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memcasecmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memcasecmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memcasecmp");
    abort();
  }

  return ptr(s1, s2, length, options);
}
UChar * u_strcpy(UChar * dst, const UChar * src) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const UChar * src);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strcpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strcpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strcpy");
    abort();
  }

  return ptr(dst, src);
}
UChar * u_strncpy(UChar * dst, const UChar * src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const UChar * src, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strncpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strncpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strncpy");
    abort();
  }

  return ptr(dst, src, n);
}
UChar * u_uastrcpy(UChar * dst, const char * src) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const char * src);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_uastrcpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_uastrcpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_uastrcpy");
    abort();
  }

  return ptr(dst, src);
}
UChar * u_uastrncpy(UChar * dst, const char * src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dst, const char * src, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_uastrncpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_uastrncpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_uastrncpy");
    abort();
  }

  return ptr(dst, src, n);
}
char * u_austrcpy(char * dst, const UChar * src) {
  pthread_once(&once_control, &init_icudata_version);

  typedef char * (*FuncPtr)(char * dst, const UChar * src);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_austrcpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_austrcpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_austrcpy");
    abort();
  }

  return ptr(dst, src);
}
char * u_austrncpy(char * dst, const UChar * src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);

  typedef char * (*FuncPtr)(char * dst, const UChar * src, int32_t n);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_austrncpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_austrncpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_austrncpy");
    abort();
  }

  return ptr(dst, src, n);
}
UChar * u_memcpy(UChar * dest, const UChar * src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, const UChar * src, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memcpy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memcpy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memcpy");
    abort();
  }

  return ptr(dest, src, count);
}
UChar * u_memmove(UChar * dest, const UChar * src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, const UChar * src, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memmove") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memmove", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memmove");
    abort();
  }

  return ptr(dest, src, count);
}
UChar * u_memset(UChar * dest, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, UChar c, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memset");
    abort();
  }

  return ptr(dest, c, count);
}
int32_t u_memcmp(const UChar * buf1, const UChar * buf2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * buf1, const UChar * buf2, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memcmp") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memcmp", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memcmp");
    abort();
  }

  return ptr(buf1, buf2, count);
}
int32_t u_memcmpCodePointOrder(const UChar * s1, const UChar * s2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, const UChar * s2, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memcmpCodePointOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memcmpCodePointOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memcmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2, count);
}
UChar * u_memchr(const UChar * s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar c, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memchr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memchr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memchr");
    abort();
  }

  return ptr(s, c, count);
}
UChar * u_memchr32(const UChar * s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar32 c, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memchr32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memchr32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memchr32");
    abort();
  }

  return ptr(s, c, count);
}
UChar * u_memrchr(const UChar * s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar c, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memrchr") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memrchr", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memrchr");
    abort();
  }

  return ptr(s, c, count);
}
UChar * u_memrchr32(const UChar * s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(const UChar * s, UChar32 c, int32_t count);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_memrchr32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_memrchr32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_memrchr32");
    abort();
  }

  return ptr(s, c, count);
}
int32_t u_unescape(const char * src, UChar * dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * src, UChar * dest, int32_t destCapacity);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_unescape") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_unescape", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_unescape");
    abort();
  }

  return ptr(src, dest, destCapacity);
}
UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t * offset, int32_t length, void * context) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UNESCAPE_CHAR_AT charAt, int32_t * offset, int32_t length, void * context);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_unescapeAt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_unescapeAt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_unescapeAt");
    abort();
  }

  return ptr(charAt, offset, length, context);
}
int32_t u_strToUpper(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToUpper") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToUpper", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToUpper");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToLower(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToLower") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToLower", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToLower");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToTitle(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UBreakIterator * titleIter, const char * locale, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UBreakIterator * titleIter, const char * locale, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToTitle") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToTitle", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToTitle");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}
int32_t u_strFoldCase(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFoldCase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFoldCase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFoldCase");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}
int * u_strToWCS(int * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int * (*FuncPtr)(int * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToWCS") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToWCS", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToWCS");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromWCS(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const int * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const int * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromWCS") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromWCS", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromWCS");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char * u_strToUTF8(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef char * (*FuncPtr)(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromUTF8(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char * u_strToUTF8WithSub(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef char * (*FuncPtr)(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToUTF8WithSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToUTF8WithSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF8WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromUTF8WithSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromUTF8WithSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF8Lenient(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromUTF8Lenient") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromUTF8Lenient", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromUTF8Lenient");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32 * u_strToUTF32(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 * (*FuncPtr)(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToUTF32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToUTF32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToUTF32");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromUTF32(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromUTF32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromUTF32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromUTF32");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32 * u_strToUTF32WithSub(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 * (*FuncPtr)(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToUTF32WithSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToUTF32WithSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToUTF32WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF32WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromUTF32WithSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromUTF32WithSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromUTF32WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
char * u_strToJavaModifiedUTF8(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef char * (*FuncPtr)(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strToJavaModifiedUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strToJavaModifiedUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strToJavaModifiedUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromJavaModifiedUTF8WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar * (*FuncPtr)(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_strFromJavaModifiedUTF8WithSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_strFromJavaModifiedUTF8WithSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_strFromJavaModifiedUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
u_nl_catd u_catopen(const char * name, const char * locale, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef u_nl_catd (*FuncPtr)(const char * name, const char * locale, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_catopen") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_catopen", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_catopen");
    abort();
  }

  return ptr(name, locale, ec);
}
void u_catclose(u_nl_catd catd) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(u_nl_catd catd);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_catclose") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_catclose", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_catclose");
    abort();
  }

  ptr(catd);
}
const UChar * u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar * s, int32_t * len, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar * s, int32_t * len, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_catgets") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_catgets", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_catgets");
    abort();
  }

  return ptr(catd, set_num, msg_num, s, len, ec);
}
UIDNA * uidna_openUTS46(uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UIDNA * (*FuncPtr)(uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_openUTS46") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_openUTS46", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_openUTS46");
    abort();
  }

  return ptr(options, pErrorCode);
}
void uidna_close(UIDNA * idna) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UIDNA * idna);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_close");
    abort();
  }

  ptr(idna);
}
int32_t uidna_labelToASCII(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_labelToASCII") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_labelToASCII", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_labelToASCII");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicode(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_labelToUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_labelToUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_labelToUnicode");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_nameToASCII") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_nameToASCII", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_nameToASCII");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicode(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_nameToUnicode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_nameToUnicode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_nameToUnicode");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToASCII_UTF8(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_labelToASCII_UTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_labelToASCII_UTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_labelToASCII_UTF8");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicodeUTF8(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_labelToUnicodeUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_labelToUnicodeUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_labelToUnicodeUTF8");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII_UTF8(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_nameToASCII_UTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_nameToASCII_UTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_nameToASCII_UTF8");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicodeUTF8(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uidna_nameToUnicodeUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uidna_nameToUnicodeUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uidna_nameToUnicodeUTF8");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
void ucnv_cbFromUWriteBytes(UConverterFromUnicodeArgs * args, const char * source, int32_t length, int32_t offsetIndex, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterFromUnicodeArgs * args, const char * source, int32_t length, int32_t offsetIndex, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_cbFromUWriteBytes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_cbFromUWriteBytes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_cbFromUWriteBytes");
    abort();
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbFromUWriteSub(UConverterFromUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterFromUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_cbFromUWriteSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_cbFromUWriteSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_cbFromUWriteSub");
    abort();
  }

  ptr(args, offsetIndex, err);
}
void ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs * args, const UChar ** source, const UChar * sourceLimit, int32_t offsetIndex, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterFromUnicodeArgs * args, const UChar ** source, const UChar * sourceLimit, int32_t offsetIndex, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_cbFromUWriteUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_cbFromUWriteUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_cbFromUWriteUChars");
    abort();
  }

  ptr(args, source, sourceLimit, offsetIndex, err);
}
void ucnv_cbToUWriteUChars(UConverterToUnicodeArgs * args, const UChar * source, int32_t length, int32_t offsetIndex, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterToUnicodeArgs * args, const UChar * source, int32_t length, int32_t offsetIndex, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_cbToUWriteUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_cbToUWriteUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_cbToUWriteUChars");
    abort();
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbToUWriteSub(UConverterToUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterToUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnv_cbToUWriteSub") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnv_cbToUWriteSub", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnv_cbToUWriteSub");
    abort();
  }

  ptr(args, offsetIndex, err);
}
ULocaleDisplayNames * uldn_open(const char * locale, UDialectHandling dialectHandling, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef ULocaleDisplayNames * (*FuncPtr)(const char * locale, UDialectHandling dialectHandling, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_open");
    abort();
  }

  return ptr(locale, dialectHandling, pErrorCode);
}
void uldn_close(ULocaleDisplayNames * ldn) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(ULocaleDisplayNames * ldn);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_close");
    abort();
  }

  ptr(ldn);
}
const char * uldn_getLocale(const ULocaleDisplayNames * ldn) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const ULocaleDisplayNames * ldn);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_getLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_getLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_getLocale");
    abort();
  }

  return ptr(ldn);
}
UDialectHandling uldn_getDialectHandling(const ULocaleDisplayNames * ldn) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDialectHandling (*FuncPtr)(const ULocaleDisplayNames * ldn);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_getDialectHandling") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_getDialectHandling", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_getDialectHandling");
    abort();
  }

  return ptr(ldn);
}
int32_t uldn_localeDisplayName(const ULocaleDisplayNames * ldn, const char * locale, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * locale, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_localeDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_localeDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_localeDisplayName");
    abort();
  }

  return ptr(ldn, locale, result, maxResultSize, pErrorCode);
}
int32_t uldn_languageDisplayName(const ULocaleDisplayNames * ldn, const char * lang, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * lang, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_languageDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_languageDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_languageDisplayName");
    abort();
  }

  return ptr(ldn, lang, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptDisplayName(const ULocaleDisplayNames * ldn, const char * script, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * script, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_scriptDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_scriptDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_scriptDisplayName");
    abort();
  }

  return ptr(ldn, script, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptCodeDisplayName(const ULocaleDisplayNames * ldn, UScriptCode scriptCode, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, UScriptCode scriptCode, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_scriptCodeDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_scriptCodeDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_scriptCodeDisplayName");
    abort();
  }

  return ptr(ldn, scriptCode, result, maxResultSize, pErrorCode);
}
int32_t uldn_regionDisplayName(const ULocaleDisplayNames * ldn, const char * region, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * region, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_regionDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_regionDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_regionDisplayName");
    abort();
  }

  return ptr(ldn, region, result, maxResultSize, pErrorCode);
}
int32_t uldn_variantDisplayName(const ULocaleDisplayNames * ldn, const char * variant, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * variant, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_variantDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_variantDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_variantDisplayName");
    abort();
  }

  return ptr(ldn, variant, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyDisplayName(const ULocaleDisplayNames * ldn, const char * key, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * key, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_keyDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_keyDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_keyDisplayName");
    abort();
  }

  return ptr(ldn, key, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyValueDisplayName(const ULocaleDisplayNames * ldn, const char * key, const char * value, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const ULocaleDisplayNames * ldn, const char * key, const char * value, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_keyValueDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_keyValueDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_keyValueDisplayName");
    abort();
  }

  return ptr(ldn, key, value, result, maxResultSize, pErrorCode);
}
ULocaleDisplayNames * uldn_openForContext(const char * locale, UDisplayContext * contexts, int32_t length, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef ULocaleDisplayNames * (*FuncPtr)(const char * locale, UDisplayContext * contexts, int32_t length, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_openForContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_openForContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_openForContext");
    abort();
  }

  return ptr(locale, contexts, length, pErrorCode);
}
UDisplayContext uldn_getContext(const ULocaleDisplayNames * ldn, UDisplayContextType type, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDisplayContext (*FuncPtr)(const ULocaleDisplayNames * ldn, UDisplayContextType type, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uldn_getContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uldn_getContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uldn_getContext");
    abort();
  }

  return ptr(ldn, type, pErrorCode);
}
void u_init(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_init") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_init", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_init");
    abort();
  }

  ptr(status);
}
void u_cleanup() {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_cleanup") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_cleanup", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_cleanup");
    abort();
  }

  ptr();
}
void u_setMemoryFunctions(const void * context, UMemAllocFn * a, UMemReallocFn * r, UMemFreeFn * f, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UMemAllocFn * a, UMemReallocFn * r, UMemFreeFn * f, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_setMemoryFunctions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_setMemoryFunctions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_setMemoryFunctions");
    abort();
  }

  ptr(context, a, r, f, status);
}
const char * u_errorName(UErrorCode code) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UErrorCode code);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_errorName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_errorName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_errorName");
    abort();
  }

  return ptr(code);
}
int32_t ucurr_forLocale(const char * locale, UChar * buff, int32_t buffCapacity, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, UChar * buff, int32_t buffCapacity, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_forLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_forLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_forLocale");
    abort();
  }

  return ptr(locale, buff, buffCapacity, ec);
}
UCurrRegistryKey ucurr_register(const UChar * isoCode, const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCurrRegistryKey (*FuncPtr)(const UChar * isoCode, const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_register") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_register", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_register");
    abort();
  }

  return ptr(isoCode, locale, status);
}
UBool ucurr_unregister(UCurrRegistryKey key, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UCurrRegistryKey key, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_unregister") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_unregister", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_unregister");
    abort();
  }

  return ptr(key, status);
}
const UChar * ucurr_getName(const UChar * currency, const char * locale, UCurrNameStyle nameStyle, UBool * isChoiceFormat, int32_t * len, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UChar * currency, const char * locale, UCurrNameStyle nameStyle, UBool * isChoiceFormat, int32_t * len, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getName");
    abort();
  }

  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}
const UChar * ucurr_getPluralName(const UChar * currency, const char * locale, UBool * isChoiceFormat, const char * pluralCount, int32_t * len, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UChar * currency, const char * locale, UBool * isChoiceFormat, const char * pluralCount, int32_t * len, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getPluralName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getPluralName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getPluralName");
    abort();
  }

  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}
int32_t ucurr_getDefaultFractionDigits(const UChar * currency, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * currency, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getDefaultFractionDigits") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getDefaultFractionDigits", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getDefaultFractionDigits");
    abort();
  }

  return ptr(currency, ec);
}
int32_t ucurr_getDefaultFractionDigitsForUsage(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getDefaultFractionDigitsForUsage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getDefaultFractionDigitsForUsage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getDefaultFractionDigitsForUsage");
    abort();
  }

  return ptr(currency, usage, ec);
}
double ucurr_getRoundingIncrement(const UChar * currency, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(const UChar * currency, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getRoundingIncrement") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getRoundingIncrement", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getRoundingIncrement");
    abort();
  }

  return ptr(currency, ec);
}
double ucurr_getRoundingIncrementForUsage(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getRoundingIncrementForUsage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getRoundingIncrementForUsage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getRoundingIncrementForUsage");
    abort();
  }

  return ptr(currency, usage, ec);
}
UEnumeration * ucurr_openISOCurrencies(uint32_t currType, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(uint32_t currType, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_openISOCurrencies") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_openISOCurrencies", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_openISOCurrencies");
    abort();
  }

  return ptr(currType, pErrorCode);
}
UBool ucurr_isAvailable(const UChar * isoCode, UDate from, UDate to, UErrorCode * errorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UChar * isoCode, UDate from, UDate to, UErrorCode * errorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_isAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_isAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_isAvailable");
    abort();
  }

  return ptr(isoCode, from, to, errorCode);
}
int32_t ucurr_countCurrencies(const char * locale, UDate date, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, UDate date, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_countCurrencies") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_countCurrencies", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_countCurrencies");
    abort();
  }

  return ptr(locale, date, ec);
}
int32_t ucurr_forLocaleAndDate(const char * locale, UDate date, int32_t index, UChar * buff, int32_t buffCapacity, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, UDate date, int32_t index, UChar * buff, int32_t buffCapacity, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_forLocaleAndDate") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_forLocaleAndDate", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_forLocaleAndDate");
    abort();
  }

  return ptr(locale, date, index, buff, buffCapacity, ec);
}
UEnumeration * ucurr_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getKeywordValuesForLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getKeywordValuesForLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucurr_getNumericCode(const UChar * currency) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * currency);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucurr_getNumericCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucurr_getNumericCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucurr_getNumericCode");
    abort();
  }

  return ptr(currency);
}
USet * uset_openEmpty() {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_openEmpty") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_openEmpty", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_openEmpty");
    abort();
  }

  return ptr();
}
USet * uset_open(UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_open");
    abort();
  }

  return ptr(start, end);
}
USet * uset_openPattern(const UChar * pattern, int32_t patternLength, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const UChar * pattern, int32_t patternLength, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_openPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_openPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_openPattern");
    abort();
  }

  return ptr(pattern, patternLength, ec);
}
USet * uset_openPatternOptions(const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_openPatternOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_openPatternOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_openPatternOptions");
    abort();
  }

  return ptr(pattern, patternLength, options, ec);
}
void uset_close(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_close");
    abort();
  }

  ptr(set);
}
USet * uset_clone(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_clone");
    abort();
  }

  return ptr(set);
}
UBool uset_isFrozen(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_isFrozen") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_isFrozen", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_isFrozen");
    abort();
  }

  return ptr(set);
}
void uset_freeze(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_freeze") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_freeze", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_freeze");
    abort();
  }

  ptr(set);
}
USet * uset_cloneAsThawed(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_cloneAsThawed") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_cloneAsThawed", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_cloneAsThawed");
    abort();
  }

  return ptr(set);
}
void uset_set(USet * set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_set") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_set", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_set");
    abort();
  }

  ptr(set, start, end);
}
int32_t uset_applyPattern(USet * set, const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(USet * set, const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_applyPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_applyPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_applyPattern");
    abort();
  }

  return ptr(set, pattern, patternLength, options, status);
}
void uset_applyIntPropertyValue(USet * set, UProperty prop, int32_t value, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UProperty prop, int32_t value, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_applyIntPropertyValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_applyIntPropertyValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_applyIntPropertyValue");
    abort();
  }

  ptr(set, prop, value, ec);
}
void uset_applyPropertyAlias(USet * set, const UChar * prop, int32_t propLength, const UChar * value, int32_t valueLength, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const UChar * prop, int32_t propLength, const UChar * value, int32_t valueLength, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_applyPropertyAlias") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_applyPropertyAlias", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_applyPropertyAlias");
    abort();
  }

  ptr(set, prop, propLength, value, valueLength, ec);
}
UBool uset_resemblesPattern(const UChar * pattern, int32_t patternLength, int32_t pos) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UChar * pattern, int32_t patternLength, int32_t pos);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_resemblesPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_resemblesPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_resemblesPattern");
    abort();
  }

  return ptr(pattern, patternLength, pos);
}
int32_t uset_toPattern(const USet * set, UChar * result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, UChar * result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_toPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_toPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_toPattern");
    abort();
  }

  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}
void uset_add(USet * set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_add") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_add", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_add");
    abort();
  }

  ptr(set, c);
}
void uset_addAll(USet * set, const USet * additionalSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const USet * additionalSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_addAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_addAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_addAll");
    abort();
  }

  ptr(set, additionalSet);
}
void uset_addRange(USet * set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_addRange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_addRange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_addRange");
    abort();
  }

  ptr(set, start, end);
}
void uset_addString(USet * set, const UChar * str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const UChar * str, int32_t strLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_addString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_addString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_addString");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_addAllCodePoints(USet * set, const UChar * str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const UChar * str, int32_t strLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_addAllCodePoints") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_addAllCodePoints", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_addAllCodePoints");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_remove(USet * set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_remove") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_remove", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_remove");
    abort();
  }

  ptr(set, c);
}
void uset_removeRange(USet * set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_removeRange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_removeRange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_removeRange");
    abort();
  }

  ptr(set, start, end);
}
void uset_removeString(USet * set, const UChar * str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const UChar * str, int32_t strLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_removeString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_removeString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_removeString");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_removeAll(USet * set, const USet * removeSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const USet * removeSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_removeAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_removeAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_removeAll");
    abort();
  }

  ptr(set, removeSet);
}
void uset_retain(USet * set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_retain") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_retain", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_retain");
    abort();
  }

  ptr(set, start, end);
}
void uset_retainAll(USet * set, const USet * retain) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const USet * retain);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_retainAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_retainAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_retainAll");
    abort();
  }

  ptr(set, retain);
}
void uset_compact(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_compact") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_compact", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_compact");
    abort();
  }

  ptr(set);
}
void uset_complement(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_complement") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_complement", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_complement");
    abort();
  }

  ptr(set);
}
void uset_complementAll(USet * set, const USet * complement) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, const USet * complement);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_complementAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_complementAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_complementAll");
    abort();
  }

  ptr(set, complement);
}
void uset_clear(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_clear") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_clear", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_clear");
    abort();
  }

  ptr(set);
}
void uset_closeOver(USet * set, int32_t attributes) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set, int32_t attributes);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_closeOver") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_closeOver", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_closeOver");
    abort();
  }

  ptr(set, attributes);
}
void uset_removeAllStrings(USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_removeAllStrings") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_removeAllStrings", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_removeAllStrings");
    abort();
  }

  ptr(set);
}
UBool uset_isEmpty(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_isEmpty") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_isEmpty", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_isEmpty");
    abort();
  }

  return ptr(set);
}
UBool uset_contains(const USet * set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_contains") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_contains", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_contains");
    abort();
  }

  return ptr(set, c);
}
UBool uset_containsRange(const USet * set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set, UChar32 start, UChar32 end);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsRange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsRange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsRange");
    abort();
  }

  return ptr(set, start, end);
}
UBool uset_containsString(const USet * set, const UChar * str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set, const UChar * str, int32_t strLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsString");
    abort();
  }

  return ptr(set, str, strLen);
}
int32_t uset_indexOf(const USet * set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_indexOf") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_indexOf", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_indexOf");
    abort();
  }

  return ptr(set, c);
}
UChar32 uset_charAt(const USet * set, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(const USet * set, int32_t charIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_charAt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_charAt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_charAt");
    abort();
  }

  return ptr(set, charIndex);
}
int32_t uset_size(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_size") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_size", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_size");
    abort();
  }

  return ptr(set);
}
int32_t uset_getItemCount(const USet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_getItemCount") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_getItemCount", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_getItemCount");
    abort();
  }

  return ptr(set);
}
int32_t uset_getItem(const USet * set, int32_t itemIndex, UChar32 * start, UChar32 * end, UChar * str, int32_t strCapacity, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, int32_t itemIndex, UChar32 * start, UChar32 * end, UChar * str, int32_t strCapacity, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_getItem") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_getItem", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_getItem");
    abort();
  }

  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}
UBool uset_containsAll(const USet * set1, const USet * set2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set1, const USet * set2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsAll");
    abort();
  }

  return ptr(set1, set2);
}
UBool uset_containsAllCodePoints(const USet * set, const UChar * str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set, const UChar * str, int32_t strLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsAllCodePoints") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsAllCodePoints", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsAllCodePoints");
    abort();
  }

  return ptr(set, str, strLen);
}
UBool uset_containsNone(const USet * set1, const USet * set2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set1, const USet * set2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsNone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsNone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsNone");
    abort();
  }

  return ptr(set1, set2);
}
UBool uset_containsSome(const USet * set1, const USet * set2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set1, const USet * set2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_containsSome") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_containsSome", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_containsSome");
    abort();
  }

  return ptr(set1, set2);
}
int32_t uset_span(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_span") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_span", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_span");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBack(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_spanBack") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_spanBack", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_spanBack");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanUTF8(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_spanUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_spanUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_spanUTF8");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBackUTF8(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_spanBackUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_spanBackUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_spanBackUTF8");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
UBool uset_equals(const USet * set1, const USet * set2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USet * set1, const USet * set2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_equals") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_equals", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_equals");
    abort();
  }

  return ptr(set1, set2);
}
int32_t uset_serialize(const USet * set, uint16_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USet * set, uint16_t * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_serialize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_serialize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_serialize");
    abort();
  }

  return ptr(set, dest, destCapacity, pErrorCode);
}
UBool uset_getSerializedSet(USerializedSet * fillSet, const uint16_t * src, int32_t srcLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(USerializedSet * fillSet, const uint16_t * src, int32_t srcLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_getSerializedSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_getSerializedSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_getSerializedSet");
    abort();
  }

  return ptr(fillSet, src, srcLength);
}
void uset_setSerializedToOne(USerializedSet * fillSet, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USerializedSet * fillSet, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_setSerializedToOne") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_setSerializedToOne", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_setSerializedToOne");
    abort();
  }

  ptr(fillSet, c);
}
UBool uset_serializedContains(const USerializedSet * set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USerializedSet * set, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_serializedContains") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_serializedContains", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_serializedContains");
    abort();
  }

  return ptr(set, c);
}
int32_t uset_getSerializedRangeCount(const USerializedSet * set) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USerializedSet * set);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_getSerializedRangeCount") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_getSerializedRangeCount", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_getSerializedRangeCount");
    abort();
  }

  return ptr(set);
}
UBool uset_getSerializedRange(const USerializedSet * set, int32_t rangeIndex, UChar32 * pStart, UChar32 * pEnd) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const USerializedSet * set, int32_t rangeIndex, UChar32 * pStart, UChar32 * pEnd);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uset_getSerializedRange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uset_getSerializedRange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uset_getSerializedRange");
    abort();
  }

  return ptr(set, rangeIndex, pStart, pEnd);
}
int32_t u_shapeArabic(const UChar * source, int32_t sourceLength, UChar * dest, int32_t destSize, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * source, int32_t sourceLength, UChar * dest, int32_t destSize, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_shapeArabic") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_shapeArabic", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_shapeArabic");
    abort();
  }

  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}
UBreakIterator * ubrk_open(UBreakIteratorType type, const char * locale, const UChar * text, int32_t textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBreakIterator * (*FuncPtr)(UBreakIteratorType type, const char * locale, const UChar * text, int32_t textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_open");
    abort();
  }

  return ptr(type, locale, text, textLength, status);
}
UBreakIterator * ubrk_openRules(const UChar * rules, int32_t rulesLength, const UChar * text, int32_t textLength, UParseError * parseErr, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBreakIterator * (*FuncPtr)(const UChar * rules, int32_t rulesLength, const UChar * text, int32_t textLength, UParseError * parseErr, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_openRules") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_openRules", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_openRules");
    abort();
  }

  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}
UBreakIterator * ubrk_safeClone(const UBreakIterator * bi, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBreakIterator * (*FuncPtr)(const UBreakIterator * bi, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_safeClone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_safeClone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_safeClone");
    abort();
  }

  return ptr(bi, stackBuffer, pBufferSize, status);
}
void ubrk_close(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_close");
    abort();
  }

  ptr(bi);
}
void ubrk_setText(UBreakIterator * bi, const UChar * text, int32_t textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBreakIterator * bi, const UChar * text, int32_t textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_setText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_setText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_setText");
    abort();
  }

  ptr(bi, text, textLength, status);
}
void ubrk_setUText(UBreakIterator * bi, UText * text, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBreakIterator * bi, UText * text, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_setUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_setUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_setUText");
    abort();
  }

  ptr(bi, text, status);
}
int32_t ubrk_current(const UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_current") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_current", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_current");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_next(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_next") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_next", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_next");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_previous(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_previous") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_previous", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_previous");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_first(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_first") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_first", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_first");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_last(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_last") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_last", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_last");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_preceding(UBreakIterator * bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi, int32_t offset);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_preceding") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_preceding", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_preceding");
    abort();
  }

  return ptr(bi, offset);
}
int32_t ubrk_following(UBreakIterator * bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi, int32_t offset);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_following") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_following", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_following");
    abort();
  }

  return ptr(bi, offset);
}
const char * ubrk_getAvailable(int32_t index) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t index);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_getAvailable");
    abort();
  }

  return ptr(index);
}
int32_t ubrk_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_countAvailable");
    abort();
  }

  return ptr();
}
UBool ubrk_isBoundary(UBreakIterator * bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UBreakIterator * bi, int32_t offset);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_isBoundary") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_isBoundary", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_isBoundary");
    abort();
  }

  return ptr(bi, offset);
}
int32_t ubrk_getRuleStatus(UBreakIterator * bi) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_getRuleStatus") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_getRuleStatus", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_getRuleStatus");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_getRuleStatusVec(UBreakIterator * bi, int32_t * fillInVec, int32_t capacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UBreakIterator * bi, int32_t * fillInVec, int32_t capacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_getRuleStatusVec") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_getRuleStatusVec", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_getRuleStatusVec");
    abort();
  }

  return ptr(bi, fillInVec, capacity, status);
}
const char * ubrk_getLocaleByType(const UBreakIterator * bi, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UBreakIterator * bi, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_getLocaleByType");
    abort();
  }

  return ptr(bi, type, status);
}
void ubrk_refreshUText(UBreakIterator * bi, UText * text, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBreakIterator * bi, UText * text, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubrk_refreshUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubrk_refreshUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubrk_refreshUText");
    abort();
  }

  ptr(bi, text, status);
}
void utrace_setLevel(int32_t traceLevel) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(int32_t traceLevel);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_setLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_setLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_setLevel");
    abort();
  }

  ptr(traceLevel);
}
int32_t utrace_getLevel() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_getLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_getLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_getLevel");
    abort();
  }

  return ptr();
}
void utrace_setFunctions(const void * context, UTraceEntry * e, UTraceExit * x, UTraceData * d) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void * context, UTraceEntry * e, UTraceExit * x, UTraceData * d);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_setFunctions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_setFunctions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_setFunctions");
    abort();
  }

  ptr(context, e, x, d);
}
void utrace_getFunctions(const void ** context, UTraceEntry ** e, UTraceExit ** x, UTraceData ** d) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const void ** context, UTraceEntry ** e, UTraceExit ** x, UTraceData ** d);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_getFunctions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_getFunctions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_getFunctions");
    abort();
  }

  ptr(context, e, x, d);
}
int32_t utrace_vformat(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, int args) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, int args);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_vformat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_vformat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_vformat");
    abort();
  }

  return ptr(outBuf, capacity, indent, fmt, args);
}
int32_t utrace_format(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_vformat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_vformat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_format");
    abort();
  }

  return ptr(outBuf, capacity, indent, fmt);
}
const char * utrace_functionName(int32_t fnNumber) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t fnNumber);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrace_functionName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrace_functionName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrace_functionName");
    abort();
  }

  return ptr(fnNumber);
}
UText * utext_close(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_close");
    abort();
  }

  return ptr(ut);
}
UText * utext_openUTF8(UText * ut, const char * s, int64_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(UText * ut, const char * s, int64_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_openUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_openUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_openUTF8");
    abort();
  }

  return ptr(ut, s, length, status);
}
UText * utext_openUChars(UText * ut, const UChar * s, int64_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(UText * ut, const UChar * s, int64_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_openUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_openUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_openUChars");
    abort();
  }

  return ptr(ut, s, length, status);
}
UText * utext_clone(UText * dest, const UText * src, UBool deep, UBool readOnly, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(UText * dest, const UText * src, UBool deep, UBool readOnly, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_clone");
    abort();
  }

  return ptr(dest, src, deep, readOnly, status);
}
UBool utext_equals(const UText * a, const UText * b) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UText * a, const UText * b);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_equals") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_equals", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_equals");
    abort();
  }

  return ptr(a, b);
}
int64_t utext_nativeLength(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_nativeLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_nativeLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_nativeLength");
    abort();
  }

  return ptr(ut);
}
UBool utext_isLengthExpensive(const UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_isLengthExpensive") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_isLengthExpensive", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_isLengthExpensive");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_char32At(UText * ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut, int64_t nativeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_char32At") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_char32At", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_char32At");
    abort();
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_current32(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_current32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_current32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_current32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_next32(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_next32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_next32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_next32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_previous32(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_previous32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_previous32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_previous32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_next32From(UText * ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut, int64_t nativeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_next32From") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_next32From", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_next32From");
    abort();
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_previous32From(UText * ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UText * ut, int64_t nativeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_previous32From") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_previous32From", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_previous32From");
    abort();
  }

  return ptr(ut, nativeIndex);
}
int64_t utext_getNativeIndex(const UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(const UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_getNativeIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_getNativeIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_getNativeIndex");
    abort();
  }

  return ptr(ut);
}
void utext_setNativeIndex(UText * ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UText * ut, int64_t nativeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_setNativeIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_setNativeIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_setNativeIndex");
    abort();
  }

  ptr(ut, nativeIndex);
}
UBool utext_moveIndex32(UText * ut, int32_t delta) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UText * ut, int32_t delta);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_moveIndex32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_moveIndex32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_moveIndex32");
    abort();
  }

  return ptr(ut, delta);
}
int64_t utext_getPreviousNativeIndex(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_getPreviousNativeIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_getPreviousNativeIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_getPreviousNativeIndex");
    abort();
  }

  return ptr(ut);
}
int32_t utext_extract(UText * ut, int64_t nativeStart, int64_t nativeLimit, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UText * ut, int64_t nativeStart, int64_t nativeLimit, UChar * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_extract") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_extract", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_extract");
    abort();
  }

  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}
UBool utext_isWritable(const UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_isWritable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_isWritable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_isWritable");
    abort();
  }

  return ptr(ut);
}
UBool utext_hasMetaData(const UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_hasMetaData") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_hasMetaData", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_hasMetaData");
    abort();
  }

  return ptr(ut);
}
int32_t utext_replace(UText * ut, int64_t nativeStart, int64_t nativeLimit, const UChar * replacementText, int32_t replacementLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UText * ut, int64_t nativeStart, int64_t nativeLimit, const UChar * replacementText, int32_t replacementLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_replace") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_replace", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_replace");
    abort();
  }

  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}
void utext_copy(UText * ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UText * ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_copy") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_copy", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_copy");
    abort();
  }

  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
}
void utext_freeze(UText * ut) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UText * ut);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_freeze") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_freeze", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_freeze");
    abort();
  }

  ptr(ut);
}
UText * utext_setup(UText * ut, int32_t extraSpace, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(UText * ut, int32_t extraSpace, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utext_setup") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utext_setup", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utext_setup");
    abort();
  }

  return ptr(ut, extraSpace, status);
}
void uenum_close(UEnumeration * en) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UEnumeration * en);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_close");
    abort();
  }

  ptr(en);
}
int32_t uenum_count(UEnumeration * en, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UEnumeration * en, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_count") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_count", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_count");
    abort();
  }

  return ptr(en, status);
}
const UChar * uenum_unext(UEnumeration * en, int32_t * resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(UEnumeration * en, int32_t * resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_unext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_unext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_unext");
    abort();
  }

  return ptr(en, resultLength, status);
}
const char * uenum_next(UEnumeration * en, int32_t * resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UEnumeration * en, int32_t * resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_next") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_next", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_next");
    abort();
  }

  return ptr(en, resultLength, status);
}
void uenum_reset(UEnumeration * en, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UEnumeration * en, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_reset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_reset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_reset");
    abort();
  }

  ptr(en, status);
}
UEnumeration * uenum_openUCharStringsEnumeration(const UChar *const  strings[], int32_t count, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UChar *const  strings[], int32_t count, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_openUCharStringsEnumeration") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_openUCharStringsEnumeration", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_openUCharStringsEnumeration");
    abort();
  }

  return ptr(strings, count, ec);
}
UEnumeration * uenum_openCharStringsEnumeration(const char *const  strings[], int32_t count, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char *const  strings[], int32_t count, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uenum_openCharStringsEnumeration") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uenum_openCharStringsEnumeration", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uenum_openCharStringsEnumeration");
    abort();
  }

  return ptr(strings, count, ec);
}
void u_versionFromString(UVersionInfo versionArray, const char * versionString) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo versionArray, const char * versionString);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_versionFromString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_versionFromString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_versionFromString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_versionFromUString(UVersionInfo versionArray, const UChar * versionString) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo versionArray, const UChar * versionString);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_versionFromUString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_versionFromUString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_versionFromUString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_versionToString(const UVersionInfo versionArray, char * versionString) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UVersionInfo versionArray, char * versionString);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_versionToString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_versionToString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_versionToString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_getVersion(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo versionArray);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getVersion");
    abort();
  }

  ptr(versionArray);
}
UStringPrepProfile * usprep_open(const char * path, const char * fileName, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UStringPrepProfile * (*FuncPtr)(const char * path, const char * fileName, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usprep_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usprep_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usprep_open");
    abort();
  }

  return ptr(path, fileName, status);
}
UStringPrepProfile * usprep_openByType(UStringPrepProfileType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UStringPrepProfile * (*FuncPtr)(UStringPrepProfileType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usprep_openByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usprep_openByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usprep_openByType");
    abort();
  }

  return ptr(type, status);
}
void usprep_close(UStringPrepProfile * profile) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringPrepProfile * profile);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usprep_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usprep_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usprep_close");
    abort();
  }

  ptr(profile);
}
int32_t usprep_prepare(const UStringPrepProfile * prep, const UChar * src, int32_t srcLength, UChar * dest, int32_t destCapacity, int32_t options, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UStringPrepProfile * prep, const UChar * src, int32_t srcLength, UChar * dest, int32_t destCapacity, int32_t options, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usprep_prepare") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usprep_prepare", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usprep_prepare");
    abort();
  }

  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}
int32_t uscript_getCode(const char * nameOrAbbrOrLocale, UScriptCode * fillIn, int32_t capacity, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * nameOrAbbrOrLocale, UScriptCode * fillIn, int32_t capacity, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getCode");
    abort();
  }

  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}
const char * uscript_getName(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UScriptCode scriptCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getName");
    abort();
  }

  return ptr(scriptCode);
}
const char * uscript_getShortName(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UScriptCode scriptCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getShortName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getShortName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getShortName");
    abort();
  }

  return ptr(scriptCode);
}
UScriptCode uscript_getScript(UChar32 codepoint, UErrorCode * err) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UScriptCode (*FuncPtr)(UChar32 codepoint, UErrorCode * err);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getScript") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getScript", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getScript");
    abort();
  }

  return ptr(codepoint, err);
}
UBool uscript_hasScript(UChar32 c, UScriptCode sc) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UChar32 c, UScriptCode sc);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_hasScript") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_hasScript", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_hasScript");
    abort();
  }

  return ptr(c, sc);
}
int32_t uscript_getScriptExtensions(UChar32 c, UScriptCode * scripts, int32_t capacity, UErrorCode * errorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar32 c, UScriptCode * scripts, int32_t capacity, UErrorCode * errorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getScriptExtensions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getScriptExtensions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getScriptExtensions");
    abort();
  }

  return ptr(c, scripts, capacity, errorCode);
}
int32_t uscript_getSampleString(UScriptCode script, UChar * dest, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UScriptCode script, UChar * dest, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getSampleString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getSampleString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getSampleString");
    abort();
  }

  return ptr(script, dest, capacity, pErrorCode);
}
UScriptUsage uscript_getUsage(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UScriptUsage (*FuncPtr)(UScriptCode script);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_getUsage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_getUsage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_getUsage");
    abort();
  }

  return ptr(script);
}
UBool uscript_isRightToLeft(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UScriptCode script);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_isRightToLeft") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_isRightToLeft", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_isRightToLeft");
    abort();
  }

  return ptr(script);
}
UBool uscript_breaksBetweenLetters(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UScriptCode script);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_breaksBetweenLetters") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_breaksBetweenLetters", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_breaksBetweenLetters");
    abort();
  }

  return ptr(script);
}
UBool uscript_isCased(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UScriptCode script);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uscript_isCased") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uscript_isCased", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uscript_isCased");
    abort();
  }

  return ptr(script);
}
const char * u_getDataDirectory() {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_getDataDirectory") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_getDataDirectory", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_getDataDirectory");
    abort();
  }

  return ptr();
}
void u_setDataDirectory(const char * directory) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * directory);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_setDataDirectory") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_setDataDirectory", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_setDataDirectory");
    abort();
  }

  ptr(directory);
}
void u_charsToUChars(const char * cs, UChar * us, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * cs, UChar * us, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_charsToUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_charsToUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_charsToUChars");
    abort();
  }

  ptr(cs, us, length);
}
void u_UCharsToChars(const UChar * us, char * cs, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UChar * us, char * cs, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_UCharsToChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_UCharsToChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_UCharsToChars");
    abort();
  }

  ptr(us, cs, length);
}
UCaseMap * ucasemap_open(const char * locale, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCaseMap * (*FuncPtr)(const char * locale, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_open");
    abort();
  }

  return ptr(locale, options, pErrorCode);
}
void ucasemap_close(UCaseMap * csm) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCaseMap * csm);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_close");
    abort();
  }

  ptr(csm);
}
const char * ucasemap_getLocale(const UCaseMap * csm) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCaseMap * csm);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_getLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_getLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_getLocale");
    abort();
  }

  return ptr(csm);
}
uint32_t ucasemap_getOptions(const UCaseMap * csm) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(const UCaseMap * csm);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_getOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_getOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_getOptions");
    abort();
  }

  return ptr(csm);
}
void ucasemap_setLocale(UCaseMap * csm, const char * locale, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCaseMap * csm, const char * locale, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_setLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_setLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_setLocale");
    abort();
  }

  ptr(csm, locale, pErrorCode);
}
void ucasemap_setOptions(UCaseMap * csm, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCaseMap * csm, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_setOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_setOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_setOptions");
    abort();
  }

  ptr(csm, options, pErrorCode);
}
const UBreakIterator * ucasemap_getBreakIterator(const UCaseMap * csm) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UBreakIterator * (*FuncPtr)(const UCaseMap * csm);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_getBreakIterator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_getBreakIterator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_getBreakIterator");
    abort();
  }

  return ptr(csm);
}
void ucasemap_setBreakIterator(UCaseMap * csm, UBreakIterator * iterToAdopt, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCaseMap * csm, UBreakIterator * iterToAdopt, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_setBreakIterator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_setBreakIterator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_setBreakIterator");
    abort();
  }

  ptr(csm, iterToAdopt, pErrorCode);
}
int32_t ucasemap_toTitle(UCaseMap * csm, UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCaseMap * csm, UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_toTitle") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_toTitle", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_toTitle");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToLower(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_utf8ToLower") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_utf8ToLower", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_utf8ToLower");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToUpper(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_utf8ToUpper") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_utf8ToUpper", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_utf8ToUpper");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToTitle(UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_utf8ToTitle") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_utf8ToTitle", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_utf8ToTitle");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8FoldCase(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucasemap_utf8FoldCase") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucasemap_utf8FoldCase", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucasemap_utf8FoldCase");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
const UNormalizer2 * unorm2_getNFCInstance(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getNFCInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getNFCInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getNFCInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFDInstance(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getNFDInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getNFDInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getNFDInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKCInstance(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getNFKCInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getNFKCInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getNFKCInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKDInstance(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getNFKDInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getNFKDInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getNFKDInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKCCasefoldInstance(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getNFKCCasefoldInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getNFKCCasefoldInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getNFKCCasefoldInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getInstance(const char * packageName, const char * name, UNormalization2Mode mode, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNormalizer2 * (*FuncPtr)(const char * packageName, const char * name, UNormalization2Mode mode, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getInstance");
    abort();
  }

  return ptr(packageName, name, mode, pErrorCode);
}
UNormalizer2 * unorm2_openFiltered(const UNormalizer2 * norm2, const USet * filterSet, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNormalizer2 * (*FuncPtr)(const UNormalizer2 * norm2, const USet * filterSet, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_openFiltered") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_openFiltered", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_openFiltered");
    abort();
  }

  return ptr(norm2, filterSet, pErrorCode);
}
void unorm2_close(UNormalizer2 * norm2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNormalizer2 * norm2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_close");
    abort();
  }

  ptr(norm2);
}
int32_t unorm2_normalize(const UNormalizer2 * norm2, const UChar * src, int32_t length, UChar * dest, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, const UChar * src, int32_t length, UChar * dest, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_normalize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_normalize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_normalize");
    abort();
  }

  return ptr(norm2, src, length, dest, capacity, pErrorCode);
}
int32_t unorm2_normalizeSecondAndAppend(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_normalizeSecondAndAppend") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_normalizeSecondAndAppend", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_normalizeSecondAndAppend");
    abort();
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_append(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_append") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_append", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_append");
    abort();
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_getDecomposition(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getDecomposition") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getDecomposition", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getDecomposition");
    abort();
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
int32_t unorm2_getRawDecomposition(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getRawDecomposition") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getRawDecomposition", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getRawDecomposition");
    abort();
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
UChar32 unorm2_composePair(const UNormalizer2 * norm2, UChar32 a, UChar32 b) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(const UNormalizer2 * norm2, UChar32 a, UChar32 b);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_composePair") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_composePair", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_composePair");
    abort();
  }

  return ptr(norm2, a, b);
}
uint8_t unorm2_getCombiningClass(const UNormalizer2 * norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint8_t (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_getCombiningClass") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_getCombiningClass", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_getCombiningClass");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_isNormalized(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_isNormalized") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_isNormalized", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_isNormalized");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
UNormalizationCheckResult unorm2_quickCheck(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNormalizationCheckResult (*FuncPtr)(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_quickCheck") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_quickCheck", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_quickCheck");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
int32_t unorm2_spanQuickCheckYes(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_spanQuickCheckYes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_spanQuickCheckYes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_spanQuickCheckYes");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
UBool unorm2_hasBoundaryBefore(const UNormalizer2 * norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_hasBoundaryBefore") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_hasBoundaryBefore", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_hasBoundaryBefore");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_hasBoundaryAfter(const UNormalizer2 * norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_hasBoundaryAfter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_hasBoundaryAfter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_hasBoundaryAfter");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_isInert(const UNormalizer2 * norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UNormalizer2 * norm2, UChar32 c);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm2_isInert") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm2_isInert", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm2_isInert");
    abort();
  }

  return ptr(norm2, c);
}
int32_t unorm_compare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unorm_compare") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unorm_compare", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unorm_compare");
    abort();
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
UChar32 uiter_current32(UCharIterator * iter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UCharIterator * iter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_current32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_current32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_current32");
    abort();
  }

  return ptr(iter);
}
UChar32 uiter_next32(UCharIterator * iter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UCharIterator * iter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_next32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_next32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_next32");
    abort();
  }

  return ptr(iter);
}
UChar32 uiter_previous32(UCharIterator * iter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UChar32 (*FuncPtr)(UCharIterator * iter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_previous32") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_previous32", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_previous32");
    abort();
  }

  return ptr(iter);
}
uint32_t uiter_getState(const UCharIterator * iter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(const UCharIterator * iter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_getState") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_getState", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_getState");
    abort();
  }

  return ptr(iter);
}
void uiter_setState(UCharIterator * iter, uint32_t state, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharIterator * iter, uint32_t state, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_setState") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_setState", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_setState");
    abort();
  }

  ptr(iter, state, pErrorCode);
}
void uiter_setString(UCharIterator * iter, const UChar * s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharIterator * iter, const UChar * s, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_setString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_setString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_setString");
    abort();
  }

  ptr(iter, s, length);
}
void uiter_setUTF16BE(UCharIterator * iter, const char * s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharIterator * iter, const char * s, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_setUTF16BE") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_setUTF16BE", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_setUTF16BE");
    abort();
  }

  ptr(iter, s, length);
}
void uiter_setUTF8(UCharIterator * iter, const char * s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharIterator * iter, const char * s, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uiter_setUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uiter_setUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uiter_setUTF8");
    abort();
  }

  ptr(iter, s, length);
}
UConverterSelector * ucnvsel_open(const char *const * converterList, int32_t converterListSize, const USet * excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverterSelector * (*FuncPtr)(const char *const * converterList, int32_t converterListSize, const USet * excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_open");
    abort();
  }

  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}
void ucnvsel_close(UConverterSelector * sel) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UConverterSelector * sel);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_close");
    abort();
  }

  ptr(sel);
}
UConverterSelector * ucnvsel_openFromSerialized(const void * buffer, int32_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UConverterSelector * (*FuncPtr)(const void * buffer, int32_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_openFromSerialized") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_openFromSerialized", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_openFromSerialized");
    abort();
  }

  return ptr(buffer, length, status);
}
int32_t ucnvsel_serialize(const UConverterSelector * sel, void * buffer, int32_t bufferCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UConverterSelector * sel, void * buffer, int32_t bufferCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_serialize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_serialize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_serialize");
    abort();
  }

  return ptr(sel, buffer, bufferCapacity, status);
}
UEnumeration * ucnvsel_selectForString(const UConverterSelector * sel, const UChar * s, int32_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UConverterSelector * sel, const UChar * s, int32_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_selectForString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_selectForString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_selectForString");
    abort();
  }

  return ptr(sel, s, length, status);
}
UEnumeration * ucnvsel_selectForUTF8(const UConverterSelector * sel, const char * s, int32_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UConverterSelector * sel, const char * s, int32_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucnvsel_selectForUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucnvsel_selectForUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucnvsel_selectForUTF8");
    abort();
  }

  return ptr(sel, s, length, status);
}
uint32_t ubiditransform_transform(UBiDiTransform * pBiDiTransform, const UChar * src, int32_t srcLength, UChar * dest, int32_t destSize, UBiDiLevel inParaLevel, UBiDiOrder inOrder, UBiDiLevel outParaLevel, UBiDiOrder outOrder, UBiDiMirroring doMirroring, uint32_t shapingOptions, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(UBiDiTransform * pBiDiTransform, const UChar * src, int32_t srcLength, UChar * dest, int32_t destSize, UBiDiLevel inParaLevel, UBiDiOrder inOrder, UBiDiLevel outParaLevel, UBiDiOrder outOrder, UBiDiMirroring doMirroring, uint32_t shapingOptions, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubiditransform_transform") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubiditransform_transform", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubiditransform_transform");
    abort();
  }

  return ptr(pBiDiTransform, src, srcLength, dest, destSize, inParaLevel, inOrder, outParaLevel, outOrder, doMirroring, shapingOptions, pErrorCode);
}
UBiDiTransform * ubiditransform_open(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBiDiTransform * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubiditransform_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubiditransform_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubiditransform_open");
    abort();
  }

  return ptr(pErrorCode);
}
void ubiditransform_close(UBiDiTransform * pBidiTransform) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UBiDiTransform * pBidiTransform);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ubiditransform_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ubiditransform_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ubiditransform_close");
    abort();
  }

  ptr(pBidiTransform);
}
UListFormatter * ulistfmt_open(const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UListFormatter * (*FuncPtr)(const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulistfmt_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulistfmt_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulistfmt_open");
    abort();
  }

  return ptr(locale, status);
}
void ulistfmt_close(UListFormatter * listfmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UListFormatter * listfmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulistfmt_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulistfmt_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulistfmt_close");
    abort();
  }

  ptr(listfmt);
}
int32_t ulistfmt_format(const UListFormatter * listfmt, const UChar *const  strings[], const int32_t * stringLengths, int32_t stringCount, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UListFormatter * listfmt, const UChar *const  strings[], const int32_t * stringLengths, int32_t stringCount, UChar * result, int32_t resultCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulistfmt_format") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulistfmt_format", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulistfmt_format");
    abort();
  }

  return ptr(listfmt, strings, stringLengths, stringCount, result, resultCapacity, status);
}
UResourceBundle * ures_open(const char * packageName, const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(const char * packageName, const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_open");
    abort();
  }

  return ptr(packageName, locale, status);
}
UResourceBundle * ures_openDirect(const char * packageName, const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(const char * packageName, const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_openDirect") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_openDirect", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_openDirect");
    abort();
  }

  return ptr(packageName, locale, status);
}
UResourceBundle * ures_openU(const UChar * packageName, const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(const UChar * packageName, const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_openU") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_openU", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_openU");
    abort();
  }

  return ptr(packageName, locale, status);
}
void ures_close(UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_close");
    abort();
  }

  ptr(resourceBundle);
}
void ures_getVersion(const UResourceBundle * resB, UVersionInfo versionInfo) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UResourceBundle * resB, UVersionInfo versionInfo);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getVersion");
    abort();
  }

  ptr(resB, versionInfo);
}
const char * ures_getLocaleByType(const UResourceBundle * resourceBundle, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UResourceBundle * resourceBundle, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getLocaleByType");
    abort();
  }

  return ptr(resourceBundle, type, status);
}
const UChar * ures_getString(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getString");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
const char * ures_getUTF8String(const UResourceBundle * resB, char * dest, int32_t * length, UBool forceCopy, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UResourceBundle * resB, char * dest, int32_t * length, UBool forceCopy, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getUTF8String") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getUTF8String", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getUTF8String");
    abort();
  }

  return ptr(resB, dest, length, forceCopy, status);
}
const uint8_t * ures_getBinary(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const uint8_t * (*FuncPtr)(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getBinary") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getBinary", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getBinary");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
const int32_t * ures_getIntVector(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const int32_t * (*FuncPtr)(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getIntVector") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getIntVector", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getIntVector");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
uint32_t ures_getUInt(const UResourceBundle * resourceBundle, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(const UResourceBundle * resourceBundle, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getUInt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getUInt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getUInt");
    abort();
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getInt(const UResourceBundle * resourceBundle, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UResourceBundle * resourceBundle, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getInt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getInt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getInt");
    abort();
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getSize(const UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getSize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getSize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getSize");
    abort();
  }

  return ptr(resourceBundle);
}
UResType ures_getType(const UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResType (*FuncPtr)(const UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getType");
    abort();
  }

  return ptr(resourceBundle);
}
const char * ures_getKey(const UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getKey");
    abort();
  }

  return ptr(resourceBundle);
}
void ures_resetIterator(UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_resetIterator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_resetIterator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_resetIterator");
    abort();
  }

  ptr(resourceBundle);
}
UBool ures_hasNext(const UResourceBundle * resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UResourceBundle * resourceBundle);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_hasNext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_hasNext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_hasNext");
    abort();
  }

  return ptr(resourceBundle);
}
UResourceBundle * ures_getNextResource(UResourceBundle * resourceBundle, UResourceBundle * fillIn, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(UResourceBundle * resourceBundle, UResourceBundle * fillIn, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getNextResource") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getNextResource", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getNextResource");
    abort();
  }

  return ptr(resourceBundle, fillIn, status);
}
const UChar * ures_getNextString(UResourceBundle * resourceBundle, int32_t * len, const char ** key, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(UResourceBundle * resourceBundle, int32_t * len, const char ** key, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getNextString") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getNextString", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getNextString");
    abort();
  }

  return ptr(resourceBundle, len, key, status);
}
UResourceBundle * ures_getByIndex(const UResourceBundle * resourceBundle, int32_t indexR, UResourceBundle * fillIn, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(const UResourceBundle * resourceBundle, int32_t indexR, UResourceBundle * fillIn, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getByIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getByIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getByIndex");
    abort();
  }

  return ptr(resourceBundle, indexR, fillIn, status);
}
const UChar * ures_getStringByIndex(const UResourceBundle * resourceBundle, int32_t indexS, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UResourceBundle * resourceBundle, int32_t indexS, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getStringByIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getStringByIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getStringByIndex");
    abort();
  }

  return ptr(resourceBundle, indexS, len, status);
}
const char * ures_getUTF8StringByIndex(const UResourceBundle * resB, int32_t stringIndex, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UResourceBundle * resB, int32_t stringIndex, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getUTF8StringByIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getUTF8StringByIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getUTF8StringByIndex");
    abort();
  }

  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}
UResourceBundle * ures_getByKey(const UResourceBundle * resourceBundle, const char * key, UResourceBundle * fillIn, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UResourceBundle * (*FuncPtr)(const UResourceBundle * resourceBundle, const char * key, UResourceBundle * fillIn, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getByKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getByKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getByKey");
    abort();
  }

  return ptr(resourceBundle, key, fillIn, status);
}
const UChar * ures_getStringByKey(const UResourceBundle * resB, const char * key, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UResourceBundle * resB, const char * key, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getStringByKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getStringByKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getStringByKey");
    abort();
  }

  return ptr(resB, key, len, status);
}
const char * ures_getUTF8StringByKey(const UResourceBundle * resB, const char * key, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UResourceBundle * resB, const char * key, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_getUTF8StringByKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_getUTF8StringByKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_getUTF8StringByKey");
    abort();
  }

  return ptr(resB, key, dest, pLength, forceCopy, status);
}
UEnumeration * ures_openAvailableLocales(const char * packageName, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * packageName, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ures_openAvailableLocales") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ures_openAvailableLocales", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_common, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ures_openAvailableLocales");
    abort();
  }

  return ptr(packageName, status);
}
UCollationElements * ucol_openElements(const UCollator * coll, const UChar * text, int32_t textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollationElements * (*FuncPtr)(const UCollator * coll, const UChar * text, int32_t textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_openElements") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_openElements", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_openElements");
    abort();
  }

  return ptr(coll, text, textLength, status);
}
int32_t ucol_keyHashCode(const uint8_t * key, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const uint8_t * key, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_keyHashCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_keyHashCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_keyHashCode");
    abort();
  }

  return ptr(key, length);
}
void ucol_closeElements(UCollationElements * elems) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollationElements * elems);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_closeElements") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_closeElements", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_closeElements");
    abort();
  }

  ptr(elems);
}
void ucol_reset(UCollationElements * elems) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollationElements * elems);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_reset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_reset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_reset");
    abort();
  }

  ptr(elems);
}
int32_t ucol_next(UCollationElements * elems, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCollationElements * elems, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_next") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_next", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_next");
    abort();
  }

  return ptr(elems, status);
}
int32_t ucol_previous(UCollationElements * elems, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCollationElements * elems, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_previous") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_previous", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_previous");
    abort();
  }

  return ptr(elems, status);
}
int32_t ucol_getMaxExpansion(const UCollationElements * elems, int32_t order) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollationElements * elems, int32_t order);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getMaxExpansion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getMaxExpansion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getMaxExpansion");
    abort();
  }

  return ptr(elems, order);
}
void ucol_setText(UCollationElements * elems, const UChar * text, int32_t textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollationElements * elems, const UChar * text, int32_t textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setText");
    abort();
  }

  ptr(elems, text, textLength, status);
}
int32_t ucol_getOffset(const UCollationElements * elems) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollationElements * elems);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getOffset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getOffset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getOffset");
    abort();
  }

  return ptr(elems);
}
void ucol_setOffset(UCollationElements * elems, int32_t offset, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollationElements * elems, int32_t offset, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setOffset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setOffset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setOffset");
    abort();
  }

  ptr(elems, offset, status);
}
int32_t ucol_primaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(int32_t order);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_primaryOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_primaryOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_primaryOrder");
    abort();
  }

  return ptr(order);
}
int32_t ucol_secondaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(int32_t order);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_secondaryOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_secondaryOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_secondaryOrder");
    abort();
  }

  return ptr(order);
}
int32_t ucol_tertiaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(int32_t order);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_tertiaryOrder") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_tertiaryOrder", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_tertiaryOrder");
    abort();
  }

  return ptr(order);
}
UCharsetDetector * ucsdet_open(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCharsetDetector * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_open");
    abort();
  }

  return ptr(status);
}
void ucsdet_close(UCharsetDetector * ucsd) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharsetDetector * ucsd);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_close");
    abort();
  }

  ptr(ucsd);
}
void ucsdet_setText(UCharsetDetector * ucsd, const char * textIn, int32_t len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharsetDetector * ucsd, const char * textIn, int32_t len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_setText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_setText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_setText");
    abort();
  }

  ptr(ucsd, textIn, len, status);
}
void ucsdet_setDeclaredEncoding(UCharsetDetector * ucsd, const char * encoding, int32_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCharsetDetector * ucsd, const char * encoding, int32_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_setDeclaredEncoding") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_setDeclaredEncoding", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_setDeclaredEncoding");
    abort();
  }

  ptr(ucsd, encoding, length, status);
}
const UCharsetMatch * ucsdet_detect(UCharsetDetector * ucsd, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UCharsetMatch * (*FuncPtr)(UCharsetDetector * ucsd, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_detect") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_detect", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_detect");
    abort();
  }

  return ptr(ucsd, status);
}
const UCharsetMatch ** ucsdet_detectAll(UCharsetDetector * ucsd, int32_t * matchesFound, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UCharsetMatch ** (*FuncPtr)(UCharsetDetector * ucsd, int32_t * matchesFound, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_detectAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_detectAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_detectAll");
    abort();
  }

  return ptr(ucsd, matchesFound, status);
}
const char * ucsdet_getName(const UCharsetMatch * ucsm, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCharsetMatch * ucsm, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_getName");
    abort();
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getConfidence(const UCharsetMatch * ucsm, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCharsetMatch * ucsm, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_getConfidence") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_getConfidence", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_getConfidence");
    abort();
  }

  return ptr(ucsm, status);
}
const char * ucsdet_getLanguage(const UCharsetMatch * ucsm, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCharsetMatch * ucsm, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_getLanguage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_getLanguage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_getLanguage");
    abort();
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getUChars(const UCharsetMatch * ucsm, UChar * buf, int32_t cap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCharsetMatch * ucsm, UChar * buf, int32_t cap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_getUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_getUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_getUChars");
    abort();
  }

  return ptr(ucsm, buf, cap, status);
}
UEnumeration * ucsdet_getAllDetectableCharsets(const UCharsetDetector * ucsd, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UCharsetDetector * ucsd, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_getAllDetectableCharsets") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_getAllDetectableCharsets", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_getAllDetectableCharsets");
    abort();
  }

  return ptr(ucsd, status);
}
UBool ucsdet_isInputFilterEnabled(const UCharsetDetector * ucsd) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCharsetDetector * ucsd);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_isInputFilterEnabled") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_isInputFilterEnabled", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_isInputFilterEnabled");
    abort();
  }

  return ptr(ucsd);
}
UBool ucsdet_enableInputFilter(UCharsetDetector * ucsd, UBool filter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(UCharsetDetector * ucsd, UBool filter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucsdet_enableInputFilter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucsdet_enableInputFilter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucsdet_enableInputFilter");
    abort();
  }

  return ptr(ucsd, filter);
}
int64_t utmscale_getTimeScaleValue(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utmscale_getTimeScaleValue") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utmscale_getTimeScaleValue", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utmscale_getTimeScaleValue");
    abort();
  }

  return ptr(timeScale, value, status);
}
int64_t utmscale_fromInt64(int64_t otherTime, UDateTimeScale timeScale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(int64_t otherTime, UDateTimeScale timeScale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utmscale_fromInt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utmscale_fromInt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utmscale_fromInt64");
    abort();
  }

  return ptr(otherTime, timeScale, status);
}
int64_t utmscale_toInt64(int64_t universalTime, UDateTimeScale timeScale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(int64_t universalTime, UDateTimeScale timeScale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utmscale_toInt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utmscale_toInt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utmscale_toInt64");
    abort();
  }

  return ptr(universalTime, timeScale, status);
}
UDateTimePatternGenerator * udatpg_open(const char * locale, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateTimePatternGenerator * (*FuncPtr)(const char * locale, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_open");
    abort();
  }

  return ptr(locale, pErrorCode);
}
UDateTimePatternGenerator * udatpg_openEmpty(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateTimePatternGenerator * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_openEmpty") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_openEmpty", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_openEmpty");
    abort();
  }

  return ptr(pErrorCode);
}
void udatpg_close(UDateTimePatternGenerator * dtpg) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateTimePatternGenerator * dtpg);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_close");
    abort();
  }

  ptr(dtpg);
}
UDateTimePatternGenerator * udatpg_clone(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateTimePatternGenerator * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_clone");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
int32_t udatpg_getBestPattern(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getBestPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getBestPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getBestPattern");
    abort();
  }

  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getBestPatternWithOptions(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UDateTimePatternMatchOptions options, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UDateTimePatternMatchOptions options, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getBestPatternWithOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getBestPatternWithOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getBestPatternWithOptions");
    abort();
  }

  return ptr(dtpg, skeleton, length, options, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getSkeleton(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * skeleton, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * skeleton, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getSkeleton") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getSkeleton", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getSkeleton");
    abort();
  }

  return ptr(unusedDtpg, pattern, length, skeleton, capacity, pErrorCode);
}
int32_t udatpg_getBaseSkeleton(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * baseSkeleton, int32_t capacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * baseSkeleton, int32_t capacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getBaseSkeleton") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getBaseSkeleton", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getBaseSkeleton");
    abort();
  }

  return ptr(unusedDtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}
UDateTimePatternConflict udatpg_addPattern(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, UBool override, UChar * conflictingPattern, int32_t capacity, int32_t * pLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateTimePatternConflict (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, UBool override, UChar * conflictingPattern, int32_t capacity, int32_t * pLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_addPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_addPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_addPattern");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}
void udatpg_setAppendItemFormat(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_setAppendItemFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_setAppendItemFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_setAppendItemFormat");
    abort();
  }

  ptr(dtpg, field, value, length);
}
const UChar * udatpg_getAppendItemFormat(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getAppendItemFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getAppendItemFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getAppendItemFormat");
    abort();
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setAppendItemName(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_setAppendItemName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_setAppendItemName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_setAppendItemName");
    abort();
  }

  ptr(dtpg, field, value, length);
}
const UChar * udatpg_getAppendItemName(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getAppendItemName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getAppendItemName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getAppendItemName");
    abort();
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setDateTimeFormat(const UDateTimePatternGenerator * dtpg, const UChar * dtFormat, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UDateTimePatternGenerator * dtpg, const UChar * dtFormat, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_setDateTimeFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_setDateTimeFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_setDateTimeFormat");
    abort();
  }

  ptr(dtpg, dtFormat, length);
}
const UChar * udatpg_getDateTimeFormat(const UDateTimePatternGenerator * dtpg, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getDateTimeFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getDateTimeFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getDateTimeFormat");
    abort();
  }

  return ptr(dtpg, pLength);
}
void udatpg_setDecimal(UDateTimePatternGenerator * dtpg, const UChar * decimal, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * decimal, int32_t length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_setDecimal") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_setDecimal", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_setDecimal");
    abort();
  }

  ptr(dtpg, decimal, length);
}
const UChar * udatpg_getDecimal(const UDateTimePatternGenerator * dtpg, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getDecimal") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getDecimal", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getDecimal");
    abort();
  }

  return ptr(dtpg, pLength);
}
int32_t udatpg_replaceFieldTypes(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_replaceFieldTypes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_replaceFieldTypes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_replaceFieldTypes");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}
int32_t udatpg_replaceFieldTypesWithOptions(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UDateTimePatternMatchOptions options, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UDateTimePatternMatchOptions options, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_replaceFieldTypesWithOptions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_replaceFieldTypesWithOptions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_replaceFieldTypesWithOptions");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, options, dest, destCapacity, pErrorCode);
}
UEnumeration * udatpg_openSkeletons(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_openSkeletons") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_openSkeletons", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_openSkeletons");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
UEnumeration * udatpg_openBaseSkeletons(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_openBaseSkeletons") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_openBaseSkeletons", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_openBaseSkeletons");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
const UChar * udatpg_getPatternForSkeleton(const UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t skeletonLength, int32_t * pLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t skeletonLength, int32_t * pLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udatpg_getPatternForSkeleton") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udatpg_getPatternForSkeleton", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udatpg_getPatternForSkeleton");
    abort();
  }

  return ptr(dtpg, skeleton, skeletonLength, pLength);
}
UCalendarDateFields udat_toCalendarDateField(UDateFormatField field) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCalendarDateFields (*FuncPtr)(UDateFormatField field);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_toCalendarDateField") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_toCalendarDateField", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_toCalendarDateField");
    abort();
  }

  return ptr(field);
}
UDateFormat * udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char * locale, const UChar * tzID, int32_t tzIDLength, const UChar * pattern, int32_t patternLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateFormat * (*FuncPtr)(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char * locale, const UChar * tzID, int32_t tzIDLength, const UChar * pattern, int32_t patternLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_open");
    abort();
  }

  return ptr(timeStyle, dateStyle, locale, tzID, tzIDLength, pattern, patternLength, status);
}
void udat_close(UDateFormat * format) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * format);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_close");
    abort();
  }

  ptr(format);
}
UBool udat_getBooleanAttribute(const UDateFormat * fmt, UDateFormatBooleanAttribute attr, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UDateFormat * fmt, UDateFormatBooleanAttribute attr, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getBooleanAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getBooleanAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getBooleanAttribute");
    abort();
  }

  return ptr(fmt, attr, status);
}
void udat_setBooleanAttribute(UDateFormat * fmt, UDateFormatBooleanAttribute attr, UBool newValue, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, UDateFormatBooleanAttribute attr, UBool newValue, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setBooleanAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setBooleanAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setBooleanAttribute");
    abort();
  }

  ptr(fmt, attr, newValue, status);
}
UDateFormat * udat_clone(const UDateFormat * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateFormat * (*FuncPtr)(const UDateFormat * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_clone");
    abort();
  }

  return ptr(fmt, status);
}
int32_t udat_format(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPosition * position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPosition * position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_format") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_format", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_format");
    abort();
  }

  return ptr(format, dateToFormat, result, resultLength, position, status);
}
int32_t udat_formatCalendar(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPosition * position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPosition * position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_formatCalendar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_formatCalendar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_formatCalendar");
    abort();
  }

  return ptr(format, calendar, result, capacity, position, status);
}
int32_t udat_formatForFields(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPositionIterator * fpositer, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPositionIterator * fpositer, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_formatForFields") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_formatForFields", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_formatForFields");
    abort();
  }

  return ptr(format, dateToFormat, result, resultLength, fpositer, status);
}
int32_t udat_formatCalendarForFields(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPositionIterator * fpositer, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPositionIterator * fpositer, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_formatCalendarForFields") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_formatCalendarForFields", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_formatCalendarForFields");
    abort();
  }

  return ptr(format, calendar, result, capacity, fpositer, status);
}
UDate udat_parse(const UDateFormat * format, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)(const UDateFormat * format, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_parse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_parse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_parse");
    abort();
  }

  return ptr(format, text, textLength, parsePos, status);
}
void udat_parseCalendar(const UDateFormat * format, UCalendar * calendar, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UDateFormat * format, UCalendar * calendar, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_parseCalendar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_parseCalendar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_parseCalendar");
    abort();
  }

  ptr(format, calendar, text, textLength, parsePos, status);
}
UBool udat_isLenient(const UDateFormat * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UDateFormat * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_isLenient") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_isLenient", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_isLenient");
    abort();
  }

  return ptr(fmt);
}
void udat_setLenient(UDateFormat * fmt, UBool isLenient) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, UBool isLenient);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setLenient") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setLenient", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setLenient");
    abort();
  }

  ptr(fmt, isLenient);
}
const UCalendar * udat_getCalendar(const UDateFormat * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UCalendar * (*FuncPtr)(const UDateFormat * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getCalendar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getCalendar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getCalendar");
    abort();
  }

  return ptr(fmt);
}
void udat_setCalendar(UDateFormat * fmt, const UCalendar * calendarToSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, const UCalendar * calendarToSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setCalendar") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setCalendar", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setCalendar");
    abort();
  }

  ptr(fmt, calendarToSet);
}
const UNumberFormat * udat_getNumberFormat(const UDateFormat * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNumberFormat * (*FuncPtr)(const UDateFormat * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getNumberFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getNumberFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getNumberFormat");
    abort();
  }

  return ptr(fmt);
}
const UNumberFormat * udat_getNumberFormatForField(const UDateFormat * fmt, UChar field) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UNumberFormat * (*FuncPtr)(const UDateFormat * fmt, UChar field);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getNumberFormatForField") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getNumberFormatForField", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getNumberFormatForField");
    abort();
  }

  return ptr(fmt, field);
}
void udat_adoptNumberFormatForFields(UDateFormat * fmt, const UChar * fields, UNumberFormat * numberFormatToSet, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, const UChar * fields, UNumberFormat * numberFormatToSet, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_adoptNumberFormatForFields") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_adoptNumberFormatForFields", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_adoptNumberFormatForFields");
    abort();
  }

  ptr(fmt, fields, numberFormatToSet, status);
}
void udat_setNumberFormat(UDateFormat * fmt, const UNumberFormat * numberFormatToSet) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, const UNumberFormat * numberFormatToSet);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setNumberFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setNumberFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setNumberFormat");
    abort();
  }

  ptr(fmt, numberFormatToSet);
}
void udat_adoptNumberFormat(UDateFormat * fmt, UNumberFormat * numberFormatToAdopt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, UNumberFormat * numberFormatToAdopt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_adoptNumberFormat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_adoptNumberFormat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_adoptNumberFormat");
    abort();
  }

  ptr(fmt, numberFormatToAdopt);
}
const char * udat_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t localeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t udat_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_countAvailable");
    abort();
  }

  return ptr();
}
UDate udat_get2DigitYearStart(const UDateFormat * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)(const UDateFormat * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_get2DigitYearStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_get2DigitYearStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_get2DigitYearStart");
    abort();
  }

  return ptr(fmt, status);
}
void udat_set2DigitYearStart(UDateFormat * fmt, UDate d, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, UDate d, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_set2DigitYearStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_set2DigitYearStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_set2DigitYearStart");
    abort();
  }

  ptr(fmt, d, status);
}
int32_t udat_toPattern(const UDateFormat * fmt, UBool localized, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * fmt, UBool localized, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_toPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_toPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_toPattern");
    abort();
  }

  return ptr(fmt, localized, result, resultLength, status);
}
void udat_applyPattern(UDateFormat * format, UBool localized, const UChar * pattern, int32_t patternLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * format, UBool localized, const UChar * pattern, int32_t patternLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_applyPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_applyPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_applyPattern");
    abort();
  }

  ptr(format, localized, pattern, patternLength);
}
int32_t udat_getSymbols(const UDateFormat * fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getSymbols") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getSymbols", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getSymbols");
    abort();
  }

  return ptr(fmt, type, symbolIndex, result, resultLength, status);
}
int32_t udat_countSymbols(const UDateFormat * fmt, UDateFormatSymbolType type) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateFormat * fmt, UDateFormatSymbolType type);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_countSymbols") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_countSymbols", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_countSymbols");
    abort();
  }

  return ptr(fmt, type);
}
void udat_setSymbols(UDateFormat * format, UDateFormatSymbolType type, int32_t symbolIndex, UChar * value, int32_t valueLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * format, UDateFormatSymbolType type, int32_t symbolIndex, UChar * value, int32_t valueLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setSymbols") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setSymbols", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setSymbols");
    abort();
  }

  ptr(format, type, symbolIndex, value, valueLength, status);
}
const char * udat_getLocaleByType(const UDateFormat * fmt, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UDateFormat * fmt, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getLocaleByType");
    abort();
  }

  return ptr(fmt, type, status);
}
void udat_setContext(UDateFormat * fmt, UDisplayContext value, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateFormat * fmt, UDisplayContext value, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_setContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_setContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_setContext");
    abort();
  }

  ptr(fmt, value, status);
}
UDisplayContext udat_getContext(const UDateFormat * fmt, UDisplayContextType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDisplayContext (*FuncPtr)(const UDateFormat * fmt, UDisplayContextType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udat_getContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udat_getContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udat_getContext");
    abort();
  }

  return ptr(fmt, type, status);
}
USpoofChecker * uspoof_open(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USpoofChecker * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_open");
    abort();
  }

  return ptr(status);
}
USpoofChecker * uspoof_openFromSerialized(const void * data, int32_t length, int32_t * pActualLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USpoofChecker * (*FuncPtr)(const void * data, int32_t length, int32_t * pActualLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_openFromSerialized") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_openFromSerialized", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_openFromSerialized");
    abort();
  }

  return ptr(data, length, pActualLength, pErrorCode);
}
USpoofChecker * uspoof_openFromSource(const char * confusables, int32_t confusablesLen, const char * confusablesWholeScript, int32_t confusablesWholeScriptLen, int32_t * errType, UParseError * pe, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USpoofChecker * (*FuncPtr)(const char * confusables, int32_t confusablesLen, const char * confusablesWholeScript, int32_t confusablesWholeScriptLen, int32_t * errType, UParseError * pe, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_openFromSource") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_openFromSource", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_openFromSource");
    abort();
  }

  return ptr(confusables, confusablesLen, confusablesWholeScript, confusablesWholeScriptLen, errType, pe, status);
}
void uspoof_close(USpoofChecker * sc) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofChecker * sc);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_close");
    abort();
  }

  ptr(sc);
}
USpoofChecker * uspoof_clone(const USpoofChecker * sc, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USpoofChecker * (*FuncPtr)(const USpoofChecker * sc, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_clone");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setChecks(USpoofChecker * sc, int32_t checks, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofChecker * sc, int32_t checks, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_setChecks") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_setChecks", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_setChecks");
    abort();
  }

  ptr(sc, checks, status);
}
int32_t uspoof_getChecks(const USpoofChecker * sc, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getChecks") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getChecks", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getChecks");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setRestrictionLevel(USpoofChecker * sc, URestrictionLevel restrictionLevel) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofChecker * sc, URestrictionLevel restrictionLevel);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_setRestrictionLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_setRestrictionLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_setRestrictionLevel");
    abort();
  }

  ptr(sc, restrictionLevel);
}
URestrictionLevel uspoof_getRestrictionLevel(const USpoofChecker * sc) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URestrictionLevel (*FuncPtr)(const USpoofChecker * sc);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getRestrictionLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getRestrictionLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getRestrictionLevel");
    abort();
  }

  return ptr(sc);
}
void uspoof_setAllowedLocales(USpoofChecker * sc, const char * localesList, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofChecker * sc, const char * localesList, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_setAllowedLocales") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_setAllowedLocales", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_setAllowedLocales");
    abort();
  }

  ptr(sc, localesList, status);
}
const char * uspoof_getAllowedLocales(USpoofChecker * sc, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(USpoofChecker * sc, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getAllowedLocales") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getAllowedLocales", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getAllowedLocales");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setAllowedChars(USpoofChecker * sc, const USet * chars, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofChecker * sc, const USet * chars, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_setAllowedChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_setAllowedChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_setAllowedChars");
    abort();
  }

  ptr(sc, chars, status);
}
const USet * uspoof_getAllowedChars(const USpoofChecker * sc, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const USet * (*FuncPtr)(const USpoofChecker * sc, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getAllowedChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getAllowedChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getAllowedChars");
    abort();
  }

  return ptr(sc, status);
}
int32_t uspoof_check(const USpoofChecker * sc, const UChar * id, int32_t length, int32_t * position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const UChar * id, int32_t length, int32_t * position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_check") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_check", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_check");
    abort();
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_checkUTF8(const USpoofChecker * sc, const char * id, int32_t length, int32_t * position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const char * id, int32_t length, int32_t * position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_checkUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_checkUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_checkUTF8");
    abort();
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_check2(const USpoofChecker * sc, const UChar * id, int32_t length, USpoofCheckResult * checkResult, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const UChar * id, int32_t length, USpoofCheckResult * checkResult, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_check2") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_check2", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_check2");
    abort();
  }

  return ptr(sc, id, length, checkResult, status);
}
int32_t uspoof_check2UTF8(const USpoofChecker * sc, const char * id, int32_t length, USpoofCheckResult * checkResult, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const char * id, int32_t length, USpoofCheckResult * checkResult, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_check2UTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_check2UTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_check2UTF8");
    abort();
  }

  return ptr(sc, id, length, checkResult, status);
}
USpoofCheckResult * uspoof_openCheckResult(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USpoofCheckResult * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_openCheckResult") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_openCheckResult", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_openCheckResult");
    abort();
  }

  return ptr(status);
}
void uspoof_closeCheckResult(USpoofCheckResult * checkResult) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(USpoofCheckResult * checkResult);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_closeCheckResult") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_closeCheckResult", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_closeCheckResult");
    abort();
  }

  ptr(checkResult);
}
int32_t uspoof_getCheckResultChecks(const USpoofCheckResult * checkResult, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofCheckResult * checkResult, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getCheckResultChecks") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getCheckResultChecks", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getCheckResultChecks");
    abort();
  }

  return ptr(checkResult, status);
}
URestrictionLevel uspoof_getCheckResultRestrictionLevel(const USpoofCheckResult * checkResult, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URestrictionLevel (*FuncPtr)(const USpoofCheckResult * checkResult, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getCheckResultRestrictionLevel") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getCheckResultRestrictionLevel", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getCheckResultRestrictionLevel");
    abort();
  }

  return ptr(checkResult, status);
}
const USet * uspoof_getCheckResultNumerics(const USpoofCheckResult * checkResult, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const USet * (*FuncPtr)(const USpoofCheckResult * checkResult, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getCheckResultNumerics") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getCheckResultNumerics", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getCheckResultNumerics");
    abort();
  }

  return ptr(checkResult, status);
}
int32_t uspoof_areConfusable(const USpoofChecker * sc, const UChar * id1, int32_t length1, const UChar * id2, int32_t length2, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const UChar * id1, int32_t length1, const UChar * id2, int32_t length2, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_areConfusable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_areConfusable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_areConfusable");
    abort();
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_areConfusableUTF8(const USpoofChecker * sc, const char * id1, int32_t length1, const char * id2, int32_t length2, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, const char * id1, int32_t length1, const char * id2, int32_t length2, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_areConfusableUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_areConfusableUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_areConfusableUTF8");
    abort();
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_getSkeleton(const USpoofChecker * sc, uint32_t type, const UChar * id, int32_t length, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, uint32_t type, const UChar * id, int32_t length, UChar * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getSkeleton") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getSkeleton", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getSkeleton");
    abort();
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
int32_t uspoof_getSkeletonUTF8(const USpoofChecker * sc, uint32_t type, const char * id, int32_t length, char * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const USpoofChecker * sc, uint32_t type, const char * id, int32_t length, char * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getSkeletonUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getSkeletonUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getSkeletonUTF8");
    abort();
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
const USet * uspoof_getInclusionSet(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const USet * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getInclusionSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getInclusionSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getInclusionSet");
    abort();
  }

  return ptr(status);
}
const USet * uspoof_getRecommendedSet(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const USet * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_getRecommendedSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_getRecommendedSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_getRecommendedSet");
    abort();
  }

  return ptr(status);
}
int32_t uspoof_serialize(USpoofChecker * sc, void * data, int32_t capacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(USpoofChecker * sc, void * data, int32_t capacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uspoof_serialize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uspoof_serialize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uspoof_serialize");
    abort();
  }

  return ptr(sc, data, capacity, status);
}
int32_t u_formatMessage(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vformatMessage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vformatMessage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_formatMessage");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, status);
}
int32_t u_vformatMessage(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, int ap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, int ap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vformatMessage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vformatMessage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_vformatMessage");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}
void u_parseMessage(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vparseMessage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vparseMessage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_parseMessage");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, status);
}
void u_vparseMessage(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, int ap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, int ap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vparseMessage") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vparseMessage", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_vparseMessage");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
}
int32_t u_formatMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vformatMessageWithError") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vformatMessageWithError", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_formatMessageWithError");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, parseError, status);
}
int32_t u_vformatMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, int ap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, int ap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vformatMessageWithError") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vformatMessageWithError", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_vformatMessageWithError");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}
void u_parseMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UParseError * parseError, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UParseError * parseError, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vparseMessageWithError") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vparseMessageWithError", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_parseMessageWithError");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, parseError, status);
}
void u_vparseMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, int ap, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, int ap, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("u_vparseMessageWithError") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "u_vparseMessageWithError", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "u_vparseMessageWithError");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
}
UMessageFormat * umsg_open(const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UMessageFormat * (*FuncPtr)(const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_open");
    abort();
  }

  return ptr(pattern, patternLength, locale, parseError, status);
}
void umsg_close(UMessageFormat * format) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UMessageFormat * format);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_close");
    abort();
  }

  ptr(format);
}
UMessageFormat umsg_clone(const UMessageFormat * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UMessageFormat (*FuncPtr)(const UMessageFormat * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_clone");
    abort();
  }

  return ptr(fmt, status);
}
void umsg_setLocale(UMessageFormat * fmt, const char * locale) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UMessageFormat * fmt, const char * locale);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_setLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_setLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_setLocale");
    abort();
  }

  ptr(fmt, locale);
}
const char * umsg_getLocale(const UMessageFormat * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UMessageFormat * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_getLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_getLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_getLocale");
    abort();
  }

  return ptr(fmt);
}
void umsg_applyPattern(UMessageFormat * fmt, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UMessageFormat * fmt, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_applyPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_applyPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_applyPattern");
    abort();
  }

  ptr(fmt, pattern, patternLength, parseError, status);
}
int32_t umsg_toPattern(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_toPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_toPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_toPattern");
    abort();
  }

  return ptr(fmt, result, resultLength, status);
}
int32_t umsg_format(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_vformat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_vformat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_format");
    abort();
  }

  return ptr(fmt, result, resultLength, status);
}
int32_t umsg_vformat(const UMessageFormat * fmt, UChar * result, int32_t resultLength, int ap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UMessageFormat * fmt, UChar * result, int32_t resultLength, int ap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_vformat") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_vformat", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_vformat");
    abort();
  }

  return ptr(fmt, result, resultLength, ap, status);
}
void umsg_parse(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, UErrorCode * status, ...) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, UErrorCode * status, ...);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_vparse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_vparse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_parse");
    abort();
  }

  ptr(fmt, source, sourceLength, count, status);
}
void umsg_vparse(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, int ap, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, int ap, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_vparse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_vparse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_vparse");
    abort();
  }

  ptr(fmt, source, sourceLength, count, ap, status);
}
int32_t umsg_autoQuoteApostrophe(const UChar * pattern, int32_t patternLength, UChar * dest, int32_t destCapacity, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * pattern, int32_t patternLength, UChar * dest, int32_t destCapacity, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("umsg_autoQuoteApostrophe") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "umsg_autoQuoteApostrophe", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "umsg_autoQuoteApostrophe");
    abort();
  }

  return ptr(pattern, patternLength, dest, destCapacity, ec);
}
URelativeDateTimeFormatter * ureldatefmt_open(const char * locale, UNumberFormat * nfToAdopt, UDateRelativeDateTimeFormatterStyle width, UDisplayContext capitalizationContext, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URelativeDateTimeFormatter * (*FuncPtr)(const char * locale, UNumberFormat * nfToAdopt, UDateRelativeDateTimeFormatterStyle width, UDisplayContext capitalizationContext, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ureldatefmt_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ureldatefmt_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ureldatefmt_open");
    abort();
  }

  return ptr(locale, nfToAdopt, width, capitalizationContext, status);
}
void ureldatefmt_close(URelativeDateTimeFormatter * reldatefmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URelativeDateTimeFormatter * reldatefmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ureldatefmt_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ureldatefmt_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ureldatefmt_close");
    abort();
  }

  ptr(reldatefmt);
}
int32_t ureldatefmt_formatNumeric(const URelativeDateTimeFormatter * reldatefmt, double offset, URelativeDateTimeUnit unit, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URelativeDateTimeFormatter * reldatefmt, double offset, URelativeDateTimeUnit unit, UChar * result, int32_t resultCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ureldatefmt_formatNumeric") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ureldatefmt_formatNumeric", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ureldatefmt_formatNumeric");
    abort();
  }

  return ptr(reldatefmt, offset, unit, result, resultCapacity, status);
}
int32_t ureldatefmt_format(const URelativeDateTimeFormatter * reldatefmt, double offset, URelativeDateTimeUnit unit, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URelativeDateTimeFormatter * reldatefmt, double offset, URelativeDateTimeUnit unit, UChar * result, int32_t resultCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ureldatefmt_format") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ureldatefmt_format", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ureldatefmt_format");
    abort();
  }

  return ptr(reldatefmt, offset, unit, result, resultCapacity, status);
}
int32_t ureldatefmt_combineDateAndTime(const URelativeDateTimeFormatter * reldatefmt, const UChar * relativeDateString, int32_t relativeDateStringLen, const UChar * timeString, int32_t timeStringLen, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URelativeDateTimeFormatter * reldatefmt, const UChar * relativeDateString, int32_t relativeDateStringLen, const UChar * timeString, int32_t timeStringLen, UChar * result, int32_t resultCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ureldatefmt_combineDateAndTime") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ureldatefmt_combineDateAndTime", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ureldatefmt_combineDateAndTime");
    abort();
  }

  return ptr(reldatefmt, relativeDateString, relativeDateStringLen, timeString, timeStringLen, result, resultCapacity, status);
}
URegularExpression * uregex_open(const UChar * pattern, int32_t patternLength, uint32_t flags, UParseError * pe, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URegularExpression * (*FuncPtr)(const UChar * pattern, int32_t patternLength, uint32_t flags, UParseError * pe, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_open");
    abort();
  }

  return ptr(pattern, patternLength, flags, pe, status);
}
URegularExpression * uregex_openUText(UText * pattern, uint32_t flags, UParseError * pe, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URegularExpression * (*FuncPtr)(UText * pattern, uint32_t flags, UParseError * pe, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_openUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_openUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_openUText");
    abort();
  }

  return ptr(pattern, flags, pe, status);
}
URegularExpression * uregex_openC(const char * pattern, uint32_t flags, UParseError * pe, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URegularExpression * (*FuncPtr)(const char * pattern, uint32_t flags, UParseError * pe, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_openC") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_openC", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_openC");
    abort();
  }

  return ptr(pattern, flags, pe, status);
}
void uregex_close(URegularExpression * regexp) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_close");
    abort();
  }

  ptr(regexp);
}
URegularExpression * uregex_clone(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URegularExpression * (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_clone");
    abort();
  }

  return ptr(regexp, status);
}
const UChar * uregex_pattern(const URegularExpression * regexp, int32_t * patLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const URegularExpression * regexp, int32_t * patLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_pattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_pattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_pattern");
    abort();
  }

  return ptr(regexp, patLength, status);
}
UText * uregex_patternUText(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_patternUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_patternUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_patternUText");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_flags(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_flags") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_flags", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_flags");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setText(URegularExpression * regexp, const UChar * text, int32_t textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, const UChar * text, int32_t textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setText");
    abort();
  }

  ptr(regexp, text, textLength, status);
}
void uregex_setUText(URegularExpression * regexp, UText * text, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, UText * text, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setUText");
    abort();
  }

  ptr(regexp, text, status);
}
const UChar * uregex_getText(URegularExpression * regexp, int32_t * textLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(URegularExpression * regexp, int32_t * textLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getText");
    abort();
  }

  return ptr(regexp, textLength, status);
}
UText * uregex_getUText(URegularExpression * regexp, UText * dest, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(URegularExpression * regexp, UText * dest, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getUText");
    abort();
  }

  return ptr(regexp, dest, status);
}
void uregex_refreshUText(URegularExpression * regexp, UText * text, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, UText * text, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_refreshUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_refreshUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_refreshUText");
    abort();
  }

  ptr(regexp, text, status);
}
UBool uregex_matches(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int32_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_matches") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_matches", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_matches");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_matches64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int64_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_matches64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_matches64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_matches64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int32_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_lookingAt") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_lookingAt", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_lookingAt");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int64_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_lookingAt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_lookingAt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_lookingAt64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int32_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_find") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_find", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_find");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, int64_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_find64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_find64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_find64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_findNext(URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_findNext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_findNext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_findNext");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_groupCount(URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_groupCount") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_groupCount", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_groupCount");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_groupNumberFromName(URegularExpression * regexp, const UChar * groupName, int32_t nameLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, const UChar * groupName, int32_t nameLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_groupNumberFromName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_groupNumberFromName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_groupNumberFromName");
    abort();
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_groupNumberFromCName(URegularExpression * regexp, const char * groupName, int32_t nameLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, const char * groupName, int32_t nameLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_groupNumberFromCName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_groupNumberFromCName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_groupNumberFromCName");
    abort();
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_group(URegularExpression * regexp, int32_t groupNum, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UChar * dest, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_group") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_group", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_group");
    abort();
  }

  return ptr(regexp, groupNum, dest, destCapacity, status);
}
UText * uregex_groupUText(URegularExpression * regexp, int32_t groupNum, UText * dest, int64_t * groupLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UText * dest, int64_t * groupLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_groupUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_groupUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_groupUText");
    abort();
  }

  return ptr(regexp, groupNum, dest, groupLength, status);
}
int32_t uregex_start(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_start") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_start", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_start");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_start64(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_start64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_start64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_start64");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int32_t uregex_end(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_end") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_end", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_end");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_end64(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(URegularExpression * regexp, int32_t groupNum, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_end64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_end64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_end64");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
void uregex_reset(URegularExpression * regexp, int32_t index, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int32_t index, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_reset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_reset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_reset");
    abort();
  }

  ptr(regexp, index, status);
}
void uregex_reset64(URegularExpression * regexp, int64_t index, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int64_t index, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_reset64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_reset64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_reset64");
    abort();
  }

  ptr(regexp, index, status);
}
void uregex_setRegion(URegularExpression * regexp, int32_t regionStart, int32_t regionLimit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int32_t regionStart, int32_t regionLimit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setRegion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setRegion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setRegion");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegion64(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setRegion64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setRegion64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setRegion64");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegionAndStart(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setRegionAndStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setRegionAndStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setRegionAndStart");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, startIndex, status);
}
int32_t uregex_regionStart(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_regionStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_regionStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_regionStart");
    abort();
  }

  return ptr(regexp, status);
}
int64_t uregex_regionStart64(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_regionStart64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_regionStart64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_regionStart64");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_regionEnd(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_regionEnd") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_regionEnd", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_regionEnd");
    abort();
  }

  return ptr(regexp, status);
}
int64_t uregex_regionEnd64(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_regionEnd64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_regionEnd64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_regionEnd64");
    abort();
  }

  return ptr(regexp, status);
}
UBool uregex_hasTransparentBounds(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_hasTransparentBounds") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_hasTransparentBounds", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_hasTransparentBounds");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_useTransparentBounds(URegularExpression * regexp, UBool b, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, UBool b, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_useTransparentBounds") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_useTransparentBounds", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_useTransparentBounds");
    abort();
  }

  ptr(regexp, b, status);
}
UBool uregex_hasAnchoringBounds(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_hasAnchoringBounds") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_hasAnchoringBounds", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_hasAnchoringBounds");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_useAnchoringBounds(URegularExpression * regexp, UBool b, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, UBool b, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_useAnchoringBounds") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_useAnchoringBounds", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_useAnchoringBounds");
    abort();
  }

  ptr(regexp, b, status);
}
UBool uregex_hitEnd(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_hitEnd") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_hitEnd", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_hitEnd");
    abort();
  }

  return ptr(regexp, status);
}
UBool uregex_requireEnd(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_requireEnd") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_requireEnd", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_requireEnd");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_replaceAll(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_replaceAll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_replaceAll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_replaceAll");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText * uregex_replaceAllUText(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_replaceAllUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_replaceAllUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_replaceAllUText");
    abort();
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_replaceFirst(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_replaceFirst") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_replaceFirst", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_replaceFirst");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText * uregex_replaceFirstUText(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_replaceFirstUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_replaceFirstUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_replaceFirstUText");
    abort();
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_appendReplacement(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_appendReplacement") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_appendReplacement", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_appendReplacement");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
void uregex_appendReplacementUText(URegularExpression * regexp, UText * replacementText, UText * dest, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, UText * replacementText, UText * dest, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_appendReplacementUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_appendReplacementUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_appendReplacementUText");
    abort();
  }

  ptr(regexp, replacementText, dest, status);
}
int32_t uregex_appendTail(URegularExpression * regexp, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_appendTail") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_appendTail", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_appendTail");
    abort();
  }

  return ptr(regexp, destBuf, destCapacity, status);
}
UText * uregex_appendTailUText(URegularExpression * regexp, UText * dest, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UText * (*FuncPtr)(URegularExpression * regexp, UText * dest, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_appendTailUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_appendTailUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_appendTailUText");
    abort();
  }

  return ptr(regexp, dest, status);
}
int32_t uregex_split(URegularExpression * regexp, UChar * destBuf, int32_t destCapacity, int32_t * requiredCapacity, UChar * destFields[], int32_t destFieldsCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, UChar * destBuf, int32_t destCapacity, int32_t * requiredCapacity, UChar * destFields[], int32_t destFieldsCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_split") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_split", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_split");
    abort();
  }

  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}
int32_t uregex_splitUText(URegularExpression * regexp, UText * destFields[], int32_t destFieldsCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(URegularExpression * regexp, UText * destFields[], int32_t destFieldsCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_splitUText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_splitUText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_splitUText");
    abort();
  }

  return ptr(regexp, destFields, destFieldsCapacity, status);
}
void uregex_setTimeLimit(URegularExpression * regexp, int32_t limit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int32_t limit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setTimeLimit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setTimeLimit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setTimeLimit");
    abort();
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getTimeLimit(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getTimeLimit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getTimeLimit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getTimeLimit");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setStackLimit(URegularExpression * regexp, int32_t limit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, int32_t limit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setStackLimit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setStackLimit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setStackLimit");
    abort();
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getStackLimit(const URegularExpression * regexp, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegularExpression * regexp, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getStackLimit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getStackLimit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getStackLimit");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setMatchCallback(URegularExpression * regexp, URegexMatchCallback * callback, const void * context, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, URegexMatchCallback * callback, const void * context, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setMatchCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setMatchCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setMatchCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_getMatchCallback(const URegularExpression * regexp, URegexMatchCallback ** callback, const void ** context, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const URegularExpression * regexp, URegexMatchCallback ** callback, const void ** context, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getMatchCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getMatchCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getMatchCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_setFindProgressCallback(URegularExpression * regexp, URegexFindProgressCallback * callback, const void * context, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(URegularExpression * regexp, URegexFindProgressCallback * callback, const void * context, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_setFindProgressCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_setFindProgressCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_setFindProgressCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_getFindProgressCallback(const URegularExpression * regexp, URegexFindProgressCallback ** callback, const void ** context, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const URegularExpression * regexp, URegexFindProgressCallback ** callback, const void ** context, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregex_getFindProgressCallback") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregex_getFindProgressCallback", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregex_getFindProgressCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
UNumberingSystem * unumsys_open(const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNumberingSystem * (*FuncPtr)(const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_open");
    abort();
  }

  return ptr(locale, status);
}
UNumberingSystem * unumsys_openByName(const char * name, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNumberingSystem * (*FuncPtr)(const char * name, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_openByName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_openByName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_openByName");
    abort();
  }

  return ptr(name, status);
}
void unumsys_close(UNumberingSystem * unumsys) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberingSystem * unumsys);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_close");
    abort();
  }

  ptr(unumsys);
}
UEnumeration * unumsys_openAvailableNames(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_openAvailableNames") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_openAvailableNames", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_openAvailableNames");
    abort();
  }

  return ptr(status);
}
const char * unumsys_getName(const UNumberingSystem * unumsys) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UNumberingSystem * unumsys);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_getName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_getName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_getName");
    abort();
  }

  return ptr(unumsys);
}
UBool unumsys_isAlgorithmic(const UNumberingSystem * unumsys) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UNumberingSystem * unumsys);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_isAlgorithmic") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_isAlgorithmic", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_isAlgorithmic");
    abort();
  }

  return ptr(unumsys);
}
int32_t unumsys_getRadix(const UNumberingSystem * unumsys) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberingSystem * unumsys);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_getRadix") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_getRadix", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_getRadix");
    abort();
  }

  return ptr(unumsys);
}
int32_t unumsys_getDescription(const UNumberingSystem * unumsys, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberingSystem * unumsys, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unumsys_getDescription") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unumsys_getDescription", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unumsys_getDescription");
    abort();
  }

  return ptr(unumsys, result, resultLength, status);
}
UCollator * ucol_open(const char * loc, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollator * (*FuncPtr)(const char * loc, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_open");
    abort();
  }

  return ptr(loc, status);
}
UCollator * ucol_openRules(const UChar * rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollator * (*FuncPtr)(const UChar * rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_openRules") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_openRules", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_openRules");
    abort();
  }

  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}
void ucol_getContractionsAndExpansions(const UCollator * coll, USet * contractions, USet * expansions, UBool addPrefixes, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UCollator * coll, USet * contractions, USet * expansions, UBool addPrefixes, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getContractionsAndExpansions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getContractionsAndExpansions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getContractionsAndExpansions");
    abort();
  }

  ptr(coll, contractions, expansions, addPrefixes, status);
}
void ucol_close(UCollator * coll) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollator * coll);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_close");
    abort();
  }

  ptr(coll);
}
UCollationResult ucol_strcoll(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollationResult (*FuncPtr)(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_strcoll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_strcoll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_strcoll");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollUTF8(const UCollator * coll, const char * source, int32_t sourceLength, const char * target, int32_t targetLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollationResult (*FuncPtr)(const UCollator * coll, const char * source, int32_t sourceLength, const char * target, int32_t targetLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_strcollUTF8") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_strcollUTF8", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_strcollUTF8");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength, status);
}
UBool ucol_greater(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_greater") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_greater", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_greater");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_greaterOrEqual(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_greaterOrEqual") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_greaterOrEqual", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_greaterOrEqual");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_equal(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_equal") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_equal", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_equal");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollIter(const UCollator * coll, UCharIterator * sIter, UCharIterator * tIter, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollationResult (*FuncPtr)(const UCollator * coll, UCharIterator * sIter, UCharIterator * tIter, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_strcollIter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_strcollIter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_strcollIter");
    abort();
  }

  return ptr(coll, sIter, tIter, status);
}
UCollationStrength ucol_getStrength(const UCollator * coll) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollationStrength (*FuncPtr)(const UCollator * coll);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getStrength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getStrength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getStrength");
    abort();
  }

  return ptr(coll);
}
void ucol_setStrength(UCollator * coll, UCollationStrength strength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollator * coll, UCollationStrength strength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setStrength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setStrength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setStrength");
    abort();
  }

  ptr(coll, strength);
}
int32_t ucol_getReorderCodes(const UCollator * coll, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollator * coll, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getReorderCodes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getReorderCodes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getReorderCodes");
    abort();
  }

  return ptr(coll, dest, destCapacity, pErrorCode);
}
void ucol_setReorderCodes(UCollator * coll, const int32_t * reorderCodes, int32_t reorderCodesLength, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollator * coll, const int32_t * reorderCodes, int32_t reorderCodesLength, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setReorderCodes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setReorderCodes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setReorderCodes");
    abort();
  }

  ptr(coll, reorderCodes, reorderCodesLength, pErrorCode);
}
int32_t ucol_getEquivalentReorderCodes(int32_t reorderCode, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(int32_t reorderCode, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getEquivalentReorderCodes") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getEquivalentReorderCodes", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getEquivalentReorderCodes");
    abort();
  }

  return ptr(reorderCode, dest, destCapacity, pErrorCode);
}
int32_t ucol_getDisplayName(const char * objLoc, const char * dispLoc, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const char * objLoc, const char * dispLoc, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getDisplayName");
    abort();
  }

  return ptr(objLoc, dispLoc, result, resultLength, status);
}
const char * ucol_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t localeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t ucol_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_countAvailable");
    abort();
  }

  return ptr();
}
UEnumeration * ucol_openAvailableLocales(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_openAvailableLocales") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_openAvailableLocales", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_openAvailableLocales");
    abort();
  }

  return ptr(status);
}
UEnumeration * ucol_getKeywords(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getKeywords") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getKeywords", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getKeywords");
    abort();
  }

  return ptr(status);
}
UEnumeration * ucol_getKeywordValues(const char * keyword, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * keyword, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getKeywordValues") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getKeywordValues", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getKeywordValues");
    abort();
  }

  return ptr(keyword, status);
}
UEnumeration * ucol_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getKeywordValuesForLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getKeywordValuesForLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucol_getFunctionalEquivalent(char * result, int32_t resultCapacity, const char * keyword, const char * locale, UBool * isAvailable, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(char * result, int32_t resultCapacity, const char * keyword, const char * locale, UBool * isAvailable, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getFunctionalEquivalent") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getFunctionalEquivalent", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getFunctionalEquivalent");
    abort();
  }

  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}
const UChar * ucol_getRules(const UCollator * coll, int32_t * length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UCollator * coll, int32_t * length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getRules") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getRules", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getRules");
    abort();
  }

  return ptr(coll, length);
}
int32_t ucol_getSortKey(const UCollator * coll, const UChar * source, int32_t sourceLength, uint8_t * result, int32_t resultLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollator * coll, const UChar * source, int32_t sourceLength, uint8_t * result, int32_t resultLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getSortKey") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getSortKey", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getSortKey");
    abort();
  }

  return ptr(coll, source, sourceLength, result, resultLength);
}
int32_t ucol_nextSortKeyPart(const UCollator * coll, UCharIterator * iter, uint32_t  state[2], uint8_t * dest, int32_t count, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollator * coll, UCharIterator * iter, uint32_t  state[2], uint8_t * dest, int32_t count, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_nextSortKeyPart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_nextSortKeyPart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_nextSortKeyPart");
    abort();
  }

  return ptr(coll, iter, state, dest, count, status);
}
int32_t ucol_getBound(const uint8_t * source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const uint8_t * source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getBound") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getBound", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getBound");
    abort();
  }

  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}
void ucol_getVersion(const UCollator * coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UCollator * coll, UVersionInfo info);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getVersion");
    abort();
  }

  ptr(coll, info);
}
void ucol_getUCAVersion(const UCollator * coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UCollator * coll, UVersionInfo info);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getUCAVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getUCAVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getUCAVersion");
    abort();
  }

  ptr(coll, info);
}
int32_t ucol_mergeSortkeys(const uint8_t * src1, int32_t src1Length, const uint8_t * src2, int32_t src2Length, uint8_t * dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const uint8_t * src1, int32_t src1Length, const uint8_t * src2, int32_t src2Length, uint8_t * dest, int32_t destCapacity);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_mergeSortkeys") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_mergeSortkeys", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_mergeSortkeys");
    abort();
  }

  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}
void ucol_setAttribute(UCollator * coll, UColAttribute attr, UColAttributeValue value, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollator * coll, UColAttribute attr, UColAttributeValue value, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setAttribute");
    abort();
  }

  ptr(coll, attr, value, status);
}
UColAttributeValue ucol_getAttribute(const UCollator * coll, UColAttribute attr, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UColAttributeValue (*FuncPtr)(const UCollator * coll, UColAttribute attr, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getAttribute");
    abort();
  }

  return ptr(coll, attr, status);
}
void ucol_setMaxVariable(UCollator * coll, UColReorderCode group, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCollator * coll, UColReorderCode group, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_setMaxVariable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_setMaxVariable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_setMaxVariable");
    abort();
  }

  ptr(coll, group, pErrorCode);
}
UColReorderCode ucol_getMaxVariable(const UCollator * coll) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UColReorderCode (*FuncPtr)(const UCollator * coll);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getMaxVariable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getMaxVariable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getMaxVariable");
    abort();
  }

  return ptr(coll);
}
uint32_t ucol_getVariableTop(const UCollator * coll, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef uint32_t (*FuncPtr)(const UCollator * coll, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getVariableTop") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getVariableTop", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getVariableTop");
    abort();
  }

  return ptr(coll, status);
}
UCollator * ucol_safeClone(const UCollator * coll, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollator * (*FuncPtr)(const UCollator * coll, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_safeClone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_safeClone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_safeClone");
    abort();
  }

  return ptr(coll, stackBuffer, pBufferSize, status);
}
int32_t ucol_getRulesEx(const UCollator * coll, UColRuleOption delta, UChar * buffer, int32_t bufferLen) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollator * coll, UColRuleOption delta, UChar * buffer, int32_t bufferLen);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getRulesEx") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getRulesEx", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getRulesEx");
    abort();
  }

  return ptr(coll, delta, buffer, bufferLen);
}
const char * ucol_getLocaleByType(const UCollator * coll, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCollator * coll, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getLocaleByType");
    abort();
  }

  return ptr(coll, type, status);
}
USet * ucol_getTailoredSet(const UCollator * coll, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const UCollator * coll, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_getTailoredSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_getTailoredSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_getTailoredSet");
    abort();
  }

  return ptr(coll, status);
}
int32_t ucol_cloneBinary(const UCollator * coll, uint8_t * buffer, int32_t capacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCollator * coll, uint8_t * buffer, int32_t capacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_cloneBinary") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_cloneBinary", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_cloneBinary");
    abort();
  }

  return ptr(coll, buffer, capacity, status);
}
UCollator * ucol_openBinary(const uint8_t * bin, int32_t length, const UCollator * base, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollator * (*FuncPtr)(const uint8_t * bin, int32_t length, const UCollator * base, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucol_openBinary") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucol_openBinary", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucol_openBinary");
    abort();
  }

  return ptr(bin, length, base, status);
}
UTransliterator * utrans_openU(const UChar * id, int32_t idLength, UTransDirection dir, const UChar * rules, int32_t rulesLength, UParseError * parseError, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UTransliterator * (*FuncPtr)(const UChar * id, int32_t idLength, UTransDirection dir, const UChar * rules, int32_t rulesLength, UParseError * parseError, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_openU") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_openU", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_openU");
    abort();
  }

  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}
UTransliterator * utrans_openInverse(const UTransliterator * trans, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UTransliterator * (*FuncPtr)(const UTransliterator * trans, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_openInverse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_openInverse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_openInverse");
    abort();
  }

  return ptr(trans, status);
}
UTransliterator * utrans_clone(const UTransliterator * trans, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UTransliterator * (*FuncPtr)(const UTransliterator * trans, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_clone");
    abort();
  }

  return ptr(trans, status);
}
void utrans_close(UTransliterator * trans) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UTransliterator * trans);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_close");
    abort();
  }

  ptr(trans);
}
const UChar * utrans_getUnicodeID(const UTransliterator * trans, int32_t * resultLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UTransliterator * trans, int32_t * resultLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_getUnicodeID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_getUnicodeID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_getUnicodeID");
    abort();
  }

  return ptr(trans, resultLength);
}
void utrans_register(UTransliterator * adoptedTrans, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UTransliterator * adoptedTrans, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_register") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_register", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_register");
    abort();
  }

  ptr(adoptedTrans, status);
}
void utrans_unregisterID(const UChar * id, int32_t idLength) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UChar * id, int32_t idLength);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_unregisterID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_unregisterID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_unregisterID");
    abort();
  }

  ptr(id, idLength);
}
void utrans_setFilter(UTransliterator * trans, const UChar * filterPattern, int32_t filterPatternLen, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UTransliterator * trans, const UChar * filterPattern, int32_t filterPatternLen, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_setFilter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_setFilter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_setFilter");
    abort();
  }

  ptr(trans, filterPattern, filterPatternLen, status);
}
int32_t utrans_countAvailableIDs() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_countAvailableIDs") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_countAvailableIDs", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_countAvailableIDs");
    abort();
  }

  return ptr();
}
UEnumeration * utrans_openIDs(UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_openIDs") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_openIDs", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_openIDs");
    abort();
  }

  return ptr(pErrorCode);
}
void utrans_trans(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, int32_t start, int32_t * limit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, int32_t start, int32_t * limit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_trans") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_trans", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_trans");
    abort();
  }

  ptr(trans, rep, repFunc, start, limit, status);
}
void utrans_transIncremental(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, UTransPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, UTransPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_transIncremental") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_transIncremental", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_transIncremental");
    abort();
  }

  ptr(trans, rep, repFunc, pos, status);
}
void utrans_transUChars(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, int32_t start, int32_t * limit, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, int32_t start, int32_t * limit, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_transUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_transUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_transUChars");
    abort();
  }

  ptr(trans, text, textLength, textCapacity, start, limit, status);
}
void utrans_transIncrementalUChars(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, UTransPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, UTransPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_transIncrementalUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_transIncrementalUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_transIncrementalUChars");
    abort();
  }

  ptr(trans, text, textLength, textCapacity, pos, status);
}
int32_t utrans_toRules(const UTransliterator * trans, UBool escapeUnprintable, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UTransliterator * trans, UBool escapeUnprintable, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_toRules") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_toRules", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_toRules");
    abort();
  }

  return ptr(trans, escapeUnprintable, result, resultLength, status);
}
USet * utrans_getSourceSet(const UTransliterator * trans, UBool ignoreFilter, USet * fillIn, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(const UTransliterator * trans, UBool ignoreFilter, USet * fillIn, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("utrans_getSourceSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "utrans_getSourceSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "utrans_getSourceSet");
    abort();
  }

  return ptr(trans, ignoreFilter, fillIn, status);
}
UStringSearch * usearch_open(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const char * locale, UBreakIterator * breakiter, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UStringSearch * (*FuncPtr)(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const char * locale, UBreakIterator * breakiter, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_open");
    abort();
  }

  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}
UStringSearch * usearch_openFromCollator(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const UCollator * collator, UBreakIterator * breakiter, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UStringSearch * (*FuncPtr)(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const UCollator * collator, UBreakIterator * breakiter, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_openFromCollator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_openFromCollator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_openFromCollator");
    abort();
  }

  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}
void usearch_close(UStringSearch * searchiter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * searchiter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_close");
    abort();
  }

  ptr(searchiter);
}
void usearch_setOffset(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, int32_t position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setOffset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setOffset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setOffset");
    abort();
  }

  ptr(strsrch, position, status);
}
int32_t usearch_getOffset(const UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getOffset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getOffset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getOffset");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setAttribute(UStringSearch * strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setAttribute");
    abort();
  }

  ptr(strsrch, attribute, value, status);
}
USearchAttributeValue usearch_getAttribute(const UStringSearch * strsrch, USearchAttribute attribute) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USearchAttributeValue (*FuncPtr)(const UStringSearch * strsrch, USearchAttribute attribute);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getAttribute");
    abort();
  }

  return ptr(strsrch, attribute);
}
int32_t usearch_getMatchedStart(const UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getMatchedStart") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getMatchedStart", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getMatchedStart");
    abort();
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedLength(const UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getMatchedLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getMatchedLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getMatchedLength");
    abort();
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedText(const UStringSearch * strsrch, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UStringSearch * strsrch, UChar * result, int32_t resultCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getMatchedText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getMatchedText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getMatchedText");
    abort();
  }

  return ptr(strsrch, result, resultCapacity, status);
}
void usearch_setBreakIterator(UStringSearch * strsrch, UBreakIterator * breakiter, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, UBreakIterator * breakiter, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setBreakIterator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setBreakIterator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setBreakIterator");
    abort();
  }

  ptr(strsrch, breakiter, status);
}
const UBreakIterator * usearch_getBreakIterator(const UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UBreakIterator * (*FuncPtr)(const UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getBreakIterator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getBreakIterator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getBreakIterator");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setText(UStringSearch * strsrch, const UChar * text, int32_t textlength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, const UChar * text, int32_t textlength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setText");
    abort();
  }

  ptr(strsrch, text, textlength, status);
}
const UChar * usearch_getText(const UStringSearch * strsrch, int32_t * length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UStringSearch * strsrch, int32_t * length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getText") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getText", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getText");
    abort();
  }

  return ptr(strsrch, length);
}
UCollator * usearch_getCollator(const UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCollator * (*FuncPtr)(const UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getCollator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getCollator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getCollator");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setCollator(UStringSearch * strsrch, const UCollator * collator, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, const UCollator * collator, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setCollator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setCollator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setCollator");
    abort();
  }

  ptr(strsrch, collator, status);
}
void usearch_setPattern(UStringSearch * strsrch, const UChar * pattern, int32_t patternlength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch, const UChar * pattern, int32_t patternlength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_setPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_setPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_setPattern");
    abort();
  }

  ptr(strsrch, pattern, patternlength, status);
}
const UChar * usearch_getPattern(const UStringSearch * strsrch, int32_t * length) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(const UStringSearch * strsrch, int32_t * length);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_getPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_getPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_getPattern");
    abort();
  }

  return ptr(strsrch, length);
}
int32_t usearch_first(UStringSearch * strsrch, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_first") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_first", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_first");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_following(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, int32_t position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_following") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_following", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_following");
    abort();
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_last(UStringSearch * strsrch, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_last") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_last", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_last");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_preceding(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, int32_t position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_preceding") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_preceding", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_preceding");
    abort();
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_next(UStringSearch * strsrch, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_next") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_next", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_next");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_previous(UStringSearch * strsrch, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UStringSearch * strsrch, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_previous") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_previous", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_previous");
    abort();
  }

  return ptr(strsrch, status);
}
void usearch_reset(UStringSearch * strsrch) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UStringSearch * strsrch);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("usearch_reset") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "usearch_reset", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "usearch_reset");
    abort();
  }

  ptr(strsrch);
}
UNumberFormat * unum_open(UNumberFormatStyle style, const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseErr, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNumberFormat * (*FuncPtr)(UNumberFormatStyle style, const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseErr, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_open");
    abort();
  }

  return ptr(style, pattern, patternLength, locale, parseErr, status);
}
void unum_close(UNumberFormat * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_close");
    abort();
  }

  ptr(fmt);
}
UNumberFormat * unum_clone(const UNumberFormat * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UNumberFormat * (*FuncPtr)(const UNumberFormat * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_clone");
    abort();
  }

  return ptr(fmt, status);
}
int32_t unum_format(const UNumberFormat * fmt, int32_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, int32_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_format") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_format", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_format");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatInt64(const UNumberFormat * fmt, int64_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, int64_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_formatInt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_formatInt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_formatInt64");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDouble(const UNumberFormat * fmt, double number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, double number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_formatDouble") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_formatDouble", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_formatDouble");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDecimal(const UNumberFormat * fmt, const char * number, int32_t length, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, const char * number, int32_t length, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_formatDecimal") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_formatDecimal", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_formatDecimal");
    abort();
  }

  return ptr(fmt, number, length, result, resultLength, pos, status);
}
int32_t unum_formatDoubleCurrency(const UNumberFormat * fmt, double number, UChar * currency, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, double number, UChar * currency, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_formatDoubleCurrency") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_formatDoubleCurrency", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_formatDoubleCurrency");
    abort();
  }

  return ptr(fmt, number, currency, result, resultLength, pos, status);
}
int32_t unum_formatUFormattable(const UNumberFormat * fmt, const UFormattable * number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, const UFormattable * number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_formatUFormattable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_formatUFormattable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_formatUFormattable");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_parse(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parse") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parse", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parse");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int64_t unum_parseInt64(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parseInt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parseInt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parseInt64");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
double unum_parseDouble(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parseDouble") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parseDouble", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parseDouble");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int32_t unum_parseDecimal(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, char * outBuf, int32_t outBufLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, char * outBuf, int32_t outBufLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parseDecimal") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parseDecimal", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parseDecimal");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, outBuf, outBufLength, status);
}
double unum_parseDoubleCurrency(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UChar * currency, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UChar * currency, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parseDoubleCurrency") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parseDoubleCurrency", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parseDoubleCurrency");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, currency, status);
}
UFormattable * unum_parseToUFormattable(const UNumberFormat * fmt, UFormattable * result, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UFormattable * (*FuncPtr)(const UNumberFormat * fmt, UFormattable * result, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_parseToUFormattable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_parseToUFormattable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_parseToUFormattable");
    abort();
  }

  return ptr(fmt, result, text, textLength, parsePos, status);
}
void unum_applyPattern(UNumberFormat * format, UBool localized, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * format, UBool localized, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_applyPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_applyPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_applyPattern");
    abort();
  }

  ptr(format, localized, pattern, patternLength, parseError, status);
}
const char * unum_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t localeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t unum_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_countAvailable");
    abort();
  }

  return ptr();
}
int32_t unum_getAttribute(const UNumberFormat * fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, UNumberFormatAttribute attr);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getAttribute");
    abort();
  }

  return ptr(fmt, attr);
}
void unum_setAttribute(UNumberFormat * fmt, UNumberFormatAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt, UNumberFormatAttribute attr, int32_t newValue);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_setAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_setAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_setAttribute");
    abort();
  }

  ptr(fmt, attr, newValue);
}
double unum_getDoubleAttribute(const UNumberFormat * fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(const UNumberFormat * fmt, UNumberFormatAttribute attr);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getDoubleAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getDoubleAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getDoubleAttribute");
    abort();
  }

  return ptr(fmt, attr);
}
void unum_setDoubleAttribute(UNumberFormat * fmt, UNumberFormatAttribute attr, double newValue) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt, UNumberFormatAttribute attr, double newValue);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_setDoubleAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_setDoubleAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_setDoubleAttribute");
    abort();
  }

  ptr(fmt, attr, newValue);
}
int32_t unum_getTextAttribute(const UNumberFormat * fmt, UNumberFormatTextAttribute tag, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, UNumberFormatTextAttribute tag, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getTextAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getTextAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getTextAttribute");
    abort();
  }

  return ptr(fmt, tag, result, resultLength, status);
}
void unum_setTextAttribute(UNumberFormat * fmt, UNumberFormatTextAttribute tag, const UChar * newValue, int32_t newValueLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt, UNumberFormatTextAttribute tag, const UChar * newValue, int32_t newValueLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_setTextAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_setTextAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_setTextAttribute");
    abort();
  }

  ptr(fmt, tag, newValue, newValueLength, status);
}
int32_t unum_toPattern(const UNumberFormat * fmt, UBool isPatternLocalized, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, UBool isPatternLocalized, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_toPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_toPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_toPattern");
    abort();
  }

  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}
int32_t unum_getSymbol(const UNumberFormat * fmt, UNumberFormatSymbol symbol, UChar * buffer, int32_t size, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UNumberFormat * fmt, UNumberFormatSymbol symbol, UChar * buffer, int32_t size, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getSymbol") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getSymbol", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getSymbol");
    abort();
  }

  return ptr(fmt, symbol, buffer, size, status);
}
void unum_setSymbol(UNumberFormat * fmt, UNumberFormatSymbol symbol, const UChar * value, int32_t length, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt, UNumberFormatSymbol symbol, const UChar * value, int32_t length, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_setSymbol") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_setSymbol", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_setSymbol");
    abort();
  }

  ptr(fmt, symbol, value, length, status);
}
const char * unum_getLocaleByType(const UNumberFormat * fmt, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UNumberFormat * fmt, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getLocaleByType");
    abort();
  }

  return ptr(fmt, type, status);
}
void unum_setContext(UNumberFormat * fmt, UDisplayContext value, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UNumberFormat * fmt, UDisplayContext value, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_setContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_setContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_setContext");
    abort();
  }

  ptr(fmt, value, status);
}
UDisplayContext unum_getContext(const UNumberFormat * fmt, UDisplayContextType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDisplayContext (*FuncPtr)(const UNumberFormat * fmt, UDisplayContextType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("unum_getContext") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "unum_getContext", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "unum_getContext");
    abort();
  }

  return ptr(fmt, type, status);
}
const UGenderInfo * ugender_getInstance(const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UGenderInfo * (*FuncPtr)(const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ugender_getInstance") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ugender_getInstance", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ugender_getInstance");
    abort();
  }

  return ptr(locale, status);
}
UGender ugender_getListGender(const UGenderInfo * genderinfo, const UGender * genders, int32_t size, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UGender (*FuncPtr)(const UGenderInfo * genderinfo, const UGender * genders, int32_t size, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ugender_getListGender") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ugender_getListGender", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ugender_getListGender");
    abort();
  }

  return ptr(genderinfo, genders, size, status);
}
UFieldPositionIterator * ufieldpositer_open(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UFieldPositionIterator * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufieldpositer_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufieldpositer_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufieldpositer_open");
    abort();
  }

  return ptr(status);
}
void ufieldpositer_close(UFieldPositionIterator * fpositer) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UFieldPositionIterator * fpositer);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufieldpositer_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufieldpositer_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufieldpositer_close");
    abort();
  }

  ptr(fpositer);
}
int32_t ufieldpositer_next(UFieldPositionIterator * fpositer, int32_t * beginIndex, int32_t * endIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UFieldPositionIterator * fpositer, int32_t * beginIndex, int32_t * endIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufieldpositer_next") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufieldpositer_next", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufieldpositer_next");
    abort();
  }

  return ptr(fpositer, beginIndex, endIndex);
}
UEnumeration * ucal_openTimeZoneIDEnumeration(USystemTimeZoneType zoneType, const char * region, const int32_t * rawOffset, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(USystemTimeZoneType zoneType, const char * region, const int32_t * rawOffset, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_openTimeZoneIDEnumeration") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_openTimeZoneIDEnumeration", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_openTimeZoneIDEnumeration");
    abort();
  }

  return ptr(zoneType, region, rawOffset, ec);
}
UEnumeration * ucal_openTimeZones(UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_openTimeZones") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_openTimeZones", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_openTimeZones");
    abort();
  }

  return ptr(ec);
}
UEnumeration * ucal_openCountryTimeZones(const char * country, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * country, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_openCountryTimeZones") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_openCountryTimeZones", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_openCountryTimeZones");
    abort();
  }

  return ptr(country, ec);
}
int32_t ucal_getDefaultTimeZone(UChar * result, int32_t resultCapacity, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UChar * result, int32_t resultCapacity, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getDefaultTimeZone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getDefaultTimeZone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getDefaultTimeZone");
    abort();
  }

  return ptr(result, resultCapacity, ec);
}
void ucal_setDefaultTimeZone(const UChar * zoneID, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const UChar * zoneID, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setDefaultTimeZone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setDefaultTimeZone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setDefaultTimeZone");
    abort();
  }

  ptr(zoneID, ec);
}
int32_t ucal_getDSTSavings(const UChar * zoneID, UErrorCode * ec) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * zoneID, UErrorCode * ec);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getDSTSavings") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getDSTSavings", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getDSTSavings");
    abort();
  }

  return ptr(zoneID, ec);
}
UDate ucal_getNow() {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getNow") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getNow", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getNow");
    abort();
  }

  return ptr();
}
UCalendar * ucal_open(const UChar * zoneID, int32_t len, const char * locale, UCalendarType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCalendar * (*FuncPtr)(const UChar * zoneID, int32_t len, const char * locale, UCalendarType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_open");
    abort();
  }

  return ptr(zoneID, len, locale, type, status);
}
void ucal_close(UCalendar * cal) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_close");
    abort();
  }

  ptr(cal);
}
UCalendar * ucal_clone(const UCalendar * cal, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCalendar * (*FuncPtr)(const UCalendar * cal, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_clone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_clone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_clone");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setTimeZone(UCalendar * cal, const UChar * zoneID, int32_t len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, const UChar * zoneID, int32_t len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setTimeZone") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setTimeZone", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setTimeZone");
    abort();
  }

  ptr(cal, zoneID, len, status);
}
int32_t ucal_getTimeZoneID(const UCalendar * cal, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getTimeZoneID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getTimeZoneID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getTimeZoneID");
    abort();
  }

  return ptr(cal, result, resultLength, status);
}
int32_t ucal_getTimeZoneDisplayName(const UCalendar * cal, UCalendarDisplayNameType type, const char * locale, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UCalendarDisplayNameType type, const char * locale, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getTimeZoneDisplayName") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getTimeZoneDisplayName", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getTimeZoneDisplayName");
    abort();
  }

  return ptr(cal, type, locale, result, resultLength, status);
}
UBool ucal_inDaylightTime(const UCalendar * cal, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCalendar * cal, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_inDaylightTime") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_inDaylightTime", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_inDaylightTime");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setGregorianChange(UCalendar * cal, UDate date, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UDate date, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setGregorianChange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setGregorianChange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setGregorianChange");
    abort();
  }

  ptr(cal, date, pErrorCode);
}
UDate ucal_getGregorianChange(const UCalendar * cal, UErrorCode * pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)(const UCalendar * cal, UErrorCode * pErrorCode);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getGregorianChange") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getGregorianChange", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getGregorianChange");
    abort();
  }

  return ptr(cal, pErrorCode);
}
int32_t ucal_getAttribute(const UCalendar * cal, UCalendarAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UCalendarAttribute attr);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getAttribute");
    abort();
  }

  return ptr(cal, attr);
}
void ucal_setAttribute(UCalendar * cal, UCalendarAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UCalendarAttribute attr, int32_t newValue);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setAttribute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setAttribute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setAttribute");
    abort();
  }

  ptr(cal, attr, newValue);
}
const char * ucal_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(int32_t localeIndex);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t ucal_countAvailable() {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)();
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_countAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_countAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_countAvailable");
    abort();
  }

  return ptr();
}
UDate ucal_getMillis(const UCalendar * cal, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)(const UCalendar * cal, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getMillis") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getMillis", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getMillis");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setMillis(UCalendar * cal, UDate dateTime, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UDate dateTime, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setMillis") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setMillis", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setMillis");
    abort();
  }

  ptr(cal, dateTime, status);
}
void ucal_setDate(UCalendar * cal, int32_t year, int32_t month, int32_t date, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, int32_t year, int32_t month, int32_t date, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setDate") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setDate", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setDate");
    abort();
  }

  ptr(cal, year, month, date, status);
}
void ucal_setDateTime(UCalendar * cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_setDateTime") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_setDateTime", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_setDateTime");
    abort();
  }

  ptr(cal, year, month, date, hour, minute, second, status);
}
UBool ucal_equivalentTo(const UCalendar * cal1, const UCalendar * cal2) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCalendar * cal1, const UCalendar * cal2);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_equivalentTo") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_equivalentTo", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_equivalentTo");
    abort();
  }

  return ptr(cal1, cal2);
}
void ucal_add(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_add") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_add", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_add");
    abort();
  }

  ptr(cal, field, amount, status);
}
void ucal_roll(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_roll") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_roll", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_roll");
    abort();
  }

  ptr(cal, field, amount, status);
}
int32_t ucal_get(const UCalendar * cal, UCalendarDateFields field, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UCalendarDateFields field, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_get") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_get", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_get");
    abort();
  }

  return ptr(cal, field, status);
}
void ucal_set(UCalendar * cal, UCalendarDateFields field, int32_t value) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UCalendarDateFields field, int32_t value);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_set") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_set", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_set");
    abort();
  }

  ptr(cal, field, value);
}
UBool ucal_isSet(const UCalendar * cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCalendar * cal, UCalendarDateFields field);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_isSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_isSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_isSet");
    abort();
  }

  return ptr(cal, field);
}
void ucal_clearField(UCalendar * cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * cal, UCalendarDateFields field);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_clearField") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_clearField", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_clearField");
    abort();
  }

  ptr(cal, field);
}
void ucal_clear(UCalendar * calendar) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UCalendar * calendar);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_clear") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_clear", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_clear");
    abort();
  }

  ptr(calendar);
}
int32_t ucal_getLimit(const UCalendar * cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getLimit") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getLimit", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getLimit");
    abort();
  }

  return ptr(cal, field, type, status);
}
const char * ucal_getLocaleByType(const UCalendar * cal, ULocDataLocaleType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCalendar * cal, ULocDataLocaleType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getLocaleByType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getLocaleByType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getLocaleByType");
    abort();
  }

  return ptr(cal, type, status);
}
const char * ucal_getTZDataVersion(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getTZDataVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getTZDataVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getTZDataVersion");
    abort();
  }

  return ptr(status);
}
int32_t ucal_getCanonicalTimeZoneID(const UChar * id, int32_t len, UChar * result, int32_t resultCapacity, UBool * isSystemID, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * id, int32_t len, UChar * result, int32_t resultCapacity, UBool * isSystemID, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getCanonicalTimeZoneID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getCanonicalTimeZoneID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getCanonicalTimeZoneID");
    abort();
  }

  return ptr(id, len, result, resultCapacity, isSystemID, status);
}
const char * ucal_getType(const UCalendar * cal, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const UCalendar * cal, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getType");
    abort();
  }

  return ptr(cal, status);
}
UEnumeration * ucal_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getKeywordValuesForLocale") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getKeywordValuesForLocale", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
UCalendarWeekdayType ucal_getDayOfWeekType(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UCalendarWeekdayType (*FuncPtr)(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getDayOfWeekType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getDayOfWeekType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getDayOfWeekType");
    abort();
  }

  return ptr(cal, dayOfWeek, status);
}
int32_t ucal_getWeekendTransition(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getWeekendTransition") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getWeekendTransition", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getWeekendTransition");
    abort();
  }

  return ptr(cal, dayOfWeek, status);
}
UBool ucal_isWeekend(const UCalendar * cal, UDate date, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCalendar * cal, UDate date, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_isWeekend") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_isWeekend", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_isWeekend");
    abort();
  }

  return ptr(cal, date, status);
}
int32_t ucal_getFieldDifference(UCalendar * cal, UDate target, UCalendarDateFields field, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UCalendar * cal, UDate target, UCalendarDateFields field, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getFieldDifference") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getFieldDifference", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getFieldDifference");
    abort();
  }

  return ptr(cal, target, field, status);
}
UBool ucal_getTimeZoneTransitionDate(const UCalendar * cal, UTimeZoneTransitionType type, UDate * transition, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UCalendar * cal, UTimeZoneTransitionType type, UDate * transition, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getTimeZoneTransitionDate") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getTimeZoneTransitionDate", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getTimeZoneTransitionDate");
    abort();
  }

  return ptr(cal, type, transition, status);
}
int32_t ucal_getWindowsTimeZoneID(const UChar * id, int32_t len, UChar * winid, int32_t winidCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * id, int32_t len, UChar * winid, int32_t winidCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getWindowsTimeZoneID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getWindowsTimeZoneID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getWindowsTimeZoneID");
    abort();
  }

  return ptr(id, len, winid, winidCapacity, status);
}
int32_t ucal_getTimeZoneIDForWindowsID(const UChar * winid, int32_t len, const char * region, UChar * id, int32_t idCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UChar * winid, int32_t len, const char * region, UChar * id, int32_t idCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ucal_getTimeZoneIDForWindowsID") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ucal_getTimeZoneIDForWindowsID", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ucal_getTimeZoneIDForWindowsID");
    abort();
  }

  return ptr(winid, len, region, id, idCapacity, status);
}
UDateIntervalFormat * udtitvfmt_open(const char * locale, const UChar * skeleton, int32_t skeletonLength, const UChar * tzID, int32_t tzIDLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDateIntervalFormat * (*FuncPtr)(const char * locale, const UChar * skeleton, int32_t skeletonLength, const UChar * tzID, int32_t tzIDLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udtitvfmt_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udtitvfmt_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udtitvfmt_open");
    abort();
  }

  return ptr(locale, skeleton, skeletonLength, tzID, tzIDLength, status);
}
void udtitvfmt_close(UDateIntervalFormat * formatter) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UDateIntervalFormat * formatter);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udtitvfmt_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udtitvfmt_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udtitvfmt_close");
    abort();
  }

  ptr(formatter);
}
int32_t udtitvfmt_format(const UDateIntervalFormat * formatter, UDate fromDate, UDate toDate, UChar * result, int32_t resultCapacity, UFieldPosition * position, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UDateIntervalFormat * formatter, UDate fromDate, UDate toDate, UChar * result, int32_t resultCapacity, UFieldPosition * position, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("udtitvfmt_format") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "udtitvfmt_format", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "udtitvfmt_format");
    abort();
  }

  return ptr(formatter, fromDate, toDate, result, resultCapacity, position, status);
}
ULocaleData * ulocdata_open(const char * localeID, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef ULocaleData * (*FuncPtr)(const char * localeID, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_open");
    abort();
  }

  return ptr(localeID, status);
}
void ulocdata_close(ULocaleData * uld) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(ULocaleData * uld);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_close");
    abort();
  }

  ptr(uld);
}
void ulocdata_setNoSubstitute(ULocaleData * uld, UBool setting) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(ULocaleData * uld, UBool setting);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_setNoSubstitute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_setNoSubstitute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_setNoSubstitute");
    abort();
  }

  ptr(uld, setting);
}
UBool ulocdata_getNoSubstitute(ULocaleData * uld) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(ULocaleData * uld);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getNoSubstitute") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getNoSubstitute", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getNoSubstitute");
    abort();
  }

  return ptr(uld);
}
USet * ulocdata_getExemplarSet(ULocaleData * uld, USet * fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef USet * (*FuncPtr)(ULocaleData * uld, USet * fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getExemplarSet") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getExemplarSet", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getExemplarSet");
    abort();
  }

  return ptr(uld, fillIn, options, extype, status);
}
int32_t ulocdata_getDelimiter(ULocaleData * uld, ULocaleDataDelimiterType type, UChar * result, int32_t resultLength, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(ULocaleData * uld, ULocaleDataDelimiterType type, UChar * result, int32_t resultLength, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getDelimiter") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getDelimiter", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getDelimiter");
    abort();
  }

  return ptr(uld, type, result, resultLength, status);
}
UMeasurementSystem ulocdata_getMeasurementSystem(const char * localeID, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UMeasurementSystem (*FuncPtr)(const char * localeID, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getMeasurementSystem") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getMeasurementSystem", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getMeasurementSystem");
    abort();
  }

  return ptr(localeID, status);
}
void ulocdata_getPaperSize(const char * localeID, int32_t * height, int32_t * width, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(const char * localeID, int32_t * height, int32_t * width, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getPaperSize") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getPaperSize", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getPaperSize");
    abort();
  }

  ptr(localeID, height, width, status);
}
void ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UVersionInfo versionArray, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getCLDRVersion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getCLDRVersion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getCLDRVersion");
    abort();
  }

  ptr(versionArray, status);
}
int32_t ulocdata_getLocaleDisplayPattern(ULocaleData * uld, UChar * pattern, int32_t patternCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(ULocaleData * uld, UChar * pattern, int32_t patternCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getLocaleDisplayPattern") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getLocaleDisplayPattern", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getLocaleDisplayPattern");
    abort();
  }

  return ptr(uld, pattern, patternCapacity, status);
}
int32_t ulocdata_getLocaleSeparator(ULocaleData * uld, UChar * separator, int32_t separatorCapacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(ULocaleData * uld, UChar * separator, int32_t separatorCapacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ulocdata_getLocaleSeparator") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ulocdata_getLocaleSeparator", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ulocdata_getLocaleSeparator");
    abort();
  }

  return ptr(uld, separator, separatorCapacity, status);
}
UFormattable * ufmt_open(UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UFormattable * (*FuncPtr)(UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_open");
    abort();
  }

  return ptr(status);
}
void ufmt_close(UFormattable * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UFormattable * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_close");
    abort();
  }

  ptr(fmt);
}
UFormattableType ufmt_getType(const UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UFormattableType (*FuncPtr)(const UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getType");
    abort();
  }

  return ptr(fmt, status);
}
UBool ufmt_isNumeric(const UFormattable * fmt) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const UFormattable * fmt);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_isNumeric") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_isNumeric", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_isNumeric");
    abort();
  }

  return ptr(fmt);
}
UDate ufmt_getDate(const UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UDate (*FuncPtr)(const UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getDate") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getDate", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getDate");
    abort();
  }

  return ptr(fmt, status);
}
double ufmt_getDouble(UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef double (*FuncPtr)(UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getDouble") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getDouble", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getDouble");
    abort();
  }

  return ptr(fmt, status);
}
int32_t ufmt_getLong(UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getLong") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getLong", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getLong");
    abort();
  }

  return ptr(fmt, status);
}
int64_t ufmt_getInt64(UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int64_t (*FuncPtr)(UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getInt64") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getInt64", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getInt64");
    abort();
  }

  return ptr(fmt, status);
}
const void * ufmt_getObject(const UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const void * (*FuncPtr)(const UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getObject") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getObject", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getObject");
    abort();
  }

  return ptr(fmt, status);
}
const UChar * ufmt_getUChars(UFormattable * fmt, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const UChar * (*FuncPtr)(UFormattable * fmt, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getUChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getUChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getUChars");
    abort();
  }

  return ptr(fmt, len, status);
}
int32_t ufmt_getArrayLength(const UFormattable * fmt, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UFormattable * fmt, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getArrayLength") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getArrayLength", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getArrayLength");
    abort();
  }

  return ptr(fmt, status);
}
UFormattable * ufmt_getArrayItemByIndex(UFormattable * fmt, int32_t n, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UFormattable * (*FuncPtr)(UFormattable * fmt, int32_t n, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getArrayItemByIndex") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getArrayItemByIndex", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getArrayItemByIndex");
    abort();
  }

  return ptr(fmt, n, status);
}
const char * ufmt_getDecNumChars(UFormattable * fmt, int32_t * len, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(UFormattable * fmt, int32_t * len, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("ufmt_getDecNumChars") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "ufmt_getDecNumChars", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "ufmt_getDecNumChars");
    abort();
  }

  return ptr(fmt, len, status);
}
const URegion * uregion_getRegionFromCode(const char * regionCode, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const URegion * (*FuncPtr)(const char * regionCode, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getRegionFromCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getRegionFromCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getRegionFromCode");
    abort();
  }

  return ptr(regionCode, status);
}
const URegion * uregion_getRegionFromNumericCode(int32_t code, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const URegion * (*FuncPtr)(int32_t code, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getRegionFromNumericCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getRegionFromNumericCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getRegionFromNumericCode");
    abort();
  }

  return ptr(code, status);
}
UEnumeration * uregion_getAvailable(URegionType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(URegionType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getAvailable") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getAvailable", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getAvailable");
    abort();
  }

  return ptr(type, status);
}
UBool uregion_areEqual(const URegion * uregion, const URegion * otherRegion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegion * uregion, const URegion * otherRegion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_areEqual") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_areEqual", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_areEqual");
    abort();
  }

  return ptr(uregion, otherRegion);
}
const URegion * uregion_getContainingRegion(const URegion * uregion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const URegion * (*FuncPtr)(const URegion * uregion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getContainingRegion") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getContainingRegion", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getContainingRegion");
    abort();
  }

  return ptr(uregion);
}
const URegion * uregion_getContainingRegionOfType(const URegion * uregion, URegionType type) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const URegion * (*FuncPtr)(const URegion * uregion, URegionType type);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getContainingRegionOfType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getContainingRegionOfType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getContainingRegionOfType");
    abort();
  }

  return ptr(uregion, type);
}
UEnumeration * uregion_getContainedRegions(const URegion * uregion, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const URegion * uregion, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getContainedRegions") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getContainedRegions", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getContainedRegions");
    abort();
  }

  return ptr(uregion, status);
}
UEnumeration * uregion_getContainedRegionsOfType(const URegion * uregion, URegionType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const URegion * uregion, URegionType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getContainedRegionsOfType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getContainedRegionsOfType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getContainedRegionsOfType");
    abort();
  }

  return ptr(uregion, type, status);
}
UBool uregion_contains(const URegion * uregion, const URegion * otherRegion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UBool (*FuncPtr)(const URegion * uregion, const URegion * otherRegion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_contains") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_contains", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_contains");
    abort();
  }

  return ptr(uregion, otherRegion);
}
UEnumeration * uregion_getPreferredValues(const URegion * uregion, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UEnumeration * (*FuncPtr)(const URegion * uregion, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getPreferredValues") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getPreferredValues", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getPreferredValues");
    abort();
  }

  return ptr(uregion, status);
}
const char * uregion_getRegionCode(const URegion * uregion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef const char * (*FuncPtr)(const URegion * uregion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getRegionCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getRegionCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getRegionCode");
    abort();
  }

  return ptr(uregion);
}
int32_t uregion_getNumericCode(const URegion * uregion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const URegion * uregion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getNumericCode") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getNumericCode", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getNumericCode");
    abort();
  }

  return ptr(uregion);
}
URegionType uregion_getType(const URegion * uregion) {
  pthread_once(&once_control, &init_icudata_version);

  typedef URegionType (*FuncPtr)(const URegion * uregion);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uregion_getType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uregion_getType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uregion_getType");
    abort();
  }

  return ptr(uregion);
}
UPluralRules * uplrules_open(const char * locale, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UPluralRules * (*FuncPtr)(const char * locale, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uplrules_open") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uplrules_open", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uplrules_open");
    abort();
  }

  return ptr(locale, status);
}
UPluralRules * uplrules_openForType(const char * locale, UPluralType type, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef UPluralRules * (*FuncPtr)(const char * locale, UPluralType type, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uplrules_openForType") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uplrules_openForType", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uplrules_openForType");
    abort();
  }

  return ptr(locale, type, status);
}
void uplrules_close(UPluralRules * uplrules) {
  pthread_once(&once_control, &init_icudata_version);

  typedef void (*FuncPtr)(UPluralRules * uplrules);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uplrules_close") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uplrules_close", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uplrules_close");
    abort();
  }

  ptr(uplrules);
}
int32_t uplrules_select(const UPluralRules * uplrules, double number, UChar * keyword, int32_t capacity, UErrorCode * status) {
  pthread_once(&once_control, &init_icudata_version);

  typedef int32_t (*FuncPtr)(const UPluralRules * uplrules, double number, UChar * keyword, int32_t capacity, UErrorCode * status);
  static FuncPtr ptr = nullptr;
  static bool initialized = false;
  if (!initialized) {
    char versioned_symbol_name[strlen("uplrules_select") +
                               sizeof(g_icudata_version)];
    snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s",
             "uplrules_select", g_icudata_version);

    ptr =
        reinterpret_cast<FuncPtr>(dlsym(handle_i18n, versioned_symbol_name));
    initialized = true;
  }

  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        kUnavailableFunctionErrorFmt, "uplrules_select");
    abort();
  }

  return ptr(uplrules, number, keyword, capacity, status);
}

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

#include <unicode/ucurr.h>
#include <unicode/udat.h>
#include <unicode/unum.h>
#include <unicode/ucoleitr.h>
#include <unicode/utrans.h>
#include <unicode/usearch.h>
#include <unicode/umsg.h>
#include <unicode/ulocdata.h>
#include <unicode/ucol.h>
#include <unicode/utmscale.h>
#include <unicode/uregex.h>
#include <unicode/uspoof.h>
#include <unicode/udatpg.h>
#include <unicode/ucsdet.h>
#include <unicode/ucal.h>
#include <unicode/ucnv_err.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#include <unicode/uiter.h>
#include <unicode/utypes.h>
#include <unicode/udata.h>
#include <unicode/ucasemap.h>
#include <unicode/ucnv_cb.h>
#include <unicode/uset.h>
#include <unicode/utext.h>
#include <unicode/ures.h>
#include <unicode/ubidi.h>
#include <unicode/uclean.h>
#include <unicode/uscript.h>
#include <unicode/ushape.h>
#include <unicode/utrace.h>
#include <unicode/putil.h>
#include <unicode/unorm.h>
#include <unicode/ubrk.h>
#include <unicode/uloc.h>
#include <unicode/ucat.h>
#include <unicode/ucnvsel.h>
#include <unicode/usprep.h>
#include <unicode/uversion.h>
#include <unicode/uidna.h>
#include <unicode/uenum.h>

// Allowed version number ranges between [10, 999]
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

static void* handle_i18n = NULL;
static void* handle_common = NULL;

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
      handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
      handle_common = dlopen("libicuuc.so", RTLD_LOCAL);
    }
  }
}


/* unicode/ucurr.h */

int32_t ucurr_forLocale_android(const char* locale, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_forLocale");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, buff, buffCapacity, ec);
}

UCurrRegistryKey ucurr_register_android(const UChar* isoCode, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCurrRegistryKey (*ptr)(const UChar*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_register");
  strcat(func_name, icudata_version);
  ptr = (UCurrRegistryKey(*)(const UChar*, const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(isoCode, locale, status);
}

UBool ucurr_unregister_android(UCurrRegistryKey key, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UCurrRegistryKey, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_unregister");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UCurrRegistryKey, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(key, status);
}

const UChar * ucurr_getName_android(const UChar* currency, const char* locale, UCurrNameStyle nameStyle, UBool* isChoiceFormat, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_getName");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}

const UChar * ucurr_getPluralName_android(const UChar* currency, const char* locale, UBool* isChoiceFormat, const char* pluralCount, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_getPluralName");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}

int32_t ucurr_getDefaultFractionDigits_android(const UChar* currency, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_getDefaultFractionDigits");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(currency, ec);
}

double ucurr_getRoundingIncrement_android(const UChar* currency, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_getRoundingIncrement");
  strcat(func_name, icudata_version);
  ptr = (double(*)(const UChar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(currency, ec);
}

UEnumeration * ucurr_openISOCurrencies_android(uint32_t currType, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_openISOCurrencies");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(uint32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(currType, pErrorCode);
}

int32_t ucurr_countCurrencies_android(const char* locale, UDate date, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UDate, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_countCurrencies");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, UDate, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, date, ec);
}

int32_t ucurr_forLocaleAndDate_android(const char* locale, UDate date, int32_t index, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_forLocaleAndDate");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, date, index, buff, buffCapacity, ec);
}

UEnumeration * ucurr_getKeywordValuesForLocale_android(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, const char*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucurr_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, const char*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(key, locale, commonlyUsed, status);
}

/* unicode/udat.h */

UDateFormat * udat_open_android(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char* locale, const UChar* tzID, int32_t tzIDLength, const UChar* pattern, int32_t patternLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDateFormat * (*ptr)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_open");
  strcat(func_name, icudata_version);
  ptr = (UDateFormat *(*)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(timeStyle, dateStyle, locale, tzID, tzIDLength, pattern, patternLength, status);
}

void udat_close_android(UDateFormat* format) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*);
  char func_name[128];
  strcpy(func_name, "udat_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*))dlsym(handle_i18n, func_name);
  ptr(format);
  return;
}

UDateFormat * udat_clone_android(const UDateFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDateFormat * (*ptr)(const UDateFormat*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_clone");
  strcat(func_name, icudata_version);
  ptr = (UDateFormat *(*)(const UDateFormat*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, status);
}

int32_t udat_format_android(const UDateFormat* format, UDate dateToFormat, UChar* result, int32_t resultLength, UFieldPosition* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_format");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(format, dateToFormat, result, resultLength, position, status);
}

UDate udat_parse_android(const UDateFormat* format, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_parse");
  strcat(func_name, icudata_version);
  ptr = (UDate(*)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(format, text, textLength, parsePos, status);
}

void udat_parseCalendar_android(const UDateFormat* format, UCalendar* calendar, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_parseCalendar");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(format, calendar, text, textLength, parsePos, status);
  return;
}

UBool udat_isLenient_android(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UDateFormat*);
  char func_name[128];
  strcpy(func_name, "udat_isLenient");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UDateFormat*))dlsym(handle_i18n, func_name);
  return ptr(fmt);
}

void udat_setLenient_android(UDateFormat* fmt, UBool isLenient) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UBool);
  char func_name[128];
  strcpy(func_name, "udat_setLenient");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, UBool))dlsym(handle_i18n, func_name);
  ptr(fmt, isLenient);
  return;
}

const UCalendar * udat_getCalendar_android(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const UCalendar * (*ptr)(const UDateFormat*);
  char func_name[128];
  strcpy(func_name, "udat_getCalendar");
  strcat(func_name, icudata_version);
  ptr = (const UCalendar *(*)(const UDateFormat*))dlsym(handle_i18n, func_name);
  return ptr(fmt);
}

void udat_setCalendar_android(UDateFormat* fmt, const UCalendar* calendarToSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, const UCalendar*);
  char func_name[128];
  strcpy(func_name, "udat_setCalendar");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, const UCalendar*))dlsym(handle_i18n, func_name);
  ptr(fmt, calendarToSet);
  return;
}

const UNumberFormat * udat_getNumberFormat_android(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const UNumberFormat * (*ptr)(const UDateFormat*);
  char func_name[128];
  strcpy(func_name, "udat_getNumberFormat");
  strcat(func_name, icudata_version);
  ptr = (const UNumberFormat *(*)(const UDateFormat*))dlsym(handle_i18n, func_name);
  return ptr(fmt);
}

void udat_setNumberFormat_android(UDateFormat* fmt, const UNumberFormat* numberFormatToSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, const UNumberFormat*);
  char func_name[128];
  strcpy(func_name, "udat_setNumberFormat");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, const UNumberFormat*))dlsym(handle_i18n, func_name);
  ptr(fmt, numberFormatToSet);
  return;
}

const char * udat_getAvailable_android(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "udat_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(localeIndex);
}

int32_t udat_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "udat_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

UDate udat_get2DigitYearStart_android(const UDateFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UDateFormat*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_get2DigitYearStart");
  strcat(func_name, icudata_version);
  ptr = (UDate(*)(const UDateFormat*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, status);
}

void udat_set2DigitYearStart_android(UDateFormat* fmt, UDate d, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UDate, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_set2DigitYearStart");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, UDate, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(fmt, d, status);
  return;
}

int32_t udat_toPattern_android(const UDateFormat* fmt, UBool localized, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_toPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, localized, result, resultLength, status);
}

void udat_applyPattern_android(UDateFormat* format, UBool localized, const UChar* pattern, int32_t patternLength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UBool, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "udat_applyPattern");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, UBool, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(format, localized, pattern, patternLength);
  return;
}

int32_t udat_getSymbols_android(const UDateFormat* fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_getSymbols");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, type, symbolIndex, result, resultLength, status);
}

int32_t udat_countSymbols_android(const UDateFormat* fmt, UDateFormatSymbolType type) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType);
  char func_name[128];
  strcpy(func_name, "udat_countSymbols");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType))dlsym(handle_i18n, func_name);
  return ptr(fmt, type);
}

void udat_setSymbols_android(UDateFormat* format, UDateFormatSymbolType type, int32_t symbolIndex, UChar* value, int32_t valueLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_setSymbols");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(format, type, symbolIndex, value, valueLength, status);
  return;
}

const char * udat_getLocaleByType_android(const UDateFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UDateFormat*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udat_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UDateFormat*, ULocDataLocaleType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, type, status);
}

/* unicode/unum.h */

UNumberFormat * unum_open_android(UNumberFormatStyle style, const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseErr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberFormat * (*ptr)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_open");
  strcat(func_name, icudata_version);
  ptr = (UNumberFormat *(*)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(style, pattern, patternLength, locale, parseErr, status);
}

void unum_close_android(UNumberFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*);
  char func_name[128];
  strcpy(func_name, "unum_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*))dlsym(handle_i18n, func_name);
  ptr(fmt);
  return;
}

UNumberFormat * unum_clone_android(const UNumberFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberFormat * (*ptr)(const UNumberFormat*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_clone");
  strcat(func_name, icudata_version);
  ptr = (UNumberFormat *(*)(const UNumberFormat*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, status);
}

int32_t unum_format_android(const UNumberFormat* fmt, int32_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_format");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatInt64_android(const UNumberFormat* fmt, int64_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_formatInt64");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDouble_android(const UNumberFormat* fmt, double number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_formatDouble");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDoubleCurrency_android(const UNumberFormat* fmt, double number, UChar* currency, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_formatDoubleCurrency");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, number, currency, result, resultLength, pos, status);
}

int32_t unum_parse_android(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_parse");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, text, textLength, parsePos, status);
}

int64_t unum_parseInt64_android(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_parseInt64");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, text, textLength, parsePos, status);
}

double unum_parseDouble_android(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_parseDouble");
  strcat(func_name, icudata_version);
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, text, textLength, parsePos, status);
}

double unum_parseDoubleCurrency_android(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UChar* currency, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_parseDoubleCurrency");
  strcat(func_name, icudata_version);
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, text, textLength, parsePos, currency, status);
}

void unum_applyPattern_android(UNumberFormat* format, UBool localized, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_applyPattern");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(format, localized, pattern, patternLength, parseError, status);
  return;
}

const char * unum_getAvailable_android(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "unum_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(localeIndex);
}

int32_t unum_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "unum_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

int32_t unum_getAttribute_android(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  char func_name[128];
  strcpy(func_name, "unum_getAttribute");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatAttribute))dlsym(handle_i18n, func_name);
  return ptr(fmt, attr);
}

void unum_setAttribute_android(UNumberFormat* fmt, UNumberFormatAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, int32_t);
  char func_name[128];
  strcpy(func_name, "unum_setAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, int32_t))dlsym(handle_i18n, func_name);
  ptr(fmt, attr, newValue);
  return;
}

double unum_getDoubleAttribute_android(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  char func_name[128];
  strcpy(func_name, "unum_getDoubleAttribute");
  strcat(func_name, icudata_version);
  ptr = (double(*)(const UNumberFormat*, UNumberFormatAttribute))dlsym(handle_i18n, func_name);
  return ptr(fmt, attr);
}

void unum_setDoubleAttribute_android(UNumberFormat* fmt, UNumberFormatAttribute attr, double newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, double);
  char func_name[128];
  strcpy(func_name, "unum_setDoubleAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, double))dlsym(handle_i18n, func_name);
  ptr(fmt, attr, newValue);
  return;
}

int32_t unum_getTextAttribute_android(const UNumberFormat* fmt, UNumberFormatTextAttribute tag, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_getTextAttribute");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, tag, result, resultLength, status);
}

void unum_setTextAttribute_android(UNumberFormat* fmt, UNumberFormatTextAttribute tag, const UChar* newValue, int32_t newValueLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_setTextAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(fmt, tag, newValue, newValueLength, status);
  return;
}

int32_t unum_toPattern_android(const UNumberFormat* fmt, UBool isPatternLocalized, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_toPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}

int32_t unum_getSymbol_android(const UNumberFormat* fmt, UNumberFormatSymbol symbol, UChar* buffer, int32_t size, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_getSymbol");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, symbol, buffer, size, status);
}

void unum_setSymbol_android(UNumberFormat* fmt, UNumberFormatSymbol symbol, const UChar* value, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_setSymbol");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(fmt, symbol, value, length, status);
  return;
}

const char * unum_getLocaleByType_android(const UNumberFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unum_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, type, status);
}

/* unicode/ucoleitr.h */

UCollationElements * ucol_openElements_android(const UCollator* coll, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationElements * (*ptr)(const UCollator*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_openElements");
  strcat(func_name, icudata_version);
  ptr = (UCollationElements *(*)(const UCollator*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, text, textLength, status);
}

int32_t ucol_keyHashCode_android(const uint8_t* key, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_keyHashCode");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const uint8_t*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(key, length);
}

void ucol_closeElements_android(UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*);
  char func_name[128];
  strcpy(func_name, "ucol_closeElements");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollationElements*))dlsym(handle_i18n, func_name);
  ptr(elems);
  return;
}

void ucol_reset_android(UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*);
  char func_name[128];
  strcpy(func_name, "ucol_reset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollationElements*))dlsym(handle_i18n, func_name);
  ptr(elems);
  return;
}

int32_t ucol_next_android(UCollationElements* elems, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_next");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(elems, status);
}

int32_t ucol_previous_android(UCollationElements* elems, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_previous");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(elems, status);
}

int32_t ucol_getMaxExpansion_android(const UCollationElements* elems, int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollationElements*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_getMaxExpansion");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollationElements*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(elems, order);
}

void ucol_setText_android(UCollationElements* elems, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_setText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollationElements*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(elems, text, textLength, status);
  return;
}

int32_t ucol_getOffset_android(const UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollationElements*);
  char func_name[128];
  strcpy(func_name, "ucol_getOffset");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollationElements*))dlsym(handle_i18n, func_name);
  return ptr(elems);
}

void ucol_setOffset_android(UCollationElements* elems, int32_t offset, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_setOffset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollationElements*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(elems, offset, status);
  return;
}

int32_t ucol_primaryOrder_android(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_primaryOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(order);
}

int32_t ucol_secondaryOrder_android(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_secondaryOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(order);
}

int32_t ucol_tertiaryOrder_android(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_tertiaryOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(order);
}

/* unicode/utrans.h */

UTransliterator * utrans_openU_android(const UChar* id, int32_t idLength, UTransDirection dir, const UChar* rules, int32_t rulesLength, UParseError* parseError, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator * (*ptr)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_openU");
  strcat(func_name, icudata_version);
  ptr = (UTransliterator *(*)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}

UTransliterator * utrans_openInverse_android(const UTransliterator* trans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator * (*ptr)(const UTransliterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_openInverse");
  strcat(func_name, icudata_version);
  ptr = (UTransliterator *(*)(const UTransliterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(trans, status);
}

UTransliterator * utrans_clone_android(const UTransliterator* trans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator * (*ptr)(const UTransliterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_clone");
  strcat(func_name, icudata_version);
  ptr = (UTransliterator *(*)(const UTransliterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(trans, status);
}

void utrans_close_android(UTransliterator* trans) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*);
  char func_name[128];
  strcpy(func_name, "utrans_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UTransliterator*))dlsym(handle_i18n, func_name);
  ptr(trans);
  return;
}

const UChar * utrans_getUnicodeID_android(const UTransliterator* trans, int32_t* resultLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UTransliterator*, int32_t*);
  char func_name[128];
  strcpy(func_name, "utrans_getUnicodeID");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UTransliterator*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(trans, resultLength);
}

void utrans_register_android(UTransliterator* adoptedTrans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_register");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UTransliterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(adoptedTrans, status);
  return;
}

void utrans_unregisterID_android(const UChar* id, int32_t idLength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "utrans_unregisterID");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(id, idLength);
  return;
}

void utrans_setFilter_android(UTransliterator* trans, const UChar* filterPattern, int32_t filterPatternLen, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_setFilter");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UTransliterator*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(trans, filterPattern, filterPatternLen, status);
  return;
}

int32_t utrans_countAvailableIDs_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "utrans_countAvailableIDs");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

UEnumeration * utrans_openIDs_android(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_openIDs");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pErrorCode);
}

void utrans_trans_android(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, int32_t start, int32_t* limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_trans");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(trans, rep, repFunc, start, limit, status);
  return;
}

void utrans_transIncremental_android(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, UTransPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_transIncremental");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(trans, rep, repFunc, pos, status);
  return;
}

void utrans_transUChars_android(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, int32_t start, int32_t* limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_transUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(trans, text, textLength, textCapacity, start, limit, status);
  return;
}

void utrans_transIncrementalUChars_android(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, UTransPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utrans_transIncrementalUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(trans, text, textLength, textCapacity, pos, status);
  return;
}

/* unicode/usearch.h */

UStringSearch * usearch_open_android(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const char* locale, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringSearch * (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_open");
  strcat(func_name, icudata_version);
  ptr = (UStringSearch *(*)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}

UStringSearch * usearch_openFromCollator_android(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const UCollator* collator, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringSearch * (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_openFromCollator");
  strcat(func_name, icudata_version);
  ptr = (UStringSearch *(*)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}

void usearch_close_android(UStringSearch* searchiter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*))dlsym(handle_i18n, func_name);
  ptr(searchiter);
  return;
}

void usearch_setOffset_android(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setOffset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, position, status);
  return;
}

int32_t usearch_getOffset_android(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_getOffset");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UStringSearch*))dlsym(handle_i18n, func_name);
  return ptr(strsrch);
}

void usearch_setAttribute_android(UStringSearch* strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, attribute, value, status);
  return;
}

USearchAttributeValue usearch_getAttribute_android(const UStringSearch* strsrch, USearchAttribute attribute) {
  pthread_once(&once_control, &init_icudata_version);
  USearchAttributeValue (*ptr)(const UStringSearch*, USearchAttribute);
  char func_name[128];
  strcpy(func_name, "usearch_getAttribute");
  strcat(func_name, icudata_version);
  ptr = (USearchAttributeValue(*)(const UStringSearch*, USearchAttribute))dlsym(handle_i18n, func_name);
  return ptr(strsrch, attribute);
}

int32_t usearch_getMatchedStart_android(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_getMatchedStart");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UStringSearch*))dlsym(handle_i18n, func_name);
  return ptr(strsrch);
}

int32_t usearch_getMatchedLength_android(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_getMatchedLength");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UStringSearch*))dlsym(handle_i18n, func_name);
  return ptr(strsrch);
}

int32_t usearch_getMatchedText_android(const UStringSearch* strsrch, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_getMatchedText");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UStringSearch*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, result, resultCapacity, status);
}

void usearch_setBreakIterator_android(UStringSearch* strsrch, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, UBreakIterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setBreakIterator");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, UBreakIterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, breakiter, status);
  return;
}

const UBreakIterator * usearch_getBreakIterator_android(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  const UBreakIterator * (*ptr)(const UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_getBreakIterator");
  strcat(func_name, icudata_version);
  ptr = (const UBreakIterator *(*)(const UStringSearch*))dlsym(handle_i18n, func_name);
  return ptr(strsrch);
}

void usearch_setText_android(UStringSearch* strsrch, const UChar* text, int32_t textlength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, text, textlength, status);
  return;
}

const UChar * usearch_getText_android(const UStringSearch* strsrch, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UStringSearch*, int32_t*);
  char func_name[128];
  strcpy(func_name, "usearch_getText");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UStringSearch*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, length);
}

UCollator * usearch_getCollator_android(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_getCollator");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const UStringSearch*))dlsym(handle_i18n, func_name);
  return ptr(strsrch);
}

void usearch_setCollator_android(UStringSearch* strsrch, const UCollator* collator, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UCollator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setCollator");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, const UCollator*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, collator, status);
  return;
}

void usearch_setPattern_android(UStringSearch* strsrch, const UChar* pattern, int32_t patternlength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_setPattern");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(strsrch, pattern, patternlength, status);
  return;
}

const UChar * usearch_getPattern_android(const UStringSearch* strsrch, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UStringSearch*, int32_t*);
  char func_name[128];
  strcpy(func_name, "usearch_getPattern");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UStringSearch*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, length);
}

int32_t usearch_first_android(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_first");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, status);
}

int32_t usearch_following_android(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_following");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, position, status);
}

int32_t usearch_last_android(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_last");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, status);
}

int32_t usearch_preceding_android(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_preceding");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, position, status);
}

int32_t usearch_next_android(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_next");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, status);
}

int32_t usearch_previous_android(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usearch_previous");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(strsrch, status);
}

void usearch_reset_android(UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*);
  char func_name[128];
  strcpy(func_name, "usearch_reset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringSearch*))dlsym(handle_i18n, func_name);
  ptr(strsrch);
  return;
}

/* unicode/umsg.h */

int32_t u_formatMessage_android(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "u_formatMessage");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessage_android(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_vformatMessage");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}

void u_parseMessage_android(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "u_parseMessage");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, status, args);
  va_end(args);
  return;
}

void u_vparseMessage_android(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_vparseMessage");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
  return;
}

int32_t u_formatMessageWithError_android(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "u_formatMessageWithError");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, parseError, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessageWithError_android(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_vformatMessageWithError");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}

void u_parseMessageWithError_android(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UParseError* parseError, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "u_parseMessageWithError");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, parseError, status, args);
  va_end(args);
  return;
}

void u_vparseMessageWithError_android(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_vparseMessageWithError");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
  return;
}

UMessageFormat * umsg_open_android(const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMessageFormat * (*ptr)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_open");
  strcat(func_name, icudata_version);
  ptr = (UMessageFormat *(*)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, patternLength, locale, parseError, status);
}

void umsg_close_android(UMessageFormat* format) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*);
  char func_name[128];
  strcpy(func_name, "umsg_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UMessageFormat*))dlsym(handle_i18n, func_name);
  ptr(format);
  return;
}

UMessageFormat umsg_clone_android(const UMessageFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMessageFormat (*ptr)(const UMessageFormat*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_clone");
  strcat(func_name, icudata_version);
  ptr = (UMessageFormat(*)(const UMessageFormat*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, status);
}

void umsg_setLocale_android(UMessageFormat* fmt, const char* locale) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*, const char*);
  char func_name[128];
  strcpy(func_name, "umsg_setLocale");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UMessageFormat*, const char*))dlsym(handle_i18n, func_name);
  ptr(fmt, locale);
  return;
}

const char * umsg_getLocale_android(const UMessageFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UMessageFormat*);
  char func_name[128];
  strcpy(func_name, "umsg_getLocale");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UMessageFormat*))dlsym(handle_i18n, func_name);
  return ptr(fmt);
}

void umsg_applyPattern_android(UMessageFormat* fmt, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_applyPattern");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(fmt, pattern, patternLength, parseError, status);
  return;
}

int32_t umsg_toPattern_android(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_toPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, result, resultLength, status);
}

int32_t umsg_format_android(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "umsg_format");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(fmt, result, resultLength, status);
  va_end(args);
  return ret;
}

int32_t umsg_vformat_android(const UMessageFormat* fmt, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_vformat");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(fmt, result, resultLength, ap, status);
}

void umsg_parse_android(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, UErrorCode*, ...);
  char func_name[128];
  strcpy(func_name, "umsg_parse");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, UErrorCode*, ...))dlsym(handle_i18n, func_name);
  va_list args;
  va_start(args, status);
  ptr(fmt, source, sourceLength, count, status, args);
  va_end(args);
  return;
}

void umsg_vparse_android(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_vparse");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(fmt, source, sourceLength, count, ap, status);
  return;
}

int32_t umsg_autoQuoteApostrophe_android(const UChar* pattern, int32_t patternLength, UChar* dest, int32_t destCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "umsg_autoQuoteApostrophe");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, patternLength, dest, destCapacity, ec);
}

/* unicode/ulocdata.h */

ULocaleData * ulocdata_open_android(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULocaleData * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_open");
  strcat(func_name, icudata_version);
  ptr = (ULocaleData *(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(localeID, status);
}

void ulocdata_close_android(ULocaleData* uld) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(ULocaleData*);
  char func_name[128];
  strcpy(func_name, "ulocdata_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(ULocaleData*))dlsym(handle_i18n, func_name);
  ptr(uld);
  return;
}

void ulocdata_setNoSubstitute_android(ULocaleData* uld, UBool setting) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(ULocaleData*, UBool);
  char func_name[128];
  strcpy(func_name, "ulocdata_setNoSubstitute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(ULocaleData*, UBool))dlsym(handle_i18n, func_name);
  ptr(uld, setting);
  return;
}

UBool ulocdata_getNoSubstitute_android(ULocaleData* uld) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(ULocaleData*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getNoSubstitute");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(ULocaleData*))dlsym(handle_i18n, func_name);
  return ptr(uld);
}

USet * ulocdata_getExemplarSet_android(ULocaleData* uld, USet* fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getExemplarSet");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(uld, fillIn, options, extype, status);
}

int32_t ulocdata_getDelimiter_android(ULocaleData* uld, ULocaleDataDelimiterType type, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getDelimiter");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(uld, type, result, resultLength, status);
}

UMeasurementSystem ulocdata_getMeasurementSystem_android(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMeasurementSystem (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getMeasurementSystem");
  strcat(func_name, icudata_version);
  ptr = (UMeasurementSystem(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(localeID, status);
}

void ulocdata_getPaperSize_android(const char* localeID, int32_t* height, int32_t* width, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, int32_t*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getPaperSize");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, int32_t*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(localeID, height, width, status);
  return;
}

void ulocdata_getCLDRVersion_android(UVersionInfo versionArray, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getCLDRVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(versionArray, status);
  return;
}

int32_t ulocdata_getLocaleDisplayPattern_android(ULocaleData* uld, UChar* pattern, int32_t patternCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getLocaleDisplayPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(uld, pattern, patternCapacity, status);
}

int32_t ulocdata_getLocaleSeparator_android(ULocaleData* uld, UChar* separator, int32_t separatorCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ulocdata_getLocaleSeparator");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(uld, separator, separatorCapacity, status);
}

/* unicode/ucol.h */

UCollator * ucol_open_android(const char* loc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_open");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(loc, status);
}

UCollator * ucol_openRules_android(const UChar* rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_openRules");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}

UCollator * ucol_openFromShortString_android(const char* definition, UBool forceDefaults, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const char*, UBool, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_openFromShortString");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const char*, UBool, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(definition, forceDefaults, parseError, status);
}

void ucol_getContractionsAndExpansions_android(const UCollator* coll, USet* contractions, USet* expansions, UBool addPrefixes, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, USet*, USet*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getContractionsAndExpansions");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UCollator*, USet*, USet*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(coll, contractions, expansions, addPrefixes, status);
  return;
}

void ucol_close_android(UCollator* coll) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*);
  char func_name[128];
  strcpy(func_name, "ucol_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollator*))dlsym(handle_i18n, func_name);
  ptr(coll);
  return;
}

UCollationResult ucol_strcoll_android(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationResult (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_strcoll");
  strcat(func_name, icudata_version);
  ptr = (UCollationResult(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_greater_android(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_greater");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_greaterOrEqual_android(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_greaterOrEqual");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_equal_android(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_equal");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, source, sourceLength, target, targetLength);
}

UCollationResult ucol_strcollIter_android(const UCollator* coll, UCharIterator* sIter, UCharIterator* tIter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationResult (*ptr)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_strcollIter");
  strcat(func_name, icudata_version);
  ptr = (UCollationResult(*)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, sIter, tIter, status);
}

UCollationStrength ucol_getStrength_android(const UCollator* coll) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationStrength (*ptr)(const UCollator*);
  char func_name[128];
  strcpy(func_name, "ucol_getStrength");
  strcat(func_name, icudata_version);
  ptr = (UCollationStrength(*)(const UCollator*))dlsym(handle_i18n, func_name);
  return ptr(coll);
}

void ucol_setStrength_android(UCollator* coll, UCollationStrength strength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, UCollationStrength);
  char func_name[128];
  strcpy(func_name, "ucol_setStrength");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollator*, UCollationStrength))dlsym(handle_i18n, func_name);
  ptr(coll, strength);
  return;
}

int32_t ucol_getDisplayName_android(const char* objLoc, const char* dispLoc, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getDisplayName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(objLoc, dispLoc, result, resultLength, status);
}

const char * ucol_getAvailable_android(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(localeIndex);
}

int32_t ucol_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucol_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

UEnumeration * ucol_openAvailableLocales_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_openAvailableLocales");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(status);
}

UEnumeration * ucol_getKeywords_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getKeywords");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(status);
}

UEnumeration * ucol_getKeywordValues_android(const char* keyword, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getKeywordValues");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(keyword, status);
}

UEnumeration * ucol_getKeywordValuesForLocale_android(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, const char*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, const char*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(key, locale, commonlyUsed, status);
}

int32_t ucol_getFunctionalEquivalent_android(char* result, int32_t resultCapacity, const char* keyword, const char* locale, UBool* isAvailable, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getFunctionalEquivalent");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}

const UChar * ucol_getRules_android(const UCollator* coll, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UCollator*, int32_t*);
  char func_name[128];
  strcpy(func_name, "ucol_getRules");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UCollator*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(coll, length);
}

int32_t ucol_getShortDefinitionString_android(const UCollator* coll, const char* locale, char* buffer, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getShortDefinitionString");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollator*, const char*, char*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, locale, buffer, capacity, status);
}

int32_t ucol_normalizeShortDefinitionString_android(const char* source, char* destination, int32_t capacity, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_normalizeShortDefinitionString");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(source, destination, capacity, parseError, status);
}

int32_t ucol_getSortKey_android(const UCollator* coll, const UChar* source, int32_t sourceLength, uint8_t* result, int32_t resultLength) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_getSortKey");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, source, sourceLength, result, resultLength);
}

int32_t ucol_nextSortKeyPart_android(const UCollator* coll, UCharIterator* iter, uint32_t state [ 2], uint8_t* dest, int32_t count, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_nextSortKeyPart");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, iter, state, dest, count, status);
}

int32_t ucol_getBound_android(const uint8_t* source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getBound");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}

void ucol_getVersion_android(const UCollator* coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, UVersionInfo);
  char func_name[128];
  strcpy(func_name, "ucol_getVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UCollator*, UVersionInfo))dlsym(handle_i18n, func_name);
  ptr(coll, info);
  return;
}

void ucol_getUCAVersion_android(const UCollator* coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, UVersionInfo);
  char func_name[128];
  strcpy(func_name, "ucol_getUCAVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UCollator*, UVersionInfo))dlsym(handle_i18n, func_name);
  ptr(coll, info);
  return;
}

int32_t ucol_mergeSortkeys_android(const uint8_t* src1, int32_t src1Length, const uint8_t* src2, int32_t src2Length, uint8_t* dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_mergeSortkeys");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}

void ucol_setAttribute_android(UCollator* coll, UColAttribute attr, UColAttributeValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_setAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(coll, attr, value, status);
  return;
}

UColAttributeValue ucol_getAttribute_android(const UCollator* coll, UColAttribute attr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UColAttributeValue (*ptr)(const UCollator*, UColAttribute, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getAttribute");
  strcat(func_name, icudata_version);
  ptr = (UColAttributeValue(*)(const UCollator*, UColAttribute, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, attr, status);
}

uint32_t ucol_setVariableTop_android(UCollator* coll, const UChar* varTop, int32_t len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(UCollator*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_setVariableTop");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(UCollator*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, varTop, len, status);
}

uint32_t ucol_getVariableTop_android(const UCollator* coll, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCollator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getVariableTop");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(const UCollator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, status);
}

void ucol_restoreVariableTop_android(UCollator* coll, const uint32_t varTop, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, const uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_restoreVariableTop");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCollator*, const uint32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(coll, varTop, status);
  return;
}

UCollator * ucol_safeClone_android(const UCollator* coll, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const UCollator*, void*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_safeClone");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const UCollator*, void*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, stackBuffer, pBufferSize, status);
}

int32_t ucol_getRulesEx_android(const UCollator* coll, UColRuleOption delta, UChar* buffer, int32_t bufferLen) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, UColRuleOption, UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucol_getRulesEx");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollator*, UColRuleOption, UChar*, int32_t))dlsym(handle_i18n, func_name);
  return ptr(coll, delta, buffer, bufferLen);
}

const char * ucol_getLocaleByType_android(const UCollator* coll, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCollator*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCollator*, ULocDataLocaleType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, type, status);
}

USet * ucol_getTailoredSet_android(const UCollator* coll, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(const UCollator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_getTailoredSet");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(const UCollator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, status);
}

int32_t ucol_cloneBinary_android(const UCollator* coll, uint8_t* buffer, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, uint8_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_cloneBinary");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCollator*, uint8_t*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(coll, buffer, capacity, status);
}

UCollator * ucol_openBinary_android(const uint8_t* bin, int32_t length, const UCollator* base, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator * (*ptr)(const uint8_t*, int32_t, const UCollator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucol_openBinary");
  strcat(func_name, icudata_version);
  ptr = (UCollator *(*)(const uint8_t*, int32_t, const UCollator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(bin, length, base, status);
}

/* unicode/utmscale.h */

int64_t utmscale_getTimeScaleValue_android(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UDateTimeScale, UTimeScaleValue, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utmscale_getTimeScaleValue");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(UDateTimeScale, UTimeScaleValue, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(timeScale, value, status);
}

int64_t utmscale_fromInt64_android(int64_t otherTime, UDateTimeScale timeScale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utmscale_fromInt64");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(otherTime, timeScale, status);
}

int64_t utmscale_toInt64_android(int64_t universalTime, UDateTimeScale timeScale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utmscale_toInt64");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(universalTime, timeScale, status);
}

/* unicode/uregex.h */

URegularExpression * uregex_open_android(const UChar* pattern, int32_t patternLength, uint32_t flags, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression * (*ptr)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_open");
  strcat(func_name, icudata_version);
  ptr = (URegularExpression *(*)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, patternLength, flags, pe, status);
}

URegularExpression * uregex_openC_android(const char* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression * (*ptr)(const char*, uint32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_openC");
  strcat(func_name, icudata_version);
  ptr = (URegularExpression *(*)(const char*, uint32_t, UParseError*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pattern, flags, pe, status);
}

void uregex_close_android(URegularExpression* regexp) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*);
  char func_name[128];
  strcpy(func_name, "uregex_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*))dlsym(handle_i18n, func_name);
  ptr(regexp);
  return;
}

URegularExpression * uregex_clone_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression * (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_clone");
  strcat(func_name, icudata_version);
  ptr = (URegularExpression *(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

const UChar * uregex_pattern_android(const URegularExpression* regexp, int32_t* patLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const URegularExpression*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_pattern");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const URegularExpression*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, patLength, status);
}

int32_t uregex_flags_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_flags");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

void uregex_setText_android(URegularExpression* regexp, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_setText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, text, textLength, status);
  return;
}

const UChar * uregex_getText_android(URegularExpression* regexp, int32_t* textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(URegularExpression*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_getText");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(URegularExpression*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, textLength, status);
}

UBool uregex_matches_android(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_matches");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, startIndex, status);
}

UBool uregex_lookingAt_android(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_lookingAt");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, startIndex, status);
}

UBool uregex_find_android(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_find");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, startIndex, status);
}

UBool uregex_findNext_android(URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_findNext");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

int32_t uregex_groupCount_android(URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_groupCount");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

int32_t uregex_group_android(URegularExpression* regexp, int32_t groupNum, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_group");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, groupNum, dest, destCapacity, status);
}

int32_t uregex_start_android(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_start");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, groupNum, status);
}

int32_t uregex_end_android(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_end");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, groupNum, status);
}

void uregex_reset_android(URegularExpression* regexp, int32_t index, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_reset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, index, status);
  return;
}

void uregex_setRegion_android(URegularExpression* regexp, int32_t regionStart, int32_t regionLimit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_setRegion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, int32_t, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, regionStart, regionLimit, status);
  return;
}

int32_t uregex_regionStart_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_regionStart");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

int32_t uregex_regionEnd_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_regionEnd");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

UBool uregex_hasTransparentBounds_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_hasTransparentBounds");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

void uregex_useTransparentBounds_android(URegularExpression* regexp, UBool b, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_useTransparentBounds");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, b, status);
  return;
}

UBool uregex_hasAnchoringBounds_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_hasAnchoringBounds");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

void uregex_useAnchoringBounds_android(URegularExpression* regexp, UBool b, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_useAnchoringBounds");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, b, status);
  return;
}

UBool uregex_hitEnd_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_hitEnd");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

UBool uregex_requireEnd_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_requireEnd");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

int32_t uregex_replaceAll_android(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_replaceAll");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_replaceFirst_android(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_replaceFirst");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_appendReplacement_android(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_appendReplacement");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_appendTail_android(URegularExpression* regexp, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UChar**, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_appendTail");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, UChar**, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, destBuf, destCapacity, status);
}

int32_t uregex_split_android(URegularExpression* regexp, UChar* destBuf, int32_t destCapacity, int32_t* requiredCapacity, UChar* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_split");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}

void uregex_setTimeLimit_android(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_setTimeLimit");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getTimeLimit_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_getTimeLimit");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

void uregex_setStackLimit_android(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_setStackLimit");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getStackLimit_android(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_getStackLimit");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(regexp, status);
}

void uregex_setMatchCallback_android(URegularExpression* regexp, URegexMatchCallback* callback, const void* context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_setMatchCallback");
  strcat(func_name, icudata_version);
  ptr = (void(*)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, callback, context, status);
  return;
}

void uregex_getMatchCallback_android(const URegularExpression* regexp, URegexMatchCallback** callback, const void** context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uregex_getMatchCallback");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(regexp, callback, context, status);
  return;
}

/* unicode/uspoof.h */

USpoofChecker * uspoof_open_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_open");
  strcat(func_name, icudata_version);
  ptr = (USpoofChecker *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(status);
}

void uspoof_close_android(USpoofChecker* sc) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*);
  char func_name[128];
  strcpy(func_name, "uspoof_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USpoofChecker*))dlsym(handle_i18n, func_name);
  ptr(sc);
  return;
}

USpoofChecker * uspoof_clone_android(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker * (*ptr)(const USpoofChecker*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_clone");
  strcat(func_name, icudata_version);
  ptr = (USpoofChecker *(*)(const USpoofChecker*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, status);
}

void uspoof_setChecks_android(USpoofChecker* sc, int32_t checks, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_setChecks");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USpoofChecker*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(sc, checks, status);
  return;
}

int32_t uspoof_getChecks_android(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_getChecks");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, status);
}

void uspoof_setAllowedLocales_android(USpoofChecker* sc, const char* localesList, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_setAllowedLocales");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USpoofChecker*, const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(sc, localesList, status);
  return;
}

const char * uspoof_getAllowedLocales_android(USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(USpoofChecker*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_getAllowedLocales");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(USpoofChecker*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, status);
}

void uspoof_setAllowedChars_android(USpoofChecker* sc, const USet* chars, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, const USet*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_setAllowedChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USpoofChecker*, const USet*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(sc, chars, status);
  return;
}

const USet * uspoof_getAllowedChars_android(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const USet * (*ptr)(const USpoofChecker*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_getAllowedChars");
  strcat(func_name, icudata_version);
  ptr = (const USet *(*)(const USpoofChecker*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, status);
}

int32_t uspoof_check_android(const USpoofChecker* sc, const UChar* text, int32_t length, int32_t* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_check");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, text, length, position, status);
}

int32_t uspoof_checkUTF8_android(const USpoofChecker* sc, const char* text, int32_t length, int32_t* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_checkUTF8");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, text, length, position, status);
}

int32_t uspoof_areConfusable_android(const USpoofChecker* sc, const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_areConfusable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, s1, length1, s2, length2, status);
}

int32_t uspoof_areConfusableUTF8_android(const USpoofChecker* sc, const char* s1, int32_t length1, const char* s2, int32_t length2, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_areConfusableUTF8");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, s1, length1, s2, length2, status);
}

int32_t uspoof_getSkeleton_android(const USpoofChecker* sc, uint32_t type, const UChar* s, int32_t length, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_getSkeleton");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, type, s, length, dest, destCapacity, status);
}

int32_t uspoof_getSkeletonUTF8_android(const USpoofChecker* sc, uint32_t type, const char* s, int32_t length, char* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uspoof_getSkeletonUTF8");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(sc, type, s, length, dest, destCapacity, status);
}

/* unicode/udatpg.h */

UDateTimePatternGenerator * udatpg_open_android(const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_open");
  strcat(func_name, icudata_version);
  ptr = (UDateTimePatternGenerator *(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(locale, pErrorCode);
}

UDateTimePatternGenerator * udatpg_openEmpty_android(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_openEmpty");
  strcat(func_name, icudata_version);
  ptr = (UDateTimePatternGenerator *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(pErrorCode);
}

void udatpg_close_android(UDateTimePatternGenerator* dtpg) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*);
  char func_name[128];
  strcpy(func_name, "udatpg_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateTimePatternGenerator*))dlsym(handle_i18n, func_name);
  ptr(dtpg);
  return;
}

UDateTimePatternGenerator * udatpg_clone_android(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator * (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_clone");
  strcat(func_name, icudata_version);
  ptr = (UDateTimePatternGenerator *(*)(const UDateTimePatternGenerator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pErrorCode);
}

int32_t udatpg_getBestPattern_android(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_getBestPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}

int32_t udatpg_getSkeleton_android(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* skeleton, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_getSkeleton");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pattern, length, skeleton, capacity, pErrorCode);
}

int32_t udatpg_getBaseSkeleton_android(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* baseSkeleton, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_getBaseSkeleton");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}

UDateTimePatternConflict udatpg_addPattern_android(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, UBool override, UChar* conflictingPattern, int32_t capacity, int32_t* pLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternConflict (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_addPattern");
  strcat(func_name, icudata_version);
  ptr = (UDateTimePatternConflict(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}

void udatpg_setAppendItemFormat_android(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "udatpg_setAppendItemFormat");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(dtpg, field, value, length);
  return;
}

const UChar * udatpg_getAppendItemFormat_android(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  char func_name[128];
  strcpy(func_name, "udatpg_getAppendItemFormat");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, field, pLength);
}

void udatpg_setAppendItemName_android(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "udatpg_setAppendItemName");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(dtpg, field, value, length);
  return;
}

const UChar * udatpg_getAppendItemName_android(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  char func_name[128];
  strcpy(func_name, "udatpg_getAppendItemName");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, field, pLength);
}

void udatpg_setDateTimeFormat_android(const UDateTimePatternGenerator* dtpg, const UChar* dtFormat, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "udatpg_setDateTimeFormat");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UDateTimePatternGenerator*, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(dtpg, dtFormat, length);
  return;
}

const UChar * udatpg_getDateTimeFormat_android(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  char func_name[128];
  strcpy(func_name, "udatpg_getDateTimeFormat");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UDateTimePatternGenerator*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pLength);
}

void udatpg_setDecimal_android(UDateTimePatternGenerator* dtpg, const UChar* decimal, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "udatpg_setDecimal");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDateTimePatternGenerator*, const UChar*, int32_t))dlsym(handle_i18n, func_name);
  ptr(dtpg, decimal, length);
  return;
}

const UChar * udatpg_getDecimal_android(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  char func_name[128];
  strcpy(func_name, "udatpg_getDecimal");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UDateTimePatternGenerator*, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pLength);
}

int32_t udatpg_replaceFieldTypes_android(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_replaceFieldTypes");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}

UEnumeration * udatpg_openSkeletons_android(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_openSkeletons");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const UDateTimePatternGenerator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pErrorCode);
}

UEnumeration * udatpg_openBaseSkeletons_android(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udatpg_openBaseSkeletons");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const UDateTimePatternGenerator*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, pErrorCode);
}

const UChar * udatpg_getPatternForSkeleton_android(const UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t skeletonLength, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*);
  char func_name[128];
  strcpy(func_name, "udatpg_getPatternForSkeleton");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*))dlsym(handle_i18n, func_name);
  return ptr(dtpg, skeleton, skeletonLength, pLength);
}

/* unicode/ucsdet.h */

UCharsetDetector * ucsdet_open_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCharsetDetector * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_open");
  strcat(func_name, icudata_version);
  ptr = (UCharsetDetector *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(status);
}

void ucsdet_close_android(UCharsetDetector* ucsd) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*);
  char func_name[128];
  strcpy(func_name, "ucsdet_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharsetDetector*))dlsym(handle_i18n, func_name);
  ptr(ucsd);
  return;
}

void ucsdet_setText_android(UCharsetDetector* ucsd, const char* textIn, int32_t len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_setText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(ucsd, textIn, len, status);
  return;
}

void ucsdet_setDeclaredEncoding_android(UCharsetDetector* ucsd, const char* encoding, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_setDeclaredEncoding");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(ucsd, encoding, length, status);
  return;
}

const UCharsetMatch * ucsdet_detect_android(UCharsetDetector* ucsd, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UCharsetMatch * (*ptr)(UCharsetDetector*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_detect");
  strcat(func_name, icudata_version);
  ptr = (const UCharsetMatch *(*)(UCharsetDetector*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsd, status);
}

const UCharsetMatch * * ucsdet_detectAll_android(UCharsetDetector* ucsd, int32_t* matchesFound, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UCharsetMatch * * (*ptr)(UCharsetDetector*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_detectAll");
  strcat(func_name, icudata_version);
  ptr = (const UCharsetMatch * *(*)(UCharsetDetector*, int32_t*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsd, matchesFound, status);
}

const char * ucsdet_getName_android(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCharsetMatch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_getName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCharsetMatch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsm, status);
}

int32_t ucsdet_getConfidence_android(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCharsetMatch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_getConfidence");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCharsetMatch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsm, status);
}

const char * ucsdet_getLanguage_android(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCharsetMatch*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_getLanguage");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCharsetMatch*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsm, status);
}

int32_t ucsdet_getUChars_android(const UCharsetMatch* ucsm, UChar* buf, int32_t cap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_getUChars");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsm, buf, cap, status);
}

UEnumeration * ucsdet_getAllDetectableCharsets_android(const UCharsetDetector* ucsd, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const UCharsetDetector*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucsdet_getAllDetectableCharsets");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const UCharsetDetector*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ucsd, status);
}

UBool ucsdet_isInputFilterEnabled_android(const UCharsetDetector* ucsd) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCharsetDetector*);
  char func_name[128];
  strcpy(func_name, "ucsdet_isInputFilterEnabled");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCharsetDetector*))dlsym(handle_i18n, func_name);
  return ptr(ucsd);
}

UBool ucsdet_enableInputFilter_android(UCharsetDetector* ucsd, UBool filter) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UCharsetDetector*, UBool);
  char func_name[128];
  strcpy(func_name, "ucsdet_enableInputFilter");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UCharsetDetector*, UBool))dlsym(handle_i18n, func_name);
  return ptr(ucsd, filter);
}

/* unicode/ucal.h */

UEnumeration * ucal_openTimeZones_android(UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_openTimeZones");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(ec);
}

UEnumeration * ucal_openCountryTimeZones_android(const char* country, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_openCountryTimeZones");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(country, ec);
}

int32_t ucal_getDefaultTimeZone_android(UChar* result, int32_t resultCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getDefaultTimeZone");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(result, resultCapacity, ec);
}

void ucal_setDefaultTimeZone_android(const UChar* zoneID, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setDefaultTimeZone");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UChar*, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(zoneID, ec);
  return;
}

int32_t ucal_getDSTSavings_android(const UChar* zoneID, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getDSTSavings");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(zoneID, ec);
}

UDate ucal_getNow_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucal_getNow");
  strcat(func_name, icudata_version);
  ptr = (UDate(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

UCalendar * ucal_open_android(const UChar* zoneID, int32_t len, const char* locale, UCalendarType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendar * (*ptr)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_open");
  strcat(func_name, icudata_version);
  ptr = (UCalendar *(*)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(zoneID, len, locale, type, status);
}

void ucal_close_android(UCalendar* cal) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*);
  char func_name[128];
  strcpy(func_name, "ucal_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*))dlsym(handle_i18n, func_name);
  ptr(cal);
  return;
}

UCalendar * ucal_clone_android(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendar * (*ptr)(const UCalendar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_clone");
  strcat(func_name, icudata_version);
  ptr = (UCalendar *(*)(const UCalendar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, status);
}

void ucal_setTimeZone_android(UCalendar* cal, const UChar* zoneID, int32_t len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setTimeZone");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, const UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, zoneID, len, status);
  return;
}

int32_t ucal_getTimeZoneDisplayName_android(const UCalendar* cal, UCalendarDisplayNameType type, const char* locale, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getTimeZoneDisplayName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, type, locale, result, resultLength, status);
}

UBool ucal_inDaylightTime_android(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_inDaylightTime");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCalendar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, status);
}

void ucal_setGregorianChange_android(UCalendar* cal, UDate date, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setGregorianChange");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, date, pErrorCode);
  return;
}

UDate ucal_getGregorianChange_android(const UCalendar* cal, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getGregorianChange");
  strcat(func_name, icudata_version);
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, pErrorCode);
}

int32_t ucal_getAttribute_android(const UCalendar* cal, UCalendarAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarAttribute);
  char func_name[128];
  strcpy(func_name, "ucal_getAttribute");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCalendar*, UCalendarAttribute))dlsym(handle_i18n, func_name);
  return ptr(cal, attr);
}

void ucal_setAttribute_android(UCalendar* cal, UCalendarAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarAttribute, int32_t);
  char func_name[128];
  strcpy(func_name, "ucal_setAttribute");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UCalendarAttribute, int32_t))dlsym(handle_i18n, func_name);
  ptr(cal, attr, newValue);
  return;
}

const char * ucal_getAvailable_android(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucal_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_i18n, func_name);
  return ptr(localeIndex);
}

int32_t ucal_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucal_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_i18n, func_name);
  return ptr();
}

UDate ucal_getMillis_android(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getMillis");
  strcat(func_name, icudata_version);
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, status);
}

void ucal_setMillis_android(UCalendar* cal, UDate dateTime, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setMillis");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, dateTime, status);
  return;
}

void ucal_setDate_android(UCalendar* cal, int32_t year, int32_t month, int32_t date, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setDate");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, year, month, date, status);
  return;
}

void ucal_setDateTime_android(UCalendar* cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_setDateTime");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, year, month, date, hour, minute, second, status);
  return;
}

UBool ucal_equivalentTo_android(const UCalendar* cal1, const UCalendar* cal2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, const UCalendar*);
  char func_name[128];
  strcpy(func_name, "ucal_equivalentTo");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCalendar*, const UCalendar*))dlsym(handle_i18n, func_name);
  return ptr(cal1, cal2);
}

void ucal_add_android(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_add");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, field, amount, status);
  return;
}

void ucal_roll_android(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_roll");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))dlsym(handle_i18n, func_name);
  ptr(cal, field, amount, status);
  return;
}

int32_t ucal_get_android(const UCalendar* cal, UCalendarDateFields field, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_get");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, field, status);
}

void ucal_set_android(UCalendar* cal, UCalendarDateFields field, int32_t value) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t);
  char func_name[128];
  strcpy(func_name, "ucal_set");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t))dlsym(handle_i18n, func_name);
  ptr(cal, field, value);
  return;
}

UBool ucal_isSet_android(const UCalendar* cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UCalendarDateFields);
  char func_name[128];
  strcpy(func_name, "ucal_isSet");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UCalendar*, UCalendarDateFields))dlsym(handle_i18n, func_name);
  return ptr(cal, field);
}

void ucal_clearField_android(UCalendar* cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields);
  char func_name[128];
  strcpy(func_name, "ucal_clearField");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*, UCalendarDateFields))dlsym(handle_i18n, func_name);
  ptr(cal, field);
  return;
}

void ucal_clear_android(UCalendar* calendar) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*);
  char func_name[128];
  strcpy(func_name, "ucal_clear");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCalendar*))dlsym(handle_i18n, func_name);
  ptr(calendar);
  return;
}

int32_t ucal_getLimit_android(const UCalendar* cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getLimit");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, field, type, status);
}

const char * ucal_getLocaleByType_android(const UCalendar* cal, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCalendar*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCalendar*, ULocDataLocaleType, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, type, status);
}

const char * ucal_getTZDataVersion_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getTZDataVersion");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(status);
}

int32_t ucal_getCanonicalTimeZoneID_android(const UChar* id, int32_t len, UChar* result, int32_t resultCapacity, UBool* isSystemID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getCanonicalTimeZoneID");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(id, len, result, resultCapacity, isSystemID, status);
}

const char * ucal_getType_android(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCalendar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCalendar*, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(cal, status);
}

UEnumeration * ucal_getKeywordValuesForLocale_android(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, const char*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucal_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, const char*, UBool, UErrorCode*))dlsym(handle_i18n, func_name);
  return ptr(key, locale, commonlyUsed, status);
}

/* unicode/ucnv_err.h */

void UCNV_FROM_U_CALLBACK_STOP_android(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_FROM_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_STOP_android(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_TO_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SKIP_android(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SUBSTITUTE_android(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_ESCAPE_android(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_FROM_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SKIP_android(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_TO_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SUBSTITUTE_android(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_TO_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_ESCAPE_android(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "UCNV_TO_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))dlsym(handle_common, func_name);
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

/* unicode/ucnv.h */

int ucnv_compareNames_android(const char* name1, const char* name2) {
  pthread_once(&once_control, &init_icudata_version);
  int (*ptr)(const char*, const char*);
  char func_name[128];
  strcpy(func_name, "ucnv_compareNames");
  strcat(func_name, icudata_version);
  ptr = (int(*)(const char*, const char*))dlsym(handle_common, func_name);
  return ptr(name1, name2);
}

UConverter * ucnv_open_android(const char* converterName, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_open");
  strcat(func_name, icudata_version);
  ptr = (UConverter *(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converterName, err);
}

UConverter * ucnv_openU_android(const UChar* name, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter * (*ptr)(const UChar*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_openU");
  strcat(func_name, icudata_version);
  ptr = (UConverter *(*)(const UChar*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(name, err);
}

UConverter * ucnv_openCCSID_android(int32_t codepage, UConverterPlatform platform, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter * (*ptr)(int32_t, UConverterPlatform, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_openCCSID");
  strcat(func_name, icudata_version);
  ptr = (UConverter *(*)(int32_t, UConverterPlatform, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(codepage, platform, err);
}

UConverter * ucnv_openPackage_android(const char* packageName, const char* converterName, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_openPackage");
  strcat(func_name, icudata_version);
  ptr = (UConverter *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(packageName, converterName, err);
}

UConverter * ucnv_safeClone_android(const UConverter* cnv, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter * (*ptr)(const UConverter*, void*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_safeClone");
  strcat(func_name, icudata_version);
  ptr = (UConverter *(*)(const UConverter*, void*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, stackBuffer, pBufferSize, status);
}

void ucnv_close_android(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*))dlsym(handle_common, func_name);
  ptr(converter);
  return;
}

void ucnv_getSubstChars_android(const UConverter* converter, char* subChars, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getSubstChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstChars_android(UConverter* converter, const char* subChars, int8_t len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, const char*, int8_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_setSubstChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, const char*, int8_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstString_android(UConverter* cnv, const UChar* s, int32_t length, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_setSubstString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(cnv, s, length, err);
  return;
}

void ucnv_getInvalidChars_android(const UConverter* converter, char* errBytes, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getInvalidChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, errBytes, len, err);
  return;
}

void ucnv_getInvalidUChars_android(const UConverter* converter, UChar* errUChars, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UChar*, int8_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getInvalidUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, UChar*, int8_t*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, errUChars, len, err);
  return;
}

void ucnv_reset_android(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_reset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*))dlsym(handle_common, func_name);
  ptr(converter);
  return;
}

void ucnv_resetToUnicode_android(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_resetToUnicode");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*))dlsym(handle_common, func_name);
  ptr(converter);
  return;
}

void ucnv_resetFromUnicode_android(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_resetFromUnicode");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*))dlsym(handle_common, func_name);
  ptr(converter);
  return;
}

int8_t ucnv_getMaxCharSize_android(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(const UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_getMaxCharSize");
  strcat(func_name, icudata_version);
  ptr = (int8_t(*)(const UConverter*))dlsym(handle_common, func_name);
  return ptr(converter);
}

int8_t ucnv_getMinCharSize_android(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(const UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_getMinCharSize");
  strcat(func_name, icudata_version);
  ptr = (int8_t(*)(const UConverter*))dlsym(handle_common, func_name);
  return ptr(converter);
}

int32_t ucnv_getDisplayName_android(const UConverter* converter, const char* displayLocale, UChar* displayName, int32_t displayNameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getDisplayName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}

const char * ucnv_getName_android(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UConverter*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UConverter*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converter, err);
}

int32_t ucnv_getCCSID_android(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getCCSID");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converter, err);
}

UConverterPlatform ucnv_getPlatform_android(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterPlatform (*ptr)(const UConverter*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getPlatform");
  strcat(func_name, icudata_version);
  ptr = (UConverterPlatform(*)(const UConverter*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converter, err);
}

UConverterType ucnv_getType_android(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterType (*ptr)(const UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_getType");
  strcat(func_name, icudata_version);
  ptr = (UConverterType(*)(const UConverter*))dlsym(handle_common, func_name);
  return ptr(converter);
}

void ucnv_getStarters_android(const UConverter* converter, UBool starters [ 256], UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UBool [ 256], UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getStarters");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, UBool [ 256], UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, starters, err);
  return;
}

void ucnv_getUnicodeSet_android(const UConverter* cnv, USet* setFillIn, UConverterUnicodeSet whichSet, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getUnicodeSet");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*))dlsym(handle_common, func_name);
  ptr(cnv, setFillIn, whichSet, pErrorCode);
  return;
}

void ucnv_getToUCallBack_android(const UConverter* converter, UConverterToUCallback* action, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UConverterToUCallback*, const void**);
  char func_name[128];
  strcpy(func_name, "ucnv_getToUCallBack");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, UConverterToUCallback*, const void**))dlsym(handle_common, func_name);
  ptr(converter, action, context);
  return;
}

void ucnv_getFromUCallBack_android(const UConverter* converter, UConverterFromUCallback* action, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UConverterFromUCallback*, const void**);
  char func_name[128];
  strcpy(func_name, "ucnv_getFromUCallBack");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, UConverterFromUCallback*, const void**))dlsym(handle_common, func_name);
  ptr(converter, action, context);
  return;
}

void ucnv_setToUCallBack_android(UConverter* converter, UConverterToUCallback newAction, const void* newContext, UConverterToUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_setToUCallBack");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_setFromUCallBack_android(UConverter* converter, UConverterFromUCallback newAction, const void* newContext, UConverterFromUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_setFromUCallBack");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_fromUnicode_android(UConverter* converter, char** target, const char* targetLimit, const UChar** source, const UChar* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_fromUnicode");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

void ucnv_toUnicode_android(UConverter* converter, UChar** target, const UChar* targetLimit, const char** source, const char* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_toUnicode");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*))dlsym(handle_common, func_name);
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

int32_t ucnv_fromUChars_android(UConverter* cnv, char* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_fromUChars");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucnv_toUChars_android(UConverter* cnv, UChar* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_toUChars");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

UChar32 ucnv_getNextUChar_android(UConverter* converter, const char** source, const char* sourceLimit, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UConverter*, const char**, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getNextUChar");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UConverter*, const char**, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converter, source, sourceLimit, err);
}

void ucnv_convertEx_android(UConverter* targetCnv, UConverter* sourceCnv, char** target, const char* targetLimit, const char** source, const char* sourceLimit, UChar* pivotStart, UChar** pivotSource, UChar** pivotTarget, const UChar* pivotLimit, UBool reset, UBool flush, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_convertEx");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*))dlsym(handle_common, func_name);
  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
  return;
}

int32_t ucnv_convert_android(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_convert");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_toAlgorithmic_android(UConverterType algorithmicType, UConverter* cnv, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_toAlgorithmic");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_fromAlgorithmic_android(UConverter* cnv, UConverterType algorithmicType, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_fromAlgorithmic");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_flushCache_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucnv_flushCache");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

int32_t ucnv_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucnv_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

const char * ucnv_getAvailableName_android(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ucnv_getAvailableName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_common, func_name);
  return ptr(n);
}

UEnumeration * ucnv_openAllNames_android(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_openAllNames");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pErrorCode);
}

uint16_t ucnv_countAliases_android(const char* alias, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  uint16_t (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_countAliases");
  strcat(func_name, icudata_version);
  ptr = (uint16_t(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(alias, pErrorCode);
}

const char * ucnv_getAlias_android(const char* alias, uint16_t n, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*, uint16_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getAlias");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*, uint16_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(alias, n, pErrorCode);
}

void ucnv_getAliases_android(const char* alias, const char** aliases, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const char**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getAliases");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const char**, UErrorCode*))dlsym(handle_common, func_name);
  ptr(alias, aliases, pErrorCode);
  return;
}

UEnumeration * ucnv_openStandardNames_android(const char* convName, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_openStandardNames");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(convName, standard, pErrorCode);
}

uint16_t ucnv_countStandards_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  uint16_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucnv_countStandards");
  strcat(func_name, icudata_version);
  ptr = (uint16_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

const char * ucnv_getStandard_android(uint16_t n, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(uint16_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getStandard");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(uint16_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(n, pErrorCode);
}

const char * ucnv_getStandardName_android(const char* name, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getStandardName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(name, standard, pErrorCode);
}

const char * ucnv_getCanonicalName_android(const char* alias, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_getCanonicalName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(alias, standard, pErrorCode);
}

const char * ucnv_getDefaultName_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ucnv_getDefaultName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

void ucnv_fixFileSeparator_android(const UConverter* cnv, UChar* source, int32_t sourceLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "ucnv_fixFileSeparator");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UConverter*, UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(cnv, source, sourceLen);
  return;
}

UBool ucnv_isAmbiguous_android(const UConverter* cnv) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_isAmbiguous");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UConverter*))dlsym(handle_common, func_name);
  return ptr(cnv);
}

void ucnv_setFallback_android(UConverter* cnv, UBool usesFallback) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UBool);
  char func_name[128];
  strcpy(func_name, "ucnv_setFallback");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverter*, UBool))dlsym(handle_common, func_name);
  ptr(cnv, usesFallback);
  return;
}

UBool ucnv_usesFallback_android(const UConverter* cnv) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UConverter*);
  char func_name[128];
  strcpy(func_name, "ucnv_usesFallback");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UConverter*))dlsym(handle_common, func_name);
  return ptr(cnv);
}

const char * ucnv_detectUnicodeSignature_android(const char* source, int32_t sourceLength, int32_t* signatureLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_detectUnicodeSignature");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*, int32_t, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(source, sourceLength, signatureLength, pErrorCode);
}

int32_t ucnv_fromUCountPending_android(const UConverter* cnv, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_fromUCountPending");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, status);
}

int32_t ucnv_toUCountPending_android(const UConverter* cnv, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_toUCountPending");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(cnv, status);
}

/* unicode/ustring.h */

int32_t u_strlen_android(const UChar* s) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strlen");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*))dlsym(handle_common, func_name);
  return ptr(s);
}

int32_t u_countChar32_android(const UChar* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_countChar32");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(s, length);
}

UBool u_strHasMoreChar32Than_android(const UChar* s, int32_t length, int32_t number) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strHasMoreChar32Than");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))dlsym(handle_common, func_name);
  return ptr(s, length, number);
}

UChar * u_strcat_android(UChar* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strcat");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(dst, src);
}

UChar * u_strncat_android(UChar* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strncat");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(dst, src, n);
}

UChar * u_strstr_android(const UChar* s, const UChar* substring) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strstr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(s, substring);
}

UChar * u_strFindFirst_android(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strFindFirst");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(s, length, substring, subLength);
}

UChar * u_strchr_android(const UChar* s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar);
  char func_name[128];
  strcpy(func_name, "u_strchr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar))dlsym(handle_common, func_name);
  return ptr(s, c);
}

UChar * u_strchr32_android(const UChar* s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar32);
  char func_name[128];
  strcpy(func_name, "u_strchr32");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar32))dlsym(handle_common, func_name);
  return ptr(s, c);
}

UChar * u_strrstr_android(const UChar* s, const UChar* substring) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strrstr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(s, substring);
}

UChar * u_strFindLast_android(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strFindLast");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, int32_t, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(s, length, substring, subLength);
}

UChar * u_strrchr_android(const UChar* s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar);
  char func_name[128];
  strcpy(func_name, "u_strrchr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar))dlsym(handle_common, func_name);
  return ptr(s, c);
}

UChar * u_strrchr32_android(const UChar* s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar32);
  char func_name[128];
  strcpy(func_name, "u_strrchr32");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar32))dlsym(handle_common, func_name);
  return ptr(s, c);
}

UChar * u_strpbrk_android(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strpbrk");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(string, matchSet);
}

int32_t u_strcspn_android(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strcspn");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(string, matchSet);
}

int32_t u_strspn_android(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strspn");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(string, matchSet);
}

UChar * u_strtok_r_android(UChar* src, const UChar* delim, UChar** saveState) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*, UChar**);
  char func_name[128];
  strcpy(func_name, "u_strtok_r");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*, UChar**))dlsym(handle_common, func_name);
  return ptr(src, delim, saveState);
}

int32_t u_strcmp_android(const UChar* s1, const UChar* s2) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strcmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(s1, s2);
}

int32_t u_strcmpCodePointOrder_android(const UChar* s1, const UChar* s2) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strcmpCodePointOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(s1, s2);
}

int32_t u_strCompare_android(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UBool);
  char func_name[128];
  strcpy(func_name, "u_strCompare");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UBool))dlsym(handle_common, func_name);
  return ptr(s1, length1, s2, length2, codePointOrder);
}

int32_t u_strCompareIter_android(UCharIterator* iter1, UCharIterator* iter2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UCharIterator*, UBool);
  char func_name[128];
  strcpy(func_name, "u_strCompareIter");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCharIterator*, UCharIterator*, UBool))dlsym(handle_common, func_name);
  return ptr(iter1, iter2, codePointOrder);
}

int32_t u_strCaseCompare_android(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strCaseCompare");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

int32_t u_strncmp_android(const UChar* ucs1, const UChar* ucs2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strncmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(ucs1, ucs2, n);
}

int32_t u_strncmpCodePointOrder_android(const UChar* s1, const UChar* s2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strncmpCodePointOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(s1, s2, n);
}

int32_t u_strcasecmp_android(const UChar* s1, const UChar* s2, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, uint32_t);
  char func_name[128];
  strcpy(func_name, "u_strcasecmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, uint32_t))dlsym(handle_common, func_name);
  return ptr(s1, s2, options);
}

int32_t u_strncasecmp_android(const UChar* s1, const UChar* s2, int32_t n, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  char func_name[128];
  strcpy(func_name, "u_strncasecmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))dlsym(handle_common, func_name);
  return ptr(s1, s2, n, options);
}

int32_t u_memcasecmp_android(const UChar* s1, const UChar* s2, int32_t length, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  char func_name[128];
  strcpy(func_name, "u_memcasecmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))dlsym(handle_common, func_name);
  return ptr(s1, s2, length, options);
}

UChar * u_strcpy_android(UChar* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_strcpy");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*))dlsym(handle_common, func_name);
  return ptr(dst, src);
}

UChar * u_strncpy_android(UChar* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_strncpy");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(dst, src, n);
}

UChar * u_uastrcpy_android(UChar* dst, const char* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const char*);
  char func_name[128];
  strcpy(func_name, "u_uastrcpy");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const char*))dlsym(handle_common, func_name);
  return ptr(dst, src);
}

UChar * u_uastrncpy_android(UChar* dst, const char* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const char*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_uastrncpy");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const char*, int32_t))dlsym(handle_common, func_name);
  return ptr(dst, src, n);
}

char * u_austrcpy_android(char* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  char * (*ptr)(char*, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_austrcpy");
  strcat(func_name, icudata_version);
  ptr = (char *(*)(char*, const UChar*))dlsym(handle_common, func_name);
  return ptr(dst, src);
}

char * u_austrncpy_android(char* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  char * (*ptr)(char*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_austrncpy");
  strcat(func_name, icudata_version);
  ptr = (char *(*)(char*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(dst, src, n);
}

UChar * u_memcpy_android(UChar* dest, const UChar* src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memcpy");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(dest, src, count);
}

UChar * u_memmove_android(UChar* dest, const UChar* src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memmove");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(dest, src, count);
}

UChar * u_memset_android(UChar* dest, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, UChar, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memset");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, UChar, int32_t))dlsym(handle_common, func_name);
  return ptr(dest, c, count);
}

int32_t u_memcmp_android(const UChar* buf1, const UChar* buf2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memcmp");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(buf1, buf2, count);
}

int32_t u_memcmpCodePointOrder_android(const UChar* s1, const UChar* s2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memcmpCodePointOrder");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(s1, s2, count);
}

UChar * u_memchr_android(const UChar* s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memchr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar, int32_t))dlsym(handle_common, func_name);
  return ptr(s, c, count);
}

UChar * u_memchr32_android(const UChar* s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar32, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memchr32");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar32, int32_t))dlsym(handle_common, func_name);
  return ptr(s, c, count);
}

UChar * u_memrchr_android(const UChar* s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memrchr");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar, int32_t))dlsym(handle_common, func_name);
  return ptr(s, c, count);
}

UChar * u_memrchr32_android(const UChar* s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(const UChar*, UChar32, int32_t);
  char func_name[128];
  strcpy(func_name, "u_memrchr32");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(const UChar*, UChar32, int32_t))dlsym(handle_common, func_name);
  return ptr(s, c, count);
}

int32_t u_unescape_android(const char* src, UChar* dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_unescape");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(src, dest, destCapacity);
}

UChar32 u_unescapeAt_android(UNESCAPE_CHAR_AT charAt, int32_t* offset, int32_t length, void* context) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*);
  char func_name[128];
  strcpy(func_name, "u_unescapeAt");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*))dlsym(handle_common, func_name);
  return ptr(charAt, offset, length, context);
}

int32_t u_strToUpper_android(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToUpper");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToLower_android(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToLower");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToTitle_android(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UBreakIterator* titleIter, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToTitle");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}

int32_t u_strFoldCase_android(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFoldCase");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}

wchar_t * u_strToWCS_android(wchar_t* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  wchar_t * (*ptr)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToWCS");
  strcat(func_name, icudata_version);
  ptr = (wchar_t *(*)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar * u_strFromWCS_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const wchar_t* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromWCS");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char * u_strToUTF8_android(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  char * (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToUTF8");
  strcat(func_name, icudata_version);
  ptr = (char *(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar * u_strFromUTF8_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromUTF8");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char * u_strToUTF8WithSub_android(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  char * (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToUTF8WithSub");
  strcat(func_name, icudata_version);
  ptr = (char *(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar * u_strFromUTF8WithSub_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromUTF8WithSub");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar * u_strFromUTF8Lenient_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromUTF8Lenient");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32 * u_strToUTF32_android(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 * (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToUTF32");
  strcat(func_name, icudata_version);
  ptr = (UChar32 *(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar * u_strFromUTF32_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromUTF32");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32 * u_strToUTF32WithSub_android(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 * (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strToUTF32WithSub");
  strcat(func_name, icudata_version);
  ptr = (UChar32 *(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar * u_strFromUTF32WithSub_android(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar * (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_strFromUTF32WithSub");
  strcat(func_name, icudata_version);
  ptr = (UChar *(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

/* unicode/uchar.h */

UBool u_hasBinaryProperty_android(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32, UProperty);
  char func_name[128];
  strcpy(func_name, "u_hasBinaryProperty");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32, UProperty))dlsym(handle_common, func_name);
  return ptr(c, which);
}

UBool u_isUAlphabetic_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isUAlphabetic");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isULowercase_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isULowercase");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isUUppercase_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isUUppercase");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isUWhiteSpace_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isUWhiteSpace");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

int32_t u_getIntPropertyValue_android(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UProperty);
  char func_name[128];
  strcpy(func_name, "u_getIntPropertyValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32, UProperty))dlsym(handle_common, func_name);
  return ptr(c, which);
}

int32_t u_getIntPropertyMinValue_android(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty);
  char func_name[128];
  strcpy(func_name, "u_getIntPropertyMinValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UProperty))dlsym(handle_common, func_name);
  return ptr(which);
}

int32_t u_getIntPropertyMaxValue_android(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty);
  char func_name[128];
  strcpy(func_name, "u_getIntPropertyMaxValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UProperty))dlsym(handle_common, func_name);
  return ptr(which);
}

double u_getNumericValue_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_getNumericValue");
  strcat(func_name, icudata_version);
  ptr = (double(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_islower_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_islower");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isupper_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isupper");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_istitle_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_istitle");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isdigit_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isdigit");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isalpha_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isalpha");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isalnum_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isalnum");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isxdigit_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isxdigit");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_ispunct_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_ispunct");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isgraph_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isgraph");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isblank_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isblank");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isdefined_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isdefined");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isspace_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isspace");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isJavaSpaceChar_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isJavaSpaceChar");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isWhitespace_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isWhitespace");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_iscntrl_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_iscntrl");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isISOControl_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isISOControl");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isprint_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isprint");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isbase_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isbase");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UCharDirection u_charDirection_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UCharDirection (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_charDirection");
  strcat(func_name, icudata_version);
  ptr = (UCharDirection(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isMirrored_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isMirrored");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UChar32 u_charMirror_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_charMirror");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

int8_t u_charType_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_charType");
  strcat(func_name, icudata_version);
  ptr = (int8_t(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

void u_enumCharTypes_android(UCharEnumTypeRange* enumRange, const void* context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharEnumTypeRange*, const void*);
  char func_name[128];
  strcpy(func_name, "u_enumCharTypes");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharEnumTypeRange*, const void*))dlsym(handle_common, func_name);
  ptr(enumRange, context);
  return;
}

uint8_t u_getCombiningClass_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  uint8_t (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_getCombiningClass");
  strcat(func_name, icudata_version);
  ptr = (uint8_t(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

int32_t u_charDigitValue_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_charDigitValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBlockCode ublock_getCode_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBlockCode (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "ublock_getCode");
  strcat(func_name, icudata_version);
  ptr = (UBlockCode(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

int32_t u_charName_android(UChar32 code, UCharNameChoice nameChoice, char* buffer, int32_t bufferLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_charName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}

int32_t u_getISOComment_android(UChar32 c, char* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_getISOComment");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(c, dest, destCapacity, pErrorCode);
}

UChar32 u_charFromName_android(UCharNameChoice nameChoice, const char* name, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharNameChoice, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_charFromName");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UCharNameChoice, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(nameChoice, name, pErrorCode);
}

void u_enumCharNames_android(UChar32 start, UChar32 limit, UEnumCharNamesFn* fn, void* context, UCharNameChoice nameChoice, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_enumCharNames");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*))dlsym(handle_common, func_name);
  ptr(start, limit, fn, context, nameChoice, pErrorCode);
  return;
}

const char * u_getPropertyName_android(UProperty property, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UProperty, UPropertyNameChoice);
  char func_name[128];
  strcpy(func_name, "u_getPropertyName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UProperty, UPropertyNameChoice))dlsym(handle_common, func_name);
  return ptr(property, nameChoice);
}

UProperty u_getPropertyEnum_android(const char* alias) {
  pthread_once(&once_control, &init_icudata_version);
  UProperty (*ptr)(const char*);
  char func_name[128];
  strcpy(func_name, "u_getPropertyEnum");
  strcat(func_name, icudata_version);
  ptr = (UProperty(*)(const char*))dlsym(handle_common, func_name);
  return ptr(alias);
}

const char * u_getPropertyValueName_android(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UProperty, int32_t, UPropertyNameChoice);
  char func_name[128];
  strcpy(func_name, "u_getPropertyValueName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UProperty, int32_t, UPropertyNameChoice))dlsym(handle_common, func_name);
  return ptr(property, value, nameChoice);
}

int32_t u_getPropertyValueEnum_android(UProperty property, const char* alias) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty, const char*);
  char func_name[128];
  strcpy(func_name, "u_getPropertyValueEnum");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UProperty, const char*))dlsym(handle_common, func_name);
  return ptr(property, alias);
}

UBool u_isIDStart_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isIDStart");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isIDPart_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isIDPart");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isIDIgnorable_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isIDIgnorable");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isJavaIDStart_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isJavaIDStart");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UBool u_isJavaIDPart_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_isJavaIDPart");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UChar32 u_tolower_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_tolower");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UChar32 u_toupper_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_toupper");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UChar32 u_totitle_android(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  char func_name[128];
  strcpy(func_name, "u_totitle");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UChar32))dlsym(handle_common, func_name);
  return ptr(c);
}

UChar32 u_foldCase_android(UChar32 c, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32, uint32_t);
  char func_name[128];
  strcpy(func_name, "u_foldCase");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UChar32, uint32_t))dlsym(handle_common, func_name);
  return ptr(c, options);
}

int32_t u_digit_android(UChar32 ch, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, int8_t);
  char func_name[128];
  strcpy(func_name, "u_digit");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32, int8_t))dlsym(handle_common, func_name);
  return ptr(ch, radix);
}

UChar32 u_forDigit_android(int32_t digit, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(int32_t, int8_t);
  char func_name[128];
  strcpy(func_name, "u_forDigit");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(int32_t, int8_t))dlsym(handle_common, func_name);
  return ptr(digit, radix);
}

void u_charAge_android(UChar32 c, UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UChar32, UVersionInfo);
  char func_name[128];
  strcpy(func_name, "u_charAge");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UChar32, UVersionInfo))dlsym(handle_common, func_name);
  ptr(c, versionArray);
  return;
}

void u_getUnicodeVersion_android(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo);
  char func_name[128];
  strcpy(func_name, "u_getUnicodeVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo))dlsym(handle_common, func_name);
  ptr(versionArray);
  return;
}

int32_t u_getFC_NFKC_Closure_android(UChar32 c, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_getFC_NFKC_Closure");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UChar32, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(c, dest, destCapacity, pErrorCode);
}

/* unicode/uiter.h */

UChar32 uiter_current32_android(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  char func_name[128];
  strcpy(func_name, "uiter_current32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UCharIterator*))dlsym(handle_common, func_name);
  return ptr(iter);
}

UChar32 uiter_next32_android(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  char func_name[128];
  strcpy(func_name, "uiter_next32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UCharIterator*))dlsym(handle_common, func_name);
  return ptr(iter);
}

UChar32 uiter_previous32_android(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  char func_name[128];
  strcpy(func_name, "uiter_previous32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UCharIterator*))dlsym(handle_common, func_name);
  return ptr(iter);
}

uint32_t uiter_getState_android(const UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCharIterator*);
  char func_name[128];
  strcpy(func_name, "uiter_getState");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(const UCharIterator*))dlsym(handle_common, func_name);
  return ptr(iter);
}

void uiter_setState_android(UCharIterator* iter, uint32_t state, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uiter_setState");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharIterator*, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(iter, state, pErrorCode);
  return;
}

void uiter_setString_android(UCharIterator* iter, const UChar* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uiter_setString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharIterator*, const UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(iter, s, length);
  return;
}

void uiter_setUTF16BE_android(UCharIterator* iter, const char* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const char*, int32_t);
  char func_name[128];
  strcpy(func_name, "uiter_setUTF16BE");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))dlsym(handle_common, func_name);
  ptr(iter, s, length);
  return;
}

void uiter_setUTF8_android(UCharIterator* iter, const char* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const char*, int32_t);
  char func_name[128];
  strcpy(func_name, "uiter_setUTF8");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))dlsym(handle_common, func_name);
  ptr(iter, s, length);
  return;
}

/* unicode/utypes.h */

const char * u_errorName_android(UErrorCode code) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UErrorCode);
  char func_name[128];
  strcpy(func_name, "u_errorName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UErrorCode))dlsym(handle_common, func_name);
  return ptr(code);
}

/* unicode/udata.h */

UDataMemory * udata_open_android(const char* path, const char* type, const char* name, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDataMemory * (*ptr)(const char*, const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udata_open");
  strcat(func_name, icudata_version);
  ptr = (UDataMemory *(*)(const char*, const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(path, type, name, pErrorCode);
}

UDataMemory * udata_openChoice_android(const char* path, const char* type, const char* name, UDataMemoryIsAcceptable* isAcceptable, void* context, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDataMemory * (*ptr)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udata_openChoice");
  strcat(func_name, icudata_version);
  ptr = (UDataMemory *(*)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}

void udata_close_android(UDataMemory* pData) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataMemory*);
  char func_name[128];
  strcpy(func_name, "udata_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDataMemory*))dlsym(handle_common, func_name);
  ptr(pData);
  return;
}

const void * udata_getMemory_android(UDataMemory* pData) {
  pthread_once(&once_control, &init_icudata_version);
  const void * (*ptr)(UDataMemory*);
  char func_name[128];
  strcpy(func_name, "udata_getMemory");
  strcat(func_name, icudata_version);
  ptr = (const void *(*)(UDataMemory*))dlsym(handle_common, func_name);
  return ptr(pData);
}

void udata_getInfo_android(UDataMemory* pData, UDataInfo* pInfo) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataMemory*, UDataInfo*);
  char func_name[128];
  strcpy(func_name, "udata_getInfo");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDataMemory*, UDataInfo*))dlsym(handle_common, func_name);
  ptr(pData, pInfo);
  return;
}

void udata_setCommonData_android(const void* data, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udata_setCommonData");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(data, err);
  return;
}

void udata_setAppData_android(const char* packageName, const void* data, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const void*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udata_setAppData");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, const void*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(packageName, data, err);
  return;
}

void udata_setFileAccess_android(UDataFileAccess access, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataFileAccess, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "udata_setFileAccess");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UDataFileAccess, UErrorCode*))dlsym(handle_common, func_name);
  ptr(access, status);
  return;
}

/* unicode/ucasemap.h */

UCaseMap * ucasemap_open_android(const char* locale, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UCaseMap * (*ptr)(const char*, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_open");
  strcat(func_name, icudata_version);
  ptr = (UCaseMap *(*)(const char*, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, options, pErrorCode);
}

void ucasemap_close_android(UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*);
  char func_name[128];
  strcpy(func_name, "ucasemap_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCaseMap*))dlsym(handle_common, func_name);
  ptr(csm);
  return;
}

const char * ucasemap_getLocale_android(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UCaseMap*);
  char func_name[128];
  strcpy(func_name, "ucasemap_getLocale");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UCaseMap*))dlsym(handle_common, func_name);
  return ptr(csm);
}

uint32_t ucasemap_getOptions_android(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCaseMap*);
  char func_name[128];
  strcpy(func_name, "ucasemap_getOptions");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(const UCaseMap*))dlsym(handle_common, func_name);
  return ptr(csm);
}

void ucasemap_setLocale_android(UCaseMap* csm, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_setLocale");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCaseMap*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(csm, locale, pErrorCode);
  return;
}

void ucasemap_setOptions_android(UCaseMap* csm, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_setOptions");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCaseMap*, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(csm, options, pErrorCode);
  return;
}

const UBreakIterator * ucasemap_getBreakIterator_android(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  const UBreakIterator * (*ptr)(const UCaseMap*);
  char func_name[128];
  strcpy(func_name, "ucasemap_getBreakIterator");
  strcat(func_name, icudata_version);
  ptr = (const UBreakIterator *(*)(const UCaseMap*))dlsym(handle_common, func_name);
  return ptr(csm);
}

void ucasemap_setBreakIterator_android(UCaseMap* csm, UBreakIterator* iterToAdopt, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, UBreakIterator*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_setBreakIterator");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UCaseMap*, UBreakIterator*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(csm, iterToAdopt, pErrorCode);
  return;
}

int32_t ucasemap_toTitle_android(UCaseMap* csm, UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_toTitle");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToLower_android(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_utf8ToLower");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToUpper_android(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_utf8ToUpper");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToTitle_android(UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_utf8ToTitle");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8FoldCase_android(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucasemap_utf8FoldCase");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

/* unicode/ucnv_cb.h */

void ucnv_cbFromUWriteBytes_android(UConverterFromUnicodeArgs* args, const char* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_cbFromUWriteBytes");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteSub_android(UConverterFromUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_cbFromUWriteSub");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(args, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteUChars_android(UConverterFromUnicodeArgs* args, const UChar** source, const UChar* sourceLimit, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_cbFromUWriteUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(args, source, sourceLimit, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteUChars_android(UConverterToUnicodeArgs* args, const UChar* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_cbToUWriteUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteSub_android(UConverterToUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterToUnicodeArgs*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnv_cbToUWriteSub");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterToUnicodeArgs*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(args, offsetIndex, err);
  return;
}

/* unicode/uset.h */

USet * uset_openEmpty_android() {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "uset_openEmpty");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

USet * uset_open_android(UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_open");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(UChar32, UChar32))dlsym(handle_common, func_name);
  return ptr(start, end);
}

USet * uset_openPattern_android(const UChar* pattern, int32_t patternLength, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_openPattern");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pattern, patternLength, ec);
}

USet * uset_openPatternOptions_android(const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(const UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_openPatternOptions");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(const UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pattern, patternLength, options, ec);
}

void uset_close_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

USet * uset_clone_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_clone");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

UBool uset_isFrozen_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_isFrozen");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

void uset_freeze_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_freeze");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

USet * uset_cloneAsThawed_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  USet * (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_cloneAsThawed");
  strcat(func_name, icudata_version);
  ptr = (USet *(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

void uset_set_android(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_set");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32, UChar32))dlsym(handle_common, func_name);
  ptr(set, start, end);
  return;
}

int32_t uset_applyPattern_android(USet* set, const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_applyPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(set, pattern, patternLength, options, status);
}

void uset_applyIntPropertyValue_android(USet* set, UProperty prop, int32_t value, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UProperty, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_applyIntPropertyValue");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UProperty, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(set, prop, value, ec);
  return;
}

void uset_applyPropertyAlias_android(USet* set, const UChar* prop, int32_t propLength, const UChar* value, int32_t valueLength, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_applyPropertyAlias");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(set, prop, propLength, value, valueLength, ec);
  return;
}

UBool uset_resemblesPattern_android(const UChar* pattern, int32_t patternLength, int32_t pos) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_resemblesPattern");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))dlsym(handle_common, func_name);
  return ptr(pattern, patternLength, pos);
}

int32_t uset_toPattern_android(const USet* set, UChar* result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, UChar*, int32_t, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_toPattern");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, UChar*, int32_t, UBool, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}

void uset_add_android(USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_add");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32))dlsym(handle_common, func_name);
  ptr(set, c);
  return;
}

void uset_addAll_android(USet* set, const USet* additionalSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_addAll");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const USet*))dlsym(handle_common, func_name);
  ptr(set, additionalSet);
  return;
}

void uset_addRange_android(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_addRange");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32, UChar32))dlsym(handle_common, func_name);
  ptr(set, start, end);
  return;
}

void uset_addString_android(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_addString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(set, str, strLen);
  return;
}

void uset_addAllCodePoints_android(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_addAllCodePoints");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(set, str, strLen);
  return;
}

void uset_remove_android(USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_remove");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32))dlsym(handle_common, func_name);
  ptr(set, c);
  return;
}

void uset_removeRange_android(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_removeRange");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32, UChar32))dlsym(handle_common, func_name);
  ptr(set, start, end);
  return;
}

void uset_removeString_android(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_removeString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(set, str, strLen);
  return;
}

void uset_removeAll_android(USet* set, const USet* removeSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_removeAll");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const USet*))dlsym(handle_common, func_name);
  ptr(set, removeSet);
  return;
}

void uset_retain_android(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_retain");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, UChar32, UChar32))dlsym(handle_common, func_name);
  ptr(set, start, end);
  return;
}

void uset_retainAll_android(USet* set, const USet* retain) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_retainAll");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const USet*))dlsym(handle_common, func_name);
  ptr(set, retain);
  return;
}

void uset_compact_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_compact");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

void uset_complement_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_complement");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

void uset_complementAll_android(USet* set, const USet* complement) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_complementAll");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, const USet*))dlsym(handle_common, func_name);
  ptr(set, complement);
  return;
}

void uset_clear_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_clear");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

void uset_closeOver_android(USet* set, int32_t attributes) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_closeOver");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*, int32_t))dlsym(handle_common, func_name);
  ptr(set, attributes);
  return;
}

void uset_removeAllStrings_android(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  char func_name[128];
  strcpy(func_name, "uset_removeAllStrings");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USet*))dlsym(handle_common, func_name);
  ptr(set);
  return;
}

UBool uset_isEmpty_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_isEmpty");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

UBool uset_contains_android(const USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_contains");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, UChar32))dlsym(handle_common, func_name);
  return ptr(set, c);
}

UBool uset_containsRange_android(const USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, UChar32, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_containsRange");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, UChar32, UChar32))dlsym(handle_common, func_name);
  return ptr(set, start, end);
}

UBool uset_containsString_android(const USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_containsString");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(set, str, strLen);
}

int32_t uset_indexOf_android(const USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_indexOf");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, UChar32))dlsym(handle_common, func_name);
  return ptr(set, c);
}

UChar32 uset_charAt_android(const USet* set, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(const USet*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_charAt");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(const USet*, int32_t))dlsym(handle_common, func_name);
  return ptr(set, charIndex);
}

int32_t uset_size_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_size");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

int32_t uset_getItemCount_android(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*);
  char func_name[128];
  strcpy(func_name, "uset_getItemCount");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*))dlsym(handle_common, func_name);
  return ptr(set);
}

int32_t uset_getItem_android(const USet* set, int32_t itemIndex, UChar32* start, UChar32* end, UChar* str, int32_t strCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_getItem");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}

UBool uset_containsAll_android(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_containsAll");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const USet*))dlsym(handle_common, func_name);
  return ptr(set1, set2);
}

UBool uset_containsAllCodePoints_android(const USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_containsAllCodePoints");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))dlsym(handle_common, func_name);
  return ptr(set, str, strLen);
}

UBool uset_containsNone_android(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_containsNone");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const USet*))dlsym(handle_common, func_name);
  return ptr(set1, set2);
}

UBool uset_containsSome_android(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_containsSome");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const USet*))dlsym(handle_common, func_name);
  return ptr(set1, set2);
}

int32_t uset_span_android(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  char func_name[128];
  strcpy(func_name, "uset_span");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))dlsym(handle_common, func_name);
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBack_android(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  char func_name[128];
  strcpy(func_name, "uset_spanBack");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))dlsym(handle_common, func_name);
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanUTF8_android(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  char func_name[128];
  strcpy(func_name, "uset_spanUTF8");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))dlsym(handle_common, func_name);
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBackUTF8_android(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  char func_name[128];
  strcpy(func_name, "uset_spanBackUTF8");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))dlsym(handle_common, func_name);
  return ptr(set, s, length, spanCondition);
}

UBool uset_equals_android(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  char func_name[128];
  strcpy(func_name, "uset_equals");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USet*, const USet*))dlsym(handle_common, func_name);
  return ptr(set1, set2);
}

int32_t uset_serialize_android(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, uint16_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uset_serialize");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USet*, uint16_t*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(set, dest, destCapacity, pErrorCode);
}

UBool uset_getSerializedSet_android(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(USerializedSet*, const uint16_t*, int32_t);
  char func_name[128];
  strcpy(func_name, "uset_getSerializedSet");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(USerializedSet*, const uint16_t*, int32_t))dlsym(handle_common, func_name);
  return ptr(fillSet, src, srcLength);
}

void uset_setSerializedToOne_android(USerializedSet* fillSet, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USerializedSet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_setSerializedToOne");
  strcat(func_name, icudata_version);
  ptr = (void(*)(USerializedSet*, UChar32))dlsym(handle_common, func_name);
  ptr(fillSet, c);
  return;
}

UBool uset_serializedContains_android(const USerializedSet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USerializedSet*, UChar32);
  char func_name[128];
  strcpy(func_name, "uset_serializedContains");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USerializedSet*, UChar32))dlsym(handle_common, func_name);
  return ptr(set, c);
}

int32_t uset_getSerializedRangeCount_android(const USerializedSet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USerializedSet*);
  char func_name[128];
  strcpy(func_name, "uset_getSerializedRangeCount");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const USerializedSet*))dlsym(handle_common, func_name);
  return ptr(set);
}

UBool uset_getSerializedRange_android(const USerializedSet* set, int32_t rangeIndex, UChar32* pStart, UChar32* pEnd) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USerializedSet*, int32_t, UChar32*, UChar32*);
  char func_name[128];
  strcpy(func_name, "uset_getSerializedRange");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const USerializedSet*, int32_t, UChar32*, UChar32*))dlsym(handle_common, func_name);
  return ptr(set, rangeIndex, pStart, pEnd);
}

/* unicode/utext.h */

UText * utext_close_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UText * (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_close");
  strcat(func_name, icudata_version);
  ptr = (UText *(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UText * utext_openUTF8_android(UText* ut, const char* s, int64_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText * (*ptr)(UText*, const char*, int64_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_openUTF8");
  strcat(func_name, icudata_version);
  ptr = (UText *(*)(UText*, const char*, int64_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(ut, s, length, status);
}

UText * utext_openUChars_android(UText* ut, const UChar* s, int64_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText * (*ptr)(UText*, const UChar*, int64_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_openUChars");
  strcat(func_name, icudata_version);
  ptr = (UText *(*)(UText*, const UChar*, int64_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(ut, s, length, status);
}

UText * utext_clone_android(UText* dest, const UText* src, UBool deep, UBool readOnly, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText * (*ptr)(UText*, const UText*, UBool, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_clone");
  strcat(func_name, icudata_version);
  ptr = (UText *(*)(UText*, const UText*, UBool, UBool, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(dest, src, deep, readOnly, status);
}

UBool utext_equals_android(const UText* a, const UText* b) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*, const UText*);
  char func_name[128];
  strcpy(func_name, "utext_equals");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UText*, const UText*))dlsym(handle_common, func_name);
  return ptr(a, b);
}

int64_t utext_nativeLength_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_nativeLength");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UBool utext_isLengthExpensive_android(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  char func_name[128];
  strcpy(func_name, "utext_isLengthExpensive");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UChar32 utext_char32At_android(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  char func_name[128];
  strcpy(func_name, "utext_char32At");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*, int64_t))dlsym(handle_common, func_name);
  return ptr(ut, nativeIndex);
}

UChar32 utext_current32_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_current32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UChar32 utext_next32_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_next32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UChar32 utext_previous32_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_previous32");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UChar32 utext_next32From_android(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  char func_name[128];
  strcpy(func_name, "utext_next32From");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*, int64_t))dlsym(handle_common, func_name);
  return ptr(ut, nativeIndex);
}

UChar32 utext_previous32From_android(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  char func_name[128];
  strcpy(func_name, "utext_previous32From");
  strcat(func_name, icudata_version);
  ptr = (UChar32(*)(UText*, int64_t))dlsym(handle_common, func_name);
  return ptr(ut, nativeIndex);
}

int64_t utext_getNativeIndex_android(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const UText*);
  char func_name[128];
  strcpy(func_name, "utext_getNativeIndex");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(const UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

void utext_setNativeIndex_android(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*, int64_t);
  char func_name[128];
  strcpy(func_name, "utext_setNativeIndex");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UText*, int64_t))dlsym(handle_common, func_name);
  ptr(ut, nativeIndex);
  return;
}

UBool utext_moveIndex32_android(UText* ut, int32_t delta) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UText*, int32_t);
  char func_name[128];
  strcpy(func_name, "utext_moveIndex32");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UText*, int32_t))dlsym(handle_common, func_name);
  return ptr(ut, delta);
}

int64_t utext_getPreviousNativeIndex_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_getPreviousNativeIndex");
  strcat(func_name, icudata_version);
  ptr = (int64_t(*)(UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

int32_t utext_extract_android(UText* ut, int64_t nativeStart, int64_t nativeLimit, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_extract");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}

UBool utext_isWritable_android(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  char func_name[128];
  strcpy(func_name, "utext_isWritable");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

UBool utext_hasMetaData_android(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  char func_name[128];
  strcpy(func_name, "utext_hasMetaData");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UText*))dlsym(handle_common, func_name);
  return ptr(ut);
}

int32_t utext_replace_android(UText* ut, int64_t nativeStart, int64_t nativeLimit, const UChar* replacementText, int32_t replacementLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_replace");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}

void utext_copy_android(UText* ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_copy");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*))dlsym(handle_common, func_name);
  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
  return;
}

void utext_freeze_android(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*);
  char func_name[128];
  strcpy(func_name, "utext_freeze");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UText*))dlsym(handle_common, func_name);
  ptr(ut);
  return;
}

UText * utext_setup_android(UText* ut, int32_t extraSpace, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText * (*ptr)(UText*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "utext_setup");
  strcat(func_name, icudata_version);
  ptr = (UText *(*)(UText*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(ut, extraSpace, status);
}

/* unicode/ures.h */

UResourceBundle * ures_open_android(const char* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_open");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(packageName, locale, status);
}

UResourceBundle * ures_openDirect_android(const char* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_openDirect");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(packageName, locale, status);
}

UResourceBundle * ures_openU_android(const UChar* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(const UChar*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_openU");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(const UChar*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(packageName, locale, status);
}

void ures_close_android(UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UResourceBundle*))dlsym(handle_common, func_name);
  ptr(resourceBundle);
  return;
}

void ures_getVersion_android(const UResourceBundle* resB, UVersionInfo versionInfo) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UResourceBundle*, UVersionInfo);
  char func_name[128];
  strcpy(func_name, "ures_getVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UResourceBundle*, UVersionInfo))dlsym(handle_common, func_name);
  ptr(resB, versionInfo);
  return;
}

const char * ures_getLocaleByType_android(const UResourceBundle* resourceBundle, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, type, status);
}

const UChar * ures_getString_android(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getString");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UResourceBundle*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, len, status);
}

const char * ures_getUTF8String_android(const UResourceBundle* resB, char* dest, int32_t* length, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getUTF8String");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resB, dest, length, forceCopy, status);
}

const uint8_t * ures_getBinary_android(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const uint8_t * (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getBinary");
  strcat(func_name, icudata_version);
  ptr = (const uint8_t *(*)(const UResourceBundle*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, len, status);
}

const int32_t * ures_getIntVector_android(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const int32_t * (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getIntVector");
  strcat(func_name, icudata_version);
  ptr = (const int32_t *(*)(const UResourceBundle*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, len, status);
}

uint32_t ures_getUInt_android(const UResourceBundle* resourceBundle, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getUInt");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(const UResourceBundle*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, status);
}

int32_t ures_getInt_android(const UResourceBundle* resourceBundle, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getInt");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UResourceBundle*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, status);
}

int32_t ures_getSize_android(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_getSize");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UResourceBundle*))dlsym(handle_common, func_name);
  return ptr(resourceBundle);
}

UResType ures_getType_android(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  UResType (*ptr)(const UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_getType");
  strcat(func_name, icudata_version);
  ptr = (UResType(*)(const UResourceBundle*))dlsym(handle_common, func_name);
  return ptr(resourceBundle);
}

const char * ures_getKey_android(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_getKey");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UResourceBundle*))dlsym(handle_common, func_name);
  return ptr(resourceBundle);
}

void ures_resetIterator_android(UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_resetIterator");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UResourceBundle*))dlsym(handle_common, func_name);
  ptr(resourceBundle);
  return;
}

UBool ures_hasNext_android(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UResourceBundle*);
  char func_name[128];
  strcpy(func_name, "ures_hasNext");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UResourceBundle*))dlsym(handle_common, func_name);
  return ptr(resourceBundle);
}

UResourceBundle * ures_getNextResource_android(UResourceBundle* resourceBundle, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(UResourceBundle*, UResourceBundle*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getNextResource");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(UResourceBundle*, UResourceBundle*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, fillIn, status);
}

const UChar * ures_getNextString_android(UResourceBundle* resourceBundle, int32_t* len, const char** key, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(UResourceBundle*, int32_t*, const char**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getNextString");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(UResourceBundle*, int32_t*, const char**, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, len, key, status);
}

UResourceBundle * ures_getByIndex_android(const UResourceBundle* resourceBundle, int32_t indexR, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getByIndex");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, indexR, fillIn, status);
}

const UChar * ures_getStringByIndex_android(const UResourceBundle* resourceBundle, int32_t indexS, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getStringByIndex");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, indexS, len, status);
}

const char * ures_getUTF8StringByIndex_android(const UResourceBundle* resB, int32_t stringIndex, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getUTF8StringByIndex");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}

UResourceBundle * ures_getByKey_android(const UResourceBundle* resourceBundle, const char* key, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle * (*ptr)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getByKey");
  strcat(func_name, icudata_version);
  ptr = (UResourceBundle *(*)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resourceBundle, key, fillIn, status);
}

const UChar * ures_getStringByKey_android(const UResourceBundle* resB, const char* key, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UResourceBundle*, const char*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getStringByKey");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UResourceBundle*, const char*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resB, key, len, status);
}

const char * ures_getUTF8StringByKey_android(const UResourceBundle* resB, const char* key, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_getUTF8StringByKey");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(resB, key, dest, pLength, forceCopy, status);
}

UEnumeration * ures_openAvailableLocales_android(const char* packageName, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ures_openAvailableLocales");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(packageName, status);
}

/* unicode/ubidi.h */

UBiDi * ubidi_open_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDi * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ubidi_open");
  strcat(func_name, icudata_version);
  ptr = (UBiDi *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

UBiDi * ubidi_openSized_android(int32_t maxLength, int32_t maxRunCount, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDi * (*ptr)(int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_openSized");
  strcat(func_name, icudata_version);
  ptr = (UBiDi *(*)(int32_t, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(maxLength, maxRunCount, pErrorCode);
}

void ubidi_close_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*))dlsym(handle_common, func_name);
  ptr(pBiDi);
  return;
}

void ubidi_setInverse_android(UBiDi* pBiDi, UBool isInverse) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBool);
  char func_name[128];
  strcpy(func_name, "ubidi_setInverse");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, UBool))dlsym(handle_common, func_name);
  ptr(pBiDi, isInverse);
  return;
}

UBool ubidi_isInverse_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_isInverse");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

void ubidi_orderParagraphsLTR_android(UBiDi* pBiDi, UBool orderParagraphsLTR) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBool);
  char func_name[128];
  strcpy(func_name, "ubidi_orderParagraphsLTR");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, UBool))dlsym(handle_common, func_name);
  ptr(pBiDi, orderParagraphsLTR);
  return;
}

UBool ubidi_isOrderParagraphsLTR_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_isOrderParagraphsLTR");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

void ubidi_setReorderingMode_android(UBiDi* pBiDi, UBiDiReorderingMode reorderingMode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiReorderingMode);
  char func_name[128];
  strcpy(func_name, "ubidi_setReorderingMode");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, UBiDiReorderingMode))dlsym(handle_common, func_name);
  ptr(pBiDi, reorderingMode);
  return;
}

UBiDiReorderingMode ubidi_getReorderingMode_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiReorderingMode (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getReorderingMode");
  strcat(func_name, icudata_version);
  ptr = (UBiDiReorderingMode(*)(UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

void ubidi_setReorderingOptions_android(UBiDi* pBiDi, uint32_t reorderingOptions) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, uint32_t);
  char func_name[128];
  strcpy(func_name, "ubidi_setReorderingOptions");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, uint32_t))dlsym(handle_common, func_name);
  ptr(pBiDi, reorderingOptions);
  return;
}

uint32_t ubidi_getReorderingOptions_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getReorderingOptions");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

void ubidi_setPara_android(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_setPara");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
  return;
}

void ubidi_setLine_android(const UBiDi* pParaBiDi, int32_t start, int32_t limit, UBiDi* pLineBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_setLine");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
  return;
}

UBiDiDirection ubidi_getDirection_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiDirection (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getDirection");
  strcat(func_name, icudata_version);
  ptr = (UBiDiDirection(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

const UChar * ubidi_getText_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getText");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

int32_t ubidi_getLength_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getLength");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

UBiDiLevel ubidi_getParaLevel_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiLevel (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getParaLevel");
  strcat(func_name, icudata_version);
  ptr = (UBiDiLevel(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

int32_t ubidi_countParagraphs_android(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_countParagraphs");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

int32_t ubidi_getParagraph_android(const UBiDi* pBiDi, int32_t charIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getParagraph");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}

void ubidi_getParagraphByIndex_android(const UBiDi* pBiDi, int32_t paraIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getParagraphByIndex");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
  return;
}

UBiDiLevel ubidi_getLevelAt_android(const UBiDi* pBiDi, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiLevel (*ptr)(const UBiDi*, int32_t);
  char func_name[128];
  strcpy(func_name, "ubidi_getLevelAt");
  strcat(func_name, icudata_version);
  ptr = (UBiDiLevel(*)(const UBiDi*, int32_t))dlsym(handle_common, func_name);
  return ptr(pBiDi, charIndex);
}

const UBiDiLevel * ubidi_getLevels_android(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UBiDiLevel * (*ptr)(UBiDi*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getLevels");
  strcat(func_name, icudata_version);
  ptr = (const UBiDiLevel *(*)(UBiDi*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, pErrorCode);
}

void ubidi_getLogicalRun_android(const UBiDi* pBiDi, int32_t logicalPosition, int32_t* pLogicalLimit, UBiDiLevel* pLevel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*);
  char func_name[128];
  strcpy(func_name, "ubidi_getLogicalRun");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*))dlsym(handle_common, func_name);
  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
  return;
}

int32_t ubidi_countRuns_android(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_countRuns");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBiDi*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, pErrorCode);
}

UBiDiDirection ubidi_getVisualRun_android(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiDirection (*ptr)(UBiDi*, int32_t, int32_t*, int32_t*);
  char func_name[128];
  strcpy(func_name, "ubidi_getVisualRun");
  strcat(func_name, icudata_version);
  ptr = (UBiDiDirection(*)(UBiDi*, int32_t, int32_t*, int32_t*))dlsym(handle_common, func_name);
  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}

int32_t ubidi_getVisualIndex_android(UBiDi* pBiDi, int32_t logicalIndex, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getVisualIndex");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, logicalIndex, pErrorCode);
}

int32_t ubidi_getLogicalIndex_android(UBiDi* pBiDi, int32_t visualIndex, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getLogicalIndex");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, visualIndex, pErrorCode);
}

void ubidi_getLogicalMap_android(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getLogicalMap");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_getVisualMap_android(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_getVisualMap");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_reorderLogical_android(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  char func_name[128];
  strcpy(func_name, "ubidi_reorderLogical");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))dlsym(handle_common, func_name);
  ptr(levels, length, indexMap);
  return;
}

void ubidi_reorderVisual_android(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  char func_name[128];
  strcpy(func_name, "ubidi_reorderVisual");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))dlsym(handle_common, func_name);
  ptr(levels, length, indexMap);
  return;
}

void ubidi_invertMap_android(const int32_t* srcMap, int32_t* destMap, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const int32_t*, int32_t*, int32_t);
  char func_name[128];
  strcpy(func_name, "ubidi_invertMap");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const int32_t*, int32_t*, int32_t))dlsym(handle_common, func_name);
  ptr(srcMap, destMap, length);
  return;
}

int32_t ubidi_getProcessedLength_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getProcessedLength");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

int32_t ubidi_getResultLength_android(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  char func_name[128];
  strcpy(func_name, "ubidi_getResultLength");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UBiDi*))dlsym(handle_common, func_name);
  return ptr(pBiDi);
}

UCharDirection ubidi_getCustomizedClass_android(UBiDi* pBiDi, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UCharDirection (*ptr)(UBiDi*, UChar32);
  char func_name[128];
  strcpy(func_name, "ubidi_getCustomizedClass");
  strcat(func_name, icudata_version);
  ptr = (UCharDirection(*)(UBiDi*, UChar32))dlsym(handle_common, func_name);
  return ptr(pBiDi, c);
}

void ubidi_setClassCallback_android(UBiDi* pBiDi, UBiDiClassCallback* newFn, const void* newContext, UBiDiClassCallback** oldFn, const void** oldContext, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_setClassCallback");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*))dlsym(handle_common, func_name);
  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
  return;
}

void ubidi_getClassCallback_android(UBiDi* pBiDi, UBiDiClassCallback** fn, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiClassCallback**, const void**);
  char func_name[128];
  strcpy(func_name, "ubidi_getClassCallback");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBiDi*, UBiDiClassCallback**, const void**))dlsym(handle_common, func_name);
  ptr(pBiDi, fn, context);
  return;
}

int32_t ubidi_writeReordered_android(UBiDi* pBiDi, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_writeReordered");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}

int32_t ubidi_writeReverse_android(const UChar* src, int32_t srcLength, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubidi_writeReverse");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}

/* unicode/uclean.h */

void u_init_android(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_init");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UErrorCode*))dlsym(handle_common, func_name);
  ptr(status);
  return;
}

/* unicode/uscript.h */

int32_t uscript_getCode_android(const char* nameOrAbbrOrLocale, UScriptCode* fillIn, int32_t capacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UScriptCode*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uscript_getCode");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, UScriptCode*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}

const char * uscript_getName_android(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UScriptCode);
  char func_name[128];
  strcpy(func_name, "uscript_getName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UScriptCode))dlsym(handle_common, func_name);
  return ptr(scriptCode);
}

const char * uscript_getShortName_android(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UScriptCode);
  char func_name[128];
  strcpy(func_name, "uscript_getShortName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UScriptCode))dlsym(handle_common, func_name);
  return ptr(scriptCode);
}

UScriptCode uscript_getScript_android(UChar32 codepoint, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UScriptCode (*ptr)(UChar32, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uscript_getScript");
  strcat(func_name, icudata_version);
  ptr = (UScriptCode(*)(UChar32, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(codepoint, err);
}

/* unicode/ushape.h */

int32_t u_shapeArabic_android(const UChar* source, int32_t sourceLength, UChar* dest, int32_t destSize, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_shapeArabic");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}

/* unicode/utrace.h */

void utrace_setLevel_android(int32_t traceLevel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "utrace_setLevel");
  strcat(func_name, icudata_version);
  ptr = (void(*)(int32_t))dlsym(handle_common, func_name);
  ptr(traceLevel);
  return;
}

int32_t utrace_getLevel_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "utrace_getLevel");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

void utrace_setFunctions_android(const void* context, UTraceEntry* e, UTraceExit* x, UTraceData* d) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UTraceEntry*, UTraceExit*, UTraceData*);
  char func_name[128];
  strcpy(func_name, "utrace_setFunctions");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void*, UTraceEntry*, UTraceExit*, UTraceData*))dlsym(handle_common, func_name);
  ptr(context, e, x, d);
  return;
}

void utrace_getFunctions_android(const void** context, UTraceEntry** e, UTraceExit** x, UTraceData** d) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void**, UTraceEntry**, UTraceExit**, UTraceData**);
  char func_name[128];
  strcpy(func_name, "utrace_getFunctions");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const void**, UTraceEntry**, UTraceExit**, UTraceData**))dlsym(handle_common, func_name);
  ptr(context, e, x, d);
  return;
}

int32_t utrace_vformat_android(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, va_list args) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, va_list);
  char func_name[128];
  strcpy(func_name, "utrace_vformat");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, va_list))dlsym(handle_common, func_name);
  return ptr(outBuf, capacity, indent, fmt, args);
}

int32_t utrace_format_android(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, ...);
  char func_name[128];
  strcpy(func_name, "utrace_format");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, ...))dlsym(handle_common, func_name);
  va_list args;
  va_start(args, fmt);
  int32_t ret = ptr(outBuf, capacity, indent, fmt);
  va_end(args);
  return ret;
}

const char * utrace_functionName_android(int32_t fnNumber) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "utrace_functionName");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_common, func_name);
  return ptr(fnNumber);
}

/* unicode/putil.h */

const char * u_getDataDirectory_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "u_getDataDirectory");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

void u_setDataDirectory_android(const char* directory) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*);
  char func_name[128];
  strcpy(func_name, "u_setDataDirectory");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*))dlsym(handle_common, func_name);
  ptr(directory);
  return;
}

void u_charsToUChars_android(const char* cs, UChar* us, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, UChar*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_charsToUChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const char*, UChar*, int32_t))dlsym(handle_common, func_name);
  ptr(cs, us, length);
  return;
}

void u_UCharsToChars_android(const UChar* us, char* cs, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, char*, int32_t);
  char func_name[128];
  strcpy(func_name, "u_UCharsToChars");
  strcat(func_name, icudata_version);
  ptr = (void(*)(const UChar*, char*, int32_t))dlsym(handle_common, func_name);
  ptr(us, cs, length);
  return;
}

/* unicode/unorm.h */

int32_t unorm_normalize_android(const UChar* source, int32_t sourceLength, UNormalizationMode mode, int32_t options, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_normalize");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(source, sourceLength, mode, options, result, resultLength, status);
}

UNormalizationCheckResult unorm_quickCheck_android(const UChar* source, int32_t sourcelength, UNormalizationMode mode, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_quickCheck");
  strcat(func_name, icudata_version);
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(source, sourcelength, mode, status);
}

UNormalizationCheckResult unorm_quickCheckWithOptions_android(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_quickCheckWithOptions");
  strcat(func_name, icudata_version);
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, mode, options, pErrorCode);
}

UBool unorm_isNormalized_android(const UChar* src, int32_t srcLength, UNormalizationMode mode, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_isNormalized");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, mode, pErrorCode);
}

UBool unorm_isNormalizedWithOptions_android(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_isNormalizedWithOptions");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, mode, options, pErrorCode);
}

int32_t unorm_next_android(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_next");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_previous_android(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_previous");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_concatenate_android(const UChar* left, int32_t leftLength, const UChar* right, int32_t rightLength, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_concatenate");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(left, leftLength, right, rightLength, dest, destCapacity, mode, options, pErrorCode);
}

int32_t unorm_compare_android(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "unorm_compare");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

/* unicode/ubrk.h */

UBreakIterator * ubrk_open_android(UBreakIteratorType type, const char* locale, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator * (*ptr)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_open");
  strcat(func_name, icudata_version);
  ptr = (UBreakIterator *(*)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(type, locale, text, textLength, status);
}

UBreakIterator * ubrk_openRules_android(const UChar* rules, int32_t rulesLength, const UChar* text, int32_t textLength, UParseError* parseErr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator * (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_openRules");
  strcat(func_name, icudata_version);
  ptr = (UBreakIterator *(*)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}

UBreakIterator * ubrk_safeClone_android(const UBreakIterator* bi, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator * (*ptr)(const UBreakIterator*, void*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_safeClone");
  strcat(func_name, icudata_version);
  ptr = (UBreakIterator *(*)(const UBreakIterator*, void*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(bi, stackBuffer, pBufferSize, status);
}

void ubrk_close_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBreakIterator*))dlsym(handle_common, func_name);
  ptr(bi);
  return;
}

void ubrk_setText_android(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_setText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBreakIterator*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  ptr(bi, text, textLength, status);
  return;
}

void ubrk_setUText_android(UBreakIterator* bi, UText* text, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*, UText*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_setUText");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UBreakIterator*, UText*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(bi, text, status);
  return;
}

int32_t ubrk_current_android(const UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_current");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_next_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_next");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_previous_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_previous");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_first_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_first");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_last_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_last");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_preceding_android(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t);
  char func_name[128];
  strcpy(func_name, "ubrk_preceding");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))dlsym(handle_common, func_name);
  return ptr(bi, offset);
}

int32_t ubrk_following_android(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t);
  char func_name[128];
  strcpy(func_name, "ubrk_following");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))dlsym(handle_common, func_name);
  return ptr(bi, offset);
}

const char * ubrk_getAvailable_android(int32_t index) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "ubrk_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_common, func_name);
  return ptr(index);
}

int32_t ubrk_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "ubrk_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

UBool ubrk_isBoundary_android(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBreakIterator*, int32_t);
  char func_name[128];
  strcpy(func_name, "ubrk_isBoundary");
  strcat(func_name, icudata_version);
  ptr = (UBool(*)(UBreakIterator*, int32_t))dlsym(handle_common, func_name);
  return ptr(bi, offset);
}

int32_t ubrk_getRuleStatus_android(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  char func_name[128];
  strcpy(func_name, "ubrk_getRuleStatus");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*))dlsym(handle_common, func_name);
  return ptr(bi);
}

int32_t ubrk_getRuleStatusVec_android(UBreakIterator* bi, int32_t* fillInVec, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_getRuleStatusVec");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UBreakIterator*, int32_t*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(bi, fillInVec, capacity, status);
}

const char * ubrk_getLocaleByType_android(const UBreakIterator* bi, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ubrk_getLocaleByType");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(bi, type, status);
}

/* unicode/uloc.h */

int32_t uloc_getLanguage_android(const char* localeID, char* language, int32_t languageCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getLanguage");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, language, languageCapacity, err);
}

int32_t uloc_getScript_android(const char* localeID, char* script, int32_t scriptCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getScript");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, script, scriptCapacity, err);
}

int32_t uloc_getCountry_android(const char* localeID, char* country, int32_t countryCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getCountry");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, country, countryCapacity, err);
}

int32_t uloc_getVariant_android(const char* localeID, char* variant, int32_t variantCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getVariant");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, variant, variantCapacity, err);
}

int32_t uloc_getName_android(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, name, nameCapacity, err);
}

int32_t uloc_canonicalize_android(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_canonicalize");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, name, nameCapacity, err);
}

const char * uloc_getISO3Language_android(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*);
  char func_name[128];
  strcpy(func_name, "uloc_getISO3Language");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*))dlsym(handle_common, func_name);
  return ptr(localeID);
}

const char * uloc_getISO3Country_android(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(const char*);
  char func_name[128];
  strcpy(func_name, "uloc_getISO3Country");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(const char*))dlsym(handle_common, func_name);
  return ptr(localeID);
}

uint32_t uloc_getLCID_android(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const char*);
  char func_name[128];
  strcpy(func_name, "uloc_getLCID");
  strcat(func_name, icudata_version);
  ptr = (uint32_t(*)(const char*))dlsym(handle_common, func_name);
  return ptr(localeID);
}

int32_t uloc_getDisplayLanguage_android(const char* locale, const char* displayLocale, UChar* language, int32_t languageCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayLanguage");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, displayLocale, language, languageCapacity, status);
}

int32_t uloc_getDisplayScript_android(const char* locale, const char* displayLocale, UChar* script, int32_t scriptCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayScript");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, displayLocale, script, scriptCapacity, status);
}

int32_t uloc_getDisplayCountry_android(const char* locale, const char* displayLocale, UChar* country, int32_t countryCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayCountry");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, displayLocale, country, countryCapacity, status);
}

int32_t uloc_getDisplayVariant_android(const char* locale, const char* displayLocale, UChar* variant, int32_t variantCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayVariant");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, displayLocale, variant, variantCapacity, status);
}

int32_t uloc_getDisplayKeyword_android(const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayKeyword");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayKeywordValue_android(const char* locale, const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayKeywordValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayName_android(const char* localeID, const char* inLocaleID, UChar* result, int32_t maxResultSize, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getDisplayName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}

const char * uloc_getAvailable_android(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(int32_t);
  char func_name[128];
  strcpy(func_name, "uloc_getAvailable");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(int32_t))dlsym(handle_common, func_name);
  return ptr(n);
}

int32_t uloc_countAvailable_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "uloc_countAvailable");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

const char * const * uloc_getISOLanguages_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char * const * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "uloc_getISOLanguages");
  strcat(func_name, icudata_version);
  ptr = (const char * const *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

const char * const * uloc_getISOCountries_android(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char * const * (*ptr)(void);
  char func_name[128];
  strcpy(func_name, "uloc_getISOCountries");
  strcat(func_name, icudata_version);
  ptr = (const char * const *(*)(void))dlsym(handle_common, func_name);
  return ptr();
}

int32_t uloc_getParent_android(const char* localeID, char* parent, int32_t parentCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getParent");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, parent, parentCapacity, err);
}

int32_t uloc_getBaseName_android(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getBaseName");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, name, nameCapacity, err);
}

UEnumeration * uloc_openKeywords_android(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_openKeywords");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, status);
}

int32_t uloc_getKeywordValue_android(const char* localeID, const char* keywordName, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getKeywordValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}

int32_t uloc_setKeywordValue_android(const char* keywordName, const char* keywordValue, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_setKeywordValue");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}

ULayoutType uloc_getCharacterOrientation_android(const char* localeId, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULayoutType (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getCharacterOrientation");
  strcat(func_name, icudata_version);
  ptr = (ULayoutType(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeId, status);
}

ULayoutType uloc_getLineOrientation_android(const char* localeId, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULayoutType (*ptr)(const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getLineOrientation");
  strcat(func_name, icudata_version);
  ptr = (ULayoutType(*)(const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeId, status);
}

int32_t uloc_acceptLanguageFromHTTP_android(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char* httpAcceptLanguage, UEnumeration* availableLocales, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_acceptLanguageFromHTTP");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}

int32_t uloc_acceptLanguage_android(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char** acceptList, int32_t acceptListCount, UEnumeration* availableLocales, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_acceptLanguage");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}

int32_t uloc_getLocaleForLCID_android(uint32_t hostID, char* locale, int32_t localeCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(uint32_t, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_getLocaleForLCID");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(uint32_t, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(hostID, locale, localeCapacity, status);
}

int32_t uloc_addLikelySubtags_android(const char* localeID, char* maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_addLikelySubtags");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}

int32_t uloc_minimizeSubtags_android(const char* localeID, char* minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uloc_minimizeSubtags");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}

/* unicode/ucat.h */

u_nl_catd u_catopen_android(const char* name, const char* locale, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  u_nl_catd (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_catopen");
  strcat(func_name, icudata_version);
  ptr = (u_nl_catd(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(name, locale, ec);
}

void u_catclose_android(u_nl_catd catd) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(u_nl_catd);
  char func_name[128];
  strcpy(func_name, "u_catclose");
  strcat(func_name, icudata_version);
  ptr = (void(*)(u_nl_catd))dlsym(handle_common, func_name);
  ptr(catd);
  return;
}

const UChar * u_catgets_android(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar* s, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "u_catgets");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(catd, set_num, msg_num, s, len, ec);
}

/* unicode/ucnvsel.h */

UConverterSelector * ucnvsel_open_android(const char* const* converterList, int32_t converterListSize, const USet* excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterSelector * (*ptr)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_open");
  strcat(func_name, icudata_version);
  ptr = (UConverterSelector *(*)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}

void ucnvsel_close_android(UConverterSelector* sel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterSelector*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UConverterSelector*))dlsym(handle_common, func_name);
  ptr(sel);
  return;
}

UConverterSelector * ucnvsel_openFromSerialized_android(const void* buffer, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterSelector * (*ptr)(const void*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_openFromSerialized");
  strcat(func_name, icudata_version);
  ptr = (UConverterSelector *(*)(const void*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(buffer, length, status);
}

int32_t ucnvsel_serialize_android(const UConverterSelector* sel, void* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverterSelector*, void*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_serialize");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UConverterSelector*, void*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(sel, buffer, bufferCapacity, status);
}

UEnumeration * ucnvsel_selectForString_android(const UConverterSelector* sel, const UChar* s, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_selectForString");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(sel, s, length, status);
}

UEnumeration * ucnvsel_selectForUTF8_android(const UConverterSelector* sel, const char* s, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration * (*ptr)(const UConverterSelector*, const char*, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "ucnvsel_selectForUTF8");
  strcat(func_name, icudata_version);
  ptr = (UEnumeration *(*)(const UConverterSelector*, const char*, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(sel, s, length, status);
}

/* unicode/usprep.h */

UStringPrepProfile * usprep_open_android(const char* path, const char* fileName, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringPrepProfile * (*ptr)(const char*, const char*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usprep_open");
  strcat(func_name, icudata_version);
  ptr = (UStringPrepProfile *(*)(const char*, const char*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(path, fileName, status);
}

UStringPrepProfile * usprep_openByType_android(UStringPrepProfileType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringPrepProfile * (*ptr)(UStringPrepProfileType, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usprep_openByType");
  strcat(func_name, icudata_version);
  ptr = (UStringPrepProfile *(*)(UStringPrepProfileType, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(type, status);
}

void usprep_close_android(UStringPrepProfile* profile) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringPrepProfile*);
  char func_name[128];
  strcpy(func_name, "usprep_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UStringPrepProfile*))dlsym(handle_common, func_name);
  ptr(profile);
  return;
}

int32_t usprep_prepare_android(const UStringPrepProfile* prep, const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "usprep_prepare");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}

/* unicode/uversion.h */

void u_versionFromString_android(UVersionInfo versionArray, const char* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, const char*);
  char func_name[128];
  strcpy(func_name, "u_versionFromString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo, const char*))dlsym(handle_common, func_name);
  ptr(versionArray, versionString);
  return;
}

void u_versionFromUString_android(UVersionInfo versionArray, const UChar* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, const UChar*);
  char func_name[128];
  strcpy(func_name, "u_versionFromUString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo, const UChar*))dlsym(handle_common, func_name);
  ptr(versionArray, versionString);
  return;
}

void u_versionToString_android(UVersionInfo versionArray, char* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, char*);
  char func_name[128];
  strcpy(func_name, "u_versionToString");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo, char*))dlsym(handle_common, func_name);
  ptr(versionArray, versionString);
  return;
}

void u_getVersion_android(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo);
  char func_name[128];
  strcpy(func_name, "u_getVersion");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UVersionInfo))dlsym(handle_common, func_name);
  ptr(versionArray);
  return;
}

/* unicode/uidna.h */

int32_t uidna_toASCII_android(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uidna_toASCII");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_toUnicode_android(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uidna_toUnicode");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_IDNToASCII_android(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uidna_IDNToASCII");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_IDNToUnicode_android(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uidna_IDNToUnicode");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_compare_android(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, int32_t options, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, int32_t, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uidna_compare");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, int32_t, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(s1, length1, s2, length2, options, status);
}

/* unicode/uenum.h */

void uenum_close_android(UEnumeration* en) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UEnumeration*);
  char func_name[128];
  strcpy(func_name, "uenum_close");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UEnumeration*))dlsym(handle_common, func_name);
  ptr(en);
  return;
}

int32_t uenum_count_android(UEnumeration* en, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UEnumeration*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uenum_count");
  strcat(func_name, icudata_version);
  ptr = (int32_t(*)(UEnumeration*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(en, status);
}

const UChar * uenum_unext_android(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar * (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uenum_unext");
  strcat(func_name, icudata_version);
  ptr = (const UChar *(*)(UEnumeration*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(en, resultLength, status);
}

const char * uenum_next_android(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char * (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uenum_next");
  strcat(func_name, icudata_version);
  ptr = (const char *(*)(UEnumeration*, int32_t*, UErrorCode*))dlsym(handle_common, func_name);
  return ptr(en, resultLength, status);
}

void uenum_reset_android(UEnumeration* en, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UEnumeration*, UErrorCode*);
  char func_name[128];
  strcpy(func_name, "uenum_reset");
  strcat(func_name, icudata_version);
  ptr = (void(*)(UEnumeration*, UErrorCode*))dlsym(handle_common, func_name);
  ptr(en, status);
  return;
}



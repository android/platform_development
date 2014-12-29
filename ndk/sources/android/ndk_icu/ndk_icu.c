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

/* Allowed version number ranges between [44, 999].
 * 44 is the minimum supported ICU version that was shipped in
 * Gingerbread (2.3.3) devices.
 */
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3
#define ICUDATA_VERSION_MIN        44

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

static void* handle_i18n = NULL;
static void* handle_common = NULL;
static void* syms[768];

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

__attribute__((constructor))
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

  char func_name[128];
  strcpy(func_name, "ucurr_forLocale");
  strcat(func_name, icudata_version);
  syms[0] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_register");
  strcat(func_name, icudata_version);
  syms[1] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_unregister");
  strcat(func_name, icudata_version);
  syms[2] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getName");
  strcat(func_name, icudata_version);
  syms[3] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getPluralName");
  strcat(func_name, icudata_version);
  syms[4] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getDefaultFractionDigits");
  strcat(func_name, icudata_version);
  syms[5] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getRoundingIncrement");
  strcat(func_name, icudata_version);
  syms[6] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_openISOCurrencies");
  strcat(func_name, icudata_version);
  syms[7] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_countCurrencies");
  strcat(func_name, icudata_version);
  syms[8] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_forLocaleAndDate");
  strcat(func_name, icudata_version);
  syms[9] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[10] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_open");
  strcat(func_name, icudata_version);
  syms[11] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_close");
  strcat(func_name, icudata_version);
  syms[12] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_clone");
  strcat(func_name, icudata_version);
  syms[13] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_format");
  strcat(func_name, icudata_version);
  syms[14] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_parse");
  strcat(func_name, icudata_version);
  syms[15] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_parseCalendar");
  strcat(func_name, icudata_version);
  syms[16] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_isLenient");
  strcat(func_name, icudata_version);
  syms[17] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setLenient");
  strcat(func_name, icudata_version);
  syms[18] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getCalendar");
  strcat(func_name, icudata_version);
  syms[19] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setCalendar");
  strcat(func_name, icudata_version);
  syms[20] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getNumberFormat");
  strcat(func_name, icudata_version);
  syms[21] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setNumberFormat");
  strcat(func_name, icudata_version);
  syms[22] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getAvailable");
  strcat(func_name, icudata_version);
  syms[23] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_countAvailable");
  strcat(func_name, icudata_version);
  syms[24] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_get2DigitYearStart");
  strcat(func_name, icudata_version);
  syms[25] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_set2DigitYearStart");
  strcat(func_name, icudata_version);
  syms[26] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_toPattern");
  strcat(func_name, icudata_version);
  syms[27] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_applyPattern");
  strcat(func_name, icudata_version);
  syms[28] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getSymbols");
  strcat(func_name, icudata_version);
  syms[29] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_countSymbols");
  strcat(func_name, icudata_version);
  syms[30] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setSymbols");
  strcat(func_name, icudata_version);
  syms[31] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[32] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_open");
  strcat(func_name, icudata_version);
  syms[33] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_close");
  strcat(func_name, icudata_version);
  syms[34] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_clone");
  strcat(func_name, icudata_version);
  syms[35] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_format");
  strcat(func_name, icudata_version);
  syms[36] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatInt64");
  strcat(func_name, icudata_version);
  syms[37] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatDouble");
  strcat(func_name, icudata_version);
  syms[38] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatDoubleCurrency");
  strcat(func_name, icudata_version);
  syms[39] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parse");
  strcat(func_name, icudata_version);
  syms[40] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseInt64");
  strcat(func_name, icudata_version);
  syms[41] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseDouble");
  strcat(func_name, icudata_version);
  syms[42] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseDoubleCurrency");
  strcat(func_name, icudata_version);
  syms[43] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_applyPattern");
  strcat(func_name, icudata_version);
  syms[44] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getAvailable");
  strcat(func_name, icudata_version);
  syms[45] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_countAvailable");
  strcat(func_name, icudata_version);
  syms[46] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getAttribute");
  strcat(func_name, icudata_version);
  syms[47] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setAttribute");
  strcat(func_name, icudata_version);
  syms[48] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getDoubleAttribute");
  strcat(func_name, icudata_version);
  syms[49] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setDoubleAttribute");
  strcat(func_name, icudata_version);
  syms[50] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getTextAttribute");
  strcat(func_name, icudata_version);
  syms[51] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setTextAttribute");
  strcat(func_name, icudata_version);
  syms[52] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_toPattern");
  strcat(func_name, icudata_version);
  syms[53] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getSymbol");
  strcat(func_name, icudata_version);
  syms[54] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setSymbol");
  strcat(func_name, icudata_version);
  syms[55] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[56] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openElements");
  strcat(func_name, icudata_version);
  syms[57] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_keyHashCode");
  strcat(func_name, icudata_version);
  syms[58] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_closeElements");
  strcat(func_name, icudata_version);
  syms[59] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_reset");
  strcat(func_name, icudata_version);
  syms[60] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_next");
  strcat(func_name, icudata_version);
  syms[61] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_previous");
  strcat(func_name, icudata_version);
  syms[62] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getMaxExpansion");
  strcat(func_name, icudata_version);
  syms[63] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setText");
  strcat(func_name, icudata_version);
  syms[64] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getOffset");
  strcat(func_name, icudata_version);
  syms[65] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setOffset");
  strcat(func_name, icudata_version);
  syms[66] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_primaryOrder");
  strcat(func_name, icudata_version);
  syms[67] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_secondaryOrder");
  strcat(func_name, icudata_version);
  syms[68] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_tertiaryOrder");
  strcat(func_name, icudata_version);
  syms[69] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openU");
  strcat(func_name, icudata_version);
  syms[70] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openInverse");
  strcat(func_name, icudata_version);
  syms[71] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_clone");
  strcat(func_name, icudata_version);
  syms[72] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_close");
  strcat(func_name, icudata_version);
  syms[73] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_getUnicodeID");
  strcat(func_name, icudata_version);
  syms[74] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_register");
  strcat(func_name, icudata_version);
  syms[75] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_unregisterID");
  strcat(func_name, icudata_version);
  syms[76] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_setFilter");
  strcat(func_name, icudata_version);
  syms[77] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_countAvailableIDs");
  strcat(func_name, icudata_version);
  syms[78] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openIDs");
  strcat(func_name, icudata_version);
  syms[79] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_trans");
  strcat(func_name, icudata_version);
  syms[80] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transIncremental");
  strcat(func_name, icudata_version);
  syms[81] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transUChars");
  strcat(func_name, icudata_version);
  syms[82] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transIncrementalUChars");
  strcat(func_name, icudata_version);
  syms[83] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_open");
  strcat(func_name, icudata_version);
  syms[84] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_openFromCollator");
  strcat(func_name, icudata_version);
  syms[85] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_close");
  strcat(func_name, icudata_version);
  syms[86] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setOffset");
  strcat(func_name, icudata_version);
  syms[87] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getOffset");
  strcat(func_name, icudata_version);
  syms[88] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setAttribute");
  strcat(func_name, icudata_version);
  syms[89] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getAttribute");
  strcat(func_name, icudata_version);
  syms[90] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedStart");
  strcat(func_name, icudata_version);
  syms[91] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedLength");
  strcat(func_name, icudata_version);
  syms[92] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedText");
  strcat(func_name, icudata_version);
  syms[93] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setBreakIterator");
  strcat(func_name, icudata_version);
  syms[94] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getBreakIterator");
  strcat(func_name, icudata_version);
  syms[95] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setText");
  strcat(func_name, icudata_version);
  syms[96] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getText");
  strcat(func_name, icudata_version);
  syms[97] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getCollator");
  strcat(func_name, icudata_version);
  syms[98] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setCollator");
  strcat(func_name, icudata_version);
  syms[99] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setPattern");
  strcat(func_name, icudata_version);
  syms[100] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getPattern");
  strcat(func_name, icudata_version);
  syms[101] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_first");
  strcat(func_name, icudata_version);
  syms[102] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_following");
  strcat(func_name, icudata_version);
  syms[103] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_last");
  strcat(func_name, icudata_version);
  syms[104] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_preceding");
  strcat(func_name, icudata_version);
  syms[105] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_next");
  strcat(func_name, icudata_version);
  syms[106] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_previous");
  strcat(func_name, icudata_version);
  syms[107] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_reset");
  strcat(func_name, icudata_version);
  syms[108] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessage");
  strcat(func_name, icudata_version);
  syms[109] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessage");
  strcat(func_name, icudata_version);
  syms[110] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessage");
  strcat(func_name, icudata_version);
  syms[111] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessage");
  strcat(func_name, icudata_version);
  syms[112] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessageWithError");
  strcat(func_name, icudata_version);
  syms[113] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessageWithError");
  strcat(func_name, icudata_version);
  syms[114] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessageWithError");
  strcat(func_name, icudata_version);
  syms[115] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessageWithError");
  strcat(func_name, icudata_version);
  syms[116] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_open");
  strcat(func_name, icudata_version);
  syms[117] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_close");
  strcat(func_name, icudata_version);
  syms[118] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_clone");
  strcat(func_name, icudata_version);
  syms[119] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_setLocale");
  strcat(func_name, icudata_version);
  syms[120] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_getLocale");
  strcat(func_name, icudata_version);
  syms[121] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_applyPattern");
  strcat(func_name, icudata_version);
  syms[122] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_toPattern");
  strcat(func_name, icudata_version);
  syms[123] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vformat");
  strcat(func_name, icudata_version);
  syms[124] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vformat");
  strcat(func_name, icudata_version);
  syms[125] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vparse");
  strcat(func_name, icudata_version);
  syms[126] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vparse");
  strcat(func_name, icudata_version);
  syms[127] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_autoQuoteApostrophe");
  strcat(func_name, icudata_version);
  syms[128] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_open");
  strcat(func_name, icudata_version);
  syms[129] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_close");
  strcat(func_name, icudata_version);
  syms[130] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_setNoSubstitute");
  strcat(func_name, icudata_version);
  syms[131] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getNoSubstitute");
  strcat(func_name, icudata_version);
  syms[132] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getExemplarSet");
  strcat(func_name, icudata_version);
  syms[133] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getDelimiter");
  strcat(func_name, icudata_version);
  syms[134] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getMeasurementSystem");
  strcat(func_name, icudata_version);
  syms[135] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getPaperSize");
  strcat(func_name, icudata_version);
  syms[136] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getCLDRVersion");
  strcat(func_name, icudata_version);
  syms[137] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getLocaleDisplayPattern");
  strcat(func_name, icudata_version);
  syms[138] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getLocaleSeparator");
  strcat(func_name, icudata_version);
  syms[139] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_open");
  strcat(func_name, icudata_version);
  syms[140] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openRules");
  strcat(func_name, icudata_version);
  syms[141] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getContractionsAndExpansions");
  strcat(func_name, icudata_version);
  syms[142] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_close");
  strcat(func_name, icudata_version);
  syms[143] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_strcoll");
  strcat(func_name, icudata_version);
  syms[144] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_greater");
  strcat(func_name, icudata_version);
  syms[145] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_greaterOrEqual");
  strcat(func_name, icudata_version);
  syms[146] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_equal");
  strcat(func_name, icudata_version);
  syms[147] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_strcollIter");
  strcat(func_name, icudata_version);
  syms[148] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getStrength");
  strcat(func_name, icudata_version);
  syms[149] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setStrength");
  strcat(func_name, icudata_version);
  syms[150] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getDisplayName");
  strcat(func_name, icudata_version);
  syms[151] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getAvailable");
  strcat(func_name, icudata_version);
  syms[152] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_countAvailable");
  strcat(func_name, icudata_version);
  syms[153] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openAvailableLocales");
  strcat(func_name, icudata_version);
  syms[154] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywords");
  strcat(func_name, icudata_version);
  syms[155] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywordValues");
  strcat(func_name, icudata_version);
  syms[156] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[157] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getFunctionalEquivalent");
  strcat(func_name, icudata_version);
  syms[158] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getRules");
  strcat(func_name, icudata_version);
  syms[159] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getSortKey");
  strcat(func_name, icudata_version);
  syms[160] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_nextSortKeyPart");
  strcat(func_name, icudata_version);
  syms[161] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getBound");
  strcat(func_name, icudata_version);
  syms[162] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getVersion");
  strcat(func_name, icudata_version);
  syms[163] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getUCAVersion");
  strcat(func_name, icudata_version);
  syms[164] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_mergeSortkeys");
  strcat(func_name, icudata_version);
  syms[165] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setAttribute");
  strcat(func_name, icudata_version);
  syms[166] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getAttribute");
  strcat(func_name, icudata_version);
  syms[167] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getVariableTop");
  strcat(func_name, icudata_version);
  syms[168] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_safeClone");
  strcat(func_name, icudata_version);
  syms[169] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getRulesEx");
  strcat(func_name, icudata_version);
  syms[170] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[171] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getTailoredSet");
  strcat(func_name, icudata_version);
  syms[172] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_cloneBinary");
  strcat(func_name, icudata_version);
  syms[173] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openBinary");
  strcat(func_name, icudata_version);
  syms[174] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_getTimeScaleValue");
  strcat(func_name, icudata_version);
  syms[175] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_fromInt64");
  strcat(func_name, icudata_version);
  syms[176] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_toInt64");
  strcat(func_name, icudata_version);
  syms[177] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_open");
  strcat(func_name, icudata_version);
  syms[178] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_openC");
  strcat(func_name, icudata_version);
  syms[179] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_close");
  strcat(func_name, icudata_version);
  syms[180] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_clone");
  strcat(func_name, icudata_version);
  syms[181] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_pattern");
  strcat(func_name, icudata_version);
  syms[182] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_flags");
  strcat(func_name, icudata_version);
  syms[183] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setText");
  strcat(func_name, icudata_version);
  syms[184] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getText");
  strcat(func_name, icudata_version);
  syms[185] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_matches");
  strcat(func_name, icudata_version);
  syms[186] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_lookingAt");
  strcat(func_name, icudata_version);
  syms[187] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_find");
  strcat(func_name, icudata_version);
  syms[188] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_findNext");
  strcat(func_name, icudata_version);
  syms[189] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_groupCount");
  strcat(func_name, icudata_version);
  syms[190] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_group");
  strcat(func_name, icudata_version);
  syms[191] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_start");
  strcat(func_name, icudata_version);
  syms[192] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_end");
  strcat(func_name, icudata_version);
  syms[193] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_reset");
  strcat(func_name, icudata_version);
  syms[194] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setRegion");
  strcat(func_name, icudata_version);
  syms[195] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionStart");
  strcat(func_name, icudata_version);
  syms[196] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionEnd");
  strcat(func_name, icudata_version);
  syms[197] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hasTransparentBounds");
  strcat(func_name, icudata_version);
  syms[198] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_useTransparentBounds");
  strcat(func_name, icudata_version);
  syms[199] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hasAnchoringBounds");
  strcat(func_name, icudata_version);
  syms[200] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_useAnchoringBounds");
  strcat(func_name, icudata_version);
  syms[201] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hitEnd");
  strcat(func_name, icudata_version);
  syms[202] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_requireEnd");
  strcat(func_name, icudata_version);
  syms[203] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceAll");
  strcat(func_name, icudata_version);
  syms[204] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceFirst");
  strcat(func_name, icudata_version);
  syms[205] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendReplacement");
  strcat(func_name, icudata_version);
  syms[206] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendTail");
  strcat(func_name, icudata_version);
  syms[207] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_split");
  strcat(func_name, icudata_version);
  syms[208] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setTimeLimit");
  strcat(func_name, icudata_version);
  syms[209] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getTimeLimit");
  strcat(func_name, icudata_version);
  syms[210] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setStackLimit");
  strcat(func_name, icudata_version);
  syms[211] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getStackLimit");
  strcat(func_name, icudata_version);
  syms[212] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setMatchCallback");
  strcat(func_name, icudata_version);
  syms[213] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getMatchCallback");
  strcat(func_name, icudata_version);
  syms[214] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_open");
  strcat(func_name, icudata_version);
  syms[215] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_close");
  strcat(func_name, icudata_version);
  syms[216] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_clone");
  strcat(func_name, icudata_version);
  syms[217] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setChecks");
  strcat(func_name, icudata_version);
  syms[218] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getChecks");
  strcat(func_name, icudata_version);
  syms[219] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setAllowedLocales");
  strcat(func_name, icudata_version);
  syms[220] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getAllowedLocales");
  strcat(func_name, icudata_version);
  syms[221] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setAllowedChars");
  strcat(func_name, icudata_version);
  syms[222] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getAllowedChars");
  strcat(func_name, icudata_version);
  syms[223] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_check");
  strcat(func_name, icudata_version);
  syms[224] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_checkUTF8");
  strcat(func_name, icudata_version);
  syms[225] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_areConfusable");
  strcat(func_name, icudata_version);
  syms[226] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_areConfusableUTF8");
  strcat(func_name, icudata_version);
  syms[227] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getSkeleton");
  strcat(func_name, icudata_version);
  syms[228] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getSkeletonUTF8");
  strcat(func_name, icudata_version);
  syms[229] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_open");
  strcat(func_name, icudata_version);
  syms[230] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openEmpty");
  strcat(func_name, icudata_version);
  syms[231] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_close");
  strcat(func_name, icudata_version);
  syms[232] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_clone");
  strcat(func_name, icudata_version);
  syms[233] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getBestPattern");
  strcat(func_name, icudata_version);
  syms[234] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getSkeleton");
  strcat(func_name, icudata_version);
  syms[235] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getBaseSkeleton");
  strcat(func_name, icudata_version);
  syms[236] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_addPattern");
  strcat(func_name, icudata_version);
  syms[237] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setAppendItemFormat");
  strcat(func_name, icudata_version);
  syms[238] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getAppendItemFormat");
  strcat(func_name, icudata_version);
  syms[239] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setAppendItemName");
  strcat(func_name, icudata_version);
  syms[240] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getAppendItemName");
  strcat(func_name, icudata_version);
  syms[241] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setDateTimeFormat");
  strcat(func_name, icudata_version);
  syms[242] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getDateTimeFormat");
  strcat(func_name, icudata_version);
  syms[243] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setDecimal");
  strcat(func_name, icudata_version);
  syms[244] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getDecimal");
  strcat(func_name, icudata_version);
  syms[245] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_replaceFieldTypes");
  strcat(func_name, icudata_version);
  syms[246] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openSkeletons");
  strcat(func_name, icudata_version);
  syms[247] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openBaseSkeletons");
  strcat(func_name, icudata_version);
  syms[248] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getPatternForSkeleton");
  strcat(func_name, icudata_version);
  syms[249] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_open");
  strcat(func_name, icudata_version);
  syms[250] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_close");
  strcat(func_name, icudata_version);
  syms[251] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_setText");
  strcat(func_name, icudata_version);
  syms[252] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_setDeclaredEncoding");
  strcat(func_name, icudata_version);
  syms[253] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_detect");
  strcat(func_name, icudata_version);
  syms[254] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_detectAll");
  strcat(func_name, icudata_version);
  syms[255] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getName");
  strcat(func_name, icudata_version);
  syms[256] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getConfidence");
  strcat(func_name, icudata_version);
  syms[257] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getLanguage");
  strcat(func_name, icudata_version);
  syms[258] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getUChars");
  strcat(func_name, icudata_version);
  syms[259] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getAllDetectableCharsets");
  strcat(func_name, icudata_version);
  syms[260] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_isInputFilterEnabled");
  strcat(func_name, icudata_version);
  syms[261] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_enableInputFilter");
  strcat(func_name, icudata_version);
  syms[262] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_openTimeZones");
  strcat(func_name, icudata_version);
  syms[263] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_openCountryTimeZones");
  strcat(func_name, icudata_version);
  syms[264] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getDefaultTimeZone");
  strcat(func_name, icudata_version);
  syms[265] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDefaultTimeZone");
  strcat(func_name, icudata_version);
  syms[266] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getDSTSavings");
  strcat(func_name, icudata_version);
  syms[267] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getNow");
  strcat(func_name, icudata_version);
  syms[268] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_open");
  strcat(func_name, icudata_version);
  syms[269] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_close");
  strcat(func_name, icudata_version);
  syms[270] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clone");
  strcat(func_name, icudata_version);
  syms[271] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setTimeZone");
  strcat(func_name, icudata_version);
  syms[272] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTimeZoneDisplayName");
  strcat(func_name, icudata_version);
  syms[273] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_inDaylightTime");
  strcat(func_name, icudata_version);
  syms[274] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setGregorianChange");
  strcat(func_name, icudata_version);
  syms[275] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getGregorianChange");
  strcat(func_name, icudata_version);
  syms[276] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getAttribute");
  strcat(func_name, icudata_version);
  syms[277] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setAttribute");
  strcat(func_name, icudata_version);
  syms[278] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getAvailable");
  strcat(func_name, icudata_version);
  syms[279] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_countAvailable");
  strcat(func_name, icudata_version);
  syms[280] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getMillis");
  strcat(func_name, icudata_version);
  syms[281] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setMillis");
  strcat(func_name, icudata_version);
  syms[282] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDate");
  strcat(func_name, icudata_version);
  syms[283] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDateTime");
  strcat(func_name, icudata_version);
  syms[284] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_equivalentTo");
  strcat(func_name, icudata_version);
  syms[285] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_add");
  strcat(func_name, icudata_version);
  syms[286] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_roll");
  strcat(func_name, icudata_version);
  syms[287] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_get");
  strcat(func_name, icudata_version);
  syms[288] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_set");
  strcat(func_name, icudata_version);
  syms[289] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_isSet");
  strcat(func_name, icudata_version);
  syms[290] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clearField");
  strcat(func_name, icudata_version);
  syms[291] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clear");
  strcat(func_name, icudata_version);
  syms[292] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getLimit");
  strcat(func_name, icudata_version);
  syms[293] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[294] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTZDataVersion");
  strcat(func_name, icudata_version);
  syms[295] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getCanonicalTimeZoneID");
  strcat(func_name, icudata_version);
  syms[296] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getType");
  strcat(func_name, icudata_version);
  syms[297] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[298] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  syms[299] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  syms[300] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  syms[301] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  syms[302] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  syms[303] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  syms[304] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  syms[305] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  syms[306] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_compareNames");
  strcat(func_name, icudata_version);
  syms[307] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_open");
  strcat(func_name, icudata_version);
  syms[308] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openU");
  strcat(func_name, icudata_version);
  syms[309] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openCCSID");
  strcat(func_name, icudata_version);
  syms[310] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openPackage");
  strcat(func_name, icudata_version);
  syms[311] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_safeClone");
  strcat(func_name, icudata_version);
  syms[312] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_close");
  strcat(func_name, icudata_version);
  syms[313] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getSubstChars");
  strcat(func_name, icudata_version);
  syms[314] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setSubstChars");
  strcat(func_name, icudata_version);
  syms[315] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setSubstString");
  strcat(func_name, icudata_version);
  syms[316] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getInvalidChars");
  strcat(func_name, icudata_version);
  syms[317] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getInvalidUChars");
  strcat(func_name, icudata_version);
  syms[318] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_reset");
  strcat(func_name, icudata_version);
  syms[319] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_resetToUnicode");
  strcat(func_name, icudata_version);
  syms[320] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_resetFromUnicode");
  strcat(func_name, icudata_version);
  syms[321] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getMaxCharSize");
  strcat(func_name, icudata_version);
  syms[322] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getMinCharSize");
  strcat(func_name, icudata_version);
  syms[323] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getDisplayName");
  strcat(func_name, icudata_version);
  syms[324] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getName");
  strcat(func_name, icudata_version);
  syms[325] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getCCSID");
  strcat(func_name, icudata_version);
  syms[326] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getPlatform");
  strcat(func_name, icudata_version);
  syms[327] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getType");
  strcat(func_name, icudata_version);
  syms[328] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStarters");
  strcat(func_name, icudata_version);
  syms[329] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getUnicodeSet");
  strcat(func_name, icudata_version);
  syms[330] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getToUCallBack");
  strcat(func_name, icudata_version);
  syms[331] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getFromUCallBack");
  strcat(func_name, icudata_version);
  syms[332] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setToUCallBack");
  strcat(func_name, icudata_version);
  syms[333] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setFromUCallBack");
  strcat(func_name, icudata_version);
  syms[334] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUnicode");
  strcat(func_name, icudata_version);
  syms[335] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUnicode");
  strcat(func_name, icudata_version);
  syms[336] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUChars");
  strcat(func_name, icudata_version);
  syms[337] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUChars");
  strcat(func_name, icudata_version);
  syms[338] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getNextUChar");
  strcat(func_name, icudata_version);
  syms[339] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_convertEx");
  strcat(func_name, icudata_version);
  syms[340] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_convert");
  strcat(func_name, icudata_version);
  syms[341] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toAlgorithmic");
  strcat(func_name, icudata_version);
  syms[342] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromAlgorithmic");
  strcat(func_name, icudata_version);
  syms[343] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_flushCache");
  strcat(func_name, icudata_version);
  syms[344] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countAvailable");
  strcat(func_name, icudata_version);
  syms[345] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAvailableName");
  strcat(func_name, icudata_version);
  syms[346] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openAllNames");
  strcat(func_name, icudata_version);
  syms[347] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countAliases");
  strcat(func_name, icudata_version);
  syms[348] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAlias");
  strcat(func_name, icudata_version);
  syms[349] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAliases");
  strcat(func_name, icudata_version);
  syms[350] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openStandardNames");
  strcat(func_name, icudata_version);
  syms[351] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countStandards");
  strcat(func_name, icudata_version);
  syms[352] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStandard");
  strcat(func_name, icudata_version);
  syms[353] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStandardName");
  strcat(func_name, icudata_version);
  syms[354] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getCanonicalName");
  strcat(func_name, icudata_version);
  syms[355] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getDefaultName");
  strcat(func_name, icudata_version);
  syms[356] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setDefaultName");
  strcat(func_name, icudata_version);
  syms[357] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fixFileSeparator");
  strcat(func_name, icudata_version);
  syms[358] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_isAmbiguous");
  strcat(func_name, icudata_version);
  syms[359] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setFallback");
  strcat(func_name, icudata_version);
  syms[360] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_usesFallback");
  strcat(func_name, icudata_version);
  syms[361] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_detectUnicodeSignature");
  strcat(func_name, icudata_version);
  syms[362] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUCountPending");
  strcat(func_name, icudata_version);
  syms[363] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUCountPending");
  strcat(func_name, icudata_version);
  syms[364] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strlen");
  strcat(func_name, icudata_version);
  syms[365] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_countChar32");
  strcat(func_name, icudata_version);
  syms[366] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strHasMoreChar32Than");
  strcat(func_name, icudata_version);
  syms[367] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcat");
  strcat(func_name, icudata_version);
  syms[368] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncat");
  strcat(func_name, icudata_version);
  syms[369] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strstr");
  strcat(func_name, icudata_version);
  syms[370] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFindFirst");
  strcat(func_name, icudata_version);
  syms[371] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strchr");
  strcat(func_name, icudata_version);
  syms[372] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strchr32");
  strcat(func_name, icudata_version);
  syms[373] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrstr");
  strcat(func_name, icudata_version);
  syms[374] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFindLast");
  strcat(func_name, icudata_version);
  syms[375] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrchr");
  strcat(func_name, icudata_version);
  syms[376] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrchr32");
  strcat(func_name, icudata_version);
  syms[377] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strpbrk");
  strcat(func_name, icudata_version);
  syms[378] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcspn");
  strcat(func_name, icudata_version);
  syms[379] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strspn");
  strcat(func_name, icudata_version);
  syms[380] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strtok_r");
  strcat(func_name, icudata_version);
  syms[381] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcmp");
  strcat(func_name, icudata_version);
  syms[382] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[383] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCompare");
  strcat(func_name, icudata_version);
  syms[384] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCompareIter");
  strcat(func_name, icudata_version);
  syms[385] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCaseCompare");
  strcat(func_name, icudata_version);
  syms[386] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncmp");
  strcat(func_name, icudata_version);
  syms[387] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[388] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcasecmp");
  strcat(func_name, icudata_version);
  syms[389] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncasecmp");
  strcat(func_name, icudata_version);
  syms[390] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcasecmp");
  strcat(func_name, icudata_version);
  syms[391] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcpy");
  strcat(func_name, icudata_version);
  syms[392] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncpy");
  strcat(func_name, icudata_version);
  syms[393] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_uastrcpy");
  strcat(func_name, icudata_version);
  syms[394] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_uastrncpy");
  strcat(func_name, icudata_version);
  syms[395] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_austrcpy");
  strcat(func_name, icudata_version);
  syms[396] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_austrncpy");
  strcat(func_name, icudata_version);
  syms[397] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcpy");
  strcat(func_name, icudata_version);
  syms[398] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memmove");
  strcat(func_name, icudata_version);
  syms[399] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memset");
  strcat(func_name, icudata_version);
  syms[400] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcmp");
  strcat(func_name, icudata_version);
  syms[401] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[402] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memchr");
  strcat(func_name, icudata_version);
  syms[403] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memchr32");
  strcat(func_name, icudata_version);
  syms[404] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memrchr");
  strcat(func_name, icudata_version);
  syms[405] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memrchr32");
  strcat(func_name, icudata_version);
  syms[406] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_unescape");
  strcat(func_name, icudata_version);
  syms[407] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_unescapeAt");
  strcat(func_name, icudata_version);
  syms[408] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUpper");
  strcat(func_name, icudata_version);
  syms[409] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToLower");
  strcat(func_name, icudata_version);
  syms[410] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToTitle");
  strcat(func_name, icudata_version);
  syms[411] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFoldCase");
  strcat(func_name, icudata_version);
  syms[412] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToWCS");
  strcat(func_name, icudata_version);
  syms[413] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromWCS");
  strcat(func_name, icudata_version);
  syms[414] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF8");
  strcat(func_name, icudata_version);
  syms[415] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8");
  strcat(func_name, icudata_version);
  syms[416] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF8WithSub");
  strcat(func_name, icudata_version);
  syms[417] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8WithSub");
  strcat(func_name, icudata_version);
  syms[418] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8Lenient");
  strcat(func_name, icudata_version);
  syms[419] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF32");
  strcat(func_name, icudata_version);
  syms[420] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF32");
  strcat(func_name, icudata_version);
  syms[421] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF32WithSub");
  strcat(func_name, icudata_version);
  syms[422] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF32WithSub");
  strcat(func_name, icudata_version);
  syms[423] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_hasBinaryProperty");
  strcat(func_name, icudata_version);
  syms[424] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUAlphabetic");
  strcat(func_name, icudata_version);
  syms[425] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isULowercase");
  strcat(func_name, icudata_version);
  syms[426] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUUppercase");
  strcat(func_name, icudata_version);
  syms[427] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUWhiteSpace");
  strcat(func_name, icudata_version);
  syms[428] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyValue");
  strcat(func_name, icudata_version);
  syms[429] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyMinValue");
  strcat(func_name, icudata_version);
  syms[430] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyMaxValue");
  strcat(func_name, icudata_version);
  syms[431] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getNumericValue");
  strcat(func_name, icudata_version);
  syms[432] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_islower");
  strcat(func_name, icudata_version);
  syms[433] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isupper");
  strcat(func_name, icudata_version);
  syms[434] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_istitle");
  strcat(func_name, icudata_version);
  syms[435] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isdigit");
  strcat(func_name, icudata_version);
  syms[436] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isalpha");
  strcat(func_name, icudata_version);
  syms[437] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isalnum");
  strcat(func_name, icudata_version);
  syms[438] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isxdigit");
  strcat(func_name, icudata_version);
  syms[439] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_ispunct");
  strcat(func_name, icudata_version);
  syms[440] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isgraph");
  strcat(func_name, icudata_version);
  syms[441] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isblank");
  strcat(func_name, icudata_version);
  syms[442] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isdefined");
  strcat(func_name, icudata_version);
  syms[443] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isspace");
  strcat(func_name, icudata_version);
  syms[444] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaSpaceChar");
  strcat(func_name, icudata_version);
  syms[445] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isWhitespace");
  strcat(func_name, icudata_version);
  syms[446] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_iscntrl");
  strcat(func_name, icudata_version);
  syms[447] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isISOControl");
  strcat(func_name, icudata_version);
  syms[448] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isprint");
  strcat(func_name, icudata_version);
  syms[449] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isbase");
  strcat(func_name, icudata_version);
  syms[450] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charDirection");
  strcat(func_name, icudata_version);
  syms[451] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isMirrored");
  strcat(func_name, icudata_version);
  syms[452] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charMirror");
  strcat(func_name, icudata_version);
  syms[453] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charType");
  strcat(func_name, icudata_version);
  syms[454] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_enumCharTypes");
  strcat(func_name, icudata_version);
  syms[455] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getCombiningClass");
  strcat(func_name, icudata_version);
  syms[456] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charDigitValue");
  strcat(func_name, icudata_version);
  syms[457] = dlsym(handle_common, func_name);

  strcpy(func_name, "ublock_getCode");
  strcat(func_name, icudata_version);
  syms[458] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charName");
  strcat(func_name, icudata_version);
  syms[459] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charFromName");
  strcat(func_name, icudata_version);
  syms[460] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_enumCharNames");
  strcat(func_name, icudata_version);
  syms[461] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyName");
  strcat(func_name, icudata_version);
  syms[462] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyEnum");
  strcat(func_name, icudata_version);
  syms[463] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyValueName");
  strcat(func_name, icudata_version);
  syms[464] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyValueEnum");
  strcat(func_name, icudata_version);
  syms[465] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDStart");
  strcat(func_name, icudata_version);
  syms[466] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDPart");
  strcat(func_name, icudata_version);
  syms[467] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDIgnorable");
  strcat(func_name, icudata_version);
  syms[468] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaIDStart");
  strcat(func_name, icudata_version);
  syms[469] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaIDPart");
  strcat(func_name, icudata_version);
  syms[470] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_tolower");
  strcat(func_name, icudata_version);
  syms[471] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_toupper");
  strcat(func_name, icudata_version);
  syms[472] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_totitle");
  strcat(func_name, icudata_version);
  syms[473] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_foldCase");
  strcat(func_name, icudata_version);
  syms[474] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_digit");
  strcat(func_name, icudata_version);
  syms[475] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_forDigit");
  strcat(func_name, icudata_version);
  syms[476] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charAge");
  strcat(func_name, icudata_version);
  syms[477] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getUnicodeVersion");
  strcat(func_name, icudata_version);
  syms[478] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getFC_NFKC_Closure");
  strcat(func_name, icudata_version);
  syms[479] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_current32");
  strcat(func_name, icudata_version);
  syms[480] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_next32");
  strcat(func_name, icudata_version);
  syms[481] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_previous32");
  strcat(func_name, icudata_version);
  syms[482] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_getState");
  strcat(func_name, icudata_version);
  syms[483] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setState");
  strcat(func_name, icudata_version);
  syms[484] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setString");
  strcat(func_name, icudata_version);
  syms[485] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setUTF16BE");
  strcat(func_name, icudata_version);
  syms[486] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setUTF8");
  strcat(func_name, icudata_version);
  syms[487] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_errorName");
  strcat(func_name, icudata_version);
  syms[488] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_open");
  strcat(func_name, icudata_version);
  syms[489] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_openChoice");
  strcat(func_name, icudata_version);
  syms[490] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_close");
  strcat(func_name, icudata_version);
  syms[491] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_getMemory");
  strcat(func_name, icudata_version);
  syms[492] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_getInfo");
  strcat(func_name, icudata_version);
  syms[493] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setCommonData");
  strcat(func_name, icudata_version);
  syms[494] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setAppData");
  strcat(func_name, icudata_version);
  syms[495] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setFileAccess");
  strcat(func_name, icudata_version);
  syms[496] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_open");
  strcat(func_name, icudata_version);
  syms[497] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_close");
  strcat(func_name, icudata_version);
  syms[498] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getLocale");
  strcat(func_name, icudata_version);
  syms[499] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getOptions");
  strcat(func_name, icudata_version);
  syms[500] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setLocale");
  strcat(func_name, icudata_version);
  syms[501] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setOptions");
  strcat(func_name, icudata_version);
  syms[502] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getBreakIterator");
  strcat(func_name, icudata_version);
  syms[503] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setBreakIterator");
  strcat(func_name, icudata_version);
  syms[504] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_toTitle");
  strcat(func_name, icudata_version);
  syms[505] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToLower");
  strcat(func_name, icudata_version);
  syms[506] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToUpper");
  strcat(func_name, icudata_version);
  syms[507] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToTitle");
  strcat(func_name, icudata_version);
  syms[508] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8FoldCase");
  strcat(func_name, icudata_version);
  syms[509] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteBytes");
  strcat(func_name, icudata_version);
  syms[510] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteSub");
  strcat(func_name, icudata_version);
  syms[511] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteUChars");
  strcat(func_name, icudata_version);
  syms[512] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbToUWriteUChars");
  strcat(func_name, icudata_version);
  syms[513] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbToUWriteSub");
  strcat(func_name, icudata_version);
  syms[514] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openEmpty");
  strcat(func_name, icudata_version);
  syms[515] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_open");
  strcat(func_name, icudata_version);
  syms[516] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openPattern");
  strcat(func_name, icudata_version);
  syms[517] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openPatternOptions");
  strcat(func_name, icudata_version);
  syms[518] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_close");
  strcat(func_name, icudata_version);
  syms[519] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_clone");
  strcat(func_name, icudata_version);
  syms[520] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_isFrozen");
  strcat(func_name, icudata_version);
  syms[521] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_freeze");
  strcat(func_name, icudata_version);
  syms[522] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_cloneAsThawed");
  strcat(func_name, icudata_version);
  syms[523] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_set");
  strcat(func_name, icudata_version);
  syms[524] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyPattern");
  strcat(func_name, icudata_version);
  syms[525] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyIntPropertyValue");
  strcat(func_name, icudata_version);
  syms[526] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyPropertyAlias");
  strcat(func_name, icudata_version);
  syms[527] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_resemblesPattern");
  strcat(func_name, icudata_version);
  syms[528] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_toPattern");
  strcat(func_name, icudata_version);
  syms[529] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_add");
  strcat(func_name, icudata_version);
  syms[530] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addAll");
  strcat(func_name, icudata_version);
  syms[531] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addRange");
  strcat(func_name, icudata_version);
  syms[532] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addString");
  strcat(func_name, icudata_version);
  syms[533] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addAllCodePoints");
  strcat(func_name, icudata_version);
  syms[534] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_remove");
  strcat(func_name, icudata_version);
  syms[535] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeRange");
  strcat(func_name, icudata_version);
  syms[536] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeString");
  strcat(func_name, icudata_version);
  syms[537] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeAll");
  strcat(func_name, icudata_version);
  syms[538] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_retain");
  strcat(func_name, icudata_version);
  syms[539] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_retainAll");
  strcat(func_name, icudata_version);
  syms[540] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_compact");
  strcat(func_name, icudata_version);
  syms[541] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_complement");
  strcat(func_name, icudata_version);
  syms[542] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_complementAll");
  strcat(func_name, icudata_version);
  syms[543] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_clear");
  strcat(func_name, icudata_version);
  syms[544] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_closeOver");
  strcat(func_name, icudata_version);
  syms[545] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeAllStrings");
  strcat(func_name, icudata_version);
  syms[546] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_isEmpty");
  strcat(func_name, icudata_version);
  syms[547] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_contains");
  strcat(func_name, icudata_version);
  syms[548] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsRange");
  strcat(func_name, icudata_version);
  syms[549] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsString");
  strcat(func_name, icudata_version);
  syms[550] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_indexOf");
  strcat(func_name, icudata_version);
  syms[551] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_charAt");
  strcat(func_name, icudata_version);
  syms[552] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_size");
  strcat(func_name, icudata_version);
  syms[553] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getItemCount");
  strcat(func_name, icudata_version);
  syms[554] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getItem");
  strcat(func_name, icudata_version);
  syms[555] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsAll");
  strcat(func_name, icudata_version);
  syms[556] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsAllCodePoints");
  strcat(func_name, icudata_version);
  syms[557] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsNone");
  strcat(func_name, icudata_version);
  syms[558] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsSome");
  strcat(func_name, icudata_version);
  syms[559] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_span");
  strcat(func_name, icudata_version);
  syms[560] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanBack");
  strcat(func_name, icudata_version);
  syms[561] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanUTF8");
  strcat(func_name, icudata_version);
  syms[562] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanBackUTF8");
  strcat(func_name, icudata_version);
  syms[563] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_equals");
  strcat(func_name, icudata_version);
  syms[564] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_serialize");
  strcat(func_name, icudata_version);
  syms[565] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedSet");
  strcat(func_name, icudata_version);
  syms[566] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_setSerializedToOne");
  strcat(func_name, icudata_version);
  syms[567] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_serializedContains");
  strcat(func_name, icudata_version);
  syms[568] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedRangeCount");
  strcat(func_name, icudata_version);
  syms[569] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedRange");
  strcat(func_name, icudata_version);
  syms[570] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_close");
  strcat(func_name, icudata_version);
  syms[571] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_openUTF8");
  strcat(func_name, icudata_version);
  syms[572] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_openUChars");
  strcat(func_name, icudata_version);
  syms[573] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_clone");
  strcat(func_name, icudata_version);
  syms[574] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_equals");
  strcat(func_name, icudata_version);
  syms[575] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_nativeLength");
  strcat(func_name, icudata_version);
  syms[576] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_isLengthExpensive");
  strcat(func_name, icudata_version);
  syms[577] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_char32At");
  strcat(func_name, icudata_version);
  syms[578] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_current32");
  strcat(func_name, icudata_version);
  syms[579] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_next32");
  strcat(func_name, icudata_version);
  syms[580] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_previous32");
  strcat(func_name, icudata_version);
  syms[581] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_next32From");
  strcat(func_name, icudata_version);
  syms[582] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_previous32From");
  strcat(func_name, icudata_version);
  syms[583] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_getNativeIndex");
  strcat(func_name, icudata_version);
  syms[584] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_setNativeIndex");
  strcat(func_name, icudata_version);
  syms[585] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_moveIndex32");
  strcat(func_name, icudata_version);
  syms[586] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_getPreviousNativeIndex");
  strcat(func_name, icudata_version);
  syms[587] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_extract");
  strcat(func_name, icudata_version);
  syms[588] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_isWritable");
  strcat(func_name, icudata_version);
  syms[589] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_hasMetaData");
  strcat(func_name, icudata_version);
  syms[590] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_replace");
  strcat(func_name, icudata_version);
  syms[591] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_copy");
  strcat(func_name, icudata_version);
  syms[592] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_freeze");
  strcat(func_name, icudata_version);
  syms[593] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_setup");
  strcat(func_name, icudata_version);
  syms[594] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_open");
  strcat(func_name, icudata_version);
  syms[595] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openDirect");
  strcat(func_name, icudata_version);
  syms[596] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openU");
  strcat(func_name, icudata_version);
  syms[597] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_close");
  strcat(func_name, icudata_version);
  syms[598] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getVersion");
  strcat(func_name, icudata_version);
  syms[599] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[600] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getString");
  strcat(func_name, icudata_version);
  syms[601] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8String");
  strcat(func_name, icudata_version);
  syms[602] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getBinary");
  strcat(func_name, icudata_version);
  syms[603] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getIntVector");
  strcat(func_name, icudata_version);
  syms[604] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUInt");
  strcat(func_name, icudata_version);
  syms[605] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getInt");
  strcat(func_name, icudata_version);
  syms[606] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getSize");
  strcat(func_name, icudata_version);
  syms[607] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getType");
  strcat(func_name, icudata_version);
  syms[608] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getKey");
  strcat(func_name, icudata_version);
  syms[609] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_resetIterator");
  strcat(func_name, icudata_version);
  syms[610] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_hasNext");
  strcat(func_name, icudata_version);
  syms[611] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getNextResource");
  strcat(func_name, icudata_version);
  syms[612] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getNextString");
  strcat(func_name, icudata_version);
  syms[613] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getByIndex");
  strcat(func_name, icudata_version);
  syms[614] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getStringByIndex");
  strcat(func_name, icudata_version);
  syms[615] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8StringByIndex");
  strcat(func_name, icudata_version);
  syms[616] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getByKey");
  strcat(func_name, icudata_version);
  syms[617] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getStringByKey");
  strcat(func_name, icudata_version);
  syms[618] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8StringByKey");
  strcat(func_name, icudata_version);
  syms[619] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openAvailableLocales");
  strcat(func_name, icudata_version);
  syms[620] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_open");
  strcat(func_name, icudata_version);
  syms[621] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_openSized");
  strcat(func_name, icudata_version);
  syms[622] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_close");
  strcat(func_name, icudata_version);
  syms[623] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setInverse");
  strcat(func_name, icudata_version);
  syms[624] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_isInverse");
  strcat(func_name, icudata_version);
  syms[625] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_orderParagraphsLTR");
  strcat(func_name, icudata_version);
  syms[626] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_isOrderParagraphsLTR");
  strcat(func_name, icudata_version);
  syms[627] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setReorderingMode");
  strcat(func_name, icudata_version);
  syms[628] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getReorderingMode");
  strcat(func_name, icudata_version);
  syms[629] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setReorderingOptions");
  strcat(func_name, icudata_version);
  syms[630] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getReorderingOptions");
  strcat(func_name, icudata_version);
  syms[631] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setPara");
  strcat(func_name, icudata_version);
  syms[632] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setLine");
  strcat(func_name, icudata_version);
  syms[633] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getDirection");
  strcat(func_name, icudata_version);
  syms[634] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getText");
  strcat(func_name, icudata_version);
  syms[635] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLength");
  strcat(func_name, icudata_version);
  syms[636] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParaLevel");
  strcat(func_name, icudata_version);
  syms[637] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_countParagraphs");
  strcat(func_name, icudata_version);
  syms[638] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParagraph");
  strcat(func_name, icudata_version);
  syms[639] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParagraphByIndex");
  strcat(func_name, icudata_version);
  syms[640] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLevelAt");
  strcat(func_name, icudata_version);
  syms[641] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLevels");
  strcat(func_name, icudata_version);
  syms[642] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalRun");
  strcat(func_name, icudata_version);
  syms[643] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_countRuns");
  strcat(func_name, icudata_version);
  syms[644] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualRun");
  strcat(func_name, icudata_version);
  syms[645] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualIndex");
  strcat(func_name, icudata_version);
  syms[646] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalIndex");
  strcat(func_name, icudata_version);
  syms[647] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalMap");
  strcat(func_name, icudata_version);
  syms[648] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualMap");
  strcat(func_name, icudata_version);
  syms[649] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_reorderLogical");
  strcat(func_name, icudata_version);
  syms[650] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_reorderVisual");
  strcat(func_name, icudata_version);
  syms[651] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_invertMap");
  strcat(func_name, icudata_version);
  syms[652] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getProcessedLength");
  strcat(func_name, icudata_version);
  syms[653] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getResultLength");
  strcat(func_name, icudata_version);
  syms[654] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getCustomizedClass");
  strcat(func_name, icudata_version);
  syms[655] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setClassCallback");
  strcat(func_name, icudata_version);
  syms[656] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getClassCallback");
  strcat(func_name, icudata_version);
  syms[657] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_writeReordered");
  strcat(func_name, icudata_version);
  syms[658] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_writeReverse");
  strcat(func_name, icudata_version);
  syms[659] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_init");
  strcat(func_name, icudata_version);
  syms[660] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_cleanup");
  strcat(func_name, icudata_version);
  syms[661] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_setMemoryFunctions");
  strcat(func_name, icudata_version);
  syms[662] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getCode");
  strcat(func_name, icudata_version);
  syms[663] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getName");
  strcat(func_name, icudata_version);
  syms[664] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getShortName");
  strcat(func_name, icudata_version);
  syms[665] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getScript");
  strcat(func_name, icudata_version);
  syms[666] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_shapeArabic");
  strcat(func_name, icudata_version);
  syms[667] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_setLevel");
  strcat(func_name, icudata_version);
  syms[668] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_getLevel");
  strcat(func_name, icudata_version);
  syms[669] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_setFunctions");
  strcat(func_name, icudata_version);
  syms[670] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_getFunctions");
  strcat(func_name, icudata_version);
  syms[671] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_vformat");
  strcat(func_name, icudata_version);
  syms[672] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_vformat");
  strcat(func_name, icudata_version);
  syms[673] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_functionName");
  strcat(func_name, icudata_version);
  syms[674] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getDataDirectory");
  strcat(func_name, icudata_version);
  syms[675] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_setDataDirectory");
  strcat(func_name, icudata_version);
  syms[676] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charsToUChars");
  strcat(func_name, icudata_version);
  syms[677] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_UCharsToChars");
  strcat(func_name, icudata_version);
  syms[678] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_normalize");
  strcat(func_name, icudata_version);
  syms[679] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_quickCheck");
  strcat(func_name, icudata_version);
  syms[680] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_quickCheckWithOptions");
  strcat(func_name, icudata_version);
  syms[681] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_isNormalized");
  strcat(func_name, icudata_version);
  syms[682] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_isNormalizedWithOptions");
  strcat(func_name, icudata_version);
  syms[683] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_next");
  strcat(func_name, icudata_version);
  syms[684] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_previous");
  strcat(func_name, icudata_version);
  syms[685] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_concatenate");
  strcat(func_name, icudata_version);
  syms[686] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_compare");
  strcat(func_name, icudata_version);
  syms[687] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_open");
  strcat(func_name, icudata_version);
  syms[688] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_openRules");
  strcat(func_name, icudata_version);
  syms[689] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_safeClone");
  strcat(func_name, icudata_version);
  syms[690] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_close");
  strcat(func_name, icudata_version);
  syms[691] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_setText");
  strcat(func_name, icudata_version);
  syms[692] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_setUText");
  strcat(func_name, icudata_version);
  syms[693] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_current");
  strcat(func_name, icudata_version);
  syms[694] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_next");
  strcat(func_name, icudata_version);
  syms[695] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_previous");
  strcat(func_name, icudata_version);
  syms[696] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_first");
  strcat(func_name, icudata_version);
  syms[697] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_last");
  strcat(func_name, icudata_version);
  syms[698] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_preceding");
  strcat(func_name, icudata_version);
  syms[699] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_following");
  strcat(func_name, icudata_version);
  syms[700] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getAvailable");
  strcat(func_name, icudata_version);
  syms[701] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_countAvailable");
  strcat(func_name, icudata_version);
  syms[702] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_isBoundary");
  strcat(func_name, icudata_version);
  syms[703] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getRuleStatus");
  strcat(func_name, icudata_version);
  syms[704] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getRuleStatusVec");
  strcat(func_name, icudata_version);
  syms[705] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[706] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDefault");
  strcat(func_name, icudata_version);
  syms[707] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_setDefault");
  strcat(func_name, icudata_version);
  syms[708] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLanguage");
  strcat(func_name, icudata_version);
  syms[709] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getScript");
  strcat(func_name, icudata_version);
  syms[710] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getCountry");
  strcat(func_name, icudata_version);
  syms[711] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getVariant");
  strcat(func_name, icudata_version);
  syms[712] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getName");
  strcat(func_name, icudata_version);
  syms[713] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_canonicalize");
  strcat(func_name, icudata_version);
  syms[714] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISO3Language");
  strcat(func_name, icudata_version);
  syms[715] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISO3Country");
  strcat(func_name, icudata_version);
  syms[716] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLCID");
  strcat(func_name, icudata_version);
  syms[717] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayLanguage");
  strcat(func_name, icudata_version);
  syms[718] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayScript");
  strcat(func_name, icudata_version);
  syms[719] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayCountry");
  strcat(func_name, icudata_version);
  syms[720] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayVariant");
  strcat(func_name, icudata_version);
  syms[721] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayKeyword");
  strcat(func_name, icudata_version);
  syms[722] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayKeywordValue");
  strcat(func_name, icudata_version);
  syms[723] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayName");
  strcat(func_name, icudata_version);
  syms[724] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getAvailable");
  strcat(func_name, icudata_version);
  syms[725] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_countAvailable");
  strcat(func_name, icudata_version);
  syms[726] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISOLanguages");
  strcat(func_name, icudata_version);
  syms[727] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISOCountries");
  strcat(func_name, icudata_version);
  syms[728] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getParent");
  strcat(func_name, icudata_version);
  syms[729] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getBaseName");
  strcat(func_name, icudata_version);
  syms[730] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_openKeywords");
  strcat(func_name, icudata_version);
  syms[731] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getKeywordValue");
  strcat(func_name, icudata_version);
  syms[732] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_setKeywordValue");
  strcat(func_name, icudata_version);
  syms[733] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getCharacterOrientation");
  strcat(func_name, icudata_version);
  syms[734] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLineOrientation");
  strcat(func_name, icudata_version);
  syms[735] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_acceptLanguageFromHTTP");
  strcat(func_name, icudata_version);
  syms[736] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_acceptLanguage");
  strcat(func_name, icudata_version);
  syms[737] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLocaleForLCID");
  strcat(func_name, icudata_version);
  syms[738] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_addLikelySubtags");
  strcat(func_name, icudata_version);
  syms[739] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_minimizeSubtags");
  strcat(func_name, icudata_version);
  syms[740] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catopen");
  strcat(func_name, icudata_version);
  syms[741] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catclose");
  strcat(func_name, icudata_version);
  syms[742] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catgets");
  strcat(func_name, icudata_version);
  syms[743] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_open");
  strcat(func_name, icudata_version);
  syms[744] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_close");
  strcat(func_name, icudata_version);
  syms[745] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_openFromSerialized");
  strcat(func_name, icudata_version);
  syms[746] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_serialize");
  strcat(func_name, icudata_version);
  syms[747] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_selectForString");
  strcat(func_name, icudata_version);
  syms[748] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_selectForUTF8");
  strcat(func_name, icudata_version);
  syms[749] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_open");
  strcat(func_name, icudata_version);
  syms[750] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_openByType");
  strcat(func_name, icudata_version);
  syms[751] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_close");
  strcat(func_name, icudata_version);
  syms[752] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_prepare");
  strcat(func_name, icudata_version);
  syms[753] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionFromString");
  strcat(func_name, icudata_version);
  syms[754] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionFromUString");
  strcat(func_name, icudata_version);
  syms[755] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionToString");
  strcat(func_name, icudata_version);
  syms[756] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getVersion");
  strcat(func_name, icudata_version);
  syms[757] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_toASCII");
  strcat(func_name, icudata_version);
  syms[758] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_toUnicode");
  strcat(func_name, icudata_version);
  syms[759] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_IDNToASCII");
  strcat(func_name, icudata_version);
  syms[760] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_IDNToUnicode");
  strcat(func_name, icudata_version);
  syms[761] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_compare");
  strcat(func_name, icudata_version);
  syms[762] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_close");
  strcat(func_name, icudata_version);
  syms[763] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_count");
  strcat(func_name, icudata_version);
  syms[764] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_unext");
  strcat(func_name, icudata_version);
  syms[765] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_next");
  strcat(func_name, icudata_version);
  syms[766] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_reset");
  strcat(func_name, icudata_version);
  syms[767] = dlsym(handle_common, func_name);

  status = max_version;
}

/* unicode/ndkicu.h */
int ndk_icu_status(void) {
  return status;
}

/* unicode/ucurr.h */
int32_t ucurr_forLocale(const char* locale, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  int32_t (*ptr)(const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, UChar*, int32_t, UErrorCode*))syms[0];
  return ptr(locale, buff, buffCapacity, ec);
}

UCurrRegistryKey ucurr_register(const UChar* isoCode, const char* locale, UErrorCode* status) {
  UCurrRegistryKey (*ptr)(const UChar*, const char*, UErrorCode*);
  ptr = (UCurrRegistryKey(*)(const UChar*, const char*, UErrorCode*))syms[1];
  return ptr(isoCode, locale, status);
}

UBool ucurr_unregister(UCurrRegistryKey key, UErrorCode* status) {
  UBool (*ptr)(UCurrRegistryKey, UErrorCode*);
  ptr = (UBool(*)(UCurrRegistryKey, UErrorCode*))syms[2];
  return ptr(key, status);
}

const UChar* ucurr_getName(const UChar* currency, const char* locale, UCurrNameStyle nameStyle, UBool* isChoiceFormat, int32_t* len, UErrorCode* ec) {
  const UChar* (*ptr)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*))syms[3];
  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}

const UChar* ucurr_getPluralName(const UChar* currency, const char* locale, UBool* isChoiceFormat, const char* pluralCount, int32_t* len, UErrorCode* ec) {
  const UChar* (*ptr)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*))syms[4];
  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}

int32_t ucurr_getDefaultFractionDigits(const UChar* currency, UErrorCode* ec) {
  int32_t (*ptr)(const UChar*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, UErrorCode*))syms[5];
  return ptr(currency, ec);
}

double ucurr_getRoundingIncrement(const UChar* currency, UErrorCode* ec) {
  double (*ptr)(const UChar*, UErrorCode*);
  ptr = (double(*)(const UChar*, UErrorCode*))syms[6];
  return ptr(currency, ec);
}

UEnumeration* ucurr_openISOCurrencies(uint32_t currType, UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(uint32_t, UErrorCode*);
  ptr = (UEnumeration*(*)(uint32_t, UErrorCode*))syms[7];
  return ptr(currType, pErrorCode);
}

int32_t ucurr_countCurrencies(const char* locale, UDate date, UErrorCode* ec) {
  int32_t (*ptr)(const char*, UDate, UErrorCode*);
  ptr = (int32_t(*)(const char*, UDate, UErrorCode*))syms[8];
  return ptr(locale, date, ec);
}

int32_t ucurr_forLocaleAndDate(const char* locale, UDate date, int32_t index, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  int32_t (*ptr)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*))syms[9];
  return ptr(locale, date, index, buff, buffCapacity, ec);
}

UEnumeration* ucurr_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[10];
  return ptr(key, locale, commonlyUsed, status);
}

/* unicode/udat.h */
UDateFormat* udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char* locale, const UChar* tzID, int32_t tzIDLength, const UChar* pattern, int32_t patternLength, UErrorCode* status) {
  UDateFormat* (*ptr)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  ptr = (UDateFormat*(*)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[11];
  return ptr(timeStyle, dateStyle, locale, tzID, tzIDLength, pattern, patternLength, status);
}

void udat_close(UDateFormat* format) {
  void (*ptr)(UDateFormat*);
  ptr = (void(*)(UDateFormat*))syms[12];
  ptr(format);
  return;
}

UDateFormat* udat_clone(const UDateFormat* fmt, UErrorCode* status) {
  UDateFormat* (*ptr)(const UDateFormat*, UErrorCode*);
  ptr = (UDateFormat*(*)(const UDateFormat*, UErrorCode*))syms[13];
  return ptr(fmt, status);
}

int32_t udat_format(const UDateFormat* format, UDate dateToFormat, UChar* result, int32_t resultLength, UFieldPosition* position, UErrorCode* status) {
  int32_t (*ptr)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  ptr = (int32_t(*)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[14];
  return ptr(format, dateToFormat, result, resultLength, position, status);
}

UDate udat_parse(const UDateFormat* format, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  UDate (*ptr)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (UDate(*)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[15];
  return ptr(format, text, textLength, parsePos, status);
}

void udat_parseCalendar(const UDateFormat* format, UCalendar* calendar, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  void (*ptr)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (void(*)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[16];
  ptr(format, calendar, text, textLength, parsePos, status);
  return;
}

UBool udat_isLenient(const UDateFormat* fmt) {
  UBool (*ptr)(const UDateFormat*);
  ptr = (UBool(*)(const UDateFormat*))syms[17];
  return ptr(fmt);
}

void udat_setLenient(UDateFormat* fmt, UBool isLenient) {
  void (*ptr)(UDateFormat*, UBool);
  ptr = (void(*)(UDateFormat*, UBool))syms[18];
  ptr(fmt, isLenient);
  return;
}

const UCalendar* udat_getCalendar(const UDateFormat* fmt) {
  const UCalendar* (*ptr)(const UDateFormat*);
  ptr = (const UCalendar*(*)(const UDateFormat*))syms[19];
  return ptr(fmt);
}

void udat_setCalendar(UDateFormat* fmt, const UCalendar* calendarToSet) {
  void (*ptr)(UDateFormat*, const UCalendar*);
  ptr = (void(*)(UDateFormat*, const UCalendar*))syms[20];
  ptr(fmt, calendarToSet);
  return;
}

const UNumberFormat* udat_getNumberFormat(const UDateFormat* fmt) {
  const UNumberFormat* (*ptr)(const UDateFormat*);
  ptr = (const UNumberFormat*(*)(const UDateFormat*))syms[21];
  return ptr(fmt);
}

void udat_setNumberFormat(UDateFormat* fmt, const UNumberFormat* numberFormatToSet) {
  void (*ptr)(UDateFormat*, const UNumberFormat*);
  ptr = (void(*)(UDateFormat*, const UNumberFormat*))syms[22];
  ptr(fmt, numberFormatToSet);
  return;
}

const char* udat_getAvailable(int32_t localeIndex) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[23];
  return ptr(localeIndex);
}

int32_t udat_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[24];
  return ptr();
}

UDate udat_get2DigitYearStart(const UDateFormat* fmt, UErrorCode* status) {
  UDate (*ptr)(const UDateFormat*, UErrorCode*);
  ptr = (UDate(*)(const UDateFormat*, UErrorCode*))syms[25];
  return ptr(fmt, status);
}

void udat_set2DigitYearStart(UDateFormat* fmt, UDate d, UErrorCode* status) {
  void (*ptr)(UDateFormat*, UDate, UErrorCode*);
  ptr = (void(*)(UDateFormat*, UDate, UErrorCode*))syms[26];
  ptr(fmt, d, status);
  return;
}

int32_t udat_toPattern(const UDateFormat* fmt, UBool localized, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*))syms[27];
  return ptr(fmt, localized, result, resultLength, status);
}

void udat_applyPattern(UDateFormat* format, UBool localized, const UChar* pattern, int32_t patternLength) {
  void (*ptr)(UDateFormat*, UBool, const UChar*, int32_t);
  ptr = (void(*)(UDateFormat*, UBool, const UChar*, int32_t))syms[28];
  ptr(format, localized, pattern, patternLength);
  return;
}

int32_t udat_getSymbols(const UDateFormat* fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))syms[29];
  return ptr(fmt, type, symbolIndex, result, resultLength, status);
}

int32_t udat_countSymbols(const UDateFormat* fmt, UDateFormatSymbolType type) {
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType);
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType))syms[30];
  return ptr(fmt, type);
}

void udat_setSymbols(UDateFormat* format, UDateFormatSymbolType type, int32_t symbolIndex, UChar* value, int32_t valueLength, UErrorCode* status) {
  void (*ptr)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))syms[31];
  ptr(format, type, symbolIndex, value, valueLength, status);
  return;
}

const char* udat_getLocaleByType(const UDateFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UDateFormat*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UDateFormat*, ULocDataLocaleType, UErrorCode*))syms[32];
  return ptr(fmt, type, status);
}

/* unicode/unum.h */
UNumberFormat* unum_open(UNumberFormatStyle style, const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseErr, UErrorCode* status) {
  UNumberFormat* (*ptr)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  ptr = (UNumberFormat*(*)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*))syms[33];
  return ptr(style, pattern, patternLength, locale, parseErr, status);
}

void unum_close(UNumberFormat* fmt) {
  void (*ptr)(UNumberFormat*);
  ptr = (void(*)(UNumberFormat*))syms[34];
  ptr(fmt);
  return;
}

UNumberFormat* unum_clone(const UNumberFormat* fmt, UErrorCode* status) {
  UNumberFormat* (*ptr)(const UNumberFormat*, UErrorCode*);
  ptr = (UNumberFormat*(*)(const UNumberFormat*, UErrorCode*))syms[35];
  return ptr(fmt, status);
}

int32_t unum_format(const UNumberFormat* fmt, int32_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[36];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatInt64(const UNumberFormat* fmt, int64_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[37];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDouble(const UNumberFormat* fmt, double number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[38];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDoubleCurrency(const UNumberFormat* fmt, double number, UChar* currency, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[39];
  return ptr(fmt, number, currency, result, resultLength, pos, status);
}

int32_t unum_parse(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[40];
  return ptr(fmt, text, textLength, parsePos, status);
}

int64_t unum_parseInt64(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  int64_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (int64_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[41];
  return ptr(fmt, text, textLength, parsePos, status);
}

double unum_parseDouble(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[42];
  return ptr(fmt, text, textLength, parsePos, status);
}

double unum_parseDoubleCurrency(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UChar* currency, UErrorCode* status) {
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*);
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*))syms[43];
  return ptr(fmt, text, textLength, parsePos, currency, status);
}

void unum_applyPattern(UNumberFormat* format, UBool localized, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  void (*ptr)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*);
  ptr = (void(*)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*))syms[44];
  ptr(format, localized, pattern, patternLength, parseError, status);
  return;
}

const char* unum_getAvailable(int32_t localeIndex) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[45];
  return ptr(localeIndex);
}

int32_t unum_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[46];
  return ptr();
}

int32_t unum_getAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatAttribute))syms[47];
  return ptr(fmt, attr);
}

void unum_setAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, int32_t newValue) {
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, int32_t);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, int32_t))syms[48];
  ptr(fmt, attr, newValue);
  return;
}

double unum_getDoubleAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  double (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  ptr = (double(*)(const UNumberFormat*, UNumberFormatAttribute))syms[49];
  return ptr(fmt, attr);
}

void unum_setDoubleAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, double newValue) {
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, double);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, double))syms[50];
  ptr(fmt, attr, newValue);
  return;
}

int32_t unum_getTextAttribute(const UNumberFormat* fmt, UNumberFormatTextAttribute tag, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*))syms[51];
  return ptr(fmt, tag, result, resultLength, status);
}

void unum_setTextAttribute(UNumberFormat* fmt, UNumberFormatTextAttribute tag, const UChar* newValue, int32_t newValueLength, UErrorCode* status) {
  void (*ptr)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*))syms[52];
  ptr(fmt, tag, newValue, newValueLength, status);
  return;
}

int32_t unum_toPattern(const UNumberFormat* fmt, UBool isPatternLocalized, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*))syms[53];
  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}

int32_t unum_getSymbol(const UNumberFormat* fmt, UNumberFormatSymbol symbol, UChar* buffer, int32_t size, UErrorCode* status) {
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*))syms[54];
  return ptr(fmt, symbol, buffer, size, status);
}

void unum_setSymbol(UNumberFormat* fmt, UNumberFormatSymbol symbol, const UChar* value, int32_t length, UErrorCode* status) {
  void (*ptr)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*))syms[55];
  ptr(fmt, symbol, value, length, status);
  return;
}

const char* unum_getLocaleByType(const UNumberFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*))syms[56];
  return ptr(fmt, type, status);
}

/* unicode/ucoleitr.h */
UCollationElements* ucol_openElements(const UCollator* coll, const UChar* text, int32_t textLength, UErrorCode* status) {
  UCollationElements* (*ptr)(const UCollator*, const UChar*, int32_t, UErrorCode*);
  ptr = (UCollationElements*(*)(const UCollator*, const UChar*, int32_t, UErrorCode*))syms[57];
  return ptr(coll, text, textLength, status);
}

int32_t ucol_keyHashCode(const uint8_t* key, int32_t length) {
  int32_t (*ptr)(const uint8_t*, int32_t);
  ptr = (int32_t(*)(const uint8_t*, int32_t))syms[58];
  return ptr(key, length);
}

void ucol_closeElements(UCollationElements* elems) {
  void (*ptr)(UCollationElements*);
  ptr = (void(*)(UCollationElements*))syms[59];
  ptr(elems);
  return;
}

void ucol_reset(UCollationElements* elems) {
  void (*ptr)(UCollationElements*);
  ptr = (void(*)(UCollationElements*))syms[60];
  ptr(elems);
  return;
}

int32_t ucol_next(UCollationElements* elems, UErrorCode* status) {
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))syms[61];
  return ptr(elems, status);
}

int32_t ucol_previous(UCollationElements* elems, UErrorCode* status) {
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))syms[62];
  return ptr(elems, status);
}

int32_t ucol_getMaxExpansion(const UCollationElements* elems, int32_t order) {
  int32_t (*ptr)(const UCollationElements*, int32_t);
  ptr = (int32_t(*)(const UCollationElements*, int32_t))syms[63];
  return ptr(elems, order);
}

void ucol_setText(UCollationElements* elems, const UChar* text, int32_t textLength, UErrorCode* status) {
  void (*ptr)(UCollationElements*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UCollationElements*, const UChar*, int32_t, UErrorCode*))syms[64];
  ptr(elems, text, textLength, status);
  return;
}

int32_t ucol_getOffset(const UCollationElements* elems) {
  int32_t (*ptr)(const UCollationElements*);
  ptr = (int32_t(*)(const UCollationElements*))syms[65];
  return ptr(elems);
}

void ucol_setOffset(UCollationElements* elems, int32_t offset, UErrorCode* status) {
  void (*ptr)(UCollationElements*, int32_t, UErrorCode*);
  ptr = (void(*)(UCollationElements*, int32_t, UErrorCode*))syms[66];
  ptr(elems, offset, status);
  return;
}

int32_t ucol_primaryOrder(int32_t order) {
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[67];
  return ptr(order);
}

int32_t ucol_secondaryOrder(int32_t order) {
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[68];
  return ptr(order);
}

int32_t ucol_tertiaryOrder(int32_t order) {
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[69];
  return ptr(order);
}

/* unicode/utrans.h */
UTransliterator* utrans_openU(const UChar* id, int32_t idLength, UTransDirection dir, const UChar* rules, int32_t rulesLength, UParseError* parseError, UErrorCode* pErrorCode) {
  UTransliterator* (*ptr)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*);
  ptr = (UTransliterator*(*)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*))syms[70];
  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}

UTransliterator* utrans_openInverse(const UTransliterator* trans, UErrorCode* status) {
  UTransliterator* (*ptr)(const UTransliterator*, UErrorCode*);
  ptr = (UTransliterator*(*)(const UTransliterator*, UErrorCode*))syms[71];
  return ptr(trans, status);
}

UTransliterator* utrans_clone(const UTransliterator* trans, UErrorCode* status) {
  UTransliterator* (*ptr)(const UTransliterator*, UErrorCode*);
  ptr = (UTransliterator*(*)(const UTransliterator*, UErrorCode*))syms[72];
  return ptr(trans, status);
}

void utrans_close(UTransliterator* trans) {
  void (*ptr)(UTransliterator*);
  ptr = (void(*)(UTransliterator*))syms[73];
  ptr(trans);
  return;
}

const UChar* utrans_getUnicodeID(const UTransliterator* trans, int32_t* resultLength) {
  const UChar* (*ptr)(const UTransliterator*, int32_t*);
  ptr = (const UChar*(*)(const UTransliterator*, int32_t*))syms[74];
  return ptr(trans, resultLength);
}

void utrans_register(UTransliterator* adoptedTrans, UErrorCode* status) {
  void (*ptr)(UTransliterator*, UErrorCode*);
  ptr = (void(*)(UTransliterator*, UErrorCode*))syms[75];
  ptr(adoptedTrans, status);
  return;
}

void utrans_unregisterID(const UChar* id, int32_t idLength) {
  void (*ptr)(const UChar*, int32_t);
  ptr = (void(*)(const UChar*, int32_t))syms[76];
  ptr(id, idLength);
  return;
}

void utrans_setFilter(UTransliterator* trans, const UChar* filterPattern, int32_t filterPatternLen, UErrorCode* status) {
  void (*ptr)(UTransliterator*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UTransliterator*, const UChar*, int32_t, UErrorCode*))syms[77];
  ptr(trans, filterPattern, filterPatternLen, status);
  return;
}

int32_t utrans_countAvailableIDs(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[78];
  return ptr();
}

UEnumeration* utrans_openIDs(UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(UErrorCode*);
  ptr = (UEnumeration*(*)(UErrorCode*))syms[79];
  return ptr(pErrorCode);
}

void utrans_trans(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, int32_t start, int32_t* limit, UErrorCode* status) {
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*);
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*))syms[80];
  ptr(trans, rep, repFunc, start, limit, status);
  return;
}

void utrans_transIncremental(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, UTransPosition* pos, UErrorCode* status) {
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*);
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*))syms[81];
  ptr(trans, rep, repFunc, pos, status);
  return;
}

void utrans_transUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, int32_t start, int32_t* limit, UErrorCode* status) {
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*);
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*))syms[82];
  ptr(trans, text, textLength, textCapacity, start, limit, status);
  return;
}

void utrans_transIncrementalUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, UTransPosition* pos, UErrorCode* status) {
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*);
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*))syms[83];
  ptr(trans, text, textLength, textCapacity, pos, status);
  return;
}

/* unicode/usearch.h */
UStringSearch* usearch_open(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const char* locale, UBreakIterator* breakiter, UErrorCode* status) {
  UStringSearch* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*);
  ptr = (UStringSearch*(*)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*))syms[84];
  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}

UStringSearch* usearch_openFromCollator(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const UCollator* collator, UBreakIterator* breakiter, UErrorCode* status) {
  UStringSearch* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*);
  ptr = (UStringSearch*(*)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*))syms[85];
  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}

void usearch_close(UStringSearch* searchiter) {
  void (*ptr)(UStringSearch*);
  ptr = (void(*)(UStringSearch*))syms[86];
  ptr(searchiter);
  return;
}

void usearch_setOffset(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  void (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  ptr = (void(*)(UStringSearch*, int32_t, UErrorCode*))syms[87];
  ptr(strsrch, position, status);
  return;
}

int32_t usearch_getOffset(const UStringSearch* strsrch) {
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[88];
  return ptr(strsrch);
}

void usearch_setAttribute(UStringSearch* strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode* status) {
  void (*ptr)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*);
  ptr = (void(*)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*))syms[89];
  ptr(strsrch, attribute, value, status);
  return;
}

USearchAttributeValue usearch_getAttribute(const UStringSearch* strsrch, USearchAttribute attribute) {
  USearchAttributeValue (*ptr)(const UStringSearch*, USearchAttribute);
  ptr = (USearchAttributeValue(*)(const UStringSearch*, USearchAttribute))syms[90];
  return ptr(strsrch, attribute);
}

int32_t usearch_getMatchedStart(const UStringSearch* strsrch) {
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[91];
  return ptr(strsrch);
}

int32_t usearch_getMatchedLength(const UStringSearch* strsrch) {
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[92];
  return ptr(strsrch);
}

int32_t usearch_getMatchedText(const UStringSearch* strsrch, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  int32_t (*ptr)(const UStringSearch*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UStringSearch*, UChar*, int32_t, UErrorCode*))syms[93];
  return ptr(strsrch, result, resultCapacity, status);
}

void usearch_setBreakIterator(UStringSearch* strsrch, UBreakIterator* breakiter, UErrorCode* status) {
  void (*ptr)(UStringSearch*, UBreakIterator*, UErrorCode*);
  ptr = (void(*)(UStringSearch*, UBreakIterator*, UErrorCode*))syms[94];
  ptr(strsrch, breakiter, status);
  return;
}

const UBreakIterator* usearch_getBreakIterator(const UStringSearch* strsrch) {
  const UBreakIterator* (*ptr)(const UStringSearch*);
  ptr = (const UBreakIterator*(*)(const UStringSearch*))syms[95];
  return ptr(strsrch);
}

void usearch_setText(UStringSearch* strsrch, const UChar* text, int32_t textlength, UErrorCode* status) {
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))syms[96];
  ptr(strsrch, text, textlength, status);
  return;
}

const UChar* usearch_getText(const UStringSearch* strsrch, int32_t* length) {
  const UChar* (*ptr)(const UStringSearch*, int32_t*);
  ptr = (const UChar*(*)(const UStringSearch*, int32_t*))syms[97];
  return ptr(strsrch, length);
}

UCollator* usearch_getCollator(const UStringSearch* strsrch) {
  UCollator* (*ptr)(const UStringSearch*);
  ptr = (UCollator*(*)(const UStringSearch*))syms[98];
  return ptr(strsrch);
}

void usearch_setCollator(UStringSearch* strsrch, const UCollator* collator, UErrorCode* status) {
  void (*ptr)(UStringSearch*, const UCollator*, UErrorCode*);
  ptr = (void(*)(UStringSearch*, const UCollator*, UErrorCode*))syms[99];
  ptr(strsrch, collator, status);
  return;
}

void usearch_setPattern(UStringSearch* strsrch, const UChar* pattern, int32_t patternlength, UErrorCode* status) {
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))syms[100];
  ptr(strsrch, pattern, patternlength, status);
  return;
}

const UChar* usearch_getPattern(const UStringSearch* strsrch, int32_t* length) {
  const UChar* (*ptr)(const UStringSearch*, int32_t*);
  ptr = (const UChar*(*)(const UStringSearch*, int32_t*))syms[101];
  return ptr(strsrch, length);
}

int32_t usearch_first(UStringSearch* strsrch, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[102];
  return ptr(strsrch, status);
}

int32_t usearch_following(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))syms[103];
  return ptr(strsrch, position, status);
}

int32_t usearch_last(UStringSearch* strsrch, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[104];
  return ptr(strsrch, status);
}

int32_t usearch_preceding(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))syms[105];
  return ptr(strsrch, position, status);
}

int32_t usearch_next(UStringSearch* strsrch, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[106];
  return ptr(strsrch, status);
}

int32_t usearch_previous(UStringSearch* strsrch, UErrorCode* status) {
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[107];
  return ptr(strsrch, status);
}

void usearch_reset(UStringSearch* strsrch) {
  void (*ptr)(UStringSearch*);
  ptr = (void(*)(UStringSearch*))syms[108];
  ptr(strsrch);
  return;
}

/* unicode/umsg.h */
int32_t u_formatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*))syms[109];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, args, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*))syms[110];
  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}

void u_parseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UErrorCode* status, ...) {
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*))syms[111];
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, status);
  va_end(args);
  return;
}

void u_vparseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UErrorCode* status) {
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*))syms[112];
  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
  return;
}

int32_t u_formatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, UErrorCode* status, ...) {
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*))syms[113];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, parseError, args, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, va_list ap, UErrorCode* status) {
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*);
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*))syms[114];
  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}

void u_parseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UParseError* parseError, UErrorCode* status, ...) {
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*))syms[115];
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, parseError, status);
  va_end(args);
  return;
}

void u_vparseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UParseError* parseError, UErrorCode* status) {
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*);
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*))syms[116];
  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
  return;
}

UMessageFormat* umsg_open(const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseError, UErrorCode* status) {
  UMessageFormat* (*ptr)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  ptr = (UMessageFormat*(*)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*))syms[117];
  return ptr(pattern, patternLength, locale, parseError, status);
}

void umsg_close(UMessageFormat* format) {
  void (*ptr)(UMessageFormat*);
  ptr = (void(*)(UMessageFormat*))syms[118];
  ptr(format);
  return;
}

UMessageFormat umsg_clone(const UMessageFormat* fmt, UErrorCode* status) {
  UMessageFormat (*ptr)(const UMessageFormat*, UErrorCode*);
  ptr = (UMessageFormat(*)(const UMessageFormat*, UErrorCode*))syms[119];
  return ptr(fmt, status);
}

void umsg_setLocale(UMessageFormat* fmt, const char* locale) {
  void (*ptr)(UMessageFormat*, const char*);
  ptr = (void(*)(UMessageFormat*, const char*))syms[120];
  ptr(fmt, locale);
  return;
}

const char* umsg_getLocale(const UMessageFormat* fmt) {
  const char* (*ptr)(const UMessageFormat*);
  ptr = (const char*(*)(const UMessageFormat*))syms[121];
  return ptr(fmt);
}

void umsg_applyPattern(UMessageFormat* fmt, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  void (*ptr)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*);
  ptr = (void(*)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*))syms[122];
  ptr(fmt, pattern, patternLength, parseError, status);
  return;
}

int32_t umsg_toPattern(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, UErrorCode*))syms[123];
  return ptr(fmt, result, resultLength, status);
}

int32_t umsg_format(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*))syms[124];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(fmt, result, resultLength, args, status);
  va_end(args);
  return ret;
}

int32_t umsg_vformat(const UMessageFormat* fmt, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*);
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*))syms[125];
  return ptr(fmt, result, resultLength, ap, status);
}

void umsg_parse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, UErrorCode* status, ...) {
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*);
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*))syms[126];
  va_list args;
  va_start(args, status);
  ptr(fmt, source, sourceLength, count, args, status);
  va_end(args);
  return;
}

void umsg_vparse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, va_list ap, UErrorCode* status) {
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*);
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*))syms[127];
  ptr(fmt, source, sourceLength, count, ap, status);
  return;
}

int32_t umsg_autoQuoteApostrophe(const UChar* pattern, int32_t patternLength, UChar* dest, int32_t destCapacity, UErrorCode* ec) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[128];
  return ptr(pattern, patternLength, dest, destCapacity, ec);
}

/* unicode/ulocdata.h */
ULocaleData* ulocdata_open(const char* localeID, UErrorCode* status) {
  ULocaleData* (*ptr)(const char*, UErrorCode*);
  ptr = (ULocaleData*(*)(const char*, UErrorCode*))syms[129];
  return ptr(localeID, status);
}

void ulocdata_close(ULocaleData* uld) {
  void (*ptr)(ULocaleData*);
  ptr = (void(*)(ULocaleData*))syms[130];
  ptr(uld);
  return;
}

void ulocdata_setNoSubstitute(ULocaleData* uld, UBool setting) {
  void (*ptr)(ULocaleData*, UBool);
  ptr = (void(*)(ULocaleData*, UBool))syms[131];
  ptr(uld, setting);
  return;
}

UBool ulocdata_getNoSubstitute(ULocaleData* uld) {
  UBool (*ptr)(ULocaleData*);
  ptr = (UBool(*)(ULocaleData*))syms[132];
  return ptr(uld);
}

USet* ulocdata_getExemplarSet(ULocaleData* uld, USet* fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode* status) {
  USet* (*ptr)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*);
  ptr = (USet*(*)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*))syms[133];
  return ptr(uld, fillIn, options, extype, status);
}

int32_t ulocdata_getDelimiter(ULocaleData* uld, ULocaleDataDelimiterType type, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*))syms[134];
  return ptr(uld, type, result, resultLength, status);
}

UMeasurementSystem ulocdata_getMeasurementSystem(const char* localeID, UErrorCode* status) {
  UMeasurementSystem (*ptr)(const char*, UErrorCode*);
  ptr = (UMeasurementSystem(*)(const char*, UErrorCode*))syms[135];
  return ptr(localeID, status);
}

void ulocdata_getPaperSize(const char* localeID, int32_t* height, int32_t* width, UErrorCode* status) {
  void (*ptr)(const char*, int32_t*, int32_t*, UErrorCode*);
  ptr = (void(*)(const char*, int32_t*, int32_t*, UErrorCode*))syms[136];
  ptr(localeID, height, width, status);
  return;
}

void ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode* status) {
  void (*ptr)(UVersionInfo, UErrorCode*);
  ptr = (void(*)(UVersionInfo, UErrorCode*))syms[137];
  ptr(versionArray, status);
  return;
}

int32_t ulocdata_getLocaleDisplayPattern(ULocaleData* uld, UChar* pattern, int32_t patternCapacity, UErrorCode* status) {
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))syms[138];
  return ptr(uld, pattern, patternCapacity, status);
}

int32_t ulocdata_getLocaleSeparator(ULocaleData* uld, UChar* separator, int32_t separatorCapacity, UErrorCode* status) {
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))syms[139];
  return ptr(uld, separator, separatorCapacity, status);
}

/* unicode/ucol.h */
UCollator* ucol_open(const char* loc, UErrorCode* status) {
  UCollator* (*ptr)(const char*, UErrorCode*);
  ptr = (UCollator*(*)(const char*, UErrorCode*))syms[140];
  return ptr(loc, status);
}

UCollator* ucol_openRules(const UChar* rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError* parseError, UErrorCode* status) {
  UCollator* (*ptr)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*);
  ptr = (UCollator*(*)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*))syms[141];
  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}

void ucol_getContractionsAndExpansions(const UCollator* coll, USet* contractions, USet* expansions, UBool addPrefixes, UErrorCode* status) {
  void (*ptr)(const UCollator*, USet*, USet*, UBool, UErrorCode*);
  ptr = (void(*)(const UCollator*, USet*, USet*, UBool, UErrorCode*))syms[142];
  ptr(coll, contractions, expansions, addPrefixes, status);
  return;
}

void ucol_close(UCollator* coll) {
  void (*ptr)(UCollator*);
  ptr = (void(*)(UCollator*))syms[143];
  ptr(coll);
  return;
}

UCollationResult ucol_strcoll(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  UCollationResult (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UCollationResult(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[144];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_greater(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[145];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_greaterOrEqual(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[146];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_equal(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[147];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UCollationResult ucol_strcollIter(const UCollator* coll, UCharIterator* sIter, UCharIterator* tIter, UErrorCode* status) {
  UCollationResult (*ptr)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*);
  ptr = (UCollationResult(*)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*))syms[148];
  return ptr(coll, sIter, tIter, status);
}

UCollationStrength ucol_getStrength(const UCollator* coll) {
  UCollationStrength (*ptr)(const UCollator*);
  ptr = (UCollationStrength(*)(const UCollator*))syms[149];
  return ptr(coll);
}

void ucol_setStrength(UCollator* coll, UCollationStrength strength) {
  void (*ptr)(UCollator*, UCollationStrength);
  ptr = (void(*)(UCollator*, UCollationStrength))syms[150];
  ptr(coll, strength);
  return;
}

int32_t ucol_getDisplayName(const char* objLoc, const char* dispLoc, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[151];
  return ptr(objLoc, dispLoc, result, resultLength, status);
}

const char* ucol_getAvailable(int32_t localeIndex) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[152];
  return ptr(localeIndex);
}

int32_t ucol_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[153];
  return ptr();
}

UEnumeration* ucol_openAvailableLocales(UErrorCode* status) {
  UEnumeration* (*ptr)(UErrorCode*);
  ptr = (UEnumeration*(*)(UErrorCode*))syms[154];
  return ptr(status);
}

UEnumeration* ucol_getKeywords(UErrorCode* status) {
  UEnumeration* (*ptr)(UErrorCode*);
  ptr = (UEnumeration*(*)(UErrorCode*))syms[155];
  return ptr(status);
}

UEnumeration* ucol_getKeywordValues(const char* keyword, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[156];
  return ptr(keyword, status);
}

UEnumeration* ucol_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[157];
  return ptr(key, locale, commonlyUsed, status);
}

int32_t ucol_getFunctionalEquivalent(char* result, int32_t resultCapacity, const char* keyword, const char* locale, UBool* isAvailable, UErrorCode* status) {
  int32_t (*ptr)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*);
  ptr = (int32_t(*)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*))syms[158];
  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}

const UChar* ucol_getRules(const UCollator* coll, int32_t* length) {
  const UChar* (*ptr)(const UCollator*, int32_t*);
  ptr = (const UChar*(*)(const UCollator*, int32_t*))syms[159];
  return ptr(coll, length);
}

int32_t ucol_getSortKey(const UCollator* coll, const UChar* source, int32_t sourceLength, uint8_t* result, int32_t resultLength) {
  int32_t (*ptr)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t);
  ptr = (int32_t(*)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t))syms[160];
  return ptr(coll, source, sourceLength, result, resultLength);
}

int32_t ucol_nextSortKeyPart(const UCollator* coll, UCharIterator* iter, uint32_t state [ 2], uint8_t* dest, int32_t count, UErrorCode* status) {
  int32_t (*ptr)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*))syms[161];
  return ptr(coll, iter, state, dest, count, status);
}

int32_t ucol_getBound(const uint8_t* source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*))syms[162];
  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}

void ucol_getVersion(const UCollator* coll, UVersionInfo info) {
  void (*ptr)(const UCollator*, UVersionInfo);
  ptr = (void(*)(const UCollator*, UVersionInfo))syms[163];
  ptr(coll, info);
  return;
}

void ucol_getUCAVersion(const UCollator* coll, UVersionInfo info) {
  void (*ptr)(const UCollator*, UVersionInfo);
  ptr = (void(*)(const UCollator*, UVersionInfo))syms[164];
  ptr(coll, info);
  return;
}

int32_t ucol_mergeSortkeys(const uint8_t* src1, int32_t src1Length, const uint8_t* src2, int32_t src2Length, uint8_t* dest, int32_t destCapacity) {
  int32_t (*ptr)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t);
  ptr = (int32_t(*)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t))syms[165];
  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}

void ucol_setAttribute(UCollator* coll, UColAttribute attr, UColAttributeValue value, UErrorCode* status) {
  void (*ptr)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*);
  ptr = (void(*)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*))syms[166];
  ptr(coll, attr, value, status);
  return;
}

UColAttributeValue ucol_getAttribute(const UCollator* coll, UColAttribute attr, UErrorCode* status) {
  UColAttributeValue (*ptr)(const UCollator*, UColAttribute, UErrorCode*);
  ptr = (UColAttributeValue(*)(const UCollator*, UColAttribute, UErrorCode*))syms[167];
  return ptr(coll, attr, status);
}

uint32_t ucol_getVariableTop(const UCollator* coll, UErrorCode* status) {
  uint32_t (*ptr)(const UCollator*, UErrorCode*);
  ptr = (uint32_t(*)(const UCollator*, UErrorCode*))syms[168];
  return ptr(coll, status);
}

UCollator* ucol_safeClone(const UCollator* coll, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  UCollator* (*ptr)(const UCollator*, void*, int32_t*, UErrorCode*);
  ptr = (UCollator*(*)(const UCollator*, void*, int32_t*, UErrorCode*))syms[169];
  return ptr(coll, stackBuffer, pBufferSize, status);
}

int32_t ucol_getRulesEx(const UCollator* coll, UColRuleOption delta, UChar* buffer, int32_t bufferLen) {
  int32_t (*ptr)(const UCollator*, UColRuleOption, UChar*, int32_t);
  ptr = (int32_t(*)(const UCollator*, UColRuleOption, UChar*, int32_t))syms[170];
  return ptr(coll, delta, buffer, bufferLen);
}

const char* ucol_getLocaleByType(const UCollator* coll, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UCollator*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UCollator*, ULocDataLocaleType, UErrorCode*))syms[171];
  return ptr(coll, type, status);
}

USet* ucol_getTailoredSet(const UCollator* coll, UErrorCode* status) {
  USet* (*ptr)(const UCollator*, UErrorCode*);
  ptr = (USet*(*)(const UCollator*, UErrorCode*))syms[172];
  return ptr(coll, status);
}

int32_t ucol_cloneBinary(const UCollator* coll, uint8_t* buffer, int32_t capacity, UErrorCode* status) {
  int32_t (*ptr)(const UCollator*, uint8_t*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCollator*, uint8_t*, int32_t, UErrorCode*))syms[173];
  return ptr(coll, buffer, capacity, status);
}

UCollator* ucol_openBinary(const uint8_t* bin, int32_t length, const UCollator* base, UErrorCode* status) {
  UCollator* (*ptr)(const uint8_t*, int32_t, const UCollator*, UErrorCode*);
  ptr = (UCollator*(*)(const uint8_t*, int32_t, const UCollator*, UErrorCode*))syms[174];
  return ptr(bin, length, base, status);
}

/* unicode/utmscale.h */
int64_t utmscale_getTimeScaleValue(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode* status) {
  int64_t (*ptr)(UDateTimeScale, UTimeScaleValue, UErrorCode*);
  ptr = (int64_t(*)(UDateTimeScale, UTimeScaleValue, UErrorCode*))syms[175];
  return ptr(timeScale, value, status);
}

int64_t utmscale_fromInt64(int64_t otherTime, UDateTimeScale timeScale, UErrorCode* status) {
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))syms[176];
  return ptr(otherTime, timeScale, status);
}

int64_t utmscale_toInt64(int64_t universalTime, UDateTimeScale timeScale, UErrorCode* status) {
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))syms[177];
  return ptr(universalTime, timeScale, status);
}

/* unicode/uregex.h */
URegularExpression* uregex_open(const UChar* pattern, int32_t patternLength, uint32_t flags, UParseError* pe, UErrorCode* status) {
  URegularExpression* (*ptr)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*);
  ptr = (URegularExpression*(*)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*))syms[178];
  return ptr(pattern, patternLength, flags, pe, status);
}

URegularExpression* uregex_openC(const char* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  URegularExpression* (*ptr)(const char*, uint32_t, UParseError*, UErrorCode*);
  ptr = (URegularExpression*(*)(const char*, uint32_t, UParseError*, UErrorCode*))syms[179];
  return ptr(pattern, flags, pe, status);
}

void uregex_close(URegularExpression* regexp) {
  void (*ptr)(URegularExpression*);
  ptr = (void(*)(URegularExpression*))syms[180];
  ptr(regexp);
  return;
}

URegularExpression* uregex_clone(const URegularExpression* regexp, UErrorCode* status) {
  URegularExpression* (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (URegularExpression*(*)(const URegularExpression*, UErrorCode*))syms[181];
  return ptr(regexp, status);
}

const UChar* uregex_pattern(const URegularExpression* regexp, int32_t* patLength, UErrorCode* status) {
  const UChar* (*ptr)(const URegularExpression*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const URegularExpression*, int32_t*, UErrorCode*))syms[182];
  return ptr(regexp, patLength, status);
}

int32_t uregex_flags(const URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[183];
  return ptr(regexp, status);
}

void uregex_setText(URegularExpression* regexp, const UChar* text, int32_t textLength, UErrorCode* status) {
  void (*ptr)(URegularExpression*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(URegularExpression*, const UChar*, int32_t, UErrorCode*))syms[184];
  ptr(regexp, text, textLength, status);
  return;
}

const UChar* uregex_getText(URegularExpression* regexp, int32_t* textLength, UErrorCode* status) {
  const UChar* (*ptr)(URegularExpression*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(URegularExpression*, int32_t*, UErrorCode*))syms[185];
  return ptr(regexp, textLength, status);
}

UBool uregex_matches(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[186];
  return ptr(regexp, startIndex, status);
}

UBool uregex_lookingAt(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[187];
  return ptr(regexp, startIndex, status);
}

UBool uregex_find(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[188];
  return ptr(regexp, startIndex, status);
}

UBool uregex_findNext(URegularExpression* regexp, UErrorCode* status) {
  UBool (*ptr)(URegularExpression*, UErrorCode*);
  ptr = (UBool(*)(URegularExpression*, UErrorCode*))syms[189];
  return ptr(regexp, status);
}

int32_t uregex_groupCount(URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, UErrorCode*))syms[190];
  return ptr(regexp, status);
}

int32_t uregex_group(URegularExpression* regexp, int32_t groupNum, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*))syms[191];
  return ptr(regexp, groupNum, dest, destCapacity, status);
}

int32_t uregex_start(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[192];
  return ptr(regexp, groupNum, status);
}

int32_t uregex_end(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[193];
  return ptr(regexp, groupNum, status);
}

void uregex_reset(URegularExpression* regexp, int32_t index, UErrorCode* status) {
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[194];
  ptr(regexp, index, status);
  return;
}

void uregex_setRegion(URegularExpression* regexp, int32_t regionStart, int32_t regionLimit, UErrorCode* status) {
  void (*ptr)(URegularExpression*, int32_t, int32_t, UErrorCode*);
  ptr = (void(*)(URegularExpression*, int32_t, int32_t, UErrorCode*))syms[195];
  ptr(regexp, regionStart, regionLimit, status);
  return;
}

int32_t uregex_regionStart(const URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[196];
  return ptr(regexp, status);
}

int32_t uregex_regionEnd(const URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[197];
  return ptr(regexp, status);
}

UBool uregex_hasTransparentBounds(const URegularExpression* regexp, UErrorCode* status) {
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[198];
  return ptr(regexp, status);
}

void uregex_useTransparentBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))syms[199];
  ptr(regexp, b, status);
  return;
}

UBool uregex_hasAnchoringBounds(const URegularExpression* regexp, UErrorCode* status) {
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[200];
  return ptr(regexp, status);
}

void uregex_useAnchoringBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))syms[201];
  ptr(regexp, b, status);
  return;
}

UBool uregex_hitEnd(const URegularExpression* regexp, UErrorCode* status) {
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[202];
  return ptr(regexp, status);
}

UBool uregex_requireEnd(const URegularExpression* regexp, UErrorCode* status) {
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[203];
  return ptr(regexp, status);
}

int32_t uregex_replaceAll(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[204];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_replaceFirst(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[205];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_appendReplacement(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*))syms[206];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

int32_t uregex_appendTail(URegularExpression* regexp, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, UChar**, int32_t*, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, UChar**, int32_t*, UErrorCode*))syms[207];
  return ptr(regexp, destBuf, destCapacity, status);
}

int32_t uregex_split(URegularExpression* regexp, UChar* destBuf, int32_t destCapacity, int32_t* requiredCapacity, UChar* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  int32_t (*ptr)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*);
  ptr = (int32_t(*)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*))syms[208];
  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}

void uregex_setTimeLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[209];
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getTimeLimit(const URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[210];
  return ptr(regexp, status);
}

void uregex_setStackLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[211];
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getStackLimit(const URegularExpression* regexp, UErrorCode* status) {
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[212];
  return ptr(regexp, status);
}

void uregex_setMatchCallback(URegularExpression* regexp, URegexMatchCallback* callback, const void* context, UErrorCode* status) {
  void (*ptr)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*);
  ptr = (void(*)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*))syms[213];
  ptr(regexp, callback, context, status);
  return;
}

void uregex_getMatchCallback(const URegularExpression* regexp, URegexMatchCallback** callback, const void** context, UErrorCode* status) {
  void (*ptr)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*);
  ptr = (void(*)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*))syms[214];
  ptr(regexp, callback, context, status);
  return;
}

/* unicode/uspoof.h */
USpoofChecker* uspoof_open(UErrorCode* status) {
  USpoofChecker* (*ptr)(UErrorCode*);
  ptr = (USpoofChecker*(*)(UErrorCode*))syms[215];
  return ptr(status);
}

void uspoof_close(USpoofChecker* sc) {
  void (*ptr)(USpoofChecker*);
  ptr = (void(*)(USpoofChecker*))syms[216];
  ptr(sc);
  return;
}

USpoofChecker* uspoof_clone(const USpoofChecker* sc, UErrorCode* status) {
  USpoofChecker* (*ptr)(const USpoofChecker*, UErrorCode*);
  ptr = (USpoofChecker*(*)(const USpoofChecker*, UErrorCode*))syms[217];
  return ptr(sc, status);
}

void uspoof_setChecks(USpoofChecker* sc, int32_t checks, UErrorCode* status) {
  void (*ptr)(USpoofChecker*, int32_t, UErrorCode*);
  ptr = (void(*)(USpoofChecker*, int32_t, UErrorCode*))syms[218];
  ptr(sc, checks, status);
  return;
}

int32_t uspoof_getChecks(const USpoofChecker* sc, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, UErrorCode*))syms[219];
  return ptr(sc, status);
}

void uspoof_setAllowedLocales(USpoofChecker* sc, const char* localesList, UErrorCode* status) {
  void (*ptr)(USpoofChecker*, const char*, UErrorCode*);
  ptr = (void(*)(USpoofChecker*, const char*, UErrorCode*))syms[220];
  ptr(sc, localesList, status);
  return;
}

const char* uspoof_getAllowedLocales(USpoofChecker* sc, UErrorCode* status) {
  const char* (*ptr)(USpoofChecker*, UErrorCode*);
  ptr = (const char*(*)(USpoofChecker*, UErrorCode*))syms[221];
  return ptr(sc, status);
}

void uspoof_setAllowedChars(USpoofChecker* sc, const USet* chars, UErrorCode* status) {
  void (*ptr)(USpoofChecker*, const USet*, UErrorCode*);
  ptr = (void(*)(USpoofChecker*, const USet*, UErrorCode*))syms[222];
  ptr(sc, chars, status);
  return;
}

const USet* uspoof_getAllowedChars(const USpoofChecker* sc, UErrorCode* status) {
  const USet* (*ptr)(const USpoofChecker*, UErrorCode*);
  ptr = (const USet*(*)(const USpoofChecker*, UErrorCode*))syms[223];
  return ptr(sc, status);
}

int32_t uspoof_check(const USpoofChecker* sc, const UChar* text, int32_t length, int32_t* position, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[224];
  return ptr(sc, text, length, position, status);
}

int32_t uspoof_checkUTF8(const USpoofChecker* sc, const char* text, int32_t length, int32_t* position, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*))syms[225];
  return ptr(sc, text, length, position, status);
}

int32_t uspoof_areConfusable(const USpoofChecker* sc, const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[226];
  return ptr(sc, s1, length1, s2, length2, status);
}

int32_t uspoof_areConfusableUTF8(const USpoofChecker* sc, const char* s1, int32_t length1, const char* s2, int32_t length2, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*))syms[227];
  return ptr(sc, s1, length1, s2, length2, status);
}

int32_t uspoof_getSkeleton(const USpoofChecker* sc, uint32_t type, const UChar* s, int32_t length, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[228];
  return ptr(sc, type, s, length, dest, destCapacity, status);
}

int32_t uspoof_getSkeletonUTF8(const USpoofChecker* sc, uint32_t type, const char* s, int32_t length, char* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*))syms[229];
  return ptr(sc, type, s, length, dest, destCapacity, status);
}

/* unicode/udatpg.h */
UDateTimePatternGenerator* udatpg_open(const char* locale, UErrorCode* pErrorCode) {
  UDateTimePatternGenerator* (*ptr)(const char*, UErrorCode*);
  ptr = (UDateTimePatternGenerator*(*)(const char*, UErrorCode*))syms[230];
  return ptr(locale, pErrorCode);
}

UDateTimePatternGenerator* udatpg_openEmpty(UErrorCode* pErrorCode) {
  UDateTimePatternGenerator* (*ptr)(UErrorCode*);
  ptr = (UDateTimePatternGenerator*(*)(UErrorCode*))syms[231];
  return ptr(pErrorCode);
}

void udatpg_close(UDateTimePatternGenerator* dtpg) {
  void (*ptr)(UDateTimePatternGenerator*);
  ptr = (void(*)(UDateTimePatternGenerator*))syms[232];
  ptr(dtpg);
  return;
}

UDateTimePatternGenerator* udatpg_clone(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  UDateTimePatternGenerator* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  ptr = (UDateTimePatternGenerator*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[233];
  return ptr(dtpg, pErrorCode);
}

int32_t udatpg_getBestPattern(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[234];
  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}

int32_t udatpg_getSkeleton(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* skeleton, int32_t capacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[235];
  return ptr(dtpg, pattern, length, skeleton, capacity, pErrorCode);
}

int32_t udatpg_getBaseSkeleton(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* baseSkeleton, int32_t capacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[236];
  return ptr(dtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}

UDateTimePatternConflict udatpg_addPattern(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, UBool override, UChar* conflictingPattern, int32_t capacity, int32_t* pLength, UErrorCode* pErrorCode) {
  UDateTimePatternConflict (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*);
  ptr = (UDateTimePatternConflict(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*))syms[237];
  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}

void udatpg_setAppendItemFormat(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))syms[238];
  ptr(dtpg, field, value, length);
  return;
}

const UChar* udatpg_getAppendItemFormat(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  const UChar* (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))syms[239];
  return ptr(dtpg, field, pLength);
}

void udatpg_setAppendItemName(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))syms[240];
  ptr(dtpg, field, value, length);
  return;
}

const UChar* udatpg_getAppendItemName(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  const UChar* (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))syms[241];
  return ptr(dtpg, field, pLength);
}

void udatpg_setDateTimeFormat(const UDateTimePatternGenerator* dtpg, const UChar* dtFormat, int32_t length) {
  void (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t);
  ptr = (void(*)(const UDateTimePatternGenerator*, const UChar*, int32_t))syms[242];
  ptr(dtpg, dtFormat, length);
  return;
}

const UChar* udatpg_getDateTimeFormat(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  const UChar* (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, int32_t*))syms[243];
  return ptr(dtpg, pLength);
}

void udatpg_setDecimal(UDateTimePatternGenerator* dtpg, const UChar* decimal, int32_t length) {
  void (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, const UChar*, int32_t))syms[244];
  ptr(dtpg, decimal, length);
  return;
}

const UChar* udatpg_getDecimal(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  const UChar* (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, int32_t*))syms[245];
  return ptr(dtpg, pLength);
}

int32_t udatpg_replaceFieldTypes(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[246];
  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}

UEnumeration* udatpg_openSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  ptr = (UEnumeration*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[247];
  return ptr(dtpg, pErrorCode);
}

UEnumeration* udatpg_openBaseSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  ptr = (UEnumeration*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[248];
  return ptr(dtpg, pErrorCode);
}

const UChar* udatpg_getPatternForSkeleton(const UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t skeletonLength, int32_t* pLength) {
  const UChar* (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*))syms[249];
  return ptr(dtpg, skeleton, skeletonLength, pLength);
}

/* unicode/ucsdet.h */
UCharsetDetector* ucsdet_open(UErrorCode* status) {
  UCharsetDetector* (*ptr)(UErrorCode*);
  ptr = (UCharsetDetector*(*)(UErrorCode*))syms[250];
  return ptr(status);
}

void ucsdet_close(UCharsetDetector* ucsd) {
  void (*ptr)(UCharsetDetector*);
  ptr = (void(*)(UCharsetDetector*))syms[251];
  ptr(ucsd);
  return;
}

void ucsdet_setText(UCharsetDetector* ucsd, const char* textIn, int32_t len, UErrorCode* status) {
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))syms[252];
  ptr(ucsd, textIn, len, status);
  return;
}

void ucsdet_setDeclaredEncoding(UCharsetDetector* ucsd, const char* encoding, int32_t length, UErrorCode* status) {
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))syms[253];
  ptr(ucsd, encoding, length, status);
  return;
}

const UCharsetMatch* ucsdet_detect(UCharsetDetector* ucsd, UErrorCode* status) {
  const UCharsetMatch* (*ptr)(UCharsetDetector*, UErrorCode*);
  ptr = (const UCharsetMatch*(*)(UCharsetDetector*, UErrorCode*))syms[254];
  return ptr(ucsd, status);
}

const UCharsetMatch** ucsdet_detectAll(UCharsetDetector* ucsd, int32_t* matchesFound, UErrorCode* status) {
  const UCharsetMatch** (*ptr)(UCharsetDetector*, int32_t*, UErrorCode*);
  ptr = (const UCharsetMatch**(*)(UCharsetDetector*, int32_t*, UErrorCode*))syms[255];
  return ptr(ucsd, matchesFound, status);
}

const char* ucsdet_getName(const UCharsetMatch* ucsm, UErrorCode* status) {
  const char* (*ptr)(const UCharsetMatch*, UErrorCode*);
  ptr = (const char*(*)(const UCharsetMatch*, UErrorCode*))syms[256];
  return ptr(ucsm, status);
}

int32_t ucsdet_getConfidence(const UCharsetMatch* ucsm, UErrorCode* status) {
  int32_t (*ptr)(const UCharsetMatch*, UErrorCode*);
  ptr = (int32_t(*)(const UCharsetMatch*, UErrorCode*))syms[257];
  return ptr(ucsm, status);
}

const char* ucsdet_getLanguage(const UCharsetMatch* ucsm, UErrorCode* status) {
  const char* (*ptr)(const UCharsetMatch*, UErrorCode*);
  ptr = (const char*(*)(const UCharsetMatch*, UErrorCode*))syms[258];
  return ptr(ucsm, status);
}

int32_t ucsdet_getUChars(const UCharsetMatch* ucsm, UChar* buf, int32_t cap, UErrorCode* status) {
  int32_t (*ptr)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*))syms[259];
  return ptr(ucsm, buf, cap, status);
}

UEnumeration* ucsdet_getAllDetectableCharsets(const UCharsetDetector* ucsd, UErrorCode* status) {
  UEnumeration* (*ptr)(const UCharsetDetector*, UErrorCode*);
  ptr = (UEnumeration*(*)(const UCharsetDetector*, UErrorCode*))syms[260];
  return ptr(ucsd, status);
}

UBool ucsdet_isInputFilterEnabled(const UCharsetDetector* ucsd) {
  UBool (*ptr)(const UCharsetDetector*);
  ptr = (UBool(*)(const UCharsetDetector*))syms[261];
  return ptr(ucsd);
}

UBool ucsdet_enableInputFilter(UCharsetDetector* ucsd, UBool filter) {
  UBool (*ptr)(UCharsetDetector*, UBool);
  ptr = (UBool(*)(UCharsetDetector*, UBool))syms[262];
  return ptr(ucsd, filter);
}

/* unicode/ucal.h */
UEnumeration* ucal_openTimeZones(UErrorCode* ec) {
  UEnumeration* (*ptr)(UErrorCode*);
  ptr = (UEnumeration*(*)(UErrorCode*))syms[263];
  return ptr(ec);
}

UEnumeration* ucal_openCountryTimeZones(const char* country, UErrorCode* ec) {
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[264];
  return ptr(country, ec);
}

int32_t ucal_getDefaultTimeZone(UChar* result, int32_t resultCapacity, UErrorCode* ec) {
  int32_t (*ptr)(UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UChar*, int32_t, UErrorCode*))syms[265];
  return ptr(result, resultCapacity, ec);
}

void ucal_setDefaultTimeZone(const UChar* zoneID, UErrorCode* ec) {
  void (*ptr)(const UChar*, UErrorCode*);
  ptr = (void(*)(const UChar*, UErrorCode*))syms[266];
  ptr(zoneID, ec);
  return;
}

int32_t ucal_getDSTSavings(const UChar* zoneID, UErrorCode* ec) {
  int32_t (*ptr)(const UChar*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, UErrorCode*))syms[267];
  return ptr(zoneID, ec);
}

UDate ucal_getNow(void) {
  UDate (*ptr)(void);
  ptr = (UDate(*)(void))syms[268];
  return ptr();
}

UCalendar* ucal_open(const UChar* zoneID, int32_t len, const char* locale, UCalendarType type, UErrorCode* status) {
  UCalendar* (*ptr)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*);
  ptr = (UCalendar*(*)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*))syms[269];
  return ptr(zoneID, len, locale, type, status);
}

void ucal_close(UCalendar* cal) {
  void (*ptr)(UCalendar*);
  ptr = (void(*)(UCalendar*))syms[270];
  ptr(cal);
  return;
}

UCalendar* ucal_clone(const UCalendar* cal, UErrorCode* status) {
  UCalendar* (*ptr)(const UCalendar*, UErrorCode*);
  ptr = (UCalendar*(*)(const UCalendar*, UErrorCode*))syms[271];
  return ptr(cal, status);
}

void ucal_setTimeZone(UCalendar* cal, const UChar* zoneID, int32_t len, UErrorCode* status) {
  void (*ptr)(UCalendar*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UCalendar*, const UChar*, int32_t, UErrorCode*))syms[272];
  ptr(cal, zoneID, len, status);
  return;
}

int32_t ucal_getTimeZoneDisplayName(const UCalendar* cal, UCalendarDisplayNameType type, const char* locale, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*))syms[273];
  return ptr(cal, type, locale, result, resultLength, status);
}

UBool ucal_inDaylightTime(const UCalendar* cal, UErrorCode* status) {
  UBool (*ptr)(const UCalendar*, UErrorCode*);
  ptr = (UBool(*)(const UCalendar*, UErrorCode*))syms[274];
  return ptr(cal, status);
}

void ucal_setGregorianChange(UCalendar* cal, UDate date, UErrorCode* pErrorCode) {
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))syms[275];
  ptr(cal, date, pErrorCode);
  return;
}

UDate ucal_getGregorianChange(const UCalendar* cal, UErrorCode* pErrorCode) {
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))syms[276];
  return ptr(cal, pErrorCode);
}

int32_t ucal_getAttribute(const UCalendar* cal, UCalendarAttribute attr) {
  int32_t (*ptr)(const UCalendar*, UCalendarAttribute);
  ptr = (int32_t(*)(const UCalendar*, UCalendarAttribute))syms[277];
  return ptr(cal, attr);
}

void ucal_setAttribute(UCalendar* cal, UCalendarAttribute attr, int32_t newValue) {
  void (*ptr)(UCalendar*, UCalendarAttribute, int32_t);
  ptr = (void(*)(UCalendar*, UCalendarAttribute, int32_t))syms[278];
  ptr(cal, attr, newValue);
  return;
}

const char* ucal_getAvailable(int32_t localeIndex) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[279];
  return ptr(localeIndex);
}

int32_t ucal_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[280];
  return ptr();
}

UDate ucal_getMillis(const UCalendar* cal, UErrorCode* status) {
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))syms[281];
  return ptr(cal, status);
}

void ucal_setMillis(UCalendar* cal, UDate dateTime, UErrorCode* status) {
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))syms[282];
  ptr(cal, dateTime, status);
  return;
}

void ucal_setDate(UCalendar* cal, int32_t year, int32_t month, int32_t date, UErrorCode* status) {
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*);
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*))syms[283];
  ptr(cal, year, month, date, status);
  return;
}

void ucal_setDateTime(UCalendar* cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode* status) {
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*);
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*))syms[284];
  ptr(cal, year, month, date, hour, minute, second, status);
  return;
}

UBool ucal_equivalentTo(const UCalendar* cal1, const UCalendar* cal2) {
  UBool (*ptr)(const UCalendar*, const UCalendar*);
  ptr = (UBool(*)(const UCalendar*, const UCalendar*))syms[285];
  return ptr(cal1, cal2);
}

void ucal_add(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))syms[286];
  ptr(cal, field, amount, status);
  return;
}

void ucal_roll(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))syms[287];
  ptr(cal, field, amount, status);
  return;
}

int32_t ucal_get(const UCalendar* cal, UCalendarDateFields field, UErrorCode* status) {
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UErrorCode*);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UErrorCode*))syms[288];
  return ptr(cal, field, status);
}

void ucal_set(UCalendar* cal, UCalendarDateFields field, int32_t value) {
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t))syms[289];
  ptr(cal, field, value);
  return;
}

UBool ucal_isSet(const UCalendar* cal, UCalendarDateFields field) {
  UBool (*ptr)(const UCalendar*, UCalendarDateFields);
  ptr = (UBool(*)(const UCalendar*, UCalendarDateFields))syms[290];
  return ptr(cal, field);
}

void ucal_clearField(UCalendar* cal, UCalendarDateFields field) {
  void (*ptr)(UCalendar*, UCalendarDateFields);
  ptr = (void(*)(UCalendar*, UCalendarDateFields))syms[291];
  ptr(cal, field);
  return;
}

void ucal_clear(UCalendar* calendar) {
  void (*ptr)(UCalendar*);
  ptr = (void(*)(UCalendar*))syms[292];
  ptr(calendar);
  return;
}

int32_t ucal_getLimit(const UCalendar* cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode* status) {
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*);
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*))syms[293];
  return ptr(cal, field, type, status);
}

const char* ucal_getLocaleByType(const UCalendar* cal, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UCalendar*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UCalendar*, ULocDataLocaleType, UErrorCode*))syms[294];
  return ptr(cal, type, status);
}

const char* ucal_getTZDataVersion(UErrorCode* status) {
  const char* (*ptr)(UErrorCode*);
  ptr = (const char*(*)(UErrorCode*))syms[295];
  return ptr(status);
}

int32_t ucal_getCanonicalTimeZoneID(const UChar* id, int32_t len, UChar* result, int32_t resultCapacity, UBool* isSystemID, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*))syms[296];
  return ptr(id, len, result, resultCapacity, isSystemID, status);
}

const char* ucal_getType(const UCalendar* cal, UErrorCode* status) {
  const char* (*ptr)(const UCalendar*, UErrorCode*);
  ptr = (const char*(*)(const UCalendar*, UErrorCode*))syms[297];
  return ptr(cal, status);
}

UEnumeration* ucal_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[298];
  return ptr(key, locale, commonlyUsed, status);
}

/* unicode/ucnv_err.h */
void UCNV_FROM_U_CALLBACK_STOP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[299];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_STOP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[300];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SKIP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[301];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SUBSTITUTE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[302];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_ESCAPE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[303];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SKIP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[304];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SUBSTITUTE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[305];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_ESCAPE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[306];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

/* unicode/ucnv.h */
int ucnv_compareNames(const char* name1, const char* name2) {
  int (*ptr)(const char*, const char*);
  ptr = (int(*)(const char*, const char*))syms[307];
  return ptr(name1, name2);
}

UConverter* ucnv_open(const char* converterName, UErrorCode* err) {
  UConverter* (*ptr)(const char*, UErrorCode*);
  ptr = (UConverter*(*)(const char*, UErrorCode*))syms[308];
  return ptr(converterName, err);
}

UConverter* ucnv_openU(const UChar* name, UErrorCode* err) {
  UConverter* (*ptr)(const UChar*, UErrorCode*);
  ptr = (UConverter*(*)(const UChar*, UErrorCode*))syms[309];
  return ptr(name, err);
}

UConverter* ucnv_openCCSID(int32_t codepage, UConverterPlatform platform, UErrorCode* err) {
  UConverter* (*ptr)(int32_t, UConverterPlatform, UErrorCode*);
  ptr = (UConverter*(*)(int32_t, UConverterPlatform, UErrorCode*))syms[310];
  return ptr(codepage, platform, err);
}

UConverter* ucnv_openPackage(const char* packageName, const char* converterName, UErrorCode* err) {
  UConverter* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (UConverter*(*)(const char*, const char*, UErrorCode*))syms[311];
  return ptr(packageName, converterName, err);
}

UConverter* ucnv_safeClone(const UConverter* cnv, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  UConverter* (*ptr)(const UConverter*, void*, int32_t*, UErrorCode*);
  ptr = (UConverter*(*)(const UConverter*, void*, int32_t*, UErrorCode*))syms[312];
  return ptr(cnv, stackBuffer, pBufferSize, status);
}

void ucnv_close(UConverter* converter) {
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[313];
  ptr(converter);
  return;
}

void ucnv_getSubstChars(const UConverter* converter, char* subChars, int8_t* len, UErrorCode* err) {
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))syms[314];
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstChars(UConverter* converter, const char* subChars, int8_t len, UErrorCode* err) {
  void (*ptr)(UConverter*, const char*, int8_t, UErrorCode*);
  ptr = (void(*)(UConverter*, const char*, int8_t, UErrorCode*))syms[315];
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstString(UConverter* cnv, const UChar* s, int32_t length, UErrorCode* err) {
  void (*ptr)(UConverter*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UConverter*, const UChar*, int32_t, UErrorCode*))syms[316];
  ptr(cnv, s, length, err);
  return;
}

void ucnv_getInvalidChars(const UConverter* converter, char* errBytes, int8_t* len, UErrorCode* err) {
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))syms[317];
  ptr(converter, errBytes, len, err);
  return;
}

void ucnv_getInvalidUChars(const UConverter* converter, UChar* errUChars, int8_t* len, UErrorCode* err) {
  void (*ptr)(const UConverter*, UChar*, int8_t*, UErrorCode*);
  ptr = (void(*)(const UConverter*, UChar*, int8_t*, UErrorCode*))syms[318];
  ptr(converter, errUChars, len, err);
  return;
}

void ucnv_reset(UConverter* converter) {
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[319];
  ptr(converter);
  return;
}

void ucnv_resetToUnicode(UConverter* converter) {
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[320];
  ptr(converter);
  return;
}

void ucnv_resetFromUnicode(UConverter* converter) {
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[321];
  ptr(converter);
  return;
}

int8_t ucnv_getMaxCharSize(const UConverter* converter) {
  int8_t (*ptr)(const UConverter*);
  ptr = (int8_t(*)(const UConverter*))syms[322];
  return ptr(converter);
}

int8_t ucnv_getMinCharSize(const UConverter* converter) {
  int8_t (*ptr)(const UConverter*);
  ptr = (int8_t(*)(const UConverter*))syms[323];
  return ptr(converter);
}

int32_t ucnv_getDisplayName(const UConverter* converter, const char* displayLocale, UChar* displayName, int32_t displayNameCapacity, UErrorCode* err) {
  int32_t (*ptr)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*))syms[324];
  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}

const char* ucnv_getName(const UConverter* converter, UErrorCode* err) {
  const char* (*ptr)(const UConverter*, UErrorCode*);
  ptr = (const char*(*)(const UConverter*, UErrorCode*))syms[325];
  return ptr(converter, err);
}

int32_t ucnv_getCCSID(const UConverter* converter, UErrorCode* err) {
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[326];
  return ptr(converter, err);
}

UConverterPlatform ucnv_getPlatform(const UConverter* converter, UErrorCode* err) {
  UConverterPlatform (*ptr)(const UConverter*, UErrorCode*);
  ptr = (UConverterPlatform(*)(const UConverter*, UErrorCode*))syms[327];
  return ptr(converter, err);
}

UConverterType ucnv_getType(const UConverter* converter) {
  UConverterType (*ptr)(const UConverter*);
  ptr = (UConverterType(*)(const UConverter*))syms[328];
  return ptr(converter);
}

void ucnv_getStarters(const UConverter* converter, UBool starters [ 256], UErrorCode* err) {
  void (*ptr)(const UConverter*, UBool [ 256], UErrorCode*);
  ptr = (void(*)(const UConverter*, UBool [ 256], UErrorCode*))syms[329];
  ptr(converter, starters, err);
  return;
}

void ucnv_getUnicodeSet(const UConverter* cnv, USet* setFillIn, UConverterUnicodeSet whichSet, UErrorCode* pErrorCode) {
  void (*ptr)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*);
  ptr = (void(*)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*))syms[330];
  ptr(cnv, setFillIn, whichSet, pErrorCode);
  return;
}

void ucnv_getToUCallBack(const UConverter* converter, UConverterToUCallback* action, const void** context) {
  void (*ptr)(const UConverter*, UConverterToUCallback*, const void**);
  ptr = (void(*)(const UConverter*, UConverterToUCallback*, const void**))syms[331];
  ptr(converter, action, context);
  return;
}

void ucnv_getFromUCallBack(const UConverter* converter, UConverterFromUCallback* action, const void** context) {
  void (*ptr)(const UConverter*, UConverterFromUCallback*, const void**);
  ptr = (void(*)(const UConverter*, UConverterFromUCallback*, const void**))syms[332];
  ptr(converter, action, context);
  return;
}

void ucnv_setToUCallBack(UConverter* converter, UConverterToUCallback newAction, const void* newContext, UConverterToUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  void (*ptr)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*);
  ptr = (void(*)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*))syms[333];
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_setFromUCallBack(UConverter* converter, UConverterFromUCallback newAction, const void* newContext, UConverterFromUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  void (*ptr)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*);
  ptr = (void(*)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*))syms[334];
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_fromUnicode(UConverter* converter, char** target, const char* targetLimit, const UChar** source, const UChar* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  void (*ptr)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*);
  ptr = (void(*)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*))syms[335];
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

void ucnv_toUnicode(UConverter* converter, UChar** target, const UChar* targetLimit, const char** source, const char* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  void (*ptr)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*);
  ptr = (void(*)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*))syms[336];
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

int32_t ucnv_fromUChars(UConverter* cnv, char* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*))syms[337];
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucnv_toUChars(UConverter* cnv, UChar* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*))syms[338];
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

UChar32 ucnv_getNextUChar(UConverter* converter, const char** source, const char* sourceLimit, UErrorCode* err) {
  UChar32 (*ptr)(UConverter*, const char**, const char*, UErrorCode*);
  ptr = (UChar32(*)(UConverter*, const char**, const char*, UErrorCode*))syms[339];
  return ptr(converter, source, sourceLimit, err);
}

void ucnv_convertEx(UConverter* targetCnv, UConverter* sourceCnv, char** target, const char* targetLimit, const char** source, const char* sourceLimit, UChar* pivotStart, UChar** pivotSource, UChar** pivotTarget, const UChar* pivotLimit, UBool reset, UBool flush, UErrorCode* pErrorCode) {
  void (*ptr)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*);
  ptr = (void(*)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*))syms[340];
  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
  return;
}

int32_t ucnv_convert(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[341];
  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_toAlgorithmic(UConverterType algorithmicType, UConverter* cnv, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[342];
  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_fromAlgorithmic(UConverter* cnv, UConverterType algorithmicType, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*))syms[343];
  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_flushCache(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[344];
  return ptr();
}

int32_t ucnv_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[345];
  return ptr();
}

const char* ucnv_getAvailableName(int32_t n) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[346];
  return ptr(n);
}

UEnumeration* ucnv_openAllNames(UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(UErrorCode*);
  ptr = (UEnumeration*(*)(UErrorCode*))syms[347];
  return ptr(pErrorCode);
}

uint16_t ucnv_countAliases(const char* alias, UErrorCode* pErrorCode) {
  uint16_t (*ptr)(const char*, UErrorCode*);
  ptr = (uint16_t(*)(const char*, UErrorCode*))syms[348];
  return ptr(alias, pErrorCode);
}

const char* ucnv_getAlias(const char* alias, uint16_t n, UErrorCode* pErrorCode) {
  const char* (*ptr)(const char*, uint16_t, UErrorCode*);
  ptr = (const char*(*)(const char*, uint16_t, UErrorCode*))syms[349];
  return ptr(alias, n, pErrorCode);
}

void ucnv_getAliases(const char* alias, const char** aliases, UErrorCode* pErrorCode) {
  void (*ptr)(const char*, const char**, UErrorCode*);
  ptr = (void(*)(const char*, const char**, UErrorCode*))syms[350];
  ptr(alias, aliases, pErrorCode);
  return;
}

UEnumeration* ucnv_openStandardNames(const char* convName, const char* standard, UErrorCode* pErrorCode) {
  UEnumeration* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, const char*, UErrorCode*))syms[351];
  return ptr(convName, standard, pErrorCode);
}

uint16_t ucnv_countStandards(void) {
  uint16_t (*ptr)(void);
  ptr = (uint16_t(*)(void))syms[352];
  return ptr();
}

const char* ucnv_getStandard(uint16_t n, UErrorCode* pErrorCode) {
  const char* (*ptr)(uint16_t, UErrorCode*);
  ptr = (const char*(*)(uint16_t, UErrorCode*))syms[353];
  return ptr(n, pErrorCode);
}

const char* ucnv_getStandardName(const char* name, const char* standard, UErrorCode* pErrorCode) {
  const char* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (const char*(*)(const char*, const char*, UErrorCode*))syms[354];
  return ptr(name, standard, pErrorCode);
}

const char* ucnv_getCanonicalName(const char* alias, const char* standard, UErrorCode* pErrorCode) {
  const char* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (const char*(*)(const char*, const char*, UErrorCode*))syms[355];
  return ptr(alias, standard, pErrorCode);
}

const char* ucnv_getDefaultName(void) {
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[356];
  return ptr();
}

void ucnv_setDefaultName(const char* name) {
  void (*ptr)(const char*);
  ptr = (void(*)(const char*))syms[357];
  ptr(name);
  return;
}

void ucnv_fixFileSeparator(const UConverter* cnv, UChar* source, int32_t sourceLen) {
  void (*ptr)(const UConverter*, UChar*, int32_t);
  ptr = (void(*)(const UConverter*, UChar*, int32_t))syms[358];
  ptr(cnv, source, sourceLen);
  return;
}

UBool ucnv_isAmbiguous(const UConverter* cnv) {
  UBool (*ptr)(const UConverter*);
  ptr = (UBool(*)(const UConverter*))syms[359];
  return ptr(cnv);
}

void ucnv_setFallback(UConverter* cnv, UBool usesFallback) {
  void (*ptr)(UConverter*, UBool);
  ptr = (void(*)(UConverter*, UBool))syms[360];
  ptr(cnv, usesFallback);
  return;
}

UBool ucnv_usesFallback(const UConverter* cnv) {
  UBool (*ptr)(const UConverter*);
  ptr = (UBool(*)(const UConverter*))syms[361];
  return ptr(cnv);
}

const char* ucnv_detectUnicodeSignature(const char* source, int32_t sourceLength, int32_t* signatureLength, UErrorCode* pErrorCode) {
  const char* (*ptr)(const char*, int32_t, int32_t*, UErrorCode*);
  ptr = (const char*(*)(const char*, int32_t, int32_t*, UErrorCode*))syms[362];
  return ptr(source, sourceLength, signatureLength, pErrorCode);
}

int32_t ucnv_fromUCountPending(const UConverter* cnv, UErrorCode* status) {
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[363];
  return ptr(cnv, status);
}

int32_t ucnv_toUCountPending(const UConverter* cnv, UErrorCode* status) {
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[364];
  return ptr(cnv, status);
}

/* unicode/ustring.h */
int32_t u_strlen(const UChar* s) {
  int32_t (*ptr)(const UChar*);
  ptr = (int32_t(*)(const UChar*))syms[365];
  return ptr(s);
}

int32_t u_countChar32(const UChar* s, int32_t length) {
  int32_t (*ptr)(const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, int32_t))syms[366];
  return ptr(s, length);
}

UBool u_strHasMoreChar32Than(const UChar* s, int32_t length, int32_t number) {
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))syms[367];
  return ptr(s, length, number);
}

UChar* u_strcat(UChar* dst, const UChar* src) {
  UChar* (*ptr)(UChar*, const UChar*);
  ptr = (UChar*(*)(UChar*, const UChar*))syms[368];
  return ptr(dst, src);
}

UChar* u_strncat(UChar* dst, const UChar* src, int32_t n) {
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[369];
  return ptr(dst, src, n);
}

UChar* u_strstr(const UChar* s, const UChar* substring) {
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[370];
  return ptr(s, substring);
}

UChar* u_strFindFirst(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  UChar* (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UChar*(*)(const UChar*, int32_t, const UChar*, int32_t))syms[371];
  return ptr(s, length, substring, subLength);
}

UChar* u_strchr(const UChar* s, UChar c) {
  UChar* (*ptr)(const UChar*, UChar);
  ptr = (UChar*(*)(const UChar*, UChar))syms[372];
  return ptr(s, c);
}

UChar* u_strchr32(const UChar* s, UChar32 c) {
  UChar* (*ptr)(const UChar*, UChar32);
  ptr = (UChar*(*)(const UChar*, UChar32))syms[373];
  return ptr(s, c);
}

UChar* u_strrstr(const UChar* s, const UChar* substring) {
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[374];
  return ptr(s, substring);
}

UChar* u_strFindLast(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  UChar* (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UChar*(*)(const UChar*, int32_t, const UChar*, int32_t))syms[375];
  return ptr(s, length, substring, subLength);
}

UChar* u_strrchr(const UChar* s, UChar c) {
  UChar* (*ptr)(const UChar*, UChar);
  ptr = (UChar*(*)(const UChar*, UChar))syms[376];
  return ptr(s, c);
}

UChar* u_strrchr32(const UChar* s, UChar32 c) {
  UChar* (*ptr)(const UChar*, UChar32);
  ptr = (UChar*(*)(const UChar*, UChar32))syms[377];
  return ptr(s, c);
}

UChar* u_strpbrk(const UChar* string, const UChar* matchSet) {
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[378];
  return ptr(string, matchSet);
}

int32_t u_strcspn(const UChar* string, const UChar* matchSet) {
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[379];
  return ptr(string, matchSet);
}

int32_t u_strspn(const UChar* string, const UChar* matchSet) {
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[380];
  return ptr(string, matchSet);
}

UChar* u_strtok_r(UChar* src, const UChar* delim, UChar** saveState) {
  UChar* (*ptr)(UChar*, const UChar*, UChar**);
  ptr = (UChar*(*)(UChar*, const UChar*, UChar**))syms[381];
  return ptr(src, delim, saveState);
}

int32_t u_strcmp(const UChar* s1, const UChar* s2) {
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[382];
  return ptr(s1, s2);
}

int32_t u_strcmpCodePointOrder(const UChar* s1, const UChar* s2) {
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[383];
  return ptr(s1, s2);
}

int32_t u_strCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UBool codePointOrder) {
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UBool);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UBool))syms[384];
  return ptr(s1, length1, s2, length2, codePointOrder);
}

int32_t u_strCompareIter(UCharIterator* iter1, UCharIterator* iter2, UBool codePointOrder) {
  int32_t (*ptr)(UCharIterator*, UCharIterator*, UBool);
  ptr = (int32_t(*)(UCharIterator*, UCharIterator*, UBool))syms[385];
  return ptr(iter1, iter2, codePointOrder);
}

int32_t u_strCaseCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[386];
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

int32_t u_strncmp(const UChar* ucs1, const UChar* ucs2, int32_t n) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[387];
  return ptr(ucs1, ucs2, n);
}

int32_t u_strncmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t n) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[388];
  return ptr(s1, s2, n);
}

int32_t u_strcasecmp(const UChar* s1, const UChar* s2, uint32_t options) {
  int32_t (*ptr)(const UChar*, const UChar*, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, uint32_t))syms[389];
  return ptr(s1, s2, options);
}

int32_t u_strncasecmp(const UChar* s1, const UChar* s2, int32_t n, uint32_t options) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))syms[390];
  return ptr(s1, s2, n, options);
}

int32_t u_memcasecmp(const UChar* s1, const UChar* s2, int32_t length, uint32_t options) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))syms[391];
  return ptr(s1, s2, length, options);
}

UChar* u_strcpy(UChar* dst, const UChar* src) {
  UChar* (*ptr)(UChar*, const UChar*);
  ptr = (UChar*(*)(UChar*, const UChar*))syms[392];
  return ptr(dst, src);
}

UChar* u_strncpy(UChar* dst, const UChar* src, int32_t n) {
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[393];
  return ptr(dst, src, n);
}

UChar* u_uastrcpy(UChar* dst, const char* src) {
  UChar* (*ptr)(UChar*, const char*);
  ptr = (UChar*(*)(UChar*, const char*))syms[394];
  return ptr(dst, src);
}

UChar* u_uastrncpy(UChar* dst, const char* src, int32_t n) {
  UChar* (*ptr)(UChar*, const char*, int32_t);
  ptr = (UChar*(*)(UChar*, const char*, int32_t))syms[395];
  return ptr(dst, src, n);
}

char* u_austrcpy(char* dst, const UChar* src) {
  char* (*ptr)(char*, const UChar*);
  ptr = (char*(*)(char*, const UChar*))syms[396];
  return ptr(dst, src);
}

char* u_austrncpy(char* dst, const UChar* src, int32_t n) {
  char* (*ptr)(char*, const UChar*, int32_t);
  ptr = (char*(*)(char*, const UChar*, int32_t))syms[397];
  return ptr(dst, src, n);
}

UChar* u_memcpy(UChar* dest, const UChar* src, int32_t count) {
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[398];
  return ptr(dest, src, count);
}

UChar* u_memmove(UChar* dest, const UChar* src, int32_t count) {
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[399];
  return ptr(dest, src, count);
}

UChar* u_memset(UChar* dest, UChar c, int32_t count) {
  UChar* (*ptr)(UChar*, UChar, int32_t);
  ptr = (UChar*(*)(UChar*, UChar, int32_t))syms[400];
  return ptr(dest, c, count);
}

int32_t u_memcmp(const UChar* buf1, const UChar* buf2, int32_t count) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[401];
  return ptr(buf1, buf2, count);
}

int32_t u_memcmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t count) {
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[402];
  return ptr(s1, s2, count);
}

UChar* u_memchr(const UChar* s, UChar c, int32_t count) {
  UChar* (*ptr)(const UChar*, UChar, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar, int32_t))syms[403];
  return ptr(s, c, count);
}

UChar* u_memchr32(const UChar* s, UChar32 c, int32_t count) {
  UChar* (*ptr)(const UChar*, UChar32, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar32, int32_t))syms[404];
  return ptr(s, c, count);
}

UChar* u_memrchr(const UChar* s, UChar c, int32_t count) {
  UChar* (*ptr)(const UChar*, UChar, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar, int32_t))syms[405];
  return ptr(s, c, count);
}

UChar* u_memrchr32(const UChar* s, UChar32 c, int32_t count) {
  UChar* (*ptr)(const UChar*, UChar32, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar32, int32_t))syms[406];
  return ptr(s, c, count);
}

int32_t u_unescape(const char* src, UChar* dest, int32_t destCapacity) {
  int32_t (*ptr)(const char*, UChar*, int32_t);
  ptr = (int32_t(*)(const char*, UChar*, int32_t))syms[407];
  return ptr(src, dest, destCapacity);
}

UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t* offset, int32_t length, void* context) {
  UChar32 (*ptr)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*);
  ptr = (UChar32(*)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*))syms[408];
  return ptr(charAt, offset, length, context);
}

int32_t u_strToUpper(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))syms[409];
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToLower(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))syms[410];
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToTitle(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UBreakIterator* titleIter, const char* locale, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*))syms[411];
  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}

int32_t u_strFoldCase(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, uint32_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[412];
  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}

wchar_t* u_strToWCS(wchar_t* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  wchar_t* (*ptr)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  ptr = (wchar_t*(*)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[413];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromWCS(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const wchar_t* src, int32_t srcLength, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*))syms[414];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char* u_strToUTF8(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  char* (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  ptr = (char*(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[415];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromUTF8(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))syms[416];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char* u_strToUTF8WithSub(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  char* (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  ptr = (char*(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))syms[417];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF8WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*))syms[418];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF8Lenient(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))syms[419];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32* u_strToUTF32(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  UChar32* (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  ptr = (UChar32*(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[420];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromUTF32(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*))syms[421];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32* u_strToUTF32WithSub(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  UChar32* (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  ptr = (UChar32*(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))syms[422];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF32WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*);
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*))syms[423];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

/* unicode/uchar.h */
UBool u_hasBinaryProperty(UChar32 c, UProperty which) {
  UBool (*ptr)(UChar32, UProperty);
  ptr = (UBool(*)(UChar32, UProperty))syms[424];
  return ptr(c, which);
}

UBool u_isUAlphabetic(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[425];
  return ptr(c);
}

UBool u_isULowercase(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[426];
  return ptr(c);
}

UBool u_isUUppercase(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[427];
  return ptr(c);
}

UBool u_isUWhiteSpace(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[428];
  return ptr(c);
}

int32_t u_getIntPropertyValue(UChar32 c, UProperty which) {
  int32_t (*ptr)(UChar32, UProperty);
  ptr = (int32_t(*)(UChar32, UProperty))syms[429];
  return ptr(c, which);
}

int32_t u_getIntPropertyMinValue(UProperty which) {
  int32_t (*ptr)(UProperty);
  ptr = (int32_t(*)(UProperty))syms[430];
  return ptr(which);
}

int32_t u_getIntPropertyMaxValue(UProperty which) {
  int32_t (*ptr)(UProperty);
  ptr = (int32_t(*)(UProperty))syms[431];
  return ptr(which);
}

double u_getNumericValue(UChar32 c) {
  double (*ptr)(UChar32);
  ptr = (double(*)(UChar32))syms[432];
  return ptr(c);
}

UBool u_islower(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[433];
  return ptr(c);
}

UBool u_isupper(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[434];
  return ptr(c);
}

UBool u_istitle(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[435];
  return ptr(c);
}

UBool u_isdigit(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[436];
  return ptr(c);
}

UBool u_isalpha(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[437];
  return ptr(c);
}

UBool u_isalnum(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[438];
  return ptr(c);
}

UBool u_isxdigit(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[439];
  return ptr(c);
}

UBool u_ispunct(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[440];
  return ptr(c);
}

UBool u_isgraph(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[441];
  return ptr(c);
}

UBool u_isblank(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[442];
  return ptr(c);
}

UBool u_isdefined(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[443];
  return ptr(c);
}

UBool u_isspace(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[444];
  return ptr(c);
}

UBool u_isJavaSpaceChar(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[445];
  return ptr(c);
}

UBool u_isWhitespace(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[446];
  return ptr(c);
}

UBool u_iscntrl(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[447];
  return ptr(c);
}

UBool u_isISOControl(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[448];
  return ptr(c);
}

UBool u_isprint(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[449];
  return ptr(c);
}

UBool u_isbase(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[450];
  return ptr(c);
}

UCharDirection u_charDirection(UChar32 c) {
  UCharDirection (*ptr)(UChar32);
  ptr = (UCharDirection(*)(UChar32))syms[451];
  return ptr(c);
}

UBool u_isMirrored(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[452];
  return ptr(c);
}

UChar32 u_charMirror(UChar32 c) {
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[453];
  return ptr(c);
}

int8_t u_charType(UChar32 c) {
  int8_t (*ptr)(UChar32);
  ptr = (int8_t(*)(UChar32))syms[454];
  return ptr(c);
}

void u_enumCharTypes(UCharEnumTypeRange* enumRange, const void* context) {
  void (*ptr)(UCharEnumTypeRange*, const void*);
  ptr = (void(*)(UCharEnumTypeRange*, const void*))syms[455];
  ptr(enumRange, context);
  return;
}

uint8_t u_getCombiningClass(UChar32 c) {
  uint8_t (*ptr)(UChar32);
  ptr = (uint8_t(*)(UChar32))syms[456];
  return ptr(c);
}

int32_t u_charDigitValue(UChar32 c) {
  int32_t (*ptr)(UChar32);
  ptr = (int32_t(*)(UChar32))syms[457];
  return ptr(c);
}

UBlockCode ublock_getCode(UChar32 c) {
  UBlockCode (*ptr)(UChar32);
  ptr = (UBlockCode(*)(UChar32))syms[458];
  return ptr(c);
}

int32_t u_charName(UChar32 code, UCharNameChoice nameChoice, char* buffer, int32_t bufferLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*))syms[459];
  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}

UChar32 u_charFromName(UCharNameChoice nameChoice, const char* name, UErrorCode* pErrorCode) {
  UChar32 (*ptr)(UCharNameChoice, const char*, UErrorCode*);
  ptr = (UChar32(*)(UCharNameChoice, const char*, UErrorCode*))syms[460];
  return ptr(nameChoice, name, pErrorCode);
}

void u_enumCharNames(UChar32 start, UChar32 limit, UEnumCharNamesFn* fn, void* context, UCharNameChoice nameChoice, UErrorCode* pErrorCode) {
  void (*ptr)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*);
  ptr = (void(*)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*))syms[461];
  ptr(start, limit, fn, context, nameChoice, pErrorCode);
  return;
}

const char* u_getPropertyName(UProperty property, UPropertyNameChoice nameChoice) {
  const char* (*ptr)(UProperty, UPropertyNameChoice);
  ptr = (const char*(*)(UProperty, UPropertyNameChoice))syms[462];
  return ptr(property, nameChoice);
}

UProperty u_getPropertyEnum(const char* alias) {
  UProperty (*ptr)(const char*);
  ptr = (UProperty(*)(const char*))syms[463];
  return ptr(alias);
}

const char* u_getPropertyValueName(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  const char* (*ptr)(UProperty, int32_t, UPropertyNameChoice);
  ptr = (const char*(*)(UProperty, int32_t, UPropertyNameChoice))syms[464];
  return ptr(property, value, nameChoice);
}

int32_t u_getPropertyValueEnum(UProperty property, const char* alias) {
  int32_t (*ptr)(UProperty, const char*);
  ptr = (int32_t(*)(UProperty, const char*))syms[465];
  return ptr(property, alias);
}

UBool u_isIDStart(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[466];
  return ptr(c);
}

UBool u_isIDPart(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[467];
  return ptr(c);
}

UBool u_isIDIgnorable(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[468];
  return ptr(c);
}

UBool u_isJavaIDStart(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[469];
  return ptr(c);
}

UBool u_isJavaIDPart(UChar32 c) {
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[470];
  return ptr(c);
}

UChar32 u_tolower(UChar32 c) {
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[471];
  return ptr(c);
}

UChar32 u_toupper(UChar32 c) {
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[472];
  return ptr(c);
}

UChar32 u_totitle(UChar32 c) {
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[473];
  return ptr(c);
}

UChar32 u_foldCase(UChar32 c, uint32_t options) {
  UChar32 (*ptr)(UChar32, uint32_t);
  ptr = (UChar32(*)(UChar32, uint32_t))syms[474];
  return ptr(c, options);
}

int32_t u_digit(UChar32 ch, int8_t radix) {
  int32_t (*ptr)(UChar32, int8_t);
  ptr = (int32_t(*)(UChar32, int8_t))syms[475];
  return ptr(ch, radix);
}

UChar32 u_forDigit(int32_t digit, int8_t radix) {
  UChar32 (*ptr)(int32_t, int8_t);
  ptr = (UChar32(*)(int32_t, int8_t))syms[476];
  return ptr(digit, radix);
}

void u_charAge(UChar32 c, UVersionInfo versionArray) {
  void (*ptr)(UChar32, UVersionInfo);
  ptr = (void(*)(UChar32, UVersionInfo))syms[477];
  ptr(c, versionArray);
  return;
}

void u_getUnicodeVersion(UVersionInfo versionArray) {
  void (*ptr)(UVersionInfo);
  ptr = (void(*)(UVersionInfo))syms[478];
  ptr(versionArray);
  return;
}

int32_t u_getFC_NFKC_Closure(UChar32 c, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UChar32, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UChar32, UChar*, int32_t, UErrorCode*))syms[479];
  return ptr(c, dest, destCapacity, pErrorCode);
}

/* unicode/uiter.h */
UChar32 uiter_current32(UCharIterator* iter) {
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[480];
  return ptr(iter);
}

UChar32 uiter_next32(UCharIterator* iter) {
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[481];
  return ptr(iter);
}

UChar32 uiter_previous32(UCharIterator* iter) {
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[482];
  return ptr(iter);
}

uint32_t uiter_getState(const UCharIterator* iter) {
  uint32_t (*ptr)(const UCharIterator*);
  ptr = (uint32_t(*)(const UCharIterator*))syms[483];
  return ptr(iter);
}

void uiter_setState(UCharIterator* iter, uint32_t state, UErrorCode* pErrorCode) {
  void (*ptr)(UCharIterator*, uint32_t, UErrorCode*);
  ptr = (void(*)(UCharIterator*, uint32_t, UErrorCode*))syms[484];
  ptr(iter, state, pErrorCode);
  return;
}

void uiter_setString(UCharIterator* iter, const UChar* s, int32_t length) {
  void (*ptr)(UCharIterator*, const UChar*, int32_t);
  ptr = (void(*)(UCharIterator*, const UChar*, int32_t))syms[485];
  ptr(iter, s, length);
  return;
}

void uiter_setUTF16BE(UCharIterator* iter, const char* s, int32_t length) {
  void (*ptr)(UCharIterator*, const char*, int32_t);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))syms[486];
  ptr(iter, s, length);
  return;
}

void uiter_setUTF8(UCharIterator* iter, const char* s, int32_t length) {
  void (*ptr)(UCharIterator*, const char*, int32_t);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))syms[487];
  ptr(iter, s, length);
  return;
}

/* unicode/utypes.h */
const char* u_errorName(UErrorCode code) {
  const char* (*ptr)(UErrorCode);
  ptr = (const char*(*)(UErrorCode))syms[488];
  return ptr(code);
}

/* unicode/udata.h */
UDataMemory* udata_open(const char* path, const char* type, const char* name, UErrorCode* pErrorCode) {
  UDataMemory* (*ptr)(const char*, const char*, const char*, UErrorCode*);
  ptr = (UDataMemory*(*)(const char*, const char*, const char*, UErrorCode*))syms[489];
  return ptr(path, type, name, pErrorCode);
}

UDataMemory* udata_openChoice(const char* path, const char* type, const char* name, UDataMemoryIsAcceptable* isAcceptable, void* context, UErrorCode* pErrorCode) {
  UDataMemory* (*ptr)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*);
  ptr = (UDataMemory*(*)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*))syms[490];
  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}

void udata_close(UDataMemory* pData) {
  void (*ptr)(UDataMemory*);
  ptr = (void(*)(UDataMemory*))syms[491];
  ptr(pData);
  return;
}

const void* udata_getMemory(UDataMemory* pData) {
  const void* (*ptr)(UDataMemory*);
  ptr = (const void*(*)(UDataMemory*))syms[492];
  return ptr(pData);
}

void udata_getInfo(UDataMemory* pData, UDataInfo* pInfo) {
  void (*ptr)(UDataMemory*, UDataInfo*);
  ptr = (void(*)(UDataMemory*, UDataInfo*))syms[493];
  ptr(pData, pInfo);
  return;
}

void udata_setCommonData(const void* data, UErrorCode* err) {
  void (*ptr)(const void*, UErrorCode*);
  ptr = (void(*)(const void*, UErrorCode*))syms[494];
  ptr(data, err);
  return;
}

void udata_setAppData(const char* packageName, const void* data, UErrorCode* err) {
  void (*ptr)(const char*, const void*, UErrorCode*);
  ptr = (void(*)(const char*, const void*, UErrorCode*))syms[495];
  ptr(packageName, data, err);
  return;
}

void udata_setFileAccess(UDataFileAccess access, UErrorCode* status) {
  void (*ptr)(UDataFileAccess, UErrorCode*);
  ptr = (void(*)(UDataFileAccess, UErrorCode*))syms[496];
  ptr(access, status);
  return;
}

/* unicode/ucasemap.h */
UCaseMap* ucasemap_open(const char* locale, uint32_t options, UErrorCode* pErrorCode) {
  UCaseMap* (*ptr)(const char*, uint32_t, UErrorCode*);
  ptr = (UCaseMap*(*)(const char*, uint32_t, UErrorCode*))syms[497];
  return ptr(locale, options, pErrorCode);
}

void ucasemap_close(UCaseMap* csm) {
  void (*ptr)(UCaseMap*);
  ptr = (void(*)(UCaseMap*))syms[498];
  ptr(csm);
  return;
}

const char* ucasemap_getLocale(const UCaseMap* csm) {
  const char* (*ptr)(const UCaseMap*);
  ptr = (const char*(*)(const UCaseMap*))syms[499];
  return ptr(csm);
}

uint32_t ucasemap_getOptions(const UCaseMap* csm) {
  uint32_t (*ptr)(const UCaseMap*);
  ptr = (uint32_t(*)(const UCaseMap*))syms[500];
  return ptr(csm);
}

void ucasemap_setLocale(UCaseMap* csm, const char* locale, UErrorCode* pErrorCode) {
  void (*ptr)(UCaseMap*, const char*, UErrorCode*);
  ptr = (void(*)(UCaseMap*, const char*, UErrorCode*))syms[501];
  ptr(csm, locale, pErrorCode);
  return;
}

void ucasemap_setOptions(UCaseMap* csm, uint32_t options, UErrorCode* pErrorCode) {
  void (*ptr)(UCaseMap*, uint32_t, UErrorCode*);
  ptr = (void(*)(UCaseMap*, uint32_t, UErrorCode*))syms[502];
  ptr(csm, options, pErrorCode);
  return;
}

const UBreakIterator* ucasemap_getBreakIterator(const UCaseMap* csm) {
  const UBreakIterator* (*ptr)(const UCaseMap*);
  ptr = (const UBreakIterator*(*)(const UCaseMap*))syms[503];
  return ptr(csm);
}

void ucasemap_setBreakIterator(UCaseMap* csm, UBreakIterator* iterToAdopt, UErrorCode* pErrorCode) {
  void (*ptr)(UCaseMap*, UBreakIterator*, UErrorCode*);
  ptr = (void(*)(UCaseMap*, UBreakIterator*, UErrorCode*))syms[504];
  ptr(csm, iterToAdopt, pErrorCode);
  return;
}

int32_t ucasemap_toTitle(UCaseMap* csm, UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[505];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToLower(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[506];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToUpper(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[507];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToTitle(UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[508];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8FoldCase(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[509];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

/* unicode/ucnv_cb.h */
void ucnv_cbFromUWriteBytes(UConverterFromUnicodeArgs* args, const char* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  void (*ptr)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*);
  ptr = (void(*)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*))syms[510];
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteSub(UConverterFromUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  void (*ptr)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*);
  ptr = (void(*)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*))syms[511];
  ptr(args, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs* args, const UChar** source, const UChar* sourceLimit, int32_t offsetIndex, UErrorCode* err) {
  void (*ptr)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*))syms[512];
  ptr(args, source, sourceLimit, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteUChars(UConverterToUnicodeArgs* args, const UChar* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  void (*ptr)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*);
  ptr = (void(*)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*))syms[513];
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteSub(UConverterToUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  void (*ptr)(UConverterToUnicodeArgs*, int32_t, UErrorCode*);
  ptr = (void(*)(UConverterToUnicodeArgs*, int32_t, UErrorCode*))syms[514];
  ptr(args, offsetIndex, err);
  return;
}

/* unicode/uset.h */
USet* uset_openEmpty() {
  USet* (*ptr)(void);
  ptr = (USet*(*)(void))syms[515];
  return ptr();
}

USet* uset_open(UChar32 start, UChar32 end) {
  USet* (*ptr)(UChar32, UChar32);
  ptr = (USet*(*)(UChar32, UChar32))syms[516];
  return ptr(start, end);
}

USet* uset_openPattern(const UChar* pattern, int32_t patternLength, UErrorCode* ec) {
  USet* (*ptr)(const UChar*, int32_t, UErrorCode*);
  ptr = (USet*(*)(const UChar*, int32_t, UErrorCode*))syms[517];
  return ptr(pattern, patternLength, ec);
}

USet* uset_openPatternOptions(const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* ec) {
  USet* (*ptr)(const UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (USet*(*)(const UChar*, int32_t, uint32_t, UErrorCode*))syms[518];
  return ptr(pattern, patternLength, options, ec);
}

void uset_close(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[519];
  ptr(set);
  return;
}

USet* uset_clone(const USet* set) {
  USet* (*ptr)(const USet*);
  ptr = (USet*(*)(const USet*))syms[520];
  return ptr(set);
}

UBool uset_isFrozen(const USet* set) {
  UBool (*ptr)(const USet*);
  ptr = (UBool(*)(const USet*))syms[521];
  return ptr(set);
}

void uset_freeze(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[522];
  ptr(set);
  return;
}

USet* uset_cloneAsThawed(const USet* set) {
  USet* (*ptr)(const USet*);
  ptr = (USet*(*)(const USet*))syms[523];
  return ptr(set);
}

void uset_set(USet* set, UChar32 start, UChar32 end) {
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[524];
  ptr(set, start, end);
  return;
}

int32_t uset_applyPattern(USet* set, const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* status) {
  int32_t (*ptr)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (int32_t(*)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*))syms[525];
  return ptr(set, pattern, patternLength, options, status);
}

void uset_applyIntPropertyValue(USet* set, UProperty prop, int32_t value, UErrorCode* ec) {
  void (*ptr)(USet*, UProperty, int32_t, UErrorCode*);
  ptr = (void(*)(USet*, UProperty, int32_t, UErrorCode*))syms[526];
  ptr(set, prop, value, ec);
  return;
}

void uset_applyPropertyAlias(USet* set, const UChar* prop, int32_t propLength, const UChar* value, int32_t valueLength, UErrorCode* ec) {
  void (*ptr)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[527];
  ptr(set, prop, propLength, value, valueLength, ec);
  return;
}

UBool uset_resemblesPattern(const UChar* pattern, int32_t patternLength, int32_t pos) {
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))syms[528];
  return ptr(pattern, patternLength, pos);
}

int32_t uset_toPattern(const USet* set, UChar* result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode* ec) {
  int32_t (*ptr)(const USet*, UChar*, int32_t, UBool, UErrorCode*);
  ptr = (int32_t(*)(const USet*, UChar*, int32_t, UBool, UErrorCode*))syms[529];
  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}

void uset_add(USet* set, UChar32 c) {
  void (*ptr)(USet*, UChar32);
  ptr = (void(*)(USet*, UChar32))syms[530];
  ptr(set, c);
  return;
}

void uset_addAll(USet* set, const USet* additionalSet) {
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[531];
  ptr(set, additionalSet);
  return;
}

void uset_addRange(USet* set, UChar32 start, UChar32 end) {
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[532];
  ptr(set, start, end);
  return;
}

void uset_addString(USet* set, const UChar* str, int32_t strLen) {
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[533];
  ptr(set, str, strLen);
  return;
}

void uset_addAllCodePoints(USet* set, const UChar* str, int32_t strLen) {
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[534];
  ptr(set, str, strLen);
  return;
}

void uset_remove(USet* set, UChar32 c) {
  void (*ptr)(USet*, UChar32);
  ptr = (void(*)(USet*, UChar32))syms[535];
  ptr(set, c);
  return;
}

void uset_removeRange(USet* set, UChar32 start, UChar32 end) {
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[536];
  ptr(set, start, end);
  return;
}

void uset_removeString(USet* set, const UChar* str, int32_t strLen) {
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[537];
  ptr(set, str, strLen);
  return;
}

void uset_removeAll(USet* set, const USet* removeSet) {
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[538];
  ptr(set, removeSet);
  return;
}

void uset_retain(USet* set, UChar32 start, UChar32 end) {
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[539];
  ptr(set, start, end);
  return;
}

void uset_retainAll(USet* set, const USet* retain) {
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[540];
  ptr(set, retain);
  return;
}

void uset_compact(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[541];
  ptr(set);
  return;
}

void uset_complement(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[542];
  ptr(set);
  return;
}

void uset_complementAll(USet* set, const USet* complement) {
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[543];
  ptr(set, complement);
  return;
}

void uset_clear(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[544];
  ptr(set);
  return;
}

void uset_closeOver(USet* set, int32_t attributes) {
  void (*ptr)(USet*, int32_t);
  ptr = (void(*)(USet*, int32_t))syms[545];
  ptr(set, attributes);
  return;
}

void uset_removeAllStrings(USet* set) {
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[546];
  ptr(set);
  return;
}

UBool uset_isEmpty(const USet* set) {
  UBool (*ptr)(const USet*);
  ptr = (UBool(*)(const USet*))syms[547];
  return ptr(set);
}

UBool uset_contains(const USet* set, UChar32 c) {
  UBool (*ptr)(const USet*, UChar32);
  ptr = (UBool(*)(const USet*, UChar32))syms[548];
  return ptr(set, c);
}

UBool uset_containsRange(const USet* set, UChar32 start, UChar32 end) {
  UBool (*ptr)(const USet*, UChar32, UChar32);
  ptr = (UBool(*)(const USet*, UChar32, UChar32))syms[549];
  return ptr(set, start, end);
}

UBool uset_containsString(const USet* set, const UChar* str, int32_t strLen) {
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))syms[550];
  return ptr(set, str, strLen);
}

int32_t uset_indexOf(const USet* set, UChar32 c) {
  int32_t (*ptr)(const USet*, UChar32);
  ptr = (int32_t(*)(const USet*, UChar32))syms[551];
  return ptr(set, c);
}

UChar32 uset_charAt(const USet* set, int32_t charIndex) {
  UChar32 (*ptr)(const USet*, int32_t);
  ptr = (UChar32(*)(const USet*, int32_t))syms[552];
  return ptr(set, charIndex);
}

int32_t uset_size(const USet* set) {
  int32_t (*ptr)(const USet*);
  ptr = (int32_t(*)(const USet*))syms[553];
  return ptr(set);
}

int32_t uset_getItemCount(const USet* set) {
  int32_t (*ptr)(const USet*);
  ptr = (int32_t(*)(const USet*))syms[554];
  return ptr(set);
}

int32_t uset_getItem(const USet* set, int32_t itemIndex, UChar32* start, UChar32* end, UChar* str, int32_t strCapacity, UErrorCode* ec) {
  int32_t (*ptr)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*))syms[555];
  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}

UBool uset_containsAll(const USet* set1, const USet* set2) {
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[556];
  return ptr(set1, set2);
}

UBool uset_containsAllCodePoints(const USet* set, const UChar* str, int32_t strLen) {
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))syms[557];
  return ptr(set, str, strLen);
}

UBool uset_containsNone(const USet* set1, const USet* set2) {
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[558];
  return ptr(set1, set2);
}

UBool uset_containsSome(const USet* set1, const USet* set2) {
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[559];
  return ptr(set1, set2);
}

int32_t uset_span(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))syms[560];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBack(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))syms[561];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))syms[562];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBackUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))syms[563];
  return ptr(set, s, length, spanCondition);
}

UBool uset_equals(const USet* set1, const USet* set2) {
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[564];
  return ptr(set1, set2);
}

int32_t uset_serialize(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const USet*, uint16_t*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const USet*, uint16_t*, int32_t, UErrorCode*))syms[565];
  return ptr(set, dest, destCapacity, pErrorCode);
}

UBool uset_getSerializedSet(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength) {
  UBool (*ptr)(USerializedSet*, const uint16_t*, int32_t);
  ptr = (UBool(*)(USerializedSet*, const uint16_t*, int32_t))syms[566];
  return ptr(fillSet, src, srcLength);
}

void uset_setSerializedToOne(USerializedSet* fillSet, UChar32 c) {
  void (*ptr)(USerializedSet*, UChar32);
  ptr = (void(*)(USerializedSet*, UChar32))syms[567];
  ptr(fillSet, c);
  return;
}

UBool uset_serializedContains(const USerializedSet* set, UChar32 c) {
  UBool (*ptr)(const USerializedSet*, UChar32);
  ptr = (UBool(*)(const USerializedSet*, UChar32))syms[568];
  return ptr(set, c);
}

int32_t uset_getSerializedRangeCount(const USerializedSet* set) {
  int32_t (*ptr)(const USerializedSet*);
  ptr = (int32_t(*)(const USerializedSet*))syms[569];
  return ptr(set);
}

UBool uset_getSerializedRange(const USerializedSet* set, int32_t rangeIndex, UChar32* pStart, UChar32* pEnd) {
  UBool (*ptr)(const USerializedSet*, int32_t, UChar32*, UChar32*);
  ptr = (UBool(*)(const USerializedSet*, int32_t, UChar32*, UChar32*))syms[570];
  return ptr(set, rangeIndex, pStart, pEnd);
}

/* unicode/utext.h */
UText* utext_close(UText* ut) {
  UText* (*ptr)(UText*);
  ptr = (UText*(*)(UText*))syms[571];
  return ptr(ut);
}

UText* utext_openUTF8(UText* ut, const char* s, int64_t length, UErrorCode* status) {
  UText* (*ptr)(UText*, const char*, int64_t, UErrorCode*);
  ptr = (UText*(*)(UText*, const char*, int64_t, UErrorCode*))syms[572];
  return ptr(ut, s, length, status);
}

UText* utext_openUChars(UText* ut, const UChar* s, int64_t length, UErrorCode* status) {
  UText* (*ptr)(UText*, const UChar*, int64_t, UErrorCode*);
  ptr = (UText*(*)(UText*, const UChar*, int64_t, UErrorCode*))syms[573];
  return ptr(ut, s, length, status);
}

UText* utext_clone(UText* dest, const UText* src, UBool deep, UBool readOnly, UErrorCode* status) {
  UText* (*ptr)(UText*, const UText*, UBool, UBool, UErrorCode*);
  ptr = (UText*(*)(UText*, const UText*, UBool, UBool, UErrorCode*))syms[574];
  return ptr(dest, src, deep, readOnly, status);
}

UBool utext_equals(const UText* a, const UText* b) {
  UBool (*ptr)(const UText*, const UText*);
  ptr = (UBool(*)(const UText*, const UText*))syms[575];
  return ptr(a, b);
}

int64_t utext_nativeLength(UText* ut) {
  int64_t (*ptr)(UText*);
  ptr = (int64_t(*)(UText*))syms[576];
  return ptr(ut);
}

UBool utext_isLengthExpensive(const UText* ut) {
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[577];
  return ptr(ut);
}

UChar32 utext_char32At(UText* ut, int64_t nativeIndex) {
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[578];
  return ptr(ut, nativeIndex);
}

UChar32 utext_current32(UText* ut) {
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[579];
  return ptr(ut);
}

UChar32 utext_next32(UText* ut) {
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[580];
  return ptr(ut);
}

UChar32 utext_previous32(UText* ut) {
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[581];
  return ptr(ut);
}

UChar32 utext_next32From(UText* ut, int64_t nativeIndex) {
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[582];
  return ptr(ut, nativeIndex);
}

UChar32 utext_previous32From(UText* ut, int64_t nativeIndex) {
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[583];
  return ptr(ut, nativeIndex);
}

int64_t utext_getNativeIndex(const UText* ut) {
  int64_t (*ptr)(const UText*);
  ptr = (int64_t(*)(const UText*))syms[584];
  return ptr(ut);
}

void utext_setNativeIndex(UText* ut, int64_t nativeIndex) {
  void (*ptr)(UText*, int64_t);
  ptr = (void(*)(UText*, int64_t))syms[585];
  ptr(ut, nativeIndex);
  return;
}

UBool utext_moveIndex32(UText* ut, int32_t delta) {
  UBool (*ptr)(UText*, int32_t);
  ptr = (UBool(*)(UText*, int32_t))syms[586];
  return ptr(ut, delta);
}

int64_t utext_getPreviousNativeIndex(UText* ut) {
  int64_t (*ptr)(UText*);
  ptr = (int64_t(*)(UText*))syms[587];
  return ptr(ut);
}

int32_t utext_extract(UText* ut, int64_t nativeStart, int64_t nativeLimit, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*))syms[588];
  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}

UBool utext_isWritable(const UText* ut) {
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[589];
  return ptr(ut);
}

UBool utext_hasMetaData(const UText* ut) {
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[590];
  return ptr(ut);
}

int32_t utext_replace(UText* ut, int64_t nativeStart, int64_t nativeLimit, const UChar* replacementText, int32_t replacementLength, UErrorCode* status) {
  int32_t (*ptr)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*))syms[591];
  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}

void utext_copy(UText* ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode* status) {
  void (*ptr)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*);
  ptr = (void(*)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*))syms[592];
  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
  return;
}

void utext_freeze(UText* ut) {
  void (*ptr)(UText*);
  ptr = (void(*)(UText*))syms[593];
  ptr(ut);
  return;
}

UText* utext_setup(UText* ut, int32_t extraSpace, UErrorCode* status) {
  UText* (*ptr)(UText*, int32_t, UErrorCode*);
  ptr = (UText*(*)(UText*, int32_t, UErrorCode*))syms[594];
  return ptr(ut, extraSpace, status);
}

/* unicode/ures.h */
UResourceBundle* ures_open(const char* packageName, const char* locale, UErrorCode* status) {
  UResourceBundle* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (UResourceBundle*(*)(const char*, const char*, UErrorCode*))syms[595];
  return ptr(packageName, locale, status);
}

UResourceBundle* ures_openDirect(const char* packageName, const char* locale, UErrorCode* status) {
  UResourceBundle* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (UResourceBundle*(*)(const char*, const char*, UErrorCode*))syms[596];
  return ptr(packageName, locale, status);
}

UResourceBundle* ures_openU(const UChar* packageName, const char* locale, UErrorCode* status) {
  UResourceBundle* (*ptr)(const UChar*, const char*, UErrorCode*);
  ptr = (UResourceBundle*(*)(const UChar*, const char*, UErrorCode*))syms[597];
  return ptr(packageName, locale, status);
}

void ures_close(UResourceBundle* resourceBundle) {
  void (*ptr)(UResourceBundle*);
  ptr = (void(*)(UResourceBundle*))syms[598];
  ptr(resourceBundle);
  return;
}

void ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
  void (*ptr)(const UResourceBundle*, UVersionInfo);
  ptr = (void(*)(const UResourceBundle*, UVersionInfo))syms[599];
  ptr(resB, versionInfo);
  return;
}

const char* ures_getLocaleByType(const UResourceBundle* resourceBundle, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*))syms[600];
  return ptr(resourceBundle, type, status);
}

const UChar* ures_getString(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  const UChar* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[601];
  return ptr(resourceBundle, len, status);
}

const char* ures_getUTF8String(const UResourceBundle* resB, char* dest, int32_t* length, UBool forceCopy, UErrorCode* status) {
  const char* (*ptr)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*);
  ptr = (const char*(*)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*))syms[602];
  return ptr(resB, dest, length, forceCopy, status);
}

const uint8_t* ures_getBinary(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  const uint8_t* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  ptr = (const uint8_t*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[603];
  return ptr(resourceBundle, len, status);
}

const int32_t* ures_getIntVector(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  const int32_t* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  ptr = (const int32_t*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[604];
  return ptr(resourceBundle, len, status);
}

uint32_t ures_getUInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  uint32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  ptr = (uint32_t(*)(const UResourceBundle*, UErrorCode*))syms[605];
  return ptr(resourceBundle, status);
}

int32_t ures_getInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  int32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  ptr = (int32_t(*)(const UResourceBundle*, UErrorCode*))syms[606];
  return ptr(resourceBundle, status);
}

int32_t ures_getSize(const UResourceBundle* resourceBundle) {
  int32_t (*ptr)(const UResourceBundle*);
  ptr = (int32_t(*)(const UResourceBundle*))syms[607];
  return ptr(resourceBundle);
}

UResType ures_getType(const UResourceBundle* resourceBundle) {
  UResType (*ptr)(const UResourceBundle*);
  ptr = (UResType(*)(const UResourceBundle*))syms[608];
  return ptr(resourceBundle);
}

const char* ures_getKey(const UResourceBundle* resourceBundle) {
  const char* (*ptr)(const UResourceBundle*);
  ptr = (const char*(*)(const UResourceBundle*))syms[609];
  return ptr(resourceBundle);
}

void ures_resetIterator(UResourceBundle* resourceBundle) {
  void (*ptr)(UResourceBundle*);
  ptr = (void(*)(UResourceBundle*))syms[610];
  ptr(resourceBundle);
  return;
}

UBool ures_hasNext(const UResourceBundle* resourceBundle) {
  UBool (*ptr)(const UResourceBundle*);
  ptr = (UBool(*)(const UResourceBundle*))syms[611];
  return ptr(resourceBundle);
}

UResourceBundle* ures_getNextResource(UResourceBundle* resourceBundle, UResourceBundle* fillIn, UErrorCode* status) {
  UResourceBundle* (*ptr)(UResourceBundle*, UResourceBundle*, UErrorCode*);
  ptr = (UResourceBundle*(*)(UResourceBundle*, UResourceBundle*, UErrorCode*))syms[612];
  return ptr(resourceBundle, fillIn, status);
}

const UChar* ures_getNextString(UResourceBundle* resourceBundle, int32_t* len, const char** key, UErrorCode* status) {
  const UChar* (*ptr)(UResourceBundle*, int32_t*, const char**, UErrorCode*);
  ptr = (const UChar*(*)(UResourceBundle*, int32_t*, const char**, UErrorCode*))syms[613];
  return ptr(resourceBundle, len, key, status);
}

UResourceBundle* ures_getByIndex(const UResourceBundle* resourceBundle, int32_t indexR, UResourceBundle* fillIn, UErrorCode* status) {
  UResourceBundle* (*ptr)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*);
  ptr = (UResourceBundle*(*)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*))syms[614];
  return ptr(resourceBundle, indexR, fillIn, status);
}

const UChar* ures_getStringByIndex(const UResourceBundle* resourceBundle, int32_t indexS, int32_t* len, UErrorCode* status) {
  const UChar* (*ptr)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*))syms[615];
  return ptr(resourceBundle, indexS, len, status);
}

const char* ures_getUTF8StringByIndex(const UResourceBundle* resB, int32_t stringIndex, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  const char* (*ptr)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*);
  ptr = (const char*(*)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*))syms[616];
  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}

UResourceBundle* ures_getByKey(const UResourceBundle* resourceBundle, const char* key, UResourceBundle* fillIn, UErrorCode* status) {
  UResourceBundle* (*ptr)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*);
  ptr = (UResourceBundle*(*)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*))syms[617];
  return ptr(resourceBundle, key, fillIn, status);
}

const UChar* ures_getStringByKey(const UResourceBundle* resB, const char* key, int32_t* len, UErrorCode* status) {
  const UChar* (*ptr)(const UResourceBundle*, const char*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(const UResourceBundle*, const char*, int32_t*, UErrorCode*))syms[618];
  return ptr(resB, key, len, status);
}

const char* ures_getUTF8StringByKey(const UResourceBundle* resB, const char* key, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  const char* (*ptr)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*);
  ptr = (const char*(*)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*))syms[619];
  return ptr(resB, key, dest, pLength, forceCopy, status);
}

UEnumeration* ures_openAvailableLocales(const char* packageName, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[620];
  return ptr(packageName, status);
}

/* unicode/ubidi.h */
UBiDi* ubidi_open(void) {
  UBiDi* (*ptr)(void);
  ptr = (UBiDi*(*)(void))syms[621];
  return ptr();
}

UBiDi* ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode* pErrorCode) {
  UBiDi* (*ptr)(int32_t, int32_t, UErrorCode*);
  ptr = (UBiDi*(*)(int32_t, int32_t, UErrorCode*))syms[622];
  return ptr(maxLength, maxRunCount, pErrorCode);
}

void ubidi_close(UBiDi* pBiDi) {
  void (*ptr)(UBiDi*);
  ptr = (void(*)(UBiDi*))syms[623];
  ptr(pBiDi);
  return;
}

void ubidi_setInverse(UBiDi* pBiDi, UBool isInverse) {
  void (*ptr)(UBiDi*, UBool);
  ptr = (void(*)(UBiDi*, UBool))syms[624];
  ptr(pBiDi, isInverse);
  return;
}

UBool ubidi_isInverse(UBiDi* pBiDi) {
  UBool (*ptr)(UBiDi*);
  ptr = (UBool(*)(UBiDi*))syms[625];
  return ptr(pBiDi);
}

void ubidi_orderParagraphsLTR(UBiDi* pBiDi, UBool orderParagraphsLTR) {
  void (*ptr)(UBiDi*, UBool);
  ptr = (void(*)(UBiDi*, UBool))syms[626];
  ptr(pBiDi, orderParagraphsLTR);
  return;
}

UBool ubidi_isOrderParagraphsLTR(UBiDi* pBiDi) {
  UBool (*ptr)(UBiDi*);
  ptr = (UBool(*)(UBiDi*))syms[627];
  return ptr(pBiDi);
}

void ubidi_setReorderingMode(UBiDi* pBiDi, UBiDiReorderingMode reorderingMode) {
  void (*ptr)(UBiDi*, UBiDiReorderingMode);
  ptr = (void(*)(UBiDi*, UBiDiReorderingMode))syms[628];
  ptr(pBiDi, reorderingMode);
  return;
}

UBiDiReorderingMode ubidi_getReorderingMode(UBiDi* pBiDi) {
  UBiDiReorderingMode (*ptr)(UBiDi*);
  ptr = (UBiDiReorderingMode(*)(UBiDi*))syms[629];
  return ptr(pBiDi);
}

void ubidi_setReorderingOptions(UBiDi* pBiDi, uint32_t reorderingOptions) {
  void (*ptr)(UBiDi*, uint32_t);
  ptr = (void(*)(UBiDi*, uint32_t))syms[630];
  ptr(pBiDi, reorderingOptions);
  return;
}

uint32_t ubidi_getReorderingOptions(UBiDi* pBiDi) {
  uint32_t (*ptr)(UBiDi*);
  ptr = (uint32_t(*)(UBiDi*))syms[631];
  return ptr(pBiDi);
}

void ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode) {
  void (*ptr)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*);
  ptr = (void(*)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*))syms[632];
  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
  return;
}

void ubidi_setLine(const UBiDi* pParaBiDi, int32_t start, int32_t limit, UBiDi* pLineBiDi, UErrorCode* pErrorCode) {
  void (*ptr)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*))syms[633];
  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
  return;
}

UBiDiDirection ubidi_getDirection(const UBiDi* pBiDi) {
  UBiDiDirection (*ptr)(const UBiDi*);
  ptr = (UBiDiDirection(*)(const UBiDi*))syms[634];
  return ptr(pBiDi);
}

const UChar* ubidi_getText(const UBiDi* pBiDi) {
  const UChar* (*ptr)(const UBiDi*);
  ptr = (const UChar*(*)(const UBiDi*))syms[635];
  return ptr(pBiDi);
}

int32_t ubidi_getLength(const UBiDi* pBiDi) {
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[636];
  return ptr(pBiDi);
}

UBiDiLevel ubidi_getParaLevel(const UBiDi* pBiDi) {
  UBiDiLevel (*ptr)(const UBiDi*);
  ptr = (UBiDiLevel(*)(const UBiDi*))syms[637];
  return ptr(pBiDi);
}

int32_t ubidi_countParagraphs(UBiDi* pBiDi) {
  int32_t (*ptr)(UBiDi*);
  ptr = (int32_t(*)(UBiDi*))syms[638];
  return ptr(pBiDi);
}

int32_t ubidi_getParagraph(const UBiDi* pBiDi, int32_t charIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  ptr = (int32_t(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))syms[639];
  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}

void ubidi_getParagraphByIndex(const UBiDi* pBiDi, int32_t paraIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  void (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))syms[640];
  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
  return;
}

UBiDiLevel ubidi_getLevelAt(const UBiDi* pBiDi, int32_t charIndex) {
  UBiDiLevel (*ptr)(const UBiDi*, int32_t);
  ptr = (UBiDiLevel(*)(const UBiDi*, int32_t))syms[641];
  return ptr(pBiDi, charIndex);
}

const UBiDiLevel* ubidi_getLevels(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  const UBiDiLevel* (*ptr)(UBiDi*, UErrorCode*);
  ptr = (const UBiDiLevel*(*)(UBiDi*, UErrorCode*))syms[642];
  return ptr(pBiDi, pErrorCode);
}

void ubidi_getLogicalRun(const UBiDi* pBiDi, int32_t logicalPosition, int32_t* pLogicalLimit, UBiDiLevel* pLevel) {
  void (*ptr)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*))syms[643];
  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
  return;
}

int32_t ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UBiDi*, UErrorCode*);
  ptr = (int32_t(*)(UBiDi*, UErrorCode*))syms[644];
  return ptr(pBiDi, pErrorCode);
}

UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength) {
  UBiDiDirection (*ptr)(UBiDi*, int32_t, int32_t*, int32_t*);
  ptr = (UBiDiDirection(*)(UBiDi*, int32_t, int32_t*, int32_t*))syms[645];
  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}

int32_t ubidi_getVisualIndex(UBiDi* pBiDi, int32_t logicalIndex, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))syms[646];
  return ptr(pBiDi, logicalIndex, pErrorCode);
}

int32_t ubidi_getLogicalIndex(UBiDi* pBiDi, int32_t visualIndex, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))syms[647];
  return ptr(pBiDi, visualIndex, pErrorCode);
}

void ubidi_getLogicalMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))syms[648];
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_getVisualMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))syms[649];
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_reorderLogical(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))syms[650];
  ptr(levels, length, indexMap);
  return;
}

void ubidi_reorderVisual(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))syms[651];
  ptr(levels, length, indexMap);
  return;
}

void ubidi_invertMap(const int32_t* srcMap, int32_t* destMap, int32_t length) {
  void (*ptr)(const int32_t*, int32_t*, int32_t);
  ptr = (void(*)(const int32_t*, int32_t*, int32_t))syms[652];
  ptr(srcMap, destMap, length);
  return;
}

int32_t ubidi_getProcessedLength(const UBiDi* pBiDi) {
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[653];
  return ptr(pBiDi);
}

int32_t ubidi_getResultLength(const UBiDi* pBiDi) {
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[654];
  return ptr(pBiDi);
}

UCharDirection ubidi_getCustomizedClass(UBiDi* pBiDi, UChar32 c) {
  UCharDirection (*ptr)(UBiDi*, UChar32);
  ptr = (UCharDirection(*)(UBiDi*, UChar32))syms[655];
  return ptr(pBiDi, c);
}

void ubidi_setClassCallback(UBiDi* pBiDi, UBiDiClassCallback* newFn, const void* newContext, UBiDiClassCallback** oldFn, const void** oldContext, UErrorCode* pErrorCode) {
  void (*ptr)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*);
  ptr = (void(*)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*))syms[656];
  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
  return;
}

void ubidi_getClassCallback(UBiDi* pBiDi, UBiDiClassCallback** fn, const void** context) {
  void (*ptr)(UBiDi*, UBiDiClassCallback**, const void**);
  ptr = (void(*)(UBiDi*, UBiDiClassCallback**, const void**))syms[657];
  ptr(pBiDi, fn, context);
  return;
}

int32_t ubidi_writeReordered(UBiDi* pBiDi, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*);
  ptr = (int32_t(*)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*))syms[658];
  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}

int32_t ubidi_writeReverse(const UChar* src, int32_t srcLength, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*))syms[659];
  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}

/* unicode/uclean.h */
void u_init(UErrorCode* status) {
  void (*ptr)(UErrorCode*);
  ptr = (void(*)(UErrorCode*))syms[660];
  ptr(status);
  return;
}

void u_cleanup(void) {
  void (*ptr)(void);
  ptr = (void(*)(void))syms[661];
  ptr();
  return;
}

void u_setMemoryFunctions(const void* context, UMemAllocFn* a, UMemReallocFn* r, UMemFreeFn* f, UErrorCode* status) {
  void (*ptr)(const void*, UMemAllocFn*, UMemReallocFn*, UMemFreeFn*, UErrorCode*);
  ptr = (void(*)(const void*, UMemAllocFn*, UMemReallocFn*, UMemFreeFn*, UErrorCode*))syms[662];
  ptr(context, a, r, f, status);
  return;
}

/* unicode/uscript.h */
int32_t uscript_getCode(const char* nameOrAbbrOrLocale, UScriptCode* fillIn, int32_t capacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, UScriptCode*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, UScriptCode*, int32_t, UErrorCode*))syms[663];
  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}

const char* uscript_getName(UScriptCode scriptCode) {
  const char* (*ptr)(UScriptCode);
  ptr = (const char*(*)(UScriptCode))syms[664];
  return ptr(scriptCode);
}

const char* uscript_getShortName(UScriptCode scriptCode) {
  const char* (*ptr)(UScriptCode);
  ptr = (const char*(*)(UScriptCode))syms[665];
  return ptr(scriptCode);
}

UScriptCode uscript_getScript(UChar32 codepoint, UErrorCode* err) {
  UScriptCode (*ptr)(UChar32, UErrorCode*);
  ptr = (UScriptCode(*)(UChar32, UErrorCode*))syms[666];
  return ptr(codepoint, err);
}

/* unicode/ushape.h */
int32_t u_shapeArabic(const UChar* source, int32_t sourceLength, UChar* dest, int32_t destSize, uint32_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*))syms[667];
  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}

/* unicode/utrace.h */
void utrace_setLevel(int32_t traceLevel) {
  void (*ptr)(int32_t);
  ptr = (void(*)(int32_t))syms[668];
  ptr(traceLevel);
  return;
}

int32_t utrace_getLevel(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[669];
  return ptr();
}

void utrace_setFunctions(const void* context, UTraceEntry* e, UTraceExit* x, UTraceData* d) {
  void (*ptr)(const void*, UTraceEntry*, UTraceExit*, UTraceData*);
  ptr = (void(*)(const void*, UTraceEntry*, UTraceExit*, UTraceData*))syms[670];
  ptr(context, e, x, d);
  return;
}

void utrace_getFunctions(const void** context, UTraceEntry** e, UTraceExit** x, UTraceData** d) {
  void (*ptr)(const void**, UTraceEntry**, UTraceExit**, UTraceData**);
  ptr = (void(*)(const void**, UTraceEntry**, UTraceExit**, UTraceData**))syms[671];
  ptr(context, e, x, d);
  return;
}

int32_t utrace_vformat(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, va_list args) {
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, va_list);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, va_list))syms[672];
  return ptr(outBuf, capacity, indent, fmt, args);
}

int32_t utrace_format(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, ...) {
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, va_list);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, va_list))syms[673];
  va_list args;
  va_start(args, fmt);
  int32_t ret = ptr(outBuf, capacity, indent, fmt, args);
  va_end(args);
  return ret;
}

const char* utrace_functionName(int32_t fnNumber) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[674];
  return ptr(fnNumber);
}

/* unicode/putil.h */
const char* u_getDataDirectory(void) {
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[675];
  return ptr();
}

void u_setDataDirectory(const char* directory) {
  void (*ptr)(const char*);
  ptr = (void(*)(const char*))syms[676];
  ptr(directory);
  return;
}

void u_charsToUChars(const char* cs, UChar* us, int32_t length) {
  void (*ptr)(const char*, UChar*, int32_t);
  ptr = (void(*)(const char*, UChar*, int32_t))syms[677];
  ptr(cs, us, length);
  return;
}

void u_UCharsToChars(const UChar* us, char* cs, int32_t length) {
  void (*ptr)(const UChar*, char*, int32_t);
  ptr = (void(*)(const UChar*, char*, int32_t))syms[678];
  ptr(us, cs, length);
  return;
}

/* unicode/unorm.h */
int32_t unorm_normalize(const UChar* source, int32_t sourceLength, UNormalizationMode mode, int32_t options, UChar* result, int32_t resultLength, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*))syms[679];
  return ptr(source, sourceLength, mode, options, result, resultLength, status);
}

UNormalizationCheckResult unorm_quickCheck(const UChar* source, int32_t sourcelength, UNormalizationMode mode, UErrorCode* status) {
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))syms[680];
  return ptr(source, sourcelength, mode, status);
}

UNormalizationCheckResult unorm_quickCheckWithOptions(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[681];
  return ptr(src, srcLength, mode, options, pErrorCode);
}

UBool unorm_isNormalized(const UChar* src, int32_t srcLength, UNormalizationMode mode, UErrorCode* pErrorCode) {
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))syms[682];
  return ptr(src, srcLength, mode, pErrorCode);
}

UBool unorm_isNormalizedWithOptions(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[683];
  return ptr(src, srcLength, mode, options, pErrorCode);
}

int32_t unorm_next(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))syms[684];
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_previous(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))syms[685];
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_concatenate(const UChar* left, int32_t leftLength, const UChar* right, int32_t rightLength, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[686];
  return ptr(left, leftLength, right, rightLength, dest, destCapacity, mode, options, pErrorCode);
}

int32_t unorm_compare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[687];
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

/* unicode/ubrk.h */
UBreakIterator* ubrk_open(UBreakIteratorType type, const char* locale, const UChar* text, int32_t textLength, UErrorCode* status) {
  UBreakIterator* (*ptr)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*);
  ptr = (UBreakIterator*(*)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*))syms[688];
  return ptr(type, locale, text, textLength, status);
}

UBreakIterator* ubrk_openRules(const UChar* rules, int32_t rulesLength, const UChar* text, int32_t textLength, UParseError* parseErr, UErrorCode* status) {
  UBreakIterator* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*);
  ptr = (UBreakIterator*(*)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*))syms[689];
  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}

UBreakIterator* ubrk_safeClone(const UBreakIterator* bi, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  UBreakIterator* (*ptr)(const UBreakIterator*, void*, int32_t*, UErrorCode*);
  ptr = (UBreakIterator*(*)(const UBreakIterator*, void*, int32_t*, UErrorCode*))syms[690];
  return ptr(bi, stackBuffer, pBufferSize, status);
}

void ubrk_close(UBreakIterator* bi) {
  void (*ptr)(UBreakIterator*);
  ptr = (void(*)(UBreakIterator*))syms[691];
  ptr(bi);
  return;
}

void ubrk_setText(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status) {
  void (*ptr)(UBreakIterator*, const UChar*, int32_t, UErrorCode*);
  ptr = (void(*)(UBreakIterator*, const UChar*, int32_t, UErrorCode*))syms[692];
  ptr(bi, text, textLength, status);
  return;
}

void ubrk_setUText(UBreakIterator* bi, UText* text, UErrorCode* status) {
  void (*ptr)(UBreakIterator*, UText*, UErrorCode*);
  ptr = (void(*)(UBreakIterator*, UText*, UErrorCode*))syms[693];
  ptr(bi, text, status);
  return;
}

int32_t ubrk_current(const UBreakIterator* bi) {
  int32_t (*ptr)(const UBreakIterator*);
  ptr = (int32_t(*)(const UBreakIterator*))syms[694];
  return ptr(bi);
}

int32_t ubrk_next(UBreakIterator* bi) {
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[695];
  return ptr(bi);
}

int32_t ubrk_previous(UBreakIterator* bi) {
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[696];
  return ptr(bi);
}

int32_t ubrk_first(UBreakIterator* bi) {
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[697];
  return ptr(bi);
}

int32_t ubrk_last(UBreakIterator* bi) {
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[698];
  return ptr(bi);
}

int32_t ubrk_preceding(UBreakIterator* bi, int32_t offset) {
  int32_t (*ptr)(UBreakIterator*, int32_t);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))syms[699];
  return ptr(bi, offset);
}

int32_t ubrk_following(UBreakIterator* bi, int32_t offset) {
  int32_t (*ptr)(UBreakIterator*, int32_t);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))syms[700];
  return ptr(bi, offset);
}

const char* ubrk_getAvailable(int32_t index) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[701];
  return ptr(index);
}

int32_t ubrk_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[702];
  return ptr();
}

UBool ubrk_isBoundary(UBreakIterator* bi, int32_t offset) {
  UBool (*ptr)(UBreakIterator*, int32_t);
  ptr = (UBool(*)(UBreakIterator*, int32_t))syms[703];
  return ptr(bi, offset);
}

int32_t ubrk_getRuleStatus(UBreakIterator* bi) {
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[704];
  return ptr(bi);
}

int32_t ubrk_getRuleStatusVec(UBreakIterator* bi, int32_t* fillInVec, int32_t capacity, UErrorCode* status) {
  int32_t (*ptr)(UBreakIterator*, int32_t*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(UBreakIterator*, int32_t*, int32_t, UErrorCode*))syms[705];
  return ptr(bi, fillInVec, capacity, status);
}

const char* ubrk_getLocaleByType(const UBreakIterator* bi, ULocDataLocaleType type, UErrorCode* status) {
  const char* (*ptr)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*);
  ptr = (const char*(*)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*))syms[706];
  return ptr(bi, type, status);
}

/* unicode/uloc.h */
const char* uloc_getDefault(void) {
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[707];
  return ptr();
}

void uloc_setDefault(const char* localeID, UErrorCode* status) {
  void (*ptr)(const char*, UErrorCode*);
  ptr = (void(*)(const char*, UErrorCode*))syms[708];
  ptr(localeID, status);
  return;
}

int32_t uloc_getLanguage(const char* localeID, char* language, int32_t languageCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[709];
  return ptr(localeID, language, languageCapacity, err);
}

int32_t uloc_getScript(const char* localeID, char* script, int32_t scriptCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[710];
  return ptr(localeID, script, scriptCapacity, err);
}

int32_t uloc_getCountry(const char* localeID, char* country, int32_t countryCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[711];
  return ptr(localeID, country, countryCapacity, err);
}

int32_t uloc_getVariant(const char* localeID, char* variant, int32_t variantCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[712];
  return ptr(localeID, variant, variantCapacity, err);
}

int32_t uloc_getName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[713];
  return ptr(localeID, name, nameCapacity, err);
}

int32_t uloc_canonicalize(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[714];
  return ptr(localeID, name, nameCapacity, err);
}

const char* uloc_getISO3Language(const char* localeID) {
  const char* (*ptr)(const char*);
  ptr = (const char*(*)(const char*))syms[715];
  return ptr(localeID);
}

const char* uloc_getISO3Country(const char* localeID) {
  const char* (*ptr)(const char*);
  ptr = (const char*(*)(const char*))syms[716];
  return ptr(localeID);
}

uint32_t uloc_getLCID(const char* localeID) {
  uint32_t (*ptr)(const char*);
  ptr = (uint32_t(*)(const char*))syms[717];
  return ptr(localeID);
}

int32_t uloc_getDisplayLanguage(const char* locale, const char* displayLocale, UChar* language, int32_t languageCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[718];
  return ptr(locale, displayLocale, language, languageCapacity, status);
}

int32_t uloc_getDisplayScript(const char* locale, const char* displayLocale, UChar* script, int32_t scriptCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[719];
  return ptr(locale, displayLocale, script, scriptCapacity, status);
}

int32_t uloc_getDisplayCountry(const char* locale, const char* displayLocale, UChar* country, int32_t countryCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[720];
  return ptr(locale, displayLocale, country, countryCapacity, status);
}

int32_t uloc_getDisplayVariant(const char* locale, const char* displayLocale, UChar* variant, int32_t variantCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[721];
  return ptr(locale, displayLocale, variant, variantCapacity, status);
}

int32_t uloc_getDisplayKeyword(const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[722];
  return ptr(keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayKeywordValue(const char* locale, const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*))syms[723];
  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayName(const char* localeID, const char* inLocaleID, UChar* result, int32_t maxResultSize, UErrorCode* err) {
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[724];
  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}

const char* uloc_getAvailable(int32_t n) {
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[725];
  return ptr(n);
}

int32_t uloc_countAvailable(void) {
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[726];
  return ptr();
}

const char* const* uloc_getISOLanguages(void) {
  const char* const* (*ptr)(void);
  ptr = (const char* const*(*)(void))syms[727];
  return ptr();
}

const char* const* uloc_getISOCountries(void) {
  const char* const* (*ptr)(void);
  ptr = (const char* const*(*)(void))syms[728];
  return ptr();
}

int32_t uloc_getParent(const char* localeID, char* parent, int32_t parentCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[729];
  return ptr(localeID, parent, parentCapacity, err);
}

int32_t uloc_getBaseName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[730];
  return ptr(localeID, name, nameCapacity, err);
}

UEnumeration* uloc_openKeywords(const char* localeID, UErrorCode* status) {
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[731];
  return ptr(localeID, status);
}

int32_t uloc_getKeywordValue(const char* localeID, const char* keywordName, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))syms[732];
  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}

int32_t uloc_setKeywordValue(const char* keywordName, const char* keywordValue, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))syms[733];
  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}

ULayoutType uloc_getCharacterOrientation(const char* localeId, UErrorCode* status) {
  ULayoutType (*ptr)(const char*, UErrorCode*);
  ptr = (ULayoutType(*)(const char*, UErrorCode*))syms[734];
  return ptr(localeId, status);
}

ULayoutType uloc_getLineOrientation(const char* localeId, UErrorCode* status) {
  ULayoutType (*ptr)(const char*, UErrorCode*);
  ptr = (ULayoutType(*)(const char*, UErrorCode*))syms[735];
  return ptr(localeId, status);
}

int32_t uloc_acceptLanguageFromHTTP(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char* httpAcceptLanguage, UEnumeration* availableLocales, UErrorCode* status) {
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*);
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*))syms[736];
  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}

int32_t uloc_acceptLanguage(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char** acceptList, int32_t acceptListCount, UEnumeration* availableLocales, UErrorCode* status) {
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*);
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*))syms[737];
  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}

int32_t uloc_getLocaleForLCID(uint32_t hostID, char* locale, int32_t localeCapacity, UErrorCode* status) {
  int32_t (*ptr)(uint32_t, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(uint32_t, char*, int32_t, UErrorCode*))syms[738];
  return ptr(hostID, locale, localeCapacity, status);
}

int32_t uloc_addLikelySubtags(const char* localeID, char* maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[739];
  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}

int32_t uloc_minimizeSubtags(const char* localeID, char* minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode* err) {
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[740];
  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}

/* unicode/ucat.h */
u_nl_catd u_catopen(const char* name, const char* locale, UErrorCode* ec) {
  u_nl_catd (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (u_nl_catd(*)(const char*, const char*, UErrorCode*))syms[741];
  return ptr(name, locale, ec);
}

void u_catclose(u_nl_catd catd) {
  void (*ptr)(u_nl_catd);
  ptr = (void(*)(u_nl_catd))syms[742];
  ptr(catd);
  return;
}

const UChar* u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar* s, int32_t* len, UErrorCode* ec) {
  const UChar* (*ptr)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*))syms[743];
  return ptr(catd, set_num, msg_num, s, len, ec);
}

/* unicode/ucnvsel.h */
UConverterSelector* ucnvsel_open(const char* const* converterList, int32_t converterListSize, const USet* excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode* status) {
  UConverterSelector* (*ptr)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*);
  ptr = (UConverterSelector*(*)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*))syms[744];
  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}

void ucnvsel_close(UConverterSelector* sel) {
  void (*ptr)(UConverterSelector*);
  ptr = (void(*)(UConverterSelector*))syms[745];
  ptr(sel);
  return;
}

UConverterSelector* ucnvsel_openFromSerialized(const void* buffer, int32_t length, UErrorCode* status) {
  UConverterSelector* (*ptr)(const void*, int32_t, UErrorCode*);
  ptr = (UConverterSelector*(*)(const void*, int32_t, UErrorCode*))syms[746];
  return ptr(buffer, length, status);
}

int32_t ucnvsel_serialize(const UConverterSelector* sel, void* buffer, int32_t bufferCapacity, UErrorCode* status) {
  int32_t (*ptr)(const UConverterSelector*, void*, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UConverterSelector*, void*, int32_t, UErrorCode*))syms[747];
  return ptr(sel, buffer, bufferCapacity, status);
}

UEnumeration* ucnvsel_selectForString(const UConverterSelector* sel, const UChar* s, int32_t length, UErrorCode* status) {
  UEnumeration* (*ptr)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*);
  ptr = (UEnumeration*(*)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*))syms[748];
  return ptr(sel, s, length, status);
}

UEnumeration* ucnvsel_selectForUTF8(const UConverterSelector* sel, const char* s, int32_t length, UErrorCode* status) {
  UEnumeration* (*ptr)(const UConverterSelector*, const char*, int32_t, UErrorCode*);
  ptr = (UEnumeration*(*)(const UConverterSelector*, const char*, int32_t, UErrorCode*))syms[749];
  return ptr(sel, s, length, status);
}

/* unicode/usprep.h */
UStringPrepProfile* usprep_open(const char* path, const char* fileName, UErrorCode* status) {
  UStringPrepProfile* (*ptr)(const char*, const char*, UErrorCode*);
  ptr = (UStringPrepProfile*(*)(const char*, const char*, UErrorCode*))syms[750];
  return ptr(path, fileName, status);
}

UStringPrepProfile* usprep_openByType(UStringPrepProfileType type, UErrorCode* status) {
  UStringPrepProfile* (*ptr)(UStringPrepProfileType, UErrorCode*);
  ptr = (UStringPrepProfile*(*)(UStringPrepProfileType, UErrorCode*))syms[751];
  return ptr(type, status);
}

void usprep_close(UStringPrepProfile* profile) {
  void (*ptr)(UStringPrepProfile*);
  ptr = (void(*)(UStringPrepProfile*))syms[752];
  ptr(profile);
  return;
}

int32_t usprep_prepare(const UStringPrepProfile* prep, const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  int32_t (*ptr)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  ptr = (int32_t(*)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[753];
  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}

/* unicode/uversion.h */
void u_versionFromString(UVersionInfo versionArray, const char* versionString) {
  void (*ptr)(UVersionInfo, const char*);
  ptr = (void(*)(UVersionInfo, const char*))syms[754];
  ptr(versionArray, versionString);
  return;
}

void u_versionFromUString(UVersionInfo versionArray, const UChar* versionString) {
  void (*ptr)(UVersionInfo, const UChar*);
  ptr = (void(*)(UVersionInfo, const UChar*))syms[755];
  ptr(versionArray, versionString);
  return;
}

void u_versionToString(UVersionInfo versionArray, char* versionString) {
  void (*ptr)(UVersionInfo, char*);
  ptr = (void(*)(UVersionInfo, char*))syms[756];
  ptr(versionArray, versionString);
  return;
}

void u_getVersion(UVersionInfo versionArray) {
  void (*ptr)(UVersionInfo);
  ptr = (void(*)(UVersionInfo))syms[757];
  ptr(versionArray);
  return;
}

/* unicode/uidna.h */
int32_t uidna_toASCII(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[758];
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_toUnicode(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[759];
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_IDNToASCII(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[760];
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_IDNToUnicode(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[761];
  return ptr(src, srcLength, dest, destCapacity, options, parseError, status);
}

int32_t uidna_compare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, int32_t options, UErrorCode* status) {
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, int32_t, UErrorCode*);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, int32_t, UErrorCode*))syms[762];
  return ptr(s1, length1, s2, length2, options, status);
}

/* unicode/uenum.h */
void uenum_close(UEnumeration* en) {
  void (*ptr)(UEnumeration*);
  ptr = (void(*)(UEnumeration*))syms[763];
  ptr(en);
  return;
}

int32_t uenum_count(UEnumeration* en, UErrorCode* status) {
  int32_t (*ptr)(UEnumeration*, UErrorCode*);
  ptr = (int32_t(*)(UEnumeration*, UErrorCode*))syms[764];
  return ptr(en, status);
}

const UChar* uenum_unext(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  const UChar* (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  ptr = (const UChar*(*)(UEnumeration*, int32_t*, UErrorCode*))syms[765];
  return ptr(en, resultLength, status);
}

const char* uenum_next(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  const char* (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  ptr = (const char*(*)(UEnumeration*, int32_t*, UErrorCode*))syms[766];
  return ptr(en, resultLength, status);
}

void uenum_reset(UEnumeration* en, UErrorCode* status) {
  void (*ptr)(UEnumeration*, UErrorCode*);
  ptr = (void(*)(UEnumeration*, UErrorCode*))syms[767];
  ptr(en, status);
  return;
}


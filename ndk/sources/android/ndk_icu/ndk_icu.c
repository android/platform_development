/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#include <unicode/ucsdet.h>
#include <unicode/udateintervalformat.h>
#include <unicode/udatpg.h>
#include <unicode/udat.h>
#include <unicode/ucol.h>
#include <unicode/ugender.h>
#include <unicode/uspoof.h>
#include <unicode/uformattable.h>
#include <unicode/unum.h>
#include <unicode/utmscale.h>
#include <unicode/upluralrules.h>
#include <unicode/unumsys.h>
#include <unicode/ucoleitr.h>
#include <unicode/uregex.h>
#include <unicode/ulocdata.h>
#include <unicode/uregion.h>
#include <unicode/ucal.h>
#include <unicode/ucurr.h>
#include <unicode/umsg.h>
#include <unicode/uldnames.h>
#include <unicode/usearch.h>
#include <unicode/utrans.h>
#include <unicode/utypes.h>
#include <unicode/ucnv_err.h>
#include <unicode/uloc.h>
#include <unicode/uchar.h>
#include <unicode/ucnvsel.h>
#include <unicode/uclean.h>
#include <unicode/utrace.h>
#include <unicode/uset.h>
#include <unicode/uenum.h>
#include <unicode/ucnv.h>
#include <unicode/uidna.h>
#include <unicode/ucnv_cb.h>
#include <unicode/ures.h>
#include <unicode/uscript.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/ucat.h>
#include <unicode/ubidi.h>
#include <unicode/udata.h>
#include <unicode/ucasemap.h>
#include <unicode/uversion.h>
#include <unicode/unorm2.h>
#include <unicode/icudataver.h>
#include <unicode/unorm.h>
#include <unicode/usprep.h>
#include <unicode/ushape.h>
#include <unicode/uiter.h>
#include <unicode/putil.h>
#include <unicode/ubrk.h>
#include <unicode/utf8.h>

/* Allowed version number ranges between [44, 999].
 * 44 is the minimum supported ICU version that was shipped in
 * Gingerbread (2.3.3) devices.
 */
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3
#define ICUDATA_VERSION_MIN        44

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

static void* handle_i18n = NULL;
static void* handle_common = NULL;
static void* syms[934];

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
    __android_log_print(ANDROID_LOG_ERROR, "NDKICU",
        "Cannot locate ICU data file at /system/usr/icu.");
    return;
  }

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);

  if (!handle_i18n || !handle_common) {
    __android_log_print(ANDROID_LOG_ERROR, "NDKICU", "Cannot open ICU libraries.");
    return;
  }

  char func_name[128];
  strcpy(func_name, "ucsdet_open");
  strcat(func_name, icudata_version);
  syms[0] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_close");
  strcat(func_name, icudata_version);
  syms[1] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_setText");
  strcat(func_name, icudata_version);
  syms[2] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_setDeclaredEncoding");
  strcat(func_name, icudata_version);
  syms[3] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_detect");
  strcat(func_name, icudata_version);
  syms[4] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_detectAll");
  strcat(func_name, icudata_version);
  syms[5] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getName");
  strcat(func_name, icudata_version);
  syms[6] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getConfidence");
  strcat(func_name, icudata_version);
  syms[7] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getLanguage");
  strcat(func_name, icudata_version);
  syms[8] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getUChars");
  strcat(func_name, icudata_version);
  syms[9] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_getAllDetectableCharsets");
  strcat(func_name, icudata_version);
  syms[10] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_isInputFilterEnabled");
  strcat(func_name, icudata_version);
  syms[11] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucsdet_enableInputFilter");
  strcat(func_name, icudata_version);
  syms[12] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udtitvfmt_open");
  strcat(func_name, icudata_version);
  syms[13] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udtitvfmt_close");
  strcat(func_name, icudata_version);
  syms[14] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udtitvfmt_format");
  strcat(func_name, icudata_version);
  syms[15] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_open");
  strcat(func_name, icudata_version);
  syms[16] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openEmpty");
  strcat(func_name, icudata_version);
  syms[17] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_close");
  strcat(func_name, icudata_version);
  syms[18] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_clone");
  strcat(func_name, icudata_version);
  syms[19] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getBestPattern");
  strcat(func_name, icudata_version);
  syms[20] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getBestPatternWithOptions");
  strcat(func_name, icudata_version);
  syms[21] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getSkeleton");
  strcat(func_name, icudata_version);
  syms[22] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getBaseSkeleton");
  strcat(func_name, icudata_version);
  syms[23] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_addPattern");
  strcat(func_name, icudata_version);
  syms[24] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setAppendItemFormat");
  strcat(func_name, icudata_version);
  syms[25] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getAppendItemFormat");
  strcat(func_name, icudata_version);
  syms[26] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setAppendItemName");
  strcat(func_name, icudata_version);
  syms[27] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getAppendItemName");
  strcat(func_name, icudata_version);
  syms[28] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setDateTimeFormat");
  strcat(func_name, icudata_version);
  syms[29] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getDateTimeFormat");
  strcat(func_name, icudata_version);
  syms[30] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_setDecimal");
  strcat(func_name, icudata_version);
  syms[31] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getDecimal");
  strcat(func_name, icudata_version);
  syms[32] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_replaceFieldTypes");
  strcat(func_name, icudata_version);
  syms[33] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_replaceFieldTypesWithOptions");
  strcat(func_name, icudata_version);
  syms[34] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openSkeletons");
  strcat(func_name, icudata_version);
  syms[35] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_openBaseSkeletons");
  strcat(func_name, icudata_version);
  syms[36] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udatpg_getPatternForSkeleton");
  strcat(func_name, icudata_version);
  syms[37] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_toCalendarDateField");
  strcat(func_name, icudata_version);
  syms[38] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_open");
  strcat(func_name, icudata_version);
  syms[39] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_close");
  strcat(func_name, icudata_version);
  syms[40] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getBooleanAttribute");
  strcat(func_name, icudata_version);
  syms[41] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setBooleanAttribute");
  strcat(func_name, icudata_version);
  syms[42] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_clone");
  strcat(func_name, icudata_version);
  syms[43] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_format");
  strcat(func_name, icudata_version);
  syms[44] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_parse");
  strcat(func_name, icudata_version);
  syms[45] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_parseCalendar");
  strcat(func_name, icudata_version);
  syms[46] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_isLenient");
  strcat(func_name, icudata_version);
  syms[47] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setLenient");
  strcat(func_name, icudata_version);
  syms[48] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getCalendar");
  strcat(func_name, icudata_version);
  syms[49] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setCalendar");
  strcat(func_name, icudata_version);
  syms[50] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getNumberFormat");
  strcat(func_name, icudata_version);
  syms[51] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setNumberFormat");
  strcat(func_name, icudata_version);
  syms[52] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getAvailable");
  strcat(func_name, icudata_version);
  syms[53] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_countAvailable");
  strcat(func_name, icudata_version);
  syms[54] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_get2DigitYearStart");
  strcat(func_name, icudata_version);
  syms[55] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_set2DigitYearStart");
  strcat(func_name, icudata_version);
  syms[56] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_toPattern");
  strcat(func_name, icudata_version);
  syms[57] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_applyPattern");
  strcat(func_name, icudata_version);
  syms[58] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getSymbols");
  strcat(func_name, icudata_version);
  syms[59] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_countSymbols");
  strcat(func_name, icudata_version);
  syms[60] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_setSymbols");
  strcat(func_name, icudata_version);
  syms[61] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[62] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "udat_getContext");
  strcat(func_name, icudata_version);
  syms[63] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_open");
  strcat(func_name, icudata_version);
  syms[64] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openRules");
  strcat(func_name, icudata_version);
  syms[65] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getContractionsAndExpansions");
  strcat(func_name, icudata_version);
  syms[66] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_close");
  strcat(func_name, icudata_version);
  syms[67] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_strcoll");
  strcat(func_name, icudata_version);
  syms[68] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_strcollUTF8");
  strcat(func_name, icudata_version);
  syms[69] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_greater");
  strcat(func_name, icudata_version);
  syms[70] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_greaterOrEqual");
  strcat(func_name, icudata_version);
  syms[71] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_equal");
  strcat(func_name, icudata_version);
  syms[72] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_strcollIter");
  strcat(func_name, icudata_version);
  syms[73] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getStrength");
  strcat(func_name, icudata_version);
  syms[74] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setStrength");
  strcat(func_name, icudata_version);
  syms[75] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getReorderCodes");
  strcat(func_name, icudata_version);
  syms[76] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setReorderCodes");
  strcat(func_name, icudata_version);
  syms[77] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getEquivalentReorderCodes");
  strcat(func_name, icudata_version);
  syms[78] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getDisplayName");
  strcat(func_name, icudata_version);
  syms[79] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getAvailable");
  strcat(func_name, icudata_version);
  syms[80] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_countAvailable");
  strcat(func_name, icudata_version);
  syms[81] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openAvailableLocales");
  strcat(func_name, icudata_version);
  syms[82] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywords");
  strcat(func_name, icudata_version);
  syms[83] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywordValues");
  strcat(func_name, icudata_version);
  syms[84] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[85] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getFunctionalEquivalent");
  strcat(func_name, icudata_version);
  syms[86] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getRules");
  strcat(func_name, icudata_version);
  syms[87] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getSortKey");
  strcat(func_name, icudata_version);
  syms[88] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_nextSortKeyPart");
  strcat(func_name, icudata_version);
  syms[89] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getBound");
  strcat(func_name, icudata_version);
  syms[90] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getVersion");
  strcat(func_name, icudata_version);
  syms[91] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getUCAVersion");
  strcat(func_name, icudata_version);
  syms[92] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_mergeSortkeys");
  strcat(func_name, icudata_version);
  syms[93] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setAttribute");
  strcat(func_name, icudata_version);
  syms[94] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getAttribute");
  strcat(func_name, icudata_version);
  syms[95] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setMaxVariable");
  strcat(func_name, icudata_version);
  syms[96] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getMaxVariable");
  strcat(func_name, icudata_version);
  syms[97] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getVariableTop");
  strcat(func_name, icudata_version);
  syms[98] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_safeClone");
  strcat(func_name, icudata_version);
  syms[99] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getRulesEx");
  strcat(func_name, icudata_version);
  syms[100] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[101] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getTailoredSet");
  strcat(func_name, icudata_version);
  syms[102] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_cloneBinary");
  strcat(func_name, icudata_version);
  syms[103] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openBinary");
  strcat(func_name, icudata_version);
  syms[104] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ugender_getInstance");
  strcat(func_name, icudata_version);
  syms[105] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ugender_getListGender");
  strcat(func_name, icudata_version);
  syms[106] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_open");
  strcat(func_name, icudata_version);
  syms[107] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_openFromSerialized");
  strcat(func_name, icudata_version);
  syms[108] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_openFromSource");
  strcat(func_name, icudata_version);
  syms[109] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_close");
  strcat(func_name, icudata_version);
  syms[110] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_clone");
  strcat(func_name, icudata_version);
  syms[111] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setChecks");
  strcat(func_name, icudata_version);
  syms[112] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getChecks");
  strcat(func_name, icudata_version);
  syms[113] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setRestrictionLevel");
  strcat(func_name, icudata_version);
  syms[114] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getRestrictionLevel");
  strcat(func_name, icudata_version);
  syms[115] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setAllowedLocales");
  strcat(func_name, icudata_version);
  syms[116] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getAllowedLocales");
  strcat(func_name, icudata_version);
  syms[117] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_setAllowedChars");
  strcat(func_name, icudata_version);
  syms[118] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getAllowedChars");
  strcat(func_name, icudata_version);
  syms[119] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_check");
  strcat(func_name, icudata_version);
  syms[120] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_checkUTF8");
  strcat(func_name, icudata_version);
  syms[121] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_areConfusable");
  strcat(func_name, icudata_version);
  syms[122] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_areConfusableUTF8");
  strcat(func_name, icudata_version);
  syms[123] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getSkeleton");
  strcat(func_name, icudata_version);
  syms[124] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getSkeletonUTF8");
  strcat(func_name, icudata_version);
  syms[125] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getInclusionSet");
  strcat(func_name, icudata_version);
  syms[126] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_getRecommendedSet");
  strcat(func_name, icudata_version);
  syms[127] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uspoof_serialize");
  strcat(func_name, icudata_version);
  syms[128] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_open");
  strcat(func_name, icudata_version);
  syms[129] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_close");
  strcat(func_name, icudata_version);
  syms[130] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getType");
  strcat(func_name, icudata_version);
  syms[131] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_isNumeric");
  strcat(func_name, icudata_version);
  syms[132] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getDate");
  strcat(func_name, icudata_version);
  syms[133] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getDouble");
  strcat(func_name, icudata_version);
  syms[134] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getLong");
  strcat(func_name, icudata_version);
  syms[135] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getInt64");
  strcat(func_name, icudata_version);
  syms[136] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getObject");
  strcat(func_name, icudata_version);
  syms[137] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getUChars");
  strcat(func_name, icudata_version);
  syms[138] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getArrayLength");
  strcat(func_name, icudata_version);
  syms[139] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getArrayItemByIndex");
  strcat(func_name, icudata_version);
  syms[140] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ufmt_getDecNumChars");
  strcat(func_name, icudata_version);
  syms[141] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_open");
  strcat(func_name, icudata_version);
  syms[142] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_close");
  strcat(func_name, icudata_version);
  syms[143] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_clone");
  strcat(func_name, icudata_version);
  syms[144] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_format");
  strcat(func_name, icudata_version);
  syms[145] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatInt64");
  strcat(func_name, icudata_version);
  syms[146] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatDouble");
  strcat(func_name, icudata_version);
  syms[147] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatDecimal");
  strcat(func_name, icudata_version);
  syms[148] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatDoubleCurrency");
  strcat(func_name, icudata_version);
  syms[149] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_formatUFormattable");
  strcat(func_name, icudata_version);
  syms[150] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parse");
  strcat(func_name, icudata_version);
  syms[151] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseInt64");
  strcat(func_name, icudata_version);
  syms[152] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseDouble");
  strcat(func_name, icudata_version);
  syms[153] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseDecimal");
  strcat(func_name, icudata_version);
  syms[154] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseDoubleCurrency");
  strcat(func_name, icudata_version);
  syms[155] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_parseToUFormattable");
  strcat(func_name, icudata_version);
  syms[156] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_applyPattern");
  strcat(func_name, icudata_version);
  syms[157] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getAvailable");
  strcat(func_name, icudata_version);
  syms[158] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_countAvailable");
  strcat(func_name, icudata_version);
  syms[159] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getAttribute");
  strcat(func_name, icudata_version);
  syms[160] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setAttribute");
  strcat(func_name, icudata_version);
  syms[161] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getDoubleAttribute");
  strcat(func_name, icudata_version);
  syms[162] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setDoubleAttribute");
  strcat(func_name, icudata_version);
  syms[163] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getTextAttribute");
  strcat(func_name, icudata_version);
  syms[164] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setTextAttribute");
  strcat(func_name, icudata_version);
  syms[165] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_toPattern");
  strcat(func_name, icudata_version);
  syms[166] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getSymbol");
  strcat(func_name, icudata_version);
  syms[167] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setSymbol");
  strcat(func_name, icudata_version);
  syms[168] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[169] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_setContext");
  strcat(func_name, icudata_version);
  syms[170] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unum_getContext");
  strcat(func_name, icudata_version);
  syms[171] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_getTimeScaleValue");
  strcat(func_name, icudata_version);
  syms[172] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_fromInt64");
  strcat(func_name, icudata_version);
  syms[173] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utmscale_toInt64");
  strcat(func_name, icudata_version);
  syms[174] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uplrules_open");
  strcat(func_name, icudata_version);
  syms[175] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uplrules_close");
  strcat(func_name, icudata_version);
  syms[176] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uplrules_select");
  strcat(func_name, icudata_version);
  syms[177] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_open");
  strcat(func_name, icudata_version);
  syms[178] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_openByName");
  strcat(func_name, icudata_version);
  syms[179] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_close");
  strcat(func_name, icudata_version);
  syms[180] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_openAvailableNames");
  strcat(func_name, icudata_version);
  syms[181] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_getName");
  strcat(func_name, icudata_version);
  syms[182] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_isAlgorithmic");
  strcat(func_name, icudata_version);
  syms[183] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_getRadix");
  strcat(func_name, icudata_version);
  syms[184] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "unumsys_getDescription");
  strcat(func_name, icudata_version);
  syms[185] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_openElements");
  strcat(func_name, icudata_version);
  syms[186] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_keyHashCode");
  strcat(func_name, icudata_version);
  syms[187] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_closeElements");
  strcat(func_name, icudata_version);
  syms[188] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_reset");
  strcat(func_name, icudata_version);
  syms[189] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_next");
  strcat(func_name, icudata_version);
  syms[190] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_previous");
  strcat(func_name, icudata_version);
  syms[191] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getMaxExpansion");
  strcat(func_name, icudata_version);
  syms[192] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setText");
  strcat(func_name, icudata_version);
  syms[193] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_getOffset");
  strcat(func_name, icudata_version);
  syms[194] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_setOffset");
  strcat(func_name, icudata_version);
  syms[195] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_primaryOrder");
  strcat(func_name, icudata_version);
  syms[196] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_secondaryOrder");
  strcat(func_name, icudata_version);
  syms[197] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucol_tertiaryOrder");
  strcat(func_name, icudata_version);
  syms[198] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_open");
  strcat(func_name, icudata_version);
  syms[199] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_openUText");
  strcat(func_name, icudata_version);
  syms[200] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_openC");
  strcat(func_name, icudata_version);
  syms[201] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_close");
  strcat(func_name, icudata_version);
  syms[202] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_clone");
  strcat(func_name, icudata_version);
  syms[203] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_pattern");
  strcat(func_name, icudata_version);
  syms[204] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_patternUText");
  strcat(func_name, icudata_version);
  syms[205] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_flags");
  strcat(func_name, icudata_version);
  syms[206] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setText");
  strcat(func_name, icudata_version);
  syms[207] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setUText");
  strcat(func_name, icudata_version);
  syms[208] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getText");
  strcat(func_name, icudata_version);
  syms[209] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getUText");
  strcat(func_name, icudata_version);
  syms[210] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_refreshUText");
  strcat(func_name, icudata_version);
  syms[211] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_matches");
  strcat(func_name, icudata_version);
  syms[212] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_matches64");
  strcat(func_name, icudata_version);
  syms[213] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_lookingAt");
  strcat(func_name, icudata_version);
  syms[214] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_lookingAt64");
  strcat(func_name, icudata_version);
  syms[215] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_find");
  strcat(func_name, icudata_version);
  syms[216] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_find64");
  strcat(func_name, icudata_version);
  syms[217] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_findNext");
  strcat(func_name, icudata_version);
  syms[218] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_groupCount");
  strcat(func_name, icudata_version);
  syms[219] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_group");
  strcat(func_name, icudata_version);
  syms[220] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_groupUText");
  strcat(func_name, icudata_version);
  syms[221] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_start");
  strcat(func_name, icudata_version);
  syms[222] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_start64");
  strcat(func_name, icudata_version);
  syms[223] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_end");
  strcat(func_name, icudata_version);
  syms[224] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_end64");
  strcat(func_name, icudata_version);
  syms[225] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_reset");
  strcat(func_name, icudata_version);
  syms[226] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_reset64");
  strcat(func_name, icudata_version);
  syms[227] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setRegion");
  strcat(func_name, icudata_version);
  syms[228] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setRegion64");
  strcat(func_name, icudata_version);
  syms[229] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setRegionAndStart");
  strcat(func_name, icudata_version);
  syms[230] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionStart");
  strcat(func_name, icudata_version);
  syms[231] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionStart64");
  strcat(func_name, icudata_version);
  syms[232] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionEnd");
  strcat(func_name, icudata_version);
  syms[233] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_regionEnd64");
  strcat(func_name, icudata_version);
  syms[234] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hasTransparentBounds");
  strcat(func_name, icudata_version);
  syms[235] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_useTransparentBounds");
  strcat(func_name, icudata_version);
  syms[236] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hasAnchoringBounds");
  strcat(func_name, icudata_version);
  syms[237] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_useAnchoringBounds");
  strcat(func_name, icudata_version);
  syms[238] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_hitEnd");
  strcat(func_name, icudata_version);
  syms[239] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_requireEnd");
  strcat(func_name, icudata_version);
  syms[240] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceAll");
  strcat(func_name, icudata_version);
  syms[241] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceAllUText");
  strcat(func_name, icudata_version);
  syms[242] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceFirst");
  strcat(func_name, icudata_version);
  syms[243] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_replaceFirstUText");
  strcat(func_name, icudata_version);
  syms[244] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendReplacement");
  strcat(func_name, icudata_version);
  syms[245] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendReplacementUText");
  strcat(func_name, icudata_version);
  syms[246] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendTail");
  strcat(func_name, icudata_version);
  syms[247] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_appendTailUText");
  strcat(func_name, icudata_version);
  syms[248] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_split");
  strcat(func_name, icudata_version);
  syms[249] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_splitUText");
  strcat(func_name, icudata_version);
  syms[250] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setTimeLimit");
  strcat(func_name, icudata_version);
  syms[251] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getTimeLimit");
  strcat(func_name, icudata_version);
  syms[252] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setStackLimit");
  strcat(func_name, icudata_version);
  syms[253] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getStackLimit");
  strcat(func_name, icudata_version);
  syms[254] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setMatchCallback");
  strcat(func_name, icudata_version);
  syms[255] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getMatchCallback");
  strcat(func_name, icudata_version);
  syms[256] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_setFindProgressCallback");
  strcat(func_name, icudata_version);
  syms[257] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregex_getFindProgressCallback");
  strcat(func_name, icudata_version);
  syms[258] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_open");
  strcat(func_name, icudata_version);
  syms[259] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_close");
  strcat(func_name, icudata_version);
  syms[260] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_setNoSubstitute");
  strcat(func_name, icudata_version);
  syms[261] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getNoSubstitute");
  strcat(func_name, icudata_version);
  syms[262] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getExemplarSet");
  strcat(func_name, icudata_version);
  syms[263] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getDelimiter");
  strcat(func_name, icudata_version);
  syms[264] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getMeasurementSystem");
  strcat(func_name, icudata_version);
  syms[265] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getPaperSize");
  strcat(func_name, icudata_version);
  syms[266] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getCLDRVersion");
  strcat(func_name, icudata_version);
  syms[267] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getLocaleDisplayPattern");
  strcat(func_name, icudata_version);
  syms[268] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ulocdata_getLocaleSeparator");
  strcat(func_name, icudata_version);
  syms[269] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getRegionFromCode");
  strcat(func_name, icudata_version);
  syms[270] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getRegionFromNumericCode");
  strcat(func_name, icudata_version);
  syms[271] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getAvailable");
  strcat(func_name, icudata_version);
  syms[272] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_areEqual");
  strcat(func_name, icudata_version);
  syms[273] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getContainingRegion");
  strcat(func_name, icudata_version);
  syms[274] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getContainingRegionOfType");
  strcat(func_name, icudata_version);
  syms[275] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getContainedRegions");
  strcat(func_name, icudata_version);
  syms[276] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getContainedRegionsOfType");
  strcat(func_name, icudata_version);
  syms[277] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_contains");
  strcat(func_name, icudata_version);
  syms[278] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getPreferredValues");
  strcat(func_name, icudata_version);
  syms[279] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getRegionCode");
  strcat(func_name, icudata_version);
  syms[280] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getNumericCode");
  strcat(func_name, icudata_version);
  syms[281] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uregion_getType");
  strcat(func_name, icudata_version);
  syms[282] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_openTimeZoneIDEnumeration");
  strcat(func_name, icudata_version);
  syms[283] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_openTimeZones");
  strcat(func_name, icudata_version);
  syms[284] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_openCountryTimeZones");
  strcat(func_name, icudata_version);
  syms[285] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getDefaultTimeZone");
  strcat(func_name, icudata_version);
  syms[286] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDefaultTimeZone");
  strcat(func_name, icudata_version);
  syms[287] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getDSTSavings");
  strcat(func_name, icudata_version);
  syms[288] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getNow");
  strcat(func_name, icudata_version);
  syms[289] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_open");
  strcat(func_name, icudata_version);
  syms[290] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_close");
  strcat(func_name, icudata_version);
  syms[291] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clone");
  strcat(func_name, icudata_version);
  syms[292] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setTimeZone");
  strcat(func_name, icudata_version);
  syms[293] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTimeZoneID");
  strcat(func_name, icudata_version);
  syms[294] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTimeZoneDisplayName");
  strcat(func_name, icudata_version);
  syms[295] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_inDaylightTime");
  strcat(func_name, icudata_version);
  syms[296] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setGregorianChange");
  strcat(func_name, icudata_version);
  syms[297] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getGregorianChange");
  strcat(func_name, icudata_version);
  syms[298] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getAttribute");
  strcat(func_name, icudata_version);
  syms[299] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setAttribute");
  strcat(func_name, icudata_version);
  syms[300] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getAvailable");
  strcat(func_name, icudata_version);
  syms[301] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_countAvailable");
  strcat(func_name, icudata_version);
  syms[302] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getMillis");
  strcat(func_name, icudata_version);
  syms[303] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setMillis");
  strcat(func_name, icudata_version);
  syms[304] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDate");
  strcat(func_name, icudata_version);
  syms[305] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_setDateTime");
  strcat(func_name, icudata_version);
  syms[306] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_equivalentTo");
  strcat(func_name, icudata_version);
  syms[307] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_add");
  strcat(func_name, icudata_version);
  syms[308] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_roll");
  strcat(func_name, icudata_version);
  syms[309] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_get");
  strcat(func_name, icudata_version);
  syms[310] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_set");
  strcat(func_name, icudata_version);
  syms[311] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_isSet");
  strcat(func_name, icudata_version);
  syms[312] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clearField");
  strcat(func_name, icudata_version);
  syms[313] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_clear");
  strcat(func_name, icudata_version);
  syms[314] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getLimit");
  strcat(func_name, icudata_version);
  syms[315] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[316] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTZDataVersion");
  strcat(func_name, icudata_version);
  syms[317] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getCanonicalTimeZoneID");
  strcat(func_name, icudata_version);
  syms[318] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getType");
  strcat(func_name, icudata_version);
  syms[319] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[320] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getDayOfWeekType");
  strcat(func_name, icudata_version);
  syms[321] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getWeekendTransition");
  strcat(func_name, icudata_version);
  syms[322] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_isWeekend");
  strcat(func_name, icudata_version);
  syms[323] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getFieldDifference");
  strcat(func_name, icudata_version);
  syms[324] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTimeZoneTransitionDate");
  strcat(func_name, icudata_version);
  syms[325] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getWindowsTimeZoneID");
  strcat(func_name, icudata_version);
  syms[326] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucal_getTimeZoneIDForWindowsID");
  strcat(func_name, icudata_version);
  syms[327] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_forLocale");
  strcat(func_name, icudata_version);
  syms[328] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_register");
  strcat(func_name, icudata_version);
  syms[329] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_unregister");
  strcat(func_name, icudata_version);
  syms[330] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getName");
  strcat(func_name, icudata_version);
  syms[331] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getPluralName");
  strcat(func_name, icudata_version);
  syms[332] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getDefaultFractionDigits");
  strcat(func_name, icudata_version);
  syms[333] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getRoundingIncrement");
  strcat(func_name, icudata_version);
  syms[334] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_openISOCurrencies");
  strcat(func_name, icudata_version);
  syms[335] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_isAvailable");
  strcat(func_name, icudata_version);
  syms[336] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_countCurrencies");
  strcat(func_name, icudata_version);
  syms[337] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_forLocaleAndDate");
  strcat(func_name, icudata_version);
  syms[338] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getKeywordValuesForLocale");
  strcat(func_name, icudata_version);
  syms[339] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "ucurr_getNumericCode");
  strcat(func_name, icudata_version);
  syms[340] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessage");
  strcat(func_name, icudata_version);
  syms[341] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessage");
  strcat(func_name, icudata_version);
  syms[342] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessage");
  strcat(func_name, icudata_version);
  syms[343] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessage");
  strcat(func_name, icudata_version);
  syms[344] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessageWithError");
  strcat(func_name, icudata_version);
  syms[345] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vformatMessageWithError");
  strcat(func_name, icudata_version);
  syms[346] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessageWithError");
  strcat(func_name, icudata_version);
  syms[347] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_vparseMessageWithError");
  strcat(func_name, icudata_version);
  syms[348] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_open");
  strcat(func_name, icudata_version);
  syms[349] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_close");
  strcat(func_name, icudata_version);
  syms[350] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_clone");
  strcat(func_name, icudata_version);
  syms[351] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_setLocale");
  strcat(func_name, icudata_version);
  syms[352] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_getLocale");
  strcat(func_name, icudata_version);
  syms[353] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_applyPattern");
  strcat(func_name, icudata_version);
  syms[354] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_toPattern");
  strcat(func_name, icudata_version);
  syms[355] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vformat");
  strcat(func_name, icudata_version);
  syms[356] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vformat");
  strcat(func_name, icudata_version);
  syms[357] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vparse");
  strcat(func_name, icudata_version);
  syms[358] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_vparse");
  strcat(func_name, icudata_version);
  syms[359] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "umsg_autoQuoteApostrophe");
  strcat(func_name, icudata_version);
  syms[360] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_open");
  strcat(func_name, icudata_version);
  syms[361] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_close");
  strcat(func_name, icudata_version);
  syms[362] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_getLocale");
  strcat(func_name, icudata_version);
  syms[363] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_getDialectHandling");
  strcat(func_name, icudata_version);
  syms[364] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_localeDisplayName");
  strcat(func_name, icudata_version);
  syms[365] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_languageDisplayName");
  strcat(func_name, icudata_version);
  syms[366] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_scriptDisplayName");
  strcat(func_name, icudata_version);
  syms[367] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_scriptCodeDisplayName");
  strcat(func_name, icudata_version);
  syms[368] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_regionDisplayName");
  strcat(func_name, icudata_version);
  syms[369] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_variantDisplayName");
  strcat(func_name, icudata_version);
  syms[370] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_keyDisplayName");
  strcat(func_name, icudata_version);
  syms[371] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_keyValueDisplayName");
  strcat(func_name, icudata_version);
  syms[372] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_openForContext");
  strcat(func_name, icudata_version);
  syms[373] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "uldn_getContext");
  strcat(func_name, icudata_version);
  syms[374] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_open");
  strcat(func_name, icudata_version);
  syms[375] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_openFromCollator");
  strcat(func_name, icudata_version);
  syms[376] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_close");
  strcat(func_name, icudata_version);
  syms[377] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setOffset");
  strcat(func_name, icudata_version);
  syms[378] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getOffset");
  strcat(func_name, icudata_version);
  syms[379] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setAttribute");
  strcat(func_name, icudata_version);
  syms[380] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getAttribute");
  strcat(func_name, icudata_version);
  syms[381] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedStart");
  strcat(func_name, icudata_version);
  syms[382] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedLength");
  strcat(func_name, icudata_version);
  syms[383] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getMatchedText");
  strcat(func_name, icudata_version);
  syms[384] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setBreakIterator");
  strcat(func_name, icudata_version);
  syms[385] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getBreakIterator");
  strcat(func_name, icudata_version);
  syms[386] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setText");
  strcat(func_name, icudata_version);
  syms[387] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getText");
  strcat(func_name, icudata_version);
  syms[388] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getCollator");
  strcat(func_name, icudata_version);
  syms[389] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setCollator");
  strcat(func_name, icudata_version);
  syms[390] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_setPattern");
  strcat(func_name, icudata_version);
  syms[391] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_getPattern");
  strcat(func_name, icudata_version);
  syms[392] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_first");
  strcat(func_name, icudata_version);
  syms[393] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_following");
  strcat(func_name, icudata_version);
  syms[394] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_last");
  strcat(func_name, icudata_version);
  syms[395] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_preceding");
  strcat(func_name, icudata_version);
  syms[396] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_next");
  strcat(func_name, icudata_version);
  syms[397] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_previous");
  strcat(func_name, icudata_version);
  syms[398] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "usearch_reset");
  strcat(func_name, icudata_version);
  syms[399] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openU");
  strcat(func_name, icudata_version);
  syms[400] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openInverse");
  strcat(func_name, icudata_version);
  syms[401] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_clone");
  strcat(func_name, icudata_version);
  syms[402] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_close");
  strcat(func_name, icudata_version);
  syms[403] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_getUnicodeID");
  strcat(func_name, icudata_version);
  syms[404] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_register");
  strcat(func_name, icudata_version);
  syms[405] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_unregisterID");
  strcat(func_name, icudata_version);
  syms[406] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_setFilter");
  strcat(func_name, icudata_version);
  syms[407] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_countAvailableIDs");
  strcat(func_name, icudata_version);
  syms[408] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_openIDs");
  strcat(func_name, icudata_version);
  syms[409] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_trans");
  strcat(func_name, icudata_version);
  syms[410] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transIncremental");
  strcat(func_name, icudata_version);
  syms[411] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transUChars");
  strcat(func_name, icudata_version);
  syms[412] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_transIncrementalUChars");
  strcat(func_name, icudata_version);
  syms[413] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_toRules");
  strcat(func_name, icudata_version);
  syms[414] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "utrans_getSourceSet");
  strcat(func_name, icudata_version);
  syms[415] = dlsym(handle_i18n, func_name);

  strcpy(func_name, "u_errorName");
  strcat(func_name, icudata_version);
  syms[416] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  syms[417] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_STOP");
  strcat(func_name, icudata_version);
  syms[418] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  syms[419] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  syms[420] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_FROM_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  syms[421] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_SKIP");
  strcat(func_name, icudata_version);
  syms[422] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_SUBSTITUTE");
  strcat(func_name, icudata_version);
  syms[423] = dlsym(handle_common, func_name);

  strcpy(func_name, "UCNV_TO_U_CALLBACK_ESCAPE");
  strcat(func_name, icudata_version);
  syms[424] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDefault");
  strcat(func_name, icudata_version);
  syms[425] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_setDefault");
  strcat(func_name, icudata_version);
  syms[426] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLanguage");
  strcat(func_name, icudata_version);
  syms[427] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getScript");
  strcat(func_name, icudata_version);
  syms[428] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getCountry");
  strcat(func_name, icudata_version);
  syms[429] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getVariant");
  strcat(func_name, icudata_version);
  syms[430] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getName");
  strcat(func_name, icudata_version);
  syms[431] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_canonicalize");
  strcat(func_name, icudata_version);
  syms[432] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISO3Language");
  strcat(func_name, icudata_version);
  syms[433] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISO3Country");
  strcat(func_name, icudata_version);
  syms[434] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLCID");
  strcat(func_name, icudata_version);
  syms[435] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayLanguage");
  strcat(func_name, icudata_version);
  syms[436] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayScript");
  strcat(func_name, icudata_version);
  syms[437] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayCountry");
  strcat(func_name, icudata_version);
  syms[438] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayVariant");
  strcat(func_name, icudata_version);
  syms[439] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayKeyword");
  strcat(func_name, icudata_version);
  syms[440] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayKeywordValue");
  strcat(func_name, icudata_version);
  syms[441] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getDisplayName");
  strcat(func_name, icudata_version);
  syms[442] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getAvailable");
  strcat(func_name, icudata_version);
  syms[443] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_countAvailable");
  strcat(func_name, icudata_version);
  syms[444] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISOLanguages");
  strcat(func_name, icudata_version);
  syms[445] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getISOCountries");
  strcat(func_name, icudata_version);
  syms[446] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getParent");
  strcat(func_name, icudata_version);
  syms[447] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getBaseName");
  strcat(func_name, icudata_version);
  syms[448] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_openKeywords");
  strcat(func_name, icudata_version);
  syms[449] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getKeywordValue");
  strcat(func_name, icudata_version);
  syms[450] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_setKeywordValue");
  strcat(func_name, icudata_version);
  syms[451] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getCharacterOrientation");
  strcat(func_name, icudata_version);
  syms[452] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLineOrientation");
  strcat(func_name, icudata_version);
  syms[453] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_acceptLanguageFromHTTP");
  strcat(func_name, icudata_version);
  syms[454] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_acceptLanguage");
  strcat(func_name, icudata_version);
  syms[455] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_getLocaleForLCID");
  strcat(func_name, icudata_version);
  syms[456] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_addLikelySubtags");
  strcat(func_name, icudata_version);
  syms[457] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_minimizeSubtags");
  strcat(func_name, icudata_version);
  syms[458] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_forLanguageTag");
  strcat(func_name, icudata_version);
  syms[459] = dlsym(handle_common, func_name);

  strcpy(func_name, "uloc_toLanguageTag");
  strcat(func_name, icudata_version);
  syms[460] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_hasBinaryProperty");
  strcat(func_name, icudata_version);
  syms[461] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUAlphabetic");
  strcat(func_name, icudata_version);
  syms[462] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isULowercase");
  strcat(func_name, icudata_version);
  syms[463] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUUppercase");
  strcat(func_name, icudata_version);
  syms[464] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isUWhiteSpace");
  strcat(func_name, icudata_version);
  syms[465] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyValue");
  strcat(func_name, icudata_version);
  syms[466] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyMinValue");
  strcat(func_name, icudata_version);
  syms[467] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getIntPropertyMaxValue");
  strcat(func_name, icudata_version);
  syms[468] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getNumericValue");
  strcat(func_name, icudata_version);
  syms[469] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_islower");
  strcat(func_name, icudata_version);
  syms[470] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isupper");
  strcat(func_name, icudata_version);
  syms[471] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_istitle");
  strcat(func_name, icudata_version);
  syms[472] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isdigit");
  strcat(func_name, icudata_version);
  syms[473] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isalpha");
  strcat(func_name, icudata_version);
  syms[474] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isalnum");
  strcat(func_name, icudata_version);
  syms[475] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isxdigit");
  strcat(func_name, icudata_version);
  syms[476] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_ispunct");
  strcat(func_name, icudata_version);
  syms[477] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isgraph");
  strcat(func_name, icudata_version);
  syms[478] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isblank");
  strcat(func_name, icudata_version);
  syms[479] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isdefined");
  strcat(func_name, icudata_version);
  syms[480] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isspace");
  strcat(func_name, icudata_version);
  syms[481] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaSpaceChar");
  strcat(func_name, icudata_version);
  syms[482] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isWhitespace");
  strcat(func_name, icudata_version);
  syms[483] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_iscntrl");
  strcat(func_name, icudata_version);
  syms[484] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isISOControl");
  strcat(func_name, icudata_version);
  syms[485] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isprint");
  strcat(func_name, icudata_version);
  syms[486] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isbase");
  strcat(func_name, icudata_version);
  syms[487] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charDirection");
  strcat(func_name, icudata_version);
  syms[488] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isMirrored");
  strcat(func_name, icudata_version);
  syms[489] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charMirror");
  strcat(func_name, icudata_version);
  syms[490] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getBidiPairedBracket");
  strcat(func_name, icudata_version);
  syms[491] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charType");
  strcat(func_name, icudata_version);
  syms[492] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_enumCharTypes");
  strcat(func_name, icudata_version);
  syms[493] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getCombiningClass");
  strcat(func_name, icudata_version);
  syms[494] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charDigitValue");
  strcat(func_name, icudata_version);
  syms[495] = dlsym(handle_common, func_name);

  strcpy(func_name, "ublock_getCode");
  strcat(func_name, icudata_version);
  syms[496] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charName");
  strcat(func_name, icudata_version);
  syms[497] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charFromName");
  strcat(func_name, icudata_version);
  syms[498] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_enumCharNames");
  strcat(func_name, icudata_version);
  syms[499] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyName");
  strcat(func_name, icudata_version);
  syms[500] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyEnum");
  strcat(func_name, icudata_version);
  syms[501] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyValueName");
  strcat(func_name, icudata_version);
  syms[502] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getPropertyValueEnum");
  strcat(func_name, icudata_version);
  syms[503] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDStart");
  strcat(func_name, icudata_version);
  syms[504] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDPart");
  strcat(func_name, icudata_version);
  syms[505] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isIDIgnorable");
  strcat(func_name, icudata_version);
  syms[506] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaIDStart");
  strcat(func_name, icudata_version);
  syms[507] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_isJavaIDPart");
  strcat(func_name, icudata_version);
  syms[508] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_tolower");
  strcat(func_name, icudata_version);
  syms[509] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_toupper");
  strcat(func_name, icudata_version);
  syms[510] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_totitle");
  strcat(func_name, icudata_version);
  syms[511] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_foldCase");
  strcat(func_name, icudata_version);
  syms[512] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_digit");
  strcat(func_name, icudata_version);
  syms[513] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_forDigit");
  strcat(func_name, icudata_version);
  syms[514] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charAge");
  strcat(func_name, icudata_version);
  syms[515] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getUnicodeVersion");
  strcat(func_name, icudata_version);
  syms[516] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getFC_NFKC_Closure");
  strcat(func_name, icudata_version);
  syms[517] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_open");
  strcat(func_name, icudata_version);
  syms[518] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_close");
  strcat(func_name, icudata_version);
  syms[519] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_openFromSerialized");
  strcat(func_name, icudata_version);
  syms[520] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_serialize");
  strcat(func_name, icudata_version);
  syms[521] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_selectForString");
  strcat(func_name, icudata_version);
  syms[522] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnvsel_selectForUTF8");
  strcat(func_name, icudata_version);
  syms[523] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_init");
  strcat(func_name, icudata_version);
  syms[524] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_cleanup");
  strcat(func_name, icudata_version);
  syms[525] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_setMemoryFunctions");
  strcat(func_name, icudata_version);
  syms[526] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_setLevel");
  strcat(func_name, icudata_version);
  syms[527] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_getLevel");
  strcat(func_name, icudata_version);
  syms[528] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_setFunctions");
  strcat(func_name, icudata_version);
  syms[529] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_getFunctions");
  strcat(func_name, icudata_version);
  syms[530] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_vformat");
  strcat(func_name, icudata_version);
  syms[531] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_vformat");
  strcat(func_name, icudata_version);
  syms[532] = dlsym(handle_common, func_name);

  strcpy(func_name, "utrace_functionName");
  strcat(func_name, icudata_version);
  syms[533] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openEmpty");
  strcat(func_name, icudata_version);
  syms[534] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_open");
  strcat(func_name, icudata_version);
  syms[535] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openPattern");
  strcat(func_name, icudata_version);
  syms[536] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_openPatternOptions");
  strcat(func_name, icudata_version);
  syms[537] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_close");
  strcat(func_name, icudata_version);
  syms[538] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_clone");
  strcat(func_name, icudata_version);
  syms[539] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_isFrozen");
  strcat(func_name, icudata_version);
  syms[540] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_freeze");
  strcat(func_name, icudata_version);
  syms[541] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_cloneAsThawed");
  strcat(func_name, icudata_version);
  syms[542] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_set");
  strcat(func_name, icudata_version);
  syms[543] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyPattern");
  strcat(func_name, icudata_version);
  syms[544] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyIntPropertyValue");
  strcat(func_name, icudata_version);
  syms[545] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_applyPropertyAlias");
  strcat(func_name, icudata_version);
  syms[546] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_resemblesPattern");
  strcat(func_name, icudata_version);
  syms[547] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_toPattern");
  strcat(func_name, icudata_version);
  syms[548] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_add");
  strcat(func_name, icudata_version);
  syms[549] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addAll");
  strcat(func_name, icudata_version);
  syms[550] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addRange");
  strcat(func_name, icudata_version);
  syms[551] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addString");
  strcat(func_name, icudata_version);
  syms[552] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_addAllCodePoints");
  strcat(func_name, icudata_version);
  syms[553] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_remove");
  strcat(func_name, icudata_version);
  syms[554] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeRange");
  strcat(func_name, icudata_version);
  syms[555] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeString");
  strcat(func_name, icudata_version);
  syms[556] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeAll");
  strcat(func_name, icudata_version);
  syms[557] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_retain");
  strcat(func_name, icudata_version);
  syms[558] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_retainAll");
  strcat(func_name, icudata_version);
  syms[559] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_compact");
  strcat(func_name, icudata_version);
  syms[560] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_complement");
  strcat(func_name, icudata_version);
  syms[561] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_complementAll");
  strcat(func_name, icudata_version);
  syms[562] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_clear");
  strcat(func_name, icudata_version);
  syms[563] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_closeOver");
  strcat(func_name, icudata_version);
  syms[564] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_removeAllStrings");
  strcat(func_name, icudata_version);
  syms[565] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_isEmpty");
  strcat(func_name, icudata_version);
  syms[566] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_contains");
  strcat(func_name, icudata_version);
  syms[567] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsRange");
  strcat(func_name, icudata_version);
  syms[568] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsString");
  strcat(func_name, icudata_version);
  syms[569] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_indexOf");
  strcat(func_name, icudata_version);
  syms[570] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_charAt");
  strcat(func_name, icudata_version);
  syms[571] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_size");
  strcat(func_name, icudata_version);
  syms[572] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getItemCount");
  strcat(func_name, icudata_version);
  syms[573] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getItem");
  strcat(func_name, icudata_version);
  syms[574] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsAll");
  strcat(func_name, icudata_version);
  syms[575] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsAllCodePoints");
  strcat(func_name, icudata_version);
  syms[576] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsNone");
  strcat(func_name, icudata_version);
  syms[577] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_containsSome");
  strcat(func_name, icudata_version);
  syms[578] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_span");
  strcat(func_name, icudata_version);
  syms[579] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanBack");
  strcat(func_name, icudata_version);
  syms[580] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanUTF8");
  strcat(func_name, icudata_version);
  syms[581] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_spanBackUTF8");
  strcat(func_name, icudata_version);
  syms[582] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_equals");
  strcat(func_name, icudata_version);
  syms[583] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_serialize");
  strcat(func_name, icudata_version);
  syms[584] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedSet");
  strcat(func_name, icudata_version);
  syms[585] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_setSerializedToOne");
  strcat(func_name, icudata_version);
  syms[586] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_serializedContains");
  strcat(func_name, icudata_version);
  syms[587] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedRangeCount");
  strcat(func_name, icudata_version);
  syms[588] = dlsym(handle_common, func_name);

  strcpy(func_name, "uset_getSerializedRange");
  strcat(func_name, icudata_version);
  syms[589] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_close");
  strcat(func_name, icudata_version);
  syms[590] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_count");
  strcat(func_name, icudata_version);
  syms[591] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_unext");
  strcat(func_name, icudata_version);
  syms[592] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_next");
  strcat(func_name, icudata_version);
  syms[593] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_reset");
  strcat(func_name, icudata_version);
  syms[594] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_openUCharStringsEnumeration");
  strcat(func_name, icudata_version);
  syms[595] = dlsym(handle_common, func_name);

  strcpy(func_name, "uenum_openCharStringsEnumeration");
  strcat(func_name, icudata_version);
  syms[596] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_compareNames");
  strcat(func_name, icudata_version);
  syms[597] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_open");
  strcat(func_name, icudata_version);
  syms[598] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openU");
  strcat(func_name, icudata_version);
  syms[599] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openCCSID");
  strcat(func_name, icudata_version);
  syms[600] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openPackage");
  strcat(func_name, icudata_version);
  syms[601] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_safeClone");
  strcat(func_name, icudata_version);
  syms[602] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_close");
  strcat(func_name, icudata_version);
  syms[603] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getSubstChars");
  strcat(func_name, icudata_version);
  syms[604] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setSubstChars");
  strcat(func_name, icudata_version);
  syms[605] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setSubstString");
  strcat(func_name, icudata_version);
  syms[606] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getInvalidChars");
  strcat(func_name, icudata_version);
  syms[607] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getInvalidUChars");
  strcat(func_name, icudata_version);
  syms[608] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_reset");
  strcat(func_name, icudata_version);
  syms[609] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_resetToUnicode");
  strcat(func_name, icudata_version);
  syms[610] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_resetFromUnicode");
  strcat(func_name, icudata_version);
  syms[611] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getMaxCharSize");
  strcat(func_name, icudata_version);
  syms[612] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getMinCharSize");
  strcat(func_name, icudata_version);
  syms[613] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getDisplayName");
  strcat(func_name, icudata_version);
  syms[614] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getName");
  strcat(func_name, icudata_version);
  syms[615] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getCCSID");
  strcat(func_name, icudata_version);
  syms[616] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getPlatform");
  strcat(func_name, icudata_version);
  syms[617] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getType");
  strcat(func_name, icudata_version);
  syms[618] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStarters");
  strcat(func_name, icudata_version);
  syms[619] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getUnicodeSet");
  strcat(func_name, icudata_version);
  syms[620] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getToUCallBack");
  strcat(func_name, icudata_version);
  syms[621] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getFromUCallBack");
  strcat(func_name, icudata_version);
  syms[622] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setToUCallBack");
  strcat(func_name, icudata_version);
  syms[623] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setFromUCallBack");
  strcat(func_name, icudata_version);
  syms[624] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUnicode");
  strcat(func_name, icudata_version);
  syms[625] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUnicode");
  strcat(func_name, icudata_version);
  syms[626] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUChars");
  strcat(func_name, icudata_version);
  syms[627] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUChars");
  strcat(func_name, icudata_version);
  syms[628] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getNextUChar");
  strcat(func_name, icudata_version);
  syms[629] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_convertEx");
  strcat(func_name, icudata_version);
  syms[630] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_convert");
  strcat(func_name, icudata_version);
  syms[631] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toAlgorithmic");
  strcat(func_name, icudata_version);
  syms[632] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromAlgorithmic");
  strcat(func_name, icudata_version);
  syms[633] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_flushCache");
  strcat(func_name, icudata_version);
  syms[634] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countAvailable");
  strcat(func_name, icudata_version);
  syms[635] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAvailableName");
  strcat(func_name, icudata_version);
  syms[636] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openAllNames");
  strcat(func_name, icudata_version);
  syms[637] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countAliases");
  strcat(func_name, icudata_version);
  syms[638] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAlias");
  strcat(func_name, icudata_version);
  syms[639] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getAliases");
  strcat(func_name, icudata_version);
  syms[640] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_openStandardNames");
  strcat(func_name, icudata_version);
  syms[641] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_countStandards");
  strcat(func_name, icudata_version);
  syms[642] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStandard");
  strcat(func_name, icudata_version);
  syms[643] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getStandardName");
  strcat(func_name, icudata_version);
  syms[644] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getCanonicalName");
  strcat(func_name, icudata_version);
  syms[645] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_getDefaultName");
  strcat(func_name, icudata_version);
  syms[646] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setDefaultName");
  strcat(func_name, icudata_version);
  syms[647] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fixFileSeparator");
  strcat(func_name, icudata_version);
  syms[648] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_isAmbiguous");
  strcat(func_name, icudata_version);
  syms[649] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_setFallback");
  strcat(func_name, icudata_version);
  syms[650] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_usesFallback");
  strcat(func_name, icudata_version);
  syms[651] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_detectUnicodeSignature");
  strcat(func_name, icudata_version);
  syms[652] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_fromUCountPending");
  strcat(func_name, icudata_version);
  syms[653] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_toUCountPending");
  strcat(func_name, icudata_version);
  syms[654] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_isFixedWidth");
  strcat(func_name, icudata_version);
  syms[655] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_openUTS46");
  strcat(func_name, icudata_version);
  syms[656] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_close");
  strcat(func_name, icudata_version);
  syms[657] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_labelToASCII");
  strcat(func_name, icudata_version);
  syms[658] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_labelToUnicode");
  strcat(func_name, icudata_version);
  syms[659] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_nameToASCII");
  strcat(func_name, icudata_version);
  syms[660] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_nameToUnicode");
  strcat(func_name, icudata_version);
  syms[661] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_labelToASCII_UTF8");
  strcat(func_name, icudata_version);
  syms[662] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_labelToUnicodeUTF8");
  strcat(func_name, icudata_version);
  syms[663] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_nameToASCII_UTF8");
  strcat(func_name, icudata_version);
  syms[664] = dlsym(handle_common, func_name);

  strcpy(func_name, "uidna_nameToUnicodeUTF8");
  strcat(func_name, icudata_version);
  syms[665] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteBytes");
  strcat(func_name, icudata_version);
  syms[666] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteSub");
  strcat(func_name, icudata_version);
  syms[667] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbFromUWriteUChars");
  strcat(func_name, icudata_version);
  syms[668] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbToUWriteUChars");
  strcat(func_name, icudata_version);
  syms[669] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucnv_cbToUWriteSub");
  strcat(func_name, icudata_version);
  syms[670] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_open");
  strcat(func_name, icudata_version);
  syms[671] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openDirect");
  strcat(func_name, icudata_version);
  syms[672] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openU");
  strcat(func_name, icudata_version);
  syms[673] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_close");
  strcat(func_name, icudata_version);
  syms[674] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getVersion");
  strcat(func_name, icudata_version);
  syms[675] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[676] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getString");
  strcat(func_name, icudata_version);
  syms[677] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8String");
  strcat(func_name, icudata_version);
  syms[678] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getBinary");
  strcat(func_name, icudata_version);
  syms[679] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getIntVector");
  strcat(func_name, icudata_version);
  syms[680] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUInt");
  strcat(func_name, icudata_version);
  syms[681] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getInt");
  strcat(func_name, icudata_version);
  syms[682] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getSize");
  strcat(func_name, icudata_version);
  syms[683] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getType");
  strcat(func_name, icudata_version);
  syms[684] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getKey");
  strcat(func_name, icudata_version);
  syms[685] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_resetIterator");
  strcat(func_name, icudata_version);
  syms[686] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_hasNext");
  strcat(func_name, icudata_version);
  syms[687] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getNextResource");
  strcat(func_name, icudata_version);
  syms[688] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getNextString");
  strcat(func_name, icudata_version);
  syms[689] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getByIndex");
  strcat(func_name, icudata_version);
  syms[690] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getStringByIndex");
  strcat(func_name, icudata_version);
  syms[691] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8StringByIndex");
  strcat(func_name, icudata_version);
  syms[692] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getByKey");
  strcat(func_name, icudata_version);
  syms[693] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getStringByKey");
  strcat(func_name, icudata_version);
  syms[694] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_getUTF8StringByKey");
  strcat(func_name, icudata_version);
  syms[695] = dlsym(handle_common, func_name);

  strcpy(func_name, "ures_openAvailableLocales");
  strcat(func_name, icudata_version);
  syms[696] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getCode");
  strcat(func_name, icudata_version);
  syms[697] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getName");
  strcat(func_name, icudata_version);
  syms[698] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getShortName");
  strcat(func_name, icudata_version);
  syms[699] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getScript");
  strcat(func_name, icudata_version);
  syms[700] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_hasScript");
  strcat(func_name, icudata_version);
  syms[701] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getScriptExtensions");
  strcat(func_name, icudata_version);
  syms[702] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getSampleString");
  strcat(func_name, icudata_version);
  syms[703] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_getUsage");
  strcat(func_name, icudata_version);
  syms[704] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_isRightToLeft");
  strcat(func_name, icudata_version);
  syms[705] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_breaksBetweenLetters");
  strcat(func_name, icudata_version);
  syms[706] = dlsym(handle_common, func_name);

  strcpy(func_name, "uscript_isCased");
  strcat(func_name, icudata_version);
  syms[707] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strlen");
  strcat(func_name, icudata_version);
  syms[708] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_countChar32");
  strcat(func_name, icudata_version);
  syms[709] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strHasMoreChar32Than");
  strcat(func_name, icudata_version);
  syms[710] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcat");
  strcat(func_name, icudata_version);
  syms[711] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncat");
  strcat(func_name, icudata_version);
  syms[712] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strstr");
  strcat(func_name, icudata_version);
  syms[713] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFindFirst");
  strcat(func_name, icudata_version);
  syms[714] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strchr");
  strcat(func_name, icudata_version);
  syms[715] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strchr32");
  strcat(func_name, icudata_version);
  syms[716] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrstr");
  strcat(func_name, icudata_version);
  syms[717] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFindLast");
  strcat(func_name, icudata_version);
  syms[718] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrchr");
  strcat(func_name, icudata_version);
  syms[719] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strrchr32");
  strcat(func_name, icudata_version);
  syms[720] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strpbrk");
  strcat(func_name, icudata_version);
  syms[721] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcspn");
  strcat(func_name, icudata_version);
  syms[722] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strspn");
  strcat(func_name, icudata_version);
  syms[723] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strtok_r");
  strcat(func_name, icudata_version);
  syms[724] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcmp");
  strcat(func_name, icudata_version);
  syms[725] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[726] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCompare");
  strcat(func_name, icudata_version);
  syms[727] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCompareIter");
  strcat(func_name, icudata_version);
  syms[728] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strCaseCompare");
  strcat(func_name, icudata_version);
  syms[729] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncmp");
  strcat(func_name, icudata_version);
  syms[730] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[731] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcasecmp");
  strcat(func_name, icudata_version);
  syms[732] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncasecmp");
  strcat(func_name, icudata_version);
  syms[733] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcasecmp");
  strcat(func_name, icudata_version);
  syms[734] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strcpy");
  strcat(func_name, icudata_version);
  syms[735] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strncpy");
  strcat(func_name, icudata_version);
  syms[736] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_uastrcpy");
  strcat(func_name, icudata_version);
  syms[737] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_uastrncpy");
  strcat(func_name, icudata_version);
  syms[738] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_austrcpy");
  strcat(func_name, icudata_version);
  syms[739] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_austrncpy");
  strcat(func_name, icudata_version);
  syms[740] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcpy");
  strcat(func_name, icudata_version);
  syms[741] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memmove");
  strcat(func_name, icudata_version);
  syms[742] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memset");
  strcat(func_name, icudata_version);
  syms[743] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcmp");
  strcat(func_name, icudata_version);
  syms[744] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memcmpCodePointOrder");
  strcat(func_name, icudata_version);
  syms[745] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memchr");
  strcat(func_name, icudata_version);
  syms[746] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memchr32");
  strcat(func_name, icudata_version);
  syms[747] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memrchr");
  strcat(func_name, icudata_version);
  syms[748] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_memrchr32");
  strcat(func_name, icudata_version);
  syms[749] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_unescape");
  strcat(func_name, icudata_version);
  syms[750] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_unescapeAt");
  strcat(func_name, icudata_version);
  syms[751] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUpper");
  strcat(func_name, icudata_version);
  syms[752] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToLower");
  strcat(func_name, icudata_version);
  syms[753] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToTitle");
  strcat(func_name, icudata_version);
  syms[754] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFoldCase");
  strcat(func_name, icudata_version);
  syms[755] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToWCS");
  strcat(func_name, icudata_version);
  syms[756] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromWCS");
  strcat(func_name, icudata_version);
  syms[757] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF8");
  strcat(func_name, icudata_version);
  syms[758] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8");
  strcat(func_name, icudata_version);
  syms[759] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF8WithSub");
  strcat(func_name, icudata_version);
  syms[760] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8WithSub");
  strcat(func_name, icudata_version);
  syms[761] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF8Lenient");
  strcat(func_name, icudata_version);
  syms[762] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF32");
  strcat(func_name, icudata_version);
  syms[763] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF32");
  strcat(func_name, icudata_version);
  syms[764] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToUTF32WithSub");
  strcat(func_name, icudata_version);
  syms[765] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromUTF32WithSub");
  strcat(func_name, icudata_version);
  syms[766] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strToJavaModifiedUTF8");
  strcat(func_name, icudata_version);
  syms[767] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_strFromJavaModifiedUTF8WithSub");
  strcat(func_name, icudata_version);
  syms[768] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_close");
  strcat(func_name, icudata_version);
  syms[769] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_openUTF8");
  strcat(func_name, icudata_version);
  syms[770] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_openUChars");
  strcat(func_name, icudata_version);
  syms[771] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_clone");
  strcat(func_name, icudata_version);
  syms[772] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_equals");
  strcat(func_name, icudata_version);
  syms[773] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_nativeLength");
  strcat(func_name, icudata_version);
  syms[774] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_isLengthExpensive");
  strcat(func_name, icudata_version);
  syms[775] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_char32At");
  strcat(func_name, icudata_version);
  syms[776] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_current32");
  strcat(func_name, icudata_version);
  syms[777] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_next32");
  strcat(func_name, icudata_version);
  syms[778] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_previous32");
  strcat(func_name, icudata_version);
  syms[779] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_next32From");
  strcat(func_name, icudata_version);
  syms[780] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_previous32From");
  strcat(func_name, icudata_version);
  syms[781] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_getNativeIndex");
  strcat(func_name, icudata_version);
  syms[782] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_setNativeIndex");
  strcat(func_name, icudata_version);
  syms[783] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_moveIndex32");
  strcat(func_name, icudata_version);
  syms[784] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_getPreviousNativeIndex");
  strcat(func_name, icudata_version);
  syms[785] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_extract");
  strcat(func_name, icudata_version);
  syms[786] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_isWritable");
  strcat(func_name, icudata_version);
  syms[787] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_hasMetaData");
  strcat(func_name, icudata_version);
  syms[788] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_replace");
  strcat(func_name, icudata_version);
  syms[789] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_copy");
  strcat(func_name, icudata_version);
  syms[790] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_freeze");
  strcat(func_name, icudata_version);
  syms[791] = dlsym(handle_common, func_name);

  strcpy(func_name, "utext_setup");
  strcat(func_name, icudata_version);
  syms[792] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catopen");
  strcat(func_name, icudata_version);
  syms[793] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catclose");
  strcat(func_name, icudata_version);
  syms[794] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_catgets");
  strcat(func_name, icudata_version);
  syms[795] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_open");
  strcat(func_name, icudata_version);
  syms[796] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_openSized");
  strcat(func_name, icudata_version);
  syms[797] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_close");
  strcat(func_name, icudata_version);
  syms[798] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setInverse");
  strcat(func_name, icudata_version);
  syms[799] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_isInverse");
  strcat(func_name, icudata_version);
  syms[800] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_orderParagraphsLTR");
  strcat(func_name, icudata_version);
  syms[801] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_isOrderParagraphsLTR");
  strcat(func_name, icudata_version);
  syms[802] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setReorderingMode");
  strcat(func_name, icudata_version);
  syms[803] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getReorderingMode");
  strcat(func_name, icudata_version);
  syms[804] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setReorderingOptions");
  strcat(func_name, icudata_version);
  syms[805] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getReorderingOptions");
  strcat(func_name, icudata_version);
  syms[806] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setContext");
  strcat(func_name, icudata_version);
  syms[807] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setPara");
  strcat(func_name, icudata_version);
  syms[808] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setLine");
  strcat(func_name, icudata_version);
  syms[809] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getDirection");
  strcat(func_name, icudata_version);
  syms[810] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getBaseDirection");
  strcat(func_name, icudata_version);
  syms[811] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getText");
  strcat(func_name, icudata_version);
  syms[812] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLength");
  strcat(func_name, icudata_version);
  syms[813] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParaLevel");
  strcat(func_name, icudata_version);
  syms[814] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_countParagraphs");
  strcat(func_name, icudata_version);
  syms[815] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParagraph");
  strcat(func_name, icudata_version);
  syms[816] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getParagraphByIndex");
  strcat(func_name, icudata_version);
  syms[817] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLevelAt");
  strcat(func_name, icudata_version);
  syms[818] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLevels");
  strcat(func_name, icudata_version);
  syms[819] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalRun");
  strcat(func_name, icudata_version);
  syms[820] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_countRuns");
  strcat(func_name, icudata_version);
  syms[821] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualRun");
  strcat(func_name, icudata_version);
  syms[822] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualIndex");
  strcat(func_name, icudata_version);
  syms[823] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalIndex");
  strcat(func_name, icudata_version);
  syms[824] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getLogicalMap");
  strcat(func_name, icudata_version);
  syms[825] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getVisualMap");
  strcat(func_name, icudata_version);
  syms[826] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_reorderLogical");
  strcat(func_name, icudata_version);
  syms[827] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_reorderVisual");
  strcat(func_name, icudata_version);
  syms[828] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_invertMap");
  strcat(func_name, icudata_version);
  syms[829] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getProcessedLength");
  strcat(func_name, icudata_version);
  syms[830] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getResultLength");
  strcat(func_name, icudata_version);
  syms[831] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getCustomizedClass");
  strcat(func_name, icudata_version);
  syms[832] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_setClassCallback");
  strcat(func_name, icudata_version);
  syms[833] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_getClassCallback");
  strcat(func_name, icudata_version);
  syms[834] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_writeReordered");
  strcat(func_name, icudata_version);
  syms[835] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubidi_writeReverse");
  strcat(func_name, icudata_version);
  syms[836] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_open");
  strcat(func_name, icudata_version);
  syms[837] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_openChoice");
  strcat(func_name, icudata_version);
  syms[838] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_close");
  strcat(func_name, icudata_version);
  syms[839] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_getMemory");
  strcat(func_name, icudata_version);
  syms[840] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_getInfo");
  strcat(func_name, icudata_version);
  syms[841] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setCommonData");
  strcat(func_name, icudata_version);
  syms[842] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setAppData");
  strcat(func_name, icudata_version);
  syms[843] = dlsym(handle_common, func_name);

  strcpy(func_name, "udata_setFileAccess");
  strcat(func_name, icudata_version);
  syms[844] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_open");
  strcat(func_name, icudata_version);
  syms[845] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_close");
  strcat(func_name, icudata_version);
  syms[846] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getLocale");
  strcat(func_name, icudata_version);
  syms[847] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getOptions");
  strcat(func_name, icudata_version);
  syms[848] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setLocale");
  strcat(func_name, icudata_version);
  syms[849] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setOptions");
  strcat(func_name, icudata_version);
  syms[850] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_getBreakIterator");
  strcat(func_name, icudata_version);
  syms[851] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_setBreakIterator");
  strcat(func_name, icudata_version);
  syms[852] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_toTitle");
  strcat(func_name, icudata_version);
  syms[853] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToLower");
  strcat(func_name, icudata_version);
  syms[854] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToUpper");
  strcat(func_name, icudata_version);
  syms[855] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8ToTitle");
  strcat(func_name, icudata_version);
  syms[856] = dlsym(handle_common, func_name);

  strcpy(func_name, "ucasemap_utf8FoldCase");
  strcat(func_name, icudata_version);
  syms[857] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionFromString");
  strcat(func_name, icudata_version);
  syms[858] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionFromUString");
  strcat(func_name, icudata_version);
  syms[859] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_versionToString");
  strcat(func_name, icudata_version);
  syms[860] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getVersion");
  strcat(func_name, icudata_version);
  syms[861] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getNFCInstance");
  strcat(func_name, icudata_version);
  syms[862] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getNFDInstance");
  strcat(func_name, icudata_version);
  syms[863] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getNFKCInstance");
  strcat(func_name, icudata_version);
  syms[864] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getNFKDInstance");
  strcat(func_name, icudata_version);
  syms[865] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getNFKCCasefoldInstance");
  strcat(func_name, icudata_version);
  syms[866] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getInstance");
  strcat(func_name, icudata_version);
  syms[867] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_openFiltered");
  strcat(func_name, icudata_version);
  syms[868] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_close");
  strcat(func_name, icudata_version);
  syms[869] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_normalize");
  strcat(func_name, icudata_version);
  syms[870] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_normalizeSecondAndAppend");
  strcat(func_name, icudata_version);
  syms[871] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_append");
  strcat(func_name, icudata_version);
  syms[872] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getDecomposition");
  strcat(func_name, icudata_version);
  syms[873] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getRawDecomposition");
  strcat(func_name, icudata_version);
  syms[874] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_composePair");
  strcat(func_name, icudata_version);
  syms[875] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_getCombiningClass");
  strcat(func_name, icudata_version);
  syms[876] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_isNormalized");
  strcat(func_name, icudata_version);
  syms[877] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_quickCheck");
  strcat(func_name, icudata_version);
  syms[878] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_spanQuickCheckYes");
  strcat(func_name, icudata_version);
  syms[879] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_hasBoundaryBefore");
  strcat(func_name, icudata_version);
  syms[880] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_hasBoundaryAfter");
  strcat(func_name, icudata_version);
  syms[881] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm2_isInert");
  strcat(func_name, icudata_version);
  syms[882] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getDataVersion");
  strcat(func_name, icudata_version);
  syms[883] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_normalize");
  strcat(func_name, icudata_version);
  syms[884] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_quickCheck");
  strcat(func_name, icudata_version);
  syms[885] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_quickCheckWithOptions");
  strcat(func_name, icudata_version);
  syms[886] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_isNormalized");
  strcat(func_name, icudata_version);
  syms[887] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_isNormalizedWithOptions");
  strcat(func_name, icudata_version);
  syms[888] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_next");
  strcat(func_name, icudata_version);
  syms[889] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_previous");
  strcat(func_name, icudata_version);
  syms[890] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_concatenate");
  strcat(func_name, icudata_version);
  syms[891] = dlsym(handle_common, func_name);

  strcpy(func_name, "unorm_compare");
  strcat(func_name, icudata_version);
  syms[892] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_open");
  strcat(func_name, icudata_version);
  syms[893] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_openByType");
  strcat(func_name, icudata_version);
  syms[894] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_close");
  strcat(func_name, icudata_version);
  syms[895] = dlsym(handle_common, func_name);

  strcpy(func_name, "usprep_prepare");
  strcat(func_name, icudata_version);
  syms[896] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_shapeArabic");
  strcat(func_name, icudata_version);
  syms[897] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_current32");
  strcat(func_name, icudata_version);
  syms[898] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_next32");
  strcat(func_name, icudata_version);
  syms[899] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_previous32");
  strcat(func_name, icudata_version);
  syms[900] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_getState");
  strcat(func_name, icudata_version);
  syms[901] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setState");
  strcat(func_name, icudata_version);
  syms[902] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setString");
  strcat(func_name, icudata_version);
  syms[903] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setUTF16BE");
  strcat(func_name, icudata_version);
  syms[904] = dlsym(handle_common, func_name);

  strcpy(func_name, "uiter_setUTF8");
  strcat(func_name, icudata_version);
  syms[905] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_getDataDirectory");
  strcat(func_name, icudata_version);
  syms[906] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_setDataDirectory");
  strcat(func_name, icudata_version);
  syms[907] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_charsToUChars");
  strcat(func_name, icudata_version);
  syms[908] = dlsym(handle_common, func_name);

  strcpy(func_name, "u_UCharsToChars");
  strcat(func_name, icudata_version);
  syms[909] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_open");
  strcat(func_name, icudata_version);
  syms[910] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_openRules");
  strcat(func_name, icudata_version);
  syms[911] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_safeClone");
  strcat(func_name, icudata_version);
  syms[912] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_close");
  strcat(func_name, icudata_version);
  syms[913] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_setText");
  strcat(func_name, icudata_version);
  syms[914] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_setUText");
  strcat(func_name, icudata_version);
  syms[915] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_current");
  strcat(func_name, icudata_version);
  syms[916] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_next");
  strcat(func_name, icudata_version);
  syms[917] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_previous");
  strcat(func_name, icudata_version);
  syms[918] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_first");
  strcat(func_name, icudata_version);
  syms[919] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_last");
  strcat(func_name, icudata_version);
  syms[920] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_preceding");
  strcat(func_name, icudata_version);
  syms[921] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_following");
  strcat(func_name, icudata_version);
  syms[922] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getAvailable");
  strcat(func_name, icudata_version);
  syms[923] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_countAvailable");
  strcat(func_name, icudata_version);
  syms[924] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_isBoundary");
  strcat(func_name, icudata_version);
  syms[925] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getRuleStatus");
  strcat(func_name, icudata_version);
  syms[926] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getRuleStatusVec");
  strcat(func_name, icudata_version);
  syms[927] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_getLocaleByType");
  strcat(func_name, icudata_version);
  syms[928] = dlsym(handle_common, func_name);

  strcpy(func_name, "ubrk_refreshUText");
  strcat(func_name, icudata_version);
  syms[929] = dlsym(handle_common, func_name);

  strcpy(func_name, "utf8_nextCharSafeBody");
  strcat(func_name, icudata_version);
  syms[930] = dlsym(handle_common, func_name);

  strcpy(func_name, "utf8_appendCharSafeBody");
  strcat(func_name, icudata_version);
  syms[931] = dlsym(handle_common, func_name);

  strcpy(func_name, "utf8_prevCharSafeBody");
  strcat(func_name, icudata_version);
  syms[932] = dlsym(handle_common, func_name);

  strcpy(func_name, "utf8_back1SafeBody");
  strcat(func_name, icudata_version);
  syms[933] = dlsym(handle_common, func_name);

}

/* unicode/ucsdet.h */
UCharsetDetector* ucsdet_open(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCharsetDetector* (*ptr)(UErrorCode*);
  if (syms[0] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCharsetDetector*)0;
  }
  ptr = (UCharsetDetector*(*)(UErrorCode*))syms[0];
  return ptr(status);
}

void ucsdet_close(UCharsetDetector* ucsd) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*);
  ptr = (void(*)(UCharsetDetector*))syms[1];
  ptr(ucsd);
  return;
}

void ucsdet_setText(UCharsetDetector* ucsd, const char* textIn, int32_t len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  if (syms[2] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))syms[2];
  ptr(ucsd, textIn, len, status);
  return;
}

void ucsdet_setDeclaredEncoding(UCharsetDetector* ucsd, const char* encoding, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharsetDetector*, const char*, int32_t, UErrorCode*);
  if (syms[3] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCharsetDetector*, const char*, int32_t, UErrorCode*))syms[3];
  ptr(ucsd, encoding, length, status);
  return;
}

const UCharsetMatch* ucsdet_detect(UCharsetDetector* ucsd, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UCharsetMatch* (*ptr)(UCharsetDetector*, UErrorCode*);
  if (syms[4] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UCharsetMatch*)0;
  }
  ptr = (const UCharsetMatch*(*)(UCharsetDetector*, UErrorCode*))syms[4];
  return ptr(ucsd, status);
}

const UCharsetMatch** ucsdet_detectAll(UCharsetDetector* ucsd, int32_t* matchesFound, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UCharsetMatch** (*ptr)(UCharsetDetector*, int32_t*, UErrorCode*);
  if (syms[5] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UCharsetMatch**)0;
  }
  ptr = (const UCharsetMatch**(*)(UCharsetDetector*, int32_t*, UErrorCode*))syms[5];
  return ptr(ucsd, matchesFound, status);
}

const char* ucsdet_getName(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCharsetMatch*, UErrorCode*);
  if (syms[6] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UCharsetMatch*, UErrorCode*))syms[6];
  return ptr(ucsm, status);
}

int32_t ucsdet_getConfidence(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCharsetMatch*, UErrorCode*);
  if (syms[7] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCharsetMatch*, UErrorCode*))syms[7];
  return ptr(ucsm, status);
}

const char* ucsdet_getLanguage(const UCharsetMatch* ucsm, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCharsetMatch*, UErrorCode*);
  if (syms[8] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UCharsetMatch*, UErrorCode*))syms[8];
  return ptr(ucsm, status);
}

int32_t ucsdet_getUChars(const UCharsetMatch* ucsm, UChar* buf, int32_t cap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*);
  if (syms[9] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCharsetMatch*, UChar*, int32_t, UErrorCode*))syms[9];
  return ptr(ucsm, buf, cap, status);
}

UEnumeration* ucsdet_getAllDetectableCharsets(const UCharsetDetector* ucsd, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UCharsetDetector*, UErrorCode*);
  if (syms[10] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UCharsetDetector*, UErrorCode*))syms[10];
  return ptr(ucsd, status);
}

UBool ucsdet_isInputFilterEnabled(const UCharsetDetector* ucsd) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCharsetDetector*);
  ptr = (UBool(*)(const UCharsetDetector*))syms[11];
  return ptr(ucsd);
}

UBool ucsdet_enableInputFilter(UCharsetDetector* ucsd, UBool filter) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UCharsetDetector*, UBool);
  ptr = (UBool(*)(UCharsetDetector*, UBool))syms[12];
  return ptr(ucsd, filter);
}

/* unicode/udateintervalformat.h */
UDateIntervalFormat* udtitvfmt_open(const char* locale, const UChar* skeleton, int32_t skeletonLength, const UChar* tzID, int32_t tzIDLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDateIntervalFormat* (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[13] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDateIntervalFormat*)0;
  }
  ptr = (UDateIntervalFormat*(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[13];
  return ptr(locale, skeleton, skeletonLength, tzID, tzIDLength, status);
}

void udtitvfmt_close(UDateIntervalFormat* formatter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateIntervalFormat*);
  ptr = (void(*)(UDateIntervalFormat*))syms[14];
  ptr(formatter);
  return;
}

int32_t udtitvfmt_format(const UDateIntervalFormat* formatter, UDate fromDate, UDate toDate, UChar* result, int32_t resultCapacity, UFieldPosition* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateIntervalFormat*, UDate, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[15] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UDateIntervalFormat*, UDate, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[15];
  return ptr(formatter, fromDate, toDate, result, resultCapacity, position, status);
}

/* unicode/udatpg.h */
UDateTimePatternGenerator* udatpg_open(const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator* (*ptr)(const char*, UErrorCode*);
  if (syms[16] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDateTimePatternGenerator*)0;
  }
  ptr = (UDateTimePatternGenerator*(*)(const char*, UErrorCode*))syms[16];
  return ptr(locale, pErrorCode);
}

UDateTimePatternGenerator* udatpg_openEmpty(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator* (*ptr)(UErrorCode*);
  if (syms[17] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDateTimePatternGenerator*)0;
  }
  ptr = (UDateTimePatternGenerator*(*)(UErrorCode*))syms[17];
  return ptr(pErrorCode);
}

void udatpg_close(UDateTimePatternGenerator* dtpg) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*);
  ptr = (void(*)(UDateTimePatternGenerator*))syms[18];
  ptr(dtpg);
  return;
}

UDateTimePatternGenerator* udatpg_clone(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternGenerator* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  if (syms[19] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDateTimePatternGenerator*)0;
  }
  ptr = (UDateTimePatternGenerator*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[19];
  return ptr(dtpg, pErrorCode);
}

int32_t udatpg_getBestPattern(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[20] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[20];
  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}

int32_t udatpg_getBestPatternWithOptions(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UDateTimePatternMatchOptions options, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UDateTimePatternMatchOptions, UChar*, int32_t, UErrorCode*);
  if (syms[21] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UDateTimePatternMatchOptions, UChar*, int32_t, UErrorCode*))syms[21];
  return ptr(dtpg, skeleton, length, options, bestPattern, capacity, pErrorCode);
}

int32_t udatpg_getSkeleton(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* skeleton, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[22] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[22];
  return ptr(dtpg, pattern, length, skeleton, capacity, pErrorCode);
}

int32_t udatpg_getBaseSkeleton(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t length, UChar* baseSkeleton, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[23] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[23];
  return ptr(dtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}

UDateTimePatternConflict udatpg_addPattern(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, UBool override, UChar* conflictingPattern, int32_t capacity, int32_t* pLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDateTimePatternConflict (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[24] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDateTimePatternConflict)0;
  }
  ptr = (UDateTimePatternConflict(*)(UDateTimePatternGenerator*, const UChar*, int32_t, UBool, UChar*, int32_t, int32_t*, UErrorCode*))syms[24];
  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}

void udatpg_setAppendItemFormat(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))syms[25];
  ptr(dtpg, field, value, length);
  return;
}

const UChar* udatpg_getAppendItemFormat(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))syms[26];
  return ptr(dtpg, field, pLength);
}

void udatpg_setAppendItemName(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, UDateTimePatternField, const UChar*, int32_t))syms[27];
  ptr(dtpg, field, value, length);
  return;
}

const UChar* udatpg_getAppendItemName(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, UDateTimePatternField, int32_t*))syms[28];
  return ptr(dtpg, field, pLength);
}

void udatpg_setDateTimeFormat(const UDateTimePatternGenerator* dtpg, const UChar* dtFormat, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t);
  ptr = (void(*)(const UDateTimePatternGenerator*, const UChar*, int32_t))syms[29];
  ptr(dtpg, dtFormat, length);
  return;
}

const UChar* udatpg_getDateTimeFormat(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, int32_t*))syms[30];
  return ptr(dtpg, pLength);
}

void udatpg_setDecimal(UDateTimePatternGenerator* dtpg, const UChar* decimal, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t);
  ptr = (void(*)(UDateTimePatternGenerator*, const UChar*, int32_t))syms[31];
  ptr(dtpg, decimal, length);
  return;
}

const UChar* udatpg_getDecimal(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UDateTimePatternGenerator*, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, int32_t*))syms[32];
  return ptr(dtpg, pLength);
}

int32_t udatpg_replaceFieldTypes(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[33] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[33];
  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}

int32_t udatpg_replaceFieldTypesWithOptions(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UDateTimePatternMatchOptions options, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UDateTimePatternMatchOptions, UChar*, int32_t, UErrorCode*);
  if (syms[34] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UDateTimePatternGenerator*, const UChar*, int32_t, const UChar*, int32_t, UDateTimePatternMatchOptions, UChar*, int32_t, UErrorCode*))syms[34];
  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, options, dest, destCapacity, pErrorCode);
}

UEnumeration* udatpg_openSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  if (syms[35] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[35];
  return ptr(dtpg, pErrorCode);
}

UEnumeration* udatpg_openBaseSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UDateTimePatternGenerator*, UErrorCode*);
  if (syms[36] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UDateTimePatternGenerator*, UErrorCode*))syms[36];
  return ptr(dtpg, pErrorCode);
}

const UChar* udatpg_getPatternForSkeleton(const UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t skeletonLength, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*);
  ptr = (const UChar*(*)(const UDateTimePatternGenerator*, const UChar*, int32_t, int32_t*))syms[37];
  return ptr(dtpg, skeleton, skeletonLength, pLength);
}

/* unicode/udat.h */
UCalendarDateFields udat_toCalendarDateField(UDateFormatField field) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendarDateFields (*ptr)(UDateFormatField);
  ptr = (UCalendarDateFields(*)(UDateFormatField))syms[38];
  return ptr(field);
}

UDateFormat* udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char* locale, const UChar* tzID, int32_t tzIDLength, const UChar* pattern, int32_t patternLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDateFormat* (*ptr)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[39] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDateFormat*)0;
  }
  ptr = (UDateFormat*(*)(UDateFormatStyle, UDateFormatStyle, const char*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[39];
  return ptr(timeStyle, dateStyle, locale, tzID, tzIDLength, pattern, patternLength, status);
}

void udat_close(UDateFormat* format) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*);
  ptr = (void(*)(UDateFormat*))syms[40];
  ptr(format);
  return;
}

UBool udat_getBooleanAttribute(const UDateFormat* fmt, UDateFormatBooleanAttribute attr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UDateFormat*, UDateFormatBooleanAttribute, UErrorCode*);
  if (syms[41] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UDateFormat*, UDateFormatBooleanAttribute, UErrorCode*))syms[41];
  return ptr(fmt, attr, status);
}

void udat_setBooleanAttribute(UDateFormat* fmt, UDateFormatBooleanAttribute attr, UBool newValue, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UDateFormatBooleanAttribute, UBool, UErrorCode*);
  if (syms[42] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UDateFormat*, UDateFormatBooleanAttribute, UBool, UErrorCode*))syms[42];
  ptr(fmt, attr, newValue, status);
  return;
}

UDateFormat* udat_clone(const UDateFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDateFormat* (*ptr)(const UDateFormat*, UErrorCode*);
  if (syms[43] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDateFormat*)0;
  }
  ptr = (UDateFormat*(*)(const UDateFormat*, UErrorCode*))syms[43];
  return ptr(fmt, status);
}

int32_t udat_format(const UDateFormat* format, UDate dateToFormat, UChar* result, int32_t resultLength, UFieldPosition* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[44] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UDateFormat*, UDate, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[44];
  return ptr(format, dateToFormat, result, resultLength, position, status);
}

UDate udat_parse(const UDateFormat* format, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[45] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDate)0;
  }
  ptr = (UDate(*)(const UDateFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[45];
  return ptr(format, text, textLength, parsePos, status);
}

void udat_parseCalendar(const UDateFormat* format, UCalendar* calendar, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[46] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UDateFormat*, UCalendar*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[46];
  ptr(format, calendar, text, textLength, parsePos, status);
  return;
}

UBool udat_isLenient(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UDateFormat*);
  ptr = (UBool(*)(const UDateFormat*))syms[47];
  return ptr(fmt);
}

void udat_setLenient(UDateFormat* fmt, UBool isLenient) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UBool);
  ptr = (void(*)(UDateFormat*, UBool))syms[48];
  ptr(fmt, isLenient);
  return;
}

const UCalendar* udat_getCalendar(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const UCalendar* (*ptr)(const UDateFormat*);
  ptr = (const UCalendar*(*)(const UDateFormat*))syms[49];
  return ptr(fmt);
}

void udat_setCalendar(UDateFormat* fmt, const UCalendar* calendarToSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, const UCalendar*);
  ptr = (void(*)(UDateFormat*, const UCalendar*))syms[50];
  ptr(fmt, calendarToSet);
  return;
}

const UNumberFormat* udat_getNumberFormat(const UDateFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const UNumberFormat* (*ptr)(const UDateFormat*);
  ptr = (const UNumberFormat*(*)(const UDateFormat*))syms[51];
  return ptr(fmt);
}

void udat_setNumberFormat(UDateFormat* fmt, const UNumberFormat* numberFormatToSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, const UNumberFormat*);
  ptr = (void(*)(UDateFormat*, const UNumberFormat*))syms[52];
  ptr(fmt, numberFormatToSet);
  return;
}

const char* udat_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[53];
  return ptr(localeIndex);
}

int32_t udat_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[54];
  return ptr();
}

UDate udat_get2DigitYearStart(const UDateFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UDateFormat*, UErrorCode*);
  if (syms[55] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDate)0;
  }
  ptr = (UDate(*)(const UDateFormat*, UErrorCode*))syms[55];
  return ptr(fmt, status);
}

void udat_set2DigitYearStart(UDateFormat* fmt, UDate d, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UDate, UErrorCode*);
  if (syms[56] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UDateFormat*, UDate, UErrorCode*))syms[56];
  ptr(fmt, d, status);
  return;
}

int32_t udat_toPattern(const UDateFormat* fmt, UBool localized, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*);
  if (syms[57] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UDateFormat*, UBool, UChar*, int32_t, UErrorCode*))syms[57];
  return ptr(fmt, localized, result, resultLength, status);
}

void udat_applyPattern(UDateFormat* format, UBool localized, const UChar* pattern, int32_t patternLength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UBool, const UChar*, int32_t);
  ptr = (void(*)(UDateFormat*, UBool, const UChar*, int32_t))syms[58];
  ptr(format, localized, pattern, patternLength);
  return;
}

int32_t udat_getSymbols(const UDateFormat* fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[59] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))syms[59];
  return ptr(fmt, type, symbolIndex, result, resultLength, status);
}

int32_t udat_countSymbols(const UDateFormat* fmt, UDateFormatSymbolType type) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UDateFormat*, UDateFormatSymbolType);
  ptr = (int32_t(*)(const UDateFormat*, UDateFormatSymbolType))syms[60];
  return ptr(fmt, type);
}

void udat_setSymbols(UDateFormat* format, UDateFormatSymbolType type, int32_t symbolIndex, UChar* value, int32_t valueLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[61] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UDateFormat*, UDateFormatSymbolType, int32_t, UChar*, int32_t, UErrorCode*))syms[61];
  ptr(format, type, symbolIndex, value, valueLength, status);
  return;
}

const char* udat_getLocaleByType(const UDateFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UDateFormat*, ULocDataLocaleType, UErrorCode*);
  if (syms[62] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UDateFormat*, ULocDataLocaleType, UErrorCode*))syms[62];
  return ptr(fmt, type, status);
}

UDisplayContext udat_getContext(const UDateFormat* fmt, UDisplayContextType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDisplayContext (*ptr)(const UDateFormat*, UDisplayContextType, UErrorCode*);
  if (syms[63] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDisplayContext)0;
  }
  ptr = (UDisplayContext(*)(const UDateFormat*, UDisplayContextType, UErrorCode*))syms[63];
  return ptr(fmt, type, status);
}

/* unicode/ucol.h */
UCollator* ucol_open(const char* loc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator* (*ptr)(const char*, UErrorCode*);
  if (syms[64] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollator*)0;
  }
  ptr = (UCollator*(*)(const char*, UErrorCode*))syms[64];
  return ptr(loc, status);
}

UCollator* ucol_openRules(const UChar* rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator* (*ptr)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*);
  if (syms[65] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollator*)0;
  }
  ptr = (UCollator*(*)(const UChar*, int32_t, UColAttributeValue, UCollationStrength, UParseError*, UErrorCode*))syms[65];
  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}

void ucol_getContractionsAndExpansions(const UCollator* coll, USet* contractions, USet* expansions, UBool addPrefixes, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, USet*, USet*, UBool, UErrorCode*);
  if (syms[66] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UCollator*, USet*, USet*, UBool, UErrorCode*))syms[66];
  ptr(coll, contractions, expansions, addPrefixes, status);
  return;
}

void ucol_close(UCollator* coll) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*);
  ptr = (void(*)(UCollator*))syms[67];
  ptr(coll);
  return;
}

UCollationResult ucol_strcoll(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationResult (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UCollationResult(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[68];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UCollationResult ucol_strcollUTF8(const UCollator* coll, const char* source, int32_t sourceLength, const char* target, int32_t targetLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationResult (*ptr)(const UCollator*, const char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[69] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollationResult)0;
  }
  ptr = (UCollationResult(*)(const UCollator*, const char*, int32_t, const char*, int32_t, UErrorCode*))syms[69];
  return ptr(coll, source, sourceLength, target, targetLength, status);
}

UBool ucol_greater(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[70];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_greaterOrEqual(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[71];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UBool ucol_equal(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UBool(*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t))syms[72];
  return ptr(coll, source, sourceLength, target, targetLength);
}

UCollationResult ucol_strcollIter(const UCollator* coll, UCharIterator* sIter, UCharIterator* tIter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationResult (*ptr)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*);
  if (syms[73] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollationResult)0;
  }
  ptr = (UCollationResult(*)(const UCollator*, UCharIterator*, UCharIterator*, UErrorCode*))syms[73];
  return ptr(coll, sIter, tIter, status);
}

UCollationStrength ucol_getStrength(const UCollator* coll) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationStrength (*ptr)(const UCollator*);
  ptr = (UCollationStrength(*)(const UCollator*))syms[74];
  return ptr(coll);
}

void ucol_setStrength(UCollator* coll, UCollationStrength strength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, UCollationStrength);
  ptr = (void(*)(UCollator*, UCollationStrength))syms[75];
  ptr(coll, strength);
  return;
}

int32_t ucol_getReorderCodes(const UCollator* coll, int32_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, int32_t*, int32_t, UErrorCode*);
  if (syms[76] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCollator*, int32_t*, int32_t, UErrorCode*))syms[76];
  return ptr(coll, dest, destCapacity, pErrorCode);
}

void ucol_setReorderCodes(UCollator* coll, const int32_t* reorderCodes, int32_t reorderCodesLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, const int32_t*, int32_t, UErrorCode*);
  if (syms[77] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCollator*, const int32_t*, int32_t, UErrorCode*))syms[77];
  ptr(coll, reorderCodes, reorderCodesLength, pErrorCode);
  return;
}

int32_t ucol_getEquivalentReorderCodes(int32_t reorderCode, int32_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t, int32_t*, int32_t, UErrorCode*);
  if (syms[78] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(int32_t, int32_t*, int32_t, UErrorCode*))syms[78];
  return ptr(reorderCode, dest, destCapacity, pErrorCode);
}

int32_t ucol_getDisplayName(const char* objLoc, const char* dispLoc, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[79] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[79];
  return ptr(objLoc, dispLoc, result, resultLength, status);
}

const char* ucol_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[80];
  return ptr(localeIndex);
}

int32_t ucol_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[81];
  return ptr();
}

UEnumeration* ucol_openAvailableLocales(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[82] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[82];
  return ptr(status);
}

UEnumeration* ucol_getKeywords(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[83] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[83];
  return ptr(status);
}

UEnumeration* ucol_getKeywordValues(const char* keyword, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  if (syms[84] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[84];
  return ptr(keyword, status);
}

UEnumeration* ucol_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  if (syms[85] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[85];
  return ptr(key, locale, commonlyUsed, status);
}

int32_t ucol_getFunctionalEquivalent(char* result, int32_t resultCapacity, const char* keyword, const char* locale, UBool* isAvailable, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*);
  if (syms[86] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(char*, int32_t, const char*, const char*, UBool*, UErrorCode*))syms[86];
  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}

const UChar* ucol_getRules(const UCollator* coll, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UCollator*, int32_t*);
  ptr = (const UChar*(*)(const UCollator*, int32_t*))syms[87];
  return ptr(coll, length);
}

int32_t ucol_getSortKey(const UCollator* coll, const UChar* source, int32_t sourceLength, uint8_t* result, int32_t resultLength) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t);
  ptr = (int32_t(*)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t))syms[88];
  return ptr(coll, source, sourceLength, result, resultLength);
}

int32_t ucol_nextSortKeyPart(const UCollator* coll, UCharIterator* iter, uint32_t state [ 2], uint8_t* dest, int32_t count, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*);
  if (syms[89] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCollator*, UCharIterator*, uint32_t [ 2], uint8_t*, int32_t, UErrorCode*))syms[89];
  return ptr(coll, iter, state, dest, count, status);
}

int32_t ucol_getBound(const uint8_t* source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*);
  if (syms[90] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const uint8_t*, int32_t, UColBoundMode, uint32_t, uint8_t*, int32_t, UErrorCode*))syms[90];
  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}

void ucol_getVersion(const UCollator* coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, UVersionInfo);
  ptr = (void(*)(const UCollator*, UVersionInfo))syms[91];
  ptr(coll, info);
  return;
}

void ucol_getUCAVersion(const UCollator* coll, UVersionInfo info) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UCollator*, UVersionInfo);
  ptr = (void(*)(const UCollator*, UVersionInfo))syms[92];
  ptr(coll, info);
  return;
}

int32_t ucol_mergeSortkeys(const uint8_t* src1, int32_t src1Length, const uint8_t* src2, int32_t src2Length, uint8_t* dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t);
  ptr = (int32_t(*)(const uint8_t*, int32_t, const uint8_t*, int32_t, uint8_t*, int32_t))syms[93];
  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}

void ucol_setAttribute(UCollator* coll, UColAttribute attr, UColAttributeValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*);
  if (syms[94] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCollator*, UColAttribute, UColAttributeValue, UErrorCode*))syms[94];
  ptr(coll, attr, value, status);
  return;
}

UColAttributeValue ucol_getAttribute(const UCollator* coll, UColAttribute attr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UColAttributeValue (*ptr)(const UCollator*, UColAttribute, UErrorCode*);
  if (syms[95] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UColAttributeValue)0;
  }
  ptr = (UColAttributeValue(*)(const UCollator*, UColAttribute, UErrorCode*))syms[95];
  return ptr(coll, attr, status);
}

void ucol_setMaxVariable(UCollator* coll, UColReorderCode group, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollator*, UColReorderCode, UErrorCode*);
  if (syms[96] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCollator*, UColReorderCode, UErrorCode*))syms[96];
  ptr(coll, group, pErrorCode);
  return;
}

UColReorderCode ucol_getMaxVariable(const UCollator* coll) {
  pthread_once(&once_control, &init_icudata_version);
  UColReorderCode (*ptr)(const UCollator*);
  ptr = (UColReorderCode(*)(const UCollator*))syms[97];
  return ptr(coll);
}

uint32_t ucol_getVariableTop(const UCollator* coll, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCollator*, UErrorCode*);
  if (syms[98] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (uint32_t)0;
  }
  ptr = (uint32_t(*)(const UCollator*, UErrorCode*))syms[98];
  return ptr(coll, status);
}

UCollator* ucol_safeClone(const UCollator* coll, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator* (*ptr)(const UCollator*, void*, int32_t*, UErrorCode*);
  if (syms[99] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollator*)0;
  }
  ptr = (UCollator*(*)(const UCollator*, void*, int32_t*, UErrorCode*))syms[99];
  return ptr(coll, stackBuffer, pBufferSize, status);
}

int32_t ucol_getRulesEx(const UCollator* coll, UColRuleOption delta, UChar* buffer, int32_t bufferLen) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, UColRuleOption, UChar*, int32_t);
  ptr = (int32_t(*)(const UCollator*, UColRuleOption, UChar*, int32_t))syms[100];
  return ptr(coll, delta, buffer, bufferLen);
}

const char* ucol_getLocaleByType(const UCollator* coll, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCollator*, ULocDataLocaleType, UErrorCode*);
  if (syms[101] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UCollator*, ULocDataLocaleType, UErrorCode*))syms[101];
  return ptr(coll, type, status);
}

USet* ucol_getTailoredSet(const UCollator* coll, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const UCollator*, UErrorCode*);
  if (syms[102] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USet*)0;
  }
  ptr = (USet*(*)(const UCollator*, UErrorCode*))syms[102];
  return ptr(coll, status);
}

int32_t ucol_cloneBinary(const UCollator* coll, uint8_t* buffer, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollator*, uint8_t*, int32_t, UErrorCode*);
  if (syms[103] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCollator*, uint8_t*, int32_t, UErrorCode*))syms[103];
  return ptr(coll, buffer, capacity, status);
}

UCollator* ucol_openBinary(const uint8_t* bin, int32_t length, const UCollator* base, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator* (*ptr)(const uint8_t*, int32_t, const UCollator*, UErrorCode*);
  if (syms[104] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollator*)0;
  }
  ptr = (UCollator*(*)(const uint8_t*, int32_t, const UCollator*, UErrorCode*))syms[104];
  return ptr(bin, length, base, status);
}

/* unicode/ugender.h */
const UGenderInfo* ugender_getInstance(const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UGenderInfo* (*ptr)(const char*, UErrorCode*);
  if (syms[105] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UGenderInfo*)0;
  }
  ptr = (const UGenderInfo*(*)(const char*, UErrorCode*))syms[105];
  return ptr(locale, status);
}

UGender ugender_getListGender(const UGenderInfo* genderinfo, const UGender* genders, int32_t size, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UGender (*ptr)(const UGenderInfo*, const UGender*, int32_t, UErrorCode*);
  if (syms[106] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UGender)0;
  }
  ptr = (UGender(*)(const UGenderInfo*, const UGender*, int32_t, UErrorCode*))syms[106];
  return ptr(genderinfo, genders, size, status);
}

/* unicode/uspoof.h */
USpoofChecker* uspoof_open(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker* (*ptr)(UErrorCode*);
  if (syms[107] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USpoofChecker*)0;
  }
  ptr = (USpoofChecker*(*)(UErrorCode*))syms[107];
  return ptr(status);
}

USpoofChecker* uspoof_openFromSerialized(const void* data, int32_t length, int32_t* pActualLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker* (*ptr)(const void*, int32_t, int32_t*, UErrorCode*);
  if (syms[108] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (USpoofChecker*)0;
  }
  ptr = (USpoofChecker*(*)(const void*, int32_t, int32_t*, UErrorCode*))syms[108];
  return ptr(data, length, pActualLength, pErrorCode);
}

USpoofChecker* uspoof_openFromSource(const char* confusables, int32_t confusablesLen, const char* confusablesWholeScript, int32_t confusablesWholeScriptLen, int32_t* errType, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker* (*ptr)(const char*, int32_t, const char*, int32_t, int32_t*, UParseError*, UErrorCode*);
  if (syms[109] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USpoofChecker*)0;
  }
  ptr = (USpoofChecker*(*)(const char*, int32_t, const char*, int32_t, int32_t*, UParseError*, UErrorCode*))syms[109];
  return ptr(confusables, confusablesLen, confusablesWholeScript, confusablesWholeScriptLen, errType, pe, status);
}

void uspoof_close(USpoofChecker* sc) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*);
  ptr = (void(*)(USpoofChecker*))syms[110];
  ptr(sc);
  return;
}

USpoofChecker* uspoof_clone(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USpoofChecker* (*ptr)(const USpoofChecker*, UErrorCode*);
  if (syms[111] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USpoofChecker*)0;
  }
  ptr = (USpoofChecker*(*)(const USpoofChecker*, UErrorCode*))syms[111];
  return ptr(sc, status);
}

void uspoof_setChecks(USpoofChecker* sc, int32_t checks, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, int32_t, UErrorCode*);
  if (syms[112] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(USpoofChecker*, int32_t, UErrorCode*))syms[112];
  ptr(sc, checks, status);
  return;
}

int32_t uspoof_getChecks(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, UErrorCode*);
  if (syms[113] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, UErrorCode*))syms[113];
  return ptr(sc, status);
}

void uspoof_setRestrictionLevel(USpoofChecker* sc, URestrictionLevel restrictionLevel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, URestrictionLevel);
  ptr = (void(*)(USpoofChecker*, URestrictionLevel))syms[114];
  ptr(sc, restrictionLevel);
  return;
}

URestrictionLevel uspoof_getRestrictionLevel(const USpoofChecker* sc) {
  pthread_once(&once_control, &init_icudata_version);
  URestrictionLevel (*ptr)(const USpoofChecker*);
  ptr = (URestrictionLevel(*)(const USpoofChecker*))syms[115];
  return ptr(sc);
}

void uspoof_setAllowedLocales(USpoofChecker* sc, const char* localesList, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, const char*, UErrorCode*);
  if (syms[116] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(USpoofChecker*, const char*, UErrorCode*))syms[116];
  ptr(sc, localesList, status);
  return;
}

const char* uspoof_getAllowedLocales(USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(USpoofChecker*, UErrorCode*);
  if (syms[117] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(USpoofChecker*, UErrorCode*))syms[117];
  return ptr(sc, status);
}

void uspoof_setAllowedChars(USpoofChecker* sc, const USet* chars, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USpoofChecker*, const USet*, UErrorCode*);
  if (syms[118] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(USpoofChecker*, const USet*, UErrorCode*))syms[118];
  ptr(sc, chars, status);
  return;
}

const USet* uspoof_getAllowedChars(const USpoofChecker* sc, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const USet* (*ptr)(const USpoofChecker*, UErrorCode*);
  if (syms[119] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const USet*)0;
  }
  ptr = (const USet*(*)(const USpoofChecker*, UErrorCode*))syms[119];
  return ptr(sc, status);
}

int32_t uspoof_check(const USpoofChecker* sc, const UChar* id, int32_t length, int32_t* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[120] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[120];
  return ptr(sc, id, length, position, status);
}

int32_t uspoof_checkUTF8(const USpoofChecker* sc, const char* id, int32_t length, int32_t* position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*);
  if (syms[121] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, int32_t*, UErrorCode*))syms[121];
  return ptr(sc, id, length, position, status);
}

int32_t uspoof_areConfusable(const USpoofChecker* sc, const UChar* id1, int32_t length1, const UChar* id2, int32_t length2, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[122] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[122];
  return ptr(sc, id1, length1, id2, length2, status);
}

int32_t uspoof_areConfusableUTF8(const USpoofChecker* sc, const char* id1, int32_t length1, const char* id2, int32_t length2, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[123] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, const char*, int32_t, const char*, int32_t, UErrorCode*))syms[123];
  return ptr(sc, id1, length1, id2, length2, status);
}

int32_t uspoof_getSkeleton(const USpoofChecker* sc, uint32_t type, const UChar* id, int32_t length, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[124] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[124];
  return ptr(sc, type, id, length, dest, destCapacity, status);
}

int32_t uspoof_getSkeletonUTF8(const USpoofChecker* sc, uint32_t type, const char* id, int32_t length, char* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*);
  if (syms[125] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USpoofChecker*, uint32_t, const char*, int32_t, char*, int32_t, UErrorCode*))syms[125];
  return ptr(sc, type, id, length, dest, destCapacity, status);
}

const USet* uspoof_getInclusionSet(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const USet* (*ptr)(UErrorCode*);
  if (syms[126] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const USet*)0;
  }
  ptr = (const USet*(*)(UErrorCode*))syms[126];
  return ptr(status);
}

const USet* uspoof_getRecommendedSet(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const USet* (*ptr)(UErrorCode*);
  if (syms[127] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const USet*)0;
  }
  ptr = (const USet*(*)(UErrorCode*))syms[127];
  return ptr(status);
}

int32_t uspoof_serialize(USpoofChecker* sc, void* data, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(USpoofChecker*, void*, int32_t, UErrorCode*);
  if (syms[128] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(USpoofChecker*, void*, int32_t, UErrorCode*))syms[128];
  return ptr(sc, data, capacity, status);
}

/* unicode/uformattable.h */
UFormattable* ufmt_open(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UFormattable* (*ptr)(UErrorCode*);
  if (syms[129] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UFormattable*)0;
  }
  ptr = (UFormattable*(*)(UErrorCode*))syms[129];
  return ptr(status);
}

void ufmt_close(UFormattable* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UFormattable*);
  ptr = (void(*)(UFormattable*))syms[130];
  ptr(fmt);
  return;
}

UFormattableType ufmt_getType(const UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UFormattableType (*ptr)(const UFormattable*, UErrorCode*);
  if (syms[131] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UFormattableType)0;
  }
  ptr = (UFormattableType(*)(const UFormattable*, UErrorCode*))syms[131];
  return ptr(fmt, status);
}

UBool ufmt_isNumeric(const UFormattable* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UFormattable*);
  ptr = (UBool(*)(const UFormattable*))syms[132];
  return ptr(fmt);
}

UDate ufmt_getDate(const UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UFormattable*, UErrorCode*);
  if (syms[133] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDate)0;
  }
  ptr = (UDate(*)(const UFormattable*, UErrorCode*))syms[133];
  return ptr(fmt, status);
}

double ufmt_getDouble(UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(UFormattable*, UErrorCode*);
  if (syms[134] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (double)0;
  }
  ptr = (double(*)(UFormattable*, UErrorCode*))syms[134];
  return ptr(fmt, status);
}

int32_t ufmt_getLong(UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UFormattable*, UErrorCode*);
  if (syms[135] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UFormattable*, UErrorCode*))syms[135];
  return ptr(fmt, status);
}

int64_t ufmt_getInt64(UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UFormattable*, UErrorCode*);
  if (syms[136] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(UFormattable*, UErrorCode*))syms[136];
  return ptr(fmt, status);
}

const void* ufmt_getObject(const UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const void* (*ptr)(const UFormattable*, UErrorCode*);
  if (syms[137] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const void*)0;
  }
  ptr = (const void*(*)(const UFormattable*, UErrorCode*))syms[137];
  return ptr(fmt, status);
}

const UChar* ufmt_getUChars(UFormattable* fmt, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(UFormattable*, int32_t*, UErrorCode*);
  if (syms[138] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(UFormattable*, int32_t*, UErrorCode*))syms[138];
  return ptr(fmt, len, status);
}

int32_t ufmt_getArrayLength(const UFormattable* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UFormattable*, UErrorCode*);
  if (syms[139] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UFormattable*, UErrorCode*))syms[139];
  return ptr(fmt, status);
}

UFormattable* ufmt_getArrayItemByIndex(UFormattable* fmt, int32_t n, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UFormattable* (*ptr)(UFormattable*, int32_t, UErrorCode*);
  if (syms[140] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UFormattable*)0;
  }
  ptr = (UFormattable*(*)(UFormattable*, int32_t, UErrorCode*))syms[140];
  return ptr(fmt, n, status);
}

const char* ufmt_getDecNumChars(UFormattable* fmt, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UFormattable*, int32_t*, UErrorCode*);
  if (syms[141] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(UFormattable*, int32_t*, UErrorCode*))syms[141];
  return ptr(fmt, len, status);
}

/* unicode/unum.h */
UNumberFormat* unum_open(UNumberFormatStyle style, const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseErr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberFormat* (*ptr)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  if (syms[142] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UNumberFormat*)0;
  }
  ptr = (UNumberFormat*(*)(UNumberFormatStyle, const UChar*, int32_t, const char*, UParseError*, UErrorCode*))syms[142];
  return ptr(style, pattern, patternLength, locale, parseErr, status);
}

void unum_close(UNumberFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*);
  ptr = (void(*)(UNumberFormat*))syms[143];
  ptr(fmt);
  return;
}

UNumberFormat* unum_clone(const UNumberFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberFormat* (*ptr)(const UNumberFormat*, UErrorCode*);
  if (syms[144] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UNumberFormat*)0;
  }
  ptr = (UNumberFormat*(*)(const UNumberFormat*, UErrorCode*))syms[144];
  return ptr(fmt, status);
}

int32_t unum_format(const UNumberFormat* fmt, int32_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[145] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[145];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatInt64(const UNumberFormat* fmt, int64_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[146] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, int64_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[146];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDouble(const UNumberFormat* fmt, double number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[147] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[147];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_formatDecimal(const UNumberFormat* fmt, const char* number, int32_t length, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, const char*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[148] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, const char*, int32_t, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[148];
  return ptr(fmt, number, length, result, resultLength, pos, status);
}

int32_t unum_formatDoubleCurrency(const UNumberFormat* fmt, double number, UChar* currency, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[149] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, double, UChar*, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[149];
  return ptr(fmt, number, currency, result, resultLength, pos, status);
}

int32_t unum_formatUFormattable(const UNumberFormat* fmt, const UFormattable* number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, const UFormattable*, UChar*, int32_t, UFieldPosition*, UErrorCode*);
  if (syms[150] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, const UFormattable*, UChar*, int32_t, UFieldPosition*, UErrorCode*))syms[150];
  return ptr(fmt, number, result, resultLength, pos, status);
}

int32_t unum_parse(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[151] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[151];
  return ptr(fmt, text, textLength, parsePos, status);
}

int64_t unum_parseInt64(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[152] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[152];
  return ptr(fmt, text, textLength, parsePos, status);
}

double unum_parseDouble(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[153] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (double)0;
  }
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[153];
  return ptr(fmt, text, textLength, parsePos, status);
}

int32_t unum_parseDecimal(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, char* outBuf, int32_t outBufLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, char*, int32_t, UErrorCode*);
  if (syms[154] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, char*, int32_t, UErrorCode*))syms[154];
  return ptr(fmt, text, textLength, parsePos, outBuf, outBufLength, status);
}

double unum_parseDoubleCurrency(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UChar* currency, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*);
  if (syms[155] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (double)0;
  }
  ptr = (double(*)(const UNumberFormat*, const UChar*, int32_t, int32_t*, UChar*, UErrorCode*))syms[155];
  return ptr(fmt, text, textLength, parsePos, currency, status);
}

UFormattable* unum_parseToUFormattable(const UNumberFormat* fmt, UFormattable* result, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UFormattable* (*ptr)(const UNumberFormat*, UFormattable*, const UChar*, int32_t, int32_t*, UErrorCode*);
  if (syms[156] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UFormattable*)0;
  }
  ptr = (UFormattable*(*)(const UNumberFormat*, UFormattable*, const UChar*, int32_t, int32_t*, UErrorCode*))syms[156];
  return ptr(fmt, result, text, textLength, parsePos, status);
}

void unum_applyPattern(UNumberFormat* format, UBool localized, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*);
  if (syms[157] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UNumberFormat*, UBool, const UChar*, int32_t, UParseError*, UErrorCode*))syms[157];
  ptr(format, localized, pattern, patternLength, parseError, status);
  return;
}

const char* unum_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[158];
  return ptr(localeIndex);
}

int32_t unum_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[159];
  return ptr();
}

int32_t unum_getAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatAttribute))syms[160];
  return ptr(fmt, attr);
}

void unum_setAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, int32_t);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, int32_t))syms[161];
  ptr(fmt, attr, newValue);
  return;
}

double unum_getDoubleAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UNumberFormat*, UNumberFormatAttribute);
  ptr = (double(*)(const UNumberFormat*, UNumberFormatAttribute))syms[162];
  return ptr(fmt, attr);
}

void unum_setDoubleAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, double newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatAttribute, double);
  ptr = (void(*)(UNumberFormat*, UNumberFormatAttribute, double))syms[163];
  ptr(fmt, attr, newValue);
  return;
}

int32_t unum_getTextAttribute(const UNumberFormat* fmt, UNumberFormatTextAttribute tag, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*);
  if (syms[164] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatTextAttribute, UChar*, int32_t, UErrorCode*))syms[164];
  return ptr(fmt, tag, result, resultLength, status);
}

void unum_setTextAttribute(UNumberFormat* fmt, UNumberFormatTextAttribute tag, const UChar* newValue, int32_t newValueLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*);
  if (syms[165] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UNumberFormat*, UNumberFormatTextAttribute, const UChar*, int32_t, UErrorCode*))syms[165];
  ptr(fmt, tag, newValue, newValueLength, status);
  return;
}

int32_t unum_toPattern(const UNumberFormat* fmt, UBool isPatternLocalized, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*);
  if (syms[166] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, UBool, UChar*, int32_t, UErrorCode*))syms[166];
  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}

int32_t unum_getSymbol(const UNumberFormat* fmt, UNumberFormatSymbol symbol, UChar* buffer, int32_t size, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*);
  if (syms[167] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberFormat*, UNumberFormatSymbol, UChar*, int32_t, UErrorCode*))syms[167];
  return ptr(fmt, symbol, buffer, size, status);
}

void unum_setSymbol(UNumberFormat* fmt, UNumberFormatSymbol symbol, const UChar* value, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*);
  if (syms[168] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UNumberFormat*, UNumberFormatSymbol, const UChar*, int32_t, UErrorCode*))syms[168];
  ptr(fmt, symbol, value, length, status);
  return;
}

const char* unum_getLocaleByType(const UNumberFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*);
  if (syms[169] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UNumberFormat*, ULocDataLocaleType, UErrorCode*))syms[169];
  return ptr(fmt, type, status);
}

void unum_setContext(UNumberFormat* fmt, UDisplayContext value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberFormat*, UDisplayContext, UErrorCode*);
  if (syms[170] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UNumberFormat*, UDisplayContext, UErrorCode*))syms[170];
  ptr(fmt, value, status);
  return;
}

UDisplayContext unum_getContext(const UNumberFormat* fmt, UDisplayContextType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDisplayContext (*ptr)(const UNumberFormat*, UDisplayContextType, UErrorCode*);
  if (syms[171] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDisplayContext)0;
  }
  ptr = (UDisplayContext(*)(const UNumberFormat*, UDisplayContextType, UErrorCode*))syms[171];
  return ptr(fmt, type, status);
}

/* unicode/utmscale.h */
int64_t utmscale_getTimeScaleValue(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UDateTimeScale, UTimeScaleValue, UErrorCode*);
  if (syms[172] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(UDateTimeScale, UTimeScaleValue, UErrorCode*))syms[172];
  return ptr(timeScale, value, status);
}

int64_t utmscale_fromInt64(int64_t otherTime, UDateTimeScale timeScale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  if (syms[173] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))syms[173];
  return ptr(otherTime, timeScale, status);
}

int64_t utmscale_toInt64(int64_t universalTime, UDateTimeScale timeScale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(int64_t, UDateTimeScale, UErrorCode*);
  if (syms[174] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(int64_t, UDateTimeScale, UErrorCode*))syms[174];
  return ptr(universalTime, timeScale, status);
}

/* unicode/upluralrules.h */
UPluralRules* uplrules_open(const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UPluralRules* (*ptr)(const char*, UErrorCode*);
  if (syms[175] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UPluralRules*)0;
  }
  ptr = (UPluralRules*(*)(const char*, UErrorCode*))syms[175];
  return ptr(locale, status);
}

void uplrules_close(UPluralRules* uplrules) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UPluralRules*);
  ptr = (void(*)(UPluralRules*))syms[176];
  ptr(uplrules);
  return;
}

int32_t uplrules_select(const UPluralRules* uplrules, double number, UChar* keyword, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UPluralRules*, double, UChar*, int32_t, UErrorCode*);
  if (syms[177] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UPluralRules*, double, UChar*, int32_t, UErrorCode*))syms[177];
  return ptr(uplrules, number, keyword, capacity, status);
}

/* unicode/unumsys.h */
UNumberingSystem* unumsys_open(const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberingSystem* (*ptr)(const char*, UErrorCode*);
  if (syms[178] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UNumberingSystem*)0;
  }
  ptr = (UNumberingSystem*(*)(const char*, UErrorCode*))syms[178];
  return ptr(locale, status);
}

UNumberingSystem* unumsys_openByName(const char* name, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNumberingSystem* (*ptr)(const char*, UErrorCode*);
  if (syms[179] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UNumberingSystem*)0;
  }
  ptr = (UNumberingSystem*(*)(const char*, UErrorCode*))syms[179];
  return ptr(name, status);
}

void unumsys_close(UNumberingSystem* unumsys) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNumberingSystem*);
  ptr = (void(*)(UNumberingSystem*))syms[180];
  ptr(unumsys);
  return;
}

UEnumeration* unumsys_openAvailableNames(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[181] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[181];
  return ptr(status);
}

const char* unumsys_getName(const UNumberingSystem* unumsys) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UNumberingSystem*);
  ptr = (const char*(*)(const UNumberingSystem*))syms[182];
  return ptr(unumsys);
}

UBool unumsys_isAlgorithmic(const UNumberingSystem* unumsys) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UNumberingSystem*);
  ptr = (UBool(*)(const UNumberingSystem*))syms[183];
  return ptr(unumsys);
}

int32_t unumsys_getRadix(const UNumberingSystem* unumsys) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberingSystem*);
  ptr = (int32_t(*)(const UNumberingSystem*))syms[184];
  return ptr(unumsys);
}

int32_t unumsys_getDescription(const UNumberingSystem* unumsys, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNumberingSystem*, UChar*, int32_t, UErrorCode*);
  if (syms[185] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNumberingSystem*, UChar*, int32_t, UErrorCode*))syms[185];
  return ptr(unumsys, result, resultLength, status);
}

/* unicode/ucoleitr.h */
UCollationElements* ucol_openElements(const UCollator* coll, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCollationElements* (*ptr)(const UCollator*, const UChar*, int32_t, UErrorCode*);
  if (syms[186] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCollationElements*)0;
  }
  ptr = (UCollationElements*(*)(const UCollator*, const UChar*, int32_t, UErrorCode*))syms[186];
  return ptr(coll, text, textLength, status);
}

int32_t ucol_keyHashCode(const uint8_t* key, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t);
  ptr = (int32_t(*)(const uint8_t*, int32_t))syms[187];
  return ptr(key, length);
}

void ucol_closeElements(UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*);
  ptr = (void(*)(UCollationElements*))syms[188];
  ptr(elems);
  return;
}

void ucol_reset(UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*);
  ptr = (void(*)(UCollationElements*))syms[189];
  ptr(elems);
  return;
}

int32_t ucol_next(UCollationElements* elems, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  if (syms[190] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))syms[190];
  return ptr(elems, status);
}

int32_t ucol_previous(UCollationElements* elems, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCollationElements*, UErrorCode*);
  if (syms[191] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCollationElements*, UErrorCode*))syms[191];
  return ptr(elems, status);
}

int32_t ucol_getMaxExpansion(const UCollationElements* elems, int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollationElements*, int32_t);
  ptr = (int32_t(*)(const UCollationElements*, int32_t))syms[192];
  return ptr(elems, order);
}

void ucol_setText(UCollationElements* elems, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*, const UChar*, int32_t, UErrorCode*);
  if (syms[193] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCollationElements*, const UChar*, int32_t, UErrorCode*))syms[193];
  ptr(elems, text, textLength, status);
  return;
}

int32_t ucol_getOffset(const UCollationElements* elems) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCollationElements*);
  ptr = (int32_t(*)(const UCollationElements*))syms[194];
  return ptr(elems);
}

void ucol_setOffset(UCollationElements* elems, int32_t offset, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCollationElements*, int32_t, UErrorCode*);
  if (syms[195] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCollationElements*, int32_t, UErrorCode*))syms[195];
  ptr(elems, offset, status);
  return;
}

int32_t ucol_primaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[196];
  return ptr(order);
}

int32_t ucol_secondaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[197];
  return ptr(order);
}

int32_t ucol_tertiaryOrder(int32_t order) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(int32_t);
  ptr = (int32_t(*)(int32_t))syms[198];
  return ptr(order);
}

/* unicode/uregex.h */
URegularExpression* uregex_open(const UChar* pattern, int32_t patternLength, uint32_t flags, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression* (*ptr)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*);
  if (syms[199] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (URegularExpression*)0;
  }
  ptr = (URegularExpression*(*)(const UChar*, int32_t, uint32_t, UParseError*, UErrorCode*))syms[199];
  return ptr(pattern, patternLength, flags, pe, status);
}

URegularExpression* uregex_openUText(UText* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression* (*ptr)(UText*, uint32_t, UParseError*, UErrorCode*);
  if (syms[200] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (URegularExpression*)0;
  }
  ptr = (URegularExpression*(*)(UText*, uint32_t, UParseError*, UErrorCode*))syms[200];
  return ptr(pattern, flags, pe, status);
}

URegularExpression* uregex_openC(const char* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression* (*ptr)(const char*, uint32_t, UParseError*, UErrorCode*);
  if (syms[201] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (URegularExpression*)0;
  }
  ptr = (URegularExpression*(*)(const char*, uint32_t, UParseError*, UErrorCode*))syms[201];
  return ptr(pattern, flags, pe, status);
}

void uregex_close(URegularExpression* regexp) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*);
  ptr = (void(*)(URegularExpression*))syms[202];
  ptr(regexp);
  return;
}

URegularExpression* uregex_clone(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  URegularExpression* (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[203] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (URegularExpression*)0;
  }
  ptr = (URegularExpression*(*)(const URegularExpression*, UErrorCode*))syms[203];
  return ptr(regexp, status);
}

const UChar* uregex_pattern(const URegularExpression* regexp, int32_t* patLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const URegularExpression*, int32_t*, UErrorCode*);
  if (syms[204] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const URegularExpression*, int32_t*, UErrorCode*))syms[204];
  return ptr(regexp, patLength, status);
}

UText* uregex_patternUText(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[205] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(const URegularExpression*, UErrorCode*))syms[205];
  return ptr(regexp, status);
}

int32_t uregex_flags(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[206] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[206];
  return ptr(regexp, status);
}

void uregex_setText(URegularExpression* regexp, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, const UChar*, int32_t, UErrorCode*);
  if (syms[207] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, const UChar*, int32_t, UErrorCode*))syms[207];
  ptr(regexp, text, textLength, status);
  return;
}

void uregex_setUText(URegularExpression* regexp, UText* text, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UText*, UErrorCode*);
  if (syms[208] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, UText*, UErrorCode*))syms[208];
  ptr(regexp, text, status);
  return;
}

const UChar* uregex_getText(URegularExpression* regexp, int32_t* textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(URegularExpression*, int32_t*, UErrorCode*);
  if (syms[209] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(URegularExpression*, int32_t*, UErrorCode*))syms[209];
  return ptr(regexp, textLength, status);
}

UText* uregex_getUText(URegularExpression* regexp, UText* dest, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(URegularExpression*, UText*, UErrorCode*);
  if (syms[210] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(URegularExpression*, UText*, UErrorCode*))syms[210];
  return ptr(regexp, dest, status);
}

void uregex_refreshUText(URegularExpression* regexp, UText* text, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UText*, UErrorCode*);
  if (syms[211] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, UText*, UErrorCode*))syms[211];
  ptr(regexp, text, status);
  return;
}

UBool uregex_matches(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[212] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[212];
  return ptr(regexp, startIndex, status);
}

UBool uregex_matches64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int64_t, UErrorCode*);
  if (syms[213] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int64_t, UErrorCode*))syms[213];
  return ptr(regexp, startIndex, status);
}

UBool uregex_lookingAt(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[214] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[214];
  return ptr(regexp, startIndex, status);
}

UBool uregex_lookingAt64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int64_t, UErrorCode*);
  if (syms[215] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int64_t, UErrorCode*))syms[215];
  return ptr(regexp, startIndex, status);
}

UBool uregex_find(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[216] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int32_t, UErrorCode*))syms[216];
  return ptr(regexp, startIndex, status);
}

UBool uregex_find64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, int64_t, UErrorCode*);
  if (syms[217] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, int64_t, UErrorCode*))syms[217];
  return ptr(regexp, startIndex, status);
}

UBool uregex_findNext(URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(URegularExpression*, UErrorCode*);
  if (syms[218] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(URegularExpression*, UErrorCode*))syms[218];
  return ptr(regexp, status);
}

int32_t uregex_groupCount(URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UErrorCode*);
  if (syms[219] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, UErrorCode*))syms[219];
  return ptr(regexp, status);
}

int32_t uregex_group(URegularExpression* regexp, int32_t groupNum, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[220] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, int32_t, UChar*, int32_t, UErrorCode*))syms[220];
  return ptr(regexp, groupNum, dest, destCapacity, status);
}

UText* uregex_groupUText(URegularExpression* regexp, int32_t groupNum, UText* dest, int64_t* groupLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(URegularExpression*, int32_t, UText*, int64_t*, UErrorCode*);
  if (syms[221] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(URegularExpression*, int32_t, UText*, int64_t*, UErrorCode*))syms[221];
  return ptr(regexp, groupNum, dest, groupLength, status);
}

int32_t uregex_start(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[222] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[222];
  return ptr(regexp, groupNum, status);
}

int64_t uregex_start64(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[223] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[223];
  return ptr(regexp, groupNum, status);
}

int32_t uregex_end(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[224] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[224];
  return ptr(regexp, groupNum, status);
}

int64_t uregex_end64(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[225] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(URegularExpression*, int32_t, UErrorCode*))syms[225];
  return ptr(regexp, groupNum, status);
}

void uregex_reset(URegularExpression* regexp, int32_t index, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[226] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[226];
  ptr(regexp, index, status);
  return;
}

void uregex_reset64(URegularExpression* regexp, int64_t index, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int64_t, UErrorCode*);
  if (syms[227] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int64_t, UErrorCode*))syms[227];
  ptr(regexp, index, status);
  return;
}

void uregex_setRegion(URegularExpression* regexp, int32_t regionStart, int32_t regionLimit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, int32_t, UErrorCode*);
  if (syms[228] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int32_t, int32_t, UErrorCode*))syms[228];
  ptr(regexp, regionStart, regionLimit, status);
  return;
}

void uregex_setRegion64(URegularExpression* regexp, int64_t regionStart, int64_t regionLimit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int64_t, int64_t, UErrorCode*);
  if (syms[229] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int64_t, int64_t, UErrorCode*))syms[229];
  ptr(regexp, regionStart, regionLimit, status);
  return;
}

void uregex_setRegionAndStart(URegularExpression* regexp, int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int64_t, int64_t, int64_t, UErrorCode*);
  if (syms[230] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int64_t, int64_t, int64_t, UErrorCode*))syms[230];
  ptr(regexp, regionStart, regionLimit, startIndex, status);
  return;
}

int32_t uregex_regionStart(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[231] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[231];
  return ptr(regexp, status);
}

int64_t uregex_regionStart64(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[232] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(const URegularExpression*, UErrorCode*))syms[232];
  return ptr(regexp, status);
}

int32_t uregex_regionEnd(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[233] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[233];
  return ptr(regexp, status);
}

int64_t uregex_regionEnd64(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[234] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int64_t)0;
  }
  ptr = (int64_t(*)(const URegularExpression*, UErrorCode*))syms[234];
  return ptr(regexp, status);
}

UBool uregex_hasTransparentBounds(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[235] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[235];
  return ptr(regexp, status);
}

void uregex_useTransparentBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  if (syms[236] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))syms[236];
  ptr(regexp, b, status);
  return;
}

UBool uregex_hasAnchoringBounds(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[237] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[237];
  return ptr(regexp, status);
}

void uregex_useAnchoringBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UBool, UErrorCode*);
  if (syms[238] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, UBool, UErrorCode*))syms[238];
  ptr(regexp, b, status);
  return;
}

UBool uregex_hitEnd(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[239] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[239];
  return ptr(regexp, status);
}

UBool uregex_requireEnd(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[240] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const URegularExpression*, UErrorCode*))syms[240];
  return ptr(regexp, status);
}

int32_t uregex_replaceAll(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[241] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[241];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

UText* uregex_replaceAllUText(URegularExpression* regexp, UText* replacement, UText* dest, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(URegularExpression*, UText*, UText*, UErrorCode*);
  if (syms[242] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(URegularExpression*, UText*, UText*, UErrorCode*))syms[242];
  return ptr(regexp, replacement, dest, status);
}

int32_t uregex_replaceFirst(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[243] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[243];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

UText* uregex_replaceFirstUText(URegularExpression* regexp, UText* replacement, UText* dest, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(URegularExpression*, UText*, UText*, UErrorCode*);
  if (syms[244] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(URegularExpression*, UText*, UText*, UErrorCode*))syms[244];
  return ptr(regexp, replacement, dest, status);
}

int32_t uregex_appendReplacement(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*);
  if (syms[245] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, const UChar*, int32_t, UChar**, int32_t*, UErrorCode*))syms[245];
  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}

void uregex_appendReplacementUText(URegularExpression* regexp, UText* replacementText, UText* dest, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, UText*, UText*, UErrorCode*);
  if (syms[246] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, UText*, UText*, UErrorCode*))syms[246];
  ptr(regexp, replacementText, dest, status);
  return;
}

int32_t uregex_appendTail(URegularExpression* regexp, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UChar**, int32_t*, UErrorCode*);
  if (syms[247] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, UChar**, int32_t*, UErrorCode*))syms[247];
  return ptr(regexp, destBuf, destCapacity, status);
}

UText* uregex_appendTailUText(URegularExpression* regexp, UText* dest, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(URegularExpression*, UText*, UErrorCode*);
  if (syms[248] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(URegularExpression*, UText*, UErrorCode*))syms[248];
  return ptr(regexp, dest, status);
}

int32_t uregex_split(URegularExpression* regexp, UChar* destBuf, int32_t destCapacity, int32_t* requiredCapacity, UChar* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*);
  if (syms[249] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, UChar*, int32_t, int32_t*, UChar* [], int32_t, UErrorCode*))syms[249];
  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}

int32_t uregex_splitUText(URegularExpression* regexp, UText* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(URegularExpression*, UText* [], int32_t, UErrorCode*);
  if (syms[250] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(URegularExpression*, UText* [], int32_t, UErrorCode*))syms[250];
  return ptr(regexp, destFields, destFieldsCapacity, status);
}

void uregex_setTimeLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[251] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[251];
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getTimeLimit(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[252] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[252];
  return ptr(regexp, status);
}

void uregex_setStackLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, int32_t, UErrorCode*);
  if (syms[253] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, int32_t, UErrorCode*))syms[253];
  ptr(regexp, limit, status);
  return;
}

int32_t uregex_getStackLimit(const URegularExpression* regexp, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegularExpression*, UErrorCode*);
  if (syms[254] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const URegularExpression*, UErrorCode*))syms[254];
  return ptr(regexp, status);
}

void uregex_setMatchCallback(URegularExpression* regexp, URegexMatchCallback* callback, const void* context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*);
  if (syms[255] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, URegexMatchCallback*, const void*, UErrorCode*))syms[255];
  ptr(regexp, callback, context, status);
  return;
}

void uregex_getMatchCallback(const URegularExpression* regexp, URegexMatchCallback** callback, const void** context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*);
  if (syms[256] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const URegularExpression*, URegexMatchCallback**, const void**, UErrorCode*))syms[256];
  ptr(regexp, callback, context, status);
  return;
}

void uregex_setFindProgressCallback(URegularExpression* regexp, URegexFindProgressCallback* callback, const void* context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(URegularExpression*, URegexFindProgressCallback*, const void*, UErrorCode*);
  if (syms[257] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(URegularExpression*, URegexFindProgressCallback*, const void*, UErrorCode*))syms[257];
  ptr(regexp, callback, context, status);
  return;
}

void uregex_getFindProgressCallback(const URegularExpression* regexp, URegexFindProgressCallback** callback, const void** context, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const URegularExpression*, URegexFindProgressCallback**, const void**, UErrorCode*);
  if (syms[258] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const URegularExpression*, URegexFindProgressCallback**, const void**, UErrorCode*))syms[258];
  ptr(regexp, callback, context, status);
  return;
}

/* unicode/ulocdata.h */
ULocaleData* ulocdata_open(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULocaleData* (*ptr)(const char*, UErrorCode*);
  if (syms[259] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (ULocaleData*)0;
  }
  ptr = (ULocaleData*(*)(const char*, UErrorCode*))syms[259];
  return ptr(localeID, status);
}

void ulocdata_close(ULocaleData* uld) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(ULocaleData*);
  ptr = (void(*)(ULocaleData*))syms[260];
  ptr(uld);
  return;
}

void ulocdata_setNoSubstitute(ULocaleData* uld, UBool setting) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(ULocaleData*, UBool);
  ptr = (void(*)(ULocaleData*, UBool))syms[261];
  ptr(uld, setting);
  return;
}

UBool ulocdata_getNoSubstitute(ULocaleData* uld) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(ULocaleData*);
  ptr = (UBool(*)(ULocaleData*))syms[262];
  return ptr(uld);
}

USet* ulocdata_getExemplarSet(ULocaleData* uld, USet* fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*);
  if (syms[263] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USet*)0;
  }
  ptr = (USet*(*)(ULocaleData*, USet*, uint32_t, ULocaleDataExemplarSetType, UErrorCode*))syms[263];
  return ptr(uld, fillIn, options, extype, status);
}

int32_t ulocdata_getDelimiter(ULocaleData* uld, ULocaleDataDelimiterType type, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*);
  if (syms[264] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(ULocaleData*, ULocaleDataDelimiterType, UChar*, int32_t, UErrorCode*))syms[264];
  return ptr(uld, type, result, resultLength, status);
}

UMeasurementSystem ulocdata_getMeasurementSystem(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMeasurementSystem (*ptr)(const char*, UErrorCode*);
  if (syms[265] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UMeasurementSystem)0;
  }
  ptr = (UMeasurementSystem(*)(const char*, UErrorCode*))syms[265];
  return ptr(localeID, status);
}

void ulocdata_getPaperSize(const char* localeID, int32_t* height, int32_t* width, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, int32_t*, int32_t*, UErrorCode*);
  if (syms[266] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, int32_t*, int32_t*, UErrorCode*))syms[266];
  ptr(localeID, height, width, status);
  return;
}

void ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, UErrorCode*);
  if (syms[267] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UVersionInfo, UErrorCode*))syms[267];
  ptr(versionArray, status);
  return;
}

int32_t ulocdata_getLocaleDisplayPattern(ULocaleData* uld, UChar* pattern, int32_t patternCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  if (syms[268] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))syms[268];
  return ptr(uld, pattern, patternCapacity, status);
}

int32_t ulocdata_getLocaleSeparator(ULocaleData* uld, UChar* separator, int32_t separatorCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(ULocaleData*, UChar*, int32_t, UErrorCode*);
  if (syms[269] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(ULocaleData*, UChar*, int32_t, UErrorCode*))syms[269];
  return ptr(uld, separator, separatorCapacity, status);
}

/* unicode/uregion.h */
const URegion* uregion_getRegionFromCode(const char* regionCode, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const URegion* (*ptr)(const char*, UErrorCode*);
  if (syms[270] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const URegion*)0;
  }
  ptr = (const URegion*(*)(const char*, UErrorCode*))syms[270];
  return ptr(regionCode, status);
}

const URegion* uregion_getRegionFromNumericCode(int32_t code, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const URegion* (*ptr)(int32_t, UErrorCode*);
  if (syms[271] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const URegion*)0;
  }
  ptr = (const URegion*(*)(int32_t, UErrorCode*))syms[271];
  return ptr(code, status);
}

UEnumeration* uregion_getAvailable(URegionType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(URegionType, UErrorCode*);
  if (syms[272] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(URegionType, UErrorCode*))syms[272];
  return ptr(type, status);
}

UBool uregion_areEqual(const URegion* uregion, const URegion* otherRegion) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegion*, const URegion*);
  ptr = (UBool(*)(const URegion*, const URegion*))syms[273];
  return ptr(uregion, otherRegion);
}

const URegion* uregion_getContainingRegion(const URegion* uregion) {
  pthread_once(&once_control, &init_icudata_version);
  const URegion* (*ptr)(const URegion*);
  ptr = (const URegion*(*)(const URegion*))syms[274];
  return ptr(uregion);
}

const URegion* uregion_getContainingRegionOfType(const URegion* uregion, URegionType type) {
  pthread_once(&once_control, &init_icudata_version);
  const URegion* (*ptr)(const URegion*, URegionType);
  ptr = (const URegion*(*)(const URegion*, URegionType))syms[275];
  return ptr(uregion, type);
}

UEnumeration* uregion_getContainedRegions(const URegion* uregion, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const URegion*, UErrorCode*);
  if (syms[276] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const URegion*, UErrorCode*))syms[276];
  return ptr(uregion, status);
}

UEnumeration* uregion_getContainedRegionsOfType(const URegion* uregion, URegionType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const URegion*, URegionType, UErrorCode*);
  if (syms[277] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const URegion*, URegionType, UErrorCode*))syms[277];
  return ptr(uregion, type, status);
}

UBool uregion_contains(const URegion* uregion, const URegion* otherRegion) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const URegion*, const URegion*);
  ptr = (UBool(*)(const URegion*, const URegion*))syms[278];
  return ptr(uregion, otherRegion);
}

UEnumeration* uregion_getPreferredValues(const URegion* uregion, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const URegion*, UErrorCode*);
  if (syms[279] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const URegion*, UErrorCode*))syms[279];
  return ptr(uregion, status);
}

const char* uregion_getRegionCode(const URegion* uregion) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const URegion*);
  ptr = (const char*(*)(const URegion*))syms[280];
  return ptr(uregion);
}

int32_t uregion_getNumericCode(const URegion* uregion) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const URegion*);
  ptr = (int32_t(*)(const URegion*))syms[281];
  return ptr(uregion);
}

URegionType uregion_getType(const URegion* uregion) {
  pthread_once(&once_control, &init_icudata_version);
  URegionType (*ptr)(const URegion*);
  ptr = (URegionType(*)(const URegion*))syms[282];
  return ptr(uregion);
}

/* unicode/ucal.h */
UEnumeration* ucal_openTimeZoneIDEnumeration(USystemTimeZoneType zoneType, const char* region, const int32_t* rawOffset, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(USystemTimeZoneType, const char*, const int32_t*, UErrorCode*);
  if (syms[283] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(USystemTimeZoneType, const char*, const int32_t*, UErrorCode*))syms[283];
  return ptr(zoneType, region, rawOffset, ec);
}

UEnumeration* ucal_openTimeZones(UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[284] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[284];
  return ptr(ec);
}

UEnumeration* ucal_openCountryTimeZones(const char* country, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  if (syms[285] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[285];
  return ptr(country, ec);
}

int32_t ucal_getDefaultTimeZone(UChar* result, int32_t resultCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, UErrorCode*);
  if (syms[286] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar*, int32_t, UErrorCode*))syms[286];
  return ptr(result, resultCapacity, ec);
}

void ucal_setDefaultTimeZone(const UChar* zoneID, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, UErrorCode*);
  if (syms[287] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UChar*, UErrorCode*))syms[287];
  ptr(zoneID, ec);
  return;
}

int32_t ucal_getDSTSavings(const UChar* zoneID, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, UErrorCode*);
  if (syms[288] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, UErrorCode*))syms[288];
  return ptr(zoneID, ec);
}

UDate ucal_getNow(void) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(void);
  ptr = (UDate(*)(void))syms[289];
  return ptr();
}

UCalendar* ucal_open(const UChar* zoneID, int32_t len, const char* locale, UCalendarType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendar* (*ptr)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*);
  if (syms[290] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCalendar*)0;
  }
  ptr = (UCalendar*(*)(const UChar*, int32_t, const char*, UCalendarType, UErrorCode*))syms[290];
  return ptr(zoneID, len, locale, type, status);
}

void ucal_close(UCalendar* cal) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*);
  ptr = (void(*)(UCalendar*))syms[291];
  ptr(cal);
  return;
}

UCalendar* ucal_clone(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendar* (*ptr)(const UCalendar*, UErrorCode*);
  if (syms[292] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCalendar*)0;
  }
  ptr = (UCalendar*(*)(const UCalendar*, UErrorCode*))syms[292];
  return ptr(cal, status);
}

void ucal_setTimeZone(UCalendar* cal, const UChar* zoneID, int32_t len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, const UChar*, int32_t, UErrorCode*);
  if (syms[293] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, const UChar*, int32_t, UErrorCode*))syms[293];
  ptr(cal, zoneID, len, status);
  return;
}

int32_t ucal_getTimeZoneID(const UCalendar* cal, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UChar*, int32_t, UErrorCode*);
  if (syms[294] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCalendar*, UChar*, int32_t, UErrorCode*))syms[294];
  return ptr(cal, result, resultLength, status);
}

int32_t ucal_getTimeZoneDisplayName(const UCalendar* cal, UCalendarDisplayNameType type, const char* locale, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[295] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCalendar*, UCalendarDisplayNameType, const char*, UChar*, int32_t, UErrorCode*))syms[295];
  return ptr(cal, type, locale, result, resultLength, status);
}

UBool ucal_inDaylightTime(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UErrorCode*);
  if (syms[296] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UCalendar*, UErrorCode*))syms[296];
  return ptr(cal, status);
}

void ucal_setGregorianChange(UCalendar* cal, UDate date, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  if (syms[297] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))syms[297];
  ptr(cal, date, pErrorCode);
  return;
}

UDate ucal_getGregorianChange(const UCalendar* cal, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  if (syms[298] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDate)0;
  }
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))syms[298];
  return ptr(cal, pErrorCode);
}

int32_t ucal_getAttribute(const UCalendar* cal, UCalendarAttribute attr) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarAttribute);
  ptr = (int32_t(*)(const UCalendar*, UCalendarAttribute))syms[299];
  return ptr(cal, attr);
}

void ucal_setAttribute(UCalendar* cal, UCalendarAttribute attr, int32_t newValue) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarAttribute, int32_t);
  ptr = (void(*)(UCalendar*, UCalendarAttribute, int32_t))syms[300];
  ptr(cal, attr, newValue);
  return;
}

const char* ucal_getAvailable(int32_t localeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[301];
  return ptr(localeIndex);
}

int32_t ucal_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[302];
  return ptr();
}

UDate ucal_getMillis(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UDate (*ptr)(const UCalendar*, UErrorCode*);
  if (syms[303] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UDate)0;
  }
  ptr = (UDate(*)(const UCalendar*, UErrorCode*))syms[303];
  return ptr(cal, status);
}

void ucal_setMillis(UCalendar* cal, UDate dateTime, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UDate, UErrorCode*);
  if (syms[304] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, UDate, UErrorCode*))syms[304];
  ptr(cal, dateTime, status);
  return;
}

void ucal_setDate(UCalendar* cal, int32_t year, int32_t month, int32_t date, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*);
  if (syms[305] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, UErrorCode*))syms[305];
  ptr(cal, year, month, date, status);
  return;
}

void ucal_setDateTime(UCalendar* cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*);
  if (syms[306] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, UErrorCode*))syms[306];
  ptr(cal, year, month, date, hour, minute, second, status);
  return;
}

UBool ucal_equivalentTo(const UCalendar* cal1, const UCalendar* cal2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, const UCalendar*);
  ptr = (UBool(*)(const UCalendar*, const UCalendar*))syms[307];
  return ptr(cal1, cal2);
}

void ucal_add(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  if (syms[308] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))syms[308];
  ptr(cal, field, amount, status);
  return;
}

void ucal_roll(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*);
  if (syms[309] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t, UErrorCode*))syms[309];
  ptr(cal, field, amount, status);
  return;
}

int32_t ucal_get(const UCalendar* cal, UCalendarDateFields field, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UErrorCode*);
  if (syms[310] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UErrorCode*))syms[310];
  return ptr(cal, field, status);
}

void ucal_set(UCalendar* cal, UCalendarDateFields field, int32_t value) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields, int32_t);
  ptr = (void(*)(UCalendar*, UCalendarDateFields, int32_t))syms[311];
  ptr(cal, field, value);
  return;
}

UBool ucal_isSet(const UCalendar* cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UCalendarDateFields);
  ptr = (UBool(*)(const UCalendar*, UCalendarDateFields))syms[312];
  return ptr(cal, field);
}

void ucal_clearField(UCalendar* cal, UCalendarDateFields field) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*, UCalendarDateFields);
  ptr = (void(*)(UCalendar*, UCalendarDateFields))syms[313];
  ptr(cal, field);
  return;
}

void ucal_clear(UCalendar* calendar) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCalendar*);
  ptr = (void(*)(UCalendar*))syms[314];
  ptr(calendar);
  return;
}

int32_t ucal_getLimit(const UCalendar* cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*);
  if (syms[315] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCalendar*, UCalendarDateFields, UCalendarLimitType, UErrorCode*))syms[315];
  return ptr(cal, field, type, status);
}

const char* ucal_getLocaleByType(const UCalendar* cal, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCalendar*, ULocDataLocaleType, UErrorCode*);
  if (syms[316] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UCalendar*, ULocDataLocaleType, UErrorCode*))syms[316];
  return ptr(cal, type, status);
}

const char* ucal_getTZDataVersion(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UErrorCode*);
  if (syms[317] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(UErrorCode*))syms[317];
  return ptr(status);
}

int32_t ucal_getCanonicalTimeZoneID(const UChar* id, int32_t len, UChar* result, int32_t resultCapacity, UBool* isSystemID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*);
  if (syms[318] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UBool*, UErrorCode*))syms[318];
  return ptr(id, len, result, resultCapacity, isSystemID, status);
}

const char* ucal_getType(const UCalendar* cal, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCalendar*, UErrorCode*);
  if (syms[319] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UCalendar*, UErrorCode*))syms[319];
  return ptr(cal, status);
}

UEnumeration* ucal_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  if (syms[320] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[320];
  return ptr(key, locale, commonlyUsed, status);
}

UCalendarWeekdayType ucal_getDayOfWeekType(const UCalendar* cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCalendarWeekdayType (*ptr)(const UCalendar*, UCalendarDaysOfWeek, UErrorCode*);
  if (syms[321] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCalendarWeekdayType)0;
  }
  ptr = (UCalendarWeekdayType(*)(const UCalendar*, UCalendarDaysOfWeek, UErrorCode*))syms[321];
  return ptr(cal, dayOfWeek, status);
}

int32_t ucal_getWeekendTransition(const UCalendar* cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCalendar*, UCalendarDaysOfWeek, UErrorCode*);
  if (syms[322] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCalendar*, UCalendarDaysOfWeek, UErrorCode*))syms[322];
  return ptr(cal, dayOfWeek, status);
}

UBool ucal_isWeekend(const UCalendar* cal, UDate date, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UDate, UErrorCode*);
  if (syms[323] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UCalendar*, UDate, UErrorCode*))syms[323];
  return ptr(cal, date, status);
}

int32_t ucal_getFieldDifference(UCalendar* cal, UDate target, UCalendarDateFields field, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCalendar*, UDate, UCalendarDateFields, UErrorCode*);
  if (syms[324] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCalendar*, UDate, UCalendarDateFields, UErrorCode*))syms[324];
  return ptr(cal, target, field, status);
}

UBool ucal_getTimeZoneTransitionDate(const UCalendar* cal, UTimeZoneTransitionType type, UDate* transition, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UCalendar*, UTimeZoneTransitionType, UDate*, UErrorCode*);
  if (syms[325] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UCalendar*, UTimeZoneTransitionType, UDate*, UErrorCode*))syms[325];
  return ptr(cal, type, transition, status);
}

int32_t ucal_getWindowsTimeZoneID(const UChar* id, int32_t len, UChar* winid, int32_t winidCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[326] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[326];
  return ptr(id, len, winid, winidCapacity, status);
}

int32_t ucal_getTimeZoneIDForWindowsID(const UChar* winid, int32_t len, const char* region, UChar* id, int32_t idCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[327] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, const char*, UChar*, int32_t, UErrorCode*))syms[327];
  return ptr(winid, len, region, id, idCapacity, status);
}

/* unicode/ucurr.h */
int32_t ucurr_forLocale(const char* locale, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UChar*, int32_t, UErrorCode*);
  if (syms[328] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, UChar*, int32_t, UErrorCode*))syms[328];
  return ptr(locale, buff, buffCapacity, ec);
}

UCurrRegistryKey ucurr_register(const UChar* isoCode, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UCurrRegistryKey (*ptr)(const UChar*, const char*, UErrorCode*);
  if (syms[329] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UCurrRegistryKey)0;
  }
  ptr = (UCurrRegistryKey(*)(const UChar*, const char*, UErrorCode*))syms[329];
  return ptr(isoCode, locale, status);
}

UBool ucurr_unregister(UCurrRegistryKey key, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UCurrRegistryKey, UErrorCode*);
  if (syms[330] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(UCurrRegistryKey, UErrorCode*))syms[330];
  return ptr(key, status);
}

const UChar* ucurr_getName(const UChar* currency, const char* locale, UCurrNameStyle nameStyle, UBool* isChoiceFormat, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*);
  if (syms[331] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const UChar*, const char*, UCurrNameStyle, UBool*, int32_t*, UErrorCode*))syms[331];
  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}

const UChar* ucurr_getPluralName(const UChar* currency, const char* locale, UBool* isChoiceFormat, const char* pluralCount, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*);
  if (syms[332] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const UChar*, const char*, UBool*, const char*, int32_t*, UErrorCode*))syms[332];
  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}

int32_t ucurr_getDefaultFractionDigits(const UChar* currency, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, UErrorCode*);
  if (syms[333] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, UErrorCode*))syms[333];
  return ptr(currency, ec);
}

double ucurr_getRoundingIncrement(const UChar* currency, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(const UChar*, UErrorCode*);
  if (syms[334] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (double)0;
  }
  ptr = (double(*)(const UChar*, UErrorCode*))syms[334];
  return ptr(currency, ec);
}

UEnumeration* ucurr_openISOCurrencies(uint32_t currType, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(uint32_t, UErrorCode*);
  if (syms[335] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(uint32_t, UErrorCode*))syms[335];
  return ptr(currType, pErrorCode);
}

UBool ucurr_isAvailable(const UChar* isoCode, UDate from, UDate to, UErrorCode* errorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, UDate, UDate, UErrorCode*);
  if (syms[336] == NULL) {
    *errorCode = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UChar*, UDate, UDate, UErrorCode*))syms[336];
  return ptr(isoCode, from, to, errorCode);
}

int32_t ucurr_countCurrencies(const char* locale, UDate date, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UDate, UErrorCode*);
  if (syms[337] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, UDate, UErrorCode*))syms[337];
  return ptr(locale, date, ec);
}

int32_t ucurr_forLocaleAndDate(const char* locale, UDate date, int32_t index, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[338] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, UDate, int32_t, UChar*, int32_t, UErrorCode*))syms[338];
  return ptr(locale, date, index, buff, buffCapacity, ec);
}

UEnumeration* ucurr_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, const char*, UBool, UErrorCode*);
  if (syms[339] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, const char*, UBool, UErrorCode*))syms[339];
  return ptr(key, locale, commonlyUsed, status);
}

int32_t ucurr_getNumericCode(const UChar* currency) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*);
  ptr = (int32_t(*)(const UChar*))syms[340];
  return ptr(currency);
}

/* unicode/umsg.h */
int32_t u_formatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*);
  if (syms[341] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*))syms[341];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, args, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*);
  if (syms[342] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, va_list, UErrorCode*))syms[342];
  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}

void u_parseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*);
  if (syms[343] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*))syms[343];
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, status);
  va_end(args);
  return;
}

void u_vparseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*);
  if (syms[344] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UErrorCode*))syms[344];
  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
  return;
}

int32_t u_formatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*);
  if (syms[345] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*))syms[345];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, parseError, args, status);
  va_end(args);
  return ret;
}

int32_t u_vformatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*);
  if (syms[346] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const UChar*, int32_t, UChar*, int32_t, UParseError*, va_list, UErrorCode*))syms[346];
  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}

void u_parseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UParseError* parseError, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*);
  if (syms[347] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*))syms[347];
  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, parseError, status);
  va_end(args);
  return;
}

void u_vparseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*);
  if (syms[348] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const UChar*, int32_t, const UChar*, int32_t, va_list, UParseError*, UErrorCode*))syms[348];
  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
  return;
}

UMessageFormat* umsg_open(const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMessageFormat* (*ptr)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*);
  if (syms[349] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UMessageFormat*)0;
  }
  ptr = (UMessageFormat*(*)(const UChar*, int32_t, const char*, UParseError*, UErrorCode*))syms[349];
  return ptr(pattern, patternLength, locale, parseError, status);
}

void umsg_close(UMessageFormat* format) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*);
  ptr = (void(*)(UMessageFormat*))syms[350];
  ptr(format);
  return;
}

UMessageFormat umsg_clone(const UMessageFormat* fmt, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UMessageFormat (*ptr)(const UMessageFormat*, UErrorCode*);
  if (syms[351] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UMessageFormat)0;
  }
  ptr = (UMessageFormat(*)(const UMessageFormat*, UErrorCode*))syms[351];
  return ptr(fmt, status);
}

void umsg_setLocale(UMessageFormat* fmt, const char* locale) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*, const char*);
  ptr = (void(*)(UMessageFormat*, const char*))syms[352];
  ptr(fmt, locale);
  return;
}

const char* umsg_getLocale(const UMessageFormat* fmt) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UMessageFormat*);
  ptr = (const char*(*)(const UMessageFormat*))syms[353];
  return ptr(fmt);
}

void umsg_applyPattern(UMessageFormat* fmt, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*);
  if (syms[354] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UMessageFormat*, const UChar*, int32_t, UParseError*, UErrorCode*))syms[354];
  ptr(fmt, pattern, patternLength, parseError, status);
  return;
}

int32_t umsg_toPattern(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, UErrorCode*);
  if (syms[355] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, UErrorCode*))syms[355];
  return ptr(fmt, result, resultLength, status);
}

int32_t umsg_format(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*);
  if (syms[356] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*))syms[356];
  va_list args;
  va_start(args, status);
  int32_t ret = ptr(fmt, result, resultLength, args, status);
  va_end(args);
  return ret;
}

int32_t umsg_vformat(const UMessageFormat* fmt, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*);
  if (syms[357] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UMessageFormat*, UChar*, int32_t, va_list, UErrorCode*))syms[357];
  return ptr(fmt, result, resultLength, ap, status);
}

void umsg_parse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, UErrorCode* status, ...) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*);
  if (syms[358] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*))syms[358];
  va_list args;
  va_start(args, status);
  ptr(fmt, source, sourceLength, count, args, status);
  va_end(args);
  return;
}

void umsg_vparse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, va_list ap, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*);
  if (syms[359] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UMessageFormat*, const UChar*, int32_t, int32_t*, va_list, UErrorCode*))syms[359];
  ptr(fmt, source, sourceLength, count, ap, status);
  return;
}

int32_t umsg_autoQuoteApostrophe(const UChar* pattern, int32_t patternLength, UChar* dest, int32_t destCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[360] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[360];
  return ptr(pattern, patternLength, dest, destCapacity, ec);
}

/* unicode/uldnames.h */
ULocaleDisplayNames* uldn_open(const char* locale, UDialectHandling dialectHandling, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  ULocaleDisplayNames* (*ptr)(const char*, UDialectHandling, UErrorCode*);
  if (syms[361] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (ULocaleDisplayNames*)0;
  }
  ptr = (ULocaleDisplayNames*(*)(const char*, UDialectHandling, UErrorCode*))syms[361];
  return ptr(locale, dialectHandling, pErrorCode);
}

void uldn_close(ULocaleDisplayNames* ldn) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(ULocaleDisplayNames*);
  ptr = (void(*)(ULocaleDisplayNames*))syms[362];
  ptr(ldn);
  return;
}

const char* uldn_getLocale(const ULocaleDisplayNames* ldn) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const ULocaleDisplayNames*);
  ptr = (const char*(*)(const ULocaleDisplayNames*))syms[363];
  return ptr(ldn);
}

UDialectHandling uldn_getDialectHandling(const ULocaleDisplayNames* ldn) {
  pthread_once(&once_control, &init_icudata_version);
  UDialectHandling (*ptr)(const ULocaleDisplayNames*);
  ptr = (UDialectHandling(*)(const ULocaleDisplayNames*))syms[364];
  return ptr(ldn);
}

int32_t uldn_localeDisplayName(const ULocaleDisplayNames* ldn, const char* locale, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[365] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[365];
  return ptr(ldn, locale, result, maxResultSize, pErrorCode);
}

int32_t uldn_languageDisplayName(const ULocaleDisplayNames* ldn, const char* lang, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[366] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[366];
  return ptr(ldn, lang, result, maxResultSize, pErrorCode);
}

int32_t uldn_scriptDisplayName(const ULocaleDisplayNames* ldn, const char* script, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[367] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[367];
  return ptr(ldn, script, result, maxResultSize, pErrorCode);
}

int32_t uldn_scriptCodeDisplayName(const ULocaleDisplayNames* ldn, UScriptCode scriptCode, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, UScriptCode, UChar*, int32_t, UErrorCode*);
  if (syms[368] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, UScriptCode, UChar*, int32_t, UErrorCode*))syms[368];
  return ptr(ldn, scriptCode, result, maxResultSize, pErrorCode);
}

int32_t uldn_regionDisplayName(const ULocaleDisplayNames* ldn, const char* region, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[369] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[369];
  return ptr(ldn, region, result, maxResultSize, pErrorCode);
}

int32_t uldn_variantDisplayName(const ULocaleDisplayNames* ldn, const char* variant, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[370] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[370];
  return ptr(ldn, variant, result, maxResultSize, pErrorCode);
}

int32_t uldn_keyDisplayName(const ULocaleDisplayNames* ldn, const char* key, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[371] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, UChar*, int32_t, UErrorCode*))syms[371];
  return ptr(ldn, key, result, maxResultSize, pErrorCode);
}

int32_t uldn_keyValueDisplayName(const ULocaleDisplayNames* ldn, const char* key, const char* value, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const ULocaleDisplayNames*, const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[372] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const ULocaleDisplayNames*, const char*, const char*, UChar*, int32_t, UErrorCode*))syms[372];
  return ptr(ldn, key, value, result, maxResultSize, pErrorCode);
}

ULocaleDisplayNames* uldn_openForContext(const char* locale, UDisplayContext* contexts, int32_t length, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  ULocaleDisplayNames* (*ptr)(const char*, UDisplayContext*, int32_t, UErrorCode*);
  if (syms[373] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (ULocaleDisplayNames*)0;
  }
  ptr = (ULocaleDisplayNames*(*)(const char*, UDisplayContext*, int32_t, UErrorCode*))syms[373];
  return ptr(locale, contexts, length, pErrorCode);
}

UDisplayContext uldn_getContext(const ULocaleDisplayNames* ldn, UDisplayContextType type, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDisplayContext (*ptr)(const ULocaleDisplayNames*, UDisplayContextType, UErrorCode*);
  if (syms[374] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDisplayContext)0;
  }
  ptr = (UDisplayContext(*)(const ULocaleDisplayNames*, UDisplayContextType, UErrorCode*))syms[374];
  return ptr(ldn, type, pErrorCode);
}

/* unicode/usearch.h */
UStringSearch* usearch_open(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const char* locale, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringSearch* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*);
  if (syms[375] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UStringSearch*)0;
  }
  ptr = (UStringSearch*(*)(const UChar*, int32_t, const UChar*, int32_t, const char*, UBreakIterator*, UErrorCode*))syms[375];
  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}

UStringSearch* usearch_openFromCollator(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const UCollator* collator, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringSearch* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*);
  if (syms[376] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UStringSearch*)0;
  }
  ptr = (UStringSearch*(*)(const UChar*, int32_t, const UChar*, int32_t, const UCollator*, UBreakIterator*, UErrorCode*))syms[376];
  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}

void usearch_close(UStringSearch* searchiter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*);
  ptr = (void(*)(UStringSearch*))syms[377];
  ptr(searchiter);
  return;
}

void usearch_setOffset(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  if (syms[378] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, int32_t, UErrorCode*))syms[378];
  ptr(strsrch, position, status);
  return;
}

int32_t usearch_getOffset(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[379];
  return ptr(strsrch);
}

void usearch_setAttribute(UStringSearch* strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*);
  if (syms[380] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, USearchAttribute, USearchAttributeValue, UErrorCode*))syms[380];
  ptr(strsrch, attribute, value, status);
  return;
}

USearchAttributeValue usearch_getAttribute(const UStringSearch* strsrch, USearchAttribute attribute) {
  pthread_once(&once_control, &init_icudata_version);
  USearchAttributeValue (*ptr)(const UStringSearch*, USearchAttribute);
  ptr = (USearchAttributeValue(*)(const UStringSearch*, USearchAttribute))syms[381];
  return ptr(strsrch, attribute);
}

int32_t usearch_getMatchedStart(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[382];
  return ptr(strsrch);
}

int32_t usearch_getMatchedLength(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*);
  ptr = (int32_t(*)(const UStringSearch*))syms[383];
  return ptr(strsrch);
}

int32_t usearch_getMatchedText(const UStringSearch* strsrch, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringSearch*, UChar*, int32_t, UErrorCode*);
  if (syms[384] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UStringSearch*, UChar*, int32_t, UErrorCode*))syms[384];
  return ptr(strsrch, result, resultCapacity, status);
}

void usearch_setBreakIterator(UStringSearch* strsrch, UBreakIterator* breakiter, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, UBreakIterator*, UErrorCode*);
  if (syms[385] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, UBreakIterator*, UErrorCode*))syms[385];
  ptr(strsrch, breakiter, status);
  return;
}

const UBreakIterator* usearch_getBreakIterator(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  const UBreakIterator* (*ptr)(const UStringSearch*);
  ptr = (const UBreakIterator*(*)(const UStringSearch*))syms[386];
  return ptr(strsrch);
}

void usearch_setText(UStringSearch* strsrch, const UChar* text, int32_t textlength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  if (syms[387] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))syms[387];
  ptr(strsrch, text, textlength, status);
  return;
}

const UChar* usearch_getText(const UStringSearch* strsrch, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UStringSearch*, int32_t*);
  ptr = (const UChar*(*)(const UStringSearch*, int32_t*))syms[388];
  return ptr(strsrch, length);
}

UCollator* usearch_getCollator(const UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  UCollator* (*ptr)(const UStringSearch*);
  ptr = (UCollator*(*)(const UStringSearch*))syms[389];
  return ptr(strsrch);
}

void usearch_setCollator(UStringSearch* strsrch, const UCollator* collator, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UCollator*, UErrorCode*);
  if (syms[390] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, const UCollator*, UErrorCode*))syms[390];
  ptr(strsrch, collator, status);
  return;
}

void usearch_setPattern(UStringSearch* strsrch, const UChar* pattern, int32_t patternlength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*, const UChar*, int32_t, UErrorCode*);
  if (syms[391] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UStringSearch*, const UChar*, int32_t, UErrorCode*))syms[391];
  ptr(strsrch, pattern, patternlength, status);
  return;
}

const UChar* usearch_getPattern(const UStringSearch* strsrch, int32_t* length) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UStringSearch*, int32_t*);
  ptr = (const UChar*(*)(const UStringSearch*, int32_t*))syms[392];
  return ptr(strsrch, length);
}

int32_t usearch_first(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  if (syms[393] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[393];
  return ptr(strsrch, status);
}

int32_t usearch_following(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  if (syms[394] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))syms[394];
  return ptr(strsrch, position, status);
}

int32_t usearch_last(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  if (syms[395] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[395];
  return ptr(strsrch, status);
}

int32_t usearch_preceding(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, int32_t, UErrorCode*);
  if (syms[396] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, int32_t, UErrorCode*))syms[396];
  return ptr(strsrch, position, status);
}

int32_t usearch_next(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  if (syms[397] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[397];
  return ptr(strsrch, status);
}

int32_t usearch_previous(UStringSearch* strsrch, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UStringSearch*, UErrorCode*);
  if (syms[398] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UStringSearch*, UErrorCode*))syms[398];
  return ptr(strsrch, status);
}

void usearch_reset(UStringSearch* strsrch) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringSearch*);
  ptr = (void(*)(UStringSearch*))syms[399];
  ptr(strsrch);
  return;
}

/* unicode/utrans.h */
UTransliterator* utrans_openU(const UChar* id, int32_t idLength, UTransDirection dir, const UChar* rules, int32_t rulesLength, UParseError* parseError, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator* (*ptr)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*);
  if (syms[400] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UTransliterator*)0;
  }
  ptr = (UTransliterator*(*)(const UChar*, int32_t, UTransDirection, const UChar*, int32_t, UParseError*, UErrorCode*))syms[400];
  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}

UTransliterator* utrans_openInverse(const UTransliterator* trans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator* (*ptr)(const UTransliterator*, UErrorCode*);
  if (syms[401] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UTransliterator*)0;
  }
  ptr = (UTransliterator*(*)(const UTransliterator*, UErrorCode*))syms[401];
  return ptr(trans, status);
}

UTransliterator* utrans_clone(const UTransliterator* trans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UTransliterator* (*ptr)(const UTransliterator*, UErrorCode*);
  if (syms[402] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UTransliterator*)0;
  }
  ptr = (UTransliterator*(*)(const UTransliterator*, UErrorCode*))syms[402];
  return ptr(trans, status);
}

void utrans_close(UTransliterator* trans) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*);
  ptr = (void(*)(UTransliterator*))syms[403];
  ptr(trans);
  return;
}

const UChar* utrans_getUnicodeID(const UTransliterator* trans, int32_t* resultLength) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UTransliterator*, int32_t*);
  ptr = (const UChar*(*)(const UTransliterator*, int32_t*))syms[404];
  return ptr(trans, resultLength);
}

void utrans_register(UTransliterator* adoptedTrans, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*, UErrorCode*);
  if (syms[405] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UTransliterator*, UErrorCode*))syms[405];
  ptr(adoptedTrans, status);
  return;
}

void utrans_unregisterID(const UChar* id, int32_t idLength) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, int32_t);
  ptr = (void(*)(const UChar*, int32_t))syms[406];
  ptr(id, idLength);
  return;
}

void utrans_setFilter(UTransliterator* trans, const UChar* filterPattern, int32_t filterPatternLen, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UTransliterator*, const UChar*, int32_t, UErrorCode*);
  if (syms[407] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UTransliterator*, const UChar*, int32_t, UErrorCode*))syms[407];
  ptr(trans, filterPattern, filterPatternLen, status);
  return;
}

int32_t utrans_countAvailableIDs(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[408];
  return ptr();
}

UEnumeration* utrans_openIDs(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[409] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[409];
  return ptr(pErrorCode);
}

void utrans_trans(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, int32_t start, int32_t* limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*);
  if (syms[410] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, int32_t, int32_t*, UErrorCode*))syms[410];
  ptr(trans, rep, repFunc, start, limit, status);
  return;
}

void utrans_transIncremental(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, UTransPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*);
  if (syms[411] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UTransliterator*, UReplaceable*, UReplaceableCallbacks*, UTransPosition*, UErrorCode*))syms[411];
  ptr(trans, rep, repFunc, pos, status);
  return;
}

void utrans_transUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, int32_t start, int32_t* limit, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*);
  if (syms[412] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*))syms[412];
  ptr(trans, text, textLength, textCapacity, start, limit, status);
  return;
}

void utrans_transIncrementalUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, UTransPosition* pos, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*);
  if (syms[413] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UTransliterator*, UChar*, int32_t*, int32_t, UTransPosition*, UErrorCode*))syms[413];
  ptr(trans, text, textLength, textCapacity, pos, status);
  return;
}

int32_t utrans_toRules(const UTransliterator* trans, UBool escapeUnprintable, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UTransliterator*, UBool, UChar*, int32_t, UErrorCode*);
  if (syms[414] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UTransliterator*, UBool, UChar*, int32_t, UErrorCode*))syms[414];
  return ptr(trans, escapeUnprintable, result, resultLength, status);
}

USet* utrans_getSourceSet(const UTransliterator* trans, UBool ignoreFilter, USet* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const UTransliterator*, UBool, USet*, UErrorCode*);
  if (syms[415] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (USet*)0;
  }
  ptr = (USet*(*)(const UTransliterator*, UBool, USet*, UErrorCode*))syms[415];
  return ptr(trans, ignoreFilter, fillIn, status);
}

/* unicode/utypes.h */
const char* u_errorName(UErrorCode code) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UErrorCode);
  ptr = (const char*(*)(UErrorCode))syms[416];
  return ptr(code);
}

/* unicode/ucnv_err.h */
void UCNV_FROM_U_CALLBACK_STOP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  if (syms[417] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[417];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_STOP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  if (syms[418] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[418];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SKIP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  if (syms[419] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[419];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_SUBSTITUTE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  if (syms[420] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[420];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_FROM_U_CALLBACK_ESCAPE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*);
  if (syms[421] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterFromUnicodeArgs*, const UChar*, int32_t, UChar32, UConverterCallbackReason, UErrorCode*))syms[421];
  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SKIP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  if (syms[422] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[422];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_SUBSTITUTE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  if (syms[423] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[423];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

void UCNV_TO_U_CALLBACK_ESCAPE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*);
  if (syms[424] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UConverterToUnicodeArgs*, const char*, int32_t, UConverterCallbackReason, UErrorCode*))syms[424];
  ptr(context, toUArgs, codeUnits, length, reason, err);
  return;
}

/* unicode/uloc.h */
const char* uloc_getDefault(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[425];
  return ptr();
}

void uloc_setDefault(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, UErrorCode*);
  if (syms[426] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, UErrorCode*))syms[426];
  ptr(localeID, status);
  return;
}

int32_t uloc_getLanguage(const char* localeID, char* language, int32_t languageCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[427] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[427];
  return ptr(localeID, language, languageCapacity, err);
}

int32_t uloc_getScript(const char* localeID, char* script, int32_t scriptCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[428] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[428];
  return ptr(localeID, script, scriptCapacity, err);
}

int32_t uloc_getCountry(const char* localeID, char* country, int32_t countryCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[429] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[429];
  return ptr(localeID, country, countryCapacity, err);
}

int32_t uloc_getVariant(const char* localeID, char* variant, int32_t variantCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[430] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[430];
  return ptr(localeID, variant, variantCapacity, err);
}

int32_t uloc_getName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[431] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[431];
  return ptr(localeID, name, nameCapacity, err);
}

int32_t uloc_canonicalize(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[432] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[432];
  return ptr(localeID, name, nameCapacity, err);
}

const char* uloc_getISO3Language(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*);
  ptr = (const char*(*)(const char*))syms[433];
  return ptr(localeID);
}

const char* uloc_getISO3Country(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*);
  ptr = (const char*(*)(const char*))syms[434];
  return ptr(localeID);
}

uint32_t uloc_getLCID(const char* localeID) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const char*);
  ptr = (uint32_t(*)(const char*))syms[435];
  return ptr(localeID);
}

int32_t uloc_getDisplayLanguage(const char* locale, const char* displayLocale, UChar* language, int32_t languageCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[436] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[436];
  return ptr(locale, displayLocale, language, languageCapacity, status);
}

int32_t uloc_getDisplayScript(const char* locale, const char* displayLocale, UChar* script, int32_t scriptCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[437] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[437];
  return ptr(locale, displayLocale, script, scriptCapacity, status);
}

int32_t uloc_getDisplayCountry(const char* locale, const char* displayLocale, UChar* country, int32_t countryCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[438] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[438];
  return ptr(locale, displayLocale, country, countryCapacity, status);
}

int32_t uloc_getDisplayVariant(const char* locale, const char* displayLocale, UChar* variant, int32_t variantCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[439] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[439];
  return ptr(locale, displayLocale, variant, variantCapacity, status);
}

int32_t uloc_getDisplayKeyword(const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[440] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[440];
  return ptr(keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayKeywordValue(const char* locale, const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[441] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, const char*, UChar*, int32_t, UErrorCode*))syms[441];
  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}

int32_t uloc_getDisplayName(const char* localeID, const char* inLocaleID, UChar* result, int32_t maxResultSize, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[442] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, UChar*, int32_t, UErrorCode*))syms[442];
  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}

const char* uloc_getAvailable(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[443];
  return ptr(n);
}

int32_t uloc_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[444];
  return ptr();
}

const char* const* uloc_getISOLanguages(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char* const* (*ptr)(void);
  ptr = (const char* const*(*)(void))syms[445];
  return ptr();
}

const char* const* uloc_getISOCountries(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char* const* (*ptr)(void);
  ptr = (const char* const*(*)(void))syms[446];
  return ptr();
}

int32_t uloc_getParent(const char* localeID, char* parent, int32_t parentCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[447] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[447];
  return ptr(localeID, parent, parentCapacity, err);
}

int32_t uloc_getBaseName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[448] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[448];
  return ptr(localeID, name, nameCapacity, err);
}

UEnumeration* uloc_openKeywords(const char* localeID, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  if (syms[449] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[449];
  return ptr(localeID, status);
}

int32_t uloc_getKeywordValue(const char* localeID, const char* keywordName, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  if (syms[450] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))syms[450];
  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}

int32_t uloc_setKeywordValue(const char* keywordName, const char* keywordValue, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, UErrorCode*);
  if (syms[451] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, UErrorCode*))syms[451];
  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}

ULayoutType uloc_getCharacterOrientation(const char* localeId, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULayoutType (*ptr)(const char*, UErrorCode*);
  if (syms[452] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (ULayoutType)0;
  }
  ptr = (ULayoutType(*)(const char*, UErrorCode*))syms[452];
  return ptr(localeId, status);
}

ULayoutType uloc_getLineOrientation(const char* localeId, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  ULayoutType (*ptr)(const char*, UErrorCode*);
  if (syms[453] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (ULayoutType)0;
  }
  ptr = (ULayoutType(*)(const char*, UErrorCode*))syms[453];
  return ptr(localeId, status);
}

int32_t uloc_acceptLanguageFromHTTP(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char* httpAcceptLanguage, UEnumeration* availableLocales, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*);
  if (syms[454] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char*, UEnumeration*, UErrorCode*))syms[454];
  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}

int32_t uloc_acceptLanguage(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char** acceptList, int32_t acceptListCount, UEnumeration* availableLocales, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*);
  if (syms[455] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(char*, int32_t, UAcceptResult*, const char**, int32_t, UEnumeration*, UErrorCode*))syms[455];
  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}

int32_t uloc_getLocaleForLCID(uint32_t hostID, char* locale, int32_t localeCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(uint32_t, char*, int32_t, UErrorCode*);
  if (syms[456] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(uint32_t, char*, int32_t, UErrorCode*))syms[456];
  return ptr(hostID, locale, localeCapacity, status);
}

int32_t uloc_addLikelySubtags(const char* localeID, char* maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[457] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[457];
  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}

int32_t uloc_minimizeSubtags(const char* localeID, char* minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UErrorCode*);
  if (syms[458] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UErrorCode*))syms[458];
  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}

int32_t uloc_forLanguageTag(const char* langtag, char* localeID, int32_t localeIDCapacity, int32_t* parsedLength, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, int32_t*, UErrorCode*);
  if (syms[459] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, int32_t*, UErrorCode*))syms[459];
  return ptr(langtag, localeID, localeIDCapacity, parsedLength, err);
}

int32_t uloc_toLanguageTag(const char* localeID, char* langtag, int32_t langtagCapacity, UBool strict, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, char*, int32_t, UBool, UErrorCode*);
  if (syms[460] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, char*, int32_t, UBool, UErrorCode*))syms[460];
  return ptr(localeID, langtag, langtagCapacity, strict, err);
}

/* unicode/uchar.h */
UBool u_hasBinaryProperty(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32, UProperty);
  ptr = (UBool(*)(UChar32, UProperty))syms[461];
  return ptr(c, which);
}

UBool u_isUAlphabetic(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[462];
  return ptr(c);
}

UBool u_isULowercase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[463];
  return ptr(c);
}

UBool u_isUUppercase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[464];
  return ptr(c);
}

UBool u_isUWhiteSpace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[465];
  return ptr(c);
}

int32_t u_getIntPropertyValue(UChar32 c, UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UProperty);
  ptr = (int32_t(*)(UChar32, UProperty))syms[466];
  return ptr(c, which);
}

int32_t u_getIntPropertyMinValue(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty);
  ptr = (int32_t(*)(UProperty))syms[467];
  return ptr(which);
}

int32_t u_getIntPropertyMaxValue(UProperty which) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty);
  ptr = (int32_t(*)(UProperty))syms[468];
  return ptr(which);
}

double u_getNumericValue(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  double (*ptr)(UChar32);
  ptr = (double(*)(UChar32))syms[469];
  return ptr(c);
}

UBool u_islower(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[470];
  return ptr(c);
}

UBool u_isupper(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[471];
  return ptr(c);
}

UBool u_istitle(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[472];
  return ptr(c);
}

UBool u_isdigit(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[473];
  return ptr(c);
}

UBool u_isalpha(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[474];
  return ptr(c);
}

UBool u_isalnum(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[475];
  return ptr(c);
}

UBool u_isxdigit(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[476];
  return ptr(c);
}

UBool u_ispunct(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[477];
  return ptr(c);
}

UBool u_isgraph(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[478];
  return ptr(c);
}

UBool u_isblank(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[479];
  return ptr(c);
}

UBool u_isdefined(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[480];
  return ptr(c);
}

UBool u_isspace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[481];
  return ptr(c);
}

UBool u_isJavaSpaceChar(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[482];
  return ptr(c);
}

UBool u_isWhitespace(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[483];
  return ptr(c);
}

UBool u_iscntrl(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[484];
  return ptr(c);
}

UBool u_isISOControl(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[485];
  return ptr(c);
}

UBool u_isprint(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[486];
  return ptr(c);
}

UBool u_isbase(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[487];
  return ptr(c);
}

UCharDirection u_charDirection(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UCharDirection (*ptr)(UChar32);
  ptr = (UCharDirection(*)(UChar32))syms[488];
  return ptr(c);
}

UBool u_isMirrored(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[489];
  return ptr(c);
}

UChar32 u_charMirror(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[490];
  return ptr(c);
}

UChar32 u_getBidiPairedBracket(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[491];
  return ptr(c);
}

int8_t u_charType(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(UChar32);
  ptr = (int8_t(*)(UChar32))syms[492];
  return ptr(c);
}

void u_enumCharTypes(UCharEnumTypeRange* enumRange, const void* context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharEnumTypeRange*, const void*);
  ptr = (void(*)(UCharEnumTypeRange*, const void*))syms[493];
  ptr(enumRange, context);
  return;
}

uint8_t u_getCombiningClass(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  uint8_t (*ptr)(UChar32);
  ptr = (uint8_t(*)(UChar32))syms[494];
  return ptr(c);
}

int32_t u_charDigitValue(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32);
  ptr = (int32_t(*)(UChar32))syms[495];
  return ptr(c);
}

UBlockCode ublock_getCode(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBlockCode (*ptr)(UChar32);
  ptr = (UBlockCode(*)(UChar32))syms[496];
  return ptr(c);
}

int32_t u_charName(UChar32 code, UCharNameChoice nameChoice, char* buffer, int32_t bufferLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*);
  if (syms[497] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar32, UCharNameChoice, char*, int32_t, UErrorCode*))syms[497];
  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}

UChar32 u_charFromName(UCharNameChoice nameChoice, const char* name, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharNameChoice, const char*, UErrorCode*);
  if (syms[498] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar32)0;
  }
  ptr = (UChar32(*)(UCharNameChoice, const char*, UErrorCode*))syms[498];
  return ptr(nameChoice, name, pErrorCode);
}

void u_enumCharNames(UChar32 start, UChar32 limit, UEnumCharNamesFn* fn, void* context, UCharNameChoice nameChoice, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*);
  if (syms[499] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UChar32, UChar32, UEnumCharNamesFn*, void*, UCharNameChoice, UErrorCode*))syms[499];
  ptr(start, limit, fn, context, nameChoice, pErrorCode);
  return;
}

const char* u_getPropertyName(UProperty property, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UProperty, UPropertyNameChoice);
  ptr = (const char*(*)(UProperty, UPropertyNameChoice))syms[500];
  return ptr(property, nameChoice);
}

UProperty u_getPropertyEnum(const char* alias) {
  pthread_once(&once_control, &init_icudata_version);
  UProperty (*ptr)(const char*);
  ptr = (UProperty(*)(const char*))syms[501];
  return ptr(alias);
}

const char* u_getPropertyValueName(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UProperty, int32_t, UPropertyNameChoice);
  ptr = (const char*(*)(UProperty, int32_t, UPropertyNameChoice))syms[502];
  return ptr(property, value, nameChoice);
}

int32_t u_getPropertyValueEnum(UProperty property, const char* alias) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UProperty, const char*);
  ptr = (int32_t(*)(UProperty, const char*))syms[503];
  return ptr(property, alias);
}

UBool u_isIDStart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[504];
  return ptr(c);
}

UBool u_isIDPart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[505];
  return ptr(c);
}

UBool u_isIDIgnorable(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[506];
  return ptr(c);
}

UBool u_isJavaIDStart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[507];
  return ptr(c);
}

UBool u_isJavaIDPart(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32);
  ptr = (UBool(*)(UChar32))syms[508];
  return ptr(c);
}

UChar32 u_tolower(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[509];
  return ptr(c);
}

UChar32 u_toupper(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[510];
  return ptr(c);
}

UChar32 u_totitle(UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32);
  ptr = (UChar32(*)(UChar32))syms[511];
  return ptr(c);
}

UChar32 u_foldCase(UChar32 c, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UChar32, uint32_t);
  ptr = (UChar32(*)(UChar32, uint32_t))syms[512];
  return ptr(c, options);
}

int32_t u_digit(UChar32 ch, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, int8_t);
  ptr = (int32_t(*)(UChar32, int8_t))syms[513];
  return ptr(ch, radix);
}

UChar32 u_forDigit(int32_t digit, int8_t radix) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(int32_t, int8_t);
  ptr = (UChar32(*)(int32_t, int8_t))syms[514];
  return ptr(digit, radix);
}

void u_charAge(UChar32 c, UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UChar32, UVersionInfo);
  ptr = (void(*)(UChar32, UVersionInfo))syms[515];
  ptr(c, versionArray);
  return;
}

void u_getUnicodeVersion(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo);
  ptr = (void(*)(UVersionInfo))syms[516];
  ptr(versionArray);
  return;
}

int32_t u_getFC_NFKC_Closure(UChar32 c, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UChar*, int32_t, UErrorCode*);
  if (syms[517] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar32, UChar*, int32_t, UErrorCode*))syms[517];
  return ptr(c, dest, destCapacity, pErrorCode);
}

/* unicode/ucnvsel.h */
UConverterSelector* ucnvsel_open(const char* const* converterList, int32_t converterListSize, const USet* excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterSelector* (*ptr)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*);
  if (syms[518] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UConverterSelector*)0;
  }
  ptr = (UConverterSelector*(*)(const char* const*, int32_t, const USet*, const UConverterUnicodeSet, UErrorCode*))syms[518];
  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}

void ucnvsel_close(UConverterSelector* sel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterSelector*);
  ptr = (void(*)(UConverterSelector*))syms[519];
  ptr(sel);
  return;
}

UConverterSelector* ucnvsel_openFromSerialized(const void* buffer, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterSelector* (*ptr)(const void*, int32_t, UErrorCode*);
  if (syms[520] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UConverterSelector*)0;
  }
  ptr = (UConverterSelector*(*)(const void*, int32_t, UErrorCode*))syms[520];
  return ptr(buffer, length, status);
}

int32_t ucnvsel_serialize(const UConverterSelector* sel, void* buffer, int32_t bufferCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverterSelector*, void*, int32_t, UErrorCode*);
  if (syms[521] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UConverterSelector*, void*, int32_t, UErrorCode*))syms[521];
  return ptr(sel, buffer, bufferCapacity, status);
}

UEnumeration* ucnvsel_selectForString(const UConverterSelector* sel, const UChar* s, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*);
  if (syms[522] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UConverterSelector*, const UChar*, int32_t, UErrorCode*))syms[522];
  return ptr(sel, s, length, status);
}

UEnumeration* ucnvsel_selectForUTF8(const UConverterSelector* sel, const char* s, int32_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UConverterSelector*, const char*, int32_t, UErrorCode*);
  if (syms[523] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UConverterSelector*, const char*, int32_t, UErrorCode*))syms[523];
  return ptr(sel, s, length, status);
}

/* unicode/uclean.h */
void u_init(UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UErrorCode*);
  if (syms[524] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UErrorCode*))syms[524];
  ptr(status);
  return;
}

void u_cleanup(void) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(void);
  ptr = (void(*)(void))syms[525];
  ptr();
  return;
}

void u_setMemoryFunctions(const void* context, UMemAllocFn* a, UMemReallocFn* r, UMemFreeFn* f, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UMemAllocFn*, UMemReallocFn*, UMemFreeFn*, UErrorCode*);
  if (syms[526] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UMemAllocFn*, UMemReallocFn*, UMemFreeFn*, UErrorCode*))syms[526];
  ptr(context, a, r, f, status);
  return;
}

/* unicode/utrace.h */
void utrace_setLevel(int32_t traceLevel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(int32_t);
  ptr = (void(*)(int32_t))syms[527];
  ptr(traceLevel);
  return;
}

int32_t utrace_getLevel(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[528];
  return ptr();
}

void utrace_setFunctions(const void* context, UTraceEntry* e, UTraceExit* x, UTraceData* d) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UTraceEntry*, UTraceExit*, UTraceData*);
  ptr = (void(*)(const void*, UTraceEntry*, UTraceExit*, UTraceData*))syms[529];
  ptr(context, e, x, d);
  return;
}

void utrace_getFunctions(const void** context, UTraceEntry** e, UTraceExit** x, UTraceData** d) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void**, UTraceEntry**, UTraceExit**, UTraceData**);
  ptr = (void(*)(const void**, UTraceEntry**, UTraceExit**, UTraceData**))syms[530];
  ptr(context, e, x, d);
  return;
}

int32_t utrace_vformat(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, va_list args) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, va_list);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, va_list))syms[531];
  return ptr(outBuf, capacity, indent, fmt, args);
}

int32_t utrace_format(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, ...) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(char*, int32_t, int32_t, const char*, va_list);
  ptr = (int32_t(*)(char*, int32_t, int32_t, const char*, va_list))syms[532];
  va_list args;
  va_start(args, fmt);
  int32_t ret = ptr(outBuf, capacity, indent, fmt, args);
  va_end(args);
  return ret;
}

const char* utrace_functionName(int32_t fnNumber) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[533];
  return ptr(fnNumber);
}

/* unicode/uset.h */
USet* uset_openEmpty(void) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(void);
  ptr = (USet*(*)(void))syms[534];
  return ptr();
}

USet* uset_open(UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(UChar32, UChar32);
  ptr = (USet*(*)(UChar32, UChar32))syms[535];
  return ptr(start, end);
}

USet* uset_openPattern(const UChar* pattern, int32_t patternLength, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const UChar*, int32_t, UErrorCode*);
  if (syms[536] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (USet*)0;
  }
  ptr = (USet*(*)(const UChar*, int32_t, UErrorCode*))syms[536];
  return ptr(pattern, patternLength, ec);
}

USet* uset_openPatternOptions(const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[537] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (USet*)0;
  }
  ptr = (USet*(*)(const UChar*, int32_t, uint32_t, UErrorCode*))syms[537];
  return ptr(pattern, patternLength, options, ec);
}

void uset_close(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[538];
  ptr(set);
  return;
}

USet* uset_clone(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const USet*);
  ptr = (USet*(*)(const USet*))syms[539];
  return ptr(set);
}

UBool uset_isFrozen(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*);
  ptr = (UBool(*)(const USet*))syms[540];
  return ptr(set);
}

void uset_freeze(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[541];
  ptr(set);
  return;
}

USet* uset_cloneAsThawed(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  USet* (*ptr)(const USet*);
  ptr = (USet*(*)(const USet*))syms[542];
  return ptr(set);
}

void uset_set(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[543];
  ptr(set, start, end);
  return;
}

int32_t uset_applyPattern(USet* set, const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[544] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(USet*, const UChar*, int32_t, uint32_t, UErrorCode*))syms[544];
  return ptr(set, pattern, patternLength, options, status);
}

void uset_applyIntPropertyValue(USet* set, UProperty prop, int32_t value, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UProperty, int32_t, UErrorCode*);
  if (syms[545] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(USet*, UProperty, int32_t, UErrorCode*))syms[545];
  ptr(set, prop, value, ec);
  return;
}

void uset_applyPropertyAlias(USet* set, const UChar* prop, int32_t propLength, const UChar* value, int32_t valueLength, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[546] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(USet*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[546];
  ptr(set, prop, propLength, value, valueLength, ec);
  return;
}

UBool uset_resemblesPattern(const UChar* pattern, int32_t patternLength, int32_t pos) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))syms[547];
  return ptr(pattern, patternLength, pos);
}

int32_t uset_toPattern(const USet* set, UChar* result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, UChar*, int32_t, UBool, UErrorCode*);
  if (syms[548] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USet*, UChar*, int32_t, UBool, UErrorCode*))syms[548];
  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}

void uset_add(USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32);
  ptr = (void(*)(USet*, UChar32))syms[549];
  ptr(set, c);
  return;
}

void uset_addAll(USet* set, const USet* additionalSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[550];
  ptr(set, additionalSet);
  return;
}

void uset_addRange(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[551];
  ptr(set, start, end);
  return;
}

void uset_addString(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[552];
  ptr(set, str, strLen);
  return;
}

void uset_addAllCodePoints(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[553];
  ptr(set, str, strLen);
  return;
}

void uset_remove(USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32);
  ptr = (void(*)(USet*, UChar32))syms[554];
  ptr(set, c);
  return;
}

void uset_removeRange(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[555];
  ptr(set, start, end);
  return;
}

void uset_removeString(USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const UChar*, int32_t);
  ptr = (void(*)(USet*, const UChar*, int32_t))syms[556];
  ptr(set, str, strLen);
  return;
}

void uset_removeAll(USet* set, const USet* removeSet) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[557];
  ptr(set, removeSet);
  return;
}

void uset_retain(USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, UChar32, UChar32);
  ptr = (void(*)(USet*, UChar32, UChar32))syms[558];
  ptr(set, start, end);
  return;
}

void uset_retainAll(USet* set, const USet* retain) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[559];
  ptr(set, retain);
  return;
}

void uset_compact(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[560];
  ptr(set);
  return;
}

void uset_complement(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[561];
  ptr(set);
  return;
}

void uset_complementAll(USet* set, const USet* complement) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, const USet*);
  ptr = (void(*)(USet*, const USet*))syms[562];
  ptr(set, complement);
  return;
}

void uset_clear(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[563];
  ptr(set);
  return;
}

void uset_closeOver(USet* set, int32_t attributes) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*, int32_t);
  ptr = (void(*)(USet*, int32_t))syms[564];
  ptr(set, attributes);
  return;
}

void uset_removeAllStrings(USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USet*);
  ptr = (void(*)(USet*))syms[565];
  ptr(set);
  return;
}

UBool uset_isEmpty(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*);
  ptr = (UBool(*)(const USet*))syms[566];
  return ptr(set);
}

UBool uset_contains(const USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, UChar32);
  ptr = (UBool(*)(const USet*, UChar32))syms[567];
  return ptr(set, c);
}

UBool uset_containsRange(const USet* set, UChar32 start, UChar32 end) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, UChar32, UChar32);
  ptr = (UBool(*)(const USet*, UChar32, UChar32))syms[568];
  return ptr(set, start, end);
}

UBool uset_containsString(const USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))syms[569];
  return ptr(set, str, strLen);
}

int32_t uset_indexOf(const USet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, UChar32);
  ptr = (int32_t(*)(const USet*, UChar32))syms[570];
  return ptr(set, c);
}

UChar32 uset_charAt(const USet* set, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(const USet*, int32_t);
  ptr = (UChar32(*)(const USet*, int32_t))syms[571];
  return ptr(set, charIndex);
}

int32_t uset_size(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*);
  ptr = (int32_t(*)(const USet*))syms[572];
  return ptr(set);
}

int32_t uset_getItemCount(const USet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*);
  ptr = (int32_t(*)(const USet*))syms[573];
  return ptr(set);
}

int32_t uset_getItem(const USet* set, int32_t itemIndex, UChar32* start, UChar32* end, UChar* str, int32_t strCapacity, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*);
  if (syms[574] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USet*, int32_t, UChar32*, UChar32*, UChar*, int32_t, UErrorCode*))syms[574];
  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}

UBool uset_containsAll(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[575];
  return ptr(set1, set2);
}

UBool uset_containsAllCodePoints(const USet* set, const UChar* str, int32_t strLen) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const UChar*, int32_t);
  ptr = (UBool(*)(const USet*, const UChar*, int32_t))syms[576];
  return ptr(set, str, strLen);
}

UBool uset_containsNone(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[577];
  return ptr(set1, set2);
}

UBool uset_containsSome(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[578];
  return ptr(set1, set2);
}

int32_t uset_span(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))syms[579];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBack(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const UChar*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const UChar*, int32_t, USetSpanCondition))syms[580];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))syms[581];
  return ptr(set, s, length, spanCondition);
}

int32_t uset_spanBackUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, const char*, int32_t, USetSpanCondition);
  ptr = (int32_t(*)(const USet*, const char*, int32_t, USetSpanCondition))syms[582];
  return ptr(set, s, length, spanCondition);
}

UBool uset_equals(const USet* set1, const USet* set2) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USet*, const USet*);
  ptr = (UBool(*)(const USet*, const USet*))syms[583];
  return ptr(set1, set2);
}

int32_t uset_serialize(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USet*, uint16_t*, int32_t, UErrorCode*);
  if (syms[584] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const USet*, uint16_t*, int32_t, UErrorCode*))syms[584];
  return ptr(set, dest, destCapacity, pErrorCode);
}

UBool uset_getSerializedSet(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(USerializedSet*, const uint16_t*, int32_t);
  ptr = (UBool(*)(USerializedSet*, const uint16_t*, int32_t))syms[585];
  return ptr(fillSet, src, srcLength);
}

void uset_setSerializedToOne(USerializedSet* fillSet, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(USerializedSet*, UChar32);
  ptr = (void(*)(USerializedSet*, UChar32))syms[586];
  ptr(fillSet, c);
  return;
}

UBool uset_serializedContains(const USerializedSet* set, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USerializedSet*, UChar32);
  ptr = (UBool(*)(const USerializedSet*, UChar32))syms[587];
  return ptr(set, c);
}

int32_t uset_getSerializedRangeCount(const USerializedSet* set) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const USerializedSet*);
  ptr = (int32_t(*)(const USerializedSet*))syms[588];
  return ptr(set);
}

UBool uset_getSerializedRange(const USerializedSet* set, int32_t rangeIndex, UChar32* pStart, UChar32* pEnd) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const USerializedSet*, int32_t, UChar32*, UChar32*);
  ptr = (UBool(*)(const USerializedSet*, int32_t, UChar32*, UChar32*))syms[589];
  return ptr(set, rangeIndex, pStart, pEnd);
}

/* unicode/uenum.h */
void uenum_close(UEnumeration* en) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UEnumeration*);
  ptr = (void(*)(UEnumeration*))syms[590];
  ptr(en);
  return;
}

int32_t uenum_count(UEnumeration* en, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UEnumeration*, UErrorCode*);
  if (syms[591] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UEnumeration*, UErrorCode*))syms[591];
  return ptr(en, status);
}

const UChar* uenum_unext(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  if (syms[592] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(UEnumeration*, int32_t*, UErrorCode*))syms[592];
  return ptr(en, resultLength, status);
}

const char* uenum_next(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UEnumeration*, int32_t*, UErrorCode*);
  if (syms[593] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(UEnumeration*, int32_t*, UErrorCode*))syms[593];
  return ptr(en, resultLength, status);
}

void uenum_reset(UEnumeration* en, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UEnumeration*, UErrorCode*);
  if (syms[594] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UEnumeration*, UErrorCode*))syms[594];
  ptr(en, status);
  return;
}

UEnumeration* uenum_openUCharStringsEnumeration(const UChar* const strings [], int32_t count, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const UChar* const [], int32_t, UErrorCode*);
  if (syms[595] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const UChar* const [], int32_t, UErrorCode*))syms[595];
  return ptr(strings, count, ec);
}

UEnumeration* uenum_openCharStringsEnumeration(const char* const strings [], int32_t count, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char* const [], int32_t, UErrorCode*);
  if (syms[596] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char* const [], int32_t, UErrorCode*))syms[596];
  return ptr(strings, count, ec);
}

/* unicode/ucnv.h */
int ucnv_compareNames(const char* name1, const char* name2) {
  pthread_once(&once_control, &init_icudata_version);
  int (*ptr)(const char*, const char*);
  ptr = (int(*)(const char*, const char*))syms[597];
  return ptr(name1, name2);
}

UConverter* ucnv_open(const char* converterName, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter* (*ptr)(const char*, UErrorCode*);
  if (syms[598] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UConverter*)0;
  }
  ptr = (UConverter*(*)(const char*, UErrorCode*))syms[598];
  return ptr(converterName, err);
}

UConverter* ucnv_openU(const UChar* name, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter* (*ptr)(const UChar*, UErrorCode*);
  if (syms[599] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UConverter*)0;
  }
  ptr = (UConverter*(*)(const UChar*, UErrorCode*))syms[599];
  return ptr(name, err);
}

UConverter* ucnv_openCCSID(int32_t codepage, UConverterPlatform platform, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter* (*ptr)(int32_t, UConverterPlatform, UErrorCode*);
  if (syms[600] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UConverter*)0;
  }
  ptr = (UConverter*(*)(int32_t, UConverterPlatform, UErrorCode*))syms[600];
  return ptr(codepage, platform, err);
}

UConverter* ucnv_openPackage(const char* packageName, const char* converterName, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[601] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UConverter*)0;
  }
  ptr = (UConverter*(*)(const char*, const char*, UErrorCode*))syms[601];
  return ptr(packageName, converterName, err);
}

UConverter* ucnv_safeClone(const UConverter* cnv, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UConverter* (*ptr)(const UConverter*, void*, int32_t*, UErrorCode*);
  if (syms[602] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UConverter*)0;
  }
  ptr = (UConverter*(*)(const UConverter*, void*, int32_t*, UErrorCode*))syms[602];
  return ptr(cnv, stackBuffer, pBufferSize, status);
}

void ucnv_close(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[603];
  ptr(converter);
  return;
}

void ucnv_getSubstChars(const UConverter* converter, char* subChars, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  if (syms[604] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))syms[604];
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstChars(UConverter* converter, const char* subChars, int8_t len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, const char*, int8_t, UErrorCode*);
  if (syms[605] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, const char*, int8_t, UErrorCode*))syms[605];
  ptr(converter, subChars, len, err);
  return;
}

void ucnv_setSubstString(UConverter* cnv, const UChar* s, int32_t length, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, const UChar*, int32_t, UErrorCode*);
  if (syms[606] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, const UChar*, int32_t, UErrorCode*))syms[606];
  ptr(cnv, s, length, err);
  return;
}

void ucnv_getInvalidChars(const UConverter* converter, char* errBytes, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, char*, int8_t*, UErrorCode*);
  if (syms[607] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UConverter*, char*, int8_t*, UErrorCode*))syms[607];
  ptr(converter, errBytes, len, err);
  return;
}

void ucnv_getInvalidUChars(const UConverter* converter, UChar* errUChars, int8_t* len, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UChar*, int8_t*, UErrorCode*);
  if (syms[608] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UConverter*, UChar*, int8_t*, UErrorCode*))syms[608];
  ptr(converter, errUChars, len, err);
  return;
}

void ucnv_reset(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[609];
  ptr(converter);
  return;
}

void ucnv_resetToUnicode(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[610];
  ptr(converter);
  return;
}

void ucnv_resetFromUnicode(UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*);
  ptr = (void(*)(UConverter*))syms[611];
  ptr(converter);
  return;
}

int8_t ucnv_getMaxCharSize(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(const UConverter*);
  ptr = (int8_t(*)(const UConverter*))syms[612];
  return ptr(converter);
}

int8_t ucnv_getMinCharSize(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  int8_t (*ptr)(const UConverter*);
  ptr = (int8_t(*)(const UConverter*))syms[613];
  return ptr(converter);
}

int32_t ucnv_getDisplayName(const UConverter* converter, const char* displayLocale, UChar* displayName, int32_t displayNameCapacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*);
  if (syms[614] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UConverter*, const char*, UChar*, int32_t, UErrorCode*))syms[614];
  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}

const char* ucnv_getName(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UConverter*, UErrorCode*);
  if (syms[615] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UConverter*, UErrorCode*))syms[615];
  return ptr(converter, err);
}

int32_t ucnv_getCCSID(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  if (syms[616] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[616];
  return ptr(converter, err);
}

UConverterPlatform ucnv_getPlatform(const UConverter* converter, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterPlatform (*ptr)(const UConverter*, UErrorCode*);
  if (syms[617] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UConverterPlatform)0;
  }
  ptr = (UConverterPlatform(*)(const UConverter*, UErrorCode*))syms[617];
  return ptr(converter, err);
}

UConverterType ucnv_getType(const UConverter* converter) {
  pthread_once(&once_control, &init_icudata_version);
  UConverterType (*ptr)(const UConverter*);
  ptr = (UConverterType(*)(const UConverter*))syms[618];
  return ptr(converter);
}

void ucnv_getStarters(const UConverter* converter, UBool starters [ 256], UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UBool [ 256], UErrorCode*);
  if (syms[619] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UConverter*, UBool [ 256], UErrorCode*))syms[619];
  ptr(converter, starters, err);
  return;
}

void ucnv_getUnicodeSet(const UConverter* cnv, USet* setFillIn, UConverterUnicodeSet whichSet, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*);
  if (syms[620] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UConverter*, USet*, UConverterUnicodeSet, UErrorCode*))syms[620];
  ptr(cnv, setFillIn, whichSet, pErrorCode);
  return;
}

void ucnv_getToUCallBack(const UConverter* converter, UConverterToUCallback* action, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UConverterToUCallback*, const void**);
  ptr = (void(*)(const UConverter*, UConverterToUCallback*, const void**))syms[621];
  ptr(converter, action, context);
  return;
}

void ucnv_getFromUCallBack(const UConverter* converter, UConverterFromUCallback* action, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UConverterFromUCallback*, const void**);
  ptr = (void(*)(const UConverter*, UConverterFromUCallback*, const void**))syms[622];
  ptr(converter, action, context);
  return;
}

void ucnv_setToUCallBack(UConverter* converter, UConverterToUCallback newAction, const void* newContext, UConverterToUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*);
  if (syms[623] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, UConverterToUCallback, const void*, UConverterToUCallback*, const void**, UErrorCode*))syms[623];
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_setFromUCallBack(UConverter* converter, UConverterFromUCallback newAction, const void* newContext, UConverterFromUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*);
  if (syms[624] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, UConverterFromUCallback, const void*, UConverterFromUCallback*, const void**, UErrorCode*))syms[624];
  ptr(converter, newAction, newContext, oldAction, oldContext, err);
  return;
}

void ucnv_fromUnicode(UConverter* converter, char** target, const char* targetLimit, const UChar** source, const UChar* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*);
  if (syms[625] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, char**, const char*, const UChar**, const UChar*, int32_t*, UBool, UErrorCode*))syms[625];
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

void ucnv_toUnicode(UConverter* converter, UChar** target, const UChar* targetLimit, const char** source, const char* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*);
  if (syms[626] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, UChar**, const UChar*, const char**, const char*, int32_t*, UBool, UErrorCode*))syms[626];
  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
  return;
}

int32_t ucnv_fromUChars(UConverter* cnv, char* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[627] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UConverter*, char*, int32_t, const UChar*, int32_t, UErrorCode*))syms[627];
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucnv_toUChars(UConverter* cnv, UChar* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[628] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UConverter*, UChar*, int32_t, const char*, int32_t, UErrorCode*))syms[628];
  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}

UChar32 ucnv_getNextUChar(UConverter* converter, const char** source, const char* sourceLimit, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UConverter*, const char**, const char*, UErrorCode*);
  if (syms[629] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UChar32)0;
  }
  ptr = (UChar32(*)(UConverter*, const char**, const char*, UErrorCode*))syms[629];
  return ptr(converter, source, sourceLimit, err);
}

void ucnv_convertEx(UConverter* targetCnv, UConverter* sourceCnv, char** target, const char* targetLimit, const char** source, const char* sourceLimit, UChar* pivotStart, UChar** pivotSource, UChar** pivotTarget, const UChar* pivotLimit, UBool reset, UBool flush, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*);
  if (syms[630] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverter*, UConverter*, char**, const char*, const char**, const char*, UChar*, UChar**, UChar**, const UChar*, UBool, UBool, UErrorCode*))syms[630];
  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
  return;
}

int32_t ucnv_convert(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[631] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, const char*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[631];
  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_toAlgorithmic(UConverterType algorithmicType, UConverter* cnv, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[632] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UConverterType, UConverter*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[632];
  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_fromAlgorithmic(UConverter* cnv, UConverterType algorithmicType, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[633] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UConverter*, UConverterType, char*, int32_t, const char*, int32_t, UErrorCode*))syms[633];
  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}

int32_t ucnv_flushCache(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[634];
  return ptr();
}

int32_t ucnv_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[635];
  return ptr();
}

const char* ucnv_getAvailableName(int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[636];
  return ptr(n);
}

UEnumeration* ucnv_openAllNames(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(UErrorCode*);
  if (syms[637] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(UErrorCode*))syms[637];
  return ptr(pErrorCode);
}

uint16_t ucnv_countAliases(const char* alias, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  uint16_t (*ptr)(const char*, UErrorCode*);
  if (syms[638] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (uint16_t)0;
  }
  ptr = (uint16_t(*)(const char*, UErrorCode*))syms[638];
  return ptr(alias, pErrorCode);
}

const char* ucnv_getAlias(const char* alias, uint16_t n, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*, uint16_t, UErrorCode*);
  if (syms[639] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const char*, uint16_t, UErrorCode*))syms[639];
  return ptr(alias, n, pErrorCode);
}

void ucnv_getAliases(const char* alias, const char** aliases, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const char**, UErrorCode*);
  if (syms[640] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const char**, UErrorCode*))syms[640];
  ptr(alias, aliases, pErrorCode);
  return;
}

UEnumeration* ucnv_openStandardNames(const char* convName, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[641] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, const char*, UErrorCode*))syms[641];
  return ptr(convName, standard, pErrorCode);
}

uint16_t ucnv_countStandards(void) {
  pthread_once(&once_control, &init_icudata_version);
  uint16_t (*ptr)(void);
  ptr = (uint16_t(*)(void))syms[642];
  return ptr();
}

const char* ucnv_getStandard(uint16_t n, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(uint16_t, UErrorCode*);
  if (syms[643] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(uint16_t, UErrorCode*))syms[643];
  return ptr(n, pErrorCode);
}

const char* ucnv_getStandardName(const char* name, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[644] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const char*, const char*, UErrorCode*))syms[644];
  return ptr(name, standard, pErrorCode);
}

const char* ucnv_getCanonicalName(const char* alias, const char* standard, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[645] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const char*, const char*, UErrorCode*))syms[645];
  return ptr(alias, standard, pErrorCode);
}

const char* ucnv_getDefaultName(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[646];
  return ptr();
}

void ucnv_setDefaultName(const char* name) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*);
  ptr = (void(*)(const char*))syms[647];
  ptr(name);
  return;
}

void ucnv_fixFileSeparator(const UConverter* cnv, UChar* source, int32_t sourceLen) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UConverter*, UChar*, int32_t);
  ptr = (void(*)(const UConverter*, UChar*, int32_t))syms[648];
  ptr(cnv, source, sourceLen);
  return;
}

UBool ucnv_isAmbiguous(const UConverter* cnv) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UConverter*);
  ptr = (UBool(*)(const UConverter*))syms[649];
  return ptr(cnv);
}

void ucnv_setFallback(UConverter* cnv, UBool usesFallback) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverter*, UBool);
  ptr = (void(*)(UConverter*, UBool))syms[650];
  ptr(cnv, usesFallback);
  return;
}

UBool ucnv_usesFallback(const UConverter* cnv) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UConverter*);
  ptr = (UBool(*)(const UConverter*))syms[651];
  return ptr(cnv);
}

const char* ucnv_detectUnicodeSignature(const char* source, int32_t sourceLength, int32_t* signatureLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const char*, int32_t, int32_t*, UErrorCode*);
  if (syms[652] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const char*, int32_t, int32_t*, UErrorCode*))syms[652];
  return ptr(source, sourceLength, signatureLength, pErrorCode);
}

int32_t ucnv_fromUCountPending(const UConverter* cnv, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  if (syms[653] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[653];
  return ptr(cnv, status);
}

int32_t ucnv_toUCountPending(const UConverter* cnv, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UConverter*, UErrorCode*);
  if (syms[654] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UConverter*, UErrorCode*))syms[654];
  return ptr(cnv, status);
}

UBool ucnv_isFixedWidth(UConverter* cnv, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UConverter*, UErrorCode*);
  if (syms[655] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(UConverter*, UErrorCode*))syms[655];
  return ptr(cnv, status);
}

/* unicode/uidna.h */
UIDNA* uidna_openUTS46(uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UIDNA* (*ptr)(uint32_t, UErrorCode*);
  if (syms[656] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UIDNA*)0;
  }
  ptr = (UIDNA*(*)(uint32_t, UErrorCode*))syms[656];
  return ptr(options, pErrorCode);
}

void uidna_close(UIDNA* idna) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UIDNA*);
  ptr = (void(*)(UIDNA*))syms[657];
  ptr(idna);
  return;
}

int32_t uidna_labelToASCII(const UIDNA* idna, const UChar* label, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[658] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*))syms[658];
  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_labelToUnicode(const UIDNA* idna, const UChar* label, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[659] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*))syms[659];
  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_nameToASCII(const UIDNA* idna, const UChar* name, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[660] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*))syms[660];
  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_nameToUnicode(const UIDNA* idna, const UChar* name, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[661] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const UChar*, int32_t, UChar*, int32_t, UIDNAInfo*, UErrorCode*))syms[661];
  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_labelToASCII_UTF8(const UIDNA* idna, const char* label, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[662] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*))syms[662];
  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_labelToUnicodeUTF8(const UIDNA* idna, const char* label, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[663] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*))syms[663];
  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_nameToASCII_UTF8(const UIDNA* idna, const char* name, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[664] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*))syms[664];
  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}

int32_t uidna_nameToUnicodeUTF8(const UIDNA* idna, const char* name, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*);
  if (syms[665] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UIDNA*, const char*, int32_t, char*, int32_t, UIDNAInfo*, UErrorCode*))syms[665];
  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}

/* unicode/ucnv_cb.h */
void ucnv_cbFromUWriteBytes(UConverterFromUnicodeArgs* args, const char* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*);
  if (syms[666] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverterFromUnicodeArgs*, const char*, int32_t, int32_t, UErrorCode*))syms[666];
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteSub(UConverterFromUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*);
  if (syms[667] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverterFromUnicodeArgs*, int32_t, UErrorCode*))syms[667];
  ptr(args, offsetIndex, err);
  return;
}

void ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs* args, const UChar** source, const UChar* sourceLimit, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*);
  if (syms[668] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverterFromUnicodeArgs*, const UChar**, const UChar*, int32_t, UErrorCode*))syms[668];
  ptr(args, source, sourceLimit, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteUChars(UConverterToUnicodeArgs* args, const UChar* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*);
  if (syms[669] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverterToUnicodeArgs*, const UChar*, int32_t, int32_t, UErrorCode*))syms[669];
  ptr(args, source, length, offsetIndex, err);
  return;
}

void ucnv_cbToUWriteSub(UConverterToUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UConverterToUnicodeArgs*, int32_t, UErrorCode*);
  if (syms[670] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UConverterToUnicodeArgs*, int32_t, UErrorCode*))syms[670];
  ptr(args, offsetIndex, err);
  return;
}

/* unicode/ures.h */
UResourceBundle* ures_open(const char* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[671] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(const char*, const char*, UErrorCode*))syms[671];
  return ptr(packageName, locale, status);
}

UResourceBundle* ures_openDirect(const char* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[672] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(const char*, const char*, UErrorCode*))syms[672];
  return ptr(packageName, locale, status);
}

UResourceBundle* ures_openU(const UChar* packageName, const char* locale, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(const UChar*, const char*, UErrorCode*);
  if (syms[673] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(const UChar*, const char*, UErrorCode*))syms[673];
  return ptr(packageName, locale, status);
}

void ures_close(UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UResourceBundle*);
  ptr = (void(*)(UResourceBundle*))syms[674];
  ptr(resourceBundle);
  return;
}

void ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UResourceBundle*, UVersionInfo);
  ptr = (void(*)(const UResourceBundle*, UVersionInfo))syms[675];
  ptr(resB, versionInfo);
  return;
}

const char* ures_getLocaleByType(const UResourceBundle* resourceBundle, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*);
  if (syms[676] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UResourceBundle*, ULocDataLocaleType, UErrorCode*))syms[676];
  return ptr(resourceBundle, type, status);
}

const UChar* ures_getString(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  if (syms[677] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[677];
  return ptr(resourceBundle, len, status);
}

const char* ures_getUTF8String(const UResourceBundle* resB, char* dest, int32_t* length, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*);
  if (syms[678] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UResourceBundle*, char*, int32_t*, UBool, UErrorCode*))syms[678];
  return ptr(resB, dest, length, forceCopy, status);
}

const uint8_t* ures_getBinary(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const uint8_t* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  if (syms[679] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const uint8_t*)0;
  }
  ptr = (const uint8_t*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[679];
  return ptr(resourceBundle, len, status);
}

const int32_t* ures_getIntVector(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const int32_t* (*ptr)(const UResourceBundle*, int32_t*, UErrorCode*);
  if (syms[680] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const int32_t*)0;
  }
  ptr = (const int32_t*(*)(const UResourceBundle*, int32_t*, UErrorCode*))syms[680];
  return ptr(resourceBundle, len, status);
}

uint32_t ures_getUInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  if (syms[681] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (uint32_t)0;
  }
  ptr = (uint32_t(*)(const UResourceBundle*, UErrorCode*))syms[681];
  return ptr(resourceBundle, status);
}

int32_t ures_getInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UResourceBundle*, UErrorCode*);
  if (syms[682] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UResourceBundle*, UErrorCode*))syms[682];
  return ptr(resourceBundle, status);
}

int32_t ures_getSize(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UResourceBundle*);
  ptr = (int32_t(*)(const UResourceBundle*))syms[683];
  return ptr(resourceBundle);
}

UResType ures_getType(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  UResType (*ptr)(const UResourceBundle*);
  ptr = (UResType(*)(const UResourceBundle*))syms[684];
  return ptr(resourceBundle);
}

const char* ures_getKey(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UResourceBundle*);
  ptr = (const char*(*)(const UResourceBundle*))syms[685];
  return ptr(resourceBundle);
}

void ures_resetIterator(UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UResourceBundle*);
  ptr = (void(*)(UResourceBundle*))syms[686];
  ptr(resourceBundle);
  return;
}

UBool ures_hasNext(const UResourceBundle* resourceBundle) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UResourceBundle*);
  ptr = (UBool(*)(const UResourceBundle*))syms[687];
  return ptr(resourceBundle);
}

UResourceBundle* ures_getNextResource(UResourceBundle* resourceBundle, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(UResourceBundle*, UResourceBundle*, UErrorCode*);
  if (syms[688] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(UResourceBundle*, UResourceBundle*, UErrorCode*))syms[688];
  return ptr(resourceBundle, fillIn, status);
}

const UChar* ures_getNextString(UResourceBundle* resourceBundle, int32_t* len, const char** key, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(UResourceBundle*, int32_t*, const char**, UErrorCode*);
  if (syms[689] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(UResourceBundle*, int32_t*, const char**, UErrorCode*))syms[689];
  return ptr(resourceBundle, len, key, status);
}

UResourceBundle* ures_getByIndex(const UResourceBundle* resourceBundle, int32_t indexR, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*);
  if (syms[690] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(const UResourceBundle*, int32_t, UResourceBundle*, UErrorCode*))syms[690];
  return ptr(resourceBundle, indexR, fillIn, status);
}

const UChar* ures_getStringByIndex(const UResourceBundle* resourceBundle, int32_t indexS, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*);
  if (syms[691] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const UResourceBundle*, int32_t, int32_t*, UErrorCode*))syms[691];
  return ptr(resourceBundle, indexS, len, status);
}

const char* ures_getUTF8StringByIndex(const UResourceBundle* resB, int32_t stringIndex, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*);
  if (syms[692] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UResourceBundle*, int32_t, char*, int32_t*, UBool, UErrorCode*))syms[692];
  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}

UResourceBundle* ures_getByKey(const UResourceBundle* resourceBundle, const char* key, UResourceBundle* fillIn, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UResourceBundle* (*ptr)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*);
  if (syms[693] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UResourceBundle*)0;
  }
  ptr = (UResourceBundle*(*)(const UResourceBundle*, const char*, UResourceBundle*, UErrorCode*))syms[693];
  return ptr(resourceBundle, key, fillIn, status);
}

const UChar* ures_getStringByKey(const UResourceBundle* resB, const char* key, int32_t* len, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UResourceBundle*, const char*, int32_t*, UErrorCode*);
  if (syms[694] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(const UResourceBundle*, const char*, int32_t*, UErrorCode*))syms[694];
  return ptr(resB, key, len, status);
}

const char* ures_getUTF8StringByKey(const UResourceBundle* resB, const char* key, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*);
  if (syms[695] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UResourceBundle*, const char*, char*, int32_t*, UBool, UErrorCode*))syms[695];
  return ptr(resB, key, dest, pLength, forceCopy, status);
}

UEnumeration* ures_openAvailableLocales(const char* packageName, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UEnumeration* (*ptr)(const char*, UErrorCode*);
  if (syms[696] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UEnumeration*)0;
  }
  ptr = (UEnumeration*(*)(const char*, UErrorCode*))syms[696];
  return ptr(packageName, status);
}

/* unicode/uscript.h */
int32_t uscript_getCode(const char* nameOrAbbrOrLocale, UScriptCode* fillIn, int32_t capacity, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UScriptCode*, int32_t, UErrorCode*);
  if (syms[697] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const char*, UScriptCode*, int32_t, UErrorCode*))syms[697];
  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}

const char* uscript_getName(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UScriptCode);
  ptr = (const char*(*)(UScriptCode))syms[698];
  return ptr(scriptCode);
}

const char* uscript_getShortName(UScriptCode scriptCode) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(UScriptCode);
  ptr = (const char*(*)(UScriptCode))syms[699];
  return ptr(scriptCode);
}

UScriptCode uscript_getScript(UChar32 codepoint, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  UScriptCode (*ptr)(UChar32, UErrorCode*);
  if (syms[700] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return (UScriptCode)0;
  }
  ptr = (UScriptCode(*)(UChar32, UErrorCode*))syms[700];
  return ptr(codepoint, err);
}

UBool uscript_hasScript(UChar32 c, UScriptCode sc) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UChar32, UScriptCode);
  ptr = (UBool(*)(UChar32, UScriptCode))syms[701];
  return ptr(c, sc);
}

int32_t uscript_getScriptExtensions(UChar32 c, UScriptCode* scripts, int32_t capacity, UErrorCode* errorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar32, UScriptCode*, int32_t, UErrorCode*);
  if (syms[702] == NULL) {
    *errorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar32, UScriptCode*, int32_t, UErrorCode*))syms[702];
  return ptr(c, scripts, capacity, errorCode);
}

int32_t uscript_getSampleString(UScriptCode script, UChar* dest, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UScriptCode, UChar*, int32_t, UErrorCode*);
  if (syms[703] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UScriptCode, UChar*, int32_t, UErrorCode*))syms[703];
  return ptr(script, dest, capacity, pErrorCode);
}

UScriptUsage uscript_getUsage(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);
  UScriptUsage (*ptr)(UScriptCode);
  ptr = (UScriptUsage(*)(UScriptCode))syms[704];
  return ptr(script);
}

UBool uscript_isRightToLeft(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UScriptCode);
  ptr = (UBool(*)(UScriptCode))syms[705];
  return ptr(script);
}

UBool uscript_breaksBetweenLetters(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UScriptCode);
  ptr = (UBool(*)(UScriptCode))syms[706];
  return ptr(script);
}

UBool uscript_isCased(UScriptCode script) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UScriptCode);
  ptr = (UBool(*)(UScriptCode))syms[707];
  return ptr(script);
}

/* unicode/ustring.h */
int32_t u_strlen(const UChar* s) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*);
  ptr = (int32_t(*)(const UChar*))syms[708];
  return ptr(s);
}

int32_t u_countChar32(const UChar* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, int32_t))syms[709];
  return ptr(s, length);
}

UBool u_strHasMoreChar32Than(const UChar* s, int32_t length, int32_t number) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, int32_t);
  ptr = (UBool(*)(const UChar*, int32_t, int32_t))syms[710];
  return ptr(s, length, number);
}

UChar* u_strcat(UChar* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*);
  ptr = (UChar*(*)(UChar*, const UChar*))syms[711];
  return ptr(dst, src);
}

UChar* u_strncat(UChar* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[712];
  return ptr(dst, src, n);
}

UChar* u_strstr(const UChar* s, const UChar* substring) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[713];
  return ptr(s, substring);
}

UChar* u_strFindFirst(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UChar*(*)(const UChar*, int32_t, const UChar*, int32_t))syms[714];
  return ptr(s, length, substring, subLength);
}

UChar* u_strchr(const UChar* s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar);
  ptr = (UChar*(*)(const UChar*, UChar))syms[715];
  return ptr(s, c);
}

UChar* u_strchr32(const UChar* s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar32);
  ptr = (UChar*(*)(const UChar*, UChar32))syms[716];
  return ptr(s, c);
}

UChar* u_strrstr(const UChar* s, const UChar* substring) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[717];
  return ptr(s, substring);
}

UChar* u_strFindLast(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, int32_t, const UChar*, int32_t);
  ptr = (UChar*(*)(const UChar*, int32_t, const UChar*, int32_t))syms[718];
  return ptr(s, length, substring, subLength);
}

UChar* u_strrchr(const UChar* s, UChar c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar);
  ptr = (UChar*(*)(const UChar*, UChar))syms[719];
  return ptr(s, c);
}

UChar* u_strrchr32(const UChar* s, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar32);
  ptr = (UChar*(*)(const UChar*, UChar32))syms[720];
  return ptr(s, c);
}

UChar* u_strpbrk(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, const UChar*);
  ptr = (UChar*(*)(const UChar*, const UChar*))syms[721];
  return ptr(string, matchSet);
}

int32_t u_strcspn(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[722];
  return ptr(string, matchSet);
}

int32_t u_strspn(const UChar* string, const UChar* matchSet) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[723];
  return ptr(string, matchSet);
}

UChar* u_strtok_r(UChar* src, const UChar* delim, UChar** saveState) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*, UChar**);
  ptr = (UChar*(*)(UChar*, const UChar*, UChar**))syms[724];
  return ptr(src, delim, saveState);
}

int32_t u_strcmp(const UChar* s1, const UChar* s2) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[725];
  return ptr(s1, s2);
}

int32_t u_strcmpCodePointOrder(const UChar* s1, const UChar* s2) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*);
  ptr = (int32_t(*)(const UChar*, const UChar*))syms[726];
  return ptr(s1, s2);
}

int32_t u_strCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UBool);
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UBool))syms[727];
  return ptr(s1, length1, s2, length2, codePointOrder);
}

int32_t u_strCompareIter(UCharIterator* iter1, UCharIterator* iter2, UBool codePointOrder) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UCharIterator*, UBool);
  ptr = (int32_t(*)(UCharIterator*, UCharIterator*, UBool))syms[728];
  return ptr(iter1, iter2, codePointOrder);
}

int32_t u_strCaseCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[729] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[729];
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

int32_t u_strncmp(const UChar* ucs1, const UChar* ucs2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[730];
  return ptr(ucs1, ucs2, n);
}

int32_t u_strncmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[731];
  return ptr(s1, s2, n);
}

int32_t u_strcasecmp(const UChar* s1, const UChar* s2, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, uint32_t))syms[732];
  return ptr(s1, s2, options);
}

int32_t u_strncasecmp(const UChar* s1, const UChar* s2, int32_t n, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))syms[733];
  return ptr(s1, s2, n, options);
}

int32_t u_memcasecmp(const UChar* s1, const UChar* s2, int32_t length, uint32_t options) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t, uint32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t, uint32_t))syms[734];
  return ptr(s1, s2, length, options);
}

UChar* u_strcpy(UChar* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*);
  ptr = (UChar*(*)(UChar*, const UChar*))syms[735];
  return ptr(dst, src);
}

UChar* u_strncpy(UChar* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[736];
  return ptr(dst, src, n);
}

UChar* u_uastrcpy(UChar* dst, const char* src) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const char*);
  ptr = (UChar*(*)(UChar*, const char*))syms[737];
  return ptr(dst, src);
}

UChar* u_uastrncpy(UChar* dst, const char* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const char*, int32_t);
  ptr = (UChar*(*)(UChar*, const char*, int32_t))syms[738];
  return ptr(dst, src, n);
}

char* u_austrcpy(char* dst, const UChar* src) {
  pthread_once(&once_control, &init_icudata_version);
  char* (*ptr)(char*, const UChar*);
  ptr = (char*(*)(char*, const UChar*))syms[739];
  return ptr(dst, src);
}

char* u_austrncpy(char* dst, const UChar* src, int32_t n) {
  pthread_once(&once_control, &init_icudata_version);
  char* (*ptr)(char*, const UChar*, int32_t);
  ptr = (char*(*)(char*, const UChar*, int32_t))syms[740];
  return ptr(dst, src, n);
}

UChar* u_memcpy(UChar* dest, const UChar* src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[741];
  return ptr(dest, src, count);
}

UChar* u_memmove(UChar* dest, const UChar* src, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, const UChar*, int32_t);
  ptr = (UChar*(*)(UChar*, const UChar*, int32_t))syms[742];
  return ptr(dest, src, count);
}

UChar* u_memset(UChar* dest, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, UChar, int32_t);
  ptr = (UChar*(*)(UChar*, UChar, int32_t))syms[743];
  return ptr(dest, c, count);
}

int32_t u_memcmp(const UChar* buf1, const UChar* buf2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[744];
  return ptr(buf1, buf2, count);
}

int32_t u_memcmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, const UChar*, int32_t);
  ptr = (int32_t(*)(const UChar*, const UChar*, int32_t))syms[745];
  return ptr(s1, s2, count);
}

UChar* u_memchr(const UChar* s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar, int32_t))syms[746];
  return ptr(s, c, count);
}

UChar* u_memchr32(const UChar* s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar32, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar32, int32_t))syms[747];
  return ptr(s, c, count);
}

UChar* u_memrchr(const UChar* s, UChar c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar, int32_t))syms[748];
  return ptr(s, c, count);
}

UChar* u_memrchr32(const UChar* s, UChar32 c, int32_t count) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(const UChar*, UChar32, int32_t);
  ptr = (UChar*(*)(const UChar*, UChar32, int32_t))syms[749];
  return ptr(s, c, count);
}

int32_t u_unescape(const char* src, UChar* dest, int32_t destCapacity) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const char*, UChar*, int32_t);
  ptr = (int32_t(*)(const char*, UChar*, int32_t))syms[750];
  return ptr(src, dest, destCapacity);
}

UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t* offset, int32_t length, void* context) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*);
  ptr = (UChar32(*)(UNESCAPE_CHAR_AT, int32_t*, int32_t, void*))syms[751];
  return ptr(charAt, offset, length, context);
}

int32_t u_strToUpper(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  if (syms[752] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))syms[752];
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToLower(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*);
  if (syms[753] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, const char*, UErrorCode*))syms[753];
  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}

int32_t u_strToTitle(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UBreakIterator* titleIter, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*);
  if (syms[754] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, UBreakIterator*, const char*, UErrorCode*))syms[754];
  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}

int32_t u_strFoldCase(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[755] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[755];
  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}

wchar_t* u_strToWCS(wchar_t* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  wchar_t* (*ptr)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  if (syms[756] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (wchar_t*)0;
  }
  ptr = (wchar_t*(*)(wchar_t*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[756];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromWCS(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const wchar_t* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*);
  if (syms[757] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const wchar_t*, int32_t, UErrorCode*))syms[757];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char* u_strToUTF8(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  char* (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  if (syms[758] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (char*)0;
  }
  ptr = (char*(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[758];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromUTF8(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  if (syms[759] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))syms[759];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

char* u_strToUTF8WithSub(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  char* (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  if (syms[760] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (char*)0;
  }
  ptr = (char*(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))syms[760];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF8WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*);
  if (syms[761] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*))syms[761];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF8Lenient(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*);
  if (syms[762] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UErrorCode*))syms[762];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32* u_strToUTF32(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32* (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  if (syms[763] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar32*)0;
  }
  ptr = (UChar32*(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[763];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromUTF32(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*);
  if (syms[764] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UErrorCode*))syms[764];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar32* u_strToUTF32WithSub(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32* (*ptr)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*);
  if (syms[765] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar32*)0;
  }
  ptr = (UChar32*(*)(UChar32*, int32_t, int32_t*, const UChar*, int32_t, UChar32, int32_t*, UErrorCode*))syms[765];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

UChar* u_strFromUTF32WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*);
  if (syms[766] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const UChar32*, int32_t, UChar32, int32_t*, UErrorCode*))syms[766];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

char* u_strToJavaModifiedUTF8(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  char* (*ptr)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*);
  if (syms[767] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (char*)0;
  }
  ptr = (char*(*)(char*, int32_t, int32_t*, const UChar*, int32_t, UErrorCode*))syms[767];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}

UChar* u_strFromJavaModifiedUTF8WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UChar* (*ptr)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*);
  if (syms[768] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UChar*)0;
  }
  ptr = (UChar*(*)(UChar*, int32_t, int32_t*, const char*, int32_t, UChar32, int32_t*, UErrorCode*))syms[768];
  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}

/* unicode/utext.h */
UText* utext_close(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(UText*);
  ptr = (UText*(*)(UText*))syms[769];
  return ptr(ut);
}

UText* utext_openUTF8(UText* ut, const char* s, int64_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(UText*, const char*, int64_t, UErrorCode*);
  if (syms[770] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(UText*, const char*, int64_t, UErrorCode*))syms[770];
  return ptr(ut, s, length, status);
}

UText* utext_openUChars(UText* ut, const UChar* s, int64_t length, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(UText*, const UChar*, int64_t, UErrorCode*);
  if (syms[771] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(UText*, const UChar*, int64_t, UErrorCode*))syms[771];
  return ptr(ut, s, length, status);
}

UText* utext_clone(UText* dest, const UText* src, UBool deep, UBool readOnly, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(UText*, const UText*, UBool, UBool, UErrorCode*);
  if (syms[772] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(UText*, const UText*, UBool, UBool, UErrorCode*))syms[772];
  return ptr(dest, src, deep, readOnly, status);
}

UBool utext_equals(const UText* a, const UText* b) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*, const UText*);
  ptr = (UBool(*)(const UText*, const UText*))syms[773];
  return ptr(a, b);
}

int64_t utext_nativeLength(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UText*);
  ptr = (int64_t(*)(UText*))syms[774];
  return ptr(ut);
}

UBool utext_isLengthExpensive(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[775];
  return ptr(ut);
}

UChar32 utext_char32At(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[776];
  return ptr(ut, nativeIndex);
}

UChar32 utext_current32(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[777];
  return ptr(ut);
}

UChar32 utext_next32(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[778];
  return ptr(ut);
}

UChar32 utext_previous32(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*);
  ptr = (UChar32(*)(UText*))syms[779];
  return ptr(ut);
}

UChar32 utext_next32From(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[780];
  return ptr(ut, nativeIndex);
}

UChar32 utext_previous32From(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UText*, int64_t);
  ptr = (UChar32(*)(UText*, int64_t))syms[781];
  return ptr(ut, nativeIndex);
}

int64_t utext_getNativeIndex(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(const UText*);
  ptr = (int64_t(*)(const UText*))syms[782];
  return ptr(ut);
}

void utext_setNativeIndex(UText* ut, int64_t nativeIndex) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*, int64_t);
  ptr = (void(*)(UText*, int64_t))syms[783];
  ptr(ut, nativeIndex);
  return;
}

UBool utext_moveIndex32(UText* ut, int32_t delta) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UText*, int32_t);
  ptr = (UBool(*)(UText*, int32_t))syms[784];
  return ptr(ut, delta);
}

int64_t utext_getPreviousNativeIndex(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  int64_t (*ptr)(UText*);
  ptr = (int64_t(*)(UText*))syms[785];
  return ptr(ut);
}

int32_t utext_extract(UText* ut, int64_t nativeStart, int64_t nativeLimit, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*);
  if (syms[786] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UText*, int64_t, int64_t, UChar*, int32_t, UErrorCode*))syms[786];
  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}

UBool utext_isWritable(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[787];
  return ptr(ut);
}

UBool utext_hasMetaData(const UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UText*);
  ptr = (UBool(*)(const UText*))syms[788];
  return ptr(ut);
}

int32_t utext_replace(UText* ut, int64_t nativeStart, int64_t nativeLimit, const UChar* replacementText, int32_t replacementLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*);
  if (syms[789] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UText*, int64_t, int64_t, const UChar*, int32_t, UErrorCode*))syms[789];
  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}

void utext_copy(UText* ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*);
  if (syms[790] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UText*, int64_t, int64_t, int64_t, UBool, UErrorCode*))syms[790];
  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
  return;
}

void utext_freeze(UText* ut) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UText*);
  ptr = (void(*)(UText*))syms[791];
  ptr(ut);
  return;
}

UText* utext_setup(UText* ut, int32_t extraSpace, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UText* (*ptr)(UText*, int32_t, UErrorCode*);
  if (syms[792] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UText*)0;
  }
  ptr = (UText*(*)(UText*, int32_t, UErrorCode*))syms[792];
  return ptr(ut, extraSpace, status);
}

/* unicode/ucat.h */
u_nl_catd u_catopen(const char* name, const char* locale, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  u_nl_catd (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[793] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (u_nl_catd)0;
  }
  ptr = (u_nl_catd(*)(const char*, const char*, UErrorCode*))syms[793];
  return ptr(name, locale, ec);
}

void u_catclose(u_nl_catd catd) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(u_nl_catd);
  ptr = (void(*)(u_nl_catd))syms[794];
  ptr(catd);
  return;
}

const UChar* u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar* s, int32_t* len, UErrorCode* ec) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*);
  if (syms[795] == NULL) {
    *ec = U_UNSUPPORTED_ERROR;
    return (const UChar*)0;
  }
  ptr = (const UChar*(*)(u_nl_catd, int32_t, int32_t, const UChar*, int32_t*, UErrorCode*))syms[795];
  return ptr(catd, set_num, msg_num, s, len, ec);
}

/* unicode/ubidi.h */
UBiDi* ubidi_open(void) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDi* (*ptr)(void);
  ptr = (UBiDi*(*)(void))syms[796];
  return ptr();
}

UBiDi* ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDi* (*ptr)(int32_t, int32_t, UErrorCode*);
  if (syms[797] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UBiDi*)0;
  }
  ptr = (UBiDi*(*)(int32_t, int32_t, UErrorCode*))syms[797];
  return ptr(maxLength, maxRunCount, pErrorCode);
}

void ubidi_close(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*);
  ptr = (void(*)(UBiDi*))syms[798];
  ptr(pBiDi);
  return;
}

void ubidi_setInverse(UBiDi* pBiDi, UBool isInverse) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBool);
  ptr = (void(*)(UBiDi*, UBool))syms[799];
  ptr(pBiDi, isInverse);
  return;
}

UBool ubidi_isInverse(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBiDi*);
  ptr = (UBool(*)(UBiDi*))syms[800];
  return ptr(pBiDi);
}

void ubidi_orderParagraphsLTR(UBiDi* pBiDi, UBool orderParagraphsLTR) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBool);
  ptr = (void(*)(UBiDi*, UBool))syms[801];
  ptr(pBiDi, orderParagraphsLTR);
  return;
}

UBool ubidi_isOrderParagraphsLTR(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBiDi*);
  ptr = (UBool(*)(UBiDi*))syms[802];
  return ptr(pBiDi);
}

void ubidi_setReorderingMode(UBiDi* pBiDi, UBiDiReorderingMode reorderingMode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiReorderingMode);
  ptr = (void(*)(UBiDi*, UBiDiReorderingMode))syms[803];
  ptr(pBiDi, reorderingMode);
  return;
}

UBiDiReorderingMode ubidi_getReorderingMode(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiReorderingMode (*ptr)(UBiDi*);
  ptr = (UBiDiReorderingMode(*)(UBiDi*))syms[804];
  return ptr(pBiDi);
}

void ubidi_setReorderingOptions(UBiDi* pBiDi, uint32_t reorderingOptions) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, uint32_t);
  ptr = (void(*)(UBiDi*, uint32_t))syms[805];
  ptr(pBiDi, reorderingOptions);
  return;
}

uint32_t ubidi_getReorderingOptions(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(UBiDi*);
  ptr = (uint32_t(*)(UBiDi*))syms[806];
  return ptr(pBiDi);
}

void ubidi_setContext(UBiDi* pBiDi, const UChar* prologue, int32_t proLength, const UChar* epilogue, int32_t epiLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[807] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBiDi*, const UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[807];
  ptr(pBiDi, prologue, proLength, epilogue, epiLength, pErrorCode);
  return;
}

void ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*);
  if (syms[808] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBiDi*, const UChar*, int32_t, UBiDiLevel, UBiDiLevel*, UErrorCode*))syms[808];
  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
  return;
}

void ubidi_setLine(const UBiDi* pParaBiDi, int32_t start, int32_t limit, UBiDi* pLineBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*);
  if (syms[809] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UBiDi*, int32_t, int32_t, UBiDi*, UErrorCode*))syms[809];
  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
  return;
}

UBiDiDirection ubidi_getDirection(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiDirection (*ptr)(const UBiDi*);
  ptr = (UBiDiDirection(*)(const UBiDi*))syms[810];
  return ptr(pBiDi);
}

UBiDiDirection ubidi_getBaseDirection(const UChar* text, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiDirection (*ptr)(const UChar*, int32_t);
  ptr = (UBiDiDirection(*)(const UChar*, int32_t))syms[811];
  return ptr(text, length);
}

const UChar* ubidi_getText(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  const UChar* (*ptr)(const UBiDi*);
  ptr = (const UChar*(*)(const UBiDi*))syms[812];
  return ptr(pBiDi);
}

int32_t ubidi_getLength(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[813];
  return ptr(pBiDi);
}

UBiDiLevel ubidi_getParaLevel(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiLevel (*ptr)(const UBiDi*);
  ptr = (UBiDiLevel(*)(const UBiDi*))syms[814];
  return ptr(pBiDi);
}

int32_t ubidi_countParagraphs(UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*);
  ptr = (int32_t(*)(UBiDi*))syms[815];
  return ptr(pBiDi);
}

int32_t ubidi_getParagraph(const UBiDi* pBiDi, int32_t charIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  if (syms[816] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))syms[816];
  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}

void ubidi_getParagraphByIndex(const UBiDi* pBiDi, int32_t paraIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*);
  if (syms[817] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, int32_t*, UBiDiLevel*, UErrorCode*))syms[817];
  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
  return;
}

UBiDiLevel ubidi_getLevelAt(const UBiDi* pBiDi, int32_t charIndex) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiLevel (*ptr)(const UBiDi*, int32_t);
  ptr = (UBiDiLevel(*)(const UBiDi*, int32_t))syms[818];
  return ptr(pBiDi, charIndex);
}

const UBiDiLevel* ubidi_getLevels(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UBiDiLevel* (*ptr)(UBiDi*, UErrorCode*);
  if (syms[819] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UBiDiLevel*)0;
  }
  ptr = (const UBiDiLevel*(*)(UBiDi*, UErrorCode*))syms[819];
  return ptr(pBiDi, pErrorCode);
}

void ubidi_getLogicalRun(const UBiDi* pBiDi, int32_t logicalPosition, int32_t* pLogicalLimit, UBiDiLevel* pLevel) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*);
  ptr = (void(*)(const UBiDi*, int32_t, int32_t*, UBiDiLevel*))syms[820];
  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
  return;
}

int32_t ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, UErrorCode*);
  if (syms[821] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UBiDi*, UErrorCode*))syms[821];
  return ptr(pBiDi, pErrorCode);
}

UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength) {
  pthread_once(&once_control, &init_icudata_version);
  UBiDiDirection (*ptr)(UBiDi*, int32_t, int32_t*, int32_t*);
  ptr = (UBiDiDirection(*)(UBiDi*, int32_t, int32_t*, int32_t*))syms[822];
  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}

int32_t ubidi_getVisualIndex(UBiDi* pBiDi, int32_t logicalIndex, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  if (syms[823] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))syms[823];
  return ptr(pBiDi, logicalIndex, pErrorCode);
}

int32_t ubidi_getLogicalIndex(UBiDi* pBiDi, int32_t visualIndex, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, int32_t, UErrorCode*);
  if (syms[824] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UBiDi*, int32_t, UErrorCode*))syms[824];
  return ptr(pBiDi, visualIndex, pErrorCode);
}

void ubidi_getLogicalMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  if (syms[825] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))syms[825];
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_getVisualMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, int32_t*, UErrorCode*);
  if (syms[826] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBiDi*, int32_t*, UErrorCode*))syms[826];
  ptr(pBiDi, indexMap, pErrorCode);
  return;
}

void ubidi_reorderLogical(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))syms[827];
  ptr(levels, length, indexMap);
  return;
}

void ubidi_reorderVisual(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UBiDiLevel*, int32_t, int32_t*);
  ptr = (void(*)(const UBiDiLevel*, int32_t, int32_t*))syms[828];
  ptr(levels, length, indexMap);
  return;
}

void ubidi_invertMap(const int32_t* srcMap, int32_t* destMap, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const int32_t*, int32_t*, int32_t);
  ptr = (void(*)(const int32_t*, int32_t*, int32_t))syms[829];
  ptr(srcMap, destMap, length);
  return;
}

int32_t ubidi_getProcessedLength(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[830];
  return ptr(pBiDi);
}

int32_t ubidi_getResultLength(const UBiDi* pBiDi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBiDi*);
  ptr = (int32_t(*)(const UBiDi*))syms[831];
  return ptr(pBiDi);
}

UCharDirection ubidi_getCustomizedClass(UBiDi* pBiDi, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UCharDirection (*ptr)(UBiDi*, UChar32);
  ptr = (UCharDirection(*)(UBiDi*, UChar32))syms[832];
  return ptr(pBiDi, c);
}

void ubidi_setClassCallback(UBiDi* pBiDi, UBiDiClassCallback* newFn, const void* newContext, UBiDiClassCallback** oldFn, const void** oldContext, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*);
  if (syms[833] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBiDi*, UBiDiClassCallback*, const void*, UBiDiClassCallback**, const void**, UErrorCode*))syms[833];
  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
  return;
}

void ubidi_getClassCallback(UBiDi* pBiDi, UBiDiClassCallback** fn, const void** context) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBiDi*, UBiDiClassCallback**, const void**);
  ptr = (void(*)(UBiDi*, UBiDiClassCallback**, const void**))syms[834];
  ptr(pBiDi, fn, context);
  return;
}

int32_t ubidi_writeReordered(UBiDi* pBiDi, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*);
  if (syms[835] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UBiDi*, UChar*, int32_t, uint16_t, UErrorCode*))syms[835];
  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}

int32_t ubidi_writeReverse(const UChar* src, int32_t srcLength, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*);
  if (syms[836] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint16_t, UErrorCode*))syms[836];
  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}

/* unicode/udata.h */
UDataMemory* udata_open(const char* path, const char* type, const char* name, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDataMemory* (*ptr)(const char*, const char*, const char*, UErrorCode*);
  if (syms[837] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDataMemory*)0;
  }
  ptr = (UDataMemory*(*)(const char*, const char*, const char*, UErrorCode*))syms[837];
  return ptr(path, type, name, pErrorCode);
}

UDataMemory* udata_openChoice(const char* path, const char* type, const char* name, UDataMemoryIsAcceptable* isAcceptable, void* context, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UDataMemory* (*ptr)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*);
  if (syms[838] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UDataMemory*)0;
  }
  ptr = (UDataMemory*(*)(const char*, const char*, const char*, UDataMemoryIsAcceptable*, void*, UErrorCode*))syms[838];
  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}

void udata_close(UDataMemory* pData) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataMemory*);
  ptr = (void(*)(UDataMemory*))syms[839];
  ptr(pData);
  return;
}

const void* udata_getMemory(UDataMemory* pData) {
  pthread_once(&once_control, &init_icudata_version);
  const void* (*ptr)(UDataMemory*);
  ptr = (const void*(*)(UDataMemory*))syms[840];
  return ptr(pData);
}

void udata_getInfo(UDataMemory* pData, UDataInfo* pInfo) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataMemory*, UDataInfo*);
  ptr = (void(*)(UDataMemory*, UDataInfo*))syms[841];
  ptr(pData, pInfo);
  return;
}

void udata_setCommonData(const void* data, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const void*, UErrorCode*);
  if (syms[842] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const void*, UErrorCode*))syms[842];
  ptr(data, err);
  return;
}

void udata_setAppData(const char* packageName, const void* data, UErrorCode* err) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, const void*, UErrorCode*);
  if (syms[843] == NULL) {
    *err = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(const char*, const void*, UErrorCode*))syms[843];
  ptr(packageName, data, err);
  return;
}

void udata_setFileAccess(UDataFileAccess access, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UDataFileAccess, UErrorCode*);
  if (syms[844] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UDataFileAccess, UErrorCode*))syms[844];
  ptr(access, status);
  return;
}

/* unicode/ucasemap.h */
UCaseMap* ucasemap_open(const char* locale, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UCaseMap* (*ptr)(const char*, uint32_t, UErrorCode*);
  if (syms[845] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UCaseMap*)0;
  }
  ptr = (UCaseMap*(*)(const char*, uint32_t, UErrorCode*))syms[845];
  return ptr(locale, options, pErrorCode);
}

void ucasemap_close(UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*);
  ptr = (void(*)(UCaseMap*))syms[846];
  ptr(csm);
  return;
}

const char* ucasemap_getLocale(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UCaseMap*);
  ptr = (const char*(*)(const UCaseMap*))syms[847];
  return ptr(csm);
}

uint32_t ucasemap_getOptions(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCaseMap*);
  ptr = (uint32_t(*)(const UCaseMap*))syms[848];
  return ptr(csm);
}

void ucasemap_setLocale(UCaseMap* csm, const char* locale, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, const char*, UErrorCode*);
  if (syms[849] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCaseMap*, const char*, UErrorCode*))syms[849];
  ptr(csm, locale, pErrorCode);
  return;
}

void ucasemap_setOptions(UCaseMap* csm, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, uint32_t, UErrorCode*);
  if (syms[850] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCaseMap*, uint32_t, UErrorCode*))syms[850];
  ptr(csm, options, pErrorCode);
  return;
}

const UBreakIterator* ucasemap_getBreakIterator(const UCaseMap* csm) {
  pthread_once(&once_control, &init_icudata_version);
  const UBreakIterator* (*ptr)(const UCaseMap*);
  ptr = (const UBreakIterator*(*)(const UCaseMap*))syms[851];
  return ptr(csm);
}

void ucasemap_setBreakIterator(UCaseMap* csm, UBreakIterator* iterToAdopt, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCaseMap*, UBreakIterator*, UErrorCode*);
  if (syms[852] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCaseMap*, UBreakIterator*, UErrorCode*))syms[852];
  ptr(csm, iterToAdopt, pErrorCode);
  return;
}

int32_t ucasemap_toTitle(UCaseMap* csm, UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[853] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCaseMap*, UChar*, int32_t, const UChar*, int32_t, UErrorCode*))syms[853];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToLower(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[854] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[854];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToUpper(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[855] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[855];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8ToTitle(UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[856] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[856];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

int32_t ucasemap_utf8FoldCase(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*);
  if (syms[857] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*))syms[857];
  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}

/* unicode/uversion.h */
void u_versionFromString(UVersionInfo versionArray, const char* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, const char*);
  ptr = (void(*)(UVersionInfo, const char*))syms[858];
  ptr(versionArray, versionString);
  return;
}

void u_versionFromUString(UVersionInfo versionArray, const UChar* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, const UChar*);
  ptr = (void(*)(UVersionInfo, const UChar*))syms[859];
  ptr(versionArray, versionString);
  return;
}

void u_versionToString(const UVersionInfo versionArray, char* versionString) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UVersionInfo, char*);
  ptr = (void(*)(const UVersionInfo, char*))syms[860];
  ptr(versionArray, versionString);
  return;
}

void u_getVersion(UVersionInfo versionArray) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo);
  ptr = (void(*)(UVersionInfo))syms[861];
  ptr(versionArray);
  return;
}

/* unicode/unorm2.h */
const UNormalizer2* unorm2_getNFCInstance(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(UErrorCode*);
  if (syms[862] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(UErrorCode*))syms[862];
  return ptr(pErrorCode);
}

const UNormalizer2* unorm2_getNFDInstance(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(UErrorCode*);
  if (syms[863] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(UErrorCode*))syms[863];
  return ptr(pErrorCode);
}

const UNormalizer2* unorm2_getNFKCInstance(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(UErrorCode*);
  if (syms[864] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(UErrorCode*))syms[864];
  return ptr(pErrorCode);
}

const UNormalizer2* unorm2_getNFKDInstance(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(UErrorCode*);
  if (syms[865] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(UErrorCode*))syms[865];
  return ptr(pErrorCode);
}

const UNormalizer2* unorm2_getNFKCCasefoldInstance(UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(UErrorCode*);
  if (syms[866] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(UErrorCode*))syms[866];
  return ptr(pErrorCode);
}

const UNormalizer2* unorm2_getInstance(const char* packageName, const char* name, UNormalization2Mode mode, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  const UNormalizer2* (*ptr)(const char*, const char*, UNormalization2Mode, UErrorCode*);
  if (syms[867] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (const UNormalizer2*)0;
  }
  ptr = (const UNormalizer2*(*)(const char*, const char*, UNormalization2Mode, UErrorCode*))syms[867];
  return ptr(packageName, name, mode, pErrorCode);
}

UNormalizer2* unorm2_openFiltered(const UNormalizer2* norm2, const USet* filterSet, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizer2* (*ptr)(const UNormalizer2*, const USet*, UErrorCode*);
  if (syms[868] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UNormalizer2*)0;
  }
  ptr = (UNormalizer2*(*)(const UNormalizer2*, const USet*, UErrorCode*))syms[868];
  return ptr(norm2, filterSet, pErrorCode);
}

void unorm2_close(UNormalizer2* norm2) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UNormalizer2*);
  ptr = (void(*)(UNormalizer2*))syms[869];
  ptr(norm2);
  return;
}

int32_t unorm2_normalize(const UNormalizer2* norm2, const UChar* src, int32_t length, UChar* dest, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[870] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, const UChar*, int32_t, UChar*, int32_t, UErrorCode*))syms[870];
  return ptr(norm2, src, length, dest, capacity, pErrorCode);
}

int32_t unorm2_normalizeSecondAndAppend(const UNormalizer2* norm2, UChar* first, int32_t firstLength, int32_t firstCapacity, const UChar* second, int32_t secondLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, UChar*, int32_t, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[871] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, UChar*, int32_t, int32_t, const UChar*, int32_t, UErrorCode*))syms[871];
  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}

int32_t unorm2_append(const UNormalizer2* norm2, UChar* first, int32_t firstLength, int32_t firstCapacity, const UChar* second, int32_t secondLength, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, UChar*, int32_t, int32_t, const UChar*, int32_t, UErrorCode*);
  if (syms[872] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, UChar*, int32_t, int32_t, const UChar*, int32_t, UErrorCode*))syms[872];
  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}

int32_t unorm2_getDecomposition(const UNormalizer2* norm2, UChar32 c, UChar* decomposition, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, UChar32, UChar*, int32_t, UErrorCode*);
  if (syms[873] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, UChar32, UChar*, int32_t, UErrorCode*))syms[873];
  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}

int32_t unorm2_getRawDecomposition(const UNormalizer2* norm2, UChar32 c, UChar* decomposition, int32_t capacity, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, UChar32, UChar*, int32_t, UErrorCode*);
  if (syms[874] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, UChar32, UChar*, int32_t, UErrorCode*))syms[874];
  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}

UChar32 unorm2_composePair(const UNormalizer2* norm2, UChar32 a, UChar32 b) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(const UNormalizer2*, UChar32, UChar32);
  ptr = (UChar32(*)(const UNormalizer2*, UChar32, UChar32))syms[875];
  return ptr(norm2, a, b);
}

uint8_t unorm2_getCombiningClass(const UNormalizer2* norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  uint8_t (*ptr)(const UNormalizer2*, UChar32);
  ptr = (uint8_t(*)(const UNormalizer2*, UChar32))syms[876];
  return ptr(norm2, c);
}

UBool unorm2_isNormalized(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*);
  if (syms[877] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*))syms[877];
  return ptr(norm2, s, length, pErrorCode);
}

UNormalizationCheckResult unorm2_quickCheck(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizationCheckResult (*ptr)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*);
  if (syms[878] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UNormalizationCheckResult)0;
  }
  ptr = (UNormalizationCheckResult(*)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*))syms[878];
  return ptr(norm2, s, length, pErrorCode);
}

int32_t unorm2_spanQuickCheckYes(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*);
  if (syms[879] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UNormalizer2*, const UChar*, int32_t, UErrorCode*))syms[879];
  return ptr(norm2, s, length, pErrorCode);
}

UBool unorm2_hasBoundaryBefore(const UNormalizer2* norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UNormalizer2*, UChar32);
  ptr = (UBool(*)(const UNormalizer2*, UChar32))syms[880];
  return ptr(norm2, c);
}

UBool unorm2_hasBoundaryAfter(const UNormalizer2* norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UNormalizer2*, UChar32);
  ptr = (UBool(*)(const UNormalizer2*, UChar32))syms[881];
  return ptr(norm2, c);
}

UBool unorm2_isInert(const UNormalizer2* norm2, UChar32 c) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UNormalizer2*, UChar32);
  ptr = (UBool(*)(const UNormalizer2*, UChar32))syms[882];
  return ptr(norm2, c);
}

/* unicode/icudataver.h */
void u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UVersionInfo, UErrorCode*);
  if (syms[883] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UVersionInfo, UErrorCode*))syms[883];
  ptr(dataVersionFillin, status);
  return;
}

/* unicode/unorm.h */
int32_t unorm_normalize(const UChar* source, int32_t sourceLength, UNormalizationMode mode, int32_t options, UChar* result, int32_t resultLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*);
  if (syms[884] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UChar*, int32_t, UErrorCode*))syms[884];
  return ptr(source, sourceLength, mode, options, result, resultLength, status);
}

UNormalizationCheckResult unorm_quickCheck(const UChar* source, int32_t sourcelength, UNormalizationMode mode, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  if (syms[885] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UNormalizationCheckResult)0;
  }
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))syms[885];
  return ptr(source, sourcelength, mode, status);
}

UNormalizationCheckResult unorm_quickCheckWithOptions(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UNormalizationCheckResult (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  if (syms[886] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UNormalizationCheckResult)0;
  }
  ptr = (UNormalizationCheckResult(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[886];
  return ptr(src, srcLength, mode, options, pErrorCode);
}

UBool unorm_isNormalized(const UChar* src, int32_t srcLength, UNormalizationMode mode, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, UErrorCode*);
  if (syms[887] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, UErrorCode*))syms[887];
  return ptr(src, srcLength, mode, pErrorCode);
}

UBool unorm_isNormalizedWithOptions(const UChar* src, int32_t srcLength, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  if (syms[888] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (UBool)0;
  }
  ptr = (UBool(*)(const UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[888];
  return ptr(src, srcLength, mode, options, pErrorCode);
}

int32_t unorm_next(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  if (syms[889] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))syms[889];
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_previous(UCharIterator* src, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UBool doNormalize, UBool* pNeededToNormalize, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*);
  if (syms[890] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UCharIterator*, UChar*, int32_t, UNormalizationMode, int32_t, UBool, UBool*, UErrorCode*))syms[890];
  return ptr(src, dest, destCapacity, mode, options, doNormalize, pNeededToNormalize, pErrorCode);
}

int32_t unorm_concatenate(const UChar* left, int32_t leftLength, const UChar* right, int32_t rightLength, UChar* dest, int32_t destCapacity, UNormalizationMode mode, int32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*);
  if (syms[891] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, UChar*, int32_t, UNormalizationMode, int32_t, UErrorCode*))syms[891];
  return ptr(left, leftLength, right, rightLength, dest, destCapacity, mode, options, pErrorCode);
}

int32_t unorm_compare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[892] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, const UChar*, int32_t, uint32_t, UErrorCode*))syms[892];
  return ptr(s1, length1, s2, length2, options, pErrorCode);
}

/* unicode/usprep.h */
UStringPrepProfile* usprep_open(const char* path, const char* fileName, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringPrepProfile* (*ptr)(const char*, const char*, UErrorCode*);
  if (syms[893] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UStringPrepProfile*)0;
  }
  ptr = (UStringPrepProfile*(*)(const char*, const char*, UErrorCode*))syms[893];
  return ptr(path, fileName, status);
}

UStringPrepProfile* usprep_openByType(UStringPrepProfileType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UStringPrepProfile* (*ptr)(UStringPrepProfileType, UErrorCode*);
  if (syms[894] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UStringPrepProfile*)0;
  }
  ptr = (UStringPrepProfile*(*)(UStringPrepProfileType, UErrorCode*))syms[894];
  return ptr(type, status);
}

void usprep_close(UStringPrepProfile* profile) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UStringPrepProfile*);
  ptr = (void(*)(UStringPrepProfile*))syms[895];
  ptr(profile);
  return;
}

int32_t usprep_prepare(const UStringPrepProfile* prep, const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*);
  if (syms[896] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UStringPrepProfile*, const UChar*, int32_t, UChar*, int32_t, int32_t, UParseError*, UErrorCode*))syms[896];
  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}

/* unicode/ushape.h */
int32_t u_shapeArabic(const UChar* source, int32_t sourceLength, UChar* dest, int32_t destSize, uint32_t options, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*);
  if (syms[897] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(const UChar*, int32_t, UChar*, int32_t, uint32_t, UErrorCode*))syms[897];
  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}

/* unicode/uiter.h */
UChar32 uiter_current32(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[898];
  return ptr(iter);
}

UChar32 uiter_next32(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[899];
  return ptr(iter);
}

UChar32 uiter_previous32(UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(UCharIterator*);
  ptr = (UChar32(*)(UCharIterator*))syms[900];
  return ptr(iter);
}

uint32_t uiter_getState(const UCharIterator* iter) {
  pthread_once(&once_control, &init_icudata_version);
  uint32_t (*ptr)(const UCharIterator*);
  ptr = (uint32_t(*)(const UCharIterator*))syms[901];
  return ptr(iter);
}

void uiter_setState(UCharIterator* iter, uint32_t state, UErrorCode* pErrorCode) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, uint32_t, UErrorCode*);
  if (syms[902] == NULL) {
    *pErrorCode = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UCharIterator*, uint32_t, UErrorCode*))syms[902];
  ptr(iter, state, pErrorCode);
  return;
}

void uiter_setString(UCharIterator* iter, const UChar* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const UChar*, int32_t);
  ptr = (void(*)(UCharIterator*, const UChar*, int32_t))syms[903];
  ptr(iter, s, length);
  return;
}

void uiter_setUTF16BE(UCharIterator* iter, const char* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const char*, int32_t);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))syms[904];
  ptr(iter, s, length);
  return;
}

void uiter_setUTF8(UCharIterator* iter, const char* s, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UCharIterator*, const char*, int32_t);
  ptr = (void(*)(UCharIterator*, const char*, int32_t))syms[905];
  ptr(iter, s, length);
  return;
}

/* unicode/putil.h */
const char* u_getDataDirectory(void) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(void);
  ptr = (const char*(*)(void))syms[906];
  return ptr();
}

void u_setDataDirectory(const char* directory) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*);
  ptr = (void(*)(const char*))syms[907];
  ptr(directory);
  return;
}

void u_charsToUChars(const char* cs, UChar* us, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const char*, UChar*, int32_t);
  ptr = (void(*)(const char*, UChar*, int32_t))syms[908];
  ptr(cs, us, length);
  return;
}

void u_UCharsToChars(const UChar* us, char* cs, int32_t length) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(const UChar*, char*, int32_t);
  ptr = (void(*)(const UChar*, char*, int32_t))syms[909];
  ptr(us, cs, length);
  return;
}

/* unicode/ubrk.h */
UBreakIterator* ubrk_open(UBreakIteratorType type, const char* locale, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator* (*ptr)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*);
  if (syms[910] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBreakIterator*)0;
  }
  ptr = (UBreakIterator*(*)(UBreakIteratorType, const char*, const UChar*, int32_t, UErrorCode*))syms[910];
  return ptr(type, locale, text, textLength, status);
}

UBreakIterator* ubrk_openRules(const UChar* rules, int32_t rulesLength, const UChar* text, int32_t textLength, UParseError* parseErr, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator* (*ptr)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*);
  if (syms[911] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBreakIterator*)0;
  }
  ptr = (UBreakIterator*(*)(const UChar*, int32_t, const UChar*, int32_t, UParseError*, UErrorCode*))syms[911];
  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}

UBreakIterator* ubrk_safeClone(const UBreakIterator* bi, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  UBreakIterator* (*ptr)(const UBreakIterator*, void*, int32_t*, UErrorCode*);
  if (syms[912] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (UBreakIterator*)0;
  }
  ptr = (UBreakIterator*(*)(const UBreakIterator*, void*, int32_t*, UErrorCode*))syms[912];
  return ptr(bi, stackBuffer, pBufferSize, status);
}

void ubrk_close(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*);
  ptr = (void(*)(UBreakIterator*))syms[913];
  ptr(bi);
  return;
}

void ubrk_setText(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*, const UChar*, int32_t, UErrorCode*);
  if (syms[914] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBreakIterator*, const UChar*, int32_t, UErrorCode*))syms[914];
  ptr(bi, text, textLength, status);
  return;
}

void ubrk_setUText(UBreakIterator* bi, UText* text, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*, UText*, UErrorCode*);
  if (syms[915] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBreakIterator*, UText*, UErrorCode*))syms[915];
  ptr(bi, text, status);
  return;
}

int32_t ubrk_current(const UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const UBreakIterator*);
  ptr = (int32_t(*)(const UBreakIterator*))syms[916];
  return ptr(bi);
}

int32_t ubrk_next(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[917];
  return ptr(bi);
}

int32_t ubrk_previous(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[918];
  return ptr(bi);
}

int32_t ubrk_first(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[919];
  return ptr(bi);
}

int32_t ubrk_last(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[920];
  return ptr(bi);
}

int32_t ubrk_preceding(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))syms[921];
  return ptr(bi, offset);
}

int32_t ubrk_following(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t);
  ptr = (int32_t(*)(UBreakIterator*, int32_t))syms[922];
  return ptr(bi, offset);
}

const char* ubrk_getAvailable(int32_t index) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(int32_t);
  ptr = (const char*(*)(int32_t))syms[923];
  return ptr(index);
}

int32_t ubrk_countAvailable(void) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(void);
  ptr = (int32_t(*)(void))syms[924];
  return ptr();
}

UBool ubrk_isBoundary(UBreakIterator* bi, int32_t offset) {
  pthread_once(&once_control, &init_icudata_version);
  UBool (*ptr)(UBreakIterator*, int32_t);
  ptr = (UBool(*)(UBreakIterator*, int32_t))syms[925];
  return ptr(bi, offset);
}

int32_t ubrk_getRuleStatus(UBreakIterator* bi) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*);
  ptr = (int32_t(*)(UBreakIterator*))syms[926];
  return ptr(bi);
}

int32_t ubrk_getRuleStatusVec(UBreakIterator* bi, int32_t* fillInVec, int32_t capacity, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(UBreakIterator*, int32_t*, int32_t, UErrorCode*);
  if (syms[927] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (int32_t)0;
  }
  ptr = (int32_t(*)(UBreakIterator*, int32_t*, int32_t, UErrorCode*))syms[927];
  return ptr(bi, fillInVec, capacity, status);
}

const char* ubrk_getLocaleByType(const UBreakIterator* bi, ULocDataLocaleType type, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  const char* (*ptr)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*);
  if (syms[928] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return (const char*)0;
  }
  ptr = (const char*(*)(const UBreakIterator*, ULocDataLocaleType, UErrorCode*))syms[928];
  return ptr(bi, type, status);
}

void ubrk_refreshUText(UBreakIterator* bi, UText* text, UErrorCode* status) {
  pthread_once(&once_control, &init_icudata_version);
  void (*ptr)(UBreakIterator*, UText*, UErrorCode*);
  if (syms[929] == NULL) {
    *status = U_UNSUPPORTED_ERROR;
    return;
  }
  ptr = (void(*)(UBreakIterator*, UText*, UErrorCode*))syms[929];
  ptr(bi, text, status);
  return;
}

/* unicode/utf8.h */
UChar32 utf8_nextCharSafeBody(const uint8_t* s, int32_t* pi, int32_t length, UChar32 c, UBool strict) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(const uint8_t*, int32_t*, int32_t, UChar32, UBool);
  ptr = (UChar32(*)(const uint8_t*, int32_t*, int32_t, UChar32, UBool))syms[930];
  return ptr(s, pi, length, c, strict);
}

int32_t utf8_appendCharSafeBody(uint8_t* s, int32_t i, int32_t length, UChar32 c, UBool* pIsError) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(uint8_t*, int32_t, int32_t, UChar32, UBool*);
  ptr = (int32_t(*)(uint8_t*, int32_t, int32_t, UChar32, UBool*))syms[931];
  return ptr(s, i, length, c, pIsError);
}

UChar32 utf8_prevCharSafeBody(const uint8_t* s, int32_t start, int32_t* pi, UChar32 c, UBool strict) {
  pthread_once(&once_control, &init_icudata_version);
  UChar32 (*ptr)(const uint8_t*, int32_t, int32_t*, UChar32, UBool);
  ptr = (UChar32(*)(const uint8_t*, int32_t, int32_t*, UChar32, UBool))syms[932];
  return ptr(s, start, pi, c, strict);
}

int32_t utf8_back1SafeBody(const uint8_t* s, int32_t start, int32_t i) {
  pthread_once(&once_control, &init_icudata_version);
  int32_t (*ptr)(const uint8_t*, int32_t, int32_t);
  ptr = (int32_t(*)(const uint8_t*, int32_t, int32_t))syms[933];
  return ptr(s, start, i);
}


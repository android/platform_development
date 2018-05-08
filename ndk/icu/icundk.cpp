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
#include <unicode/uloc.h>
#include <unicode/ulocdata.h>
#include <unicode/umsg.h>
#include <unicode/unorm2.h>
#include <unicode/unum.h>
#include <unicode/unumsys.h>
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

static void* handle_i18n = nullptr;
static void* handle_common = nullptr;

struct sym_tab {
  const char* name;
  void** handle;
  void* addr;
  bool initialized;
};

static sym_tab syms[924] = {
  {
    .name = "ucol_openElements",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_keyHashCode",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_closeElements",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_reset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_next",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_previous",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getMaxExpansion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getOffset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setOffset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_primaryOrder",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_secondaryOrder",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_tertiaryOrder",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_setText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_setDeclaredEncoding",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_detect",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_detectAll",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_getName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_getConfidence",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_getLanguage",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_getUChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_getAllDetectableCharsets",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_isInputFilterEnabled",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucsdet_enableInputFilter",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utmscale_getTimeScaleValue",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utmscale_fromInt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utmscale_toInt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_openEmpty",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getBestPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getBestPatternWithOptions",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getSkeleton",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getBaseSkeleton",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_addPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_setAppendItemFormat",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getAppendItemFormat",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_setAppendItemName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getAppendItemName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_setDateTimeFormat",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getDateTimeFormat",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_setDecimal",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getDecimal",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_replaceFieldTypes",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_replaceFieldTypesWithOptions",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_openSkeletons",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_openBaseSkeletons",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udatpg_getPatternForSkeleton",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_openFromSerialized",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_openFromSource",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_setChecks",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getChecks",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_setRestrictionLevel",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getRestrictionLevel",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_setAllowedLocales",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getAllowedLocales",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_setAllowedChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getAllowedChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_check",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_checkUTF8",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_check2",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_check2UTF8",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_openCheckResult",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_closeCheckResult",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getCheckResultChecks",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getCheckResultRestrictionLevel",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getCheckResultNumerics",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_areConfusable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_areConfusableUTF8",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getSkeleton",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getSkeletonUTF8",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getInclusionSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_getRecommendedSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uspoof_serialize",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_formatMessage",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_vformatMessage",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_parseMessage",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_vparseMessage",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_formatMessageWithError",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_vformatMessageWithError",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_parseMessageWithError",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_vparseMessageWithError",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_setLocale",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_getLocale",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_applyPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_toPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_format",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_vformat",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_parse",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_vparse",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "umsg_autoQuoteApostrophe",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ureldatefmt_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ureldatefmt_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ureldatefmt_formatNumeric",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ureldatefmt_format",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ureldatefmt_combineDateAndTime",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_openUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_openC",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_pattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_patternUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_flags",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_refreshUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_matches",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_matches64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_lookingAt",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_lookingAt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_find",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_find64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_findNext",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_groupCount",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_groupNumberFromName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_groupNumberFromCName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_group",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_groupUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_start",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_start64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_end",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_end64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_reset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_reset64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setRegion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setRegion64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setRegionAndStart",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_regionStart",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_regionStart64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_regionEnd",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_regionEnd64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_hasTransparentBounds",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_useTransparentBounds",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_hasAnchoringBounds",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_useAnchoringBounds",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_hitEnd",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_requireEnd",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_replaceAll",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_replaceAllUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_replaceFirst",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_replaceFirstUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_appendReplacement",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_appendReplacementUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_appendTail",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_appendTailUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_split",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_splitUText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setTimeLimit",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getTimeLimit",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setStackLimit",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getStackLimit",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setMatchCallback",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getMatchCallback",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_setFindProgressCallback",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregex_getFindProgressCallback",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_openByName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_openAvailableNames",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_getName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_isAlgorithmic",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_getRadix",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unumsys_getDescription",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_openRules",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getContractionsAndExpansions",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_strcoll",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_strcollUTF8",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_greater",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_greaterOrEqual",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_equal",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_strcollIter",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getStrength",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setStrength",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getReorderCodes",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setReorderCodes",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getEquivalentReorderCodes",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getDisplayName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_countAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_openAvailableLocales",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getKeywords",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getKeywordValues",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getKeywordValuesForLocale",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getFunctionalEquivalent",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getRules",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getSortKey",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_nextSortKeyPart",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getBound",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getVersion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getUCAVersion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_mergeSortkeys",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_setMaxVariable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getMaxVariable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getVariableTop",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_safeClone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getRulesEx",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getLocaleByType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_getTailoredSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_cloneBinary",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucol_openBinary",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_openU",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_openInverse",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_getUnicodeID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_register",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_unregisterID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_setFilter",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_countAvailableIDs",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_openIDs",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_trans",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_transIncremental",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_transUChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_transIncrementalUChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_toRules",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrans_getSourceSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_openFromCollator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setOffset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getOffset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getMatchedStart",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getMatchedLength",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getMatchedText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setBreakIterator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getBreakIterator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getText",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getCollator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setCollator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_setPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_getPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_first",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_following",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_last",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_preceding",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_next",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_previous",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usearch_reset",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_format",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_formatInt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_formatDouble",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_formatDecimal",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_formatDoubleCurrency",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_formatUFormattable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parse",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parseInt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parseDouble",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parseDecimal",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parseDoubleCurrency",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_parseToUFormattable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_applyPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_countAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_setAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getDoubleAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_setDoubleAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getTextAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_setTextAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_toPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getSymbol",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_setSymbol",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getLocaleByType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_setContext",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unum_getContext",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ugender_getInstance",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ugender_getListGender",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufieldpositer_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufieldpositer_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufieldpositer_next",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_openTimeZoneIDEnumeration",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_openTimeZones",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_openCountryTimeZones",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getDefaultTimeZone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setDefaultTimeZone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getDSTSavings",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getNow",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_clone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setTimeZone",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getTimeZoneID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getTimeZoneDisplayName",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_inDaylightTime",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setGregorianChange",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getGregorianChange",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setAttribute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_countAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getMillis",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setMillis",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setDate",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_setDateTime",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_equivalentTo",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_add",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_roll",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_get",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_set",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_isSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_clearField",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_clear",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getLimit",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getLocaleByType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getTZDataVersion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getCanonicalTimeZoneID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getKeywordValuesForLocale",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getDayOfWeekType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getWeekendTransition",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_isWeekend",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getFieldDifference",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getTimeZoneTransitionDate",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getWindowsTimeZoneID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucal_getTimeZoneIDForWindowsID",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udtitvfmt_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udtitvfmt_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udtitvfmt_format",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_setNoSubstitute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getNoSubstitute",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getExemplarSet",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getDelimiter",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getMeasurementSystem",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getPaperSize",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getCLDRVersion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getLocaleDisplayPattern",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ulocdata_getLocaleSeparator",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_open",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_close",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_isNumeric",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getDate",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getDouble",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getLong",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getInt64",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getObject",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getUChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getArrayLength",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getArrayItemByIndex",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ufmt_getDecNumChars",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getRegionFromCode",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getRegionFromNumericCode",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getAvailable",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_areEqual",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getContainingRegion",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getContainingRegionOfType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getContainedRegions",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getContainedRegionsOfType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_contains",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getPreferredValues",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getRegionCode",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getNumericCode",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uregion_getType",
    .handle = &handle_i18n,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDefault",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_setDefault",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getLanguage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getScript",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getCountry",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getVariant",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_canonicalize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getISO3Language",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getISO3Country",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getLCID",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayLanguage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayScript",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayCountry",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayVariant",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayKeyword",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayKeywordValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_countAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getISOLanguages",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getISOCountries",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getParent",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getBaseName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_openKeywords",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getKeywordValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_setKeywordValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_isRightToLeft",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getCharacterOrientation",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getLineOrientation",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_acceptLanguageFromHTTP",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_acceptLanguage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_getLocaleForLCID",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_addLikelySubtags",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_minimizeSubtags",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_forLanguageTag",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_toLanguageTag",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_toUnicodeLocaleKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_toUnicodeLocaleType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_toLegacyKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uloc_toLegacyType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getDataVersion",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_hasBinaryProperty",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isUAlphabetic",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isULowercase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isUUppercase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isUWhiteSpace",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getIntPropertyValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getIntPropertyMinValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getIntPropertyMaxValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getNumericValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_islower",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isupper",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_istitle",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isdigit",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isalpha",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isalnum",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isxdigit",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_ispunct",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isgraph",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isblank",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isdefined",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isspace",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isJavaSpaceChar",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isWhitespace",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_iscntrl",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isISOControl",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isprint",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isbase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charDirection",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isMirrored",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charMirror",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getBidiPairedBracket",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_enumCharTypes",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getCombiningClass",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charDigitValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ublock_getCode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charFromName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_enumCharNames",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getPropertyName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getPropertyEnum",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getPropertyValueName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getPropertyValueEnum",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isIDStart",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isIDPart",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isIDIgnorable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isJavaIDStart",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_isJavaIDPart",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_tolower",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_toupper",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_totitle",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_foldCase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_digit",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_forDigit",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charAge",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getUnicodeVersion",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getFC_NFKC_Closure",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_FROM_U_CALLBACK_STOP",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_TO_U_CALLBACK_STOP",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_FROM_U_CALLBACK_SKIP",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_FROM_U_CALLBACK_SUBSTITUTE",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_FROM_U_CALLBACK_ESCAPE",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_TO_U_CALLBACK_SKIP",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_TO_U_CALLBACK_SUBSTITUTE",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "UCNV_TO_U_CALLBACK_ESCAPE",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_openChoice",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_getMemory",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_getInfo",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_setCommonData",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_setAppData",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "udata_setFileAccess",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_compareNames",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_openU",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_openCCSID",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_openPackage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_safeClone",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getSubstChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setSubstChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setSubstString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getInvalidChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getInvalidUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_reset",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_resetToUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_resetFromUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getMaxCharSize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getMinCharSize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getCCSID",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getPlatform",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getStarters",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getUnicodeSet",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getToUCallBack",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getFromUCallBack",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setToUCallBack",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setFromUCallBack",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_fromUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_toUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_fromUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_toUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getNextUChar",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_convertEx",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_convert",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_toAlgorithmic",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_fromAlgorithmic",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_flushCache",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_countAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getAvailableName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_openAllNames",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_countAliases",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getAlias",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getAliases",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_openStandardNames",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_countStandards",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getStandard",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getStandardName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getCanonicalName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_getDefaultName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setDefaultName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_fixFileSeparator",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_isAmbiguous",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_setFallback",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_usesFallback",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_detectUnicodeSignature",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_fromUCountPending",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_toUCountPending",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_isFixedWidth",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utf8_nextCharSafeBody",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utf8_appendCharSafeBody",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utf8_prevCharSafeBody",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utf8_back1SafeBody",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_openSized",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setInverse",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_isInverse",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_orderParagraphsLTR",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_isOrderParagraphsLTR",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setReorderingMode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getReorderingMode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setReorderingOptions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getReorderingOptions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setContext",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setPara",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setLine",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getDirection",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getBaseDirection",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getText",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLength",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getParaLevel",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_countParagraphs",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getParagraph",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getParagraphByIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLevelAt",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLevels",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLogicalRun",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_countRuns",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getVisualRun",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getVisualIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLogicalIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getLogicalMap",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getVisualMap",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_reorderLogical",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_reorderVisual",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_invertMap",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getProcessedLength",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getResultLength",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getCustomizedClass",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_setClassCallback",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_getClassCallback",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_writeReordered",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubidi_writeReverse",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strlen",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_countChar32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strHasMoreChar32Than",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcat",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strncat",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strstr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFindFirst",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strchr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strchr32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strrstr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFindLast",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strrchr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strrchr32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strpbrk",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcspn",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strspn",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strtok_r",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcmpCodePointOrder",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strCompare",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strCompareIter",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strCaseCompare",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strncmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strncmpCodePointOrder",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcasecmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strncasecmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memcasecmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strcpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strncpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_uastrcpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_uastrncpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_austrcpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_austrncpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memcpy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memmove",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memset",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memcmp",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memcmpCodePointOrder",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memchr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memchr32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memrchr",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_memrchr32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_unescape",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_unescapeAt",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToUpper",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToLower",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToTitle",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFoldCase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToWCS",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromWCS",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToUTF8WithSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromUTF8WithSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromUTF8Lenient",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToUTF32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromUTF32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToUTF32WithSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromUTF32WithSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strToJavaModifiedUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_strFromJavaModifiedUTF8WithSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_catopen",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_catclose",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_catgets",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_openUTS46",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_labelToASCII",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_labelToUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_nameToASCII",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_nameToUnicode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_labelToASCII_UTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_labelToUnicodeUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_nameToASCII_UTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uidna_nameToUnicodeUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_cbFromUWriteBytes",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_cbFromUWriteSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_cbFromUWriteUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_cbToUWriteUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnv_cbToUWriteSub",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_getLocale",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_getDialectHandling",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_localeDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_languageDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_scriptDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_scriptCodeDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_regionDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_variantDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_keyDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_keyValueDisplayName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_openForContext",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uldn_getContext",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_init",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_cleanup",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_setMemoryFunctions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_errorName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_forLocale",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_register",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_unregister",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getPluralName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getDefaultFractionDigits",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getDefaultFractionDigitsForUsage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getRoundingIncrement",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getRoundingIncrementForUsage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_openISOCurrencies",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_isAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_countCurrencies",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_forLocaleAndDate",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getKeywordValuesForLocale",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucurr_getNumericCode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_openEmpty",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_openPattern",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_openPatternOptions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_clone",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_isFrozen",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_freeze",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_cloneAsThawed",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_set",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_applyPattern",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_applyIntPropertyValue",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_applyPropertyAlias",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_resemblesPattern",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_toPattern",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_add",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_addAll",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_addRange",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_addString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_addAllCodePoints",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_remove",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_removeRange",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_removeString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_removeAll",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_retain",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_retainAll",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_compact",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_complement",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_complementAll",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_clear",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_closeOver",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_removeAllStrings",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_isEmpty",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_contains",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsRange",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_indexOf",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_charAt",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_size",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_getItemCount",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_getItem",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsAll",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsAllCodePoints",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsNone",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_containsSome",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_span",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_spanBack",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_spanUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_spanBackUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_equals",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_serialize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_getSerializedSet",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_setSerializedToOne",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_serializedContains",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_getSerializedRangeCount",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uset_getSerializedRange",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_shapeArabic",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_openRules",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_safeClone",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_setText",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_setUText",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_current",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_next",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_previous",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_first",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_last",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_preceding",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_following",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_getAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_countAvailable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_isBoundary",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_getRuleStatus",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_getRuleStatusVec",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_getLocaleByType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubrk_refreshUText",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_setLevel",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_getLevel",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_setFunctions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_getFunctions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_vformat",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_format",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utrace_functionName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_openUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_openUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_clone",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_equals",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_nativeLength",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_isLengthExpensive",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_char32At",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_current32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_next32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_previous32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_next32From",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_previous32From",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_getNativeIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_setNativeIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_moveIndex32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_getPreviousNativeIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_extract",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_isWritable",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_hasMetaData",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_replace",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_copy",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_freeze",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "utext_setup",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_count",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_unext",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_next",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_reset",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_openUCharStringsEnumeration",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uenum_openCharStringsEnumeration",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_versionFromString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_versionFromUString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_versionToString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getVersion",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usprep_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usprep_openByType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usprep_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "usprep_prepare",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getCode",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getShortName",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getScript",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_hasScript",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getScriptExtensions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getSampleString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_getUsage",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_isRightToLeft",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_breaksBetweenLetters",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uscript_isCased",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_getDataDirectory",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_setDataDirectory",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_charsToUChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "u_UCharsToChars",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_getLocale",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_getOptions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_setLocale",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_setOptions",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_getBreakIterator",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_setBreakIterator",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_toTitle",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_utf8ToLower",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_utf8ToUpper",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_utf8ToTitle",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucasemap_utf8FoldCase",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getNFCInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getNFDInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getNFKCInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getNFKDInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getNFKCCasefoldInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getInstance",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_openFiltered",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_normalize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_normalizeSecondAndAppend",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_append",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getDecomposition",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getRawDecomposition",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_composePair",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_getCombiningClass",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_isNormalized",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_quickCheck",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_spanQuickCheckYes",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_hasBoundaryBefore",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_hasBoundaryAfter",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm2_isInert",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "unorm_compare",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_current32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_next32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_previous32",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_getState",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_setState",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_setString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_setUTF16BE",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "uiter_setUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_openFromSerialized",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_serialize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_selectForString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ucnvsel_selectForUTF8",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubiditransform_transform",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubiditransform_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ubiditransform_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_open",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_openDirect",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_openU",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_close",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getVersion",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getLocaleByType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getUTF8String",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getBinary",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getIntVector",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getUInt",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getInt",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getSize",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getType",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_resetIterator",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_hasNext",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getNextResource",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getNextString",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getByIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getStringByIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getUTF8StringByIndex",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getByKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getStringByKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_getUTF8StringByKey",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
  {
    .name = "ures_openAvailableLocales",
    .handle = &handle_common,
    .addr = nullptr,
    .initialized = false,
  },
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

UCollationElements* ucol_openElements(const UCollator* coll, const UChar* text, int32_t textLength, UErrorCode* status) {
  typedef decltype(&ucol_openElements) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_openElements"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_openElements");
    abort();
  }

  return ptr(coll, text, textLength, status);
}
int32_t ucol_keyHashCode(const uint8_t* key, int32_t length) {
  typedef decltype(&ucol_keyHashCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_keyHashCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_keyHashCode");
    abort();
  }

  return ptr(key, length);
}
void ucol_closeElements(UCollationElements* elems) {
  typedef decltype(&ucol_closeElements) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_closeElements"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_closeElements");
    abort();
  }

  ptr(elems);
}
void ucol_reset(UCollationElements* elems) {
  typedef decltype(&ucol_reset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_reset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_reset");
    abort();
  }

  ptr(elems);
}
int32_t ucol_next(UCollationElements* elems, UErrorCode* status) {
  typedef decltype(&ucol_next) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_next"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_next");
    abort();
  }

  return ptr(elems, status);
}
int32_t ucol_previous(UCollationElements* elems, UErrorCode* status) {
  typedef decltype(&ucol_previous) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_previous"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_previous");
    abort();
  }

  return ptr(elems, status);
}
int32_t ucol_getMaxExpansion(const UCollationElements* elems, int32_t order) {
  typedef decltype(&ucol_getMaxExpansion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getMaxExpansion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getMaxExpansion");
    abort();
  }

  return ptr(elems, order);
}
void ucol_setText(UCollationElements* elems, const UChar* text, int32_t textLength, UErrorCode* status) {
  typedef decltype(&ucol_setText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setText");
    abort();
  }

  ptr(elems, text, textLength, status);
}
int32_t ucol_getOffset(const UCollationElements* elems) {
  typedef decltype(&ucol_getOffset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getOffset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getOffset");
    abort();
  }

  return ptr(elems);
}
void ucol_setOffset(UCollationElements* elems, int32_t offset, UErrorCode* status) {
  typedef decltype(&ucol_setOffset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setOffset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setOffset");
    abort();
  }

  ptr(elems, offset, status);
}
int32_t ucol_primaryOrder(int32_t order) {
  typedef decltype(&ucol_primaryOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_primaryOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_primaryOrder");
    abort();
  }

  return ptr(order);
}
int32_t ucol_secondaryOrder(int32_t order) {
  typedef decltype(&ucol_secondaryOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_secondaryOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_secondaryOrder");
    abort();
  }

  return ptr(order);
}
int32_t ucol_tertiaryOrder(int32_t order) {
  typedef decltype(&ucol_tertiaryOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_tertiaryOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_tertiaryOrder");
    abort();
  }

  return ptr(order);
}
UCharsetDetector* ucsdet_open(UErrorCode* status) {
  typedef decltype(&ucsdet_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_open");
    abort();
  }

  return ptr(status);
}
void ucsdet_close(UCharsetDetector* ucsd) {
  typedef decltype(&ucsdet_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_close");
    abort();
  }

  ptr(ucsd);
}
void ucsdet_setText(UCharsetDetector* ucsd, const char* textIn, int32_t len, UErrorCode* status) {
  typedef decltype(&ucsdet_setText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_setText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_setText");
    abort();
  }

  ptr(ucsd, textIn, len, status);
}
void ucsdet_setDeclaredEncoding(UCharsetDetector* ucsd, const char* encoding, int32_t length, UErrorCode* status) {
  typedef decltype(&ucsdet_setDeclaredEncoding) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_setDeclaredEncoding"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_setDeclaredEncoding");
    abort();
  }

  ptr(ucsd, encoding, length, status);
}
const UCharsetMatch* ucsdet_detect(UCharsetDetector* ucsd, UErrorCode* status) {
  typedef decltype(&ucsdet_detect) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_detect"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_detect");
    abort();
  }

  return ptr(ucsd, status);
}
const UCharsetMatch** ucsdet_detectAll(UCharsetDetector* ucsd, int32_t* matchesFound, UErrorCode* status) {
  typedef decltype(&ucsdet_detectAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_detectAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_detectAll");
    abort();
  }

  return ptr(ucsd, matchesFound, status);
}
const char* ucsdet_getName(const UCharsetMatch* ucsm, UErrorCode* status) {
  typedef decltype(&ucsdet_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_getName");
    abort();
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getConfidence(const UCharsetMatch* ucsm, UErrorCode* status) {
  typedef decltype(&ucsdet_getConfidence) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_getConfidence"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_getConfidence");
    abort();
  }

  return ptr(ucsm, status);
}
const char* ucsdet_getLanguage(const UCharsetMatch* ucsm, UErrorCode* status) {
  typedef decltype(&ucsdet_getLanguage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_getLanguage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_getLanguage");
    abort();
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getUChars(const UCharsetMatch* ucsm, UChar* buf, int32_t cap, UErrorCode* status) {
  typedef decltype(&ucsdet_getUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_getUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_getUChars");
    abort();
  }

  return ptr(ucsm, buf, cap, status);
}
UEnumeration* ucsdet_getAllDetectableCharsets(const UCharsetDetector* ucsd, UErrorCode* status) {
  typedef decltype(&ucsdet_getAllDetectableCharsets) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_getAllDetectableCharsets"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_getAllDetectableCharsets");
    abort();
  }

  return ptr(ucsd, status);
}
UBool ucsdet_isInputFilterEnabled(const UCharsetDetector* ucsd) {
  typedef decltype(&ucsdet_isInputFilterEnabled) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_isInputFilterEnabled"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_isInputFilterEnabled");
    abort();
  }

  return ptr(ucsd);
}
UBool ucsdet_enableInputFilter(UCharsetDetector* ucsd, UBool filter) {
  typedef decltype(&ucsdet_enableInputFilter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucsdet_enableInputFilter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucsdet_enableInputFilter");
    abort();
  }

  return ptr(ucsd, filter);
}
int64_t utmscale_getTimeScaleValue(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode* status) {
  typedef decltype(&utmscale_getTimeScaleValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utmscale_getTimeScaleValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utmscale_getTimeScaleValue");
    abort();
  }

  return ptr(timeScale, value, status);
}
int64_t utmscale_fromInt64(int64_t otherTime, UDateTimeScale timeScale, UErrorCode* status) {
  typedef decltype(&utmscale_fromInt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utmscale_fromInt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utmscale_fromInt64");
    abort();
  }

  return ptr(otherTime, timeScale, status);
}
int64_t utmscale_toInt64(int64_t universalTime, UDateTimeScale timeScale, UErrorCode* status) {
  typedef decltype(&utmscale_toInt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utmscale_toInt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utmscale_toInt64");
    abort();
  }

  return ptr(universalTime, timeScale, status);
}
UDateTimePatternGenerator* udatpg_open(const char* locale, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_open");
    abort();
  }

  return ptr(locale, pErrorCode);
}
UDateTimePatternGenerator* udatpg_openEmpty(UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_openEmpty) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_openEmpty"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_openEmpty");
    abort();
  }

  return ptr(pErrorCode);
}
void udatpg_close(UDateTimePatternGenerator* dtpg) {
  typedef decltype(&udatpg_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_close");
    abort();
  }

  ptr(dtpg);
}
UDateTimePatternGenerator* udatpg_clone(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_clone");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
int32_t udatpg_getBestPattern(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_getBestPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getBestPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getBestPattern");
    abort();
  }

  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getBestPatternWithOptions(UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t length, UDateTimePatternMatchOptions options, UChar* bestPattern, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_getBestPatternWithOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getBestPatternWithOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getBestPatternWithOptions");
    abort();
  }

  return ptr(dtpg, skeleton, length, options, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getSkeleton(UDateTimePatternGenerator* unusedDtpg, const UChar* pattern, int32_t length, UChar* skeleton, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_getSkeleton) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getSkeleton"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getSkeleton");
    abort();
  }

  return ptr(unusedDtpg, pattern, length, skeleton, capacity, pErrorCode);
}
int32_t udatpg_getBaseSkeleton(UDateTimePatternGenerator* unusedDtpg, const UChar* pattern, int32_t length, UChar* baseSkeleton, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_getBaseSkeleton) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getBaseSkeleton"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getBaseSkeleton");
    abort();
  }

  return ptr(unusedDtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}
UDateTimePatternConflict udatpg_addPattern(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, UBool override, UChar* conflictingPattern, int32_t capacity, int32_t* pLength, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_addPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_addPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_addPattern");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}
void udatpg_setAppendItemFormat(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  typedef decltype(&udatpg_setAppendItemFormat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_setAppendItemFormat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_setAppendItemFormat");
    abort();
  }

  ptr(dtpg, field, value, length);
}
const UChar* udatpg_getAppendItemFormat(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  typedef decltype(&udatpg_getAppendItemFormat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getAppendItemFormat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getAppendItemFormat");
    abort();
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setAppendItemName(UDateTimePatternGenerator* dtpg, UDateTimePatternField field, const UChar* value, int32_t length) {
  typedef decltype(&udatpg_setAppendItemName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_setAppendItemName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_setAppendItemName");
    abort();
  }

  ptr(dtpg, field, value, length);
}
const UChar* udatpg_getAppendItemName(const UDateTimePatternGenerator* dtpg, UDateTimePatternField field, int32_t* pLength) {
  typedef decltype(&udatpg_getAppendItemName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getAppendItemName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getAppendItemName");
    abort();
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setDateTimeFormat(const UDateTimePatternGenerator* dtpg, const UChar* dtFormat, int32_t length) {
  typedef decltype(&udatpg_setDateTimeFormat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_setDateTimeFormat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_setDateTimeFormat");
    abort();
  }

  ptr(dtpg, dtFormat, length);
}
const UChar* udatpg_getDateTimeFormat(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  typedef decltype(&udatpg_getDateTimeFormat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getDateTimeFormat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getDateTimeFormat");
    abort();
  }

  return ptr(dtpg, pLength);
}
void udatpg_setDecimal(UDateTimePatternGenerator* dtpg, const UChar* decimal, int32_t length) {
  typedef decltype(&udatpg_setDecimal) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_setDecimal"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_setDecimal");
    abort();
  }

  ptr(dtpg, decimal, length);
}
const UChar* udatpg_getDecimal(const UDateTimePatternGenerator* dtpg, int32_t* pLength) {
  typedef decltype(&udatpg_getDecimal) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getDecimal"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getDecimal");
    abort();
  }

  return ptr(dtpg, pLength);
}
int32_t udatpg_replaceFieldTypes(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_replaceFieldTypes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_replaceFieldTypes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_replaceFieldTypes");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}
int32_t udatpg_replaceFieldTypesWithOptions(UDateTimePatternGenerator* dtpg, const UChar* pattern, int32_t patternLength, const UChar* skeleton, int32_t skeletonLength, UDateTimePatternMatchOptions options, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_replaceFieldTypesWithOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_replaceFieldTypesWithOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_replaceFieldTypesWithOptions");
    abort();
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, options, dest, destCapacity, pErrorCode);
}
UEnumeration* udatpg_openSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_openSkeletons) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_openSkeletons"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_openSkeletons");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
UEnumeration* udatpg_openBaseSkeletons(const UDateTimePatternGenerator* dtpg, UErrorCode* pErrorCode) {
  typedef decltype(&udatpg_openBaseSkeletons) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_openBaseSkeletons"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_openBaseSkeletons");
    abort();
  }

  return ptr(dtpg, pErrorCode);
}
const UChar* udatpg_getPatternForSkeleton(const UDateTimePatternGenerator* dtpg, const UChar* skeleton, int32_t skeletonLength, int32_t* pLength) {
  typedef decltype(&udatpg_getPatternForSkeleton) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udatpg_getPatternForSkeleton"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udatpg_getPatternForSkeleton");
    abort();
  }

  return ptr(dtpg, skeleton, skeletonLength, pLength);
}
USpoofChecker* uspoof_open(UErrorCode* status) {
  typedef decltype(&uspoof_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_open");
    abort();
  }

  return ptr(status);
}
USpoofChecker* uspoof_openFromSerialized(const void* data, int32_t length, int32_t* pActualLength, UErrorCode* pErrorCode) {
  typedef decltype(&uspoof_openFromSerialized) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_openFromSerialized"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_openFromSerialized");
    abort();
  }

  return ptr(data, length, pActualLength, pErrorCode);
}
USpoofChecker* uspoof_openFromSource(const char* confusables, int32_t confusablesLen, const char* confusablesWholeScript, int32_t confusablesWholeScriptLen, int32_t* errType, UParseError* pe, UErrorCode* status) {
  typedef decltype(&uspoof_openFromSource) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_openFromSource"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_openFromSource");
    abort();
  }

  return ptr(confusables, confusablesLen, confusablesWholeScript, confusablesWholeScriptLen, errType, pe, status);
}
void uspoof_close(USpoofChecker* sc) {
  typedef decltype(&uspoof_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_close");
    abort();
  }

  ptr(sc);
}
USpoofChecker* uspoof_clone(const USpoofChecker* sc, UErrorCode* status) {
  typedef decltype(&uspoof_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_clone");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setChecks(USpoofChecker* sc, int32_t checks, UErrorCode* status) {
  typedef decltype(&uspoof_setChecks) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_setChecks"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_setChecks");
    abort();
  }

  ptr(sc, checks, status);
}
int32_t uspoof_getChecks(const USpoofChecker* sc, UErrorCode* status) {
  typedef decltype(&uspoof_getChecks) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getChecks"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getChecks");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setRestrictionLevel(USpoofChecker* sc, URestrictionLevel restrictionLevel) {
  typedef decltype(&uspoof_setRestrictionLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_setRestrictionLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_setRestrictionLevel");
    abort();
  }

  ptr(sc, restrictionLevel);
}
URestrictionLevel uspoof_getRestrictionLevel(const USpoofChecker* sc) {
  typedef decltype(&uspoof_getRestrictionLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getRestrictionLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getRestrictionLevel");
    abort();
  }

  return ptr(sc);
}
void uspoof_setAllowedLocales(USpoofChecker* sc, const char* localesList, UErrorCode* status) {
  typedef decltype(&uspoof_setAllowedLocales) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_setAllowedLocales"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_setAllowedLocales");
    abort();
  }

  ptr(sc, localesList, status);
}
const char* uspoof_getAllowedLocales(USpoofChecker* sc, UErrorCode* status) {
  typedef decltype(&uspoof_getAllowedLocales) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getAllowedLocales"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getAllowedLocales");
    abort();
  }

  return ptr(sc, status);
}
void uspoof_setAllowedChars(USpoofChecker* sc, const USet* chars, UErrorCode* status) {
  typedef decltype(&uspoof_setAllowedChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_setAllowedChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_setAllowedChars");
    abort();
  }

  ptr(sc, chars, status);
}
const USet* uspoof_getAllowedChars(const USpoofChecker* sc, UErrorCode* status) {
  typedef decltype(&uspoof_getAllowedChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getAllowedChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getAllowedChars");
    abort();
  }

  return ptr(sc, status);
}
int32_t uspoof_check(const USpoofChecker* sc, const UChar* id, int32_t length, int32_t* position, UErrorCode* status) {
  typedef decltype(&uspoof_check) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_check"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_check");
    abort();
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_checkUTF8(const USpoofChecker* sc, const char* id, int32_t length, int32_t* position, UErrorCode* status) {
  typedef decltype(&uspoof_checkUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_checkUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_checkUTF8");
    abort();
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_check2(const USpoofChecker* sc, const UChar* id, int32_t length, USpoofCheckResult* checkResult, UErrorCode* status) {
  typedef decltype(&uspoof_check2) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_check2"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_check2");
    abort();
  }

  return ptr(sc, id, length, checkResult, status);
}
int32_t uspoof_check2UTF8(const USpoofChecker* sc, const char* id, int32_t length, USpoofCheckResult* checkResult, UErrorCode* status) {
  typedef decltype(&uspoof_check2UTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_check2UTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_check2UTF8");
    abort();
  }

  return ptr(sc, id, length, checkResult, status);
}
USpoofCheckResult* uspoof_openCheckResult(UErrorCode* status) {
  typedef decltype(&uspoof_openCheckResult) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_openCheckResult"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_openCheckResult");
    abort();
  }

  return ptr(status);
}
void uspoof_closeCheckResult(USpoofCheckResult* checkResult) {
  typedef decltype(&uspoof_closeCheckResult) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_closeCheckResult"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_closeCheckResult");
    abort();
  }

  ptr(checkResult);
}
int32_t uspoof_getCheckResultChecks(const USpoofCheckResult* checkResult, UErrorCode* status) {
  typedef decltype(&uspoof_getCheckResultChecks) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getCheckResultChecks"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getCheckResultChecks");
    abort();
  }

  return ptr(checkResult, status);
}
URestrictionLevel uspoof_getCheckResultRestrictionLevel(const USpoofCheckResult* checkResult, UErrorCode* status) {
  typedef decltype(&uspoof_getCheckResultRestrictionLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getCheckResultRestrictionLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getCheckResultRestrictionLevel");
    abort();
  }

  return ptr(checkResult, status);
}
const USet* uspoof_getCheckResultNumerics(const USpoofCheckResult* checkResult, UErrorCode* status) {
  typedef decltype(&uspoof_getCheckResultNumerics) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getCheckResultNumerics"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getCheckResultNumerics");
    abort();
  }

  return ptr(checkResult, status);
}
int32_t uspoof_areConfusable(const USpoofChecker* sc, const UChar* id1, int32_t length1, const UChar* id2, int32_t length2, UErrorCode* status) {
  typedef decltype(&uspoof_areConfusable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_areConfusable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_areConfusable");
    abort();
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_areConfusableUTF8(const USpoofChecker* sc, const char* id1, int32_t length1, const char* id2, int32_t length2, UErrorCode* status) {
  typedef decltype(&uspoof_areConfusableUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_areConfusableUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_areConfusableUTF8");
    abort();
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_getSkeleton(const USpoofChecker* sc, uint32_t type, const UChar* id, int32_t length, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uspoof_getSkeleton) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getSkeleton"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getSkeleton");
    abort();
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
int32_t uspoof_getSkeletonUTF8(const USpoofChecker* sc, uint32_t type, const char* id, int32_t length, char* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uspoof_getSkeletonUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getSkeletonUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getSkeletonUTF8");
    abort();
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
const USet* uspoof_getInclusionSet(UErrorCode* status) {
  typedef decltype(&uspoof_getInclusionSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getInclusionSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getInclusionSet");
    abort();
  }

  return ptr(status);
}
const USet* uspoof_getRecommendedSet(UErrorCode* status) {
  typedef decltype(&uspoof_getRecommendedSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_getRecommendedSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_getRecommendedSet");
    abort();
  }

  return ptr(status);
}
int32_t uspoof_serialize(USpoofChecker* sc, void* data, int32_t capacity, UErrorCode* status) {
  typedef decltype(&uspoof_serialize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uspoof_serialize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uspoof_serialize");
    abort();
  }

  return ptr(sc, data, capacity, status);
}
int32_t u_formatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  typedef decltype(&u_vformatMessage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vformatMessage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_formatMessage");
    abort();
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, args, status);
  va_end(args);
  return ret;
}
int32_t u_vformatMessage(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  typedef decltype(&u_vformatMessage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vformatMessage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_vformatMessage");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}
void u_parseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UErrorCode* status, ...) {
  typedef decltype(&u_vparseMessage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vparseMessage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_parseMessage");
    abort();
  }

  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, status);
  va_end(args);
  return;
}
void u_vparseMessage(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UErrorCode* status) {
  typedef decltype(&u_vparseMessage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vparseMessage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_vparseMessage");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
}
int32_t u_formatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, UErrorCode* status, ...) {
  typedef decltype(&u_vformatMessageWithError) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vformatMessageWithError"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_formatMessageWithError");
    abort();
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, parseError, args, status);
  va_end(args);
  return ret;
}
int32_t u_vformatMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, UChar* result, int32_t resultLength, UParseError* parseError, va_list ap, UErrorCode* status) {
  typedef decltype(&u_vformatMessageWithError) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vformatMessageWithError"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_vformatMessageWithError");
    abort();
  }

  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}
void u_parseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, UParseError* parseError, UErrorCode* status, ...) {
  typedef decltype(&u_vparseMessageWithError) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vparseMessageWithError"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_parseMessageWithError");
    abort();
  }

  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, parseError, status);
  va_end(args);
  return;
}
void u_vparseMessageWithError(const char* locale, const UChar* pattern, int32_t patternLength, const UChar* source, int32_t sourceLength, va_list ap, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&u_vparseMessageWithError) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_vparseMessageWithError"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_vparseMessageWithError");
    abort();
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
}
UMessageFormat* umsg_open(const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&umsg_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_open");
    abort();
  }

  return ptr(pattern, patternLength, locale, parseError, status);
}
void umsg_close(UMessageFormat* format) {
  typedef decltype(&umsg_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_close");
    abort();
  }

  ptr(format);
}
UMessageFormat umsg_clone(const UMessageFormat* fmt, UErrorCode* status) {
  typedef decltype(&umsg_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_clone");
    abort();
  }

  return ptr(fmt, status);
}
void umsg_setLocale(UMessageFormat* fmt, const char* locale) {
  typedef decltype(&umsg_setLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_setLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_setLocale");
    abort();
  }

  ptr(fmt, locale);
}
const char* umsg_getLocale(const UMessageFormat* fmt) {
  typedef decltype(&umsg_getLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_getLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_getLocale");
    abort();
  }

  return ptr(fmt);
}
void umsg_applyPattern(UMessageFormat* fmt, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&umsg_applyPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_applyPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_applyPattern");
    abort();
  }

  ptr(fmt, pattern, patternLength, parseError, status);
}
int32_t umsg_toPattern(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&umsg_toPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_toPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_toPattern");
    abort();
  }

  return ptr(fmt, result, resultLength, status);
}
int32_t umsg_format(const UMessageFormat* fmt, UChar* result, int32_t resultLength, UErrorCode* status, ...) {
  typedef decltype(&umsg_vformat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_vformat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_format");
    abort();
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(fmt, result, resultLength, args, status);
  va_end(args);
  return ret;
}
int32_t umsg_vformat(const UMessageFormat* fmt, UChar* result, int32_t resultLength, va_list ap, UErrorCode* status) {
  typedef decltype(&umsg_vformat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_vformat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_vformat");
    abort();
  }

  return ptr(fmt, result, resultLength, ap, status);
}
void umsg_parse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, UErrorCode* status, ...) {
  typedef decltype(&umsg_vparse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_vparse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_parse");
    abort();
  }

  va_list args;
  va_start(args, status);
  ptr(fmt, source, sourceLength, count, args, status);
  va_end(args);
  return;
}
void umsg_vparse(const UMessageFormat* fmt, const UChar* source, int32_t sourceLength, int32_t* count, va_list ap, UErrorCode* status) {
  typedef decltype(&umsg_vparse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_vparse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_vparse");
    abort();
  }

  ptr(fmt, source, sourceLength, count, ap, status);
}
int32_t umsg_autoQuoteApostrophe(const UChar* pattern, int32_t patternLength, UChar* dest, int32_t destCapacity, UErrorCode* ec) {
  typedef decltype(&umsg_autoQuoteApostrophe) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("umsg_autoQuoteApostrophe"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "umsg_autoQuoteApostrophe");
    abort();
  }

  return ptr(pattern, patternLength, dest, destCapacity, ec);
}
URelativeDateTimeFormatter* ureldatefmt_open(const char* locale, UNumberFormat* nfToAdopt, UDateRelativeDateTimeFormatterStyle width, UDisplayContext capitalizationContext, UErrorCode* status) {
  typedef decltype(&ureldatefmt_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ureldatefmt_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ureldatefmt_open");
    abort();
  }

  return ptr(locale, nfToAdopt, width, capitalizationContext, status);
}
void ureldatefmt_close(URelativeDateTimeFormatter* reldatefmt) {
  typedef decltype(&ureldatefmt_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ureldatefmt_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ureldatefmt_close");
    abort();
  }

  ptr(reldatefmt);
}
int32_t ureldatefmt_formatNumeric(const URelativeDateTimeFormatter* reldatefmt, double offset, URelativeDateTimeUnit unit, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  typedef decltype(&ureldatefmt_formatNumeric) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ureldatefmt_formatNumeric"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ureldatefmt_formatNumeric");
    abort();
  }

  return ptr(reldatefmt, offset, unit, result, resultCapacity, status);
}
int32_t ureldatefmt_format(const URelativeDateTimeFormatter* reldatefmt, double offset, URelativeDateTimeUnit unit, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  typedef decltype(&ureldatefmt_format) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ureldatefmt_format"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ureldatefmt_format");
    abort();
  }

  return ptr(reldatefmt, offset, unit, result, resultCapacity, status);
}
int32_t ureldatefmt_combineDateAndTime(const URelativeDateTimeFormatter* reldatefmt, const UChar* relativeDateString, int32_t relativeDateStringLen, const UChar* timeString, int32_t timeStringLen, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  typedef decltype(&ureldatefmt_combineDateAndTime) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ureldatefmt_combineDateAndTime"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ureldatefmt_combineDateAndTime");
    abort();
  }

  return ptr(reldatefmt, relativeDateString, relativeDateStringLen, timeString, timeStringLen, result, resultCapacity, status);
}
URegularExpression* uregex_open(const UChar* pattern, int32_t patternLength, uint32_t flags, UParseError* pe, UErrorCode* status) {
  typedef decltype(&uregex_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_open");
    abort();
  }

  return ptr(pattern, patternLength, flags, pe, status);
}
URegularExpression* uregex_openUText(UText* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  typedef decltype(&uregex_openUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_openUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_openUText");
    abort();
  }

  return ptr(pattern, flags, pe, status);
}
URegularExpression* uregex_openC(const char* pattern, uint32_t flags, UParseError* pe, UErrorCode* status) {
  typedef decltype(&uregex_openC) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_openC"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_openC");
    abort();
  }

  return ptr(pattern, flags, pe, status);
}
void uregex_close(URegularExpression* regexp) {
  typedef decltype(&uregex_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_close");
    abort();
  }

  ptr(regexp);
}
URegularExpression* uregex_clone(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_clone");
    abort();
  }

  return ptr(regexp, status);
}
const UChar* uregex_pattern(const URegularExpression* regexp, int32_t* patLength, UErrorCode* status) {
  typedef decltype(&uregex_pattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_pattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_pattern");
    abort();
  }

  return ptr(regexp, patLength, status);
}
UText* uregex_patternUText(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_patternUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_patternUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_patternUText");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_flags(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_flags) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_flags"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_flags");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setText(URegularExpression* regexp, const UChar* text, int32_t textLength, UErrorCode* status) {
  typedef decltype(&uregex_setText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setText");
    abort();
  }

  ptr(regexp, text, textLength, status);
}
void uregex_setUText(URegularExpression* regexp, UText* text, UErrorCode* status) {
  typedef decltype(&uregex_setUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setUText");
    abort();
  }

  ptr(regexp, text, status);
}
const UChar* uregex_getText(URegularExpression* regexp, int32_t* textLength, UErrorCode* status) {
  typedef decltype(&uregex_getText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getText");
    abort();
  }

  return ptr(regexp, textLength, status);
}
UText* uregex_getUText(URegularExpression* regexp, UText* dest, UErrorCode* status) {
  typedef decltype(&uregex_getUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getUText");
    abort();
  }

  return ptr(regexp, dest, status);
}
void uregex_refreshUText(URegularExpression* regexp, UText* text, UErrorCode* status) {
  typedef decltype(&uregex_refreshUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_refreshUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_refreshUText");
    abort();
  }

  ptr(regexp, text, status);
}
UBool uregex_matches(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_matches) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_matches"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_matches");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_matches64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_matches64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_matches64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_matches64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_lookingAt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_lookingAt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_lookingAt");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_lookingAt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_lookingAt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_lookingAt64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find(URegularExpression* regexp, int32_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_find) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_find"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_find");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find64(URegularExpression* regexp, int64_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_find64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_find64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_find64");
    abort();
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_findNext(URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_findNext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_findNext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_findNext");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_groupCount(URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_groupCount) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_groupCount"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_groupCount");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_groupNumberFromName(URegularExpression* regexp, const UChar* groupName, int32_t nameLength, UErrorCode* status) {
  typedef decltype(&uregex_groupNumberFromName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_groupNumberFromName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_groupNumberFromName");
    abort();
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_groupNumberFromCName(URegularExpression* regexp, const char* groupName, int32_t nameLength, UErrorCode* status) {
  typedef decltype(&uregex_groupNumberFromCName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_groupNumberFromCName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_groupNumberFromCName");
    abort();
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_group(URegularExpression* regexp, int32_t groupNum, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uregex_group) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_group"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_group");
    abort();
  }

  return ptr(regexp, groupNum, dest, destCapacity, status);
}
UText* uregex_groupUText(URegularExpression* regexp, int32_t groupNum, UText* dest, int64_t* groupLength, UErrorCode* status) {
  typedef decltype(&uregex_groupUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_groupUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_groupUText");
    abort();
  }

  return ptr(regexp, groupNum, dest, groupLength, status);
}
int32_t uregex_start(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  typedef decltype(&uregex_start) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_start"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_start");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_start64(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  typedef decltype(&uregex_start64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_start64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_start64");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int32_t uregex_end(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  typedef decltype(&uregex_end) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_end"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_end");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_end64(URegularExpression* regexp, int32_t groupNum, UErrorCode* status) {
  typedef decltype(&uregex_end64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_end64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_end64");
    abort();
  }

  return ptr(regexp, groupNum, status);
}
void uregex_reset(URegularExpression* regexp, int32_t index, UErrorCode* status) {
  typedef decltype(&uregex_reset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_reset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_reset");
    abort();
  }

  ptr(regexp, index, status);
}
void uregex_reset64(URegularExpression* regexp, int64_t index, UErrorCode* status) {
  typedef decltype(&uregex_reset64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_reset64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_reset64");
    abort();
  }

  ptr(regexp, index, status);
}
void uregex_setRegion(URegularExpression* regexp, int32_t regionStart, int32_t regionLimit, UErrorCode* status) {
  typedef decltype(&uregex_setRegion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setRegion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setRegion");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegion64(URegularExpression* regexp, int64_t regionStart, int64_t regionLimit, UErrorCode* status) {
  typedef decltype(&uregex_setRegion64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setRegion64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setRegion64");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegionAndStart(URegularExpression* regexp, int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode* status) {
  typedef decltype(&uregex_setRegionAndStart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setRegionAndStart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setRegionAndStart");
    abort();
  }

  ptr(regexp, regionStart, regionLimit, startIndex, status);
}
int32_t uregex_regionStart(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_regionStart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_regionStart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_regionStart");
    abort();
  }

  return ptr(regexp, status);
}
int64_t uregex_regionStart64(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_regionStart64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_regionStart64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_regionStart64");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_regionEnd(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_regionEnd) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_regionEnd"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_regionEnd");
    abort();
  }

  return ptr(regexp, status);
}
int64_t uregex_regionEnd64(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_regionEnd64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_regionEnd64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_regionEnd64");
    abort();
  }

  return ptr(regexp, status);
}
UBool uregex_hasTransparentBounds(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_hasTransparentBounds) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_hasTransparentBounds"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_hasTransparentBounds");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_useTransparentBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  typedef decltype(&uregex_useTransparentBounds) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_useTransparentBounds"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_useTransparentBounds");
    abort();
  }

  ptr(regexp, b, status);
}
UBool uregex_hasAnchoringBounds(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_hasAnchoringBounds) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_hasAnchoringBounds"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_hasAnchoringBounds");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_useAnchoringBounds(URegularExpression* regexp, UBool b, UErrorCode* status) {
  typedef decltype(&uregex_useAnchoringBounds) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_useAnchoringBounds"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_useAnchoringBounds");
    abort();
  }

  ptr(regexp, b, status);
}
UBool uregex_hitEnd(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_hitEnd) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_hitEnd"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_hitEnd");
    abort();
  }

  return ptr(regexp, status);
}
UBool uregex_requireEnd(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_requireEnd) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_requireEnd"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_requireEnd");
    abort();
  }

  return ptr(regexp, status);
}
int32_t uregex_replaceAll(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uregex_replaceAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_replaceAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_replaceAll");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText* uregex_replaceAllUText(URegularExpression* regexp, UText* replacement, UText* dest, UErrorCode* status) {
  typedef decltype(&uregex_replaceAllUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_replaceAllUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_replaceAllUText");
    abort();
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_replaceFirst(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar* destBuf, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uregex_replaceFirst) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_replaceFirst"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_replaceFirst");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText* uregex_replaceFirstUText(URegularExpression* regexp, UText* replacement, UText* dest, UErrorCode* status) {
  typedef decltype(&uregex_replaceFirstUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_replaceFirstUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_replaceFirstUText");
    abort();
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_appendReplacement(URegularExpression* regexp, const UChar* replacementText, int32_t replacementLength, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  typedef decltype(&uregex_appendReplacement) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_appendReplacement"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_appendReplacement");
    abort();
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
void uregex_appendReplacementUText(URegularExpression* regexp, UText* replacementText, UText* dest, UErrorCode* status) {
  typedef decltype(&uregex_appendReplacementUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_appendReplacementUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_appendReplacementUText");
    abort();
  }

  ptr(regexp, replacementText, dest, status);
}
int32_t uregex_appendTail(URegularExpression* regexp, UChar** destBuf, int32_t* destCapacity, UErrorCode* status) {
  typedef decltype(&uregex_appendTail) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_appendTail"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_appendTail");
    abort();
  }

  return ptr(regexp, destBuf, destCapacity, status);
}
UText* uregex_appendTailUText(URegularExpression* regexp, UText* dest, UErrorCode* status) {
  typedef decltype(&uregex_appendTailUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_appendTailUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_appendTailUText");
    abort();
  }

  return ptr(regexp, dest, status);
}
int32_t uregex_split(URegularExpression* regexp, UChar* destBuf, int32_t destCapacity, int32_t* requiredCapacity, UChar* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  typedef decltype(&uregex_split) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_split"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_split");
    abort();
  }

  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}
int32_t uregex_splitUText(URegularExpression* regexp, UText* destFields [], int32_t destFieldsCapacity, UErrorCode* status) {
  typedef decltype(&uregex_splitUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_splitUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_splitUText");
    abort();
  }

  return ptr(regexp, destFields, destFieldsCapacity, status);
}
void uregex_setTimeLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  typedef decltype(&uregex_setTimeLimit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setTimeLimit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setTimeLimit");
    abort();
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getTimeLimit(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_getTimeLimit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getTimeLimit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getTimeLimit");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setStackLimit(URegularExpression* regexp, int32_t limit, UErrorCode* status) {
  typedef decltype(&uregex_setStackLimit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setStackLimit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setStackLimit");
    abort();
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getStackLimit(const URegularExpression* regexp, UErrorCode* status) {
  typedef decltype(&uregex_getStackLimit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getStackLimit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getStackLimit");
    abort();
  }

  return ptr(regexp, status);
}
void uregex_setMatchCallback(URegularExpression* regexp, URegexMatchCallback* callback, const void* context, UErrorCode* status) {
  typedef decltype(&uregex_setMatchCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setMatchCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setMatchCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_getMatchCallback(const URegularExpression* regexp, URegexMatchCallback** callback, const void** context, UErrorCode* status) {
  typedef decltype(&uregex_getMatchCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getMatchCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getMatchCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_setFindProgressCallback(URegularExpression* regexp, URegexFindProgressCallback* callback, const void* context, UErrorCode* status) {
  typedef decltype(&uregex_setFindProgressCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_setFindProgressCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_setFindProgressCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
void uregex_getFindProgressCallback(const URegularExpression* regexp, URegexFindProgressCallback** callback, const void** context, UErrorCode* status) {
  typedef decltype(&uregex_getFindProgressCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregex_getFindProgressCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregex_getFindProgressCallback");
    abort();
  }

  ptr(regexp, callback, context, status);
}
UNumberingSystem* unumsys_open(const char* locale, UErrorCode* status) {
  typedef decltype(&unumsys_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_open");
    abort();
  }

  return ptr(locale, status);
}
UNumberingSystem* unumsys_openByName(const char* name, UErrorCode* status) {
  typedef decltype(&unumsys_openByName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_openByName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_openByName");
    abort();
  }

  return ptr(name, status);
}
void unumsys_close(UNumberingSystem* unumsys) {
  typedef decltype(&unumsys_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_close");
    abort();
  }

  ptr(unumsys);
}
UEnumeration* unumsys_openAvailableNames(UErrorCode* status) {
  typedef decltype(&unumsys_openAvailableNames) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_openAvailableNames"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_openAvailableNames");
    abort();
  }

  return ptr(status);
}
const char* unumsys_getName(const UNumberingSystem* unumsys) {
  typedef decltype(&unumsys_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_getName");
    abort();
  }

  return ptr(unumsys);
}
UBool unumsys_isAlgorithmic(const UNumberingSystem* unumsys) {
  typedef decltype(&unumsys_isAlgorithmic) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_isAlgorithmic"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_isAlgorithmic");
    abort();
  }

  return ptr(unumsys);
}
int32_t unumsys_getRadix(const UNumberingSystem* unumsys) {
  typedef decltype(&unumsys_getRadix) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_getRadix"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_getRadix");
    abort();
  }

  return ptr(unumsys);
}
int32_t unumsys_getDescription(const UNumberingSystem* unumsys, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&unumsys_getDescription) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unumsys_getDescription"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unumsys_getDescription");
    abort();
  }

  return ptr(unumsys, result, resultLength, status);
}
UCollator* ucol_open(const char* loc, UErrorCode* status) {
  typedef decltype(&ucol_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_open");
    abort();
  }

  return ptr(loc, status);
}
UCollator* ucol_openRules(const UChar* rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&ucol_openRules) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_openRules"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_openRules");
    abort();
  }

  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}
void ucol_getContractionsAndExpansions(const UCollator* coll, USet* contractions, USet* expansions, UBool addPrefixes, UErrorCode* status) {
  typedef decltype(&ucol_getContractionsAndExpansions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getContractionsAndExpansions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getContractionsAndExpansions");
    abort();
  }

  ptr(coll, contractions, expansions, addPrefixes, status);
}
void ucol_close(UCollator* coll) {
  typedef decltype(&ucol_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_close");
    abort();
  }

  ptr(coll);
}
UCollationResult ucol_strcoll(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  typedef decltype(&ucol_strcoll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_strcoll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_strcoll");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollUTF8(const UCollator* coll, const char* source, int32_t sourceLength, const char* target, int32_t targetLength, UErrorCode* status) {
  typedef decltype(&ucol_strcollUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_strcollUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_strcollUTF8");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength, status);
}
UBool ucol_greater(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  typedef decltype(&ucol_greater) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_greater"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_greater");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_greaterOrEqual(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  typedef decltype(&ucol_greaterOrEqual) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_greaterOrEqual"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_greaterOrEqual");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_equal(const UCollator* coll, const UChar* source, int32_t sourceLength, const UChar* target, int32_t targetLength) {
  typedef decltype(&ucol_equal) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_equal"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_equal");
    abort();
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollIter(const UCollator* coll, UCharIterator* sIter, UCharIterator* tIter, UErrorCode* status) {
  typedef decltype(&ucol_strcollIter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_strcollIter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_strcollIter");
    abort();
  }

  return ptr(coll, sIter, tIter, status);
}
UCollationStrength ucol_getStrength(const UCollator* coll) {
  typedef decltype(&ucol_getStrength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getStrength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getStrength");
    abort();
  }

  return ptr(coll);
}
void ucol_setStrength(UCollator* coll, UCollationStrength strength) {
  typedef decltype(&ucol_setStrength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setStrength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setStrength");
    abort();
  }

  ptr(coll, strength);
}
int32_t ucol_getReorderCodes(const UCollator* coll, int32_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&ucol_getReorderCodes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getReorderCodes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getReorderCodes");
    abort();
  }

  return ptr(coll, dest, destCapacity, pErrorCode);
}
void ucol_setReorderCodes(UCollator* coll, const int32_t* reorderCodes, int32_t reorderCodesLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucol_setReorderCodes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setReorderCodes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setReorderCodes");
    abort();
  }

  ptr(coll, reorderCodes, reorderCodesLength, pErrorCode);
}
int32_t ucol_getEquivalentReorderCodes(int32_t reorderCode, int32_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&ucol_getEquivalentReorderCodes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getEquivalentReorderCodes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getEquivalentReorderCodes");
    abort();
  }

  return ptr(reorderCode, dest, destCapacity, pErrorCode);
}
int32_t ucol_getDisplayName(const char* objLoc, const char* dispLoc, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&ucol_getDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getDisplayName");
    abort();
  }

  return ptr(objLoc, dispLoc, result, resultLength, status);
}
const char* ucol_getAvailable(int32_t localeIndex) {
  typedef decltype(&ucol_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t ucol_countAvailable(void) {
  typedef decltype(&ucol_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_countAvailable");
    abort();
  }

  return ptr();
}
UEnumeration* ucol_openAvailableLocales(UErrorCode* status) {
  typedef decltype(&ucol_openAvailableLocales) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_openAvailableLocales"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_openAvailableLocales");
    abort();
  }

  return ptr(status);
}
UEnumeration* ucol_getKeywords(UErrorCode* status) {
  typedef decltype(&ucol_getKeywords) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getKeywords"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getKeywords");
    abort();
  }

  return ptr(status);
}
UEnumeration* ucol_getKeywordValues(const char* keyword, UErrorCode* status) {
  typedef decltype(&ucol_getKeywordValues) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getKeywordValues"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getKeywordValues");
    abort();
  }

  return ptr(keyword, status);
}
UEnumeration* ucol_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  typedef decltype(&ucol_getKeywordValuesForLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getKeywordValuesForLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucol_getFunctionalEquivalent(char* result, int32_t resultCapacity, const char* keyword, const char* locale, UBool* isAvailable, UErrorCode* status) {
  typedef decltype(&ucol_getFunctionalEquivalent) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getFunctionalEquivalent"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getFunctionalEquivalent");
    abort();
  }

  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}
const UChar* ucol_getRules(const UCollator* coll, int32_t* length) {
  typedef decltype(&ucol_getRules) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getRules"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getRules");
    abort();
  }

  return ptr(coll, length);
}
int32_t ucol_getSortKey(const UCollator* coll, const UChar* source, int32_t sourceLength, uint8_t* result, int32_t resultLength) {
  typedef decltype(&ucol_getSortKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getSortKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getSortKey");
    abort();
  }

  return ptr(coll, source, sourceLength, result, resultLength);
}
int32_t ucol_nextSortKeyPart(const UCollator* coll, UCharIterator* iter, uint32_t state [ 2], uint8_t* dest, int32_t count, UErrorCode* status) {
  typedef decltype(&ucol_nextSortKeyPart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_nextSortKeyPart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_nextSortKeyPart");
    abort();
  }

  return ptr(coll, iter, state, dest, count, status);
}
int32_t ucol_getBound(const uint8_t* source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&ucol_getBound) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getBound"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getBound");
    abort();
  }

  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}
void ucol_getVersion(const UCollator* coll, UVersionInfo info) {
  typedef decltype(&ucol_getVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getVersion");
    abort();
  }

  ptr(coll, info);
}
void ucol_getUCAVersion(const UCollator* coll, UVersionInfo info) {
  typedef decltype(&ucol_getUCAVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getUCAVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getUCAVersion");
    abort();
  }

  ptr(coll, info);
}
int32_t ucol_mergeSortkeys(const uint8_t* src1, int32_t src1Length, const uint8_t* src2, int32_t src2Length, uint8_t* dest, int32_t destCapacity) {
  typedef decltype(&ucol_mergeSortkeys) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_mergeSortkeys"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_mergeSortkeys");
    abort();
  }

  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}
void ucol_setAttribute(UCollator* coll, UColAttribute attr, UColAttributeValue value, UErrorCode* status) {
  typedef decltype(&ucol_setAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setAttribute");
    abort();
  }

  ptr(coll, attr, value, status);
}
UColAttributeValue ucol_getAttribute(const UCollator* coll, UColAttribute attr, UErrorCode* status) {
  typedef decltype(&ucol_getAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getAttribute");
    abort();
  }

  return ptr(coll, attr, status);
}
void ucol_setMaxVariable(UCollator* coll, UColReorderCode group, UErrorCode* pErrorCode) {
  typedef decltype(&ucol_setMaxVariable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_setMaxVariable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_setMaxVariable");
    abort();
  }

  ptr(coll, group, pErrorCode);
}
UColReorderCode ucol_getMaxVariable(const UCollator* coll) {
  typedef decltype(&ucol_getMaxVariable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getMaxVariable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getMaxVariable");
    abort();
  }

  return ptr(coll);
}
uint32_t ucol_getVariableTop(const UCollator* coll, UErrorCode* status) {
  typedef decltype(&ucol_getVariableTop) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getVariableTop"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getVariableTop");
    abort();
  }

  return ptr(coll, status);
}
UCollator* ucol_safeClone(const UCollator* coll, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  typedef decltype(&ucol_safeClone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_safeClone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_safeClone");
    abort();
  }

  return ptr(coll, stackBuffer, pBufferSize, status);
}
int32_t ucol_getRulesEx(const UCollator* coll, UColRuleOption delta, UChar* buffer, int32_t bufferLen) {
  typedef decltype(&ucol_getRulesEx) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getRulesEx"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getRulesEx");
    abort();
  }

  return ptr(coll, delta, buffer, bufferLen);
}
const char* ucol_getLocaleByType(const UCollator* coll, ULocDataLocaleType type, UErrorCode* status) {
  typedef decltype(&ucol_getLocaleByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getLocaleByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getLocaleByType");
    abort();
  }

  return ptr(coll, type, status);
}
USet* ucol_getTailoredSet(const UCollator* coll, UErrorCode* status) {
  typedef decltype(&ucol_getTailoredSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_getTailoredSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_getTailoredSet");
    abort();
  }

  return ptr(coll, status);
}
int32_t ucol_cloneBinary(const UCollator* coll, uint8_t* buffer, int32_t capacity, UErrorCode* status) {
  typedef decltype(&ucol_cloneBinary) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_cloneBinary"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_cloneBinary");
    abort();
  }

  return ptr(coll, buffer, capacity, status);
}
UCollator* ucol_openBinary(const uint8_t* bin, int32_t length, const UCollator* base, UErrorCode* status) {
  typedef decltype(&ucol_openBinary) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucol_openBinary"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucol_openBinary");
    abort();
  }

  return ptr(bin, length, base, status);
}
UTransliterator* utrans_openU(const UChar* id, int32_t idLength, UTransDirection dir, const UChar* rules, int32_t rulesLength, UParseError* parseError, UErrorCode* pErrorCode) {
  typedef decltype(&utrans_openU) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_openU"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_openU");
    abort();
  }

  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}
UTransliterator* utrans_openInverse(const UTransliterator* trans, UErrorCode* status) {
  typedef decltype(&utrans_openInverse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_openInverse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_openInverse");
    abort();
  }

  return ptr(trans, status);
}
UTransliterator* utrans_clone(const UTransliterator* trans, UErrorCode* status) {
  typedef decltype(&utrans_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_clone");
    abort();
  }

  return ptr(trans, status);
}
void utrans_close(UTransliterator* trans) {
  typedef decltype(&utrans_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_close");
    abort();
  }

  ptr(trans);
}
const UChar* utrans_getUnicodeID(const UTransliterator* trans, int32_t* resultLength) {
  typedef decltype(&utrans_getUnicodeID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_getUnicodeID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_getUnicodeID");
    abort();
  }

  return ptr(trans, resultLength);
}
void utrans_register(UTransliterator* adoptedTrans, UErrorCode* status) {
  typedef decltype(&utrans_register) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_register"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_register");
    abort();
  }

  ptr(adoptedTrans, status);
}
void utrans_unregisterID(const UChar* id, int32_t idLength) {
  typedef decltype(&utrans_unregisterID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_unregisterID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_unregisterID");
    abort();
  }

  ptr(id, idLength);
}
void utrans_setFilter(UTransliterator* trans, const UChar* filterPattern, int32_t filterPatternLen, UErrorCode* status) {
  typedef decltype(&utrans_setFilter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_setFilter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_setFilter");
    abort();
  }

  ptr(trans, filterPattern, filterPatternLen, status);
}
int32_t utrans_countAvailableIDs(void) {
  typedef decltype(&utrans_countAvailableIDs) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_countAvailableIDs"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_countAvailableIDs");
    abort();
  }

  return ptr();
}
UEnumeration* utrans_openIDs(UErrorCode* pErrorCode) {
  typedef decltype(&utrans_openIDs) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_openIDs"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_openIDs");
    abort();
  }

  return ptr(pErrorCode);
}
void utrans_trans(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, int32_t start, int32_t* limit, UErrorCode* status) {
  typedef decltype(&utrans_trans) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_trans"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_trans");
    abort();
  }

  ptr(trans, rep, repFunc, start, limit, status);
}
void utrans_transIncremental(const UTransliterator* trans, UReplaceable* rep, UReplaceableCallbacks* repFunc, UTransPosition* pos, UErrorCode* status) {
  typedef decltype(&utrans_transIncremental) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_transIncremental"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_transIncremental");
    abort();
  }

  ptr(trans, rep, repFunc, pos, status);
}
void utrans_transUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, int32_t start, int32_t* limit, UErrorCode* status) {
  typedef decltype(&utrans_transUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_transUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_transUChars");
    abort();
  }

  ptr(trans, text, textLength, textCapacity, start, limit, status);
}
void utrans_transIncrementalUChars(const UTransliterator* trans, UChar* text, int32_t* textLength, int32_t textCapacity, UTransPosition* pos, UErrorCode* status) {
  typedef decltype(&utrans_transIncrementalUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_transIncrementalUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_transIncrementalUChars");
    abort();
  }

  ptr(trans, text, textLength, textCapacity, pos, status);
}
int32_t utrans_toRules(const UTransliterator* trans, UBool escapeUnprintable, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&utrans_toRules) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_toRules"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_toRules");
    abort();
  }

  return ptr(trans, escapeUnprintable, result, resultLength, status);
}
USet* utrans_getSourceSet(const UTransliterator* trans, UBool ignoreFilter, USet* fillIn, UErrorCode* status) {
  typedef decltype(&utrans_getSourceSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrans_getSourceSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrans_getSourceSet");
    abort();
  }

  return ptr(trans, ignoreFilter, fillIn, status);
}
UStringSearch* usearch_open(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const char* locale, UBreakIterator* breakiter, UErrorCode* status) {
  typedef decltype(&usearch_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_open");
    abort();
  }

  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}
UStringSearch* usearch_openFromCollator(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const UCollator* collator, UBreakIterator* breakiter, UErrorCode* status) {
  typedef decltype(&usearch_openFromCollator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_openFromCollator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_openFromCollator");
    abort();
  }

  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}
void usearch_close(UStringSearch* searchiter) {
  typedef decltype(&usearch_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_close");
    abort();
  }

  ptr(searchiter);
}
void usearch_setOffset(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  typedef decltype(&usearch_setOffset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setOffset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setOffset");
    abort();
  }

  ptr(strsrch, position, status);
}
int32_t usearch_getOffset(const UStringSearch* strsrch) {
  typedef decltype(&usearch_getOffset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getOffset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getOffset");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setAttribute(UStringSearch* strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode* status) {
  typedef decltype(&usearch_setAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setAttribute");
    abort();
  }

  ptr(strsrch, attribute, value, status);
}
USearchAttributeValue usearch_getAttribute(const UStringSearch* strsrch, USearchAttribute attribute) {
  typedef decltype(&usearch_getAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getAttribute");
    abort();
  }

  return ptr(strsrch, attribute);
}
int32_t usearch_getMatchedStart(const UStringSearch* strsrch) {
  typedef decltype(&usearch_getMatchedStart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getMatchedStart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getMatchedStart");
    abort();
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedLength(const UStringSearch* strsrch) {
  typedef decltype(&usearch_getMatchedLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getMatchedLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getMatchedLength");
    abort();
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedText(const UStringSearch* strsrch, UChar* result, int32_t resultCapacity, UErrorCode* status) {
  typedef decltype(&usearch_getMatchedText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getMatchedText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getMatchedText");
    abort();
  }

  return ptr(strsrch, result, resultCapacity, status);
}
void usearch_setBreakIterator(UStringSearch* strsrch, UBreakIterator* breakiter, UErrorCode* status) {
  typedef decltype(&usearch_setBreakIterator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setBreakIterator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setBreakIterator");
    abort();
  }

  ptr(strsrch, breakiter, status);
}
const UBreakIterator* usearch_getBreakIterator(const UStringSearch* strsrch) {
  typedef decltype(&usearch_getBreakIterator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getBreakIterator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getBreakIterator");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setText(UStringSearch* strsrch, const UChar* text, int32_t textlength, UErrorCode* status) {
  typedef decltype(&usearch_setText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setText");
    abort();
  }

  ptr(strsrch, text, textlength, status);
}
const UChar* usearch_getText(const UStringSearch* strsrch, int32_t* length) {
  typedef decltype(&usearch_getText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getText");
    abort();
  }

  return ptr(strsrch, length);
}
UCollator* usearch_getCollator(const UStringSearch* strsrch) {
  typedef decltype(&usearch_getCollator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getCollator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getCollator");
    abort();
  }

  return ptr(strsrch);
}
void usearch_setCollator(UStringSearch* strsrch, const UCollator* collator, UErrorCode* status) {
  typedef decltype(&usearch_setCollator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setCollator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setCollator");
    abort();
  }

  ptr(strsrch, collator, status);
}
void usearch_setPattern(UStringSearch* strsrch, const UChar* pattern, int32_t patternlength, UErrorCode* status) {
  typedef decltype(&usearch_setPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_setPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_setPattern");
    abort();
  }

  ptr(strsrch, pattern, patternlength, status);
}
const UChar* usearch_getPattern(const UStringSearch* strsrch, int32_t* length) {
  typedef decltype(&usearch_getPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_getPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_getPattern");
    abort();
  }

  return ptr(strsrch, length);
}
int32_t usearch_first(UStringSearch* strsrch, UErrorCode* status) {
  typedef decltype(&usearch_first) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_first"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_first");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_following(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  typedef decltype(&usearch_following) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_following"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_following");
    abort();
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_last(UStringSearch* strsrch, UErrorCode* status) {
  typedef decltype(&usearch_last) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_last"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_last");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_preceding(UStringSearch* strsrch, int32_t position, UErrorCode* status) {
  typedef decltype(&usearch_preceding) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_preceding"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_preceding");
    abort();
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_next(UStringSearch* strsrch, UErrorCode* status) {
  typedef decltype(&usearch_next) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_next"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_next");
    abort();
  }

  return ptr(strsrch, status);
}
int32_t usearch_previous(UStringSearch* strsrch, UErrorCode* status) {
  typedef decltype(&usearch_previous) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_previous"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_previous");
    abort();
  }

  return ptr(strsrch, status);
}
void usearch_reset(UStringSearch* strsrch) {
  typedef decltype(&usearch_reset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usearch_reset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usearch_reset");
    abort();
  }

  ptr(strsrch);
}
UNumberFormat* unum_open(UNumberFormatStyle style, const UChar* pattern, int32_t patternLength, const char* locale, UParseError* parseErr, UErrorCode* status) {
  typedef decltype(&unum_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_open");
    abort();
  }

  return ptr(style, pattern, patternLength, locale, parseErr, status);
}
void unum_close(UNumberFormat* fmt) {
  typedef decltype(&unum_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_close");
    abort();
  }

  ptr(fmt);
}
UNumberFormat* unum_clone(const UNumberFormat* fmt, UErrorCode* status) {
  typedef decltype(&unum_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_clone");
    abort();
  }

  return ptr(fmt, status);
}
int32_t unum_format(const UNumberFormat* fmt, int32_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_format) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_format"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_format");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatInt64(const UNumberFormat* fmt, int64_t number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_formatInt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_formatInt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_formatInt64");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDouble(const UNumberFormat* fmt, double number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_formatDouble) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_formatDouble"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_formatDouble");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDecimal(const UNumberFormat* fmt, const char* number, int32_t length, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_formatDecimal) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_formatDecimal"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_formatDecimal");
    abort();
  }

  return ptr(fmt, number, length, result, resultLength, pos, status);
}
int32_t unum_formatDoubleCurrency(const UNumberFormat* fmt, double number, UChar* currency, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_formatDoubleCurrency) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_formatDoubleCurrency"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_formatDoubleCurrency");
    abort();
  }

  return ptr(fmt, number, currency, result, resultLength, pos, status);
}
int32_t unum_formatUFormattable(const UNumberFormat* fmt, const UFormattable* number, UChar* result, int32_t resultLength, UFieldPosition* pos, UErrorCode* status) {
  typedef decltype(&unum_formatUFormattable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_formatUFormattable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_formatUFormattable");
    abort();
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_parse(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  typedef decltype(&unum_parse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parse");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int64_t unum_parseInt64(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  typedef decltype(&unum_parseInt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parseInt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parseInt64");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
double unum_parseDouble(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  typedef decltype(&unum_parseDouble) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parseDouble"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parseDouble");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int32_t unum_parseDecimal(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, char* outBuf, int32_t outBufLength, UErrorCode* status) {
  typedef decltype(&unum_parseDecimal) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parseDecimal"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parseDecimal");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, outBuf, outBufLength, status);
}
double unum_parseDoubleCurrency(const UNumberFormat* fmt, const UChar* text, int32_t textLength, int32_t* parsePos, UChar* currency, UErrorCode* status) {
  typedef decltype(&unum_parseDoubleCurrency) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parseDoubleCurrency"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parseDoubleCurrency");
    abort();
  }

  return ptr(fmt, text, textLength, parsePos, currency, status);
}
UFormattable* unum_parseToUFormattable(const UNumberFormat* fmt, UFormattable* result, const UChar* text, int32_t textLength, int32_t* parsePos, UErrorCode* status) {
  typedef decltype(&unum_parseToUFormattable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_parseToUFormattable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_parseToUFormattable");
    abort();
  }

  return ptr(fmt, result, text, textLength, parsePos, status);
}
void unum_applyPattern(UNumberFormat* format, UBool localized, const UChar* pattern, int32_t patternLength, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&unum_applyPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_applyPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_applyPattern");
    abort();
  }

  ptr(format, localized, pattern, patternLength, parseError, status);
}
const char* unum_getAvailable(int32_t localeIndex) {
  typedef decltype(&unum_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t unum_countAvailable(void) {
  typedef decltype(&unum_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_countAvailable");
    abort();
  }

  return ptr();
}
int32_t unum_getAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  typedef decltype(&unum_getAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getAttribute");
    abort();
  }

  return ptr(fmt, attr);
}
void unum_setAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, int32_t newValue) {
  typedef decltype(&unum_setAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_setAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_setAttribute");
    abort();
  }

  ptr(fmt, attr, newValue);
}
double unum_getDoubleAttribute(const UNumberFormat* fmt, UNumberFormatAttribute attr) {
  typedef decltype(&unum_getDoubleAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getDoubleAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getDoubleAttribute");
    abort();
  }

  return ptr(fmt, attr);
}
void unum_setDoubleAttribute(UNumberFormat* fmt, UNumberFormatAttribute attr, double newValue) {
  typedef decltype(&unum_setDoubleAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_setDoubleAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_setDoubleAttribute");
    abort();
  }

  ptr(fmt, attr, newValue);
}
int32_t unum_getTextAttribute(const UNumberFormat* fmt, UNumberFormatTextAttribute tag, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&unum_getTextAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getTextAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getTextAttribute");
    abort();
  }

  return ptr(fmt, tag, result, resultLength, status);
}
void unum_setTextAttribute(UNumberFormat* fmt, UNumberFormatTextAttribute tag, const UChar* newValue, int32_t newValueLength, UErrorCode* status) {
  typedef decltype(&unum_setTextAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_setTextAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_setTextAttribute");
    abort();
  }

  ptr(fmt, tag, newValue, newValueLength, status);
}
int32_t unum_toPattern(const UNumberFormat* fmt, UBool isPatternLocalized, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&unum_toPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_toPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_toPattern");
    abort();
  }

  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}
int32_t unum_getSymbol(const UNumberFormat* fmt, UNumberFormatSymbol symbol, UChar* buffer, int32_t size, UErrorCode* status) {
  typedef decltype(&unum_getSymbol) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getSymbol"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getSymbol");
    abort();
  }

  return ptr(fmt, symbol, buffer, size, status);
}
void unum_setSymbol(UNumberFormat* fmt, UNumberFormatSymbol symbol, const UChar* value, int32_t length, UErrorCode* status) {
  typedef decltype(&unum_setSymbol) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_setSymbol"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_setSymbol");
    abort();
  }

  ptr(fmt, symbol, value, length, status);
}
const char* unum_getLocaleByType(const UNumberFormat* fmt, ULocDataLocaleType type, UErrorCode* status) {
  typedef decltype(&unum_getLocaleByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getLocaleByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getLocaleByType");
    abort();
  }

  return ptr(fmt, type, status);
}
void unum_setContext(UNumberFormat* fmt, UDisplayContext value, UErrorCode* status) {
  typedef decltype(&unum_setContext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_setContext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_setContext");
    abort();
  }

  ptr(fmt, value, status);
}
UDisplayContext unum_getContext(const UNumberFormat* fmt, UDisplayContextType type, UErrorCode* status) {
  typedef decltype(&unum_getContext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unum_getContext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unum_getContext");
    abort();
  }

  return ptr(fmt, type, status);
}
const UGenderInfo* ugender_getInstance(const char* locale, UErrorCode* status) {
  typedef decltype(&ugender_getInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ugender_getInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ugender_getInstance");
    abort();
  }

  return ptr(locale, status);
}
UGender ugender_getListGender(const UGenderInfo* genderinfo, const UGender* genders, int32_t size, UErrorCode* status) {
  typedef decltype(&ugender_getListGender) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ugender_getListGender"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ugender_getListGender");
    abort();
  }

  return ptr(genderinfo, genders, size, status);
}
UFieldPositionIterator* ufieldpositer_open(UErrorCode* status) {
  typedef decltype(&ufieldpositer_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufieldpositer_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufieldpositer_open");
    abort();
  }

  return ptr(status);
}
void ufieldpositer_close(UFieldPositionIterator* fpositer) {
  typedef decltype(&ufieldpositer_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufieldpositer_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufieldpositer_close");
    abort();
  }

  ptr(fpositer);
}
int32_t ufieldpositer_next(UFieldPositionIterator* fpositer, int32_t* beginIndex, int32_t* endIndex) {
  typedef decltype(&ufieldpositer_next) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufieldpositer_next"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufieldpositer_next");
    abort();
  }

  return ptr(fpositer, beginIndex, endIndex);
}
UEnumeration* ucal_openTimeZoneIDEnumeration(USystemTimeZoneType zoneType, const char* region, const int32_t* rawOffset, UErrorCode* ec) {
  typedef decltype(&ucal_openTimeZoneIDEnumeration) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_openTimeZoneIDEnumeration"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_openTimeZoneIDEnumeration");
    abort();
  }

  return ptr(zoneType, region, rawOffset, ec);
}
UEnumeration* ucal_openTimeZones(UErrorCode* ec) {
  typedef decltype(&ucal_openTimeZones) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_openTimeZones"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_openTimeZones");
    abort();
  }

  return ptr(ec);
}
UEnumeration* ucal_openCountryTimeZones(const char* country, UErrorCode* ec) {
  typedef decltype(&ucal_openCountryTimeZones) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_openCountryTimeZones"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_openCountryTimeZones");
    abort();
  }

  return ptr(country, ec);
}
int32_t ucal_getDefaultTimeZone(UChar* result, int32_t resultCapacity, UErrorCode* ec) {
  typedef decltype(&ucal_getDefaultTimeZone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getDefaultTimeZone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getDefaultTimeZone");
    abort();
  }

  return ptr(result, resultCapacity, ec);
}
void ucal_setDefaultTimeZone(const UChar* zoneID, UErrorCode* ec) {
  typedef decltype(&ucal_setDefaultTimeZone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setDefaultTimeZone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setDefaultTimeZone");
    abort();
  }

  ptr(zoneID, ec);
}
int32_t ucal_getDSTSavings(const UChar* zoneID, UErrorCode* ec) {
  typedef decltype(&ucal_getDSTSavings) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getDSTSavings"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getDSTSavings");
    abort();
  }

  return ptr(zoneID, ec);
}
UDate ucal_getNow(void) {
  typedef decltype(&ucal_getNow) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getNow"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getNow");
    abort();
  }

  return ptr();
}
UCalendar* ucal_open(const UChar* zoneID, int32_t len, const char* locale, UCalendarType type, UErrorCode* status) {
  typedef decltype(&ucal_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_open");
    abort();
  }

  return ptr(zoneID, len, locale, type, status);
}
void ucal_close(UCalendar* cal) {
  typedef decltype(&ucal_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_close");
    abort();
  }

  ptr(cal);
}
UCalendar* ucal_clone(const UCalendar* cal, UErrorCode* status) {
  typedef decltype(&ucal_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_clone");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setTimeZone(UCalendar* cal, const UChar* zoneID, int32_t len, UErrorCode* status) {
  typedef decltype(&ucal_setTimeZone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setTimeZone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setTimeZone");
    abort();
  }

  ptr(cal, zoneID, len, status);
}
int32_t ucal_getTimeZoneID(const UCalendar* cal, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&ucal_getTimeZoneID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getTimeZoneID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getTimeZoneID");
    abort();
  }

  return ptr(cal, result, resultLength, status);
}
int32_t ucal_getTimeZoneDisplayName(const UCalendar* cal, UCalendarDisplayNameType type, const char* locale, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&ucal_getTimeZoneDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getTimeZoneDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getTimeZoneDisplayName");
    abort();
  }

  return ptr(cal, type, locale, result, resultLength, status);
}
UBool ucal_inDaylightTime(const UCalendar* cal, UErrorCode* status) {
  typedef decltype(&ucal_inDaylightTime) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_inDaylightTime"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_inDaylightTime");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setGregorianChange(UCalendar* cal, UDate date, UErrorCode* pErrorCode) {
  typedef decltype(&ucal_setGregorianChange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setGregorianChange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setGregorianChange");
    abort();
  }

  ptr(cal, date, pErrorCode);
}
UDate ucal_getGregorianChange(const UCalendar* cal, UErrorCode* pErrorCode) {
  typedef decltype(&ucal_getGregorianChange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getGregorianChange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getGregorianChange");
    abort();
  }

  return ptr(cal, pErrorCode);
}
int32_t ucal_getAttribute(const UCalendar* cal, UCalendarAttribute attr) {
  typedef decltype(&ucal_getAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getAttribute");
    abort();
  }

  return ptr(cal, attr);
}
void ucal_setAttribute(UCalendar* cal, UCalendarAttribute attr, int32_t newValue) {
  typedef decltype(&ucal_setAttribute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setAttribute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setAttribute");
    abort();
  }

  ptr(cal, attr, newValue);
}
const char* ucal_getAvailable(int32_t localeIndex) {
  typedef decltype(&ucal_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getAvailable");
    abort();
  }

  return ptr(localeIndex);
}
int32_t ucal_countAvailable(void) {
  typedef decltype(&ucal_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_countAvailable");
    abort();
  }

  return ptr();
}
UDate ucal_getMillis(const UCalendar* cal, UErrorCode* status) {
  typedef decltype(&ucal_getMillis) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getMillis"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getMillis");
    abort();
  }

  return ptr(cal, status);
}
void ucal_setMillis(UCalendar* cal, UDate dateTime, UErrorCode* status) {
  typedef decltype(&ucal_setMillis) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setMillis"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setMillis");
    abort();
  }

  ptr(cal, dateTime, status);
}
void ucal_setDate(UCalendar* cal, int32_t year, int32_t month, int32_t date, UErrorCode* status) {
  typedef decltype(&ucal_setDate) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setDate"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setDate");
    abort();
  }

  ptr(cal, year, month, date, status);
}
void ucal_setDateTime(UCalendar* cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode* status) {
  typedef decltype(&ucal_setDateTime) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_setDateTime"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_setDateTime");
    abort();
  }

  ptr(cal, year, month, date, hour, minute, second, status);
}
UBool ucal_equivalentTo(const UCalendar* cal1, const UCalendar* cal2) {
  typedef decltype(&ucal_equivalentTo) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_equivalentTo"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_equivalentTo");
    abort();
  }

  return ptr(cal1, cal2);
}
void ucal_add(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  typedef decltype(&ucal_add) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_add"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_add");
    abort();
  }

  ptr(cal, field, amount, status);
}
void ucal_roll(UCalendar* cal, UCalendarDateFields field, int32_t amount, UErrorCode* status) {
  typedef decltype(&ucal_roll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_roll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_roll");
    abort();
  }

  ptr(cal, field, amount, status);
}
int32_t ucal_get(const UCalendar* cal, UCalendarDateFields field, UErrorCode* status) {
  typedef decltype(&ucal_get) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_get"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_get");
    abort();
  }

  return ptr(cal, field, status);
}
void ucal_set(UCalendar* cal, UCalendarDateFields field, int32_t value) {
  typedef decltype(&ucal_set) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_set"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_set");
    abort();
  }

  ptr(cal, field, value);
}
UBool ucal_isSet(const UCalendar* cal, UCalendarDateFields field) {
  typedef decltype(&ucal_isSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_isSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_isSet");
    abort();
  }

  return ptr(cal, field);
}
void ucal_clearField(UCalendar* cal, UCalendarDateFields field) {
  typedef decltype(&ucal_clearField) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_clearField"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_clearField");
    abort();
  }

  ptr(cal, field);
}
void ucal_clear(UCalendar* calendar) {
  typedef decltype(&ucal_clear) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_clear"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_clear");
    abort();
  }

  ptr(calendar);
}
int32_t ucal_getLimit(const UCalendar* cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode* status) {
  typedef decltype(&ucal_getLimit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getLimit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getLimit");
    abort();
  }

  return ptr(cal, field, type, status);
}
const char* ucal_getLocaleByType(const UCalendar* cal, ULocDataLocaleType type, UErrorCode* status) {
  typedef decltype(&ucal_getLocaleByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getLocaleByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getLocaleByType");
    abort();
  }

  return ptr(cal, type, status);
}
const char* ucal_getTZDataVersion(UErrorCode* status) {
  typedef decltype(&ucal_getTZDataVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getTZDataVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getTZDataVersion");
    abort();
  }

  return ptr(status);
}
int32_t ucal_getCanonicalTimeZoneID(const UChar* id, int32_t len, UChar* result, int32_t resultCapacity, UBool* isSystemID, UErrorCode* status) {
  typedef decltype(&ucal_getCanonicalTimeZoneID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getCanonicalTimeZoneID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getCanonicalTimeZoneID");
    abort();
  }

  return ptr(id, len, result, resultCapacity, isSystemID, status);
}
const char* ucal_getType(const UCalendar* cal, UErrorCode* status) {
  typedef decltype(&ucal_getType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getType");
    abort();
  }

  return ptr(cal, status);
}
UEnumeration* ucal_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  typedef decltype(&ucal_getKeywordValuesForLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getKeywordValuesForLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
UCalendarWeekdayType ucal_getDayOfWeekType(const UCalendar* cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode* status) {
  typedef decltype(&ucal_getDayOfWeekType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getDayOfWeekType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getDayOfWeekType");
    abort();
  }

  return ptr(cal, dayOfWeek, status);
}
int32_t ucal_getWeekendTransition(const UCalendar* cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode* status) {
  typedef decltype(&ucal_getWeekendTransition) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getWeekendTransition"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getWeekendTransition");
    abort();
  }

  return ptr(cal, dayOfWeek, status);
}
UBool ucal_isWeekend(const UCalendar* cal, UDate date, UErrorCode* status) {
  typedef decltype(&ucal_isWeekend) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_isWeekend"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_isWeekend");
    abort();
  }

  return ptr(cal, date, status);
}
int32_t ucal_getFieldDifference(UCalendar* cal, UDate target, UCalendarDateFields field, UErrorCode* status) {
  typedef decltype(&ucal_getFieldDifference) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getFieldDifference"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getFieldDifference");
    abort();
  }

  return ptr(cal, target, field, status);
}
UBool ucal_getTimeZoneTransitionDate(const UCalendar* cal, UTimeZoneTransitionType type, UDate* transition, UErrorCode* status) {
  typedef decltype(&ucal_getTimeZoneTransitionDate) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getTimeZoneTransitionDate"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getTimeZoneTransitionDate");
    abort();
  }

  return ptr(cal, type, transition, status);
}
int32_t ucal_getWindowsTimeZoneID(const UChar* id, int32_t len, UChar* winid, int32_t winidCapacity, UErrorCode* status) {
  typedef decltype(&ucal_getWindowsTimeZoneID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getWindowsTimeZoneID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getWindowsTimeZoneID");
    abort();
  }

  return ptr(id, len, winid, winidCapacity, status);
}
int32_t ucal_getTimeZoneIDForWindowsID(const UChar* winid, int32_t len, const char* region, UChar* id, int32_t idCapacity, UErrorCode* status) {
  typedef decltype(&ucal_getTimeZoneIDForWindowsID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucal_getTimeZoneIDForWindowsID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucal_getTimeZoneIDForWindowsID");
    abort();
  }

  return ptr(winid, len, region, id, idCapacity, status);
}
UDateIntervalFormat* udtitvfmt_open(const char* locale, const UChar* skeleton, int32_t skeletonLength, const UChar* tzID, int32_t tzIDLength, UErrorCode* status) {
  typedef decltype(&udtitvfmt_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udtitvfmt_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udtitvfmt_open");
    abort();
  }

  return ptr(locale, skeleton, skeletonLength, tzID, tzIDLength, status);
}
void udtitvfmt_close(UDateIntervalFormat* formatter) {
  typedef decltype(&udtitvfmt_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udtitvfmt_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udtitvfmt_close");
    abort();
  }

  ptr(formatter);
}
int32_t udtitvfmt_format(const UDateIntervalFormat* formatter, UDate fromDate, UDate toDate, UChar* result, int32_t resultCapacity, UFieldPosition* position, UErrorCode* status) {
  typedef decltype(&udtitvfmt_format) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udtitvfmt_format"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udtitvfmt_format");
    abort();
  }

  return ptr(formatter, fromDate, toDate, result, resultCapacity, position, status);
}
ULocaleData* ulocdata_open(const char* localeID, UErrorCode* status) {
  typedef decltype(&ulocdata_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_open");
    abort();
  }

  return ptr(localeID, status);
}
void ulocdata_close(ULocaleData* uld) {
  typedef decltype(&ulocdata_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_close");
    abort();
  }

  ptr(uld);
}
void ulocdata_setNoSubstitute(ULocaleData* uld, UBool setting) {
  typedef decltype(&ulocdata_setNoSubstitute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_setNoSubstitute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_setNoSubstitute");
    abort();
  }

  ptr(uld, setting);
}
UBool ulocdata_getNoSubstitute(ULocaleData* uld) {
  typedef decltype(&ulocdata_getNoSubstitute) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getNoSubstitute"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getNoSubstitute");
    abort();
  }

  return ptr(uld);
}
USet* ulocdata_getExemplarSet(ULocaleData* uld, USet* fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode* status) {
  typedef decltype(&ulocdata_getExemplarSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getExemplarSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getExemplarSet");
    abort();
  }

  return ptr(uld, fillIn, options, extype, status);
}
int32_t ulocdata_getDelimiter(ULocaleData* uld, ULocaleDataDelimiterType type, UChar* result, int32_t resultLength, UErrorCode* status) {
  typedef decltype(&ulocdata_getDelimiter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getDelimiter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getDelimiter");
    abort();
  }

  return ptr(uld, type, result, resultLength, status);
}
UMeasurementSystem ulocdata_getMeasurementSystem(const char* localeID, UErrorCode* status) {
  typedef decltype(&ulocdata_getMeasurementSystem) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getMeasurementSystem"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getMeasurementSystem");
    abort();
  }

  return ptr(localeID, status);
}
void ulocdata_getPaperSize(const char* localeID, int32_t* height, int32_t* width, UErrorCode* status) {
  typedef decltype(&ulocdata_getPaperSize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getPaperSize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getPaperSize");
    abort();
  }

  ptr(localeID, height, width, status);
}
void ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode* status) {
  typedef decltype(&ulocdata_getCLDRVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getCLDRVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getCLDRVersion");
    abort();
  }

  ptr(versionArray, status);
}
int32_t ulocdata_getLocaleDisplayPattern(ULocaleData* uld, UChar* pattern, int32_t patternCapacity, UErrorCode* status) {
  typedef decltype(&ulocdata_getLocaleDisplayPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getLocaleDisplayPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getLocaleDisplayPattern");
    abort();
  }

  return ptr(uld, pattern, patternCapacity, status);
}
int32_t ulocdata_getLocaleSeparator(ULocaleData* uld, UChar* separator, int32_t separatorCapacity, UErrorCode* status) {
  typedef decltype(&ulocdata_getLocaleSeparator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ulocdata_getLocaleSeparator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ulocdata_getLocaleSeparator");
    abort();
  }

  return ptr(uld, separator, separatorCapacity, status);
}
UFormattable* ufmt_open(UErrorCode* status) {
  typedef decltype(&ufmt_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_open");
    abort();
  }

  return ptr(status);
}
void ufmt_close(UFormattable* fmt) {
  typedef decltype(&ufmt_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_close");
    abort();
  }

  ptr(fmt);
}
UFormattableType ufmt_getType(const UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getType");
    abort();
  }

  return ptr(fmt, status);
}
UBool ufmt_isNumeric(const UFormattable* fmt) {
  typedef decltype(&ufmt_isNumeric) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_isNumeric"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_isNumeric");
    abort();
  }

  return ptr(fmt);
}
UDate ufmt_getDate(const UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getDate) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getDate"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getDate");
    abort();
  }

  return ptr(fmt, status);
}
double ufmt_getDouble(UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getDouble) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getDouble"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getDouble");
    abort();
  }

  return ptr(fmt, status);
}
int32_t ufmt_getLong(UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getLong) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getLong"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getLong");
    abort();
  }

  return ptr(fmt, status);
}
int64_t ufmt_getInt64(UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getInt64) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getInt64"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getInt64");
    abort();
  }

  return ptr(fmt, status);
}
const void* ufmt_getObject(const UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getObject) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getObject"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getObject");
    abort();
  }

  return ptr(fmt, status);
}
const UChar* ufmt_getUChars(UFormattable* fmt, int32_t* len, UErrorCode* status) {
  typedef decltype(&ufmt_getUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getUChars");
    abort();
  }

  return ptr(fmt, len, status);
}
int32_t ufmt_getArrayLength(const UFormattable* fmt, UErrorCode* status) {
  typedef decltype(&ufmt_getArrayLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getArrayLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getArrayLength");
    abort();
  }

  return ptr(fmt, status);
}
UFormattable* ufmt_getArrayItemByIndex(UFormattable* fmt, int32_t n, UErrorCode* status) {
  typedef decltype(&ufmt_getArrayItemByIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getArrayItemByIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getArrayItemByIndex");
    abort();
  }

  return ptr(fmt, n, status);
}
const char* ufmt_getDecNumChars(UFormattable* fmt, int32_t* len, UErrorCode* status) {
  typedef decltype(&ufmt_getDecNumChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ufmt_getDecNumChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ufmt_getDecNumChars");
    abort();
  }

  return ptr(fmt, len, status);
}
const URegion* uregion_getRegionFromCode(const char* regionCode, UErrorCode* status) {
  typedef decltype(&uregion_getRegionFromCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getRegionFromCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getRegionFromCode");
    abort();
  }

  return ptr(regionCode, status);
}
const URegion* uregion_getRegionFromNumericCode(int32_t code, UErrorCode* status) {
  typedef decltype(&uregion_getRegionFromNumericCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getRegionFromNumericCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getRegionFromNumericCode");
    abort();
  }

  return ptr(code, status);
}
UEnumeration* uregion_getAvailable(URegionType type, UErrorCode* status) {
  typedef decltype(&uregion_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getAvailable");
    abort();
  }

  return ptr(type, status);
}
UBool uregion_areEqual(const URegion* uregion, const URegion* otherRegion) {
  typedef decltype(&uregion_areEqual) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_areEqual"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_areEqual");
    abort();
  }

  return ptr(uregion, otherRegion);
}
const URegion* uregion_getContainingRegion(const URegion* uregion) {
  typedef decltype(&uregion_getContainingRegion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getContainingRegion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getContainingRegion");
    abort();
  }

  return ptr(uregion);
}
const URegion* uregion_getContainingRegionOfType(const URegion* uregion, URegionType type) {
  typedef decltype(&uregion_getContainingRegionOfType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getContainingRegionOfType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getContainingRegionOfType");
    abort();
  }

  return ptr(uregion, type);
}
UEnumeration* uregion_getContainedRegions(const URegion* uregion, UErrorCode* status) {
  typedef decltype(&uregion_getContainedRegions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getContainedRegions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getContainedRegions");
    abort();
  }

  return ptr(uregion, status);
}
UEnumeration* uregion_getContainedRegionsOfType(const URegion* uregion, URegionType type, UErrorCode* status) {
  typedef decltype(&uregion_getContainedRegionsOfType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getContainedRegionsOfType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getContainedRegionsOfType");
    abort();
  }

  return ptr(uregion, type, status);
}
UBool uregion_contains(const URegion* uregion, const URegion* otherRegion) {
  typedef decltype(&uregion_contains) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_contains"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_contains");
    abort();
  }

  return ptr(uregion, otherRegion);
}
UEnumeration* uregion_getPreferredValues(const URegion* uregion, UErrorCode* status) {
  typedef decltype(&uregion_getPreferredValues) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getPreferredValues"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getPreferredValues");
    abort();
  }

  return ptr(uregion, status);
}
const char* uregion_getRegionCode(const URegion* uregion) {
  typedef decltype(&uregion_getRegionCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getRegionCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getRegionCode");
    abort();
  }

  return ptr(uregion);
}
int32_t uregion_getNumericCode(const URegion* uregion) {
  typedef decltype(&uregion_getNumericCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getNumericCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getNumericCode");
    abort();
  }

  return ptr(uregion);
}
URegionType uregion_getType(const URegion* uregion) {
  typedef decltype(&uregion_getType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uregion_getType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uregion_getType");
    abort();
  }

  return ptr(uregion);
}
const char* uloc_getDefault(void) {
  typedef decltype(&uloc_getDefault) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDefault"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDefault");
    abort();
  }

  return ptr();
}
void uloc_setDefault(const char* localeID, UErrorCode* status) {
  typedef decltype(&uloc_setDefault) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_setDefault"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_setDefault");
    abort();
  }

  ptr(localeID, status);
}
int32_t uloc_getLanguage(const char* localeID, char* language, int32_t languageCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getLanguage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getLanguage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getLanguage");
    abort();
  }

  return ptr(localeID, language, languageCapacity, err);
}
int32_t uloc_getScript(const char* localeID, char* script, int32_t scriptCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getScript) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getScript"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getScript");
    abort();
  }

  return ptr(localeID, script, scriptCapacity, err);
}
int32_t uloc_getCountry(const char* localeID, char* country, int32_t countryCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getCountry) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getCountry"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getCountry");
    abort();
  }

  return ptr(localeID, country, countryCapacity, err);
}
int32_t uloc_getVariant(const char* localeID, char* variant, int32_t variantCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getVariant) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getVariant"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getVariant");
    abort();
  }

  return ptr(localeID, variant, variantCapacity, err);
}
int32_t uloc_getName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getName");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
int32_t uloc_canonicalize(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  typedef decltype(&uloc_canonicalize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_canonicalize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_canonicalize");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
const char* uloc_getISO3Language(const char* localeID) {
  typedef decltype(&uloc_getISO3Language) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getISO3Language"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getISO3Language");
    abort();
  }

  return ptr(localeID);
}
const char* uloc_getISO3Country(const char* localeID) {
  typedef decltype(&uloc_getISO3Country) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getISO3Country"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getISO3Country");
    abort();
  }

  return ptr(localeID);
}
uint32_t uloc_getLCID(const char* localeID) {
  typedef decltype(&uloc_getLCID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getLCID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getLCID");
    abort();
  }

  return ptr(localeID);
}
int32_t uloc_getDisplayLanguage(const char* locale, const char* displayLocale, UChar* language, int32_t languageCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayLanguage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayLanguage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayLanguage");
    abort();
  }

  return ptr(locale, displayLocale, language, languageCapacity, status);
}
int32_t uloc_getDisplayScript(const char* locale, const char* displayLocale, UChar* script, int32_t scriptCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayScript) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayScript"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayScript");
    abort();
  }

  return ptr(locale, displayLocale, script, scriptCapacity, status);
}
int32_t uloc_getDisplayCountry(const char* locale, const char* displayLocale, UChar* country, int32_t countryCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayCountry) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayCountry"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayCountry");
    abort();
  }

  return ptr(locale, displayLocale, country, countryCapacity, status);
}
int32_t uloc_getDisplayVariant(const char* locale, const char* displayLocale, UChar* variant, int32_t variantCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayVariant) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayVariant"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayVariant");
    abort();
  }

  return ptr(locale, displayLocale, variant, variantCapacity, status);
}
int32_t uloc_getDisplayKeyword(const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayKeyword) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayKeyword"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayKeyword");
    abort();
  }

  return ptr(keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayKeywordValue(const char* locale, const char* keyword, const char* displayLocale, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getDisplayKeywordValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayKeywordValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayKeywordValue");
    abort();
  }

  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayName(const char* localeID, const char* inLocaleID, UChar* result, int32_t maxResultSize, UErrorCode* err) {
  typedef decltype(&uloc_getDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getDisplayName");
    abort();
  }

  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}
const char* uloc_getAvailable(int32_t n) {
  typedef decltype(&uloc_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getAvailable");
    abort();
  }

  return ptr(n);
}
int32_t uloc_countAvailable(void) {
  typedef decltype(&uloc_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_countAvailable");
    abort();
  }

  return ptr();
}
const char* const* uloc_getISOLanguages(void) {
  typedef decltype(&uloc_getISOLanguages) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getISOLanguages"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getISOLanguages");
    abort();
  }

  return ptr();
}
const char* const* uloc_getISOCountries(void) {
  typedef decltype(&uloc_getISOCountries) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getISOCountries"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getISOCountries");
    abort();
  }

  return ptr();
}
int32_t uloc_getParent(const char* localeID, char* parent, int32_t parentCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getParent) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getParent"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getParent");
    abort();
  }

  return ptr(localeID, parent, parentCapacity, err);
}
int32_t uloc_getBaseName(const char* localeID, char* name, int32_t nameCapacity, UErrorCode* err) {
  typedef decltype(&uloc_getBaseName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getBaseName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getBaseName");
    abort();
  }

  return ptr(localeID, name, nameCapacity, err);
}
UEnumeration* uloc_openKeywords(const char* localeID, UErrorCode* status) {
  typedef decltype(&uloc_openKeywords) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_openKeywords"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_openKeywords");
    abort();
  }

  return ptr(localeID, status);
}
int32_t uloc_getKeywordValue(const char* localeID, const char* keywordName, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getKeywordValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getKeywordValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getKeywordValue");
    abort();
  }

  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}
int32_t uloc_setKeywordValue(const char* keywordName, const char* keywordValue, char* buffer, int32_t bufferCapacity, UErrorCode* status) {
  typedef decltype(&uloc_setKeywordValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_setKeywordValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_setKeywordValue");
    abort();
  }

  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}
UBool uloc_isRightToLeft(const char* locale) {
  typedef decltype(&uloc_isRightToLeft) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_isRightToLeft"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_isRightToLeft");
    abort();
  }

  return ptr(locale);
}
ULayoutType uloc_getCharacterOrientation(const char* localeId, UErrorCode* status) {
  typedef decltype(&uloc_getCharacterOrientation) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getCharacterOrientation"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getCharacterOrientation");
    abort();
  }

  return ptr(localeId, status);
}
ULayoutType uloc_getLineOrientation(const char* localeId, UErrorCode* status) {
  typedef decltype(&uloc_getLineOrientation) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getLineOrientation"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getLineOrientation");
    abort();
  }

  return ptr(localeId, status);
}
int32_t uloc_acceptLanguageFromHTTP(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char* httpAcceptLanguage, UEnumeration* availableLocales, UErrorCode* status) {
  typedef decltype(&uloc_acceptLanguageFromHTTP) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_acceptLanguageFromHTTP"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_acceptLanguageFromHTTP");
    abort();
  }

  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}
int32_t uloc_acceptLanguage(char* result, int32_t resultAvailable, UAcceptResult* outResult, const char** acceptList, int32_t acceptListCount, UEnumeration* availableLocales, UErrorCode* status) {
  typedef decltype(&uloc_acceptLanguage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_acceptLanguage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_acceptLanguage");
    abort();
  }

  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}
int32_t uloc_getLocaleForLCID(uint32_t hostID, char* locale, int32_t localeCapacity, UErrorCode* status) {
  typedef decltype(&uloc_getLocaleForLCID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_getLocaleForLCID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_getLocaleForLCID");
    abort();
  }

  return ptr(hostID, locale, localeCapacity, status);
}
int32_t uloc_addLikelySubtags(const char* localeID, char* maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode* err) {
  typedef decltype(&uloc_addLikelySubtags) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_addLikelySubtags"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_addLikelySubtags");
    abort();
  }

  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}
int32_t uloc_minimizeSubtags(const char* localeID, char* minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode* err) {
  typedef decltype(&uloc_minimizeSubtags) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_minimizeSubtags"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_minimizeSubtags");
    abort();
  }

  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}
int32_t uloc_forLanguageTag(const char* langtag, char* localeID, int32_t localeIDCapacity, int32_t* parsedLength, UErrorCode* err) {
  typedef decltype(&uloc_forLanguageTag) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_forLanguageTag"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_forLanguageTag");
    abort();
  }

  return ptr(langtag, localeID, localeIDCapacity, parsedLength, err);
}
int32_t uloc_toLanguageTag(const char* localeID, char* langtag, int32_t langtagCapacity, UBool strict, UErrorCode* err) {
  typedef decltype(&uloc_toLanguageTag) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_toLanguageTag"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_toLanguageTag");
    abort();
  }

  return ptr(localeID, langtag, langtagCapacity, strict, err);
}
const char* uloc_toUnicodeLocaleKey(const char* keyword) {
  typedef decltype(&uloc_toUnicodeLocaleKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_toUnicodeLocaleKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_toUnicodeLocaleKey");
    abort();
  }

  return ptr(keyword);
}
const char* uloc_toUnicodeLocaleType(const char* keyword, const char* value) {
  typedef decltype(&uloc_toUnicodeLocaleType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_toUnicodeLocaleType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_toUnicodeLocaleType");
    abort();
  }

  return ptr(keyword, value);
}
const char* uloc_toLegacyKey(const char* keyword) {
  typedef decltype(&uloc_toLegacyKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_toLegacyKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_toLegacyKey");
    abort();
  }

  return ptr(keyword);
}
const char* uloc_toLegacyType(const char* keyword, const char* value) {
  typedef decltype(&uloc_toLegacyType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uloc_toLegacyType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uloc_toLegacyType");
    abort();
  }

  return ptr(keyword, value);
}
void u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode* status) {
  typedef decltype(&u_getDataVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getDataVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getDataVersion");
    abort();
  }

  ptr(dataVersionFillin, status);
}
UBool u_hasBinaryProperty(UChar32 c, UProperty which) {
  typedef decltype(&u_hasBinaryProperty) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_hasBinaryProperty"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_hasBinaryProperty");
    abort();
  }

  return ptr(c, which);
}
UBool u_isUAlphabetic(UChar32 c) {
  typedef decltype(&u_isUAlphabetic) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isUAlphabetic"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isUAlphabetic");
    abort();
  }

  return ptr(c);
}
UBool u_isULowercase(UChar32 c) {
  typedef decltype(&u_isULowercase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isULowercase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isULowercase");
    abort();
  }

  return ptr(c);
}
UBool u_isUUppercase(UChar32 c) {
  typedef decltype(&u_isUUppercase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isUUppercase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isUUppercase");
    abort();
  }

  return ptr(c);
}
UBool u_isUWhiteSpace(UChar32 c) {
  typedef decltype(&u_isUWhiteSpace) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isUWhiteSpace"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isUWhiteSpace");
    abort();
  }

  return ptr(c);
}
int32_t u_getIntPropertyValue(UChar32 c, UProperty which) {
  typedef decltype(&u_getIntPropertyValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getIntPropertyValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getIntPropertyValue");
    abort();
  }

  return ptr(c, which);
}
int32_t u_getIntPropertyMinValue(UProperty which) {
  typedef decltype(&u_getIntPropertyMinValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getIntPropertyMinValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getIntPropertyMinValue");
    abort();
  }

  return ptr(which);
}
int32_t u_getIntPropertyMaxValue(UProperty which) {
  typedef decltype(&u_getIntPropertyMaxValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getIntPropertyMaxValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getIntPropertyMaxValue");
    abort();
  }

  return ptr(which);
}
double u_getNumericValue(UChar32 c) {
  typedef decltype(&u_getNumericValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getNumericValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getNumericValue");
    abort();
  }

  return ptr(c);
}
UBool u_islower(UChar32 c) {
  typedef decltype(&u_islower) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_islower"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_islower");
    abort();
  }

  return ptr(c);
}
UBool u_isupper(UChar32 c) {
  typedef decltype(&u_isupper) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isupper"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isupper");
    abort();
  }

  return ptr(c);
}
UBool u_istitle(UChar32 c) {
  typedef decltype(&u_istitle) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_istitle"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_istitle");
    abort();
  }

  return ptr(c);
}
UBool u_isdigit(UChar32 c) {
  typedef decltype(&u_isdigit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isdigit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isdigit");
    abort();
  }

  return ptr(c);
}
UBool u_isalpha(UChar32 c) {
  typedef decltype(&u_isalpha) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isalpha"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isalpha");
    abort();
  }

  return ptr(c);
}
UBool u_isalnum(UChar32 c) {
  typedef decltype(&u_isalnum) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isalnum"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isalnum");
    abort();
  }

  return ptr(c);
}
UBool u_isxdigit(UChar32 c) {
  typedef decltype(&u_isxdigit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isxdigit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isxdigit");
    abort();
  }

  return ptr(c);
}
UBool u_ispunct(UChar32 c) {
  typedef decltype(&u_ispunct) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_ispunct"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_ispunct");
    abort();
  }

  return ptr(c);
}
UBool u_isgraph(UChar32 c) {
  typedef decltype(&u_isgraph) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isgraph"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isgraph");
    abort();
  }

  return ptr(c);
}
UBool u_isblank(UChar32 c) {
  typedef decltype(&u_isblank) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isblank"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isblank");
    abort();
  }

  return ptr(c);
}
UBool u_isdefined(UChar32 c) {
  typedef decltype(&u_isdefined) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isdefined"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isdefined");
    abort();
  }

  return ptr(c);
}
UBool u_isspace(UChar32 c) {
  typedef decltype(&u_isspace) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isspace"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isspace");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaSpaceChar(UChar32 c) {
  typedef decltype(&u_isJavaSpaceChar) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isJavaSpaceChar"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isJavaSpaceChar");
    abort();
  }

  return ptr(c);
}
UBool u_isWhitespace(UChar32 c) {
  typedef decltype(&u_isWhitespace) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isWhitespace"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isWhitespace");
    abort();
  }

  return ptr(c);
}
UBool u_iscntrl(UChar32 c) {
  typedef decltype(&u_iscntrl) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_iscntrl"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_iscntrl");
    abort();
  }

  return ptr(c);
}
UBool u_isISOControl(UChar32 c) {
  typedef decltype(&u_isISOControl) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isISOControl"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isISOControl");
    abort();
  }

  return ptr(c);
}
UBool u_isprint(UChar32 c) {
  typedef decltype(&u_isprint) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isprint"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isprint");
    abort();
  }

  return ptr(c);
}
UBool u_isbase(UChar32 c) {
  typedef decltype(&u_isbase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isbase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isbase");
    abort();
  }

  return ptr(c);
}
UCharDirection u_charDirection(UChar32 c) {
  typedef decltype(&u_charDirection) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charDirection"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charDirection");
    abort();
  }

  return ptr(c);
}
UBool u_isMirrored(UChar32 c) {
  typedef decltype(&u_isMirrored) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isMirrored"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isMirrored");
    abort();
  }

  return ptr(c);
}
UChar32 u_charMirror(UChar32 c) {
  typedef decltype(&u_charMirror) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charMirror"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charMirror");
    abort();
  }

  return ptr(c);
}
UChar32 u_getBidiPairedBracket(UChar32 c) {
  typedef decltype(&u_getBidiPairedBracket) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getBidiPairedBracket"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getBidiPairedBracket");
    abort();
  }

  return ptr(c);
}
int8_t u_charType(UChar32 c) {
  typedef decltype(&u_charType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charType");
    abort();
  }

  return ptr(c);
}
void u_enumCharTypes(UCharEnumTypeRange* enumRange, const void* context) {
  typedef decltype(&u_enumCharTypes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_enumCharTypes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_enumCharTypes");
    abort();
  }

  ptr(enumRange, context);
}
uint8_t u_getCombiningClass(UChar32 c) {
  typedef decltype(&u_getCombiningClass) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getCombiningClass"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getCombiningClass");
    abort();
  }

  return ptr(c);
}
int32_t u_charDigitValue(UChar32 c) {
  typedef decltype(&u_charDigitValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charDigitValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charDigitValue");
    abort();
  }

  return ptr(c);
}
UBlockCode ublock_getCode(UChar32 c) {
  typedef decltype(&ublock_getCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ublock_getCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ublock_getCode");
    abort();
  }

  return ptr(c);
}
int32_t u_charName(UChar32 code, UCharNameChoice nameChoice, char* buffer, int32_t bufferLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_charName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charName");
    abort();
  }

  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}
UChar32 u_charFromName(UCharNameChoice nameChoice, const char* name, UErrorCode* pErrorCode) {
  typedef decltype(&u_charFromName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charFromName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charFromName");
    abort();
  }

  return ptr(nameChoice, name, pErrorCode);
}
void u_enumCharNames(UChar32 start, UChar32 limit, UEnumCharNamesFn* fn, void* context, UCharNameChoice nameChoice, UErrorCode* pErrorCode) {
  typedef decltype(&u_enumCharNames) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_enumCharNames"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_enumCharNames");
    abort();
  }

  ptr(start, limit, fn, context, nameChoice, pErrorCode);
}
const char* u_getPropertyName(UProperty property, UPropertyNameChoice nameChoice) {
  typedef decltype(&u_getPropertyName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getPropertyName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getPropertyName");
    abort();
  }

  return ptr(property, nameChoice);
}
UProperty u_getPropertyEnum(const char* alias) {
  typedef decltype(&u_getPropertyEnum) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getPropertyEnum"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getPropertyEnum");
    abort();
  }

  return ptr(alias);
}
const char* u_getPropertyValueName(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  typedef decltype(&u_getPropertyValueName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getPropertyValueName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getPropertyValueName");
    abort();
  }

  return ptr(property, value, nameChoice);
}
int32_t u_getPropertyValueEnum(UProperty property, const char* alias) {
  typedef decltype(&u_getPropertyValueEnum) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getPropertyValueEnum"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getPropertyValueEnum");
    abort();
  }

  return ptr(property, alias);
}
UBool u_isIDStart(UChar32 c) {
  typedef decltype(&u_isIDStart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isIDStart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isIDStart");
    abort();
  }

  return ptr(c);
}
UBool u_isIDPart(UChar32 c) {
  typedef decltype(&u_isIDPart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isIDPart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isIDPart");
    abort();
  }

  return ptr(c);
}
UBool u_isIDIgnorable(UChar32 c) {
  typedef decltype(&u_isIDIgnorable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isIDIgnorable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isIDIgnorable");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaIDStart(UChar32 c) {
  typedef decltype(&u_isJavaIDStart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isJavaIDStart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isJavaIDStart");
    abort();
  }

  return ptr(c);
}
UBool u_isJavaIDPart(UChar32 c) {
  typedef decltype(&u_isJavaIDPart) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_isJavaIDPart"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_isJavaIDPart");
    abort();
  }

  return ptr(c);
}
UChar32 u_tolower(UChar32 c) {
  typedef decltype(&u_tolower) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_tolower"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_tolower");
    abort();
  }

  return ptr(c);
}
UChar32 u_toupper(UChar32 c) {
  typedef decltype(&u_toupper) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_toupper"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_toupper");
    abort();
  }

  return ptr(c);
}
UChar32 u_totitle(UChar32 c) {
  typedef decltype(&u_totitle) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_totitle"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_totitle");
    abort();
  }

  return ptr(c);
}
UChar32 u_foldCase(UChar32 c, uint32_t options) {
  typedef decltype(&u_foldCase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_foldCase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_foldCase");
    abort();
  }

  return ptr(c, options);
}
int32_t u_digit(UChar32 ch, int8_t radix) {
  typedef decltype(&u_digit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_digit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_digit");
    abort();
  }

  return ptr(ch, radix);
}
UChar32 u_forDigit(int32_t digit, int8_t radix) {
  typedef decltype(&u_forDigit) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_forDigit"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_forDigit");
    abort();
  }

  return ptr(digit, radix);
}
void u_charAge(UChar32 c, UVersionInfo versionArray) {
  typedef decltype(&u_charAge) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charAge"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charAge");
    abort();
  }

  ptr(c, versionArray);
}
void u_getUnicodeVersion(UVersionInfo versionArray) {
  typedef decltype(&u_getUnicodeVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getUnicodeVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getUnicodeVersion");
    abort();
  }

  ptr(versionArray);
}
int32_t u_getFC_NFKC_Closure(UChar32 c, UChar* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&u_getFC_NFKC_Closure) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getFC_NFKC_Closure"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getFC_NFKC_Closure");
    abort();
  }

  return ptr(c, dest, destCapacity, pErrorCode);
}
void UCNV_FROM_U_CALLBACK_STOP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_STOP) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_FROM_U_CALLBACK_STOP"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_FROM_U_CALLBACK_STOP");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_STOP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_STOP) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_TO_U_CALLBACK_STOP"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_TO_U_CALLBACK_STOP");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_FROM_U_CALLBACK_SKIP(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_SKIP) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_FROM_U_CALLBACK_SKIP"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_FROM_U_CALLBACK_SKIP");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_SUBSTITUTE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_SUBSTITUTE) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_FROM_U_CALLBACK_SUBSTITUTE"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_FROM_U_CALLBACK_SUBSTITUTE");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_ESCAPE(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_ESCAPE) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_FROM_U_CALLBACK_ESCAPE"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_FROM_U_CALLBACK_ESCAPE");
    abort();
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_SKIP(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_SKIP) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_TO_U_CALLBACK_SKIP"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_TO_U_CALLBACK_SKIP");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_SUBSTITUTE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_SUBSTITUTE) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_TO_U_CALLBACK_SUBSTITUTE"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_TO_U_CALLBACK_SUBSTITUTE");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_ESCAPE(const void* context, UConverterToUnicodeArgs* toUArgs, const char* codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode* err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_ESCAPE) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("UCNV_TO_U_CALLBACK_ESCAPE"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "UCNV_TO_U_CALLBACK_ESCAPE");
    abort();
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
UDataMemory* udata_open(const char* path, const char* type, const char* name, UErrorCode* pErrorCode) {
  typedef decltype(&udata_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_open");
    abort();
  }

  return ptr(path, type, name, pErrorCode);
}
UDataMemory* udata_openChoice(const char* path, const char* type, const char* name, UDataMemoryIsAcceptable* isAcceptable, void* context, UErrorCode* pErrorCode) {
  typedef decltype(&udata_openChoice) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_openChoice"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_openChoice");
    abort();
  }

  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}
void udata_close(UDataMemory* pData) {
  typedef decltype(&udata_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_close");
    abort();
  }

  ptr(pData);
}
const void* udata_getMemory(UDataMemory* pData) {
  typedef decltype(&udata_getMemory) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_getMemory"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_getMemory");
    abort();
  }

  return ptr(pData);
}
void udata_getInfo(UDataMemory* pData, UDataInfo* pInfo) {
  typedef decltype(&udata_getInfo) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_getInfo"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_getInfo");
    abort();
  }

  ptr(pData, pInfo);
}
void udata_setCommonData(const void* data, UErrorCode* err) {
  typedef decltype(&udata_setCommonData) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_setCommonData"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_setCommonData");
    abort();
  }

  ptr(data, err);
}
void udata_setAppData(const char* packageName, const void* data, UErrorCode* err) {
  typedef decltype(&udata_setAppData) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_setAppData"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_setAppData");
    abort();
  }

  ptr(packageName, data, err);
}
void udata_setFileAccess(UDataFileAccess access, UErrorCode* status) {
  typedef decltype(&udata_setFileAccess) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("udata_setFileAccess"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "udata_setFileAccess");
    abort();
  }

  ptr(access, status);
}
int ucnv_compareNames(const char* name1, const char* name2) {
  typedef decltype(&ucnv_compareNames) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_compareNames"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_compareNames");
    abort();
  }

  return ptr(name1, name2);
}
UConverter* ucnv_open(const char* converterName, UErrorCode* err) {
  typedef decltype(&ucnv_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_open");
    abort();
  }

  return ptr(converterName, err);
}
UConverter* ucnv_openU(const UChar* name, UErrorCode* err) {
  typedef decltype(&ucnv_openU) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_openU"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_openU");
    abort();
  }

  return ptr(name, err);
}
UConverter* ucnv_openCCSID(int32_t codepage, UConverterPlatform platform, UErrorCode* err) {
  typedef decltype(&ucnv_openCCSID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_openCCSID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_openCCSID");
    abort();
  }

  return ptr(codepage, platform, err);
}
UConverter* ucnv_openPackage(const char* packageName, const char* converterName, UErrorCode* err) {
  typedef decltype(&ucnv_openPackage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_openPackage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_openPackage");
    abort();
  }

  return ptr(packageName, converterName, err);
}
UConverter* ucnv_safeClone(const UConverter* cnv, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  typedef decltype(&ucnv_safeClone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_safeClone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_safeClone");
    abort();
  }

  return ptr(cnv, stackBuffer, pBufferSize, status);
}
void ucnv_close(UConverter* converter) {
  typedef decltype(&ucnv_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_close");
    abort();
  }

  ptr(converter);
}
void ucnv_getSubstChars(const UConverter* converter, char* subChars, int8_t* len, UErrorCode* err) {
  typedef decltype(&ucnv_getSubstChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getSubstChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getSubstChars");
    abort();
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstChars(UConverter* converter, const char* subChars, int8_t len, UErrorCode* err) {
  typedef decltype(&ucnv_setSubstChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setSubstChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setSubstChars");
    abort();
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstString(UConverter* cnv, const UChar* s, int32_t length, UErrorCode* err) {
  typedef decltype(&ucnv_setSubstString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setSubstString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setSubstString");
    abort();
  }

  ptr(cnv, s, length, err);
}
void ucnv_getInvalidChars(const UConverter* converter, char* errBytes, int8_t* len, UErrorCode* err) {
  typedef decltype(&ucnv_getInvalidChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getInvalidChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getInvalidChars");
    abort();
  }

  ptr(converter, errBytes, len, err);
}
void ucnv_getInvalidUChars(const UConverter* converter, UChar* errUChars, int8_t* len, UErrorCode* err) {
  typedef decltype(&ucnv_getInvalidUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getInvalidUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getInvalidUChars");
    abort();
  }

  ptr(converter, errUChars, len, err);
}
void ucnv_reset(UConverter* converter) {
  typedef decltype(&ucnv_reset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_reset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_reset");
    abort();
  }

  ptr(converter);
}
void ucnv_resetToUnicode(UConverter* converter) {
  typedef decltype(&ucnv_resetToUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_resetToUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_resetToUnicode");
    abort();
  }

  ptr(converter);
}
void ucnv_resetFromUnicode(UConverter* converter) {
  typedef decltype(&ucnv_resetFromUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_resetFromUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_resetFromUnicode");
    abort();
  }

  ptr(converter);
}
int8_t ucnv_getMaxCharSize(const UConverter* converter) {
  typedef decltype(&ucnv_getMaxCharSize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getMaxCharSize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getMaxCharSize");
    abort();
  }

  return ptr(converter);
}
int8_t ucnv_getMinCharSize(const UConverter* converter) {
  typedef decltype(&ucnv_getMinCharSize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getMinCharSize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getMinCharSize");
    abort();
  }

  return ptr(converter);
}
int32_t ucnv_getDisplayName(const UConverter* converter, const char* displayLocale, UChar* displayName, int32_t displayNameCapacity, UErrorCode* err) {
  typedef decltype(&ucnv_getDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getDisplayName");
    abort();
  }

  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}
const char* ucnv_getName(const UConverter* converter, UErrorCode* err) {
  typedef decltype(&ucnv_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getName");
    abort();
  }

  return ptr(converter, err);
}
int32_t ucnv_getCCSID(const UConverter* converter, UErrorCode* err) {
  typedef decltype(&ucnv_getCCSID) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getCCSID"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getCCSID");
    abort();
  }

  return ptr(converter, err);
}
UConverterPlatform ucnv_getPlatform(const UConverter* converter, UErrorCode* err) {
  typedef decltype(&ucnv_getPlatform) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getPlatform"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getPlatform");
    abort();
  }

  return ptr(converter, err);
}
UConverterType ucnv_getType(const UConverter* converter) {
  typedef decltype(&ucnv_getType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getType");
    abort();
  }

  return ptr(converter);
}
void ucnv_getStarters(const UConverter* converter, UBool starters [ 256], UErrorCode* err) {
  typedef decltype(&ucnv_getStarters) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getStarters"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getStarters");
    abort();
  }

  ptr(converter, starters, err);
}
void ucnv_getUnicodeSet(const UConverter* cnv, USet* setFillIn, UConverterUnicodeSet whichSet, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getUnicodeSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getUnicodeSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getUnicodeSet");
    abort();
  }

  ptr(cnv, setFillIn, whichSet, pErrorCode);
}
void ucnv_getToUCallBack(const UConverter* converter, UConverterToUCallback* action, const void** context) {
  typedef decltype(&ucnv_getToUCallBack) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getToUCallBack"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getToUCallBack");
    abort();
  }

  ptr(converter, action, context);
}
void ucnv_getFromUCallBack(const UConverter* converter, UConverterFromUCallback* action, const void** context) {
  typedef decltype(&ucnv_getFromUCallBack) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getFromUCallBack"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getFromUCallBack");
    abort();
  }

  ptr(converter, action, context);
}
void ucnv_setToUCallBack(UConverter* converter, UConverterToUCallback newAction, const void* newContext, UConverterToUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  typedef decltype(&ucnv_setToUCallBack) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setToUCallBack"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setToUCallBack");
    abort();
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_setFromUCallBack(UConverter* converter, UConverterFromUCallback newAction, const void* newContext, UConverterFromUCallback* oldAction, const void** oldContext, UErrorCode* err) {
  typedef decltype(&ucnv_setFromUCallBack) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setFromUCallBack"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setFromUCallBack");
    abort();
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_fromUnicode(UConverter* converter, char** target, const char* targetLimit, const UChar** source, const UChar* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  typedef decltype(&ucnv_fromUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_fromUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_fromUnicode");
    abort();
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
void ucnv_toUnicode(UConverter* converter, UChar** target, const UChar* targetLimit, const char** source, const char* sourceLimit, int32_t* offsets, UBool flush, UErrorCode* err) {
  typedef decltype(&ucnv_toUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_toUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_toUnicode");
    abort();
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
int32_t ucnv_fromUChars(UConverter* cnv, char* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_fromUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_fromUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_fromUChars");
    abort();
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucnv_toUChars(UConverter* cnv, UChar* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_toUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_toUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_toUChars");
    abort();
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
UChar32 ucnv_getNextUChar(UConverter* converter, const char** source, const char* sourceLimit, UErrorCode* err) {
  typedef decltype(&ucnv_getNextUChar) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getNextUChar"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getNextUChar");
    abort();
  }

  return ptr(converter, source, sourceLimit, err);
}
void ucnv_convertEx(UConverter* targetCnv, UConverter* sourceCnv, char** target, const char* targetLimit, const char** source, const char* sourceLimit, UChar* pivotStart, UChar** pivotSource, UChar** pivotTarget, const UChar* pivotLimit, UBool reset, UBool flush, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_convertEx) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_convertEx"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_convertEx");
    abort();
  }

  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
}
int32_t ucnv_convert(const char* toConverterName, const char* fromConverterName, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_convert) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_convert"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_convert");
    abort();
  }

  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_toAlgorithmic(UConverterType algorithmicType, UConverter* cnv, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_toAlgorithmic) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_toAlgorithmic"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_toAlgorithmic");
    abort();
  }

  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_fromAlgorithmic(UConverter* cnv, UConverterType algorithmicType, char* target, int32_t targetCapacity, const char* source, int32_t sourceLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_fromAlgorithmic) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_fromAlgorithmic"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_fromAlgorithmic");
    abort();
  }

  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_flushCache(void) {
  typedef decltype(&ucnv_flushCache) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_flushCache"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_flushCache");
    abort();
  }

  return ptr();
}
int32_t ucnv_countAvailable(void) {
  typedef decltype(&ucnv_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_countAvailable");
    abort();
  }

  return ptr();
}
const char* ucnv_getAvailableName(int32_t n) {
  typedef decltype(&ucnv_getAvailableName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getAvailableName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getAvailableName");
    abort();
  }

  return ptr(n);
}
UEnumeration* ucnv_openAllNames(UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_openAllNames) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_openAllNames"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_openAllNames");
    abort();
  }

  return ptr(pErrorCode);
}
uint16_t ucnv_countAliases(const char* alias, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_countAliases) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_countAliases"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_countAliases");
    abort();
  }

  return ptr(alias, pErrorCode);
}
const char* ucnv_getAlias(const char* alias, uint16_t n, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getAlias) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getAlias"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getAlias");
    abort();
  }

  return ptr(alias, n, pErrorCode);
}
void ucnv_getAliases(const char* alias, const char** aliases, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getAliases) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getAliases"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getAliases");
    abort();
  }

  ptr(alias, aliases, pErrorCode);
}
UEnumeration* ucnv_openStandardNames(const char* convName, const char* standard, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_openStandardNames) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_openStandardNames"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_openStandardNames");
    abort();
  }

  return ptr(convName, standard, pErrorCode);
}
uint16_t ucnv_countStandards(void) {
  typedef decltype(&ucnv_countStandards) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_countStandards"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_countStandards");
    abort();
  }

  return ptr();
}
const char* ucnv_getStandard(uint16_t n, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getStandard) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getStandard"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getStandard");
    abort();
  }

  return ptr(n, pErrorCode);
}
const char* ucnv_getStandardName(const char* name, const char* standard, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getStandardName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getStandardName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getStandardName");
    abort();
  }

  return ptr(name, standard, pErrorCode);
}
const char* ucnv_getCanonicalName(const char* alias, const char* standard, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_getCanonicalName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getCanonicalName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getCanonicalName");
    abort();
  }

  return ptr(alias, standard, pErrorCode);
}
const char* ucnv_getDefaultName(void) {
  typedef decltype(&ucnv_getDefaultName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_getDefaultName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_getDefaultName");
    abort();
  }

  return ptr();
}
void ucnv_setDefaultName(const char* name) {
  typedef decltype(&ucnv_setDefaultName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setDefaultName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setDefaultName");
    abort();
  }

  ptr(name);
}
void ucnv_fixFileSeparator(const UConverter* cnv, UChar* source, int32_t sourceLen) {
  typedef decltype(&ucnv_fixFileSeparator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_fixFileSeparator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_fixFileSeparator");
    abort();
  }

  ptr(cnv, source, sourceLen);
}
UBool ucnv_isAmbiguous(const UConverter* cnv) {
  typedef decltype(&ucnv_isAmbiguous) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_isAmbiguous"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_isAmbiguous");
    abort();
  }

  return ptr(cnv);
}
void ucnv_setFallback(UConverter* cnv, UBool usesFallback) {
  typedef decltype(&ucnv_setFallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_setFallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_setFallback");
    abort();
  }

  ptr(cnv, usesFallback);
}
UBool ucnv_usesFallback(const UConverter* cnv) {
  typedef decltype(&ucnv_usesFallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_usesFallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_usesFallback");
    abort();
  }

  return ptr(cnv);
}
const char* ucnv_detectUnicodeSignature(const char* source, int32_t sourceLength, int32_t* signatureLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucnv_detectUnicodeSignature) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_detectUnicodeSignature"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_detectUnicodeSignature");
    abort();
  }

  return ptr(source, sourceLength, signatureLength, pErrorCode);
}
int32_t ucnv_fromUCountPending(const UConverter* cnv, UErrorCode* status) {
  typedef decltype(&ucnv_fromUCountPending) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_fromUCountPending"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_fromUCountPending");
    abort();
  }

  return ptr(cnv, status);
}
int32_t ucnv_toUCountPending(const UConverter* cnv, UErrorCode* status) {
  typedef decltype(&ucnv_toUCountPending) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_toUCountPending"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_toUCountPending");
    abort();
  }

  return ptr(cnv, status);
}
UBool ucnv_isFixedWidth(UConverter* cnv, UErrorCode* status) {
  typedef decltype(&ucnv_isFixedWidth) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_isFixedWidth"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_isFixedWidth");
    abort();
  }

  return ptr(cnv, status);
}
UChar32 utf8_nextCharSafeBody(const uint8_t* s, int32_t* pi, int32_t length, UChar32 c, UBool strict) {
  typedef decltype(&utf8_nextCharSafeBody) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utf8_nextCharSafeBody"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utf8_nextCharSafeBody");
    abort();
  }

  return ptr(s, pi, length, c, strict);
}
int32_t utf8_appendCharSafeBody(uint8_t* s, int32_t i, int32_t length, UChar32 c, UBool* pIsError) {
  typedef decltype(&utf8_appendCharSafeBody) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utf8_appendCharSafeBody"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utf8_appendCharSafeBody");
    abort();
  }

  return ptr(s, i, length, c, pIsError);
}
UChar32 utf8_prevCharSafeBody(const uint8_t* s, int32_t start, int32_t* pi, UChar32 c, UBool strict) {
  typedef decltype(&utf8_prevCharSafeBody) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utf8_prevCharSafeBody"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utf8_prevCharSafeBody");
    abort();
  }

  return ptr(s, start, pi, c, strict);
}
int32_t utf8_back1SafeBody(const uint8_t* s, int32_t start, int32_t i) {
  typedef decltype(&utf8_back1SafeBody) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utf8_back1SafeBody"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utf8_back1SafeBody");
    abort();
  }

  return ptr(s, start, i);
}
UBiDi* ubidi_open(void) {
  typedef decltype(&ubidi_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_open");
    abort();
  }

  return ptr();
}
UBiDi* ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_openSized) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_openSized"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_openSized");
    abort();
  }

  return ptr(maxLength, maxRunCount, pErrorCode);
}
void ubidi_close(UBiDi* pBiDi) {
  typedef decltype(&ubidi_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_close");
    abort();
  }

  ptr(pBiDi);
}
void ubidi_setInverse(UBiDi* pBiDi, UBool isInverse) {
  typedef decltype(&ubidi_setInverse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setInverse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setInverse");
    abort();
  }

  ptr(pBiDi, isInverse);
}
UBool ubidi_isInverse(UBiDi* pBiDi) {
  typedef decltype(&ubidi_isInverse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_isInverse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_isInverse");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_orderParagraphsLTR(UBiDi* pBiDi, UBool orderParagraphsLTR) {
  typedef decltype(&ubidi_orderParagraphsLTR) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_orderParagraphsLTR"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_orderParagraphsLTR");
    abort();
  }

  ptr(pBiDi, orderParagraphsLTR);
}
UBool ubidi_isOrderParagraphsLTR(UBiDi* pBiDi) {
  typedef decltype(&ubidi_isOrderParagraphsLTR) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_isOrderParagraphsLTR"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_isOrderParagraphsLTR");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingMode(UBiDi* pBiDi, UBiDiReorderingMode reorderingMode) {
  typedef decltype(&ubidi_setReorderingMode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setReorderingMode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setReorderingMode");
    abort();
  }

  ptr(pBiDi, reorderingMode);
}
UBiDiReorderingMode ubidi_getReorderingMode(UBiDi* pBiDi) {
  typedef decltype(&ubidi_getReorderingMode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getReorderingMode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getReorderingMode");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingOptions(UBiDi* pBiDi, uint32_t reorderingOptions) {
  typedef decltype(&ubidi_setReorderingOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setReorderingOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setReorderingOptions");
    abort();
  }

  ptr(pBiDi, reorderingOptions);
}
uint32_t ubidi_getReorderingOptions(UBiDi* pBiDi) {
  typedef decltype(&ubidi_getReorderingOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getReorderingOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getReorderingOptions");
    abort();
  }

  return ptr(pBiDi);
}
void ubidi_setContext(UBiDi* pBiDi, const UChar* prologue, int32_t proLength, const UChar* epilogue, int32_t epiLength, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_setContext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setContext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setContext");
    abort();
  }

  ptr(pBiDi, prologue, proLength, epilogue, epiLength, pErrorCode);
}
void ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_setPara) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setPara"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setPara");
    abort();
  }

  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
}
void ubidi_setLine(const UBiDi* pParaBiDi, int32_t start, int32_t limit, UBiDi* pLineBiDi, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_setLine) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setLine"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setLine");
    abort();
  }

  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
}
UBiDiDirection ubidi_getDirection(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getDirection) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getDirection"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getDirection");
    abort();
  }

  return ptr(pBiDi);
}
UBiDiDirection ubidi_getBaseDirection(const UChar* text, int32_t length) {
  typedef decltype(&ubidi_getBaseDirection) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getBaseDirection"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getBaseDirection");
    abort();
  }

  return ptr(text, length);
}
const UChar* ubidi_getText(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getText");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getLength(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLength");
    abort();
  }

  return ptr(pBiDi);
}
UBiDiLevel ubidi_getParaLevel(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getParaLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getParaLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getParaLevel");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_countParagraphs(UBiDi* pBiDi) {
  typedef decltype(&ubidi_countParagraphs) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_countParagraphs"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_countParagraphs");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getParagraph(const UBiDi* pBiDi, int32_t charIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getParagraph) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getParagraph"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getParagraph");
    abort();
  }

  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
void ubidi_getParagraphByIndex(const UBiDi* pBiDi, int32_t paraIndex, int32_t* pParaStart, int32_t* pParaLimit, UBiDiLevel* pParaLevel, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getParagraphByIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getParagraphByIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getParagraphByIndex");
    abort();
  }

  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
UBiDiLevel ubidi_getLevelAt(const UBiDi* pBiDi, int32_t charIndex) {
  typedef decltype(&ubidi_getLevelAt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLevelAt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLevelAt");
    abort();
  }

  return ptr(pBiDi, charIndex);
}
const UBiDiLevel* ubidi_getLevels(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getLevels) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLevels"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLevels");
    abort();
  }

  return ptr(pBiDi, pErrorCode);
}
void ubidi_getLogicalRun(const UBiDi* pBiDi, int32_t logicalPosition, int32_t* pLogicalLimit, UBiDiLevel* pLevel) {
  typedef decltype(&ubidi_getLogicalRun) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLogicalRun"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLogicalRun");
    abort();
  }

  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
}
int32_t ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_countRuns) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_countRuns"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_countRuns");
    abort();
  }

  return ptr(pBiDi, pErrorCode);
}
UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength) {
  typedef decltype(&ubidi_getVisualRun) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getVisualRun"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getVisualRun");
    abort();
  }

  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}
int32_t ubidi_getVisualIndex(UBiDi* pBiDi, int32_t logicalIndex, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getVisualIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getVisualIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getVisualIndex");
    abort();
  }

  return ptr(pBiDi, logicalIndex, pErrorCode);
}
int32_t ubidi_getLogicalIndex(UBiDi* pBiDi, int32_t visualIndex, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getLogicalIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLogicalIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLogicalIndex");
    abort();
  }

  return ptr(pBiDi, visualIndex, pErrorCode);
}
void ubidi_getLogicalMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getLogicalMap) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getLogicalMap"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getLogicalMap");
    abort();
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_getVisualMap(UBiDi* pBiDi, int32_t* indexMap, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_getVisualMap) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getVisualMap"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getVisualMap");
    abort();
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_reorderLogical(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  typedef decltype(&ubidi_reorderLogical) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_reorderLogical"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_reorderLogical");
    abort();
  }

  ptr(levels, length, indexMap);
}
void ubidi_reorderVisual(const UBiDiLevel* levels, int32_t length, int32_t* indexMap) {
  typedef decltype(&ubidi_reorderVisual) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_reorderVisual"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_reorderVisual");
    abort();
  }

  ptr(levels, length, indexMap);
}
void ubidi_invertMap(const int32_t* srcMap, int32_t* destMap, int32_t length) {
  typedef decltype(&ubidi_invertMap) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_invertMap"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_invertMap");
    abort();
  }

  ptr(srcMap, destMap, length);
}
int32_t ubidi_getProcessedLength(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getProcessedLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getProcessedLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getProcessedLength");
    abort();
  }

  return ptr(pBiDi);
}
int32_t ubidi_getResultLength(const UBiDi* pBiDi) {
  typedef decltype(&ubidi_getResultLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getResultLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getResultLength");
    abort();
  }

  return ptr(pBiDi);
}
UCharDirection ubidi_getCustomizedClass(UBiDi* pBiDi, UChar32 c) {
  typedef decltype(&ubidi_getCustomizedClass) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getCustomizedClass"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getCustomizedClass");
    abort();
  }

  return ptr(pBiDi, c);
}
void ubidi_setClassCallback(UBiDi* pBiDi, UBiDiClassCallback* newFn, const void* newContext, UBiDiClassCallback** oldFn, const void** oldContext, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_setClassCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_setClassCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_setClassCallback");
    abort();
  }

  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
}
void ubidi_getClassCallback(UBiDi* pBiDi, UBiDiClassCallback** fn, const void** context) {
  typedef decltype(&ubidi_getClassCallback) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_getClassCallback"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_getClassCallback");
    abort();
  }

  ptr(pBiDi, fn, context);
}
int32_t ubidi_writeReordered(UBiDi* pBiDi, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_writeReordered) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_writeReordered"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_writeReordered");
    abort();
  }

  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}
int32_t ubidi_writeReverse(const UChar* src, int32_t srcLength, UChar* dest, int32_t destSize, uint16_t options, UErrorCode* pErrorCode) {
  typedef decltype(&ubidi_writeReverse) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubidi_writeReverse"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubidi_writeReverse");
    abort();
  }

  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}
int32_t u_strlen(const UChar* s) {
  typedef decltype(&u_strlen) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strlen"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strlen");
    abort();
  }

  return ptr(s);
}
int32_t u_countChar32(const UChar* s, int32_t length) {
  typedef decltype(&u_countChar32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_countChar32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_countChar32");
    abort();
  }

  return ptr(s, length);
}
UBool u_strHasMoreChar32Than(const UChar* s, int32_t length, int32_t number) {
  typedef decltype(&u_strHasMoreChar32Than) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strHasMoreChar32Than"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strHasMoreChar32Than");
    abort();
  }

  return ptr(s, length, number);
}
UChar* u_strcat(UChar* dst, const UChar* src) {
  typedef decltype(&u_strcat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcat");
    abort();
  }

  return ptr(dst, src);
}
UChar* u_strncat(UChar* dst, const UChar* src, int32_t n) {
  typedef decltype(&u_strncat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strncat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strncat");
    abort();
  }

  return ptr(dst, src, n);
}
UChar* u_strstr(const UChar* s, const UChar* substring) {
  typedef decltype(&u_strstr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strstr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strstr");
    abort();
  }

  return ptr(s, substring);
}
UChar* u_strFindFirst(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  typedef decltype(&u_strFindFirst) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFindFirst"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFindFirst");
    abort();
  }

  return ptr(s, length, substring, subLength);
}
UChar* u_strchr(const UChar* s, UChar c) {
  typedef decltype(&u_strchr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strchr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strchr");
    abort();
  }

  return ptr(s, c);
}
UChar* u_strchr32(const UChar* s, UChar32 c) {
  typedef decltype(&u_strchr32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strchr32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strchr32");
    abort();
  }

  return ptr(s, c);
}
UChar* u_strrstr(const UChar* s, const UChar* substring) {
  typedef decltype(&u_strrstr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strrstr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strrstr");
    abort();
  }

  return ptr(s, substring);
}
UChar* u_strFindLast(const UChar* s, int32_t length, const UChar* substring, int32_t subLength) {
  typedef decltype(&u_strFindLast) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFindLast"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFindLast");
    abort();
  }

  return ptr(s, length, substring, subLength);
}
UChar* u_strrchr(const UChar* s, UChar c) {
  typedef decltype(&u_strrchr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strrchr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strrchr");
    abort();
  }

  return ptr(s, c);
}
UChar* u_strrchr32(const UChar* s, UChar32 c) {
  typedef decltype(&u_strrchr32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strrchr32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strrchr32");
    abort();
  }

  return ptr(s, c);
}
UChar* u_strpbrk(const UChar* string, const UChar* matchSet) {
  typedef decltype(&u_strpbrk) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strpbrk"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strpbrk");
    abort();
  }

  return ptr(string, matchSet);
}
int32_t u_strcspn(const UChar* string, const UChar* matchSet) {
  typedef decltype(&u_strcspn) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcspn"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcspn");
    abort();
  }

  return ptr(string, matchSet);
}
int32_t u_strspn(const UChar* string, const UChar* matchSet) {
  typedef decltype(&u_strspn) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strspn"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strspn");
    abort();
  }

  return ptr(string, matchSet);
}
UChar* u_strtok_r(UChar* src, const UChar* delim, UChar** saveState) {
  typedef decltype(&u_strtok_r) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strtok_r"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strtok_r");
    abort();
  }

  return ptr(src, delim, saveState);
}
int32_t u_strcmp(const UChar* s1, const UChar* s2) {
  typedef decltype(&u_strcmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcmp");
    abort();
  }

  return ptr(s1, s2);
}
int32_t u_strcmpCodePointOrder(const UChar* s1, const UChar* s2) {
  typedef decltype(&u_strcmpCodePointOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcmpCodePointOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2);
}
int32_t u_strCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, UBool codePointOrder) {
  typedef decltype(&u_strCompare) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strCompare"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strCompare");
    abort();
  }

  return ptr(s1, length1, s2, length2, codePointOrder);
}
int32_t u_strCompareIter(UCharIterator* iter1, UCharIterator* iter2, UBool codePointOrder) {
  typedef decltype(&u_strCompareIter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strCompareIter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strCompareIter");
    abort();
  }

  return ptr(iter1, iter2, codePointOrder);
}
int32_t u_strCaseCompare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&u_strCaseCompare) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strCaseCompare"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strCaseCompare");
    abort();
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
int32_t u_strncmp(const UChar* ucs1, const UChar* ucs2, int32_t n) {
  typedef decltype(&u_strncmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strncmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strncmp");
    abort();
  }

  return ptr(ucs1, ucs2, n);
}
int32_t u_strncmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t n) {
  typedef decltype(&u_strncmpCodePointOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strncmpCodePointOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strncmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2, n);
}
int32_t u_strcasecmp(const UChar* s1, const UChar* s2, uint32_t options) {
  typedef decltype(&u_strcasecmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcasecmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcasecmp");
    abort();
  }

  return ptr(s1, s2, options);
}
int32_t u_strncasecmp(const UChar* s1, const UChar* s2, int32_t n, uint32_t options) {
  typedef decltype(&u_strncasecmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strncasecmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strncasecmp");
    abort();
  }

  return ptr(s1, s2, n, options);
}
int32_t u_memcasecmp(const UChar* s1, const UChar* s2, int32_t length, uint32_t options) {
  typedef decltype(&u_memcasecmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memcasecmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memcasecmp");
    abort();
  }

  return ptr(s1, s2, length, options);
}
UChar* u_strcpy(UChar* dst, const UChar* src) {
  typedef decltype(&u_strcpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strcpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strcpy");
    abort();
  }

  return ptr(dst, src);
}
UChar* u_strncpy(UChar* dst, const UChar* src, int32_t n) {
  typedef decltype(&u_strncpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strncpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strncpy");
    abort();
  }

  return ptr(dst, src, n);
}
UChar* u_uastrcpy(UChar* dst, const char* src) {
  typedef decltype(&u_uastrcpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_uastrcpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_uastrcpy");
    abort();
  }

  return ptr(dst, src);
}
UChar* u_uastrncpy(UChar* dst, const char* src, int32_t n) {
  typedef decltype(&u_uastrncpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_uastrncpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_uastrncpy");
    abort();
  }

  return ptr(dst, src, n);
}
char* u_austrcpy(char* dst, const UChar* src) {
  typedef decltype(&u_austrcpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_austrcpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_austrcpy");
    abort();
  }

  return ptr(dst, src);
}
char* u_austrncpy(char* dst, const UChar* src, int32_t n) {
  typedef decltype(&u_austrncpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_austrncpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_austrncpy");
    abort();
  }

  return ptr(dst, src, n);
}
UChar* u_memcpy(UChar* dest, const UChar* src, int32_t count) {
  typedef decltype(&u_memcpy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memcpy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memcpy");
    abort();
  }

  return ptr(dest, src, count);
}
UChar* u_memmove(UChar* dest, const UChar* src, int32_t count) {
  typedef decltype(&u_memmove) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memmove"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memmove");
    abort();
  }

  return ptr(dest, src, count);
}
UChar* u_memset(UChar* dest, UChar c, int32_t count) {
  typedef decltype(&u_memset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memset");
    abort();
  }

  return ptr(dest, c, count);
}
int32_t u_memcmp(const UChar* buf1, const UChar* buf2, int32_t count) {
  typedef decltype(&u_memcmp) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memcmp"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memcmp");
    abort();
  }

  return ptr(buf1, buf2, count);
}
int32_t u_memcmpCodePointOrder(const UChar* s1, const UChar* s2, int32_t count) {
  typedef decltype(&u_memcmpCodePointOrder) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memcmpCodePointOrder"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memcmpCodePointOrder");
    abort();
  }

  return ptr(s1, s2, count);
}
UChar* u_memchr(const UChar* s, UChar c, int32_t count) {
  typedef decltype(&u_memchr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memchr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memchr");
    abort();
  }

  return ptr(s, c, count);
}
UChar* u_memchr32(const UChar* s, UChar32 c, int32_t count) {
  typedef decltype(&u_memchr32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memchr32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memchr32");
    abort();
  }

  return ptr(s, c, count);
}
UChar* u_memrchr(const UChar* s, UChar c, int32_t count) {
  typedef decltype(&u_memrchr) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memrchr"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memrchr");
    abort();
  }

  return ptr(s, c, count);
}
UChar* u_memrchr32(const UChar* s, UChar32 c, int32_t count) {
  typedef decltype(&u_memrchr32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_memrchr32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_memrchr32");
    abort();
  }

  return ptr(s, c, count);
}
int32_t u_unescape(const char* src, UChar* dest, int32_t destCapacity) {
  typedef decltype(&u_unescape) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_unescape"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_unescape");
    abort();
  }

  return ptr(src, dest, destCapacity);
}
UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t* offset, int32_t length, void* context) {
  typedef decltype(&u_unescapeAt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_unescapeAt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_unescapeAt");
    abort();
  }

  return ptr(charAt, offset, length, context);
}
int32_t u_strToUpper(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToUpper) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToUpper"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToUpper");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToLower(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, const char* locale, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToLower) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToLower"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToLower");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToTitle(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UBreakIterator* titleIter, const char* locale, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToTitle) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToTitle"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToTitle");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}
int32_t u_strFoldCase(UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFoldCase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFoldCase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFoldCase");
    abort();
  }

  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}
wchar_t* u_strToWCS(wchar_t* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToWCS) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToWCS"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToWCS");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar* u_strFromWCS(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const wchar_t* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromWCS) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromWCS"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromWCS");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char* u_strToUTF8(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar* u_strFromUTF8(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char* u_strToUTF8WithSub(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToUTF8WithSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToUTF8WithSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar* u_strFromUTF8WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromUTF8WithSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromUTF8WithSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar* u_strFromUTF8Lenient(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromUTF8Lenient) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromUTF8Lenient"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromUTF8Lenient");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32* u_strToUTF32(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToUTF32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToUTF32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToUTF32");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar* u_strFromUTF32(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromUTF32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromUTF32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromUTF32");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32* u_strToUTF32WithSub(UChar32* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToUTF32WithSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToUTF32WithSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToUTF32WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar* u_strFromUTF32WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const UChar32* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromUTF32WithSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromUTF32WithSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromUTF32WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
char* u_strToJavaModifiedUTF8(char* dest, int32_t destCapacity, int32_t* pDestLength, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&u_strToJavaModifiedUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strToJavaModifiedUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strToJavaModifiedUTF8");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar* u_strFromJavaModifiedUTF8WithSub(UChar* dest, int32_t destCapacity, int32_t* pDestLength, const char* src, int32_t srcLength, UChar32 subchar, int32_t* pNumSubstitutions, UErrorCode* pErrorCode) {
  typedef decltype(&u_strFromJavaModifiedUTF8WithSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_strFromJavaModifiedUTF8WithSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_strFromJavaModifiedUTF8WithSub");
    abort();
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
u_nl_catd u_catopen(const char* name, const char* locale, UErrorCode* ec) {
  typedef decltype(&u_catopen) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_catopen"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_catopen");
    abort();
  }

  return ptr(name, locale, ec);
}
void u_catclose(u_nl_catd catd) {
  typedef decltype(&u_catclose) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_catclose"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_catclose");
    abort();
  }

  ptr(catd);
}
const UChar* u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar* s, int32_t* len, UErrorCode* ec) {
  typedef decltype(&u_catgets) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_catgets"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_catgets");
    abort();
  }

  return ptr(catd, set_num, msg_num, s, len, ec);
}
UIDNA* uidna_openUTS46(uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_openUTS46) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_openUTS46"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_openUTS46");
    abort();
  }

  return ptr(options, pErrorCode);
}
void uidna_close(UIDNA* idna) {
  typedef decltype(&uidna_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_close");
    abort();
  }

  ptr(idna);
}
int32_t uidna_labelToASCII(const UIDNA* idna, const UChar* label, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_labelToASCII) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_labelToASCII"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_labelToASCII");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicode(const UIDNA* idna, const UChar* label, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_labelToUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_labelToUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_labelToUnicode");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII(const UIDNA* idna, const UChar* name, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_nameToASCII) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_nameToASCII"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_nameToASCII");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicode(const UIDNA* idna, const UChar* name, int32_t length, UChar* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_nameToUnicode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_nameToUnicode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_nameToUnicode");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToASCII_UTF8(const UIDNA* idna, const char* label, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_labelToASCII_UTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_labelToASCII_UTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_labelToASCII_UTF8");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicodeUTF8(const UIDNA* idna, const char* label, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_labelToUnicodeUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_labelToUnicodeUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_labelToUnicodeUTF8");
    abort();
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII_UTF8(const UIDNA* idna, const char* name, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_nameToASCII_UTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_nameToASCII_UTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_nameToASCII_UTF8");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicodeUTF8(const UIDNA* idna, const char* name, int32_t length, char* dest, int32_t capacity, UIDNAInfo* pInfo, UErrorCode* pErrorCode) {
  typedef decltype(&uidna_nameToUnicodeUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uidna_nameToUnicodeUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uidna_nameToUnicodeUTF8");
    abort();
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
void ucnv_cbFromUWriteBytes(UConverterFromUnicodeArgs* args, const char* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  typedef decltype(&ucnv_cbFromUWriteBytes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_cbFromUWriteBytes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_cbFromUWriteBytes");
    abort();
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbFromUWriteSub(UConverterFromUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  typedef decltype(&ucnv_cbFromUWriteSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_cbFromUWriteSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_cbFromUWriteSub");
    abort();
  }

  ptr(args, offsetIndex, err);
}
void ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs* args, const UChar** source, const UChar* sourceLimit, int32_t offsetIndex, UErrorCode* err) {
  typedef decltype(&ucnv_cbFromUWriteUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_cbFromUWriteUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_cbFromUWriteUChars");
    abort();
  }

  ptr(args, source, sourceLimit, offsetIndex, err);
}
void ucnv_cbToUWriteUChars(UConverterToUnicodeArgs* args, const UChar* source, int32_t length, int32_t offsetIndex, UErrorCode* err) {
  typedef decltype(&ucnv_cbToUWriteUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_cbToUWriteUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_cbToUWriteUChars");
    abort();
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbToUWriteSub(UConverterToUnicodeArgs* args, int32_t offsetIndex, UErrorCode* err) {
  typedef decltype(&ucnv_cbToUWriteSub) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnv_cbToUWriteSub"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnv_cbToUWriteSub");
    abort();
  }

  ptr(args, offsetIndex, err);
}
ULocaleDisplayNames* uldn_open(const char* locale, UDialectHandling dialectHandling, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_open");
    abort();
  }

  return ptr(locale, dialectHandling, pErrorCode);
}
void uldn_close(ULocaleDisplayNames* ldn) {
  typedef decltype(&uldn_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_close");
    abort();
  }

  ptr(ldn);
}
const char* uldn_getLocale(const ULocaleDisplayNames* ldn) {
  typedef decltype(&uldn_getLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_getLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_getLocale");
    abort();
  }

  return ptr(ldn);
}
UDialectHandling uldn_getDialectHandling(const ULocaleDisplayNames* ldn) {
  typedef decltype(&uldn_getDialectHandling) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_getDialectHandling"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_getDialectHandling");
    abort();
  }

  return ptr(ldn);
}
int32_t uldn_localeDisplayName(const ULocaleDisplayNames* ldn, const char* locale, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_localeDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_localeDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_localeDisplayName");
    abort();
  }

  return ptr(ldn, locale, result, maxResultSize, pErrorCode);
}
int32_t uldn_languageDisplayName(const ULocaleDisplayNames* ldn, const char* lang, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_languageDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_languageDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_languageDisplayName");
    abort();
  }

  return ptr(ldn, lang, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptDisplayName(const ULocaleDisplayNames* ldn, const char* script, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_scriptDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_scriptDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_scriptDisplayName");
    abort();
  }

  return ptr(ldn, script, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptCodeDisplayName(const ULocaleDisplayNames* ldn, UScriptCode scriptCode, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_scriptCodeDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_scriptCodeDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_scriptCodeDisplayName");
    abort();
  }

  return ptr(ldn, scriptCode, result, maxResultSize, pErrorCode);
}
int32_t uldn_regionDisplayName(const ULocaleDisplayNames* ldn, const char* region, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_regionDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_regionDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_regionDisplayName");
    abort();
  }

  return ptr(ldn, region, result, maxResultSize, pErrorCode);
}
int32_t uldn_variantDisplayName(const ULocaleDisplayNames* ldn, const char* variant, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_variantDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_variantDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_variantDisplayName");
    abort();
  }

  return ptr(ldn, variant, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyDisplayName(const ULocaleDisplayNames* ldn, const char* key, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_keyDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_keyDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_keyDisplayName");
    abort();
  }

  return ptr(ldn, key, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyValueDisplayName(const ULocaleDisplayNames* ldn, const char* key, const char* value, UChar* result, int32_t maxResultSize, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_keyValueDisplayName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_keyValueDisplayName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_keyValueDisplayName");
    abort();
  }

  return ptr(ldn, key, value, result, maxResultSize, pErrorCode);
}
ULocaleDisplayNames* uldn_openForContext(const char* locale, UDisplayContext* contexts, int32_t length, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_openForContext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_openForContext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_openForContext");
    abort();
  }

  return ptr(locale, contexts, length, pErrorCode);
}
UDisplayContext uldn_getContext(const ULocaleDisplayNames* ldn, UDisplayContextType type, UErrorCode* pErrorCode) {
  typedef decltype(&uldn_getContext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uldn_getContext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uldn_getContext");
    abort();
  }

  return ptr(ldn, type, pErrorCode);
}
void u_init(UErrorCode* status) {
  typedef decltype(&u_init) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_init"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_init");
    abort();
  }

  ptr(status);
}
void u_cleanup(void) {
  typedef decltype(&u_cleanup) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_cleanup"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_cleanup");
    abort();
  }

  ptr();
}
void u_setMemoryFunctions(const void* context, UMemAllocFn* U_CALLCONV_FPTR a, UMemReallocFn* U_CALLCONV_FPTR r, UMemFreeFn* U_CALLCONV_FPTR f, UErrorCode* status) {
  typedef decltype(&u_setMemoryFunctions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_setMemoryFunctions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_setMemoryFunctions");
    abort();
  }

  ptr(context, a, r, f, status);
}
const char* u_errorName(UErrorCode code) {
  typedef decltype(&u_errorName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_errorName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_errorName");
    abort();
  }

  return ptr(code);
}
int32_t ucurr_forLocale(const char* locale, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  typedef decltype(&ucurr_forLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_forLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_forLocale");
    abort();
  }

  return ptr(locale, buff, buffCapacity, ec);
}
UCurrRegistryKey ucurr_register(const UChar* isoCode, const char* locale, UErrorCode* status) {
  typedef decltype(&ucurr_register) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_register"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_register");
    abort();
  }

  return ptr(isoCode, locale, status);
}
UBool ucurr_unregister(UCurrRegistryKey key, UErrorCode* status) {
  typedef decltype(&ucurr_unregister) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_unregister"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_unregister");
    abort();
  }

  return ptr(key, status);
}
const UChar* ucurr_getName(const UChar* currency, const char* locale, UCurrNameStyle nameStyle, UBool* isChoiceFormat, int32_t* len, UErrorCode* ec) {
  typedef decltype(&ucurr_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getName");
    abort();
  }

  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}
const UChar* ucurr_getPluralName(const UChar* currency, const char* locale, UBool* isChoiceFormat, const char* pluralCount, int32_t* len, UErrorCode* ec) {
  typedef decltype(&ucurr_getPluralName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getPluralName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getPluralName");
    abort();
  }

  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}
int32_t ucurr_getDefaultFractionDigits(const UChar* currency, UErrorCode* ec) {
  typedef decltype(&ucurr_getDefaultFractionDigits) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getDefaultFractionDigits"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getDefaultFractionDigits");
    abort();
  }

  return ptr(currency, ec);
}
int32_t ucurr_getDefaultFractionDigitsForUsage(const UChar* currency, const UCurrencyUsage usage, UErrorCode* ec) {
  typedef decltype(&ucurr_getDefaultFractionDigitsForUsage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getDefaultFractionDigitsForUsage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getDefaultFractionDigitsForUsage");
    abort();
  }

  return ptr(currency, usage, ec);
}
double ucurr_getRoundingIncrement(const UChar* currency, UErrorCode* ec) {
  typedef decltype(&ucurr_getRoundingIncrement) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getRoundingIncrement"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getRoundingIncrement");
    abort();
  }

  return ptr(currency, ec);
}
double ucurr_getRoundingIncrementForUsage(const UChar* currency, const UCurrencyUsage usage, UErrorCode* ec) {
  typedef decltype(&ucurr_getRoundingIncrementForUsage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getRoundingIncrementForUsage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getRoundingIncrementForUsage");
    abort();
  }

  return ptr(currency, usage, ec);
}
UEnumeration* ucurr_openISOCurrencies(uint32_t currType, UErrorCode* pErrorCode) {
  typedef decltype(&ucurr_openISOCurrencies) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_openISOCurrencies"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_openISOCurrencies");
    abort();
  }

  return ptr(currType, pErrorCode);
}
UBool ucurr_isAvailable(const UChar* isoCode, UDate from, UDate to, UErrorCode* errorCode) {
  typedef decltype(&ucurr_isAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_isAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_isAvailable");
    abort();
  }

  return ptr(isoCode, from, to, errorCode);
}
int32_t ucurr_countCurrencies(const char* locale, UDate date, UErrorCode* ec) {
  typedef decltype(&ucurr_countCurrencies) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_countCurrencies"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_countCurrencies");
    abort();
  }

  return ptr(locale, date, ec);
}
int32_t ucurr_forLocaleAndDate(const char* locale, UDate date, int32_t index, UChar* buff, int32_t buffCapacity, UErrorCode* ec) {
  typedef decltype(&ucurr_forLocaleAndDate) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_forLocaleAndDate"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_forLocaleAndDate");
    abort();
  }

  return ptr(locale, date, index, buff, buffCapacity, ec);
}
UEnumeration* ucurr_getKeywordValuesForLocale(const char* key, const char* locale, UBool commonlyUsed, UErrorCode* status) {
  typedef decltype(&ucurr_getKeywordValuesForLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getKeywordValuesForLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getKeywordValuesForLocale");
    abort();
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucurr_getNumericCode(const UChar* currency) {
  typedef decltype(&ucurr_getNumericCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucurr_getNumericCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucurr_getNumericCode");
    abort();
  }

  return ptr(currency);
}
USet* uset_openEmpty(void) {
  typedef decltype(&uset_openEmpty) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_openEmpty"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_openEmpty");
    abort();
  }

  return ptr();
}
USet* uset_open(UChar32 start, UChar32 end) {
  typedef decltype(&uset_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_open");
    abort();
  }

  return ptr(start, end);
}
USet* uset_openPattern(const UChar* pattern, int32_t patternLength, UErrorCode* ec) {
  typedef decltype(&uset_openPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_openPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_openPattern");
    abort();
  }

  return ptr(pattern, patternLength, ec);
}
USet* uset_openPatternOptions(const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* ec) {
  typedef decltype(&uset_openPatternOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_openPatternOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_openPatternOptions");
    abort();
  }

  return ptr(pattern, patternLength, options, ec);
}
void uset_close(USet* set) {
  typedef decltype(&uset_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_close");
    abort();
  }

  ptr(set);
}
USet* uset_clone(const USet* set) {
  typedef decltype(&uset_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_clone");
    abort();
  }

  return ptr(set);
}
UBool uset_isFrozen(const USet* set) {
  typedef decltype(&uset_isFrozen) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_isFrozen"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_isFrozen");
    abort();
  }

  return ptr(set);
}
void uset_freeze(USet* set) {
  typedef decltype(&uset_freeze) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_freeze"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_freeze");
    abort();
  }

  ptr(set);
}
USet* uset_cloneAsThawed(const USet* set) {
  typedef decltype(&uset_cloneAsThawed) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_cloneAsThawed"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_cloneAsThawed");
    abort();
  }

  return ptr(set);
}
void uset_set(USet* set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_set) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_set"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_set");
    abort();
  }

  ptr(set, start, end);
}
int32_t uset_applyPattern(USet* set, const UChar* pattern, int32_t patternLength, uint32_t options, UErrorCode* status) {
  typedef decltype(&uset_applyPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_applyPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_applyPattern");
    abort();
  }

  return ptr(set, pattern, patternLength, options, status);
}
void uset_applyIntPropertyValue(USet* set, UProperty prop, int32_t value, UErrorCode* ec) {
  typedef decltype(&uset_applyIntPropertyValue) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_applyIntPropertyValue"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_applyIntPropertyValue");
    abort();
  }

  ptr(set, prop, value, ec);
}
void uset_applyPropertyAlias(USet* set, const UChar* prop, int32_t propLength, const UChar* value, int32_t valueLength, UErrorCode* ec) {
  typedef decltype(&uset_applyPropertyAlias) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_applyPropertyAlias"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_applyPropertyAlias");
    abort();
  }

  ptr(set, prop, propLength, value, valueLength, ec);
}
UBool uset_resemblesPattern(const UChar* pattern, int32_t patternLength, int32_t pos) {
  typedef decltype(&uset_resemblesPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_resemblesPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_resemblesPattern");
    abort();
  }

  return ptr(pattern, patternLength, pos);
}
int32_t uset_toPattern(const USet* set, UChar* result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode* ec) {
  typedef decltype(&uset_toPattern) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_toPattern"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_toPattern");
    abort();
  }

  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}
void uset_add(USet* set, UChar32 c) {
  typedef decltype(&uset_add) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_add"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_add");
    abort();
  }

  ptr(set, c);
}
void uset_addAll(USet* set, const USet* additionalSet) {
  typedef decltype(&uset_addAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_addAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_addAll");
    abort();
  }

  ptr(set, additionalSet);
}
void uset_addRange(USet* set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_addRange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_addRange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_addRange");
    abort();
  }

  ptr(set, start, end);
}
void uset_addString(USet* set, const UChar* str, int32_t strLen) {
  typedef decltype(&uset_addString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_addString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_addString");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_addAllCodePoints(USet* set, const UChar* str, int32_t strLen) {
  typedef decltype(&uset_addAllCodePoints) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_addAllCodePoints"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_addAllCodePoints");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_remove(USet* set, UChar32 c) {
  typedef decltype(&uset_remove) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_remove"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_remove");
    abort();
  }

  ptr(set, c);
}
void uset_removeRange(USet* set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_removeRange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_removeRange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_removeRange");
    abort();
  }

  ptr(set, start, end);
}
void uset_removeString(USet* set, const UChar* str, int32_t strLen) {
  typedef decltype(&uset_removeString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_removeString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_removeString");
    abort();
  }

  ptr(set, str, strLen);
}
void uset_removeAll(USet* set, const USet* removeSet) {
  typedef decltype(&uset_removeAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_removeAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_removeAll");
    abort();
  }

  ptr(set, removeSet);
}
void uset_retain(USet* set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_retain) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_retain"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_retain");
    abort();
  }

  ptr(set, start, end);
}
void uset_retainAll(USet* set, const USet* retain) {
  typedef decltype(&uset_retainAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_retainAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_retainAll");
    abort();
  }

  ptr(set, retain);
}
void uset_compact(USet* set) {
  typedef decltype(&uset_compact) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_compact"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_compact");
    abort();
  }

  ptr(set);
}
void uset_complement(USet* set) {
  typedef decltype(&uset_complement) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_complement"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_complement");
    abort();
  }

  ptr(set);
}
void uset_complementAll(USet* set, const USet* complement) {
  typedef decltype(&uset_complementAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_complementAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_complementAll");
    abort();
  }

  ptr(set, complement);
}
void uset_clear(USet* set) {
  typedef decltype(&uset_clear) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_clear"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_clear");
    abort();
  }

  ptr(set);
}
void uset_closeOver(USet* set, int32_t attributes) {
  typedef decltype(&uset_closeOver) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_closeOver"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_closeOver");
    abort();
  }

  ptr(set, attributes);
}
void uset_removeAllStrings(USet* set) {
  typedef decltype(&uset_removeAllStrings) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_removeAllStrings"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_removeAllStrings");
    abort();
  }

  ptr(set);
}
UBool uset_isEmpty(const USet* set) {
  typedef decltype(&uset_isEmpty) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_isEmpty"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_isEmpty");
    abort();
  }

  return ptr(set);
}
UBool uset_contains(const USet* set, UChar32 c) {
  typedef decltype(&uset_contains) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_contains"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_contains");
    abort();
  }

  return ptr(set, c);
}
UBool uset_containsRange(const USet* set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_containsRange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsRange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsRange");
    abort();
  }

  return ptr(set, start, end);
}
UBool uset_containsString(const USet* set, const UChar* str, int32_t strLen) {
  typedef decltype(&uset_containsString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsString");
    abort();
  }

  return ptr(set, str, strLen);
}
int32_t uset_indexOf(const USet* set, UChar32 c) {
  typedef decltype(&uset_indexOf) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_indexOf"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_indexOf");
    abort();
  }

  return ptr(set, c);
}
UChar32 uset_charAt(const USet* set, int32_t charIndex) {
  typedef decltype(&uset_charAt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_charAt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_charAt");
    abort();
  }

  return ptr(set, charIndex);
}
int32_t uset_size(const USet* set) {
  typedef decltype(&uset_size) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_size"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_size");
    abort();
  }

  return ptr(set);
}
int32_t uset_getItemCount(const USet* set) {
  typedef decltype(&uset_getItemCount) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_getItemCount"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_getItemCount");
    abort();
  }

  return ptr(set);
}
int32_t uset_getItem(const USet* set, int32_t itemIndex, UChar32* start, UChar32* end, UChar* str, int32_t strCapacity, UErrorCode* ec) {
  typedef decltype(&uset_getItem) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_getItem"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_getItem");
    abort();
  }

  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}
UBool uset_containsAll(const USet* set1, const USet* set2) {
  typedef decltype(&uset_containsAll) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsAll"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsAll");
    abort();
  }

  return ptr(set1, set2);
}
UBool uset_containsAllCodePoints(const USet* set, const UChar* str, int32_t strLen) {
  typedef decltype(&uset_containsAllCodePoints) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsAllCodePoints"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsAllCodePoints");
    abort();
  }

  return ptr(set, str, strLen);
}
UBool uset_containsNone(const USet* set1, const USet* set2) {
  typedef decltype(&uset_containsNone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsNone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsNone");
    abort();
  }

  return ptr(set1, set2);
}
UBool uset_containsSome(const USet* set1, const USet* set2) {
  typedef decltype(&uset_containsSome) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_containsSome"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_containsSome");
    abort();
  }

  return ptr(set1, set2);
}
int32_t uset_span(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_span) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_span"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_span");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBack(const USet* set, const UChar* s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanBack) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_spanBack"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_spanBack");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_spanUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_spanUTF8");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBackUTF8(const USet* set, const char* s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanBackUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_spanBackUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_spanBackUTF8");
    abort();
  }

  return ptr(set, s, length, spanCondition);
}
UBool uset_equals(const USet* set1, const USet* set2) {
  typedef decltype(&uset_equals) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_equals"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_equals");
    abort();
  }

  return ptr(set1, set2);
}
int32_t uset_serialize(const USet* set, uint16_t* dest, int32_t destCapacity, UErrorCode* pErrorCode) {
  typedef decltype(&uset_serialize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_serialize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_serialize");
    abort();
  }

  return ptr(set, dest, destCapacity, pErrorCode);
}
UBool uset_getSerializedSet(USerializedSet* fillSet, const uint16_t* src, int32_t srcLength) {
  typedef decltype(&uset_getSerializedSet) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_getSerializedSet"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_getSerializedSet");
    abort();
  }

  return ptr(fillSet, src, srcLength);
}
void uset_setSerializedToOne(USerializedSet* fillSet, UChar32 c) {
  typedef decltype(&uset_setSerializedToOne) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_setSerializedToOne"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_setSerializedToOne");
    abort();
  }

  ptr(fillSet, c);
}
UBool uset_serializedContains(const USerializedSet* set, UChar32 c) {
  typedef decltype(&uset_serializedContains) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_serializedContains"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_serializedContains");
    abort();
  }

  return ptr(set, c);
}
int32_t uset_getSerializedRangeCount(const USerializedSet* set) {
  typedef decltype(&uset_getSerializedRangeCount) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_getSerializedRangeCount"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_getSerializedRangeCount");
    abort();
  }

  return ptr(set);
}
UBool uset_getSerializedRange(const USerializedSet* set, int32_t rangeIndex, UChar32* pStart, UChar32* pEnd) {
  typedef decltype(&uset_getSerializedRange) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uset_getSerializedRange"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uset_getSerializedRange");
    abort();
  }

  return ptr(set, rangeIndex, pStart, pEnd);
}
int32_t u_shapeArabic(const UChar* source, int32_t sourceLength, UChar* dest, int32_t destSize, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&u_shapeArabic) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_shapeArabic"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_shapeArabic");
    abort();
  }

  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}
UBreakIterator* ubrk_open(UBreakIteratorType type, const char* locale, const UChar* text, int32_t textLength, UErrorCode* status) {
  typedef decltype(&ubrk_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_open");
    abort();
  }

  return ptr(type, locale, text, textLength, status);
}
UBreakIterator* ubrk_openRules(const UChar* rules, int32_t rulesLength, const UChar* text, int32_t textLength, UParseError* parseErr, UErrorCode* status) {
  typedef decltype(&ubrk_openRules) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_openRules"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_openRules");
    abort();
  }

  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}
UBreakIterator* ubrk_safeClone(const UBreakIterator* bi, void* stackBuffer, int32_t* pBufferSize, UErrorCode* status) {
  typedef decltype(&ubrk_safeClone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_safeClone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_safeClone");
    abort();
  }

  return ptr(bi, stackBuffer, pBufferSize, status);
}
void ubrk_close(UBreakIterator* bi) {
  typedef decltype(&ubrk_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_close");
    abort();
  }

  ptr(bi);
}
void ubrk_setText(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status) {
  typedef decltype(&ubrk_setText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_setText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_setText");
    abort();
  }

  ptr(bi, text, textLength, status);
}
void ubrk_setUText(UBreakIterator* bi, UText* text, UErrorCode* status) {
  typedef decltype(&ubrk_setUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_setUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_setUText");
    abort();
  }

  ptr(bi, text, status);
}
int32_t ubrk_current(const UBreakIterator* bi) {
  typedef decltype(&ubrk_current) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_current"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_current");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_next(UBreakIterator* bi) {
  typedef decltype(&ubrk_next) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_next"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_next");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_previous(UBreakIterator* bi) {
  typedef decltype(&ubrk_previous) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_previous"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_previous");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_first(UBreakIterator* bi) {
  typedef decltype(&ubrk_first) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_first"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_first");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_last(UBreakIterator* bi) {
  typedef decltype(&ubrk_last) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_last"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_last");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_preceding(UBreakIterator* bi, int32_t offset) {
  typedef decltype(&ubrk_preceding) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_preceding"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_preceding");
    abort();
  }

  return ptr(bi, offset);
}
int32_t ubrk_following(UBreakIterator* bi, int32_t offset) {
  typedef decltype(&ubrk_following) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_following"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_following");
    abort();
  }

  return ptr(bi, offset);
}
const char* ubrk_getAvailable(int32_t index) {
  typedef decltype(&ubrk_getAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_getAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_getAvailable");
    abort();
  }

  return ptr(index);
}
int32_t ubrk_countAvailable(void) {
  typedef decltype(&ubrk_countAvailable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_countAvailable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_countAvailable");
    abort();
  }

  return ptr();
}
UBool ubrk_isBoundary(UBreakIterator* bi, int32_t offset) {
  typedef decltype(&ubrk_isBoundary) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_isBoundary"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_isBoundary");
    abort();
  }

  return ptr(bi, offset);
}
int32_t ubrk_getRuleStatus(UBreakIterator* bi) {
  typedef decltype(&ubrk_getRuleStatus) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_getRuleStatus"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_getRuleStatus");
    abort();
  }

  return ptr(bi);
}
int32_t ubrk_getRuleStatusVec(UBreakIterator* bi, int32_t* fillInVec, int32_t capacity, UErrorCode* status) {
  typedef decltype(&ubrk_getRuleStatusVec) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_getRuleStatusVec"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_getRuleStatusVec");
    abort();
  }

  return ptr(bi, fillInVec, capacity, status);
}
const char* ubrk_getLocaleByType(const UBreakIterator* bi, ULocDataLocaleType type, UErrorCode* status) {
  typedef decltype(&ubrk_getLocaleByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_getLocaleByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_getLocaleByType");
    abort();
  }

  return ptr(bi, type, status);
}
void ubrk_refreshUText(UBreakIterator* bi, UText* text, UErrorCode* status) {
  typedef decltype(&ubrk_refreshUText) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubrk_refreshUText"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubrk_refreshUText");
    abort();
  }

  ptr(bi, text, status);
}
void utrace_setLevel(int32_t traceLevel) {
  typedef decltype(&utrace_setLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_setLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_setLevel");
    abort();
  }

  ptr(traceLevel);
}
int32_t utrace_getLevel(void) {
  typedef decltype(&utrace_getLevel) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_getLevel"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_getLevel");
    abort();
  }

  return ptr();
}
void utrace_setFunctions(const void* context, UTraceEntry* e, UTraceExit* x, UTraceData* d) {
  typedef decltype(&utrace_setFunctions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_setFunctions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_setFunctions");
    abort();
  }

  ptr(context, e, x, d);
}
void utrace_getFunctions(const void** context, UTraceEntry** e, UTraceExit** x, UTraceData** d) {
  typedef decltype(&utrace_getFunctions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_getFunctions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_getFunctions");
    abort();
  }

  ptr(context, e, x, d);
}
int32_t utrace_vformat(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, va_list args) {
  typedef decltype(&utrace_vformat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_vformat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_vformat");
    abort();
  }

  return ptr(outBuf, capacity, indent, fmt, args);
}
int32_t utrace_format(char* outBuf, int32_t capacity, int32_t indent, const char* fmt, ...) {
  typedef decltype(&utrace_vformat) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_vformat"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_format");
    abort();
  }

  va_list args;
  va_start(args, fmt);
  int32_t ret = ptr(outBuf, capacity, indent, fmt, args);
  va_end(args);
  return ret;
}
const char* utrace_functionName(int32_t fnNumber) {
  typedef decltype(&utrace_functionName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utrace_functionName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utrace_functionName");
    abort();
  }

  return ptr(fnNumber);
}
UText* utext_close(UText* ut) {
  typedef decltype(&utext_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_close");
    abort();
  }

  return ptr(ut);
}
UText* utext_openUTF8(UText* ut, const char* s, int64_t length, UErrorCode* status) {
  typedef decltype(&utext_openUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_openUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_openUTF8");
    abort();
  }

  return ptr(ut, s, length, status);
}
UText* utext_openUChars(UText* ut, const UChar* s, int64_t length, UErrorCode* status) {
  typedef decltype(&utext_openUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_openUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_openUChars");
    abort();
  }

  return ptr(ut, s, length, status);
}
UText* utext_clone(UText* dest, const UText* src, UBool deep, UBool readOnly, UErrorCode* status) {
  typedef decltype(&utext_clone) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_clone"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_clone");
    abort();
  }

  return ptr(dest, src, deep, readOnly, status);
}
UBool utext_equals(const UText* a, const UText* b) {
  typedef decltype(&utext_equals) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_equals"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_equals");
    abort();
  }

  return ptr(a, b);
}
int64_t utext_nativeLength(UText* ut) {
  typedef decltype(&utext_nativeLength) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_nativeLength"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_nativeLength");
    abort();
  }

  return ptr(ut);
}
UBool utext_isLengthExpensive(const UText* ut) {
  typedef decltype(&utext_isLengthExpensive) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_isLengthExpensive"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_isLengthExpensive");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_char32At(UText* ut, int64_t nativeIndex) {
  typedef decltype(&utext_char32At) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_char32At"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_char32At");
    abort();
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_current32(UText* ut) {
  typedef decltype(&utext_current32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_current32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_current32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_next32(UText* ut) {
  typedef decltype(&utext_next32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_next32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_next32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_previous32(UText* ut) {
  typedef decltype(&utext_previous32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_previous32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_previous32");
    abort();
  }

  return ptr(ut);
}
UChar32 utext_next32From(UText* ut, int64_t nativeIndex) {
  typedef decltype(&utext_next32From) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_next32From"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_next32From");
    abort();
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_previous32From(UText* ut, int64_t nativeIndex) {
  typedef decltype(&utext_previous32From) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_previous32From"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_previous32From");
    abort();
  }

  return ptr(ut, nativeIndex);
}
int64_t utext_getNativeIndex(const UText* ut) {
  typedef decltype(&utext_getNativeIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_getNativeIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_getNativeIndex");
    abort();
  }

  return ptr(ut);
}
void utext_setNativeIndex(UText* ut, int64_t nativeIndex) {
  typedef decltype(&utext_setNativeIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_setNativeIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_setNativeIndex");
    abort();
  }

  ptr(ut, nativeIndex);
}
UBool utext_moveIndex32(UText* ut, int32_t delta) {
  typedef decltype(&utext_moveIndex32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_moveIndex32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_moveIndex32");
    abort();
  }

  return ptr(ut, delta);
}
int64_t utext_getPreviousNativeIndex(UText* ut) {
  typedef decltype(&utext_getPreviousNativeIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_getPreviousNativeIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_getPreviousNativeIndex");
    abort();
  }

  return ptr(ut);
}
int32_t utext_extract(UText* ut, int64_t nativeStart, int64_t nativeLimit, UChar* dest, int32_t destCapacity, UErrorCode* status) {
  typedef decltype(&utext_extract) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_extract"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_extract");
    abort();
  }

  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}
UBool utext_isWritable(const UText* ut) {
  typedef decltype(&utext_isWritable) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_isWritable"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_isWritable");
    abort();
  }

  return ptr(ut);
}
UBool utext_hasMetaData(const UText* ut) {
  typedef decltype(&utext_hasMetaData) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_hasMetaData"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_hasMetaData");
    abort();
  }

  return ptr(ut);
}
int32_t utext_replace(UText* ut, int64_t nativeStart, int64_t nativeLimit, const UChar* replacementText, int32_t replacementLength, UErrorCode* status) {
  typedef decltype(&utext_replace) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_replace"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_replace");
    abort();
  }

  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}
void utext_copy(UText* ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode* status) {
  typedef decltype(&utext_copy) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_copy"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_copy");
    abort();
  }

  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
}
void utext_freeze(UText* ut) {
  typedef decltype(&utext_freeze) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_freeze"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_freeze");
    abort();
  }

  ptr(ut);
}
UText* utext_setup(UText* ut, int32_t extraSpace, UErrorCode* status) {
  typedef decltype(&utext_setup) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("utext_setup"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "utext_setup");
    abort();
  }

  return ptr(ut, extraSpace, status);
}
void uenum_close(UEnumeration* en) {
  typedef decltype(&uenum_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_close");
    abort();
  }

  ptr(en);
}
int32_t uenum_count(UEnumeration* en, UErrorCode* status) {
  typedef decltype(&uenum_count) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_count"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_count");
    abort();
  }

  return ptr(en, status);
}
const UChar* uenum_unext(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  typedef decltype(&uenum_unext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_unext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_unext");
    abort();
  }

  return ptr(en, resultLength, status);
}
const char* uenum_next(UEnumeration* en, int32_t* resultLength, UErrorCode* status) {
  typedef decltype(&uenum_next) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_next"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_next");
    abort();
  }

  return ptr(en, resultLength, status);
}
void uenum_reset(UEnumeration* en, UErrorCode* status) {
  typedef decltype(&uenum_reset) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_reset"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_reset");
    abort();
  }

  ptr(en, status);
}
UEnumeration* uenum_openUCharStringsEnumeration(const UChar* const strings [], int32_t count, UErrorCode* ec) {
  typedef decltype(&uenum_openUCharStringsEnumeration) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_openUCharStringsEnumeration"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_openUCharStringsEnumeration");
    abort();
  }

  return ptr(strings, count, ec);
}
UEnumeration* uenum_openCharStringsEnumeration(const char* const strings [], int32_t count, UErrorCode* ec) {
  typedef decltype(&uenum_openCharStringsEnumeration) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uenum_openCharStringsEnumeration"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uenum_openCharStringsEnumeration");
    abort();
  }

  return ptr(strings, count, ec);
}
void u_versionFromString(UVersionInfo versionArray, const char* versionString) {
  typedef decltype(&u_versionFromString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_versionFromString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_versionFromString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_versionFromUString(UVersionInfo versionArray, const UChar* versionString) {
  typedef decltype(&u_versionFromUString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_versionFromUString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_versionFromUString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_versionToString(const UVersionInfo versionArray, char* versionString) {
  typedef decltype(&u_versionToString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_versionToString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_versionToString");
    abort();
  }

  ptr(versionArray, versionString);
}
void u_getVersion(UVersionInfo versionArray) {
  typedef decltype(&u_getVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getVersion");
    abort();
  }

  ptr(versionArray);
}
UStringPrepProfile* usprep_open(const char* path, const char* fileName, UErrorCode* status) {
  typedef decltype(&usprep_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usprep_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usprep_open");
    abort();
  }

  return ptr(path, fileName, status);
}
UStringPrepProfile* usprep_openByType(UStringPrepProfileType type, UErrorCode* status) {
  typedef decltype(&usprep_openByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usprep_openByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usprep_openByType");
    abort();
  }

  return ptr(type, status);
}
void usprep_close(UStringPrepProfile* profile) {
  typedef decltype(&usprep_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usprep_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usprep_close");
    abort();
  }

  ptr(profile);
}
int32_t usprep_prepare(const UStringPrepProfile* prep, const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status) {
  typedef decltype(&usprep_prepare) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("usprep_prepare"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "usprep_prepare");
    abort();
  }

  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}
int32_t uscript_getCode(const char* nameOrAbbrOrLocale, UScriptCode* fillIn, int32_t capacity, UErrorCode* err) {
  typedef decltype(&uscript_getCode) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getCode"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getCode");
    abort();
  }

  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}
const char* uscript_getName(UScriptCode scriptCode) {
  typedef decltype(&uscript_getName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getName");
    abort();
  }

  return ptr(scriptCode);
}
const char* uscript_getShortName(UScriptCode scriptCode) {
  typedef decltype(&uscript_getShortName) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getShortName"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getShortName");
    abort();
  }

  return ptr(scriptCode);
}
UScriptCode uscript_getScript(UChar32 codepoint, UErrorCode* err) {
  typedef decltype(&uscript_getScript) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getScript"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getScript");
    abort();
  }

  return ptr(codepoint, err);
}
UBool uscript_hasScript(UChar32 c, UScriptCode sc) {
  typedef decltype(&uscript_hasScript) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_hasScript"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_hasScript");
    abort();
  }

  return ptr(c, sc);
}
int32_t uscript_getScriptExtensions(UChar32 c, UScriptCode* scripts, int32_t capacity, UErrorCode* errorCode) {
  typedef decltype(&uscript_getScriptExtensions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getScriptExtensions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getScriptExtensions");
    abort();
  }

  return ptr(c, scripts, capacity, errorCode);
}
int32_t uscript_getSampleString(UScriptCode script, UChar* dest, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&uscript_getSampleString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getSampleString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getSampleString");
    abort();
  }

  return ptr(script, dest, capacity, pErrorCode);
}
UScriptUsage uscript_getUsage(UScriptCode script) {
  typedef decltype(&uscript_getUsage) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_getUsage"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_getUsage");
    abort();
  }

  return ptr(script);
}
UBool uscript_isRightToLeft(UScriptCode script) {
  typedef decltype(&uscript_isRightToLeft) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_isRightToLeft"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_isRightToLeft");
    abort();
  }

  return ptr(script);
}
UBool uscript_breaksBetweenLetters(UScriptCode script) {
  typedef decltype(&uscript_breaksBetweenLetters) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_breaksBetweenLetters"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_breaksBetweenLetters");
    abort();
  }

  return ptr(script);
}
UBool uscript_isCased(UScriptCode script) {
  typedef decltype(&uscript_isCased) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uscript_isCased"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uscript_isCased");
    abort();
  }

  return ptr(script);
}
const char* u_getDataDirectory(void) {
  typedef decltype(&u_getDataDirectory) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_getDataDirectory"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_getDataDirectory");
    abort();
  }

  return ptr();
}
void u_setDataDirectory(const char* directory) {
  typedef decltype(&u_setDataDirectory) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_setDataDirectory"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_setDataDirectory");
    abort();
  }

  ptr(directory);
}
void u_charsToUChars(const char* cs, UChar* us, int32_t length) {
  typedef decltype(&u_charsToUChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_charsToUChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_charsToUChars");
    abort();
  }

  ptr(cs, us, length);
}
void u_UCharsToChars(const UChar* us, char* cs, int32_t length) {
  typedef decltype(&u_UCharsToChars) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("u_UCharsToChars"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "u_UCharsToChars");
    abort();
  }

  ptr(us, cs, length);
}
UCaseMap* ucasemap_open(const char* locale, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_open");
    abort();
  }

  return ptr(locale, options, pErrorCode);
}
void ucasemap_close(UCaseMap* csm) {
  typedef decltype(&ucasemap_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_close");
    abort();
  }

  ptr(csm);
}
const char* ucasemap_getLocale(const UCaseMap* csm) {
  typedef decltype(&ucasemap_getLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_getLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_getLocale");
    abort();
  }

  return ptr(csm);
}
uint32_t ucasemap_getOptions(const UCaseMap* csm) {
  typedef decltype(&ucasemap_getOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_getOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_getOptions");
    abort();
  }

  return ptr(csm);
}
void ucasemap_setLocale(UCaseMap* csm, const char* locale, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_setLocale) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_setLocale"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_setLocale");
    abort();
  }

  ptr(csm, locale, pErrorCode);
}
void ucasemap_setOptions(UCaseMap* csm, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_setOptions) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_setOptions"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_setOptions");
    abort();
  }

  ptr(csm, options, pErrorCode);
}
const UBreakIterator* ucasemap_getBreakIterator(const UCaseMap* csm) {
  typedef decltype(&ucasemap_getBreakIterator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_getBreakIterator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_getBreakIterator");
    abort();
  }

  return ptr(csm);
}
void ucasemap_setBreakIterator(UCaseMap* csm, UBreakIterator* iterToAdopt, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_setBreakIterator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_setBreakIterator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_setBreakIterator");
    abort();
  }

  ptr(csm, iterToAdopt, pErrorCode);
}
int32_t ucasemap_toTitle(UCaseMap* csm, UChar* dest, int32_t destCapacity, const UChar* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_toTitle) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_toTitle"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_toTitle");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToLower(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_utf8ToLower) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_utf8ToLower"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_utf8ToLower");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToUpper(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_utf8ToUpper) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_utf8ToUpper"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_utf8ToUpper");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToTitle(UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_utf8ToTitle) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_utf8ToTitle"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_utf8ToTitle");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8FoldCase(const UCaseMap* csm, char* dest, int32_t destCapacity, const char* src, int32_t srcLength, UErrorCode* pErrorCode) {
  typedef decltype(&ucasemap_utf8FoldCase) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucasemap_utf8FoldCase"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucasemap_utf8FoldCase");
    abort();
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
const UNormalizer2* unorm2_getNFCInstance(UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getNFCInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getNFCInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getNFCInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2* unorm2_getNFDInstance(UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getNFDInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getNFDInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getNFDInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2* unorm2_getNFKCInstance(UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getNFKCInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getNFKCInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getNFKCInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2* unorm2_getNFKDInstance(UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getNFKDInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getNFKDInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getNFKDInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2* unorm2_getNFKCCasefoldInstance(UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getNFKCCasefoldInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getNFKCCasefoldInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getNFKCCasefoldInstance");
    abort();
  }

  return ptr(pErrorCode);
}
const UNormalizer2* unorm2_getInstance(const char* packageName, const char* name, UNormalization2Mode mode, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getInstance) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getInstance"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getInstance");
    abort();
  }

  return ptr(packageName, name, mode, pErrorCode);
}
UNormalizer2* unorm2_openFiltered(const UNormalizer2* norm2, const USet* filterSet, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_openFiltered) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_openFiltered"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_openFiltered");
    abort();
  }

  return ptr(norm2, filterSet, pErrorCode);
}
void unorm2_close(UNormalizer2* norm2) {
  typedef decltype(&unorm2_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_close");
    abort();
  }

  ptr(norm2);
}
int32_t unorm2_normalize(const UNormalizer2* norm2, const UChar* src, int32_t length, UChar* dest, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_normalize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_normalize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_normalize");
    abort();
  }

  return ptr(norm2, src, length, dest, capacity, pErrorCode);
}
int32_t unorm2_normalizeSecondAndAppend(const UNormalizer2* norm2, UChar* first, int32_t firstLength, int32_t firstCapacity, const UChar* second, int32_t secondLength, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_normalizeSecondAndAppend) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_normalizeSecondAndAppend"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_normalizeSecondAndAppend");
    abort();
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_append(const UNormalizer2* norm2, UChar* first, int32_t firstLength, int32_t firstCapacity, const UChar* second, int32_t secondLength, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_append) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_append"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_append");
    abort();
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_getDecomposition(const UNormalizer2* norm2, UChar32 c, UChar* decomposition, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getDecomposition) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getDecomposition"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getDecomposition");
    abort();
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
int32_t unorm2_getRawDecomposition(const UNormalizer2* norm2, UChar32 c, UChar* decomposition, int32_t capacity, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_getRawDecomposition) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getRawDecomposition"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getRawDecomposition");
    abort();
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
UChar32 unorm2_composePair(const UNormalizer2* norm2, UChar32 a, UChar32 b) {
  typedef decltype(&unorm2_composePair) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_composePair"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_composePair");
    abort();
  }

  return ptr(norm2, a, b);
}
uint8_t unorm2_getCombiningClass(const UNormalizer2* norm2, UChar32 c) {
  typedef decltype(&unorm2_getCombiningClass) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_getCombiningClass"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_getCombiningClass");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_isNormalized(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_isNormalized) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_isNormalized"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_isNormalized");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
UNormalizationCheckResult unorm2_quickCheck(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_quickCheck) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_quickCheck"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_quickCheck");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
int32_t unorm2_spanQuickCheckYes(const UNormalizer2* norm2, const UChar* s, int32_t length, UErrorCode* pErrorCode) {
  typedef decltype(&unorm2_spanQuickCheckYes) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_spanQuickCheckYes"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_spanQuickCheckYes");
    abort();
  }

  return ptr(norm2, s, length, pErrorCode);
}
UBool unorm2_hasBoundaryBefore(const UNormalizer2* norm2, UChar32 c) {
  typedef decltype(&unorm2_hasBoundaryBefore) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_hasBoundaryBefore"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_hasBoundaryBefore");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_hasBoundaryAfter(const UNormalizer2* norm2, UChar32 c) {
  typedef decltype(&unorm2_hasBoundaryAfter) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_hasBoundaryAfter"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_hasBoundaryAfter");
    abort();
  }

  return ptr(norm2, c);
}
UBool unorm2_isInert(const UNormalizer2* norm2, UChar32 c) {
  typedef decltype(&unorm2_isInert) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm2_isInert"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm2_isInert");
    abort();
  }

  return ptr(norm2, c);
}
int32_t unorm_compare(const UChar* s1, int32_t length1, const UChar* s2, int32_t length2, uint32_t options, UErrorCode* pErrorCode) {
  typedef decltype(&unorm_compare) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("unorm_compare"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "unorm_compare");
    abort();
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
UChar32 uiter_current32(UCharIterator* iter) {
  typedef decltype(&uiter_current32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_current32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_current32");
    abort();
  }

  return ptr(iter);
}
UChar32 uiter_next32(UCharIterator* iter) {
  typedef decltype(&uiter_next32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_next32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_next32");
    abort();
  }

  return ptr(iter);
}
UChar32 uiter_previous32(UCharIterator* iter) {
  typedef decltype(&uiter_previous32) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_previous32"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_previous32");
    abort();
  }

  return ptr(iter);
}
uint32_t uiter_getState(const UCharIterator* iter) {
  typedef decltype(&uiter_getState) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_getState"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_getState");
    abort();
  }

  return ptr(iter);
}
void uiter_setState(UCharIterator* iter, uint32_t state, UErrorCode* pErrorCode) {
  typedef decltype(&uiter_setState) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_setState"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_setState");
    abort();
  }

  ptr(iter, state, pErrorCode);
}
void uiter_setString(UCharIterator* iter, const UChar* s, int32_t length) {
  typedef decltype(&uiter_setString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_setString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_setString");
    abort();
  }

  ptr(iter, s, length);
}
void uiter_setUTF16BE(UCharIterator* iter, const char* s, int32_t length) {
  typedef decltype(&uiter_setUTF16BE) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_setUTF16BE"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_setUTF16BE");
    abort();
  }

  ptr(iter, s, length);
}
void uiter_setUTF8(UCharIterator* iter, const char* s, int32_t length) {
  typedef decltype(&uiter_setUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("uiter_setUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "uiter_setUTF8");
    abort();
  }

  ptr(iter, s, length);
}
UConverterSelector* ucnvsel_open(const char* const* converterList, int32_t converterListSize, const USet* excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode* status) {
  typedef decltype(&ucnvsel_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_open");
    abort();
  }

  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}
void ucnvsel_close(UConverterSelector* sel) {
  typedef decltype(&ucnvsel_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_close");
    abort();
  }

  ptr(sel);
}
UConverterSelector* ucnvsel_openFromSerialized(const void* buffer, int32_t length, UErrorCode* status) {
  typedef decltype(&ucnvsel_openFromSerialized) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_openFromSerialized"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_openFromSerialized");
    abort();
  }

  return ptr(buffer, length, status);
}
int32_t ucnvsel_serialize(const UConverterSelector* sel, void* buffer, int32_t bufferCapacity, UErrorCode* status) {
  typedef decltype(&ucnvsel_serialize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_serialize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_serialize");
    abort();
  }

  return ptr(sel, buffer, bufferCapacity, status);
}
UEnumeration* ucnvsel_selectForString(const UConverterSelector* sel, const UChar* s, int32_t length, UErrorCode* status) {
  typedef decltype(&ucnvsel_selectForString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_selectForString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_selectForString");
    abort();
  }

  return ptr(sel, s, length, status);
}
UEnumeration* ucnvsel_selectForUTF8(const UConverterSelector* sel, const char* s, int32_t length, UErrorCode* status) {
  typedef decltype(&ucnvsel_selectForUTF8) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ucnvsel_selectForUTF8"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ucnvsel_selectForUTF8");
    abort();
  }

  return ptr(sel, s, length, status);
}
uint32_t ubiditransform_transform(UBiDiTransform* pBiDiTransform, const UChar* src, int32_t srcLength, UChar* dest, int32_t destSize, UBiDiLevel inParaLevel, UBiDiOrder inOrder, UBiDiLevel outParaLevel, UBiDiOrder outOrder, UBiDiMirroring doMirroring, uint32_t shapingOptions, UErrorCode* pErrorCode) {
  typedef decltype(&ubiditransform_transform) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubiditransform_transform"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubiditransform_transform");
    abort();
  }

  return ptr(pBiDiTransform, src, srcLength, dest, destSize, inParaLevel, inOrder, outParaLevel, outOrder, doMirroring, shapingOptions, pErrorCode);
}
UBiDiTransform* ubiditransform_open(UErrorCode* pErrorCode) {
  typedef decltype(&ubiditransform_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubiditransform_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubiditransform_open");
    abort();
  }

  return ptr(pErrorCode);
}
void ubiditransform_close(UBiDiTransform* pBidiTransform) {
  typedef decltype(&ubiditransform_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ubiditransform_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ubiditransform_close");
    abort();
  }

  ptr(pBidiTransform);
}
UResourceBundle* ures_open(const char* packageName, const char* locale, UErrorCode* status) {
  typedef decltype(&ures_open) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_open"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_open");
    abort();
  }

  return ptr(packageName, locale, status);
}
UResourceBundle* ures_openDirect(const char* packageName, const char* locale, UErrorCode* status) {
  typedef decltype(&ures_openDirect) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_openDirect"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_openDirect");
    abort();
  }

  return ptr(packageName, locale, status);
}
UResourceBundle* ures_openU(const UChar* packageName, const char* locale, UErrorCode* status) {
  typedef decltype(&ures_openU) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_openU"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_openU");
    abort();
  }

  return ptr(packageName, locale, status);
}
void ures_close(UResourceBundle* resourceBundle) {
  typedef decltype(&ures_close) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_close"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_close");
    abort();
  }

  ptr(resourceBundle);
}
void ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
  typedef decltype(&ures_getVersion) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getVersion"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getVersion");
    abort();
  }

  ptr(resB, versionInfo);
}
const char* ures_getLocaleByType(const UResourceBundle* resourceBundle, ULocDataLocaleType type, UErrorCode* status) {
  typedef decltype(&ures_getLocaleByType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getLocaleByType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getLocaleByType");
    abort();
  }

  return ptr(resourceBundle, type, status);
}
const UChar* ures_getString(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  typedef decltype(&ures_getString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getString");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
const char* ures_getUTF8String(const UResourceBundle* resB, char* dest, int32_t* length, UBool forceCopy, UErrorCode* status) {
  typedef decltype(&ures_getUTF8String) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getUTF8String"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getUTF8String");
    abort();
  }

  return ptr(resB, dest, length, forceCopy, status);
}
const uint8_t* ures_getBinary(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  typedef decltype(&ures_getBinary) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getBinary"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getBinary");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
const int32_t* ures_getIntVector(const UResourceBundle* resourceBundle, int32_t* len, UErrorCode* status) {
  typedef decltype(&ures_getIntVector) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getIntVector"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getIntVector");
    abort();
  }

  return ptr(resourceBundle, len, status);
}
uint32_t ures_getUInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  typedef decltype(&ures_getUInt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getUInt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getUInt");
    abort();
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getInt(const UResourceBundle* resourceBundle, UErrorCode* status) {
  typedef decltype(&ures_getInt) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getInt"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getInt");
    abort();
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getSize(const UResourceBundle* resourceBundle) {
  typedef decltype(&ures_getSize) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getSize"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getSize");
    abort();
  }

  return ptr(resourceBundle);
}
UResType ures_getType(const UResourceBundle* resourceBundle) {
  typedef decltype(&ures_getType) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getType"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getType");
    abort();
  }

  return ptr(resourceBundle);
}
const char* ures_getKey(const UResourceBundle* resourceBundle) {
  typedef decltype(&ures_getKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getKey");
    abort();
  }

  return ptr(resourceBundle);
}
void ures_resetIterator(UResourceBundle* resourceBundle) {
  typedef decltype(&ures_resetIterator) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_resetIterator"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_resetIterator");
    abort();
  }

  ptr(resourceBundle);
}
UBool ures_hasNext(const UResourceBundle* resourceBundle) {
  typedef decltype(&ures_hasNext) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_hasNext"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_hasNext");
    abort();
  }

  return ptr(resourceBundle);
}
UResourceBundle* ures_getNextResource(UResourceBundle* resourceBundle, UResourceBundle* fillIn, UErrorCode* status) {
  typedef decltype(&ures_getNextResource) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getNextResource"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getNextResource");
    abort();
  }

  return ptr(resourceBundle, fillIn, status);
}
const UChar* ures_getNextString(UResourceBundle* resourceBundle, int32_t* len, const char** key, UErrorCode* status) {
  typedef decltype(&ures_getNextString) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getNextString"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getNextString");
    abort();
  }

  return ptr(resourceBundle, len, key, status);
}
UResourceBundle* ures_getByIndex(const UResourceBundle* resourceBundle, int32_t indexR, UResourceBundle* fillIn, UErrorCode* status) {
  typedef decltype(&ures_getByIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getByIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getByIndex");
    abort();
  }

  return ptr(resourceBundle, indexR, fillIn, status);
}
const UChar* ures_getStringByIndex(const UResourceBundle* resourceBundle, int32_t indexS, int32_t* len, UErrorCode* status) {
  typedef decltype(&ures_getStringByIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getStringByIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getStringByIndex");
    abort();
  }

  return ptr(resourceBundle, indexS, len, status);
}
const char* ures_getUTF8StringByIndex(const UResourceBundle* resB, int32_t stringIndex, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  typedef decltype(&ures_getUTF8StringByIndex) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getUTF8StringByIndex"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getUTF8StringByIndex");
    abort();
  }

  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}
UResourceBundle* ures_getByKey(const UResourceBundle* resourceBundle, const char* key, UResourceBundle* fillIn, UErrorCode* status) {
  typedef decltype(&ures_getByKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getByKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getByKey");
    abort();
  }

  return ptr(resourceBundle, key, fillIn, status);
}
const UChar* ures_getStringByKey(const UResourceBundle* resB, const char* key, int32_t* len, UErrorCode* status) {
  typedef decltype(&ures_getStringByKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getStringByKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getStringByKey");
    abort();
  }

  return ptr(resB, key, len, status);
}
const char* ures_getUTF8StringByKey(const UResourceBundle* resB, const char* key, char* dest, int32_t* pLength, UBool forceCopy, UErrorCode* status) {
  typedef decltype(&ures_getUTF8StringByKey) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_getUTF8StringByKey"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_getUTF8StringByKey");
    abort();
  }

  return ptr(resB, key, dest, pLength, forceCopy, status);
}
UEnumeration* ures_openAvailableLocales(const char* packageName, UErrorCode* status) {
  typedef decltype(&ures_openAvailableLocales) FuncPtr;
  FuncPtr ptr =
      reinterpret_cast<FuncPtr>(get_icu_wrapper_addr("ures_openAvailableLocales"));
  if (ptr == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, "NDKICU",
        "Attempted to call unavailable ICU function %s.", "ures_openAvailableLocales");
    abort();
  }

  return ptr(packageName, status);
}


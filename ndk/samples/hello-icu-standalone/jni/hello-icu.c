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
 *
 */
#include <string.h>
#include <stdio.h>
#include <jni.h>

#include <unicode/ucol.h>
#include <unicode/unum.h>
#include <unicode/ugender.h>

static void test_strcoll(void) {
  const char* s1 = "hrnec";
  const char* s2 = "chrt";

  printf("Testing icu4c collation in cs_CZ:\n");

  printf("With lexicographical comparison:\n");
  if(strcmp(s1, s2) < 0)
    printf("  %s before %s\n", s1, s2);
  else
    printf("  %s before %s\n", s2, s1);

  printf("With icu4c_strcoll comparison:\n");
  UErrorCode status = U_ZERO_ERROR;
  UCollator* coll = ucol_open("cs_CZ", &status);
  if (U_FAILURE(status)) {
    /* close the collator */
    ucol_close(coll);
    fprintf(stderr, "Failed to call ucol_open(): %s\n", u_errorName(status));
    return;
  }

  UChar us1[256];
  UChar us2[256];
  u_charsToUChars(s1, us1, strlen(s1) + 1);
  u_charsToUChars(s2, us2, strlen(s2) + 1);

  if (ucol_strcoll(coll, us1, strlen(s1), us2, strlen(s2)) == UCOL_LESS) {
    printf("  %s before %s\n", s1, s2);
  } else {
    printf("  %s before %s\n", s2, s1);
  }

  ucol_close(coll);
  printf("\n");
}

static void test_ugender(void) {
  UErrorCode status = U_ZERO_ERROR;
  const UGenderInfo* gi = ugender_getInstance("fr_CA", &status);

  // Will return U_UNSUPPORTED_ERROR on devices with ICU 44.
  if (U_FAILURE(status)) {
    fprintf(stderr, "Fail to create UGenderInfo: %s\n", u_errorName(status));
    return;
  }

  // Alternatively, one can test the ICU version to determine the action.
  UVersionInfo versionArray;
  u_getVersion(versionArray);
  char ver_str[512];
  u_versionToString(versionArray, ver_str);
  if (versionArray[0] >= 50) {
    printf("ICU version is greater than 50: %s\n", ver_str);

    UErrorCode status = U_ZERO_ERROR;
    const UGenderInfo* gi = ugender_getInstance("fr_CA", &status);
    if (U_FAILURE(status)) {
      fprintf(stderr, "Fail to create UGenderInfo: %s\n", u_errorName(status));
      return;
    }
    printf("Call to ugender_getInstance() succeeded\n");
  } else {
    fprintf(stderr, "ICU on device is too low (%s) to support ugender_getInstance()\n", ver_str);
  }
  printf("\n");
}


int main(int argc, const char* argv[]) {
  printf("=== hello-icu ===\n\n");

  test_strcoll();
  test_ugender();

  return 0;
}


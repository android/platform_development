/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <stdlib.h>

#include <string>

#include <unicode/ucol.h>
#include <unicode/ugender.h>
#include <unicode/unum.h>
#include <unicode/ustring.h>

#include <android/icundk.h>

TEST(ndkicu, ucoll_strcoll) {
  const std::string s1 = "hrnec";
  const std::string s2 = "chrt";

  UErrorCode status = U_ZERO_ERROR;
  UCollator* coll = ucol_open("cs_CZ", &status);
  ASSERT_FALSE(U_FAILURE(status));

  UChar us1[256];
  UChar us2[256];
  u_charsToUChars(s1.c_str(), us1, s1.size() + 1);
  u_charsToUChars(s2.c_str(), us2, s2.size() + 1);

  ASSERT_EQ(ucol_strcoll(coll, us1, s1.size(), us2, s2.size()), UCOL_LESS);

  ucol_close(coll);
}

TEST(ndkicu, ugender) {
  UVersionInfo version_info;
  u_getVersion(version_info);
  if (version_info[0] < 50) {
    GTEST_LOG_(INFO)
        << "ICU on device is too low (%s) to support ugender_getInstance()\n";
    return;
  }

  UErrorCode status = U_ZERO_ERROR;
  ugender_getInstance("fr_CA", &status);
  ASSERT_FALSE(U_FAILURE(status));
}

TEST(ndkicu, ndk_is_icu_function_available) {
  ASSERT_TRUE(ndk_is_icu_function_available("u_formatMessage"));
  ASSERT_FALSE(ndk_is_icu_function_available("u_notAFunction"));
}

int main(int argc, char **argv) {
  setenv("ICU_DATA", "/system/usr/icu", /* overwrite= */ 1);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

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
#include <unicode/udat.h>
#include <unicode/ugender.h>
#include <unicode/umsg.h>
#include <unicode/unum.h>
#include <unicode/ustring.h>

#include <android/icundk.h>

#define EXPECT_U_SUCCESS(status) EXPECT_FALSE(U_FAILURE(status)) << (status)

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

UDate MakeUDate(int year, int month, int day) {
  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<UCalendar, decltype(&ucal_close)> cal(
      ucal_open(nullptr, 0, "en_US", UCAL_GREGORIAN, &status), &ucal_close);
  EXPECT_U_SUCCESS(status);
  if (U_FAILURE(status)) {
    return 0;
  }

  status = U_ZERO_ERROR;
  ucal_setDate(cal.get(), year, month, day, &status);
  EXPECT_U_SUCCESS(status);
  if (U_FAILURE(status)) {
    return 0;
  }

  status = U_ZERO_ERROR;
  UDate date = ucal_getMillis(cal.get(), &status);
  EXPECT_U_SUCCESS(status);
  if (U_FAILURE(status)) {
    return 0;
  }

  return date;
}

std::string UStringToString(const UChar* ustr) {
  int32_t utf8_len = 0;
  UErrorCode status = U_ZERO_ERROR;
  u_strToUTF8(nullptr, 0, &utf8_len, ustr, -1, &status);
  EXPECT_EQ(status, U_BUFFER_OVERFLOW_ERROR) << u_errorName(status);
  if (status != U_BUFFER_OVERFLOW_ERROR) {
    return "";
  }

  std::string str;
  str.resize(utf8_len);
  status = U_ZERO_ERROR;
  u_strToUTF8(str.data(), str.size(), nullptr, ustr, -1, &status);
  EXPECT_U_SUCCESS(status);
  if (U_FAILURE(status)) {
    return "";
  }

  return str;
}

std::vector<UChar> FormatDate(UDate date, const char* locale) {
  UErrorCode status = U_ZERO_ERROR;
  UChar fmt[] = u"{0, date, long}";
  int32_t formatted_len =
      u_formatMessage(locale, fmt, u_strlen(fmt), nullptr, 0, &status, date);
  EXPECT_EQ(status, U_BUFFER_OVERFLOW_ERROR) << u_errorName(status);
  if (status != U_BUFFER_OVERFLOW_ERROR) {
    return std::vector<UChar>();
  }

  status = U_ZERO_ERROR;
  std::vector<UChar> formatted(formatted_len + 1);
  u_formatMessage(locale, fmt, u_strlen(fmt), formatted.data(), formatted_len,
                  &status, date);
  EXPECT_U_SUCCESS(status);
  if (U_FAILURE(status)) {
    return std::vector<UChar>();
  }

  return formatted;
}

TEST(ndkicu, u_formatMessage) {
  UDate date = MakeUDate(2018, 6, 4);
  std::vector<UChar> formatted = FormatDate(date, "ar_AE@calendar=islamic");
  std::string formatted_utf8 = UStringToString(formatted.data());
  const std::string expected =
      "\xD9\xA2\xD9\xA1 \xD8\xB4\xD9\x88\xD8\xA7\xD9\x84 "
      "\xD9\xA1\xD9\xA4\xD9\xA3\xD9\xA9 \xD9\x87\xD9\x80";
  ASSERT_EQ(expected, formatted_utf8) << formatted_utf8;
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

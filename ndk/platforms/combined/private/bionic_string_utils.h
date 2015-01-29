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

#ifndef _BIONIC_STRING_UTILS_H_
#define _BIONIC_STRING_UTILS_H_

#include <string.h>

static inline bool ends_with(const char* s1, const char* s2) {
  size_t s1_length = strlen(s1);
  size_t s2_length = strlen(s2);
  if (s2_length > s1_length) {
    return false;
  }
  return memcmp(s1 + (s1_length - s2_length), s2, s2_length) == 0;
}

#endif // _BIONIC_STRING_UTILS_H_

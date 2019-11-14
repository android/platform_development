/*
 * Copyright 2019 The Android Open Source Project
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

#ifndef CPP2C_GLOBALS_H_
#define CPP2C_GLOBALS_H_

#include <map>
#include <string>

#include "Resources.h"

namespace cpp2c {

/** Classes to be wrapped **/
Resources* kClassList;

/** Header to be included in the generated cpp file **/
std::string kHeaderForSrcFile("");

/** Map with function names used to create unique function signatures in case of
 * overloads **/
std::map<std::string, int> kFuncList;

}  // namespace cpp2c

#endif  // CPP2C_GLOBALS_H_
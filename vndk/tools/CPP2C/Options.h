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

#ifndef CPP2C_OPTIONS_H_
#define CPP2C_OPTIONS_H_

#include "clang/Tooling/CommonOptionsParser.h"

namespace cpp2c {

/** Options
 * Custom command line arguments are declared here
 * **/
llvm::cl::OptionCategory kCPP2CCategory(
    "CPP2C options");  // not declared as const as cl::cat used below
                       // expects a non-const ref

/** wrap: list of classes to be wrapped.
 * Format of file:
 * cppclass_name1
 * cppclass_name2
 **/
const llvm::cl::opt<std::string> kClassesToBeWrappedArg(
    "wrap", llvm::cl::cat(kCPP2CCategory));

// TODO: add support for including handwritten code

}  // namespace cpp2c

#endif  // CPP2C_OPTIONS_H_
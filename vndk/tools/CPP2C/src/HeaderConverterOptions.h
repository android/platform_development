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

#ifndef CPP2C_HEADERCONVERTEROPTIONS_H_
#define CPP2C_HEADERCONVERTEROPTIONS_H_

#include <string>

#include "src/utils/Resources.h"

namespace cpp2c {

using utils::Resources;

/**
 * Holds the parameters passed to the tool as command line arguments
 */
struct HeaderConverterOptions {
  std::string input_header;
  std::string output_path;
  std::unique_ptr<Resources> classes;
};

}  // namespace cpp2c

#endif  // CPP2C_HEADERCONVERTEROPTIONS_H_
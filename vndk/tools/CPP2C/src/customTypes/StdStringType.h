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

#ifndef CPP2C_STDSTRINGTYPE_H_
#define CPP2C_STDSTRINGTYPE_H_

#include "src/ICustomTypeBase.h"
#include "src/converters/FunctionConverter.h"

namespace cpp2c {
namespace customType {

/**
 * Handles std::string by creating a helper function called stringToC that
 * allocates memory on the heap (using malloc), copies the std::string (using
 * strncpy) and appends a null value after the string.
 */
class StdStringType final : public ICustomTypeBase {
 public:
  virtual bool match(const TypeMetadata& type_metadata) const override;

  virtual void addAsParameter(const std::string& param_name,
                              const TypeMetadata& type_metadata,
                              FunctionConverter* out,
                              std::string& c_type_with_name) const override;

  virtual void addAsReturnType(const cpp2c::TypeMetadata& return_type_metadata,
                               cpp2c::FunctionConverter* out) const override;

 private:
  constexpr static const char* namespace_ = "std";
  constexpr static const char* name_ = "basic_string";

  std::string createStringToCFunction(const std::string& type) const;
};

}  // namespace customType
}  // namespace cpp2c

#endif  // CPP2C_STDSTRINGTYPE_H_
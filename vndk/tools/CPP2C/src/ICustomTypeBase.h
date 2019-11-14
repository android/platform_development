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

#ifndef CPP2C_CUSTOMTYPEBASE_H_
#define CPP2C_CUSTOMTYPEBASE_H_

#include "src/converters/TypeMetadata.h"

namespace cpp2c {

class FunctionConverter;

/**
 * Interface to be implemented in order to add support for special use cases
 * The `match` function will be called first and if it returns true then the
 * corresponding function will be called (to add the type as a parameter or as a
 * return type) See src/customTypes for examples.
 */
class ICustomTypeBase {
 public:
  virtual bool match(const TypeMetadata& type_metadata) const = 0;

  virtual void addAsParameter(const std::string& param_name,
                              const TypeMetadata& type_metadata,
                              FunctionConverter* out,
                              std::string& c_type_with_name) const = 0;

  virtual void addAsReturnType(const TypeMetadata& return_type_metadata,
                               FunctionConverter* out) const = 0;
};

}  // namespace cpp2c

#endif  // CPP2C_CUSTOMTYPEBASE_H_
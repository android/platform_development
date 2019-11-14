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

#ifndef CPP2C_STDUNIQUEPTRTYPE_H_
#define CPP2C_STDUNIQUEPTRTYPE_H_

#include "src/ICustomTypeBase.h"
#include "src/converters/TypeMetadata.h"

namespace cpp2c {
namespace customType {

/**
 * Handles std::unique_ptr<T> smart pointers by either calling release or by
 * wrapping the raw pointer in a std::unique_ptr
 */
class StdUniquePtrType final : public ICustomTypeBase {
 public:
  virtual bool match(const TypeMetadata& type_metadata) const override;

  virtual void addAsParameter(const std::string& param_name,
                              const TypeMetadata& type_metadata,
                              FunctionConverter* out,
                              std::string& c_type_with_name) const override;

  virtual void addAsReturnType(const cpp2c::TypeMetadata& return_type_metadata,
                               cpp2c::FunctionConverter* out) const override;
};

}  // namespace customType
}  // namespace cpp2c

#endif  // CPP2C_STDUNIQUEPTRTYPE_H_
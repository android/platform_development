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

#ifndef CPP2C_SHAREDPTRTYPE_H_
#define CPP2C_SHAREDPTRTYPE_H_

#include "src/ICustomTypeBase.h"
#include "src/converters/TypeMetadata.h"

namespace cpp2c {
namespace customType {

/**
 * Base class used for handling smart shared pointers by leaking the pointer via
 * heap allocation. The client needs to call the generated "delete" function in
 * order to relinquish ownership and thus decrement the ref count
 *
 * This class also generates "delete" and "get" helper functions
 */
class SharedPtrType : public ICustomTypeBase {
 public:
  SharedPtrType(std::string ns, std::string name);

  virtual bool match(const TypeMetadata& type_metadata) const override;

  virtual void addAsParameter(const std::string& param_name,
                              const TypeMetadata& type_metadata,
                              FunctionConverter* out,
                              std::string& c_type_with_name) const override;

  virtual void addAsReturnType(const cpp2c::TypeMetadata& return_type_metadata,
                               cpp2c::FunctionConverter* out) const override;

  ~SharedPtrType();

  std::string createSharedPointerDeclarations(
      const TypeMetadata& type_metadata) const;
  std::string createSharedPointerImplementation(
      const TypeMetadata& type_metadata) const;

 private:
  const std::string namespace_;
  const std::string name_;
};

}  // namespace customType
}  // namespace cpp2c

#endif  // CPP2C_SHAREDPTRTYPE_H_
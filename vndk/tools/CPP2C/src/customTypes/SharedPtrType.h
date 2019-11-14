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

#include "../CTypeMetadata.h"
#include "../ICustomTypeBase.h"

namespace cpp2c {
namespace customType {

class SharedPtrType : public ICustomTypeBase {
 public:
  SharedPtrType(std::string ns, std::string name);

  virtual bool match(const CTypeMetadata& c_type_metadata) const override;

  virtual void addAsParameter(const std::string& param_name,
                              const CTypeMetadata& c_type_metadata,
                              CFunctionConverter* out,
                              std::string& c_type_with_name) const override;

  virtual void addAsReturnType(
      const cpp2c::CTypeMetadata& c_return_type_metadata,
      cpp2c::CFunctionConverter* out) const override;

  ~SharedPtrType();

  std::string createSharedPointerDeclarations(
      const CTypeMetadata& c_type_metadata) const;
  std::string createSharedPointerImplementation(
      const CTypeMetadata& c_type_metadata) const;

 private:
  const std::string namespace_;
  const std::string name_;
};

}  // namespace customType
}  // namespace cpp2c

#endif  // CPP2C_SHAREDPTRTYPE_H_
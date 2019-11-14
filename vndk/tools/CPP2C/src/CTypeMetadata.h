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

#ifndef CPP2C_CTYPEMETADATA_H_
#define CPP2C_CTYPEMETADATA_H_

#include <string>

namespace cpp2c {

class CTypeMetadata final {
 public:
  const std::string getCType() const;

  const std::string getCTypeWithName(const std::string name) const;

  const std::string getCastType() const;

  bool isPointer() const;

  bool isReference() const;

  bool isConst() const;

  bool isVoidType() const;

 private:
  const std::string c_type_;
  const std::string cast_type_;
  const bool is_pointer_;
  const bool is_reference_;
  const bool is_const_;
  const bool is_void_type_;
  const bool is_function_pointer_;
  const std::string function_pointer_name_left_;
  const std::string function_pointer_name_right_;

  friend class CTypeMetadataConverter;

  CTypeMetadata(const std::string c_type, const std::string cast_type,
                const bool is_pointer, const bool is_reference,
                const bool is_const, const bool is_void_type,
                const bool is_function_pointer,
                const std::string function_pointer_name_left,
                const std::string function_pointer_name_right);
};

}  // namespace cpp2c

#endif  // CPP2C_CTYPEMETADATA_H_
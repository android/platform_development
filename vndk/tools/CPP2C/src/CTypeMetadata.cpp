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

#include "CTypeMetadata.h"

namespace cpp2c {

const std::string CTypeMetadata::getCType() const {
  if (is_function_pointer_) {
    return function_pointer_name_left_ + function_pointer_name_right_;
  } else {
    return c_type_;
  }
}

const std::string CTypeMetadata::getCTypeWithName(
    const std::string name) const {
  if (is_function_pointer_) {
    return std::string(function_pointer_name_left_) + std::string(name) +
           std::string(function_pointer_name_right_);
  } else {
    return c_type_ + " " + name;
  }
}

const std::string CTypeMetadata::getCastType() const { return cast_type_; }

const std::string CTypeMetadata::getNamespace() const { return namespace_; }

const std::string CTypeMetadata::getCDefinition() const {
  return c_definition_;
}

bool CTypeMetadata::isEnum() const { return is_enum_; }

bool CTypeMetadata::isPointer() const { return is_pointer_; }

bool CTypeMetadata::isReference() const { return is_reference_; }

bool CTypeMetadata::isConst() const { return is_const_; }

bool CTypeMetadata::isVoidType() const { return is_void_type_; }

CTypeMetadata::CTypeMetadata(const std::string c_type,
                             const std::string cast_type,
                             const std::string namespaceName,
                             const std::string c_definition, const bool is_enum,
                             const bool is_pointer, const bool is_reference,
                             const bool is_const, const bool is_void_type,
                             const bool is_function_pointer,
                             const std::string function_pointer_name_left,
                             const std::string function_pointer_name_right)
    : c_type_(c_type),
      cast_type_(cast_type),
      namespace_(namespaceName),
      c_definition_(c_definition),
      is_enum_(is_enum),
      is_pointer_(is_pointer),
      is_reference_(is_reference),
      is_const_(is_const),
      is_void_type_(is_void_type),
      is_function_pointer_(is_function_pointer),
      function_pointer_name_left_(function_pointer_name_left),
      function_pointer_name_right_(function_pointer_name_right) {}

}  // namespace cpp2c
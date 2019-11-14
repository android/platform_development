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

#include "src/converters/TypeMetadata.h"

namespace cpp2c {

const std::string TypeMetadata::getCType() const { return c_type_; }

const std::string TypeMetadata::getCastType() const { return cast_type_; }

const std::string TypeMetadata::getFunctionPointerWithName(
    const std::string name) const {
  return std::string(function_pointer_name_left_) + std::string(name) +
         std::string(function_pointer_name_right_);
}

const std::string TypeMetadata::getNamespace() const { return namespace_; }

const std::string TypeMetadata::getCDefinition() const { return c_definition_; }

bool TypeMetadata::isEnum() const { return is_enum_; }

bool TypeMetadata::isPointer() const { return is_pointer_; }

bool TypeMetadata::isReference() const { return is_reference_; }

bool TypeMetadata::isValueType() const { return is_value_type_; }

bool TypeMetadata::isConst() const { return is_const_; }

bool TypeMetadata::isVoidType() const { return is_void_type_; }

bool TypeMetadata::isFunctionPointer() const { return is_function_pointer_; }

bool TypeMetadata::isTemplate() const { return is_template_; }

bool TypeMetadata::isBuiltinType() const { return is_builtin_type_; }

bool TypeMetadata::isArrayType() const { return is_array_type_; }

const std::string& TypeMetadata::getArraySize() const { return array_size_; }

std::string TypeMetadata::getTemplateName() const { return template_name_; }

const std::vector<const TypeMetadata> TypeMetadata::getTemplateArgs() const {
  return template_args_;
}

TypeMetadata::TypeMetadata(
    const std::string c_type, const std::string cast_type,
    const std::string namespaceName, const std::string c_definition,
    const bool is_enum, const bool is_pointer, const bool is_reference,
    const bool is_value_type, const bool is_const, const bool is_void_type,
    const bool is_function_pointer, const bool is_template,
    const bool is_builtin_type, const bool is_array_type,
    const std::string& array_size, const std::string template_name,
    const std::vector<const TypeMetadata> template_args,
    const std::string function_pointer_name_left,
    const std::string function_pointer_name_right)
    : c_type_(c_type),
      cast_type_(cast_type),
      namespace_(namespaceName),
      c_definition_(c_definition),
      is_enum_(is_enum),
      is_pointer_(is_pointer),
      is_reference_(is_reference),
      is_value_type_(is_value_type),
      is_const_(is_const),
      is_void_type_(is_void_type),
      is_function_pointer_(is_function_pointer),
      is_template_(is_template),
      is_builtin_type_(is_builtin_type),
      is_array_type_(is_array_type),
      array_size_(array_size),
      template_name_(template_name),
      template_args_(template_args),
      function_pointer_name_left_(function_pointer_name_left),
      function_pointer_name_right_(function_pointer_name_right) {}

}  // namespace cpp2c
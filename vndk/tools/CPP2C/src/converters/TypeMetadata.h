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

#ifndef CPP2C_TYPEMETADATA_H_
#define CPP2C_TYPEMETADATA_H_

#include <string>
#include <vector>

namespace cpp2c {

class TypeMetadata final {
 public:
  const std::string getCType() const;

  const std::string getCastType() const;

  const std::string getFunctionPointerWithName(
      const std::string name = "") const;

  const std::string getNamespace() const;

  const std::string getCDefinition() const;

  bool isEnum() const;

  bool isPointer() const;

  bool isReference() const;

  bool isValueType() const;

  bool isConst() const;

  bool isVoidType() const;

  bool isFunctionPointer() const;

  bool isTemplate() const;

  bool isBuiltinType() const;

  bool isArrayType() const;

  const std::string& getArraySize() const;

  std::string getTemplateName() const;

  const std::vector<const TypeMetadata> getTemplateArgs() const;

 private:
  const std::string c_type_;
  const std::string cast_type_;
  const std::string namespace_;
  const std::string c_definition_;
  const bool is_enum_;
  const bool is_pointer_;
  const bool is_reference_;
  const bool is_value_type_;
  const bool is_const_;
  const bool is_void_type_;
  const bool is_function_pointer_;
  const bool is_template_;
  const bool is_builtin_type_;
  const bool is_array_type_;
  const std::string array_size_;
  const std::string template_name_;
  const std::vector<const TypeMetadata> template_args_;
  const std::string function_pointer_name_left_;
  const std::string function_pointer_name_right_;

  friend class TypeConverter;

  TypeMetadata(const std::string c_type, const std::string cast_type,
               const std::string namespaceName, const std::string c_definition,
               const bool is_enum, const bool is_pointer,
               const bool is_reference, const bool is_value_type,
               const bool is_const, const bool is_void_type,
               const bool is_function_pointer, const bool is_template,
               const bool is_builtin_type, const bool is_array_type,
               const std::string& array_size, const std::string template_name,
               const std::vector<const TypeMetadata> template_args,
               const std::string function_pointer_name_left,
               const std::string function_pointer_name_right);
};

}  // namespace cpp2c

#endif  // CPP2C_TYPEMETADATA_H_
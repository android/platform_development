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

#include "Utils.h"

#include <sstream>

namespace cpp2c {

std::string getNamespaceFromContext(const clang::DeclContext* const context) {
  std::string namespace_name;
  if (!context) return namespace_name;

  if (context->isNamespace()) {
    const auto namespace_context =
        llvm::dyn_cast<clang::NamespaceDecl>(context);
    namespace_name = namespace_context->getDeclName().getAsString();
  }
  return namespace_name;
}

std::string createStructDefinition(const std::string& class_name) {
  return "struct W" + class_name + ";\n" + "typedef struct W" + class_name +
         " W" + class_name + ";\n";
}

std::string createSharedPointerDeclarations(
    const CTypeMetadata& c_type_metadata) {
  const std::vector<const CTypeMetadata> template_args =
      c_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];
  std::string type = template_arg_0.getCType() + "_shared";

  std::stringstream declarations;

  declarations << "struct " << type << ";\n";
  declarations << "typedef struct " << type << " " << type << ";\n";

  // create a function that retrieves a raw pointer from a std::shared_ptr to be
  // used with the generated C wrapper functions
  declarations << type << "* " << type << "_get"
               << "(" << type << "* "
               << "self);";
  declarations << "\n";

  // create a C function that decrements the ref count of the std::shared_ptr
  declarations << "void " << type << "_delete(" << type << "* self);";
  declarations << "\n";

  return declarations.str();
}

std::string createSharedPointerImplementation(
    const CTypeMetadata& c_type_metadata) {
  const std::vector<const CTypeMetadata> template_args =
      c_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon =
      cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

  std::string type = template_arg_0.getCType() + "_shared";

  std::stringstream implementation;
  implementation << type << "* " << type << "_get"
                 << "(" << type << "* "
                 << "self) {\n";
  implementation << "\treturn reinterpret_cast<" << type << "*>("
                 << "reinterpret_cast<std::shared_ptr<" << namespace_with_colon
                 << template_arg_0.getCastType() << ">*>(self)->get());";
  implementation << "\n}\n";
  implementation << "\n";

  implementation << "void " << type << "_delete(" << type << "* self) {\n";
  implementation << "\tdelete reinterpret_cast<std::shared_ptr<"
                 << namespace_with_colon << template_arg_0.getCastType()
                 << ">*>(self);";
  implementation << "\n}\n";

  return implementation.str();
}

std::string appendIfNotEmpty(const std::string& target,
                             const std::string& suffix) {
  return target.size() != 0 ? target + "::" : "";
}

bool isSmartPointer(const CTypeMetadata& c_type_metadata) {
  bool is_smart_ptr = false;

  if (c_type_metadata.isTemplate()) {
    std::vector<std::string> smart_ptrs{
        "unique_ptr",  // std
        "shared_ptr",  // std
    };

    for (unsigned int i = 0; i < smart_ptrs.size(); ++i) {
      if (c_type_metadata.getTemplateName().find(smart_ptrs[i]) !=
          std::string::npos) {
        is_smart_ptr = true;
      }
    }
  }

  return is_smart_ptr;
}

bool isUniquePointer(const CTypeMetadata& c_type_metadata) {
  std::vector<std::string> smart_ptrs{
      "unique_ptr",  // std
  };

  bool is_smart_ptr = false;

  if (c_type_metadata.isTemplate() &&
      c_type_metadata.getTemplateName() == "unique_ptr") {
    is_smart_ptr = true;
  }

  return is_smart_ptr;
}

bool isSharedPointer(const CTypeMetadata& c_type_metadata) {
  bool is_smart_ptr = false;

  if (c_type_metadata.isTemplate() &&
      c_type_metadata.getTemplateName() == "shared_ptr") {
    is_smart_ptr = true;
  }

  return is_smart_ptr;
}  // namespace cpp2c

}  // namespace cpp2c
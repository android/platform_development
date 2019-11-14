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

#include "SharedPtrType.h"

#include "src/CFunctionConverter.h"
#include "src/Utils.h"

namespace cpp2c {
namespace customType {

SharedPtrType::SharedPtrType(std::string ns, std::string name)
    : namespace_(ns), name_(name) {}

SharedPtrType::~SharedPtrType() {}

bool SharedPtrType::match(const CTypeMetadata& c_type_metadata) const {
  if (c_type_metadata.isTemplate() &&
      c_type_metadata.getTemplateName() == std::string(name_)) {
    return true;
  }

  return false;
}

void SharedPtrType::addAsParameter(const std::string& param_name,
                                   const CTypeMetadata& c_type_metadata,
                                   CFunctionConverter* out,
                                   std::string& c_type_with_name) const {
  const std::vector<const CTypeMetadata> template_args =
      c_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon =
      cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

  std::string type = template_arg_0.getCType() + "_" + name_;
  std::string is_const_string = c_type_metadata.isConst() ? "const " : "";

  c_type_with_name = type + "* " + param_name;

  out->function_body_ << "*reinterpret_cast<" << is_const_string << namespace_
                      << "::" << name_ << "<" << namespace_with_colon
                      << template_arg_0.getCastType() << ">*>(" << param_name
                      << ")";

  out->definitions_[type] = createSharedPointerDeclarations(c_type_metadata);
  out->additional_implementations_[type] =
      createSharedPointerImplementation(c_type_metadata);
}

void SharedPtrType::addAsReturnType(const CTypeMetadata& c_return_type_metadata,
                                    CFunctionConverter* out) const {
  const std::vector<const CTypeMetadata> template_args =
      c_return_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon =
      cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

  std::string type = template_arg_0.getCType() + "_" + name_;
  out->return_type_ = type + "*";

  if (template_arg_0.getCastType().size() != 0) {
    out->function_body_ << "reinterpret_cast<" << out->return_type_ << ">(";
    out->body_end_ += ")";
  }

  out->function_body_ << "new " << namespace_ << "::" << name_ << "<"
                      << namespace_with_colon << template_arg_0.getCastType()
                      << ">(";
  out->body_end_ += ")";

  out->definitions_[type] =
      createSharedPointerDeclarations(c_return_type_metadata);
  out->additional_implementations_[template_arg_0.getCType() + "_" +
                                   std::string(name_)] =
      createSharedPointerImplementation(c_return_type_metadata);
}

std::string SharedPtrType::createSharedPointerDeclarations(
    const CTypeMetadata& c_type_metadata) const {
  const std::vector<const CTypeMetadata> template_args =
      c_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];
  std::string type = template_arg_0.getCType() + "_" + name_;

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

std::string SharedPtrType::createSharedPointerImplementation(
    const CTypeMetadata& c_type_metadata) const {
  const std::vector<const CTypeMetadata> template_args =
      c_type_metadata.getTemplateArgs();

  const CTypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon =
      cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

  std::string type = template_arg_0.getCType() + "_" + name_;

  std::string cast = "reinterpret_cast<" + namespace_ + "::" + name_;

  std::stringstream implementation;
  implementation << type << "* " << type << "_get"
                 << "(" << type << "* "
                 << "self){\n";
  implementation << "\treturn reinterpret_cast<" << type << "*>(" << cast << "<"
                 << namespace_with_colon << template_arg_0.getCastType()
                 << ">*>(self)->get());";
  implementation << "\n}\n";
  implementation << "\n";

  implementation << "void " << type << "_delete(" << type << "* self){\n";
  implementation << "\tdelete " << cast << "<" << namespace_with_colon
                 << template_arg_0.getCastType() << ">*>(self);";
  implementation << "\n}\n";

  return implementation.str();
}

}  // namespace customType
}  // namespace cpp2c
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

#include "src/customTypes/StdStringType.h"

namespace cpp2c {
namespace customType {

bool StdStringType::match(const TypeMetadata& type_metadata) const {
  if (type_metadata.isTemplate() &&
      type_metadata.getTemplateName() == std::string(name_)) {
    return true;
  }

  return false;
}

void StdStringType::addAsParameter(const std::string& param_name,
                                   const TypeMetadata& type_metadata,
                                   FunctionConverter* out,
                                   std::string& c_type_with_name) const {
  const std::vector<const TypeMetadata> template_args =
      type_metadata.getTemplateArgs();

  const TypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon = std::string(namespace_) + "::";

  std::string type = template_arg_0.getCType();

  c_type_with_name = type + "* " + param_name;

  out->function_body_ << namespace_ << "::" << name_ << "<" << type << ">("
                      << param_name << ")";
}

void StdStringType::addAsReturnType(
    const cpp2c::TypeMetadata& return_type_metadata,
    cpp2c::FunctionConverter* out) const {
  const std::vector<const TypeMetadata> template_args =
      return_type_metadata.getTemplateArgs();

  const TypeMetadata template_arg_0 = template_args[0];

  std::string namespace_with_colon = std::string(namespace_) + "::";

  std::string type = template_arg_0.getCType();
  out->return_type_ = type + "*";

  out->function_body_ << "stringToC(";
  out->body_end_ += ")";

  out->addImplementation("stringToC_" + type, createStringToCFunction(type));
}

std::string StdStringType::createStringToCFunction(
    const std::string& type) const {
  return type + "* stringToC(const std::string& s) {\n\t" + type +
         "* copy;\n\t" + "copy = (" + type + "*)(malloc(s.size() * sizeof(" +
         type + ") + 1));\n\t" + "strncpy(copy, s.c_str(), s.size());\n\t" +
         "copy[s.size() + 1] = \'\\0\';\n\t" + "return copy;\n" + "}";
}

}  // namespace customType
}  // namespace cpp2c
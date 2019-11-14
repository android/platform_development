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

#include "src/customTypes/StdUniquePtrType.h"

#include "src/converters/FunctionConverter.h"
#include "src/utils/Utils.h"

namespace cpp2c {
namespace customType {

bool StdUniquePtrType::match(const TypeMetadata& type_metadata) const {
  // TODO : add namespace check
  if (type_metadata.isTemplate() &&
      type_metadata.getTemplateName() == "unique_ptr") {
    return true;
  }

  return false;
}

void StdUniquePtrType::addAsParameter(const std::string& param_name,
                                      const TypeMetadata& type_metadata,
                                      FunctionConverter* out,
                                      std::string& c_type_with_name) const {
  const std::vector<const TypeMetadata> template_args =
      type_metadata.getTemplateArgs();

  const TypeMetadata template_arg_0 = template_args[0];

  c_type_with_name = template_arg_0.getCType() + "* " + param_name;

  std::string template_arg_namespace_with_colon =
      cpp2c::utils::appendIfNotEmpty(template_arg_0.getNamespace(), "::");
  std::string is_const_string = type_metadata.isConst() ? "const " : "";

  out->function_body_ << "std::move(std::unique_ptr<"
                      << template_arg_namespace_with_colon
                      << template_arg_0.getCastType() << ">(";

  out->function_body_ << "reinterpret_cast<" << is_const_string
                      << template_arg_namespace_with_colon
                      << template_arg_0.getCastType() << "*"
                      << ">(" << param_name << ")";

  out->function_body_ << "))";

  out->addDefinition(template_arg_0.getCastType(),
                     template_arg_0.getCDefinition());
}

void StdUniquePtrType::addAsReturnType(const TypeMetadata& return_type_metadata,
                                       FunctionConverter* out) const {
  const std::vector<const TypeMetadata> template_args =
      return_type_metadata.getTemplateArgs();

  const TypeMetadata template_arg_0 = template_args[0];

  out->return_type_ = template_arg_0.getCType() + "*";

  out->function_body_ << "reinterpret_cast<" << out->return_type_ << ">(";
  out->body_end_ += ")";

  out->body_end_ += ".release()";
}

}  // namespace customType

}  // namespace cpp2c
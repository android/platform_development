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

std::string appendIfNotEmpty(const std::string& target,
                             const std::string& suffix) {
  return target.size() != 0 ? target + "::" : "";
}

}  // namespace cpp2c
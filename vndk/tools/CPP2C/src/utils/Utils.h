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

#ifndef CPP2C_UTILS_H_
#define CPP2C_UTILS_H_

#include <string>

#include "clang/AST/ASTContext.h"
#include "src/CustomTypeManager.h"
#include "src/converters/TypeMetadata.h"

namespace cpp2c {
namespace utils {

/**
 * Returns the namespace name (as a std::string) of the given context.
 * Note that this does not recur up the hierarchy
 */
std::string getNamespaceFromContext(const clang::DeclContext* const context);

/**
 * Wraps the given string and returns a struct declaration for the wrapper.
 */
std::string createStructDefinition(const std::string& class_name);

/**
 * Returns a new string comprised of target appended with suffix.
 * Return an empty string if target is empty.
 */
std::string appendIfNotEmpty(const std::string& target,
                             const std::string& suffix);

/**
 * Adds support for custom classes.
 * Example: to handle std::unique_ptr we can implement the interface required by
 * CustomTypeManager and add the class to the manager inside addCustomClasses
 */
void addCustomClasses(CustomTypeManager& manager);

}  // namespace utils
}  // namespace cpp2c

#endif  // CPP2C_UTILS_H_
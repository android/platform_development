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

#ifndef CPP2C_CFUNCTIONCONVERTER_H_
#define CPP2C_CFUNCTIONCONVERTER_H_

#include <sstream>
#include <string>

#include "clang/ASTMatchers/ASTMatchers.h"
#include "src/CustomTypeManager.h"
#include "src/HeaderConverterOptions.h"
#include "src/converters/TypeConverter.h"
#include "src/converters/TypeMetadata.h"
#include "src/utils/OutputStreams.h"

namespace cpp2c {

/**
 * Constructs an equivalent C function given a C++ function
 * This class also generates any declarations that are needed by the passed in
 * method
 */
class FunctionConverter {
 public:
  FunctionConverter(const clang::CXXMethodDecl* method_decl,
                    const HeaderConverterOptions& options);

  void run(const clang::ASTContext* c);

  std::string getFunctionName() const;

  std::string getFunctionBody() const;

  std::map<std::string, std::string> getTypeDefinitions() const;

  std::map<std::string, std::string> getAdditionalImplementations() const;

  void addDefinition(
      std::string key,
      std::string value);  // this will get added to the generated h file
  void addImplementation(
      std::string key,
      std::string value);  // this will get added to the generated cpp file

  // The below members are public in order to be accessible from classes that
  // deal with custom types such as StdUniquePtrType

  std::string method_name_ = "";
  std::string class_name_ = "";
  std::string self_ = "";
  std::string separator_ = "";
  std::string body_end_ = "";
  std::string class_namespace_ = "";
  std::string return_type_ = "";

  std::stringstream function_name_;
  std::stringstream function_body_;

 private:
  std::string generateFunctionName() const;

  void generateFunctionCall();

  void generateConstructor();

  void generateDestructor();

  void generateReturnValue();

  const clang::CXXMethodDecl* const method_decl_;

  const HeaderConverterOptions& options_;

  std::map<std::string, std::string>
      definitions_;  // maps a C++ class name to its C-transpiled definition
  std::map<std::string, std::string>
      additional_implementations_;  // stores additional functions needed by a C
                                    // class that have no C++ equivalent

  CustomTypeManager
      customTypes_;  // TODO there should only be one CustomTypeManager per
                     // converter, not one per FunctionConverter
};

}  // namespace cpp2c

#endif  // CPP2C_CFUNCTIONCONVERTER_H_

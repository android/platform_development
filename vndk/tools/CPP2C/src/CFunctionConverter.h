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

#include "CTypeMetadata.h"
#include "CTypeMetadataConverter.h"
#include "OutputStreams.h"
#include "clang/ASTMatchers/ASTMatchers.h"

namespace cpp2c {

/**
 * Constructs an equivalent C function given a C++ typefunction
 */
class CFunctionConverter {
 public:
  CFunctionConverter(const clang::CXXMethodDecl* c, OutputStreams& o);

  void run(const clang::ASTContext* c, const Resources* cl);

  std::string getFunctionName();

  std::string getFunctionBody();

  std::map<std::string, std::string> getTypeDefinitions();

 private:
  const clang::CXXMethodDecl* const method_decl_;

  std::string method_name_ = "";
  std::string class_name_ = "";
  std::string self_ = "";
  std::string separator_ = "";
  std::string body_end_ = "";
  std::string class_namespace_ = "";
  std::string return_type_ = "";

  std::stringstream function_name_;
  std::stringstream function_body_;

  std::map<std::string, std::string>
      definitions_;  // maps a C++ class name to its C-transpiled definition

  std::stringstream createFunctionName(const std::string& return_type,
                                       const std::string& class_name,
                                       const std::string& method_name) const;

  void addFunctionCall(const CTypeMetadata* c_return_type_metadata);

  void checkIfValueType(const CTypeMetadata& c_type_metadata);

  void retrieveAsConstructor();

  void retrieveAsDestructor();

  void retrieveAsFunction(CTypeMetadataConverter& type_converter);
};

}  // namespace cpp2c

#endif  // CPP2C_CFUNCTIONCONVERTER_H_

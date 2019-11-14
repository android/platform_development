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

#ifndef CPP2C_TYPECONVERTER_H_
#define CPP2C_TYPECONVERTER_H_

#include "clang/AST/ASTContext.h"
#include "src/HeaderConverterOptions.h"
#include "src/converters/TypeMetadata.h"

namespace cpp2c {

/**
 * Constructs an equivalent C type given a C++ type
 */
class TypeConverter {
 public:
  static const TypeMetadata createTypeMetadata(
      const HeaderConverterOptions& options, const clang::QualType& type);

 private:
  TypeConverter(const HeaderConverterOptions& options);

  TypeMetadata determineCType(const clang::QualType& qt);

  std::string c_type_ = "";  // the equivalent wrapped C name of the C++ record
  std::string cast_type_ =
      "";  // whether this should be casted or not, this is used
           // when a class/struct is wrapped with a C struct or when converting
           // a C++ enum to a C enum
  bool is_pointer_ = false;
  bool is_reference_ = false;
  bool is_void_type_ = false;
  bool is_function_pointer_ = false;
  std::string namespace_ = "";
  std::string c_definition_ = "";  // holds the C wrapper's definition
  bool is_template_ = false;
  std::string template_name_ = "";
  std::vector<const TypeMetadata> template_args_;

  // used if the type is a function pointer, e.g. void (* foo) (void*, void*)
  // in this case we keep the name as "void(*" + ") (void*, void*)" as we might
  // need to add the variable's name in the middle
  std::string function_pointer_name_left_ = "";
  std::string function_pointer_name_right_ = "";

  const HeaderConverterOptions& options_;

  void wrapClassName(const std::string& name);

  void handleEnum(const clang::QualType& qt);

  void handleFunctionPointer(const clang::QualType& qt);

  void handleRecordType(const clang::CXXRecordDecl* crd);

  void handleTemplate(const clang::CXXRecordDecl* crd);

  bool isConst(const clang::QualType& qt);
};

}  // namespace cpp2c

#endif  // CPP2C_TYPECONVERTER_H_
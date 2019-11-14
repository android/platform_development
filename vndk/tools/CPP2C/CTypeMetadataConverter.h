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

#ifndef CPP2C_CTYPEMETADATACONVERTER_H_
#define CPP2C_CTYPEMETADATACONVERTER_H_

#include "CTypeMetadata.h"
#include "Resources.h"
#include "clang/AST/ASTContext.h"

namespace cpp2c {

/**
 * Constructs an equivalent C type given a C++ type
 */
class CTypeMetadataConverter {
  std::string c_type_ = "";
  std::string cast_type_ =
      "";  // whether this should be casted or not, this is used
           // when a class/struct is wrapped with a C struct
  bool is_pointer_ = false;
  bool is_reference_ = false;
  bool is_void_type_ = true;
  bool is_function_pointer_ = false;

  // used if the type is a function pointer, e.g. void (* foo) (void*, void*)
  // in this case we keep the name as "void(*" + ") (void*, void*)" as we might
  // need to add the variable's name in the middle
  std::string fp_name_left_ = "";
  std::string fp_name_right_ = "";

  void reset();

  void wrapClassName(const std::string& name);

  void getAsEnum(const clang::QualType& qt);

  void getAsFunctionPointer(const clang::QualType& qt);

  bool isConst(const clang::QualType& qt);

 public:
  CTypeMetadataConverter();

  CTypeMetadata determineCType(const clang::QualType& qt);
};

}  // namespace cpp2c

#endif  // CPP2C_CTYPEMETADATACONVERTER_H_
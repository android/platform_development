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

#include "CTypeMetadataConverter.h"

#include "clang/ASTMatchers/ASTMatchers.h"

namespace cpp2c {

extern Resources* kClassList;

void CTypeMetadataConverter::reset() {
  c_type_ = "";
  cast_type_ = "";
  is_pointer_ = false;
  is_reference_ = false;
  is_void_type_ = false;
  is_function_pointer_ = false;
  fp_name_left_ = "";
  fp_name_right_ = "";
}

void CTypeMetadataConverter::wrapClassName(const std::string& name) {
  const std::vector<std::string>& classes = kClassList->getResources();
  if (std::find(classes.begin(), classes.end(), name) != classes.end()) {
    c_type_ = "W" + name + "*";
    cast_type_ = name;
  } else {
    c_type_ = name + "*";
  }
}

void CTypeMetadataConverter::getAsEnum(const clang::QualType& qt) {
  c_type_ = qt.getAsString();
}

void CTypeMetadataConverter::getAsFunctionPointer(const clang::QualType& qt) {
  const clang::QualType pointee =
      qt->castAs<clang::PointerType>()->getPointeeType();
  if (pointee->isFunctionProtoType()) {
    is_function_pointer_ = true;
    const clang::FunctionProtoType* function_prototype =
        pointee->castAs<clang::FunctionProtoType>();

    fp_name_left_ = function_prototype->getReturnType().getAsString() + "(*";
    fp_name_right_ = ") (";
    for (unsigned int i = 0; i < function_prototype->getNumParams(); ++i) {
      // TODO: we assume this is a builtin type here, we should recurse
      // instead for each param
      const clang::QualType param = function_prototype->getParamType(i);
      fp_name_right_ += (i != 0 ? ", " : "") + param.getAsString();
    }
    fp_name_right_ += ")";
  }
}

bool CTypeMetadataConverter::isConst(const clang::QualType& qt) {
  return qt.getAsString().find("const ") != std::string::npos;
}

CTypeMetadataConverter::CTypeMetadataConverter() {}

CTypeMetadata CTypeMetadataConverter::determineCType(
    const clang::QualType& qt) {
  reset();

  if (qt->isFunctionPointerType()) {
    getAsFunctionPointer(qt);
  }
  // if it is builtin type, or pointer to builtin, use it as is
  else if (qt->isBuiltinType() ||
           (qt->isPointerType() && qt->getPointeeType()->isBuiltinType())) {
    c_type_ = qt.getAsString();
    if (qt->isVoidType()) {
      is_void_type_ = true;
    }
    is_pointer_ = true;
  } else if (qt->isEnumeralType()) {
    getAsEnum(qt);
  } else if (qt->isRecordType()) {
    const clang::CXXRecordDecl* crd = qt->getAsCXXRecordDecl();
    std::string record_name = crd->getQualifiedNameAsString();

    if (llvm::isa<clang::ClassTemplateSpecializationDecl>(crd)) {
      // no template support
    } else {
      wrapClassName(record_name);
    }
  } else if ((qt->isReferenceType() || qt->isPointerType()) &&
             qt->getPointeeType()->isRecordType()) {
    if (qt->isPointerType() && qt->getPointeeType()->isRecordType()) {
      is_pointer_ = true;  // to properly differentiate among cast types
    }
    if (qt->isReferenceType()) {
      is_reference_ = true;
    }
    const clang::CXXRecordDecl* crd =
        qt->getPointeeType()->getAsCXXRecordDecl();
    std::string record_name = crd->getQualifiedNameAsString();

    wrapClassName(record_name);
  }

  return CTypeMetadata(c_type_, cast_type_, is_pointer_, is_reference_,
                       isConst(qt), is_void_type_, is_function_pointer_,
                       fp_name_left_, fp_name_right_);
}

}  // namespace cpp2c
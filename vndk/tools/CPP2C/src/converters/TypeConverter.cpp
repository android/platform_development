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

#include "src/converters/TypeConverter.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "src/utils/Utils.h"

namespace cpp2c {

const TypeMetadata TypeConverter::createTypeMetadata(
    const HeaderConverterOptions& options, const clang::QualType& type) {
  TypeConverter converter(options);
  return converter.determineCType(type);
}

TypeConverter::TypeConverter(const HeaderConverterOptions& options)
    : options_(options) {}

void TypeConverter::wrapClassName(const std::string& name) {
  const std::vector<std::string>& classes = options_.classes->getResources();
  if (std::find(classes.begin(), classes.end(), name) != classes.end()) {
    c_type_ = "W" + name;
    cast_type_ = name;
  } else {
    c_type_ = name;
  }
}

void TypeConverter::handleEnum(const clang::QualType& qt) {
  const clang::EnumType* enumtype = qt->getAs<clang::EnumType>();
  std::string enumName = enumtype->getDecl()->getNameAsString();

  wrapClassName(enumName);

  constexpr char newLine = '\n';
  c_definition_ = "enum " + c_type_ + " {" + newLine;

  const clang::EnumDecl* decl = enumtype->getDecl();
  for (auto it = decl->enumerator_begin(); it != decl->enumerator_end(); ++it) {
    c_definition_ += '\t' + it->getNameAsString() + "," + newLine;
  }

  c_definition_ += "};";
  c_definition_ += newLine;

  namespace_ = cpp2c::utils::getNamespaceFromContext(
      decl->getEnclosingNamespaceContext());
}

void TypeConverter::handleFunctionPointer(const clang::QualType& qt) {
  const clang::QualType pointee =
      qt->castAs<clang::PointerType>()->getPointeeType();

  if (pointee->isFunctionProtoType()) {
    is_function_pointer_ = true;
    const clang::FunctionProtoType* function_prototype =
        pointee->castAs<clang::FunctionProtoType>();

    function_pointer_name_left_ =
        function_prototype->getReturnType().getAsString() + "(*";
    function_pointer_name_right_ = ") (";

    for (unsigned int i = 0; i < function_prototype->getNumParams(); ++i) {
      // TODO: we assume this is a builtin type here, we should recurse
      // instead for each param
      const clang::QualType param = function_prototype->getParamType(i);
      function_pointer_name_right_ +=
          (i != 0 ? ", " : "") + param.getAsString();
    }

    function_pointer_name_right_ += ")";
  }
}

void TypeConverter::handleRecordType(const clang::CXXRecordDecl* crd) {
  std::string record_name = crd->getName();

  namespace_ = cpp2c::utils::getNamespaceFromContext(
      crd->getEnclosingNamespaceContext());

  if (llvm::isa<clang::ClassTemplateSpecializationDecl>(crd)) {
    handleTemplate(crd);
  } else {
    wrapClassName(record_name);
    c_definition_ = cpp2c::utils::createStructDefinition(record_name);
  }
}

void TypeConverter::handleTemplate(const clang::CXXRecordDecl* crd) {
  // no actual template support but we need to support smart pointers
  auto template_specialization =
      llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(crd);

  template_name_ = template_specialization->getNameAsString();
  is_template_ = true;

  const clang::TemplateArgumentList& argList =
      template_specialization->getTemplateArgs();

  const clang::TemplateArgument template_arg =
      argList[0];  // just get the first param for now as we
                   // need basic support for smart pointers

  const clang::QualType template_arg_type = template_arg.getAsType();

  const TypeMetadata template_arg_metadata =
      createTypeMetadata(options_, template_arg_type);

  template_args_.push_back(template_arg_metadata);
}

bool TypeConverter::isConst(const clang::QualType& qt) {
  return qt.getAsString().find("const ") != std::string::npos;
}

TypeMetadata TypeConverter::determineCType(const clang::QualType& type) {
  bool is_builtin_type = false;
  bool is_value_type = false;
  bool is_array_type = false;
  std::string array_size = "";

  const clang::QualType& desugared_type = type.getCanonicalType();

  if (desugared_type->isFunctionPointerType()) {
    handleFunctionPointer(desugared_type);
  } else if (desugared_type->isPointerType() &&
             desugared_type->getPointeeType()->isBuiltinType()) {
    clang::LangOptions lang_options;
    clang::PrintingPolicy printing_policy(lang_options);

    const clang::QualType pointee = type->getPointeeType();
    const auto builtin_type = pointee->getAs<const clang::BuiltinType>();
    c_type_ = builtin_type->getName(printing_policy).str();

    if (desugared_type->isVoidType()) {
      is_void_type_ = true;
    }
    is_builtin_type = true;

    // DecayedType = pointer decayed from array or function
    if (llvm::dyn_cast<clang::DecayedType>(type)) {
      auto decayed_type = llvm::dyn_cast<clang::DecayedType>(type);
      auto original_type = decayed_type->getOriginalType();

      if (llvm::dyn_cast<clang::ArrayType>(original_type)) {
        is_array_type = true;
      }

      if (llvm::dyn_cast<clang::ConstantArrayType>(original_type)) {
        is_array_type = true;

        auto constant_array_type =
            llvm::dyn_cast<clang::ConstantArrayType>(original_type);
        constexpr unsigned radix = 10;
        constexpr bool is_signed = false;
        std::string size =
            constant_array_type->getSize().toString(radix, is_signed);

        array_size += size;  // this needs to be added after the parameter name
      }
    } else {
      // if this is not an array but is a pointer
      is_pointer_ = true;
    }
  } else if (desugared_type->isBuiltinType()) {
    clang::LangOptions lang_options;
    clang::PrintingPolicy printing_policy(lang_options);

    const auto builtin_type = desugared_type->getAs<const clang::BuiltinType>();
    c_type_ = builtin_type->getName(printing_policy).str();

    if (desugared_type->isVoidType()) {
      is_void_type_ = true;
    }
    is_builtin_type = true;
  } else if (desugared_type->isEnumeralType()) {
    handleEnum(desugared_type);
  } else if (desugared_type->isRecordType()) {
    is_value_type = true;

    const clang::CXXRecordDecl* crd = desugared_type->getAsCXXRecordDecl();
    handleRecordType(crd);
  } else if ((desugared_type->isReferenceType() ||
              desugared_type->isPointerType()) &&
             desugared_type->getPointeeType()->isRecordType()) {
    if (desugared_type->isPointerType() &&
        desugared_type->getPointeeType()->isRecordType()) {
      is_pointer_ = true;
    }
    if (desugared_type->isReferenceType()) {
      is_reference_ = true;
    }
    const clang::CXXRecordDecl* crd =
        desugared_type->getPointeeType()->getAsCXXRecordDecl();

    handleRecordType(crd);
  }

  return TypeMetadata(
      c_type_, cast_type_, namespace_, c_definition_,
      desugared_type->isEnumeralType(), is_pointer_, is_reference_,
      is_value_type, isConst(desugared_type), is_void_type_,
      is_function_pointer_, is_template_, is_builtin_type, is_array_type,
      array_size, template_name_, template_args_, function_pointer_name_left_,
      function_pointer_name_right_);
}

}  // namespace cpp2c
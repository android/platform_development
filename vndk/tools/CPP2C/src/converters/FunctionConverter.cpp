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

#include "src/converters/FunctionConverter.h"

#include <map>
#include <string>

#include "src/CustomTypeManager.h"
#include "src/utils/Utils.h"

namespace cpp2c {

using cpp2c::utils::OutputStreams;

/** Map with function names used to create unique function signatures in case of
 * overloads **/
static std::map<std::string, int> kFuncList;

FunctionConverter::FunctionConverter(const clang::CXXMethodDecl* method_decl,
                                     const HeaderConverterOptions& options)
    : method_decl_(method_decl), options_(options) {
  class_name_ = method_decl_->getParent()->getDeclName().getAsString();
  self_ = "W" + class_name_ + "* self";
  separator_ = ", ";

  cpp2c::utils::addCustomClasses(customTypes_);
}

void FunctionConverter::run(const clang::ASTContext* c) {
  // ignore operator overloadings
  if (method_decl_->isOverloadedOperator()) {
    return;
  }

  class_namespace_ = cpp2c::utils::getNamespaceFromContext(
      method_decl_->getEnclosingNamespaceContext());

  addDefinition(class_name_, cpp2c::utils::createStructDefinition(class_name_));

  // create the function body (note: the parameters are added later)
  if (const clang::CXXConstructorDecl* ccd =
          llvm::dyn_cast<clang::CXXConstructorDecl>(method_decl_)) {
    if (ccd->isCopyConstructor() || ccd->isMoveConstructor()) {
      return;
    }
    generateConstructor();
  } else if (llvm::isa<clang::CXXDestructorDecl>(method_decl_)) {
    generateDestructor();
  } else {
    generateReturnValue();
    generateFunctionCall();
  }

  function_name_ << return_type_ << " " << generateFunctionName();

  // add the parameters for both the function signature and the function call
  // in the body
  for (unsigned int i = 0; i < method_decl_->getNumParams(); ++i) {
    const clang::ParmVarDecl* param = method_decl_->parameters()[i];
    const clang::QualType parameter_qt = param->getType();

    TypeMetadata type_metadata =
        TypeConverter::createTypeMetadata(options_, parameter_qt);
    std::string is_const_string = type_metadata.isConst() ? "const " : "";

    // in the header, the name is optional, only the type is mandatory
    const std::string param_name = param->getQualifiedNameAsString().size() != 0
                                       ? param->getQualifiedNameAsString()
                                       : ("param" + std::to_string(i));

    std::string namespace_with_colon =
        cpp2c::utils::appendIfNotEmpty(type_metadata.getNamespace(), "::");

    std::string c_type_with_name;
    if (type_metadata.isFunctionPointer()) {
      c_type_with_name = type_metadata.getFunctionPointerWithName(param_name);
    } else if (type_metadata.isArrayType()) {
      c_type_with_name = type_metadata.getCType() + " " + param_name + "[" +
                         type_metadata.getArraySize() + "]";
    } else {
      c_type_with_name = type_metadata.getCType();
      if (type_metadata.isPointer() || type_metadata.isReference() ||
          type_metadata.isValueType()) {
        c_type_with_name += "*";
      }
      c_type_with_name += " " + param_name;
    }

    auto customType = customTypes_.match(type_metadata);
    const bool is_custom_type = customType != nullptr;

    if (is_custom_type) {
      customType->addAsParameter(param_name, type_metadata, this,
                                 c_type_with_name);
    }

    // for the function name, separator needs to be "" initially if constr
    // or static, otherwise it's ", " because we always have self as a param
    function_name_ << separator_ << is_const_string << c_type_with_name << "";

    // for the function body, add nothing before first param and ", " before
    // the others
    if (i != 0) {
      function_body_ << separator_;
    }

    if (!is_custom_type) {
      if (type_metadata.isBuiltinType() || type_metadata.isFunctionPointer()) {
        // no special treatment needed
        function_body_ << param_name;
      } else if (type_metadata.isEnum()) {
        function_body_ << "static_cast<" << namespace_with_colon
                       << type_metadata.getCastType() << ">(" << param_name
                       << ")";

        addDefinition(type_metadata.getCastType(),
                      type_metadata.getCDefinition());
      } else {
        if (type_metadata.isReference() || (type_metadata.isValueType())) {
          // if we have "void foo(Bar& baz)" or "void foo(Bar baz)"
          // then, when calling foo, the C wrapper needs to dereference the
          // value after it was reinterpret casted
          // see tests that check passing parameters by reference

          function_body_ << "*";
        }
        function_body_ << "reinterpret_cast<" << is_const_string
                       << namespace_with_colon << type_metadata.getCastType()
                       << "*"
                       << ">(" << param_name << ")";

        addDefinition(type_metadata.getCastType(),
                      type_metadata.getCDefinition());
      }
    }

    // after dealing with the first parameter, we always add comma
    separator_ = ", ";
  }
  function_name_ << ")";
  function_body_ << body_end_;
}

std::string FunctionConverter::getFunctionName() const {
  return function_name_.str();
}

std::string FunctionConverter::getFunctionBody() const {
  return function_body_.str();
}

std::map<std::string, std::string> FunctionConverter::getTypeDefinitions()
    const {
  return definitions_;
}

std::map<std::string, std::string>
FunctionConverter::getAdditionalImplementations() const {
  return additional_implementations_;
}

void FunctionConverter::addDefinition(std::string key, std::string value) {
  definitions_[key] = value;
}
void FunctionConverter::addImplementation(std::string key, std::string value) {
  additional_implementations_[key] = value;
}

std::string FunctionConverter::generateFunctionName() const {
  std::stringstream function_name;
  function_name << class_name_ << "_" << method_name_;

  // TODO: use mangled name/hash function
  auto it = kFuncList.find(function_name.str());
  if (it != kFuncList.end()) {
    it->second++;
    function_name << "_" << it->second;
  } else {
    kFuncList[function_name.str()] = 0;
  }

  function_name << "(";
  if (!method_decl_->isStatic()) {
    function_name << self_;
  }

  return function_name.str();
}

void FunctionConverter::generateFunctionCall() {
  std::string class_namespace_with_colon =
      cpp2c::utils::appendIfNotEmpty(class_namespace_, "::");

  // if static call it properly
  if (method_decl_->isStatic()) {
    separator_ = "";
    function_body_ << class_namespace_with_colon + class_name_
                   << "::" << method_name_ << "(";
  } else {
    // use the passed object to call the method
    function_body_ << "reinterpret_cast<"
                   << class_namespace_with_colon + class_name_ << "*>(self)->"
                   << method_name_ << "(";
  }

  // Note that the paramaters for the call are added later
  body_end_ += ")";
}

void FunctionConverter::generateConstructor() {
  std::string class_namespace_with_colon =
      cpp2c::utils::appendIfNotEmpty(class_namespace_, "::");

  method_name_ = "create";
  return_type_ = "W" + class_name_ + "*";
  self_ = "";
  separator_ = "";  // no "self param so no need to have comma as a separator
                    // the first time
  function_body_ << "return reinterpret_cast<" << return_type_ << ">( new "
                 << class_namespace_with_colon + class_name_ << "(";
  body_end_ += "))";
}

void FunctionConverter::generateDestructor() {
  std::string class_namespace_with_colon =
      cpp2c::utils::appendIfNotEmpty(class_namespace_, "::");

  method_name_ = "destroy";
  return_type_ = "void";
  function_body_ << "delete reinterpret_cast<"
                 << class_namespace_with_colon + class_name_ << "*>(self)";
}

void FunctionConverter::generateReturnValue() {
  method_name_ = method_decl_->getNameAsString();
  const clang::QualType return_type_qt = method_decl_->getReturnType();

  auto return_type_metadata =
      TypeConverter::createTypeMetadata(options_, return_type_qt);

  addDefinition(return_type_metadata.getCastType(),
                return_type_metadata.getCDefinition());

  return_type_ = return_type_metadata.getCType();
  if (return_type_metadata.isPointer() || return_type_metadata.isReference() ||
      return_type_metadata.isValueType()) {
    return_type_ += "*";
  }

  if (!return_type_metadata.isVoidType()) {
    function_body_ << "return ";
  }

  std::string is_const_string = return_type_metadata.isConst() ? "const " : "";
  return_type_ = is_const_string + return_type_;

  if (return_type_metadata.getCastType() != "") {
    if (return_type_metadata.isEnum()) {
      function_body_ << return_type_ << "(";
      body_end_ += ")";
    } else {
      function_body_ << "reinterpret_cast<" << return_type_ << ">(";
      body_end_ += ")";
    }
  }

  // dereference if needed
  if (return_type_metadata.isReference()) {
    function_body_ << "&";
  }

  auto customType = customTypes_.match(return_type_metadata);
  const bool is_custom_type = customType != nullptr;

  if (is_custom_type) {
    customType->addAsReturnType(return_type_metadata, this);
  } else {
    std::string class_namespace_with_colon =
        cpp2c::utils::appendIfNotEmpty(class_namespace_, "::");

    // before we add the call to the function (and the parameters),
    // we need to check if we return by value
    // also note enums get special treatment
    if (return_type_metadata.isValueType()) {
      // if it is return by value we need to allocate on the heap
      // TODO: we assume here there is a (non-deleted deep-copy) copy
      // constructor
      function_body_ << "new " << class_namespace_with_colon
                     << return_type_metadata.getCastType() << "(";

      body_end_ += ")";
    } else if (!return_type_metadata.isReference() &&
               !return_type_metadata.isPointer() &&
               return_type_metadata.isEnum()) {
      std::string namespace_with_colon = cpp2c::utils::appendIfNotEmpty(
          return_type_metadata.getNamespace(), "::");

      function_body_ << "static_cast<typename std::underlying_type<"
                     << namespace_with_colon
                     << return_type_metadata.getCastType() << ">::type>(";
      body_end_ += ")";
    }
  }
}

}  // namespace cpp2c
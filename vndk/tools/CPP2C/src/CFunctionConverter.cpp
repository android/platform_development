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

#include "CFunctionConverter.h"

#include <map>

namespace cpp2c {

extern std::map<std::string, int> kFuncList;

CFunctionConverter::CFunctionConverter(const clang::CXXMethodDecl* c,
                                       OutputStreams& o)
    : method_decl_(c) {
  class_name_ = method_decl_->getParent()->getDeclName().getAsString();
  self_ = "W" + class_name_ + "* self";
  separator_ = ", ";
}

void CFunctionConverter::run(const clang::ASTContext* c, const Resources* cl) {
  CTypeMetadataConverter type_converter;

  namespace_prefix_ = retrieveNamespace();

  // ignore operator overloadings
  if (method_decl_->isOverloadedOperator()) {
    return;
  }

  // create the function body (note: the parameters are added later)
  if (const clang::CXXConstructorDecl* ccd =
          llvm::dyn_cast<clang::CXXConstructorDecl>(method_decl_)) {
    if (ccd->isCopyConstructor() || ccd->isMoveConstructor()) {
      return;
    }
    retrieveAsConstructor();
  } else if (llvm::isa<clang::CXXDestructorDecl>(method_decl_)) {
    retrieveAsDestructor();
  } else {
    retrieveAsFunction(type_converter);
  }

  function_name_ = createFunctionName(return_type_, class_name_, method_name_);

  // add the parameters for both the function signature and the function call
  // in the body
  for (unsigned int i = 0; i < method_decl_->getNumParams(); ++i) {
    const clang::ParmVarDecl* param = method_decl_->parameters()[i];
    const clang::QualType parameter_qt = param->getType();

    CTypeMetadata c_type_metadata = type_converter.determineCType(parameter_qt);
    std::string is_const_string = c_type_metadata.isConst() ? "const " : "";

    // in the header, the name is optional, only the type is mandatory
    const std::string param_name = param->getQualifiedNameAsString().size() != 0
                                       ? param->getQualifiedNameAsString()
                                       : ("param" + std::to_string(i));

    // for the function name, separator needs to be "" initially if constr or
    // static, otherwise it's ", " because we always have self as a param
    function_name_ << separator_ << is_const_string << namespace_prefix_
                   << c_type_metadata.getCTypeWithName(param_name) << "";

    // for the function body, add nothing before first param and ", " before
    // the others
    if (i != 0) {
      function_body_ << separator_;
    }

    if (c_type_metadata.getCastType().size() == 0) {
      function_body_ << namespace_prefix_ << param_name;
    } else {
      if (c_type_metadata.isReference()) {
        // as the parameter is a pointer in C but a reference in C++,
        // dereference it
        function_body_ << "*";
      }
      function_body_ << "reinterpret_cast<" << is_const_string
                     << namespace_prefix_ << c_type_metadata.getCastType()
                     << ">(" << param_name << ")";
    }

    // after dealing with the first parameter, we always add comma
    std::string separator = ", ";
  }
  function_name_ << ")";
  function_body_ << body_end_;
}

std::string CFunctionConverter::getFunctionName() {
  return function_name_.str();
}

std::string CFunctionConverter::getFunctionBody() {
  return function_body_.str();
}

std::string CFunctionConverter::retrieveNamespace() const {
  const clang::DeclContext* context =
      method_decl_->getParent()->getEnclosingNamespaceContext();
  if (context->isNamespace()) {
    const auto namespace_context =
        llvm::dyn_cast<clang::NamespaceDecl>(context);
    std::string namespace_name = namespace_context->getDeclName().getAsString();
    return (namespace_name.size() != 0 ? (namespace_name + "::") : "");
  }

  return "";
}

std::stringstream CFunctionConverter::createFunctionName(
    const std::string& return_type, const std::string& class_name,
    const std::string& method_name) const {
  // TODO: use mangled name?
  std::stringstream function_name;
  function_name << return_type << " " << class_name << "_" << method_name;

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

  return function_name;
}

void CFunctionConverter::addFunctionCall() {
  // if static call it properly
  if (method_decl_->isStatic()) {
    separator_ = "";
    function_body_ << namespace_prefix_ + class_name_ << "::" << method_name_
                   << "(";
  } else {
    // use the passed object to call the method
    function_body_ << "reinterpret_cast<" << namespace_prefix_ + class_name_
                   << "*>(self)->" << method_name_ << "(";
  }

  // Note that the paramaters for the call are added later
  body_end_ += ")";
}

void CFunctionConverter::checkIfValueType(
    const CTypeMetadata& c_type_metadata) {
  // before we add the call to the function (and the parameters),
  // we need to check if we return by value
  if (!c_type_metadata.isReference() && !c_type_metadata.isPointer()) {
    // if it is return by value we need to allocate on the heap
    // TODO: we assume here there is a (non-deleted deep-copy) copy
    // constructor
    function_body_ << "new " << namespace_prefix_
                   << c_type_metadata.getCastType() << "(";
    body_end_ += ")";
  }
}

void CFunctionConverter::retrieveAsConstructor() {
  method_name_ = "create";
  return_type_ = "W" + class_name_ +
                 "*";  // TODO only wrap if the class name is in kClassList
  self_ = "";
  separator_ = "";
  function_body_ << "return reinterpret_cast<" << return_type_ << ">( new "
                 << namespace_prefix_ + class_name_ << "(";
  body_end_ += "))";
}

void CFunctionConverter::retrieveAsDestructor() {
  method_name_ = "destroy";
  return_type_ = "void";
  function_body_ << "delete reinterpret_cast<"
                 << namespace_prefix_ + class_name_ << "*>(self)";
}

void CFunctionConverter::retrieveAsFunction(
    CTypeMetadataConverter& type_converter) {
  method_name_ = method_decl_->getNameAsString();
  const clang::QualType return_type_qt = method_decl_->getReturnType();
  CTypeMetadata c_return_type_metadata =
      type_converter.determineCType(return_type_qt);
  return_type_ = c_return_type_metadata.getCType();

  if (!c_return_type_metadata.isVoidType()) {
    function_body_ << "return ";
  }

  // add necessary casts
  if (c_return_type_metadata.isReference()) {
    function_body_ << "&";
  }

  if (c_return_type_metadata.getCastType() != "") {
    function_body_ << "reinterpret_cast<" << return_type_ << ">(";
    body_end_ += ")";
  }

  checkIfValueType(c_return_type_metadata);

  addFunctionCall();
}

}  // namespace cpp2c
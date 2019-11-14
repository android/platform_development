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

#include <iostream>
#include <map>

#include "Utils.h"

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

  class_namespace_ = cpp2c::getNamespaceFromContext(
      method_decl_->getEnclosingNamespaceContext());

  // ignore operator overloadings
  if (method_decl_->isOverloadedOperator()) {
    return;
  }

  definitions_[class_name_] = cpp2c::createStructDefinition(class_name_);

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

    std::string namespace_with_colon =
        cpp2c::appendIfNotEmpty(c_type_metadata.getNamespace(), "::");

    std::string c_type_with_name =
        c_type_metadata.createCTypeWithName(param_name);

    const bool is_smart_ptr = isSmartPointer(c_type_metadata);

    if (is_smart_ptr) {
      const std::vector<const CTypeMetadata> template_args =
          c_type_metadata.getTemplateArgs();

      const CTypeMetadata template_arg_0 =
          template_args[0];  // TODO add support for multiple template
                             // arguments

      if (isUniquePointer(c_type_metadata)) {
        c_type_with_name = template_arg_0.createCTypeWithName(param_name);

        std::string template_arg_namespace_with_colon =
            cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

        function_body_ << "std::move(std::unique_ptr<"
                       << template_arg_namespace_with_colon
                       << template_arg_0.getCastType() << ">(";

        function_body_ << "reinterpret_cast<" << is_const_string
                       << template_arg_namespace_with_colon
                       << template_arg_0.getCastType() << "*"
                       << ">(" << param_name << ")";

        function_body_ << "))";

        definitions_[template_arg_0.getCastType()] =
            template_arg_0.getCDefinition();

      } else if (isSharedPointer(c_type_metadata)) {
        std::string namespace_with_colon =
            cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

        std::string type = template_arg_0.getCType() + "_shared";

        c_type_with_name = type + "* " + param_name;

        function_body_ << "*reinterpret_cast<std::shared_ptr<"
                       << namespace_with_colon << template_arg_0.getCastType()
                       << ">*>(" << param_name << ")";

        definitions_[type] =
            cpp2c::createSharedPointerDeclarations(c_type_metadata);
        additional_implementations_[type] =
            cpp2c::createSharedPointerImplementation(c_type_metadata);

      } else {
        std::cout << "Unsupported smart pointer!" << std::endl;
      }
    }

    // for the function name, separator needs to be "" initially if constr
    // or static, otherwise it's ", " because we always have self as a param
    function_name_ << separator_ << is_const_string << c_type_with_name << "";

    // for the function body, add nothing before first param and ", " before
    // the others
    if (i != 0) {
      function_body_ << separator_;
    }

    if (!is_smart_ptr) {
      if (c_type_metadata.getCastType().size() == 0) {
        function_body_ << param_name;
      } else if (c_type_metadata.isEnum()) {
        function_body_ << "static_cast<" << namespace_with_colon
                       << c_type_metadata.getCastType() << ">(" << param_name
                       << ")";
      } else {
        if (c_type_metadata.isReference() ||
            (!c_type_metadata.isReference() && !c_type_metadata.isPointer() &&
             !c_type_metadata.isEnum())) {
          // if the parameter is a pointer in C but a reference in C++,
          // or if the parameter is a value type in C++ but we have a pointer
          // then dereference it
          function_body_ << "*";
        }
        function_body_ << "reinterpret_cast<" << is_const_string
                       << namespace_with_colon << c_type_metadata.getCastType()
                       << "*"
                       << ">(" << param_name << ")";

        definitions_[c_type_metadata.getCastType()] =
            c_type_metadata.getCDefinition();
      }
    }

    // after dealing with the first parameter, we always add comma
    std::string separator = ", ";
  }
  function_name_ << ")";
  function_body_ << body_end_;
}

std::string CFunctionConverter::getFunctionName() const {
  return function_name_.str();
}

std::string CFunctionConverter::getFunctionBody() const {
  return function_body_.str();
}

std::map<std::string, std::string> CFunctionConverter::getTypeDefinitions()
    const {
  return definitions_;
}

std::map<std::string, std::string>
CFunctionConverter::getAdditionalImplementations() const {
  return additional_implementations_;
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

void CFunctionConverter::addFunctionCall(
    const CTypeMetadata* c_return_type_metadata) {
  std::string class_namespace_with_colon =
      cpp2c::appendIfNotEmpty(class_namespace_, "::");

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

void CFunctionConverter::checkIfValueType(
    const CTypeMetadata& c_type_metadata) {
  std::string class_namespace_with_colon =
      cpp2c::appendIfNotEmpty(class_namespace_, "::");

  // before we add the call to the function (and the parameters),
  // we need to check if we return by value
  // also note enums get special treatment
  if (!c_type_metadata.isReference() && !c_type_metadata.isPointer() &&
      !c_type_metadata.isEnum() && !c_type_metadata.isBuiltinType()) {
    // if it is return by value we need to allocate on the heap
    // TODO: we assume here there is a (non-deleted deep-copy) copy
    // constructor
    function_body_ << "new " << class_namespace_with_colon
                   << c_type_metadata.getCastType() << "(";

    body_end_ += ")";
  } else if (!c_type_metadata.isReference() && !c_type_metadata.isPointer() &&
             c_type_metadata.isEnum()) {
    std::string namespace_with_colon =
        cpp2c::appendIfNotEmpty(c_type_metadata.getNamespace(), "::");

    function_body_ << "static_cast<typename std::underlying_type<"
                   << namespace_with_colon << c_type_metadata.getCastType()
                   << ">::type>(";
    body_end_ += ")";
  }
}

void CFunctionConverter::retrieveAsConstructor() {
  std::string class_namespace_with_colon =
      cpp2c::appendIfNotEmpty(class_namespace_, "::");

  method_name_ = "create";
  return_type_ = "W" + class_name_ + "*";
  self_ = "";
  separator_ = "";
  function_body_ << "return reinterpret_cast<" << return_type_ << ">( new "
                 << class_namespace_with_colon + class_name_ << "(";
  body_end_ += "))";
}

void CFunctionConverter::retrieveAsDestructor() {
  std::string class_namespace_with_colon =
      cpp2c::appendIfNotEmpty(class_namespace_, "::");

  method_name_ = "destroy";
  return_type_ = "void";
  function_body_ << "delete reinterpret_cast<"
                 << class_namespace_with_colon + class_name_ << "*>(self)";
}

void CFunctionConverter::retrieveAsFunction(
    CTypeMetadataConverter& type_converter) {
  method_name_ = method_decl_->getNameAsString();
  const clang::QualType return_type_qt = method_decl_->getReturnType();

  CTypeMetadata c_return_type_metadata =
      type_converter.determineCType(return_type_qt);

  definitions_[c_return_type_metadata.getCastType()] =
      c_return_type_metadata.getCDefinition();

  return_type_ = c_return_type_metadata.createCType();

  if (!c_return_type_metadata.isVoidType()) {
    function_body_ << "return ";
  }

  if (c_return_type_metadata.getCastType() != "") {
    if (c_return_type_metadata.isEnum()) {
      function_body_ << return_type_ << "(";
      body_end_ += ")";
    } else {
      function_body_ << "reinterpret_cast<" << return_type_ << ">(";
      body_end_ += ")";
    }
  }

  // add necessary casts
  if (c_return_type_metadata.isReference()) {
    function_body_ << "&";
  }

  const bool is_smart_ptr = isSmartPointer(c_return_type_metadata);

  if (is_smart_ptr) {
    const std::vector<const CTypeMetadata> template_args =
        c_return_type_metadata.getTemplateArgs();

    const CTypeMetadata template_arg_0 = template_args[0];

    if (isUniquePointer(c_return_type_metadata)) {
      return_type_ = template_arg_0.createCType();

      if (template_arg_0.getCastType().size() != 0) {
        function_body_ << "reinterpret_cast<" << return_type_ << ">(";
        body_end_ += ")";
      }

      body_end_ += ".release()";
    } else if (isSharedPointer(c_return_type_metadata)) {
      std::string namespace_with_colon =
          cpp2c::appendIfNotEmpty(template_arg_0.getNamespace(), "::");

      return_type_ = template_arg_0.getCType() + "_shared*";

      if (template_arg_0.getCastType().size() != 0) {
        function_body_ << "reinterpret_cast<" << return_type_ << ">(";
        body_end_ += ")";
      }

      function_body_ << "new std::shared_ptr<" << namespace_with_colon
                     << template_arg_0.getCastType() << ">(";
      body_end_ += ")";

      definitions_[return_type_] =
          cpp2c::createSharedPointerDeclarations(c_return_type_metadata);
      additional_implementations_[template_arg_0.getCType() + "_shared"] =
          cpp2c::createSharedPointerImplementation(c_return_type_metadata);

    } else {
      std::cout << "Unsupported smart pointer!" << std::endl;
    }
  }

  // don't do this check for smart pointers
  if (!is_smart_ptr) {
    checkIfValueType(c_return_type_metadata);
  }

  addFunctionCall(&c_return_type_metadata);
}

}  // namespace cpp2c
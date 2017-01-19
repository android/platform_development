// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "abi_wrappers.h"

#include <clang/Lex/Token.h>
#include <clang/Tooling/Core/QualTypeNames.h>

#include <google/protobuf/text_format.h>

#include <fstream>
#include <iostream>
#include <string>

ABIWrapper::ABIWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const std::string &current_file_name)
  : mangle_contextp_(mangle_contextp),
    ast_contextp_(ast_contextp),
    cip_(compiler_instance_p),
    current_file_name_(current_file_name) { }

const std::string &ABIWrapper::GetCurrentFile() const {
  return current_file_name_;
}

std::string ABIWrapper::GetDeclSourceFile(const clang::NamedDecl *decl) {
  clang::SourceManager &SM = cip_->getSourceManager();
  clang::SourceLocation location = decl->getLocation();
  llvm::StringRef file_name= SM.getFilename(location);
  return file_name.str();
}

std::string ABIWrapper::AccessToString(const clang::AccessSpecifier sp) {
  std::string str = "none";
  switch (sp) {
    case clang::AS_public:
      str = "public";
      break;
    case clang::AS_private:
      str = "private";
      break;
    case clang::AS_protected:
      str = "protected";
      break;
    default:
      break;
  }
  return str;
}

std::string ABIWrapper::GetMangledNameDecl(const clang::NamedDecl *decl) {
  std::string mangled_or_demangled_name = decl->getName();
  if (mangle_contextp_->shouldMangleDeclName(decl)) {
    llvm::raw_string_ostream ostream(mangled_or_demangled_name);
    mangle_contextp_->mangleName(decl, ostream);
    ostream.flush();
  }
  return mangled_or_demangled_name;
}

FunctionDeclWrapper::FunctionDeclWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const std::string &current_file_name,
    const clang::FunctionDecl *decl)
  :  ABIWrapper(mangle_contextp,
               ast_contextp,
               compiler_instance_p,
               current_file_name), function_decl_(decl) { }

//FIXME: dump default nature of parameters as well.
bool FunctionDeclWrapper::SetupFunction(abi_dump::FunctionDecl *functionp,
                                     const clang::FunctionDecl *decl,
                                     const std::string &source_file) {
  // Go through all the parameters in the method and add them to the fields.
  // Also get the fully qualfied name and mangled name and store them.
  functionp->set_function_name(decl->getQualifiedNameAsString());
  functionp->set_mangled_function_name(GetMangledNameDecl(decl));
  functionp->set_source_file(source_file);
  clang::QualType return_type =
      decl->getReturnType().getDesugaredType(*ast_contextp_);
  functionp->set_return_type(
      clang::TypeName::getFullyQualifiedName(return_type, *ast_contextp_));
  clang::FunctionDecl::param_const_iterator param_it = decl->param_begin();
  while (param_it != decl->param_end()) {
    abi_dump::FieldDecl *function_fieldp = functionp->add_parameters();
    if (!function_fieldp) {
      llvm::errs() << "Couldn't add parameter to method. Aborting\n";
      return false;
    }
    function_fieldp->set_field_name((*param_it)->getName());
    clang::QualType field_type =
      (*param_it)->getType().getDesugaredType(*ast_contextp_);

    function_fieldp->set_field_type(
        clang::TypeName::getFullyQualifiedName(field_type, *ast_contextp_));
    param_it++;
  }
  functionp->set_access(AccessToString(decl->getAccess()));
  functionp->set_template_kind(decl->getTemplatedKind());
  // Go through template parameters
  // FIXME: Re-write this, maybe a common function.
  clang::FunctionTemplateDecl *template_decl =
      decl->getDescribedFunctionTemplate();
  if (template_decl) {
    clang::TemplateParameterList *template_parameter_list =
        template_decl->getTemplateParameters();
    if (template_parameter_list) {
      clang::TemplateParameterList::iterator template_it =
          template_parameter_list->begin();
      while (template_it != template_parameter_list->end()) {
        abi_dump::FieldDecl *function_template_parameterp =
            functionp->add_template_parameters();
        function_template_parameterp->set_field_name((*template_it)->getName());
        template_it++;
      }
    }
  }
  return true;
}

bool FunctionDeclWrapper::GetWrappedABI() {
  std::string source_file = GetDeclSourceFile(function_decl_);
  if (source_file != GetCurrentFile())
   return true;
  if (!SetupFunction(&abi_function_decl_, function_decl_, source_file)) {
    return false;
  }
  return true;
}

const abi_dump::FunctionDecl &FunctionDeclWrapper::GetFunctionDecl() const {
  return abi_function_decl_;
}

RecordDeclWrapper::RecordDeclWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const std::string &current_file_name,
    const clang::CXXRecordDecl *decl)
  :  ABIWrapper(mangle_contextp,
               ast_contextp,
               compiler_instance_p,
               current_file_name), record_decl_(decl) { }

bool RecordDeclWrapper::SetupClassFields(abi_dump::RecordDecl *classp,
                                        const clang::CXXRecordDecl *decl,
                                        const std::string &source_file) {
  classp->set_fully_qualified_name(decl->getQualifiedNameAsString());
  classp->set_source_file(source_file);
  classp->set_entity_type("class");
  clang::RecordDecl::field_iterator field = decl->field_begin();
  while (field != decl->field_end()) {
    abi_dump::FieldDecl *class_fieldp = classp->add_fields();
    if (!class_fieldp) {
      llvm::errs() << " Couldn't add class field: " << field->getName()
                   << " to reference dump\n";
      return false;
    }
    class_fieldp->set_field_name(field->getName());
    clang::QualType field_type =
        field->getType().getDesugaredType(*ast_contextp_);
    class_fieldp->set_field_type(
        clang::TypeName::getFullyQualifiedName(field_type, *ast_contextp_));
    class_fieldp->set_access(AccessToString(field->getAccess()));
    field++;
  }
  return true;
}

bool RecordDeclWrapper::SetupClassBases(abi_dump::RecordDecl *classp,
                                        const clang::CXXRecordDecl *decl) {
  clang::CXXRecordDecl::base_class_const_iterator base_class =
      decl->bases_begin();
  while (base_class != decl->bases_end()) {
    abi_dump::CXXBaseSpecifier *base_specifierp = classp->add_base_specifiers();
    if (!base_specifierp) {
      llvm::errs() << " Couldn't add base specifier to reference dump\n";
      return false;
    }
    //TODO: Make this pair into a function, used accross.
    clang::QualType base_type =
        base_class->getType().getDesugaredType(*ast_contextp_);
    base_specifierp->set_fully_qualified_name(
        clang::TypeName::getFullyQualifiedName(base_type, *ast_contextp_));
    base_specifierp->set_is_virtual(base_class->isVirtual());
    base_specifierp->set_access(
        AccessToString(base_class->getAccessSpecifier()));
    base_class++;
  }
  return true;
}

bool RecordDeclWrapper::GetWrappedABI() {
 std::string source_file = GetDeclSourceFile(record_decl_);
 if (source_file != GetCurrentFile())
   return true;
  if (!SetupClassFields(&abi_record_decl_, record_decl_, source_file) ||
      !SetupClassBases(&abi_record_decl_, record_decl_)) {
    return false;
  }
  return true;
}

const abi_dump::RecordDecl &RecordDeclWrapper::GetRecordDecl() const {
  return abi_record_decl_;
}

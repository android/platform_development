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

#include <clang/Tooling/Core/QualTypeNames.h>

#include <string>

ABIWrapper::ABIWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p)
  : mangle_contextp_(mangle_contextp),
    ast_contextp_(ast_contextp),
    cip_(compiler_instance_p) {}

std::string ABIWrapper::GetDeclSourceFile(const clang::NamedDecl *decl) const {
  clang::SourceManager &SM = cip_->getSourceManager();
  clang::SourceLocation location = decl->getLocation();
  llvm::StringRef file_name= SM.getFilename(location);
  return file_name.str();
}

std::string ABIWrapper::AccessToString(const clang::AccessSpecifier sp) const {
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

std::string ABIWrapper::GetMangledNameDecl(const clang::NamedDecl *decl) const {
  std::string mangled_or_demangled_name = decl->getName();
  if (mangle_contextp_->shouldMangleDeclName(decl)) {
    llvm::raw_string_ostream ostream(mangled_or_demangled_name);
    mangle_contextp_->mangleName(decl, ostream);
    ostream.flush();
  }
  return mangled_or_demangled_name;
}
//FIXME : indentation.
bool ABIWrapper::SetupTemplateParamNames(abi_dump::TemplateInfo *tinfo,
                                         clang::TemplateParameterList *pl) const {
 if (tinfo->template_parameters_size() > 0)
   return true;

 clang::TemplateParameterList::iterator template_it = pl->begin();
  while (template_it != pl->end()) {
    abi_dump::FieldDecl *template_parameterp =
        tinfo->add_template_parameters();
    if (!template_parameterp)
      return false;
    template_parameterp->set_field_name((*template_it)->getName());
    template_it++;
  }
  return true;
}

//FIXME : indentation.
bool ABIWrapper::SetupTemplateArguments(abi_dump::TemplateInfo *tinfo,
                                        const clang::TemplateArgumentList *tl) const {
  for (int i = 0; i < tl->size(); i++) {
    const clang::TemplateArgument &arg = (*tl)[i];
    std::string type = QualTypeToString(arg.getAsType());
    abi_dump::FieldDecl *template_parameterp =
        tinfo->add_template_parameters();
    if (!template_parameterp)
      return false;
    template_parameterp->set_field_type((type));
  }
  return true;
}

//FIXME : indentation.
std::string ABIWrapper::QualTypeToString(const clang::QualType &sweet_qt) const {
  const clang::QualType salty_qt = sweet_qt.getDesugaredType(*ast_contextp_);
  return clang::TypeName::getFullyQualifiedName(salty_qt, *ast_contextp_);
}

FunctionDeclWrapper::FunctionDeclWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::FunctionDecl *decl)
  : ABIWrapper(mangle_contextp,
               ast_contextp,
               compiler_instance_p), function_decl_(decl) { }

bool FunctionDeclWrapper::SetupFunction(abi_dump::FunctionDecl *functionp,
                                     const clang::FunctionDecl *decl,
                                     const std::string &source_file) const {
  // Go through all the parameters in the method and add them to the fields.
  // Also get the fully qualfied name and mangled name and store them.
  functionp->set_function_name(decl->getQualifiedNameAsString());
  functionp->set_mangled_function_name(GetMangledNameDecl(decl));
  functionp->set_source_file(source_file);
  functionp->set_return_type(QualTypeToString(decl->getReturnType()));

  clang::FunctionDecl::param_const_iterator param_it = decl->param_begin();
  while (param_it != decl->param_end()) {
    abi_dump::FieldDecl *function_fieldp = functionp->add_parameters();
    if (!function_fieldp) {
      llvm::errs() << "Couldn't add parameter to method. Aborting\n";
      return false;
    }
    function_fieldp->set_field_name((*param_it)->getName());
    function_fieldp->set_default_arg((*param_it)->hasDefaultArg());
    function_fieldp->set_field_type(QualTypeToString((*param_it)->getType()));
    param_it++;
  }
  functionp->set_access(AccessToString(decl->getAccess()));
  functionp->set_template_kind(decl->getTemplatedKind());
  if(!SetupTemplateInfo(functionp, decl)) {
    return false;
  }
  return true;
}

//FIXME: Indentation
bool FunctionDeclWrapper::SetupTemplateInfo(abi_dump::FunctionDecl *functionp,
                                            const clang::FunctionDecl *decl) const {
  switch (decl->getTemplatedKind()) {
    case clang::FunctionDecl::TK_FunctionTemplate:
      {
        clang::FunctionTemplateDecl *template_decl =
            decl->getDescribedFunctionTemplate();
        if (template_decl) {
          clang::TemplateParameterList *template_parameter_list =
              template_decl->getTemplateParameters();
          if (template_parameter_list &&
              !SetupTemplateParamNames(functionp->mutable_template_info(),
                                       template_parameter_list)) {
            return false;
          }
        }
        break;
      }
    case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      {
        const clang::TemplateArgumentList *arg_list =
            decl->getTemplateSpecializationArgs();
        if (arg_list &&
            !SetupTemplateArguments(
                functionp->mutable_template_info(), arg_list)) {
          return false;
        }
        break;
      }
      default:
        break;
  }
  return true;
}

//FIXME: Indentation
std::unique_ptr<abi_dump::FunctionDecl> FunctionDeclWrapper::GetFunctionDecl() const {
  std::unique_ptr<abi_dump::FunctionDecl> abi_decl(
      new abi_dump::FunctionDecl());
  std::string source_file = GetDeclSourceFile(function_decl_);
  if (!SetupFunction(abi_decl.get(), function_decl_, source_file)) {
    return nullptr;
  }
  return abi_decl;
}

RecordDeclWrapper::RecordDeclWrapper(
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::RecordDecl *decl)
  : ABIWrapper(mangle_contextp,
               ast_contextp,
               compiler_instance_p), record_decl_(decl) { }

bool RecordDeclWrapper::SetupClassFields(abi_dump::RecordDecl *classp,
                                        const clang::RecordDecl *decl,
                                        const std::string &source_file) const {
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
    class_fieldp->set_field_type(QualTypeToString(field->getType()));
    class_fieldp->set_access(AccessToString(field->getAccess()));
    field++;
  }
  return true;
}

//FIXME : indentation.
bool RecordDeclWrapper::SetupClassBases(abi_dump::RecordDecl *classp,
                                        const clang::CXXRecordDecl *decl) const {
  clang::CXXRecordDecl::base_class_const_iterator base_class =
      decl->bases_begin();
  while (base_class != decl->bases_end()) {
    abi_dump::CXXBaseSpecifier *base_specifierp = classp->add_base_specifiers();
    if (!base_specifierp) {
      llvm::errs() << " Couldn't add base specifier to reference dump\n";
      return false;
    }
    base_specifierp->set_fully_qualified_name(
        QualTypeToString(base_class->getType()));
    base_specifierp->set_is_virtual(base_class->isVirtual());
    base_specifierp->set_access(
        AccessToString(base_class->getAccessSpecifier()));
    base_class++;
  }
  return true;
}

//FIXME : indentation.
//FIXME: switch
//FIXME: return bool.
void RecordDeclWrapper::SetupTemplateInfo(abi_dump::RecordDecl *record_declp,
                                          const clang::CXXRecordDecl *decl) const {
  clang::ClassTemplateDecl *template_decl =
      decl->getDescribedClassTemplate();
  if (template_decl) {
    clang::TemplateParameterList *template_parameter_list =
        template_decl->getTemplateParameters();
  if (template_parameter_list) {
    SetupTemplateParamNames(record_declp->mutable_template_info(),
                            template_parameter_list);
    }
  }
  const clang::ClassTemplateSpecializationDecl *specialization_decl =
      clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl);

  if(!specialization_decl) {
    return;
  }

  const clang::TemplateArgumentList *arg_list =
      &(specialization_decl->getTemplateArgs());
  if (!arg_list)
    return;
  SetupTemplateArguments(record_declp->mutable_template_info(), arg_list);
}

std::unique_ptr<abi_dump::RecordDecl> RecordDeclWrapper::GetRecordDecl() const {
  std::unique_ptr<abi_dump::RecordDecl> abi_decl(new abi_dump::RecordDecl());
  std::string source_file = GetDeclSourceFile(record_decl_);
  if (!SetupClassFields(abi_decl.get(), record_decl_, source_file)) {
    llvm::errs() << "Setting up Class Fields failed\n";
    return nullptr;
  }
  const clang::CXXRecordDecl *cxx_record_decl =
      clang::dyn_cast<clang::CXXRecordDecl>(record_decl_);
  if (!cxx_record_decl)
    return abi_decl;

  if (!SetupClassBases(abi_decl.get(), cxx_record_decl)) {
    llvm::errs() << "Setting up Class Basess failed\n";
    return nullptr;
  }
  //FIXME: use return value;
  SetupTemplateInfo(abi_decl.get(), cxx_record_decl);
  return abi_decl;
}

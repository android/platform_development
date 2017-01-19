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

#include "ast_processing.h"
#include "abi_wrappers.h"

#include <clang/Lex/Token.h>
#include <clang/Tooling/Core/QualTypeNames.h>

#include <google/protobuf/text_format.h>

#include <fstream>
#include <iostream>
#include <string>

HeaderASTVisitor::HeaderASTVisitor(
    abi_dump::TranslationUnit *tu_ptr,
    clang::MangleContext *mangle_contextp,
    const clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const std::string &current_file_name)
  : tu_ptr_(tu_ptr),
    mangle_contextp_(mangle_contextp),
    ast_contextp_(ast_contextp),
    cip_(compiler_instance_p),
    current_file_name_(current_file_name) { }

// TODO: optimize source file initial check by preferably moving this into
// TraverseTranslationUnitDecl.
bool HeaderASTVisitor::VisitCXXRecordDecl(const clang::CXXRecordDecl *decl) {
  RecordDeclWrapper record_decl_wrapper(mangle_contextp_,
                                        ast_contextp_,
                                        cip_,
                                        current_file_name_,
                                        decl);
  if (!record_decl_wrapper.GetWrappedABI()) {
    return false;
  }
  abi_dump::RecordDecl *record_declp = tu_ptr_->add_classes();
  *record_declp = record_decl_wrapper.GetRecordDecl();
  return true;
}

bool HeaderASTVisitor::VisitClassTemplateDecl(
    const clang::ClassTemplateDecl *decl) {
  llvm::errs() << "Template Decl name: " << decl->getName() << "\n";
  return true;
}

//FIXME: Take this out.
// Enable recursive traversal of template instantiations.
bool HeaderASTVisitor::shouldVisitTemplateInstantiations() {
  return true;
}

bool HeaderASTVisitor::VisitClassTemplateSpecializationDecl(
    const clang::ClassTemplateSpecializationDecl *decl) {
  llvm::errs() << "TemplateSpecializationDecl name: " << decl->getName() << "\n";
  return true;
}

bool HeaderASTVisitor::VisitFunctionDecl(const clang::FunctionDecl *decl) {
  FunctionDeclWrapper function_decl_wrapper(mangle_contextp_,
                                            ast_contextp_,
                                            cip_,
                                            current_file_name_,
                                            decl);
  if (!function_decl_wrapper.GetWrappedABI())
    return false;
  abi_dump::FunctionDecl *function_declp = tu_ptr_->add_functions();
  *function_declp = function_decl_wrapper.GetFunctionDecl();
  return true;
}

HeaderASTConsumer::HeaderASTConsumer(
    const std::string &file_name,
    clang::CompilerInstance *compiler_instancep,
    const std::string &out_dump_name)
  : file_name_(file_name),
    cip_(compiler_instancep),
    out_dump_name_(out_dump_name) { }

void HeaderASTConsumer::HandleTranslationUnit(clang::ASTContext &ctx) {
  clang::TranslationUnitDecl* translation_unit = ctx.getTranslationUnitDecl();
  std::unique_ptr<clang::MangleContext> mangle_contextp(
      ctx.createMangleContext());
  abi_dump::TranslationUnit tu;
  HeaderASTVisitor v(&tu, mangle_contextp.get(), &ctx, cip_, file_name_);
  v.TraverseDecl(translation_unit);
  std::ofstream text_output(out_dump_name_ + ".txt");
  std::fstream binary_output(
      (out_dump_name_).c_str(),
      std::ios::out | std::ios::trunc | std::ios::binary);
  std::string str_out;
  google::protobuf::TextFormat::PrintToString(tu, &str_out);
  text_output << str_out;
  if (!tu.SerializeToOstream(&binary_output)) {
    llvm::errs() << "Serialization to ostream failed\n";
  }
}

void HeaderASTConsumer::HandleVTable(clang::CXXRecordDecl *crd) {
  llvm::errs() << "HandleVTable: " << crd->getName() << "\n";
}

void HeaderASTPPCallbacks::MacroDefined(const clang::Token &macro_name_tok,
                                        const clang::MacroDirective *) {
  assert(macro_name_tok.isAnyIdentifier());
}

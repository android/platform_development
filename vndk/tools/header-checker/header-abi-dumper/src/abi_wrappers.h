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

#ifndef ABI_WRAPPERS_H_
#define ABI_WRAPPERS_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "proto/abi_dump.pb.h"
#pragma clang diagnostic pop

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PPCallbacks.h>

class ABIWrapper {
 public:
  ABIWrapper(clang::MangleContext *mangle_contextp,
             const clang::ASTContext *ast_contextp,
             const clang::CompilerInstance *compiler_instance_p,
             const std::string &current_file_name);

  const std::string &GetCurrentFile() const;
  virtual bool GetWrappedABI() = 0;

  std::string GetDeclSourceFile(const clang::NamedDecl *decl);

  std::string AccessToString(const clang::AccessSpecifier sp);

  std::string GetMangledNameDecl(const clang::NamedDecl *decl);

 private:
  //abi_dump::TranslationUnit *tu_ptr_;
  clang::MangleContext *mangle_contextp_;
 public:
  //FIXME: Make this private.
  const clang::ASTContext *ast_contextp_;
 private:
  const clang::CompilerInstance *cip_;
  const std::string current_file_name_;
};

class RecordDeclWrapper : public ABIWrapper {
 public:
  RecordDeclWrapper(clang::MangleContext *mangle_contextp,
             const clang::ASTContext *ast_contextp,
             const clang::CompilerInstance *compiler_instance_p,
             const std::string &current_file_name,
             const clang::CXXRecordDecl *decl);

  const abi_dump::RecordDecl &GetRecordDecl() const;
  virtual bool GetWrappedABI() override;

 private:
  abi_dump::RecordDecl abi_record_decl_;
  const clang::CXXRecordDecl *record_decl_;

 private:
  bool SetupClassFields(abi_dump::RecordDecl *classp,
                        const clang::CXXRecordDecl *decl,
                        const std::string &source_file);

  bool SetupClassBases(abi_dump::RecordDecl *classp,
                      const clang::CXXRecordDecl *decl);

};

class FunctionDeclWrapper : public ABIWrapper {
 public:
  FunctionDeclWrapper(clang::MangleContext *mangle_contextp,
                      const clang::ASTContext *ast_contextp,
                      const clang::CompilerInstance *compiler_instance_p,
                      const std::string &current_file_name,
                      const clang::FunctionDecl *decl);


  virtual bool GetWrappedABI() override;

  const abi_dump::FunctionDecl &GetFunctionDecl() const;

 private:
  abi_dump::FunctionDecl abi_function_decl_;
  const clang::FunctionDecl *function_decl_;

 private:
  bool SetupFunction(abi_dump::FunctionDecl *methodp,
                     const clang::FunctionDecl *decl,
                     const std::string &source_file);

};

#endif  // ABI_WRAPPERS_H_

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

#include "frontend_action.h"

#include "ast_processing.h"
#include "diagnostic_consumer.h"
#include "header_abi_util.h"
#include "ir_representation.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

#include <llvm/ADT/STLExtras.h>

HeaderCheckerFrontendOptions::HeaderCheckerFrontendOptions(
    const std::string &dump_name_arg,
    std::set<std::string> &exported_headers_arg,
    abi_util::TextFormatIR text_format_arg, bool suppress_errors_arg)
    : dump_name(dump_name_arg), exported_headers(exported_headers_arg),
      text_format(text_format_arg), suppress_errors(suppress_errors_arg) {}

HeaderCheckerFrontendAction::HeaderCheckerFrontendAction(
    const HeaderCheckerFrontendOptions &options)
    : options_(options) {}

std::unique_ptr<clang::ASTConsumer>
HeaderCheckerFrontendAction::CreateASTConsumer(clang::CompilerInstance &ci,
                                               llvm::StringRef header_file) {
  // Create AST consumers.
  return llvm::make_unique<HeaderASTConsumer>(
      &ci, options_.dump_name, options_.exported_headers, options_.text_format);
}

bool HeaderCheckerFrontendAction::BeginInvocation(clang::CompilerInstance &ci) {
  if (options_.suppress_errors) {
    clang::DiagnosticsEngine &diagnostics = ci.getDiagnostics();
    diagnostics.setClient(
        new HeaderCheckerDiagnosticConsumer(diagnostics.takeClient()),
        /* ShouldOwnClient */ true);
  }
  return true;
}

bool HeaderCheckerFrontendAction::BeginSourceFileAction(
    clang::CompilerInstance &ci) {
  ci.getPreprocessor().SetSuppressIncludeNotFoundError(
      options_.suppress_errors);
  return true;
}

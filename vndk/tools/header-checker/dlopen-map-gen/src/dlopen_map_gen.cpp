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

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/PrettyPrinter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <string>
#include <vector>

#include <stdlib.h>

using clang::ast_matchers::MatchFinder;
using clang::ast_matchers::internal::Matcher;
using clang::ast_matchers::StatementMatcher;
using clang::ast_matchers::hasName;
using clang::ast_matchers::functionDecl;
using clang::ast_matchers::callExpr;
using clang::CallExpr;
using clang::Expr;
using clang::PrintingPolicy;
using clang::ast_matchers::callee;
using clang::tooling::newFrontendActionFactory;

static llvm::cl::OptionCategory header_checker_category(
    "dlopen-map-gen options");

static llvm::cl::opt<std::string> source_file(
    llvm::cl::Positional, llvm::cl::desc("<source.cpp>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

// Hide irrelevant command line options defined in LLVM libraries.
static void HideIrrelevantCommandLineOptions() {
  llvm::StringMap<llvm::cl::Option *> &map = llvm::cl::getRegisteredOptions();
  for (llvm::StringMapEntry<llvm::cl::Option *> &p : map) {
    if (p.second->Category == &header_checker_category) {
      continue;
    }
    if (p.first().startswith("help")) {
      continue;
    }
    p.second->setHiddenFlag(llvm::cl::Hidden);
  }
}


class DlopenMatchCallback : public MatchFinder::MatchCallback {
  virtual void run (const MatchFinder::MatchResult &result) override {
    const CallExpr *call_expr = result.Nodes.getNodeAs<CallExpr>("dlopen_call");
    std::string argument = "";
    llvm::raw_string_ostream sos(argument);
    if (!call_expr) {
      llvm::errs() << "Error while recovering callExpr\n";
      return;
    }
    assert(call_expr->getNumArgs() > 0);
    const Expr *expr = call_expr->getArg(0);
    expr->printPretty(sos, nullptr,
                      PrintingPolicy(result.Context->getLangOpts()));
    sos.str();
    llvm::errs() << argument << "\n";
  }
};

int main(int argc, const char **argv) {
  HideIrrelevantCommandLineOptions();

  // Create compilation database from command line arguments after "--".
  std::unique_ptr<clang::tooling::CompilationDatabase> compilations(
      clang::tooling::FixedCompilationDatabase::loadFromCommandLine(
          argc, argv));

  // Parse the command line options.
  llvm::cl::ParseCommandLineOptions(argc, argv, "dlopen-map-gen");

  // Input header file existential check.
  if (!llvm::sys::fs::exists(source_file)) {
    llvm::errs() << "ERROR: Source file \"" << source_file << "\" not found\n";
    ::exit(1);
  }

  // Check whether we can create compilation database and deduce compiler
  // options from command line options.
  if (!compilations) {
    llvm::errs() << "ERROR: Clang compilation options not specified.\n";
    ::exit(1);
  }

  // Initialize clang tools and run front-end action.
  std::vector<std::string> source_files{ source_file };

  clang::tooling::ClangTool tool(*compilations, source_files);

  clang::ast_matchers::MatchFinder dlopen_finder;
  DlopenMatchCallback dlopen_match_callback;
  Matcher<clang::Stmt> dlopen_call_matcher =
      callExpr(callee(functionDecl(hasName("dlopen")))).bind("dlopen_call");
  dlopen_finder.addMatcher(dlopen_call_matcher, &dlopen_match_callback);

  return tool.run(newFrontendActionFactory(&dlopen_finder).get());
}

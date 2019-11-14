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

#include <iostream>

#include "Globals.h"
#include "MyFrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

/** Options
 * Custom command line arguments are declared here
 * **/
static llvm::cl::OptionCategory kCPP2CCategory(
    "CPP2C options");  // not declared as const as cl::cat used below
// expects a non-const ref

static llvm::cl::opt<std::string> kHeaderFile(llvm::cl::Positional,
                                              llvm::cl::desc("<source.h>"),
                                              llvm::cl::Required,
                                              llvm::cl::cat(kCPP2CCategory));

/** wrap: list of classes to be wrapped.
 * Format of file:
 * cppclass_name1
 * cppclass_name2
 **/
static llvm::cl::opt<std::string> kClassesToBeWrappedArg(
    "wrap", llvm::cl::desc("Classes to be wrapped"), llvm::cl::Required,
    llvm::cl::cat(kCPP2CCategory));

// TODO: add support for including handwritten code

int main(int argc, const char** argv) {
  std::string cmdline_error_msg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compilations;

  compilations = clang::tooling::FixedCompilationDatabase::loadFromCommandLine(
      argc, argv, cmdline_error_msg);

  /** Parse the command-line args passed to your code
   * Format: cpp2c headers.h -customCmdLineArgs customCmdLineArgsValues --
   *-IdependentHeader
   **/
  llvm::cl::ParseCommandLineOptions(argc, argv, "CPP2C");
  if (!compilations) {
    if (cmdline_error_msg.empty()) {
      llvm::errs() << "ERROR: Failed to parse clang command line options\n";
    } else {
      llvm::errs() << "ERROR: " << cmdline_error_msg << "\n";
    }
    ::exit(1);
  }

  cpp2c::kHeaderForSrcFile = kHeaderFile.getValue();
  std::string classes_to_be_wrapped_file = kClassesToBeWrappedArg.getValue();

  cpp2c::kClassList =
      cpp2c::Resources::CreateResource(classes_to_be_wrapped_file);

  // create a new Clang Tool instance (a LibTooling environment)
  std::vector<std::string> header_files{kHeaderFile};
  clang::tooling::ClangTool tool(*compilations, header_files);

  // run the Clang Tool, creating a new FrontendAction
  return tool.run(
      clang::tooling::newFrontendActionFactory<cpp2c::MyFrontendAction>()
          .get());
}

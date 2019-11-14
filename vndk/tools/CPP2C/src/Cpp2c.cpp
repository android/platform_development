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

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "src/HeaderConverterOptions.h"
#include "src/impl/HeaderConverterFrontendAction.h"
#include "src/impl/HeaderConverterFrontendActionFactory.h"

/** Options
 * Custom command line arguments are declared here
 * **/
static llvm::cl::OptionCategory kCPP2CCategory("CPP2C options");

/**
 * The header file the tool uses to search for function to transpile
 */
static llvm::cl::opt<std::string> kHeaderFile(llvm::cl::Positional,
                                              llvm::cl::desc("<source.h>"),
                                              llvm::cl::Required,
                                              llvm::cl::cat(kCPP2CCategory));

/** List of classes to be wrapped/handled by the tool.
 *  Format of file:
 *    cppclass_name1
 *    cppclass_name2
 **/
static llvm::cl::opt<std::string> kClassesToBeWrappedArg(
    "wrap", llvm::cl::desc("Classes to be wrapped"), llvm::cl::Required,
    llvm::cl::cat(kCPP2CCategory));

/**
 * Path where the output should be written
 */
static llvm::cl::opt<std::string> kOutput(
    "o",
    llvm::cl::desc(
        "Path to be used to outout the generated header and source files"),
    llvm::cl::Required, llvm::cl::cat(kCPP2CCategory));

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

  // get the options
  std::string classes_to_be_wrapped_file = kClassesToBeWrappedArg.getValue();
  std::string input_header = kHeaderFile.getValue();
  std::string output_path = kOutput.getValue();

  // process the options
  std::unique_ptr<cpp2c::Resources> classes(
      cpp2c::Resources::CreateResource(classes_to_be_wrapped_file));

  // create a new Clang Tool instance (a LibTooling environment)
  std::vector<std::string> header_files{kHeaderFile};
  clang::tooling::ClangTool tool(*compilations, header_files);

  cpp2c::HeaderConverterOptions options{input_header, output_path,
                                        std::move(classes)};

  // run the Clang Tool, creating a new FrontendAction
  std::unique_ptr<clang::tooling::FrontendActionFactory> factory(
      new cpp2c::HeaderConverterFrontendActionFactory(options));
  return tool.run(factory.get());
}

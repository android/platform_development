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
#include "Options.h"
#include "clang/Tooling/Tooling.h"

int main(int argc, const char** argv) {
  cpp2c::kHeaderForSrcFile = argv[1];  // TODO the header with the headers we
                                       // need will be the first parameter

  /** Parse the command-line args passed to your code
   * Format: cpp2c headers.h -customCmdLineArgs customCmdLineArgsValues --
   *-IdependentHeader
   **/
  clang::tooling::CommonOptionsParser options_parser(argc, argv,
                                                     cpp2c::kCPP2CCategory);

  std::string classes_to_be_wrapped_file =
      cpp2c::kClassesToBeWrappedArg.getValue();

  cpp2c::kClassList =
      cpp2c::Resources::CreateResource(classes_to_be_wrapped_file);

  // create a new Clang Tool instance (a LibTooling environment)
  clang::tooling::ClangTool tool(options_parser.getCompilations(),
                                 options_parser.getSourcePathList());

  // run the Clang Tool, creating a new FrontendAction
  return tool.run(
      clang::tooling::newFrontendActionFactory<cpp2c::MyFrontendAction>()
          .get());
}

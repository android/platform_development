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

#ifndef CPP2C_MYFRONTENDACTION_H_
#define CPP2C_MYFRONTENDACTION_H_

#include <string>

#include "OutputStreams.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"

namespace cpp2c {

// For each source file provided to the tool, a new FrontendAction is created.
// TODO: this means if we specify 2 headers we would generate 2 cwrapper.h, the
// second overwriting the first
class MyFrontendAction : public clang::ASTFrontendAction {
 public:
  MyFrontendAction();

  void EndSourceFileAction() override;

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& CI, llvm::StringRef file) override;

 private:
  std::string retrieve_preprocessor_define_name();

  OutputStreams output_stream;
};

}  // namespace cpp2c

#endif  // CPP2C_MYFRONTENDACTION_H_
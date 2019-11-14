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

#ifndef CPP2C_HEADERCONVERTERFRONTENDACTION_H_
#define CPP2C_HEADERCONVERTERFRONTENDACTION_H_

#include <string>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "src/HeaderConverterOptions.h"
#include "src/utils/OutputStreams.h"

namespace cpp2c {

/** For each source file provided to the tool, a new FrontendAction is created.
 *  This class handles creating directories, writing to files and any other
 *  necessary setup and then delegates to the consumer
 */
class HeaderConverterFrontendAction : public clang::ASTFrontendAction {
 public:
  HeaderConverterFrontendAction(const HeaderConverterOptions& options);

  void EndSourceFileAction() override;

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance& compiler_instance,
      llvm::StringRef file) override;

 private:
  void init();
  const std::string retrieveFileName() const;

  const HeaderConverterOptions& options_;
  std::string preprocessor_define_name_;
  std::string file_name_;
  std::string wrapper_name_h_;
  std::string wrapper_name_cpp_;

  utils::OutputStreams output_stream;
};

}  // namespace cpp2c

#endif  // CPP2C_HEADERCONVERTERFRONTENDACTION_H_
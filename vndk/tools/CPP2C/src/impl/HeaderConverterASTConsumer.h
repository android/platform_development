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

#ifndef CPP2C_HEADERCONVERTERASTCONSUMER_H_
#define CPP2C_HEADERCONVERTERASTCONSUMER_H_

#include "clang/AST/ASTConsumer.h"
#include "src/HeaderConverterOptions.h"
#include "src/impl/ClassMatchHandler.h"
#include "src/utils/OutputStreams.h"

namespace cpp2c {

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers matchers and runs them on
// the AST.
class HeaderConverterASTConsumer : public clang::ASTConsumer {
 public:
  HeaderConverterASTConsumer(utils::OutputStreams& outputStreams,
                             const HeaderConverterOptions& options);

  void HandleTranslationUnit(clang::ASTContext& context) override;

 private:
  ClassMatchHandler class_matcher_handler_;
  clang::ast_matchers::MatchFinder matcher_;
};

}  // namespace cpp2c

#endif  // CPP2C_HEADERCONVERTERASTCONSUMER_H_
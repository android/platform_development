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

#ifndef CPP2C_MYASTCONSUMER_H_
#define CPP2C_MYASTCONSUMER_H_

#include "ClassMatchHandler.h"
#include "OutputStreams.h"
#include "clang/AST/ASTConsumer.h"

namespace cpp2c {

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public clang::ASTConsumer {
 public:
  MyASTConsumer(OutputStreams& os);

  void HandleTranslationUnit(clang::ASTContext& context) override;

 private:
  OutputStreams& output_stream_ref;
  ClassMatchHandler class_matcher_handler;
  clang::ast_matchers::MatchFinder matcher;
};

}  // namespace cpp2c

#endif  // CPP2C_MYASTCONSUMER_H_
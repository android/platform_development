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

#ifndef CPP2C_CLASSMATCHHANDLER_H_
#define CPP2C_CLASSMATCHHANDLER_H_

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "src/HeaderConverterOptions.h"
#include "src/utils/OutputStreams.h"

namespace cpp2c {

/**
 * Called when a match has been made.
 * This retrieves the method matched, calls the function converter and inserts
 * in the output_stream_ the results
 */
class ClassMatchHandler
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  ClassMatchHandler(utils::OutputStreams& output_streams,
                    const HeaderConverterOptions& options);

  virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result);

 private:
  utils::OutputStreams& output_stream_;
  const HeaderConverterOptions& options_;

  std::set<std::string> added_definitions_;
  std::set<std::string> added_implementations_;
};

}  // namespace cpp2c

#endif  // CPP2C_CLASSMATCHHANDLER_H_
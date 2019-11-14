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

#include "src/impl/ClassMatchHandler.h"

#include <set>

#include "src/converters/FunctionConverter.h"

namespace cpp2c {

using cpp2c::utils::OutputStreams;

ClassMatchHandler::ClassMatchHandler(OutputStreams& output_streams,
                                     const HeaderConverterOptions& options)
    : output_stream_(output_streams), options_(options) {}

void ClassMatchHandler::run(
    const clang::ast_matchers::MatchFinder::MatchFinder::MatchResult& result) {
  if (const clang::CXXMethodDecl* method_decl =
          result.Nodes.getNodeAs<clang::CXXMethodDecl>("publicMethodDecl")) {
    FunctionConverter function_converter(method_decl, options_);

    // transpile the matched function
    function_converter.run(result.Context);

    if (function_converter.getFunctionName() == "") {
      // we don't support certain scenarios, so skip those
      return;
    }

    // check if there are any declarations needed to be added to the header
    auto definitions = function_converter.getTypeDefinitions();
    for (const auto& it : definitions) {
      if (added_definitions_.find(it.first) != added_definitions_.end()) {
        continue;
      }

      added_definitions_.insert(it.first);
      output_stream_.header_stream << it.second << "\n";
    }

    // check if there are any helper functions to be added to the source file
    auto implementations = function_converter.getAdditionalImplementations();
    for (const auto& it : implementations) {
      if (added_implementations_.find(it.first) !=
          added_implementations_.end()) {
        continue;
      }

      added_implementations_.insert(it.first);
      output_stream_.body_stream << it.second << "\n";
    }

    // add the wrapper version of the matched  function and it's implementation
    output_stream_.header_stream << function_converter.getFunctionName()
                                 << ";\n";

    output_stream_.body_stream << function_converter.getFunctionName()
                               << "{\n    ";
    output_stream_.body_stream << function_converter.getFunctionBody()
                               << "; \n}\n";
  }
}

}  // namespace cpp2c
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

#include "ClassMatchHandler.h"

#include <set>

#include "CFunctionConverter.h"
#include "Resources.h"

namespace cpp2c {

extern Resources* kClassList;

static std::set<std::string> kDefinitionsAdded;

ClassMatchHandler::ClassMatchHandler(OutputStreams& os)
    : output_stream_ref(os) {}

void ClassMatchHandler::run(
    const clang::ast_matchers::MatchFinder::MatchFinder::MatchResult& Result) {
  if (const clang::CXXMethodDecl* method_decl =
          Result.Nodes.getNodeAs<clang::CXXMethodDecl>("publicMethodDecl")) {
    CFunctionConverter function_converter(method_decl, output_stream_ref);
    function_converter.run(Result.Context, kClassList);

    if (function_converter.getFunctionName() == "") {
      // we don't support certain scenarios, so skip those
      return;
    }

    auto definitions = function_converter.getTypeDefinitions();
    for (const auto& it : definitions) {
      if (kDefinitionsAdded.find(it.first) != kDefinitionsAdded.end()) {
        continue;
      }

      kDefinitionsAdded.insert(it.first);
      output_stream_ref.header_stream << it.second << "\n";
    }

    output_stream_ref.header_stream << function_converter.getFunctionName()
                                    << ";\n";

    output_stream_ref.body_stream << function_converter.getFunctionName()
                                  << "{\n    ";
    output_stream_ref.body_stream << function_converter.getFunctionBody()
                                  << "; \n}\n";
  }
}

void ClassMatchHandler::onEndOfTranslationUnit() {}

}  // namespace cpp2c
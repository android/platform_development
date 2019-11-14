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

#include "src/impl/HeaderConverterASTConsumer.h"

namespace cpp2c {

HeaderConverterASTConsumer::HeaderConverterASTConsumer(
    utils::OutputStreams& outputStreams, const HeaderConverterOptions& options)
    : class_matcher_handler_(outputStreams, options) {
  for (const std::string& class_name : options.classes->getResources()) {
    // register a matcher : try to find all public methods of the defined
    // classes
    clang::ast_matchers::DeclarationMatcher class_matcher =
        clang::ast_matchers::cxxMethodDecl(
            clang::ast_matchers::isPublic(),
            clang::ast_matchers::ofClass(
                clang::ast_matchers::hasName(class_name)))
            .bind("publicMethodDecl");

    matcher_.addMatcher(class_matcher, &class_matcher_handler_);
  }
  outputStreams.header_stream << "\n";
}

void HeaderConverterASTConsumer::HandleTranslationUnit(
    clang::ASTContext& context) {
  // Run the matchers when we have the whole translation unit parsed.
  matcher_.matchAST(context);
}

}  // namespace cpp2c
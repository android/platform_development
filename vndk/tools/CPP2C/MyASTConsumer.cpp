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

#include "MyASTConsumer.h"

#include "Resources.h"

namespace cpp2c {

extern Resources* kClassList;

MyASTConsumer::MyASTConsumer(OutputStreams& os)
    : output_stream_ref(os), class_matcher_handler(os) {
  for (const std::string& class_name : kClassList->getResources()) {
    // TODO add the struct declarations elsewhere as this doesn't work for all
    // cases i.e. template support e.g. foo <int> might be handled by
    // translating to foo_int but at this time we don't know the template type
    output_stream_ref.header_stream << "struct W" << class_name
                                    << "; \n"
                                       "typedef struct W"
                                    << class_name << " W" << class_name
                                    << ";\n";

    // We try to find all public methods of the defined classes
    // TODO: wrap only ANDROID_API classes and methods?
    clang::ast_matchers::DeclarationMatcher class_matcher =
        clang::ast_matchers::cxxMethodDecl(
            clang::ast_matchers::isPublic(),
            clang::ast_matchers::ofClass(
                clang::ast_matchers::hasName(class_name)))
            .bind("publicMethodDecl");  // TODO extract to constant
    matcher.addMatcher(class_matcher, &class_matcher_handler);
  }
  output_stream_ref.header_stream << "\n";
}

void MyASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  // Run the matchers when we have the whole translation unit parsed.
  matcher.matchAST(context);
}

}  // namespace cpp2c
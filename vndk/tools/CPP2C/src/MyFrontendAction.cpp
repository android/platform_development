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

#include "MyFrontendAction.h"

#include "MyASTConsumer.h"

namespace cpp2c {

extern std::string kHeaderForSrcFile;

MyFrontendAction::MyFrontendAction() {
  std::string preprocessor_define_name = retrieve_preprocessor_define_name();

  // Add header guards and necessary includes and typedefs
  output_stream.header_stream << "#ifndef _" + preprocessor_define_name +
                                     "_CWRAPPER_H_\n"
                                     "#define _" +
                                     preprocessor_define_name +
                                     "_CWRAPPER_H_\n"
                                     "#include \"" +
                                     kHeaderForSrcFile +
                                     "\"\n"
                                     "#ifdef __cplusplus\n"
                                     "typedef bool _Bool;\n"
                                     "extern \"C\"{\n"
                                     "#endif\n"
                                     "#include <stdbool.h>\n";
  output_stream.body_stream
      << "#include \"cwrapper.h\"\n"
         "#include <type_traits> // for std::underlying_type usage\n"
         "#ifdef __cplusplus\n"
         "extern \"C\"{\n"
         "#endif\n";
}

void MyFrontendAction::EndSourceFileAction() {
  std::string preprocessor_define_name = retrieve_preprocessor_define_name();

  // Create and write the output files
  llvm::StringRef header_file("cwrapper.h");  // TODO don't use hardcoded names
  llvm::StringRef body_file("cwrapper.cpp");

  // Open the output files
  std::error_code error;
  llvm::raw_fd_ostream header_stream(header_file, error, llvm::sys::fs::F_None);
  if (error) {
    llvm::errs() << "while opening '" << header_file << "': " << error.message()
                 << '\n';
    exit(1);
  }
  llvm::raw_fd_ostream body_stream(body_file, error, llvm::sys::fs::F_None);
  if (error) {
    llvm::errs() << "while opening '" << body_file << "': " << error.message()
                 << '\n';
    exit(1);
  }

  // Header guards end
  output_stream.header_stream << "#ifdef __cplusplus\n"
                                 "}\n"
                                 "#endif\n"
                                 "#endif /* _" +
                                     preprocessor_define_name +
                                     "_CWRAPPER_H_ */\n";

  output_stream.body_stream << "#ifdef __cplusplus\n"
                               "}\n"
                               "#endif\n";

  output_stream.header_stream.flush();
  output_stream.body_stream.flush();

  // Write to files
  header_stream << output_stream.header_string << "\n";
  body_stream << output_stream.body_string << "\n";
}

std::unique_ptr<clang::ASTConsumer> MyFrontendAction::CreateASTConsumer(
    clang::CompilerInstance& CI, llvm::StringRef file) {
  // TODO: getCurrentFile here, grab the name and use that instead for
  // generating  the preprocessor name
  return std::make_unique<MyASTConsumer>(output_stream);
}

// TODO: consider using std::filesystem::path::filename from c++17
std::string MyFrontendAction::retrieve_preprocessor_define_name() {
  std::string copy = kHeaderForSrcFile;

  auto pos = copy.rfind('/');
  if (pos != std::string::npos) {
    copy = copy.substr(pos + 1, copy.length());
  }

  pos = copy.rfind('.');
  if (pos != std::string::npos) {
    copy = copy.substr(0, pos);
  }

  transform(begin(copy), end(copy), begin(copy), ::toupper);

  return copy;
}

}  // namespace cpp2c
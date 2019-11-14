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

#include "src/impl/HeaderConverterFrontendAction.h"

#include <filesystem>

#include "src/impl/HeaderConverterASTConsumer.h"

namespace cpp2c {

HeaderConverterFrontendAction::HeaderConverterFrontendAction(
    const HeaderConverterOptions& options)
    : options_(options) {
  init();

  // Add header guards and necessary includes and typedefs
  output_stream.header_stream << "#ifndef _" << preprocessor_define_name_
                              << "\n"
                                 "#define _"
                              << preprocessor_define_name_
                              << "\n"
                                 "#ifdef __cplusplus\n"
                                 "typedef bool _Bool;\n"
                                 "extern \"C\"{\n"
                                 "#endif\n"
                                 "#include <stdbool.h>\n"
                                 "#include <cstdint>\n"  // for int32_t
                                 "#include <stddef.h>";  // for size_t
  output_stream.body_stream
      << "#include \"" << options_.output_path << "/" << wrapper_name_h_
      << "\"\n"
      << "#include \"" << options_.input_header << "\"\n"
      << "#include <type_traits>\n"  // for std::underlying_type usage
         "#include <cstring>\n"      // for strncpy
         "#ifdef __cplusplus\n"
         "extern \"C\"{\n"
         "#endif\n";
}

void HeaderConverterFrontendAction::EndSourceFileAction() {
  // Create and write the output files
  std::string output_path_h = options_.output_path + "/" + wrapper_name_h_;
  std::string output_path_cpp = options_.output_path + "/" + wrapper_name_cpp_;
  llvm::StringRef header_file(output_path_h);
  llvm::StringRef body_file(output_path_cpp);

  std::filesystem::create_directories(options_.output_path);

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
                                     preprocessor_define_name_ + " */\n";

  output_stream.body_stream << "#ifdef __cplusplus\n"
                               "}\n"
                               "#endif\n";

  output_stream.header_stream.flush();
  output_stream.body_stream.flush();

  // Write to files
  header_stream << output_stream.header_string << "\n";
  body_stream << output_stream.body_string << "\n";
}

void HeaderConverterFrontendAction::init() {
  std::string preprocessor_define_name = retrieveFileName();
  transform(begin(preprocessor_define_name), end(preprocessor_define_name),
            begin(preprocessor_define_name), ::toupper);
  preprocessor_define_name_ =
      "_" + preprocessor_define_name + "WRAPPER" + "_H_";

  file_name_ = retrieveFileName();
  wrapper_name_h_ = file_name_ + "Wrapper.h";
  wrapper_name_cpp_ = file_name_ + "Wrapper.cpp";
}

std::unique_ptr<clang::ASTConsumer>
HeaderConverterFrontendAction::CreateASTConsumer(
    clang::CompilerInstance& compiler_instance, llvm::StringRef file) {
  return std::make_unique<HeaderConverterASTConsumer>(output_stream, options_);
}

const std::string HeaderConverterFrontendAction::retrieveFileName() const {
  std::filesystem::path header_path = options_.input_header;
  std::string file_name = header_path.filename().replace_extension().string();
  return file_name;
}

}  // namespace cpp2c
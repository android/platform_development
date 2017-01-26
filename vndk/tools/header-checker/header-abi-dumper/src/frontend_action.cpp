// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "frontend_action.h"

#include "ast_processing.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/FileSystem.h>

#include <memory>
#include <string>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using std::set;

HeaderCheckerFrontendAction::HeaderCheckerFrontendAction(
    const std::string &dump_name, const vector<std::string> &exports)
  : dump_name_(dump_name), export_include_dirs_(exports) { }

std::unique_ptr<clang::ASTConsumer>
HeaderCheckerFrontendAction::CreateASTConsumer(clang::CompilerInstance &ci,
                                               llvm::StringRef header_file) {
  // Add preprocessor callbacks.
  clang::Preprocessor &pp = ci.getPreprocessor();
  pp.addPPCallbacks(llvm::make_unique<HeaderASTPPCallbacks>());
  std::set<std::string> exported_headers;
  for (auto &it : export_include_dirs_) {
    if (!GetExportedHeaderSet(it, exported_headers)) {
         return nullptr;
    }
  }
  // Create AST consumers.
  return llvm::make_unique<HeaderASTConsumer>(header_file,
                                              &ci, dump_name_,
                                              exported_headers);
}

bool HeaderCheckerFrontendAction::GetExportedHeaderSet(std::string dir_name,
                                                       set<std::string> &eh) {
  dir_name += "/";
  DIR *directory = opendir((dir_name + "/").c_str());
  if (!directory) {
    llvm::errs() << "Opening directory : " << dir_name << " failed\n";
    return false;
  }

  while (struct dirent *directory_entry = readdir(directory)) {
    std::string dirent_name(directory_entry->d_name);
    // Hidden Files
    if (dirent_name.find(".") == 0) {
      continue;
    }
    struct stat buf;
    std::string file_path = dir_name + dirent_name;
    llvm::SmallString<128> abs_path(file_path);
    if (llvm::sys::fs::make_absolute(abs_path)) {
      return false;
    }
    file_path = abs_path.str();
    if (stat(file_path.c_str(), &buf)) {
      // USE goto instead ?
      closedir(directory);
      return false;
    }
    if (S_ISREG(buf.st_mode)) {
      eh.insert(file_path);
    }
    if (S_ISDIR(buf.st_mode)) {
      if (!GetExportedHeaderSet(file_path, eh)) {
        llvm::errs() << "Couldn't process dir : " << file_path << "\n";
        return false;
      }
    }
  }
  closedir(directory);
  return true;
}

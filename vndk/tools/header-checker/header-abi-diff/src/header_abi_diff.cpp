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

#include "abi_diff.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

static llvm::cl::OptionCategory header_checker_category(
    "header-abi-diff options");

static llvm::cl::opt<std::string> compatibility_report(
    "o", llvm::cl::desc("<compatibility report>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> new_dump(
    "new", llvm::cl::desc("<new dump>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> old_dump(
    "old", llvm::cl::desc("<old dump>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<bool> advice_only(
    "advice-only", llvm::cl::desc("Advisory mode only"), llvm::cl::Optional,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> ignore_symbol_list(
    "ignore-symbols", llvm::cl::desc("ignore symbols"), llvm::cl::Optional,
    llvm::cl::cat(header_checker_category));

static std::set<std::string> GetIgnoredSymbols(std::string &symbol_list_path) {
  std::ifstream symbol_ifstream(symbol_list_path);
  std::set<std::string> ignored_symbols;
  if (!symbol_ifstream.is_open()) {
    llvm::errs() << "Failed to file containing symbols to ignore\n";
    ::exit(1);
  }
  std::string line = "";
  while (std::getline(symbol_ifstream, line)) {
    ignored_symbols.insert(line);
  }
  return ignored_symbols;
}

int main(int argc, const char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  llvm::cl::ParseCommandLineOptions(argc, argv, "header-checker");
  uint8_t extension_or_incompatible = 0;
  std::set<std::string> ignore_symbols;
  if (llvm::sys::fs::exists(ignore_symbol_list)) {
    ignore_symbols = GetIgnoredSymbols(ignore_symbol_list);
  }
  HeaderAbiDiff judge(old_dump, new_dump, compatibility_report,
                      ignore_symbols);
  switch (judge.GenerateCompatibilityReport()) {
    case HeaderAbiDiff::COMPATIBLE:
      break;
    case HeaderAbiDiff::EXTENSION:
      extension_or_incompatible = 1;
      break;
    default:
      extension_or_incompatible = 2;
      break;
  }
  if (extension_or_incompatible) {
    llvm::errs() << "******************************************************\n"
                 << "VNDK Abi Compliance breakage:"
                 << " Please check compatiblity report at : "
                 << compatibility_report << "\n"
                 << "*****************************************************\n";
    if (!advice_only) {
      return extension_or_incompatible;
    }
  }
  return 0;
}

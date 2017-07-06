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

static llvm::cl::opt<std::string> lib_name(
    "lib", llvm::cl::desc("<lib name>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> arch(
    "arch", llvm::cl::desc("<arch>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> new_dump(
    "new", llvm::cl::desc("<new dump>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> old_dump(
    "old", llvm::cl::desc("<old dump>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<std::string> ignore_symbol_list(
    "ignore-symbols", llvm::cl::desc("ignore symbols"), llvm::cl::Optional,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<bool> advice_only(
    "advice-only", llvm::cl::desc("Advisory mode only"), llvm::cl::Optional,
    llvm::cl::cat(header_checker_category));

static llvm::cl::opt<bool> check_all_apis(
    "check-all-apis", llvm::cl::desc("All apis, whether referenced or not,\
                                  by exported symbols in the dynsym table of a\
                                  shared library are checked"),
    llvm::cl::Optional, llvm::cl::cat(header_checker_category));

static llvm::cl::opt<bool> suppress_local_warnings(
    "suppress_local_warnings", llvm::cl::desc("suppress local warnings"),
    llvm::cl::Optional, llvm::cl::cat(header_checker_category));

static llvm::cl::opt<bool> allow_extensions(
    "allow-extensions",
    llvm::cl::desc("Do not return a non zero status on extensions"),
    llvm::cl::Optional, llvm::cl::cat(header_checker_category));

static std::set<std::string> LoadIgnoredSymbols(std::string &symbol_list_path) {
  std::ifstream symbol_ifstream(symbol_list_path);
  std::set<std::string> ignored_symbols;
  if (!symbol_ifstream) {
    llvm::errs() << "Failed to open file containing symbols to ignore\n";
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
  std::set<std::string> ignored_symbols;
  if (llvm::sys::fs::exists(ignore_symbol_list)) {
    ignored_symbols = LoadIgnoredSymbols(ignore_symbol_list);
  }
  HeaderAbiDiff judge(lib_name, arch, old_dump, new_dump, compatibility_report,
                      ignored_symbols, check_all_apis);

  CompatibilityStatusIR status = judge.GenerateCompatibilityReport();

  std::string status_str = "";
  std::string unreferenced_change_str = "";
  std::string error_or_warning_str = "\033[36;1mwarning: \033[0m";

  if (status == CompatibilityStatusIR::INCOMPATIBLE) {
    error_or_warning_str = "\033[31;1merror: \033[0m";
    status_str = " INCOMPATIBLE CHANGES";
  } else if (status & CompatibilityStatusIR::EXTENSION) {
    status_str = "EXTENDING CHANGES";
  }
  if (status & CompatibilityStatusIR::UNREFERENCED_CHANGES) {
    unreferenced_change_str = ", Changes in exported headers, which are";
    unreferenced_change_str +=  " not directly referenced by exported symbols.";
    unreferenced_change_str +=  " This MIGHT be an ABI breaking change due to";
    unreferenced_change_str += " internal typecasts.";

  }
  if (!suppress_local_warnings && status) {
    llvm::errs() << "******************************************************\n"
                 << error_or_warning_str
                 << "VNDK library: "
                 << lib_name
                 << "'s ABI has "
                 << status_str
                 << unreferenced_change_str
                 << " Please check compatiblity report at : "
                 << compatibility_report << "\n"
                 << "******************************************************\n";
  }

  if (advice_only) {
    return CompatibilityStatusIR::COMPATIBLE;
  }

  if (allow_extensions && status == CompatibilityStatusIR::EXTENSION) {
    return CompatibilityStatusIR::COMPATIBLE;
  }
  return status;
}

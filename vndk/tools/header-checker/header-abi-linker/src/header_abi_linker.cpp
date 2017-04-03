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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "proto/abi_dump.pb.h"
#pragma clang diagnostic pop

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>


#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stdlib.h>

static llvm::cl::OptionCategory header_linker_category(
    "header-abi-linker options");

static llvm::cl::list<std::string> dump_files(
    llvm::cl::Positional, llvm::cl::desc("<dump-files>"), llvm::cl::Required,
    llvm::cl::cat(header_linker_category), llvm::cl::OneOrMore);

static llvm::cl::opt<std::string> linked_dump(
    "o", llvm::cl::desc("<linked dump>"), llvm::cl::Required,
    llvm::cl::cat(header_linker_category));

static llvm::cl::list<std::string> stub_files(
    "s", llvm::cl::desc("<stub files>"), llvm::cl::ZeroOrMore,
    llvm::cl::cat(header_linker_category));

class HeaderAbiLinker {
 public:
  HeaderAbiLinker(
      const std::vector<std::string> &dump_files,
      const std::vector<std::string> &stub_files,
      const std::string &linked_dump)
    : dump_files_(dump_files), stub_files_(stub_files),
      out_dump_name_(linked_dump) {};

  bool LinkAndDump();

 private:
  bool LinkRecords(const abi_dump::TranslationUnit &dump_tu,
                   abi_dump::TranslationUnit *linked_tu);

  bool LinkFunctions(const abi_dump::TranslationUnit &dump_tu,
                     abi_dump::TranslationUnit *linked_tu);

  bool LinkEnums(const abi_dump::TranslationUnit &dump_tu,
                 abi_dump::TranslationUnit *linked_tu);

  bool LinkGlobalVars(const abi_dump::TranslationUnit &dump_tu,
                      abi_dump::TranslationUnit *linked_tu);

  template <typename T>
  static inline bool LinkDecl(
    google::protobuf::RepeatedPtrField<T> *dst,
    std::set<std::string> *link_set,
    const google::protobuf::RepeatedPtrField<T> &src,
    bool use_stub_files);

  bool ParseStubFiles();

 private:
  const std::vector<std::string> &dump_files_;
  const std::vector<std::string> &stub_files_;
  const std::string &out_dump_name_;
  std::set<std::string> record_decl_set_;
  // Used in both cases : stub files as well as no stub file.
  std::set<std::string> function_decl_set_;
  std::set<std::string> enum_decl_set_;
  // Used in both cases : stub files as well as no stub file.
  std::set<std::string> globvar_decl_set_;
};

bool HeaderAbiLinker::LinkAndDump() {
  abi_dump::TranslationUnit linked_tu;
  std::ofstream text_output(out_dump_name_);
  google::protobuf::io::OstreamOutputStream text_os(&text_output);
  ParseStubFiles();
  for (auto &&i : dump_files_) {
    abi_dump::TranslationUnit dump_tu;
    std::ifstream input(i);
    google::protobuf::io::IstreamInputStream text_is(&input);
    if (!google::protobuf::TextFormat::Parse(&text_is, &dump_tu) ||
        !LinkRecords(dump_tu, &linked_tu) ||
        !LinkFunctions(dump_tu, &linked_tu) ||
        !LinkEnums(dump_tu, &linked_tu) ||
        !LinkGlobalVars(dump_tu, &linked_tu)) {
      llvm::errs() << "Failed to link elements\n";
      return false;
    }
  }

  if (!google::protobuf::TextFormat::Print(linked_tu, &text_os)) {
    llvm::errs() << "Serialization to ostream failed\n";
    return false;
  }
  return true;
}

static std::string GetSymbol(const abi_dump::RecordDecl &element) {
  return element.mangled_record_name();
}

static std::string GetSymbol(const abi_dump::FunctionDecl &element) {
  return element.mangled_function_name();
}

static std::string GetSymbol(const abi_dump::EnumDecl &element) {
  return element.basic_abi().linker_set_key();
}

static std::string GetSymbol(const abi_dump::GlobalVarDecl &element) {
  return element.basic_abi().linker_set_key();
}

template <typename T>
inline bool HeaderAbiLinker::LinkDecl(
    google::protobuf::RepeatedPtrField<T> *dst,
    std::set<std::string> *link_set,
    const google::protobuf::RepeatedPtrField<T> &src,
    bool use_stub_files) {
  assert(dst != nullptr);
  assert(link_set != nullptr);
  for (auto &&element : src) {
    // Check for the existance of the element in linked dump / symbol file.
    if (!use_stub_files &&
        !link_set->insert(element.basic_abi().linker_set_key()).second) {
        continue;
    } else if (link_set->find(GetSymbol(element)) == link_set->end()) {
      continue;
    }
    T *added_element = dst->Add();
    llvm::errs() << "Added lsk " << element.basic_abi().linker_set_key()<<"\n";
    if (!added_element) {
      llvm::errs() << "Failed to add element to linked dump\n";
      return false;
    }
    *added_element = element;
  }
  return true;
}

bool HeaderAbiLinker::LinkRecords(const abi_dump::TranslationUnit &dump_tu,
                                  abi_dump::TranslationUnit *linked_tu) {
  assert(linked_tu != nullptr);
  bool use_stub_files = false;
  std::set<std::string> *link_set = &record_decl_set_;
  if (stub_files_.size() != 0) {
    use_stub_files = true;
    link_set = &function_decl_set_;
  }
  return LinkDecl(linked_tu->mutable_records(), link_set,
                  dump_tu.records(), use_stub_files);
}

bool HeaderAbiLinker::LinkFunctions(const abi_dump::TranslationUnit &dump_tu,
                                    abi_dump::TranslationUnit *linked_tu) {
  assert(linked_tu != nullptr);
  return LinkDecl(linked_tu->mutable_functions(), &function_decl_set_,
                  dump_tu.functions(), (stub_files_.size() != 0));
}

bool HeaderAbiLinker::LinkEnums(const abi_dump::TranslationUnit &dump_tu,
                                abi_dump::TranslationUnit *linked_tu) {
  assert(linked_tu != nullptr);
  bool use_stub_files = false;
  std::set<std::string> *link_set = &record_decl_set_;
  if (stub_files_.size() != 0) {
    use_stub_files = true;
    link_set = &function_decl_set_;
  }
  return LinkDecl(linked_tu->mutable_enums(), link_set,
                  dump_tu.enums(), use_stub_files);
}

bool HeaderAbiLinker::LinkGlobalVars(const abi_dump::TranslationUnit &dump_tu,
                                     abi_dump::TranslationUnit *linked_tu) {
  assert(linked_tu != nullptr);
  return LinkDecl(linked_tu->mutable_global_vars(), &globvar_decl_set_,
                  dump_tu.global_vars(), (stub_files_.size() != 0));
}

static std::string GetSymbolFromLine(const std::string &line,
                                     const std::string &type_str,
                                     const std::string &delim) {
  std::string symbol = "";
  if(line.find(type_str) == 0) {
    std::string symbol_with_delim =
        line.substr(std::strlen(type_str.c_str()) + 1);
    std::string::size_type pos = symbol_with_delim.find(delim);
    if (pos != std::string::npos) {
      symbol = symbol_with_delim.substr(0, pos);
    }
  }
  return symbol;
}

bool HeaderAbiLinker::ParseStubFiles() {
  for (auto &&stub_file : stub_files_) {
    std::ifstream symbol_ifstream(stub_file);
    if (!symbol_ifstream.is_open()) {
      llvm::errs() << "Failed to open stub file\n";
      return false;
    }
    std::string line = "";
    while(std::getline(symbol_ifstream, line)) {
      // Try to match void <symbol>() {}
      std::string symbol = GetSymbolFromLine(line, "void", "(");
      if(!symbol.empty()) {
        function_decl_set_.insert(symbol);
        continue;
      }
      // Try to match int <symbol> = 0
      symbol = GetSymbolFromLine(line, "int", " =");
      if (!symbol.empty()) {
        globvar_decl_set_.insert(symbol);
      }
    }
  }
  return true;
}


int main(int argc, const char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  llvm::cl::ParseCommandLineOptions(argc, argv, "header-linker");
  HeaderAbiLinker Linker(dump_files, stub_files, linked_dump);
  if (!Linker.LinkAndDump()) {
    return -1;
  }
  return 0;
}

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

#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stdlib.h>


#define LINKDECL(set_type, decl_type) \
for (int i = 0; i < dump_tu->m##set_type##s_size(); i++) { \
    const abi_dump::T##decl_type##Decl& m##set_type##element = dump_tu->m##set_type##s(i); \
    std::string key = m##set_type##element.mlinker_set_key(); \
    if (m##set_type##_decl_set_.find(key) != m##set_type##_decl_set_.end()) { \
      continue; \
    } \
    m##set_type##_decl_set_.insert(key); \
    abi_dump::T##decl_type##Decl *added_m##set_type = linked_tu->add_m##set_type##s(); \
    if (!added_m##set_type) { \
      return false; \
    } \
    *added_m##set_type = m##set_type##element; \
}


using std::vector;

static llvm::cl::OptionCategory header_checker_category(
    "header-abi-linker options");

static llvm::cl::list<std::string> dump_files(
    llvm::cl::Positional, llvm::cl::desc("<dump-files>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category), llvm::cl::OneOrMore);

static llvm::cl::opt<std::string> linked_dump(
    "o", llvm::cl::desc("<linked dump>"), llvm::cl::Required,
    llvm::cl::cat(header_checker_category));

class HeaderAbiLinker {
 public:
  HeaderAbiLinker(vector<std::string> &files, std::string &linked_dump)
      : dump_files_(files), out_dump_name_(linked_dump) {};

  std::unique_ptr<abi_dump::TTranslationUnit>Link();

 private:
  bool LinkRecords(abi_dump::TTranslationUnit *dump_tu,
                   abi_dump::TTranslationUnit *linked_tu);

  bool LinkFunctions(abi_dump::TTranslationUnit *dump_tu,
                     abi_dump::TTranslationUnit *linked_tu);

  bool LinkEnums(abi_dump::TTranslationUnit *dump_tu,
                 abi_dump::TTranslationUnit *linked_tu);

 private:
  vector<std::string> &dump_files_;
  std::string &out_dump_name_;
  std::set<std::string> mrecord_decl_set_;
  std::set<std::string> mfunction_decl_set_;
  std::set<std::string> menum_decl_set_;
};

std::unique_ptr<abi_dump::TTranslationUnit> HeaderAbiLinker::Link() {
  std::unique_ptr<abi_dump::TTranslationUnit>linked_tu(
      new abi_dump::TTranslationUnit);
  for (auto &i : dump_files_) {
    abi_dump::TTranslationUnit dump_tu;
    std::fstream input(i, std::ios::binary | std::ios::in);
    if (!dump_tu.ParseFromIstream(&input)) {
      llvm::errs() << "Failed to read dump file\n";
      return nullptr;
    }
    if (!LinkRecords(&dump_tu, linked_tu.get()) ||
        !LinkFunctions(&dump_tu, linked_tu.get()) ||
        !LinkEnums(&dump_tu, linked_tu.get())) {
          return nullptr;
    }
  }

  std::ofstream text_output(out_dump_name_ + ".txt");
  std::fstream binary_output(
      (out_dump_name_).c_str(),
      std::ios::out | std::ios::trunc | std::ios::binary);
  std::string str_out;
  google::protobuf::TextFormat::PrintToString(*linked_tu, &str_out);
  text_output << str_out;
  if (!linked_tu->SerializeToOstream(&binary_output)) {
    llvm::errs() << "Serialization to ostream failed\n";
    return nullptr;
  }

  return linked_tu ;
}

bool HeaderAbiLinker::LinkRecords(abi_dump::TTranslationUnit *dump_tu,
                                  abi_dump::TTranslationUnit *linked_tu) {
  LINKDECL(record, Record)
  return true;
}

bool HeaderAbiLinker::LinkFunctions(abi_dump::TTranslationUnit *dump_tu,
                                    abi_dump::TTranslationUnit *linked_tu) {
  LINKDECL(function, Function)
  return true;
}

bool HeaderAbiLinker::LinkEnums(abi_dump::TTranslationUnit *dump_tu,
                                abi_dump::TTranslationUnit *linked_tu) {
  LINKDECL(enum, Enum)
  return true;
}

//TODO: GOOGLE_PROTOBUF_VERIFY
int main(int argc, const char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "header-checker");
  HeaderAbiLinker Linker(dump_files, linked_dump);
  if (!Linker.Link()) {
    return -1;
  }

  return 0;
}

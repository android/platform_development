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

#include <header_abi_util.h>
#include <ir_representation.h>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <mutex>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>
#include <unistd.h>

#include <stdlib.h>

#define SOURCES_PER_THREAD 5

static llvm::cl::OptionCategory header_linker_category(
    "header-abi-linker options");

static llvm::cl::list<std::string> dump_files(
    llvm::cl::Positional, llvm::cl::desc("<dump-files>"), llvm::cl::Required,
    llvm::cl::cat(header_linker_category), llvm::cl::OneOrMore);

static llvm::cl::opt<std::string> linked_dump(
    "o", llvm::cl::desc("<linked dump>"), llvm::cl::Required,
    llvm::cl::cat(header_linker_category));

static llvm::cl::list<std::string> exported_header_dirs(
    "I", llvm::cl::desc("<export_include_dirs>"), llvm::cl::Prefix,
    llvm::cl::ZeroOrMore, llvm::cl::cat(header_linker_category));

static llvm::cl::opt<std::string> version_script(
    "v", llvm::cl::desc("<version_script>"), llvm::cl::Optional,
    llvm::cl::cat(header_linker_category));

static llvm::cl::opt<std::string> api(
    "api", llvm::cl::desc("<api>"), llvm::cl::Optional,
    llvm::cl::cat(header_linker_category));

static llvm::cl::opt<std::string> text_format(
    "text-format", llvm::cl::desc("<text-format : eg: protobuf, xml>"),
    llvm::cl::Optional, llvm::cl::init("protobuf"),
    llvm::cl::cat(header_linker_category));

static llvm::cl::opt<std::string> arch(
    "arch", llvm::cl::desc("<arch>"), llvm::cl::Optional,
    llvm::cl::cat(header_linker_category));

static llvm::cl::opt<bool> no_filter(
    "no-filter", llvm::cl::desc("Do not filter any abi"), llvm::cl::Optional,
    llvm::cl::cat(header_linker_category));

static llvm::cl::opt<std::string> so_file(
    "so", llvm::cl::desc("<path to so file>"), llvm::cl::Optional,
    llvm::cl::cat(header_linker_category));

class HeaderAbiLinker {
 public:
  HeaderAbiLinker(
      const std::vector<std::string> &dump_files,
      const std::vector<std::string> &exported_header_dirs,
      const std::string &version_script,
      const std::string &so_file,
      const std::string &linked_dump,
      const std::string &arch,
      const std::string &api)
    : dump_files_(dump_files), exported_header_dirs_(exported_header_dirs),
    version_script_(version_script), so_file_(so_file),
    out_dump_name_(linked_dump), arch_(arch), api_(api) {};

  bool LinkAndDump();

 private:
  template <typename T>
  inline bool LinkDecl(abi_util::IRDumper *dst,
                       std::set<std::string> *link_set,
                       std::set<std::string> *regex_matched_link_set,
                       const std::regex *vs_regex,
                       const std::map<std::string, T> &src,
                       bool use_version_script);

  bool ParseVersionScriptFiles();

  bool ParseSoFile();

  bool LinkTypes(const abi_util::TextFormatToIRReader *ir_reader,
                 abi_util::IRDumper *ir_dumper);

  bool LinkFunctions(const abi_util::TextFormatToIRReader *ir_reader,
                     abi_util::IRDumper *ir_dumper);

  bool LinkGlobalVars(const abi_util::TextFormatToIRReader *ir_reader,
                      abi_util::IRDumper *ir_dumper);

  bool AddElfSymbols(abi_util::IRDumper *ir_dumper);


 private:
  const std::vector<std::string> &dump_files_;
  const std::vector<std::string> &exported_header_dirs_;
  const std::string &version_script_;
  const std::string &so_file_;
  const std::string &out_dump_name_;
  const std::string &arch_;
  const std::string &api_;
  // TODO: Add to a map of std::sets instead.
  std::set<std::string> exported_headers_;
  std::set<std::string> types_set_;
  std::set<std::string> function_decl_set_;
  std::set<std::string> globvar_decl_set_;
  // Version Script Regex Matching.
  std::set<std::string> functions_regex_matched_set;
  std::regex functions_vs_regex_;
  // Version Script Regex Matching.
  std::set<std::string> globvars_regex_matched_set;
  std::regex globvars_vs_regex_;
};

static int GetCpuCount() {
#if defined(__linux__)
  cpu_set_t cpu_set;
  int rc = sched_getaffinity(getpid(), sizeof(cpu_set), &cpu_set);
  if (rc != 0) {
    llvm::errs() << "sched_getaffinity failed\n";
    ::exit(1);
  }
  return CPU_COUNT(&cpu_set);
#else
  return 1;
#endif
}

template <typename T, typename Iterable>
static bool AddElfSymbols(abi_util::IRDumper *dst,
                         const Iterable &symbols) {
  for (auto &&symbol : symbols) {
    T elf_symbol(symbol);
    if (!dst->AddElfSymbolMessageIR(&elf_symbol)) {
      return false;
    }
  }
  return true;
}

// To be called right after parsing the .so file / version script.
bool HeaderAbiLinker::AddElfSymbols(abi_util::IRDumper *ir_dumper) {
  return ::AddElfSymbols<abi_util::ElfFunctionIR>(ir_dumper,
                                                  function_decl_set_)

      && ::AddElfSymbols<abi_util::ElfObjectIR>(ir_dumper,
                                                globvar_decl_set_);
}

static void LinkThread(abi_util::TextFormatToIRReader *greader,
                       const std::vector<std::string> &dump_files,
                       size_t start, size_t end) {
 static std::mutex greader_lock;
 std::unique_ptr<abi_util::TextFormatToIRReader> local_reader =
        abi_util::TextFormatToIRReader::CreateTextFormatToIRReader(text_format,
                                                                   "");
  for (size_t i = start; i <= end; i++) {
    std::unique_ptr<abi_util::TextFormatToIRReader> reader =
        abi_util::TextFormatToIRReader::CreateTextFormatToIRReader(
            text_format, dump_files[i]);
    assert(reader != nullptr);
    if (!reader->ReadDump()) {
      llvm::errs() << "ReadDump failed\n";
      ::exit(1);
    }
    *local_reader += *reader;
  }
  std::unique_lock<std::mutex> lock(greader_lock);
  *greader += *local_reader;
}

bool HeaderAbiLinker::LinkAndDump() {
  // If the user specifies that a version script should be used, use that.
  if (!so_file_.empty()) {
    exported_headers_ =
        abi_util::CollectAllExportedHeaders(exported_header_dirs_);
    if (!ParseSoFile()) {
      llvm::errs() << "Couldn't parse so file\n";
      return false;
    }
  } else if (!ParseVersionScriptFiles()) {
    llvm::errs() << "Failed to parse stub files for exported symbols\n";
    return false;
  }
  size_t max_threads = GetCpuCount();
  size_t num_threads = SOURCES_PER_THREAD < dump_files_.size() ?\
                    std::min(dump_files_.size() / SOURCES_PER_THREAD,
                             max_threads) : 0;
  std::unique_ptr<abi_util::IRDumper> ir_dumper =
      abi_util::IRDumper::CreateIRDumper(text_format, out_dump_name_);

  AddElfSymbols(ir_dumper.get());
  assert(ir_dumper != nullptr);
  // Create a reader, on which we never actually call ReadDump(), since multiple
  // dump files are associated with it.
  std::unique_ptr<abi_util::TextFormatToIRReader> greader =
      abi_util::TextFormatToIRReader::CreateTextFormatToIRReader(text_format,
                                                                 "");
  std::vector<std::thread> threads;
  if (num_threads > 0) {
    for (size_t i = 0; i < num_threads; i++) {
      size_t start = i * SOURCES_PER_THREAD;
      size_t end_boundary = start + SOURCES_PER_THREAD - 1;
      size_t end = (i == num_threads - 1) ?
          dump_files_.size() - 1 : end_boundary;
      threads.emplace_back(LinkThread, greader.get(), dump_files_, start, end);
    }
    for (auto &thread : threads) {
      thread.join();
    }
  } else {
    LinkThread(greader.get(), dump_files_, 0, dump_files_.size() - 1);
  }

  if (!LinkTypes(greader.get(), ir_dumper.get()) ||
     !LinkFunctions(greader.get(), ir_dumper.get()) ||
     !LinkGlobalVars(greader.get(), ir_dumper.get())) {
    llvm::errs() << "Failed to link elements\n";
    return false;
  }
  if (!ir_dumper->Dump()) {
    llvm::errs() << "Serialization to ostream failed\n";
    return false;
  }
  return true;
}

static bool QueryRegexMatches(std::set<std::string> *regex_matched_link_set,
                              const std::regex *vs_regex,
                              const std::string &symbol) {
  assert(regex_matched_link_set != nullptr);
  assert(vs_regex != nullptr);
  if (regex_matched_link_set->find(symbol) != regex_matched_link_set->end()) {
    return false;
  }
  if (std::regex_search(symbol, *vs_regex)) {
    regex_matched_link_set->insert(symbol);
    return true;
  }
  return false;
}

static std::regex CreateRegexMatchExprFromSet(
    const std::set<std::string> &link_set) {
  std::string all_regex_match_str = "";
  std::set<std::string>::iterator it = link_set.begin();
  while (it != link_set.end()) {
    std::string regex_match_str_find_glob =
      abi_util::FindAndReplace(*it, "\\*", ".*");
    all_regex_match_str += "(\\b" + regex_match_str_find_glob + "\\b)";
    if (++it != link_set.end()) {
      all_regex_match_str += "|";
    }
  }
  if (all_regex_match_str == "") {
    return std::regex();
  }
  return std::regex(all_regex_match_str);
}

template <typename T>
inline bool HeaderAbiLinker::LinkDecl(
    abi_util::IRDumper *dst, std::set<std::string> *link_set,
    std::set<std::string> *regex_matched_link_set, const std::regex *vs_regex,
    const  std::map<std::string, T> &src, bool use_version_script) {
  assert(dst != nullptr);
  assert(link_set != nullptr);
  for (auto &&element : src) {
    // If we are not using a version script and exported headers are available,
    // filter out unexported abi.
    std::string source_file = element.second.GetSourceFile();
    // Builtin types will not have source file information.
    if (!exported_headers_.empty() && !source_file.empty() &&
        exported_headers_.find(source_file) ==
        exported_headers_.end()) {
      continue;
    }
    const std::string &element_str = element.first;
    // Check for the existence of the element in linked dump / symbol file.
    if (!use_version_script) {
      if (!link_set->insert(element_str).second) {
        continue;
      }
    } else {
      std::set<std::string>::iterator it =
          link_set->find(element_str);
      if (it == link_set->end()) {
        if (!QueryRegexMatches(regex_matched_link_set, vs_regex, element_str)) {
          continue;
        }
      } else {
        // We get a pre-filled link name set while using version script.
        link_set->erase(*it); // Avoid multiple instances of the same symbol.
      }
    }
    if (!dst->AddLinkableMessageIR(&(element.second))) {
      llvm::errs() << "Failed to add element to linked dump\n";
      return false;
    }

  }
  return true;
}

bool HeaderAbiLinker::LinkTypes(const abi_util::TextFormatToIRReader *reader,
                                abi_util::IRDumper *ir_dumper) {
  assert(reader != nullptr);
  assert(ir_dumper != nullptr);
  // Even if version scripts are available we take in types, since the symbols
  // in the version script might reference a type exposed by the library.
  return LinkDecl(ir_dumper, &types_set_, nullptr,
                  nullptr, reader->GetRecordTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetEnumTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetBuiltinTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetPointerTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetRvalueReferenceTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetLvalueReferenceTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetArrayTypes(), false) &&
      LinkDecl(ir_dumper, &types_set_, nullptr,
               nullptr, reader->GetQualifiedTypes(), false);
}

bool HeaderAbiLinker::LinkFunctions(
    const abi_util::TextFormatToIRReader *reader,
    abi_util::IRDumper *ir_dumper) {

  assert(reader != nullptr);
  return LinkDecl(ir_dumper, &function_decl_set_,
                  &functions_regex_matched_set, &functions_vs_regex_,
                  reader->GetFunctions(),
                  (!version_script_.empty() || !so_file_.empty()));
}

bool HeaderAbiLinker::LinkGlobalVars(
    const abi_util::TextFormatToIRReader *reader,
    abi_util::IRDumper *ir_dumper) {

  assert(reader != nullptr);
  return LinkDecl(ir_dumper, &globvar_decl_set_,
                  &globvars_regex_matched_set, &globvars_vs_regex_,
                  reader->GetGlobalVariables(),
                  (!version_script.empty() || !so_file_.empty()));

}

bool HeaderAbiLinker::ParseVersionScriptFiles() {
  abi_util::VersionScriptParser version_script_parser(version_script_, arch_,
                                                      api_);
  if (!version_script_parser.Parse()) {
    llvm::errs() << "Failed to parse version script\n";
    return false;
  }
  function_decl_set_ = version_script_parser.GetFunctions();
  globvar_decl_set_ = version_script_parser.GetGlobVars();
  std::set<std::string> function_regexs =
      version_script_parser.GetFunctionRegexs();
  std::set<std::string> globvar_regexs =
      version_script_parser.GetGlobVarRegexs();
  functions_vs_regex_ = CreateRegexMatchExprFromSet(function_regexs);
  globvars_vs_regex_ = CreateRegexMatchExprFromSet(globvar_regexs);
  return true;
}

bool HeaderAbiLinker::ParseSoFile() {
 auto Binary = llvm::object::createBinary(so_file_);

  if (!Binary) {
    llvm::errs() << "Couldn't really create object File \n";
    return false;
  }
  llvm::object::ObjectFile *objfile =
      llvm::dyn_cast<llvm::object::ObjectFile>(&(*Binary.get().getBinary()));
  if (!objfile) {
    llvm::errs() << "Not an object file\n";
    return false;
  }

  std::unique_ptr<abi_util::SoFileParser> so_parser =
      abi_util::SoFileParser::Create(objfile);
  if (so_parser == nullptr) {
    llvm::errs() << "Couldn't create soFile Parser\n";
    return false;
  }
  so_parser->GetSymbols();
  function_decl_set_ = so_parser->GetFunctions();
  globvar_decl_set_ = so_parser->GetGlobVars();
  return true;
}

int main(int argc, const char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "header-linker");
  if (so_file.empty() && version_script.empty()) {
    llvm::errs() << "One of -so or -v needs to be specified\n";
    return -1;
  }
  if (no_filter) {
    static_cast<std::vector<std::string> &>(exported_header_dirs).clear();
  }
  HeaderAbiLinker Linker(dump_files, exported_header_dirs, version_script,
                         so_file, linked_dump, arch, api);

  if (!Linker.LinkAndDump()) {
    llvm::errs() << "Failed to link and dump elements\n";
    return -1;
  }
  return 0;
}

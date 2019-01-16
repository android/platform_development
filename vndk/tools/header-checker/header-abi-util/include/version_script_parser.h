// Copyright (C) 2018 The Android Open Source Project
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

#ifndef VERSION_SCRIPT_PARSER_H_
#define VERSION_SCRIPT_PARSER_H_

#include "api_level.h"
#include "ir_representation.h"

#include <map>
#include <set>
#include <string>


namespace abi_util {


class VersionScriptParser {
 private:
  enum class LineScope {
    GLOBAL,
    LOCAL,
  };


  struct ParsedTags {
   public:
    unsigned has_arch_tags_ : 1;
    unsigned has_current_arch_tag_ : 1;
    unsigned has_introduced_tags_ : 1;
    unsigned has_excluded_tags_ : 1;
    unsigned has_future_tag_ : 1;
    unsigned has_var_tag_ : 1;
    ApiLevel introduced_;


   public:
    ParsedTags()
        : has_arch_tags_(0), has_current_arch_tag_(0), has_introduced_tags_(0),
          has_excluded_tags_(0), has_future_tag_(0), has_var_tag_(0),
          introduced_(-1) {}
  };


 public:
  class ErrorHandler {
   public:
    virtual ~ErrorHandler();

    virtual void OnError(int line_no, const std::string &error_msg) = 0;
  };


 public:
  VersionScriptParser(
      std::istream &version_script_stream, const std::string &arch,
      ApiLevel api_level,
      const std::set<std::string> &excluded_symbol_versions = {},
      const std::set<std::string> &excluded_symbol_tags = {},
      std::unique_ptr<ErrorHandler> error_handler = nullptr);

  bool Parse();

  const std::map<std::string, ElfFunctionIR> &GetFunctions() const {
    return functions_;
  }

  const std::map<std::string, ElfObjectIR> &GetGlobVars() const {
    return vars_;
  }

  std::set<std::string> GetFunctionRegexs() const {
    return {};
  }

  std::set<std::string> GetGlobVarRegexs() const {
    return {};
  }

  const std::set<std::string> &GetGlobPatterns() const {
    return glob_patterns_;
  }

  const std::set<std::string> &GetDemangledCppSymbols() const {
    return demangled_cpp_symbols_;
  }

  const std::set<std::string> &GetDemangledCppGlobPatterns() const {
    return demangled_cpp_glob_patterns_;
  }


 private:
  bool ReadLine(std::string &line);

  bool ParseVersionBlock(bool ignore_symbols);

  bool ParseSymbolLine(const std::string &line, bool is_cpp_symbol);

  ParsedTags ParseSymbolTags(const std::string &line);

  bool IsSymbolExported(const ParsedTags &tags);

  void AddVar(const std::string &symbol);

  void AddFunction(const std::string &symbol);


 private:
  void ReportError(const std::string &error_msg) {
    if (error_handler_) {
      error_handler_->OnError(line_no_, error_msg);
    }
  }


 private:
  std::unique_ptr<ErrorHandler> error_handler_;

  std::istream &stream_;
  const std::string &arch_;
  ApiLevel api_level_;
  const std::set<std::string> &excluded_symbol_versions_;
  const std::set<std::string> &excluded_symbol_tags_;

  int line_no_;
  std::string introduced_arch_tag_;

  std::map<std::string, ElfFunctionIR> functions_;
  std::map<std::string, ElfObjectIR> vars_;

  // Patterns only visible to symbol filters.
  std::set<std::string> glob_patterns_;
  std::set<std::string> demangled_cpp_glob_patterns_;
  std::set<std::string> demangled_cpp_symbols_;
};


}  // namespace abi_util


#endif  // VERSION_SCRIPT_PARSER_H_

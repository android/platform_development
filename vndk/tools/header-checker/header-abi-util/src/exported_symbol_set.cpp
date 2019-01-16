// Copyright (C) 2019 The Android Open Source Project
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

#include "exported_symbol_set.h"

#include "ir_representation.h"
#include "string_utils.h"

#include <fnmatch.h>
#include <cxxabi.h>


namespace abi_util {


static inline bool IsCppSymbol(const std::string &name) {
  return StartsWith(name, "_Z");
}


static inline bool HasMatchingGlobPattern(
    const ExportedSymbolSet::GlobPatternSet &patterns,
    const std::string &name) {
  for (auto &&pattern : patterns) {
    if (fnmatch(pattern.c_str(), name.c_str(), 0) == 0) {
      return true;
    }
  }
  return false;
}


void ExportedSymbolSet::AddFunction(const std::string &name,
                                    ElfSymbolIR::ElfSymbolBinding binding) {
  funcs_.emplace(name, ElfFunctionIR(name, binding));
}


void ExportedSymbolSet::AddVar(const std::string &name,
                               ElfSymbolIR::ElfSymbolBinding binding) {
  vars_.emplace(name, ElfObjectIR(name, binding));
}


class FreeDeleter {
 public:
  inline void operator()(void *ptr) const {
    ::free(ptr);
  }
};


bool ExportedSymbolSet::HasSymbol(const std::string &name) const {
  if (funcs_.find(name) != funcs_.end()) {
    return true;
  }

  if (vars_.find(name) != vars_.end()) {
    return true;
  }

  if (HasMatchingGlobPattern(glob_patterns_, name)) {
    return true;
  }

  if (IsCppSymbol(name)) {
    std::unique_ptr<char, FreeDeleter> demangled_name_c_str(
        abi::__cxa_demangle(name.c_str(), nullptr, nullptr, nullptr));

    if (demangled_name_c_str) {
      std::string demangled_name(demangled_name_c_str.get());

      if (demangled_cpp_symbols_.find(demangled_name) !=
          demangled_cpp_symbols_.end()) {
        return true;
      }

      if (HasMatchingGlobPattern(demangled_cpp_glob_patterns_,
                                 demangled_name)) {
        return true;
      }
    }
  }

  return false;
}


}  // namespace abi_util

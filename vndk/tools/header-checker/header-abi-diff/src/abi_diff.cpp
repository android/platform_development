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

#include<header_abi_util.h>

#include <llvm/Support/raw_ostream.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stdlib.h>

CompatibilityStatusIR HeaderAbiDiff::GenerateCompatibilityReport() {
using abi_util::TextFormatToIRReader;
  std::unique_ptr<abi_util::TextFormatToIRReader> old_reader =
      TextFormatToIRReader::CreateTextFormatToIRReader("protobuf", old_dump_);
  std::unique_ptr<abi_util::TextFormatToIRReader> new_reader =
      TextFormatToIRReader::CreateTextFormatToIRReader("protobuf", new_dump_);
  if (!old_reader || !new_reader || !old_reader->ReadDump() ||
      !new_reader->ReadDump()) {
    llvm::errs() << "Could not create Text Format readers\n";
    ::exit(1);
  }
  std::unique_ptr<abi_util::IRDiffDumper> ir_diff_dumper =
      abi_util::IRDiffDumper::CreateIRDiffDumper("protobuf", cr_);
  CompatibilityStatusIR status =
      CompareTUs(old_reader.get(), new_reader.get(), ir_diff_dumper.get());
  if (!ir_diff_dumper->Dump()) {
    llvm::errs() << "Could not dump diff report\n";
    ::exit(1);
  }
  return status;
}

template <typename F>
static void AddTypesToMap(std::map<std::string, const abi_util::TypeIR *> *dst,
                          const abi_util::TextFormatToIRReader *tu, F func) {
  AddToMap(dst, tu->GetRecordTypes(), func);
  AddToMap(dst, tu->GetEnumTypes(), func);
  AddToMap(dst, tu->GetPointerTypes(), func);
  AddToMap(dst, tu->GetBuiltinTypes(), func);
  AddToMap(dst, tu->GetArrayTypes(), func);
  AddToMap(dst, tu->GetLvalueReferenceTypes(), func);
  AddToMap(dst, tu->GetRvalueReferenceTypes(), func);
  AddToMap(dst, tu->GetQualifiedTypes(), func);
}

CompatibilityStatusIR HeaderAbiDiff::CompareTUs(
    const abi_util::TextFormatToIRReader *old_tu,
    const abi_util::TextFormatToIRReader *new_tu,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  // Collect all old and new types in maps, so that we can refer to them by
  // type name / linker_set_key later.
  std::map<std::string, const abi_util::TypeIR *> old_types;
  std::map<std::string, const abi_util::TypeIR *> new_types;
  AddTypesToMap(&old_types, old_tu,
                [](const abi_util::TypeIR *e) {return e->GetLinkerSetKey();});
  AddTypesToMap(&new_types, new_tu,
                [](const abi_util::TypeIR *e) {return e->GetLinkerSetKey();});

  // Collect fills in added, removed ,unsafe and safe function diffs.
  if (!CollectDynsymExportables(old_tu->GetFunctions(), new_tu->GetFunctions(),
                               old_types, new_types, ir_diff_dumper) ||
      CollectDynsymExportables(old_tu->GetGlobalVariables(),
                               new_tu->GetGlobalVariables(), old_types,
                               new_types,ir_diff_dumper)) {
    llvm::errs() << "Unable to collect dynsym exportables\n";
    ::exit(1);
  }

  //TODO:Fix this by taking in information from ir_diff_dumper.
  CompatibilityStatusIR combined_status = CompatibilityStatusIR::COMPATIBLE;

  if (combined_status & CompatibilityStatusIR::INCOMPATIBLE) {
    combined_status = CompatibilityStatusIR::INCOMPATIBLE;
  } else if (combined_status & CompatibilityStatusIR::EXTENSION) {
    combined_status = CompatibilityStatusIR::EXTENSION;
  } else {
    combined_status = CompatibilityStatusIR::COMPATIBLE;
  }
  ir_diff_dumper->AddLibNameIR(lib_name_);
  ir_diff_dumper->AddArchIR(arch_);
  ir_diff_dumper->AddCompatibilityStatusIR(combined_status);
  return combined_status;
}

template <typename T>
bool HeaderAbiDiff::CollectDynsymExportables(
    const std::vector<T> &old_exportables,
    const std::vector<T> &new_exportables,
    const std::map<std::string, const abi_util::TypeIR *> &old_types_map,
    const std::map<std::string, const abi_util::TypeIR *> &new_types_map,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  std::map<std::string, const T *> old_exportables_map;
  std::map<std::string, const T *> new_exportables_map;
  abi_util::AddToMap(&old_exportables_map, old_exportables,
                     [](const T *e)
                     { return e->GetLinkerSetKey();});
  abi_util::AddToMap(&new_exportables_map, new_exportables,
                     [](const T *e)
                     { return e->GetLinkerSetKey();});

  if (!Collect(old_exportables_map,
               new_exportables_map, ir_diff_dumper) ||
      !PopulateCommonElements(old_exportables_map, new_exportables_map,
                              old_types_map, new_types_map, ir_diff_dumper)) {
    return false;
  }
  return true;
}

// Collect added and removed Elements
template <typename T>
bool HeaderAbiDiff::Collect(
    const std::map<std::string, const T*> &old_elements_map,
    const std::map<std::string, const T*> &new_elements_map,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  if (!PopulateRemovedElements(old_elements_map, new_elements_map,
                               ir_diff_dumper) ||
      !PopulateRemovedElements(new_elements_map, old_elements_map,
                               ir_diff_dumper)) {
    llvm::errs() << "Populating functions in report failed\n";
    return false;
  }
  return true;
}

template <typename T>
bool HeaderAbiDiff::PopulateRemovedElements(
    const std::map<std::string, const T*> &old_elements_map,
    const std::map<std::string, const T*> &new_elements_map,
    abi_util::IRDiffDumper *ir_diff_dumper) {

  llvm::errs() << "old  elements size :" << old_elements_map.size() <<"\n";
  llvm::errs() << "new  elements size :" << new_elements_map.size() <<"\n";
  std::vector<const T *> removed_elements =
      abi_util::FindRemovedElements(old_elements_map, new_elements_map);
  if (!DumpLoneElements(removed_elements, ir_diff_dumper)) {
    llvm::errs() << "Dumping added / removed element to report failed\n";
    return false;
  }
  return true;
}

template <typename T>
bool HeaderAbiDiff::PopulateCommonElements(
    const std::map<std::string, const T *> &old_elements_map,
    const std::map<std::string, const T *> &new_elements_map,
    const std::map<std::string, const abi_util::TypeIR *> &old_types,
    const std::map<std::string, const abi_util::TypeIR *> &new_types,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  std::vector<std::pair<const T *, const T *>> common_elements =
      abi_util::FindCommonElements(old_elements_map, new_elements_map);
  if (!DumpDiffElements(common_elements, old_types, new_types,
                        ir_diff_dumper)) {
    llvm::errs() << "Dumping difference in common element to report failed\n";
    return false;
  }
  return true;
}

template <typename T>
bool HeaderAbiDiff::DumpLoneElements(
    std::vector<const T *> &elements,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  //llvm::errs() << "Dumping lone  elements size :" << elements.size() <<"\n";
  for (auto &&element : elements) {
    if (abi_diff_wrappers::IgnoreSymbol<T>(
        element, ignored_symbols_,
        [](const T *e) {return e->GetLinkerSetKey();})) {
      continue;
    }
    if (!ir_diff_dumper->AddLinkableMessageIR(element)) {
      llvm::errs() << "Couldn't dump added /removed element\n";
      return false;
    }
  }
  return true;
}

static std::set<std::string> type_cache;

template <typename T>
bool HeaderAbiDiff::DumpDiffElements(
    std::vector<std::pair<const T *,const T *>> &pairs,
    const std::map<std::string, const abi_util::TypeIR *> &old_types,
    const std::map<std::string, const abi_util::TypeIR *> &new_types,
    abi_util::IRDiffDumper *ir_diff_dumper) {
  for (auto &&pair : pairs) {
    const T *old_element = pair.first;
    const T *new_element = pair.second;

    if (abi_diff_wrappers::IgnoreSymbol<T>(
        old_element, ignored_symbols_,
        [](const T *e) {return e->GetLinkerSetKey();})) {
      continue;
    }
    abi_diff_wrappers::DiffWrapper<T> diff_wrapper(old_element, new_element,
                                                   ir_diff_dumper, old_types,
                                                   new_types, &type_cache);
    //TODO: Enable this
    if (!diff_wrapper.DumpDiff()) {
      return false;
    }
  }
  return true;
}

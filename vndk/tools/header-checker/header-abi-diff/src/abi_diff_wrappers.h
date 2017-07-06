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

#ifndef ABI_DIFF_WRAPPERS_H
#define ABI_DIFF_WRAPPERS_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "proto/abi_dump.pb.h"
#include "proto/abi_diff.pb.h"
#pragma clang diagnostic pop

#include<ir_representation.h>

#include <queue>

namespace abi_diff_wrappers {

template <typename T, typename F>
static bool IgnoreSymbol(const T *element,
                         const std::set<std::string> &ignored_symbols,
                         F func) {
  return ignored_symbols.find(func(element)) !=
      ignored_symbols.end();
}

enum DiffStatus {
  // Previous stages of CompareAndTypeDiff should not dump the diff.
  no_diff = 0,
  // Previous stages of CompareAndTypeDiff should dump the diff if required.
  direct_diff = 1,
};

class DiffWrapperBase {
 public:
  virtual bool DumpDiff() = 0 ;
  virtual ~DiffWrapperBase() {};
 protected:
  DiffWrapperBase(
      abi_util::IRDiffDumper *ir_diff_dumper,
      const std::map<std::string, const abi_util::TypeIR *> &old_types,
      const std::map<std::string, const abi_util::TypeIR *> &new_types,
      std::set<std::string> *type_cache)
      : ir_diff_dumper_(ir_diff_dumper),
        old_types_(old_types), new_types_(new_types),
        type_cache_(type_cache) { }

  DiffStatus CompareAndDumpTypeDiff(const std::string &old_type_str,
                                    const std::string &new_type_str,
                                    std::queue<std::string> *type_queue);

  DiffStatus CompareAndDumpTypeDiff(
    const abi_util::TypeIR *old_type, const abi_util::TypeIR *new_type,
    abi_util::LinkableMessageKind kind,
    std::queue<std::string> *type_queue);


  DiffStatus CompareRecordTypes(const abi_util::RecordTypeIR *old_type,
                                const abi_util::RecordTypeIR *new_type,
                                std::queue<std::string> *type_queue);

  DiffStatus CompareQualifiedTypes(const abi_util::QualifiedTypeIR *old_type,
                                   const abi_util::QualifiedTypeIR *new_type,
                                   std::queue<std::string> *type_queue);

  DiffStatus ComparePointerTypes(const abi_util::PointerTypeIR *old_type,
                                 const abi_util::PointerTypeIR *new_type,
                                 std::queue<std::string> *type_queue);

  DiffStatus CompareLvalueReferenceTypes(
      const abi_util::LvalueReferenceTypeIR *old_type,
      const abi_util::LvalueReferenceTypeIR *new_type,
      std::queue<std::string> *type_queue);

  DiffStatus CompareRvalueReferenceTypes(
      const abi_util::RvalueReferenceTypeIR *old_type,
      const abi_util::RvalueReferenceTypeIR *new_type,
      std::queue<std::string> *type_queue);


  DiffStatus CompareBuiltinTypes(const abi_util::BuiltinTypeIR *old_type,
                                 const abi_util::BuiltinTypeIR *new_type);
  DiffStatus CompareEnumFields(
    const std::vector<abi_util::EnumFieldIR> &old_fields,
    const std::vector<abi_util::EnumFieldIR> &new_fields);

  DiffStatus CompareEnumTypes(const abi_util::EnumTypeIR *old_type,
                              const abi_util::EnumTypeIR *new_type,
                              std::queue<std::string> *type_queue);

  DiffStatus CompareCommonRecordFields(
    const abi_util::RecordFieldIR *old_field,
    const abi_util::RecordFieldIR *new_field,
    std::queue<std::string> *type_queue);

  DiffStatus CompareRecordFields(
    const std::vector<abi_util::RecordFieldIR> &old_fields,
    const std::vector<abi_util::RecordFieldIR> &new_fields,
    std::queue<std::string> *type_queue);

  DiffStatus CompareFunctionParameters(
    const std::vector<abi_util::ParamIR> &old_parameters,
    const std::vector<abi_util::ParamIR> &new_parameters,
    std::queue<std::string> *type_queue);

  bool CompareVTables(const abi_util::RecordTypeIR *old_record,
                      const abi_util::RecordTypeIR *new_record);

  bool CompareVTableComponents(
      const abi_util::VTableComponentIR &old_component,
      const abi_util::VTableComponentIR &new_component);

  bool CompareSizeAndAlignment(const abi_util::TypeIR *old_ti,
                               const abi_util::TypeIR *new_ti);

  template <typename DiffType, typename DiffElement>
  bool AddToDiff(DiffType *mutable_diff, const DiffElement *oldp,
                 const DiffElement *newp,
                 std::queue<std::string> *type_queue = nullptr);
 protected:
  abi_util::IRDiffDumper *ir_diff_dumper_;
  const std::map<std::string, const abi_util::TypeIR *> &old_types_;
  const std::map<std::string, const abi_util::TypeIR *> &new_types_;
  std::set<std::string> *type_cache_;
};

template <typename T>
class DiffWrapper : public DiffWrapperBase {
 public:
  DiffWrapper(const T *oldp, const T *newp,
              abi_util::IRDiffDumper *ir_diff_dumper,
              const std::map<std::string, const abi_util::TypeIR *> &old_types,
              const std::map<std::string, const abi_util::TypeIR *> &new_types,
              std::set<std::string> *type_cache)
      : DiffWrapperBase(ir_diff_dumper, old_types, new_types,
                           type_cache), oldp_(oldp), newp_(newp) { }
  virtual bool DumpDiff() override;
 private:
  const T *oldp_;
  const T *newp_;
};

} // abi_diff_wrappers

#endif // ABI_DIFF_WRAPPERS_H

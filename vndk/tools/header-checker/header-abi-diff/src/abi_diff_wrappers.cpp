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

#include "abi_diff_wrappers.h"

#include<header_abi_util.h>

#include <llvm/Support/raw_ostream.h>

namespace abi_diff_wrappers {


static bool IsAccessDownGraded(abi_util::AccessSpecifierIR old_access,
                               abi_util::AccessSpecifierIR new_access) {
  bool access_downgraded = false;
  switch (old_access) {
    case abi_util::AccessSpecifierIR::protected_access:
      if (new_access == abi_util::AccessSpecifierIR::private_access) {
        access_downgraded = true;
      }
      break;
    case abi_util::AccessSpecifierIR::public_access:
      if (new_access != abi_util::AccessSpecifierIR::public_access) {
        access_downgraded = true;
      }
      break;
    default:
      break;
  }
  return access_downgraded;
}

static std::string Unwind(const std::queue<std::string> *type_queue) {
  if (!type_queue) {
    return "";
  }
  std::string stack_str;
  std::queue<std::string> type_queue_copy = *type_queue;
  while (!type_queue_copy.empty()) {
    stack_str += type_queue_copy.front() + "-> ";
    type_queue_copy.pop();
  }
  return stack_str;
}

void DiffWrapperBase::CompareEnumFields(
    const std::vector<abi_util::EnumFieldIR> &old_fields,
    const std::vector<abi_util::EnumFieldIR> &new_fields,
    abi_util::EnumTypeDiffIR *enum_type_diff_ir) {
  std::map<std::string, const abi_util::EnumFieldIR *> old_fields_map;
  std::map<std::string, const abi_util::EnumFieldIR *> new_fields_map;
  abi_util::AddToMap(&old_fields_map, old_fields,
                     [](const abi_util::EnumFieldIR *f)
                     {return f->GetName();});
  abi_util::AddToMap(&new_fields_map, new_fields,
                     [](const abi_util::EnumFieldIR *f)
                     {return f->GetName();});
  std::vector<const abi_util::EnumFieldIR *> removed_fields =
      abi_util::FindRemovedElements(old_fields_map, new_fields_map);

  std::vector<const abi_util::EnumFieldIR *> added_fields =
      abi_util::FindRemovedElements(new_fields_map, old_fields_map);

  enum_type_diff_ir->SetFieldsAdded(std::move(added_fields));

  enum_type_diff_ir->SetFieldsRemoved(std::move(removed_fields));

  std::vector<std::pair<
      const abi_util::EnumFieldIR *, const abi_util::EnumFieldIR *>> cf =
      abi_util::FindCommonElements(old_fields_map, new_fields_map);
  std::vector<abi_util::EnumFieldDiffIR> enum_field_diffs;
  for (auto &&common_fields : cf) {
    if (common_fields.first->GetValue() != common_fields.second->GetValue()) {
      abi_util::EnumFieldDiffIR enum_field_diff_ir(common_fields.first,
                                                   common_fields.second);
      enum_field_diffs.emplace_back(std::move(enum_field_diff_ir));
    }
  }
  enum_type_diff_ir->SetFieldsDiff(std::move(enum_field_diffs));
}

DiffStatus DiffWrapperBase::CompareEnumTypes(
    const abi_util::EnumTypeIR *old_type, const abi_util::EnumTypeIR *new_type,
     std::queue<std::string> *type_queue,
     abi_util::IRDiffDumper::DiffKind diff_kind) {
  if (old_type->GetName() != new_type->GetName()) {
    return DiffStatus::direct_diff;
  }
  std::unique_ptr<abi_util::EnumTypeDiffIR>
      enum_type_diff_ir(new abi_util::EnumTypeDiffIR());
  enum_type_diff_ir->SetName(old_type->GetName());
  const std::string &old_underlying_type = old_type->GetUnderlyingType();
  const std::string &new_underlying_type = new_type->GetUnderlyingType();
  if (old_underlying_type != new_underlying_type) {
    enum_type_diff_ir->SetUnderlyingTypeDiff(
        std::unique_ptr<std::pair<std::string, std::string>>(
            new std::pair<std::string, std::string>(old_underlying_type,
                                                    new_underlying_type)));
  }
  CompareEnumFields(old_type->GetFields(), new_type->GetFields(),
                    enum_type_diff_ir.get());
  if ((enum_type_diff_ir->IsExtended() ||
       enum_type_diff_ir->IsIncompatible()) &&
      !ir_diff_dumper_->AddDiffMessageIR(enum_type_diff_ir.get(),
                                         Unwind(type_queue), diff_kind)) {
    llvm::errs() << "AddDiffMessage on EnumTypeDiffIR failed\n";
    ::exit(1);
  }
  return DiffStatus::no_diff;
}

bool DiffWrapperBase::CompareVTableComponents(
    const abi_util::VTableComponentIR &old_component,
    const abi_util::VTableComponentIR &new_component) {
  return old_component.GetName() == new_component.GetName() &&
      old_component.GetValue() == new_component.GetValue() &&
      old_component.GetKind() == new_component.GetKind();
}

bool DiffWrapperBase::CompareVTables(
    const abi_util::RecordTypeIR *old_record,
    const abi_util::RecordTypeIR *new_record) {

  const std::vector<abi_util::VTableComponentIR> &old_components =
      old_record->GetVTableLayout().GetVTableComponents();
  const std::vector<abi_util::VTableComponentIR> &new_components =
      new_record->GetVTableLayout().GetVTableComponents();
  if (old_components.size() > new_components.size()) {
    // Something in the vtable got deleted.
    return false;
  }
  uint32_t i = 0;
  while (i < old_components.size()) {
    auto &old_element = old_components.at(i);
    auto &new_element = new_components.at(i);
    if (!CompareVTableComponents(old_element, new_element)) {
      return false;
    }
    i++;
  }
  return true;
}

bool DiffWrapperBase::CompareSizeAndAlignment(
    const abi_util::TypeIR *old_type,
    const abi_util::TypeIR *new_type) {
  return old_type->GetSize() == new_type->GetSize() &&
      old_type->GetAlignment() == new_type->GetAlignment();
}

std::unique_ptr<abi_util::RecordFieldDiffIR>
DiffWrapperBase::CompareCommonRecordFields(
    const abi_util::RecordFieldIR *old_field,
    const abi_util::RecordFieldIR *new_field,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  if ((old_field->GetOffset() != new_field->GetOffset()) ||
      // TODO: Should this be an inquality check instead ? Some compilers can
      // make signatures dependant on absolute values of access specifiers.
      IsAccessDownGraded(old_field->GetAccess(), new_field->GetAccess()) ||
      CompareAndDumpTypeDiff(old_field->GetReferencedType(),
                             new_field->GetReferencedType(),
                             type_queue, diff_kind) ==
      DiffStatus::direct_diff) {
    return std::unique_ptr<abi_util::RecordFieldDiffIR>(
        new abi_util::RecordFieldDiffIR(old_field, new_field));
  }
  return nullptr;
}

std::pair<std::vector<abi_util::RecordFieldDiffIR>, std::vector<const abi_util::RecordFieldIR *>>
DiffWrapperBase::CompareRecordFields(
    const std::vector<abi_util::RecordFieldIR> &old_fields,
    const std::vector<abi_util::RecordFieldIR> &new_fields,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::pair<std::vector<abi_util::RecordFieldDiffIR>,
  std::vector<const abi_util::RecordFieldIR *>> diffed_and_removed_fields;
  std::map<std::string, const abi_util::RecordFieldIR *> old_fields_map;
  std::map<std::string, const abi_util::RecordFieldIR *> new_fields_map;
  abi_util::AddToMap(&old_fields_map, old_fields,
                     [](const abi_util::RecordFieldIR *f)
                     {return f->GetName();});
  abi_util::AddToMap(&new_fields_map, new_fields,
                     [](const abi_util::RecordFieldIR *f)
                     {return f->GetName();});
  std::vector<const abi_util::RecordFieldIR *> removed_fields =
      abi_util::FindRemovedElements(old_fields_map, new_fields_map);
  diffed_and_removed_fields.second = std::move(removed_fields);
  std::vector<std::pair<
      const abi_util::RecordFieldIR *, const abi_util::RecordFieldIR *>> cf =
      abi_util::FindCommonElements(old_fields_map, new_fields_map);
  for (auto &&common_fields : cf) {
    std::unique_ptr<abi_util::RecordFieldDiffIR> diffed_field_ptr =
        CompareCommonRecordFields(common_fields.first, common_fields.second,
                                  type_queue, diff_kind);
    if (diffed_field_ptr != nullptr) {

      diffed_and_removed_fields.first.emplace_back(
          std::move(*(diffed_field_ptr.release())));
    }
  }
  return diffed_and_removed_fields;
}

bool DiffWrapperBase::CompareBaseSpecifiers(
    const std::vector<abi_util::CXXBaseSpecifierIR> &old_base_specifiers,
    const std::vector<abi_util::CXXBaseSpecifierIR> &new_base_specifiers,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  if (old_base_specifiers.size() != new_base_specifiers.size()) {
    return false;
  }
  int i = 0;
  while (i < old_base_specifiers.size()) {
    if (CompareAndDumpTypeDiff(old_base_specifiers.at(i).GetReferencedType(),
                               new_base_specifiers.at(i).GetReferencedType(),
                               type_queue, diff_kind) ==
        DiffStatus::direct_diff) {
      return false;
    }
    i++;
  }
  return true;
}

DiffStatus DiffWrapperBase::CompareRecordTypes(
    const abi_util::RecordTypeIR *old_type,
    const abi_util::RecordTypeIR *new_type,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::unique_ptr<abi_util::RecordTypeDiffIR>
      record_type_diff_ir(new abi_util::RecordTypeDiffIR());
  // Compare names.
  if (old_type->GetName() != new_type->GetName()) {
    // Do not dump anything since the record types themselves are fundamentally
    // different.
    return DiffStatus::direct_diff;
  }
  record_type_diff_ir->SetName(old_type->GetName());
  if (old_type->GetAccess() != new_type->GetAccess()) {
    record_type_diff_ir->SetAccessDiff(
        std::unique_ptr<abi_util::AccessSpecifierDiffIR>(
            new abi_util::AccessSpecifierDiffIR(old_type->GetAccess(),
                                                new_type->GetAccess())));
  }

  if (!CompareSizeAndAlignment(old_type, new_type)) {
    record_type_diff_ir->SetTypeDiff(
        std::unique_ptr<abi_util::TypeDiffIR>(
            new abi_util::TypeDiffIR(
                std::make_pair(old_type->GetSize(), new_type->GetSize()),
                std::make_pair(old_type->GetAlignment(),
                               new_type->GetAlignment()))));
  }
  if (!CompareVTables(old_type, new_type)) {
    record_type_diff_ir->SetVTableLayoutDiff(
        std::unique_ptr<abi_util::VTableLayoutDiffIR>(
            new abi_util::VTableLayoutDiffIR(old_type->GetVTableLayout(),
                                             new_type->GetVTableLayout())));
  }
  std::pair<std::vector<abi_util::RecordFieldDiffIR>, std::vector<const abi_util::RecordFieldIR *>>
      field_diffs;
  field_diffs = CompareRecordFields(old_type->GetFields(),
                                    new_type->GetFields(), type_queue,
                                    diff_kind);
  record_type_diff_ir->SetFieldDiffs(std::move(field_diffs.first));
  record_type_diff_ir->SetFieldsRemoved(std::move(field_diffs.second));
  const std::vector<abi_util::CXXBaseSpecifierIR> &old_bases =
      old_type->GetBases();
  const std::vector<abi_util::CXXBaseSpecifierIR> &new_bases =
      new_type->GetBases();

  if (!CompareBaseSpecifiers(old_bases, new_bases, type_queue, diff_kind)) {
    record_type_diff_ir->SetBaseSpecifierDiffs(
        std::unique_ptr<abi_util::CXXBaseSpecifierDiffIR>(new
            abi_util::CXXBaseSpecifierDiffIR(old_bases, new_bases)));
  }
  if (record_type_diff_ir->DiffExists() &&
      !ir_diff_dumper_->AddDiffMessageIR(record_type_diff_ir.get(),
                                         Unwind(type_queue), diff_kind)) {
    llvm::errs() << "AddDiffMessage on record type failed\n";
    ::exit(1);
  } // No need to add a dump for an extension since records can't be "extended".

  // TODO: Compare TemplateInfo. Template Diffs do not need to be
  // added though since they will appear in the linker set key.
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareLvalueReferenceTypes(
    const abi_util::LvalueReferenceTypeIR *old_type,
    const abi_util::LvalueReferenceTypeIR *new_type,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue, diff_kind);
}

DiffStatus DiffWrapperBase::CompareRvalueReferenceTypes(
    const abi_util::RvalueReferenceTypeIR *old_type,
    const abi_util::RvalueReferenceTypeIR *new_type,
     std::queue<std::string> *type_queue,
     abi_util::IRDiffDumper::DiffKind diff_kind) {
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue, diff_kind);
}

DiffStatus DiffWrapperBase::CompareQualifiedTypes(
    const abi_util::QualifiedTypeIR *old_type,
    const abi_util::QualifiedTypeIR *new_type,
     std::queue<std::string> *type_queue,
     abi_util::IRDiffDumper::DiffKind diff_kind) {
  // If all the qualifiers are not the same, return direct_diff, else
  // recursively compare the unqualified types.
  if (old_type->IsConst() != new_type->IsConst() ||
      old_type->IsVolatile() != new_type->IsVolatile() ||
      old_type->IsRestricted() != new_type->IsRestricted()) {
    return DiffStatus::direct_diff;
  }
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue, diff_kind);
}

DiffStatus DiffWrapperBase::ComparePointerTypes(
    const abi_util::PointerTypeIR *old_type,
    const abi_util::PointerTypeIR *new_type,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  // The following need to be the same for two pointer types to be considered
  // equivalent:
  // 1) Number of pointer indirections are the same.
  // 2) The ultimate pointee is the same.
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue, diff_kind);
}

DiffStatus DiffWrapperBase::CompareBuiltinTypes(
    const abi_util::BuiltinTypeIR *old_type,
    const abi_util::BuiltinTypeIR *new_type) {
  // If the size, alignment and is_unsigned are the same, return no_diff
  // else return direct_diff.
  uint64_t old_signedness = old_type->IsUnsigned();
  uint64_t new_signedness = new_type->IsUnsigned();

  if (!CompareSizeAndAlignment(old_type, new_type) ||
      old_signedness != new_signedness) {
    return DiffStatus::direct_diff;
  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareFunctionParameters(
    const std::vector<abi_util::ParamIR> &old_parameters,
    const std::vector<abi_util::ParamIR> &new_parameters,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  if (old_parameters.size() != new_parameters.size()) {
    return DiffStatus::direct_diff;
  }
  uint64_t i = 0;
  while (i < old_parameters.size()) {
    const abi_util::ParamIR &old_parameter = old_parameters.at(i);
    const abi_util::ParamIR &new_parameter = new_parameters.at(i);
    if (CompareAndDumpTypeDiff(old_parameter.GetReferencedType(),
                               new_parameter.GetReferencedType(),
                               type_queue, diff_kind) ==
        DiffStatus::direct_diff ||
        (old_parameter.GetIsDefault() != new_parameter.GetIsDefault())) {
      llvm::errs() << "function direct diff returned\n";
      return DiffStatus::direct_diff;
    }
    i++;
  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareAndDumpTypeDiff(
    const abi_util::TypeIR *old_type, const abi_util::TypeIR *new_type,
    abi_util::LinkableMessageKind kind, std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  if (kind == abi_util::LinkableMessageKind::BuiltinTypeKind) {
    return CompareBuiltinTypes(
        static_cast<const abi_util::BuiltinTypeIR *>(old_type),
        static_cast<const abi_util::BuiltinTypeIR *>(new_type));
  }

  if (kind == abi_util::LinkableMessageKind::QualifiedTypeKind) {
    return CompareQualifiedTypes(
        static_cast<const abi_util::QualifiedTypeIR *>(old_type),
        static_cast<const abi_util::QualifiedTypeIR *>(new_type),
        type_queue, diff_kind);
  }

  if (kind == abi_util::LinkableMessageKind::EnumTypeKind) {
      return CompareEnumTypes(
          static_cast<const abi_util::EnumTypeIR *>(old_type),
          static_cast<const abi_util::EnumTypeIR *>(new_type),
          type_queue, diff_kind);

  }

  if (kind == abi_util::LinkableMessageKind::LvalueReferenceTypeKind) {
    return CompareLvalueReferenceTypes(
        static_cast<const abi_util::LvalueReferenceTypeIR *>(old_type),
        static_cast<const abi_util::LvalueReferenceTypeIR *>(new_type),
        type_queue, diff_kind);

  }

  if (kind == abi_util::LinkableMessageKind::RvalueReferenceTypeKind) {
    return CompareRvalueReferenceTypes(
        static_cast<const abi_util::RvalueReferenceTypeIR *>(old_type),
        static_cast<const abi_util::RvalueReferenceTypeIR *>(new_type),
        type_queue, diff_kind);

  }

  if (kind == abi_util::LinkableMessageKind::PointerTypeKind) {
    return ComparePointerTypes(
        static_cast<const abi_util::PointerTypeIR *>(old_type),
        static_cast<const abi_util::PointerTypeIR *>(new_type),
        type_queue, diff_kind);
  }

  if (kind == abi_util::LinkableMessageKind::RecordTypeKind) {
    return CompareRecordTypes(
        static_cast<const abi_util::RecordTypeIR *>(old_type),
        static_cast<const abi_util::RecordTypeIR *>(new_type),
        type_queue, diff_kind);
  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareAndDumpTypeDiff(
    const std::string &old_type_str, const std::string &new_type_str,
    std::queue<std::string> *type_queue,
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  //TODO: Check for exceptional cases on new_type_str and old_type_str :
  // uintptr_t

  // If either of the types are not found in their respective maps, the type
  // was not exposed in a public header and we do a simple string comparison.
  // Any diff found using a simple string comparison will be a direct diff.

  // Check the map for types which have already been compared
  bool same_type_str = (old_type_str == new_type_str);
  if (same_type_str) {
    // These types have already been diffed, return without further comparison.
    if (!type_cache_->insert(old_type_str).second) {
      return DiffStatus::no_diff;
    }
    type_queue->push(old_type_str);
  }
  std::map<std::string, const abi_util::TypeIR *>::const_iterator old_it =
      old_types_.find(old_type_str);
  std::map<std::string, const abi_util::TypeIR *>::const_iterator new_it =
      new_types_.find(new_type_str);
  if (old_it == old_types_.end() || new_it == new_types_.end()) {
    // Do a simple string comparison.
    return (old_type_str == new_type_str) ?
        DiffStatus::no_diff : DiffStatus::direct_diff;
  }
  abi_util::LinkableMessageKind old_kind =
      old_it->second->GetKind();
  abi_util::LinkableMessageKind new_kind =
      new_it->second->GetKind();

  if (old_kind != new_kind) {
    return DiffStatus::direct_diff;
  }
  return CompareAndDumpTypeDiff(old_it->second , new_it->second , old_kind,
                                type_queue, diff_kind);
}

template <>
bool DiffWrapper<abi_util::RecordTypeIR>::DumpDiff(
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::queue<std::string> type_queue;
  if (oldp_->GetName() != newp_->GetName()) {
    llvm::errs() << "Comparing two different unreferenced records\n";
    return false;
  }
  if (!type_cache_->insert(oldp_->GetName()).second) {
      return true;
  }
  return CompareRecordTypes(oldp_, newp_, &type_queue, diff_kind);
}

template <>
bool DiffWrapper<abi_util::EnumTypeIR>::DumpDiff(
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::queue<std::string> type_queue;
  if (oldp_->GetName() != newp_->GetName()) {
    llvm::errs() << "Comparing two different unreferenced enums\n";
    return false;
  }
  if (!type_cache_->insert(oldp_->GetName()).second) {
      return true;
  }
  return CompareEnumTypes(oldp_, newp_, &type_queue, diff_kind);
}

template <>
bool DiffWrapper<abi_util::GlobalVarIR>::DumpDiff(
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::queue<std::string> type_queue;
  DiffStatus type_diff = CompareAndDumpTypeDiff(oldp_->GetReferencedType(),
                                                newp_->GetReferencedType(),
                                                &type_queue, diff_kind);
  DiffStatus access_diff = (oldp_->GetAccess() == newp_->GetAccess()) ?
      DiffStatus::no_diff : DiffStatus::direct_diff;
  if ((type_diff | access_diff) & DiffStatus::direct_diff) {
    abi_util::GlobalVarDiffIR global_var_diff_ir(oldp_, newp_);
    global_var_diff_ir.SetName(oldp_->GetName());
    return ir_diff_dumper_->AddDiffMessageIR(&global_var_diff_ir,
                                             Unwind(&type_queue), diff_kind);
  }
  return true;
}

template <>
bool DiffWrapper<abi_util::FunctionIR>::DumpDiff(
    abi_util::IRDiffDumper::DiffKind diff_kind) {
  std::queue<std::string> type_queue;
  type_queue.push(oldp_->GetLinkerSetKey());
  DiffStatus param_diffs = CompareFunctionParameters(oldp_->GetParameters(),
                                                     newp_->GetParameters(),
                                                     &type_queue, diff_kind);
  DiffStatus return_type_diff =
      CompareAndDumpTypeDiff(oldp_->GetReferencedType(),
                             newp_->GetReferencedType(),
                             &type_queue, diff_kind);
  if ((param_diffs == DiffStatus::direct_diff ||
       return_type_diff == DiffStatus::direct_diff) ||
      (oldp_->GetAccess() != newp_->GetAccess())) {
    abi_util::FunctionDiffIR function_diff_ir(oldp_, newp_);
    function_diff_ir.SetName(oldp_->GetName());
    return ir_diff_dumper_->AddDiffMessageIR(&function_diff_ir,
                                             Unwind(&type_queue), diff_kind);
  }
  return true;
}
} // abi_diff_wrappers

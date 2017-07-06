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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#include "proto/abi_dump.pb.h"
#include "proto/abi_diff.pb.h"
#pragma clang diagnostic pop

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

DiffStatus DiffWrapperBase::CompareEnumFields(
    const std::vector<abi_util::EnumFieldIR> &old_fields,
    const std::vector<abi_util::EnumFieldIR> &new_fields) {
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
  if (removed_fields.size() > 0) {
    return DiffStatus::direct_diff;
  }
  std::vector<std::pair<
      const abi_util::EnumFieldIR *, const abi_util::EnumFieldIR *>> cf =
      abi_util::FindCommonElements(old_fields_map, new_fields_map);
  for (auto &&common_fields : cf) {
    if (common_fields.first->GetValue() != common_fields.second->GetValue()) {
      return DiffStatus::direct_diff;
    }
  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareEnumTypes(
    const abi_util::EnumTypeIR *old_type, const abi_util::EnumTypeIR *new_type,
     std::queue<std::string> *type_queue) {
  if (old_type->GetName() != new_type->GetName()) {
    return DiffStatus::direct_diff;
  }
  if (old_type->GetUnderlyingType() != new_type->GetUnderlyingType() ||
      !CompareEnumFields(old_type->GetFields(), new_type->GetFields())) {
    // TODO: Add ir_diff_dumper-> call here
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
  }
  return true;
}

bool DiffWrapperBase::CompareSizeAndAlignment(
    const abi_util::TypeIR *old_type,
    const abi_util::TypeIR *new_type) {
  return old_type->GetSize() == new_type->GetSize() &&
      old_type->GetAlignment() == new_type->GetAlignment();
}

DiffStatus DiffWrapperBase::CompareCommonRecordFields(
    const abi_util::RecordFieldIR *old_field,
    const abi_util::RecordFieldIR *new_field,
    std::queue<std::string> *type_queue) {
  if ((old_field->GetOffset() != new_field->GetOffset()) ||
      // TODO: Should this be an inquality check instead ? Some compilers can
      // make signatures dependant on absolute values of access specifiers.
      IsAccessDownGraded(old_field->GetAccess(), new_field->GetAccess())) {
    return DiffStatus::direct_diff;
  }
  return CompareAndDumpTypeDiff(old_field->GetReferencedType(),
                                new_field->GetReferencedType(),
                                type_queue);
}

DiffStatus DiffWrapperBase::CompareRecordFields(
    const std::vector<abi_util::RecordFieldIR> &old_fields,
    const std::vector<abi_util::RecordFieldIR> &new_fields,
    std::queue<std::string> *type_queue) {
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
  if (removed_fields.size() > 0) {
    return DiffStatus::direct_diff;
  }
  std::vector<std::pair<
      const abi_util::RecordFieldIR *, const abi_util::RecordFieldIR *>> cf =
      abi_util::FindCommonElements(old_fields_map, new_fields_map);
  for (auto &&common_fields : cf) {
    if (CompareCommonRecordFields(common_fields.first, common_fields.second,
                                  type_queue) == DiffStatus::direct_diff) {
      return DiffStatus::direct_diff;
    }
  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareRecordTypes(
    const abi_util::RecordTypeIR *old_type,
    const abi_util::RecordTypeIR *new_type,
    std::queue<std::string> *type_queue) {
  // Compare names.
  if (old_type->GetName() != new_type->GetName()) {
    // Do not dump anything since the record types themselves are fundamentally
    // different.
    return DiffStatus::direct_diff;
  }

  if (!CompareSizeAndAlignment(old_type, new_type) ||
      !CompareVTables(old_type, new_type) ||
      (CompareRecordFields(old_type->GetFields(), new_type->GetFields(),
                          type_queue) == DiffStatus::direct_diff)) {
    ir_diff_dumper_->AddLinkableMessagesIR(old_type, new_type,
                                           Unwind(type_queue));
  }
  // TODO: Compare TemplateInfo and Bases
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareLvalueReferenceTypes(
    const abi_util::LvalueReferenceTypeIR *old_type,
    const abi_util::LvalueReferenceTypeIR *new_type,
    std::queue<std::string> *type_queue) {
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue);
}

DiffStatus DiffWrapperBase::CompareRvalueReferenceTypes(
    const abi_util::RvalueReferenceTypeIR *old_type,
    const abi_util::RvalueReferenceTypeIR *new_type,
     std::queue<std::string> *type_queue) {
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue);
}

DiffStatus DiffWrapperBase::CompareQualifiedTypes(
    const abi_util::QualifiedTypeIR *old_type,
    const abi_util::QualifiedTypeIR *new_type,
     std::queue<std::string> *type_queue) {
  // If all the qualifiers are not the same, return direct_diff, else
  // recursively compare the unqualified types.
  if (old_type->IsConst() != new_type->IsConst() ||
      old_type->IsVolatile() != new_type->IsVolatile() ||
      old_type->IsRestricted() != new_type->IsRestricted()) {
    return DiffStatus::direct_diff;
  }
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue);
}

DiffStatus DiffWrapperBase::ComparePointerTypes(
    const abi_util::PointerTypeIR *old_type,
    const abi_util::PointerTypeIR *new_type,
    std::queue<std::string> *type_queue) {
  // The following need to be the same for two pointer types to be considered
  // equivalent:
  // 1) Number of pointer indirections are the same.
  // 2) The ultimate pointee is the same.
  return CompareAndDumpTypeDiff(old_type->GetReferencedType(),
                                new_type->GetReferencedType(),
                                type_queue);
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
    std::queue<std::string> *type_queue) {
  if (old_parameters.size() != new_parameters.size()) {
    return DiffStatus::direct_diff;
  }
  uint64_t i = 0;
  while (i < old_parameters.size()) {
    const abi_util::ParamIR &old_parameter = old_parameters.at(i);
    const abi_util::ParamIR &new_parameter = new_parameters.at(i);
    if (CompareAndDumpTypeDiff(old_parameter.GetReferencedType(),
                               new_parameter.GetReferencedType(),
                               type_queue) == DiffStatus::direct_diff ||
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
    abi_util::LinkableMessageKind kind,
    std::queue<std::string> *type_queue) {
  if (kind == abi_util::LinkableMessageKind::BuiltinTypeKind) {
    return CompareBuiltinTypes(
        static_cast<const abi_util::BuiltinTypeIR *>(old_type),
        static_cast<const abi_util::BuiltinTypeIR *>(new_type));
  }

  if (kind == abi_util::LinkableMessageKind::QualifiedTypeKind) {
    return CompareQualifiedTypes(
        static_cast<const abi_util::QualifiedTypeIR *>(old_type),
        static_cast<const abi_util::QualifiedTypeIR *>(new_type),
        type_queue);
  }

  if (kind == abi_util::LinkableMessageKind::EnumTypeKind) {
      return CompareEnumTypes(
          static_cast<const abi_util::EnumTypeIR *>(old_type),
          static_cast<const abi_util::EnumTypeIR *>(new_type),
          type_queue);

  }

  if (kind == abi_util::LinkableMessageKind::LvalueReferenceTypeKind) {
    return CompareLvalueReferenceTypes(
        static_cast<const abi_util::LvalueReferenceTypeIR *>(old_type),
        static_cast<const abi_util::LvalueReferenceTypeIR *>(new_type),
        type_queue);

  }

  if (kind == abi_util::LinkableMessageKind::RvalueReferenceTypeKind) {
    return CompareRvalueReferenceTypes(
        static_cast<const abi_util::RvalueReferenceTypeIR *>(old_type),
        static_cast<const abi_util::RvalueReferenceTypeIR *>(new_type),
        type_queue);

  }

  if (kind == abi_util::LinkableMessageKind::PointerTypeKind) {
    return ComparePointerTypes(
        static_cast<const abi_util::PointerTypeIR *>(old_type),
        static_cast<const abi_util::PointerTypeIR *>(new_type),
        type_queue);

  }

  if (kind == abi_util::LinkableMessageKind::RecordTypeKind) {
    return CompareRecordTypes(
        static_cast<const abi_util::RecordTypeIR *>(old_type),
        static_cast<const abi_util::RecordTypeIR *>(new_type),
        type_queue);

  }
  return DiffStatus::no_diff;
}

DiffStatus DiffWrapperBase::CompareAndDumpTypeDiff(
    const std::string &old_type_str, const std::string &new_type_str,
    std::queue<std::string> *type_queue) {
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
                                type_queue);
}


// TODO: Add these for RecordType and EnumType as well.

template <>
bool DiffWrapper<abi_util::GlobalVarIR>::DumpDiff() {
  std::queue<std::string> type_queue;
  DiffStatus type_diff = CompareAndDumpTypeDiff(oldp_->GetReferencedType(),
                                                newp_->GetReferencedType(),
                                                &type_queue);
  DiffStatus access_diff = (oldp_->GetAccess() == newp_->GetAccess()) ?
      DiffStatus::no_diff : DiffStatus::direct_diff;
  if ((type_diff | access_diff) & DiffStatus::direct_diff) {
    ir_diff_dumper_->AddLinkableMessagesIR(oldp_, newp_, Unwind(&type_queue));
    return true;
  }
  return true;
}

template <>
bool DiffWrapper<abi_util::FunctionIR>::DumpDiff() {
  // If CompareAndDumpTypeDiff returns an unsafe / safe status, add the
  // corresponding diff to unsafe/ safe global var changes.
  std::queue<std::string> type_queue;
  type_queue.push(oldp_->GetLinkerSetKey());
  DiffStatus param_diffs = CompareFunctionParameters(oldp_->GetParameters(),
                                                     newp_->GetParameters(),
                                                     &type_queue);
  DiffStatus return_type_diff =
      CompareAndDumpTypeDiff(oldp_->GetReferencedType(),
                             newp_->GetReferencedType(),
                             &type_queue);
  if ((param_diffs == DiffStatus::direct_diff ||
       return_type_diff == DiffStatus::direct_diff) ||
      (oldp_->GetAccess() != newp_->GetAccess())) {
    ir_diff_dumper_->AddLinkableMessagesIR(oldp_, newp_, Unwind(&type_queue));
    return true;
  }
  return true;
}
} // abi_diff_wrappers

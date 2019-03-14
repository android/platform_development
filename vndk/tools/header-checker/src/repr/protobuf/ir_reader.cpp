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

#include "repr/protobuf/ir_reader.h"

#include "repr/ir_representation_internal.h"
#include "repr/protobuf/converter.h"

#include <fstream>

#include <google/protobuf/text_format.h>


namespace header_checker {
namespace repr {


void ProtobufTextFormatToIRReader::ReadTypeInfo(
    const abi_dump::BasicNamedAndTypedDecl &type_info, TypeIR *typep) {
  typep->SetLinkerSetKey(type_info.linker_set_key());
  typep->SetName(type_info.name());
  typep->SetSourceFile(type_info.source_file());
  typep->SetReferencedType(type_info.referenced_type());
  typep->SetSelfType(type_info.self_type());
  typep->SetSize(type_info.size());
  typep->SetAlignment(type_info.alignment());
}

bool ProtobufTextFormatToIRReader::ReadDump(const std::string &dump_file) {
  abi_dump::TranslationUnit tu;
  std::ifstream input(dump_file);
  google::protobuf::io::IstreamInputStream text_is(&input);

  if (!google::protobuf::TextFormat::Parse(&text_is, &tu)) {
    llvm::errs() << "Failed to parse protobuf TextFormat file\n";
    return false;
  }
  ReadFunctions(tu);
  ReadGlobalVariables(tu);

  ReadEnumTypes(tu);
  ReadRecordTypes(tu);
  ReadFunctionTypes(tu);
  ReadArrayTypes(tu);
  ReadPointerTypes(tu);
  ReadQualifiedTypes(tu);
  ReadBuiltinTypes(tu);
  ReadLvalueReferenceTypes(tu);
  ReadRvalueReferenceTypes(tu);

  ReadElfFunctions(tu);
  ReadElfObjects(tu);
  return true;
}

TemplateInfoIR ProtobufTextFormatToIRReader::TemplateInfoProtobufToIR(
    const abi_dump::TemplateInfo &template_info_protobuf) {
  TemplateInfoIR template_info_ir;
  for (auto &&template_element : template_info_protobuf.elements()) {
    TemplateElementIR template_element_ir(template_element.referenced_type());
    template_info_ir.AddTemplateElement(std::move(template_element_ir));
  }
  return template_info_ir;
}

template <typename T>
static void SetupCFunctionLikeIR(const T &cfunction_like_protobuf,
                                 CFunctionLikeIR *cfunction_like_ir) {
  cfunction_like_ir->SetReturnType(cfunction_like_protobuf.return_type());
  for (auto &&parameter: cfunction_like_protobuf.parameters()) {
    ParamIR param_ir(parameter.referenced_type(), parameter.default_arg(),
                     false);
    cfunction_like_ir->AddParameter(std::move(param_ir));
  }
}

FunctionIR ProtobufTextFormatToIRReader::FunctionProtobufToIR(
    const abi_dump::FunctionDecl &function_protobuf) {
  FunctionIR function_ir;
  function_ir.SetReturnType(function_protobuf.return_type());
  function_ir.SetLinkerSetKey(function_protobuf.linker_set_key());
  function_ir.SetName(function_protobuf.function_name());
  function_ir.SetAccess(AccessProtobufToIR(function_protobuf.access()));
  function_ir.SetSourceFile(function_protobuf.source_file());
  // Set parameters
  for (auto &&parameter: function_protobuf.parameters()) {
    ParamIR param_ir(parameter.referenced_type(), parameter.default_arg(),
                     parameter.is_this_ptr());
    function_ir.AddParameter(std::move(param_ir));
  }
  // Set Template info
  function_ir.SetTemplateInfo(
      TemplateInfoProtobufToIR(function_protobuf.template_info()));
  return function_ir;
}

FunctionTypeIR ProtobufTextFormatToIRReader::FunctionTypeProtobufToIR(
    const abi_dump::FunctionType &function_type_protobuf) {
  FunctionTypeIR function_type_ir;
  ReadTypeInfo(function_type_protobuf.type_info(), &function_type_ir);
  SetupCFunctionLikeIR(function_type_protobuf, &function_type_ir);
  return function_type_ir;
}

VTableLayoutIR ProtobufTextFormatToIRReader::VTableLayoutProtobufToIR(
    const abi_dump::VTableLayout &vtable_layout_protobuf) {
  VTableLayoutIR vtable_layout_ir;
  for (auto &&vtable_component : vtable_layout_protobuf.vtable_components()) {
    VTableComponentIR vtable_component_ir(
        vtable_component.mangled_component_name(),
        VTableComponentKindProtobufToIR(vtable_component.kind()),
        vtable_component.component_value(),
        vtable_component.is_pure());
    vtable_layout_ir.AddVTableComponent(std::move(vtable_component_ir));
  }
  return vtable_layout_ir;
}

std::vector<RecordFieldIR>
ProtobufTextFormatToIRReader::RecordFieldsProtobufToIR(
    const google::protobuf::RepeatedPtrField<abi_dump::RecordFieldDecl> &rfp) {
  std::vector<RecordFieldIR> record_type_fields_ir;
  for (auto &&field : rfp) {
    RecordFieldIR record_field_ir(field.field_name(), field.referenced_type(),
                                  field.field_offset(),
                                  AccessProtobufToIR(field.access()));
    record_type_fields_ir.emplace_back(std::move(record_field_ir));
  }
  return record_type_fields_ir;
}

std::vector<CXXBaseSpecifierIR>
ProtobufTextFormatToIRReader::RecordCXXBaseSpecifiersProtobufToIR(
    const google::protobuf::RepeatedPtrField<abi_dump::CXXBaseSpecifier> &rbs) {
  std::vector<CXXBaseSpecifierIR> record_type_bases_ir;
  for (auto &&base : rbs) {
    CXXBaseSpecifierIR record_base_ir(
        base.referenced_type(), base.is_virtual(),
        AccessProtobufToIR(base.access()));
    record_type_bases_ir.emplace_back(std::move(record_base_ir));
  }
  return record_type_bases_ir;
}

RecordTypeIR ProtobufTextFormatToIRReader::RecordTypeProtobufToIR(
    const abi_dump::RecordType &record_type_protobuf) {
  RecordTypeIR record_type_ir;
  ReadTypeInfo(record_type_protobuf.type_info(), &record_type_ir);
  record_type_ir.SetTemplateInfo(
      TemplateInfoProtobufToIR(record_type_protobuf.template_info()));
  record_type_ir.SetAccess(AccessProtobufToIR(record_type_protobuf.access()));
  record_type_ir.SetVTableLayout(
      VTableLayoutProtobufToIR(record_type_protobuf.vtable_layout()));
  // Get fields
  record_type_ir.SetRecordFields(RecordFieldsProtobufToIR(
      record_type_protobuf.fields()));
  // Base Specifiers
  record_type_ir.SetCXXBaseSpecifiers(RecordCXXBaseSpecifiersProtobufToIR(
      record_type_protobuf.base_specifiers()));
  record_type_ir.SetRecordKind(
      RecordKindProtobufToIR(record_type_protobuf.record_kind()));
  record_type_ir.SetAnonymity(record_type_protobuf.is_anonymous());
  record_type_ir.SetUniqueId(record_type_protobuf.tag_info().unique_id());
  return record_type_ir;
}

std::vector<EnumFieldIR>
ProtobufTextFormatToIRReader::EnumFieldsProtobufToIR(
    const google::protobuf::RepeatedPtrField<abi_dump::EnumFieldDecl> &efp) {
  std::vector<EnumFieldIR> enum_type_fields_ir;
  for (auto &&field : efp) {
    EnumFieldIR enum_field_ir(field.name(), field.enum_field_value());
    enum_type_fields_ir.emplace_back(std::move(enum_field_ir));
  }
  return enum_type_fields_ir;
}

EnumTypeIR ProtobufTextFormatToIRReader::EnumTypeProtobufToIR(
    const abi_dump::EnumType &enum_type_protobuf) {
  EnumTypeIR enum_type_ir;
  ReadTypeInfo(enum_type_protobuf.type_info(), &enum_type_ir);
  enum_type_ir.SetUnderlyingType(enum_type_protobuf.underlying_type());
  enum_type_ir.SetAccess(AccessProtobufToIR(enum_type_protobuf.access()));
  enum_type_ir.SetFields(
      EnumFieldsProtobufToIR(enum_type_protobuf.enum_fields()));
  enum_type_ir.SetUniqueId(enum_type_protobuf.tag_info().unique_id());
  return enum_type_ir;
}

void ProtobufTextFormatToIRReader::ReadGlobalVariables(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&global_variable_protobuf : tu.global_vars()) {
    GlobalVarIR global_variable_ir;
    global_variable_ir.SetName(global_variable_protobuf.name());
    global_variable_ir.SetAccess(AccessProtobufToIR(global_variable_protobuf.access()));
    global_variable_ir.SetSourceFile(global_variable_protobuf.source_file());
    global_variable_ir.SetReferencedType(
        global_variable_protobuf.referenced_type());
    global_variable_ir.SetLinkerSetKey(
        global_variable_protobuf.linker_set_key());
    if (!IsLinkableMessageInExportedHeaders(&global_variable_ir)) {
      continue;
    }
    module_->global_variables_.insert(
        {global_variable_ir.GetLinkerSetKey(), std::move(global_variable_ir)});
  }
}

void ProtobufTextFormatToIRReader::ReadPointerTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&pointer_type_protobuf : tu.pointer_types()) {
    PointerTypeIR pointer_type_ir;
    ReadTypeInfo(pointer_type_protobuf.type_info(), &pointer_type_ir);
    if (!IsLinkableMessageInExportedHeaders(&pointer_type_ir)) {
      continue;
    }
    AddToMapAndTypeGraph(std::move(pointer_type_ir), &module_->pointer_types_,
                         &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadBuiltinTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&builtin_type_protobuf : tu.builtin_types()) {
    BuiltinTypeIR builtin_type_ir;
    ReadTypeInfo(builtin_type_protobuf.type_info(), &builtin_type_ir);
    builtin_type_ir.SetSignedness(builtin_type_protobuf.is_unsigned());
    builtin_type_ir.SetIntegralType(builtin_type_protobuf.is_integral());
    AddToMapAndTypeGraph(std::move(builtin_type_ir), &module_->builtin_types_,
                         &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadQualifiedTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&qualified_type_protobuf : tu.qualified_types()) {
    QualifiedTypeIR qualified_type_ir;
    ReadTypeInfo(qualified_type_protobuf.type_info(), &qualified_type_ir);
    qualified_type_ir.SetConstness(qualified_type_protobuf.is_const());
    qualified_type_ir.SetVolatility(qualified_type_protobuf.is_volatile());
    qualified_type_ir.SetRestrictedness(
        qualified_type_protobuf.is_restricted());
    if (!IsLinkableMessageInExportedHeaders(&qualified_type_ir)) {
      continue;
    }
    AddToMapAndTypeGraph(std::move(qualified_type_ir),
                         &module_->qualified_types_, &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadArrayTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&array_type_protobuf : tu.array_types()) {
    ArrayTypeIR array_type_ir;
    ReadTypeInfo(array_type_protobuf.type_info(), &array_type_ir);
    if (!IsLinkableMessageInExportedHeaders(&array_type_ir)) {
      continue;
    }
    AddToMapAndTypeGraph(std::move(array_type_ir), &module_->array_types_,
                         &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadLvalueReferenceTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&lvalue_reference_type_protobuf : tu.lvalue_reference_types()) {
    LvalueReferenceTypeIR lvalue_reference_type_ir;
    ReadTypeInfo(lvalue_reference_type_protobuf.type_info(),
                 &lvalue_reference_type_ir);
    if (!IsLinkableMessageInExportedHeaders(&lvalue_reference_type_ir)) {
      continue;
    }
    AddToMapAndTypeGraph(std::move(lvalue_reference_type_ir),
                         &module_->lvalue_reference_types_,
                         &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadRvalueReferenceTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&rvalue_reference_type_protobuf : tu.rvalue_reference_types()) {
    RvalueReferenceTypeIR rvalue_reference_type_ir;
    ReadTypeInfo(rvalue_reference_type_protobuf.type_info(),
                 &rvalue_reference_type_ir);
    if (!IsLinkableMessageInExportedHeaders(&rvalue_reference_type_ir)) {
      continue;
    }
    AddToMapAndTypeGraph(std::move(rvalue_reference_type_ir),
                         &module_->rvalue_reference_types_,
                         &module_->type_graph_);
  }
}

void ProtobufTextFormatToIRReader::ReadFunctions(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&function_protobuf : tu.functions()) {
    FunctionIR function_ir = FunctionProtobufToIR(function_protobuf);
    if (!IsLinkableMessageInExportedHeaders(&function_ir)) {
      continue;
    }
    module_->functions_.insert(
        {function_ir.GetLinkerSetKey(), std::move(function_ir)});
  }
}

void ProtobufTextFormatToIRReader::ReadRecordTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&record_type_protobuf : tu.record_types()) {
    RecordTypeIR record_type_ir = RecordTypeProtobufToIR(record_type_protobuf);
    if (!IsLinkableMessageInExportedHeaders(&record_type_ir)) {
      continue;
    }
    auto it = AddToMapAndTypeGraph(std::move(record_type_ir),
                                   &module_->record_types_,
                                   &module_->type_graph_);
    const std::string &key = GetODRListMapKey(&(it->second));
    AddToODRListMap(key, &(it->second));
  }
}

void ProtobufTextFormatToIRReader::ReadFunctionTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&function_type_protobuf : tu.function_types()) {
    FunctionTypeIR function_type_ir =
        FunctionTypeProtobufToIR(function_type_protobuf);
    if (!IsLinkableMessageInExportedHeaders(&function_type_ir)) {
      continue;
    }
    auto it = AddToMapAndTypeGraph(std::move(function_type_ir),
                                   &module_->function_types_,
                                   &module_->type_graph_);
    const std::string &key = GetODRListMapKey(&(it->second));
    AddToODRListMap(key, &(it->second));
  }
}

void ProtobufTextFormatToIRReader::ReadEnumTypes(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&enum_type_protobuf : tu.enum_types()) {
    EnumTypeIR enum_type_ir = EnumTypeProtobufToIR(enum_type_protobuf);
    if (!IsLinkableMessageInExportedHeaders(&enum_type_ir)) {
      continue;
    }
    auto it = AddToMapAndTypeGraph(std::move(enum_type_ir),
                                   &module_->enum_types_,
                                   &module_->type_graph_);
    AddToODRListMap(it->second.GetUniqueId() + it->second.GetSourceFile(),
                    (&it->second));
  }
}

void ProtobufTextFormatToIRReader::ReadElfFunctions(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&elf_function : tu.elf_functions()) {
    ElfFunctionIR elf_function_ir(
        elf_function.name(),
        ElfSymbolBindingProtobufToIR(elf_function.binding()));
    module_->elf_functions_.insert(
        {elf_function_ir.GetName(), std::move(elf_function_ir)});
  }
}

void ProtobufTextFormatToIRReader::ReadElfObjects(
    const abi_dump::TranslationUnit &tu) {
  for (auto &&elf_object : tu.elf_objects()) {
    ElfObjectIR elf_object_ir(
        elf_object.name(), ElfSymbolBindingProtobufToIR(elf_object.binding()));
    module_->elf_objects_.insert(
        {elf_object_ir.GetName(), std::move(elf_object_ir)});
  }
}


}  // namespace repr
}  // namespace header_checker

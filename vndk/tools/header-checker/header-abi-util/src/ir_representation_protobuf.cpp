// Copyright (C) 2017 The Android Open Source Project
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

#include <ir_representation_protobuf.h>

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

namespace abi_util {

void ProtobufTextFormatToIRReader::ReadTypeInfo(
    const abi_dump::BasicNamedAndTypedDecl &type_info,
    TypeIR *typep) {
  typep->SetLinkerSetKey(type_info.linker_set_key());
  typep->SetName(type_info.linker_set_key());
  typep->SetSourceFile(type_info.source_file());
  typep->SetReferencedType(type_info.referenced_type());
  typep->SetSize(type_info.size());
  typep->SetAlignment(type_info.alignment());
}

bool ProtobufTextFormatToIRReader::ReadDump() {
  abi_dump::TranslationUnit tu;
  std::ifstream input(dump_path_);
  google::protobuf::io::IstreamInputStream text_is(&input);

  if (!google::protobuf::TextFormat::Parse(&text_is, &tu)) {
    llvm::errs() << "Failed to parse protobuf TextFormat file\n";
    return false;
  }

  functions_ = ReadFunctions(tu);
  global_variables_ = ReadGlobalVariables(tu);

  enum_types_ = ReadEnumTypes(tu);
  record_types_ = ReadRecordTypes(tu);
  array_types_ = ReadArrayTypes(tu);
  pointer_types_ = ReadPointerTypes(tu);
  qualified_types_ = ReadQualifiedTypes(tu);
  builtin_types_ = ReadBuiltinTypes(tu);
  lvalue_reference_types_ = ReadLvalueReferenceTypes(tu);
  rvalue_reference_types_ = ReadRvalueReferenceTypes(tu);
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

FunctionIR ProtobufTextFormatToIRReader::FunctionProtobufToIR(
    const abi_dump::FunctionDecl &function_protobuf) {
  FunctionIR function_ir;
  function_ir.SetReferencedType(function_protobuf.referenced_type());
  function_ir.SetLinkerSetKey(function_protobuf.linker_set_key());
  function_ir.SetAccess(AccessProtobufToIR(function_protobuf.access()));
  function_ir.SetSourceFile(function_protobuf.source_file());
  // Set parameters
  for (auto &&parameter: function_protobuf.parameters()) {
    ParamIR param_ir(parameter.referenced_type(), parameter.default_arg());
    function_ir.AddParameter(std::move(param_ir));
  }
  // Set Template info
  function_ir.SetTemplateInfo(
      TemplateInfoProtobufToIR(function_protobuf.template_info()));
  return function_ir;
}

VTableLayoutIR ProtobufTextFormatToIRReader::VTableLayoutProtobufToIR(
    const abi_dump::VTableLayout &vtable_layout_protobuf) {
  VTableLayoutIR vtable_layout_ir;
  for (auto &&vtable_component : vtable_layout_protobuf.vtable_components()) {
    VTableComponentIR vtable_component_ir(
        vtable_component.mangled_component_name(),
        VTableComponentKindProtobufToIR(vtable_component.kind()),
        vtable_component.component_value());
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
  return enum_type_ir;
}

std::vector<GlobalVarIR> ProtobufTextFormatToIRReader::ReadGlobalVariables(
    const abi_dump::TranslationUnit &tu) {
  std::vector<GlobalVarIR> global_variables;
  for (auto &&global_variable_protobuf : tu.global_vars()) {
    GlobalVarIR global_variable_ir;
    global_variable_ir.SetName(global_variable_protobuf.name());
    global_variable_ir.SetSourceFile(global_variable_protobuf.source_file());
    global_variable_ir.SetReferencedType(
        global_variable_protobuf.referenced_type());
    global_variable_ir.SetLinkerSetKey(
        global_variable_protobuf.linker_set_key());
    global_variables.emplace_back(std::move(global_variable_ir));
  }
  return global_variables;
}

std::vector<PointerTypeIR> ProtobufTextFormatToIRReader::ReadPointerTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<PointerTypeIR> pointer_types;
  for (auto &&pointer_type_protobuf : tu.pointer_types()) {
    PointerTypeIR pointer_type_ir;
    ReadTypeInfo(pointer_type_protobuf.type_info(), &pointer_type_ir);
    pointer_types.emplace_back(std::move(pointer_type_ir));
  }
  return pointer_types;
}

std::vector<BuiltinTypeIR> ProtobufTextFormatToIRReader::ReadBuiltinTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<BuiltinTypeIR> builtin_types;
  for (auto &&builtin_type_protobuf : tu.builtin_types()) {
    BuiltinTypeIR builtin_type_ir;
    ReadTypeInfo(builtin_type_protobuf.type_info(), &builtin_type_ir);
    builtin_type_ir.SetSignedness(builtin_type_protobuf.is_unsigned());
    builtin_types.emplace_back(std::move(builtin_type_ir));
  }
  return builtin_types;
}

std::vector<QualifiedTypeIR> ProtobufTextFormatToIRReader::ReadQualifiedTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<QualifiedTypeIR> qualified_types;
  for (auto &&qualified_type_protobuf : tu.qualified_types()) {
    QualifiedTypeIR qualified_type_ir;
    ReadTypeInfo(qualified_type_protobuf.type_info(), &qualified_type_ir);
    qualified_types.emplace_back(std::move(qualified_type_ir));
  }
  return qualified_types;
}

std::vector<ArrayTypeIR> ProtobufTextFormatToIRReader::ReadArrayTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<ArrayTypeIR> array_types;
  for (auto &&array_type_protobuf : tu.array_types()) {
    ArrayTypeIR array_type_ir;
    ReadTypeInfo(array_type_protobuf.type_info(), &array_type_ir);
    array_types.emplace_back(std::move(array_type_ir));
  }
  return array_types;
}

std::vector<LvalueReferenceTypeIR>
ProtobufTextFormatToIRReader::ReadLvalueReferenceTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<LvalueReferenceTypeIR> lvalue_reference_types;
  for (auto &&lvalue_reference_type_protobuf : tu.lvalue_reference_types()) {
    LvalueReferenceTypeIR lvalue_reference_type_ir;
    ReadTypeInfo(lvalue_reference_type_protobuf.type_info(),
                 &lvalue_reference_type_ir);
    lvalue_reference_types.emplace_back(std::move(lvalue_reference_type_ir));
  }
  return lvalue_reference_types;
}

std::vector<RvalueReferenceTypeIR>
ProtobufTextFormatToIRReader::ReadRvalueReferenceTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<RvalueReferenceTypeIR> rvalue_reference_types;
  for (auto &&rvalue_reference_type_protobuf : tu.rvalue_reference_types()) {
    RvalueReferenceTypeIR rvalue_reference_type_ir;
    ReadTypeInfo(rvalue_reference_type_protobuf.type_info(),
                 &rvalue_reference_type_ir);
    rvalue_reference_types.emplace_back(std::move(rvalue_reference_type_ir));
  }
  return rvalue_reference_types;
}

std::vector<FunctionIR> ProtobufTextFormatToIRReader::ReadFunctions(
    const abi_dump::TranslationUnit &tu) {
  std::vector<FunctionIR> functions;
  for (auto &&function_protobuf : tu.functions()) {
    FunctionIR function_ir = FunctionProtobufToIR(function_protobuf);
    functions.emplace_back(std::move(function_ir));
  }
  return functions;
}

std::vector<RecordTypeIR> ProtobufTextFormatToIRReader::ReadRecordTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<RecordTypeIR> record_types;
  for (auto &&record_type_protobuf : tu.record_types()) {
    RecordTypeIR record_type_ir = RecordTypeProtobufToIR(record_type_protobuf);
    record_types.emplace_back(std::move(record_type_ir));
  }
  return record_types;
}

std::vector<EnumTypeIR> ProtobufTextFormatToIRReader::ReadEnumTypes(
    const abi_dump::TranslationUnit &tu) {
  std::vector<EnumTypeIR> enum_types;
  for (auto &&enum_type_protobuf : tu.enum_types()) {
    EnumTypeIR enum_type_ir = EnumTypeProtobufToIR(enum_type_protobuf);
    enum_types.emplace_back(std::move(enum_type_ir));
  }
  return enum_types;
}

bool IRToProtobufConverter::AddTemplateInformation(
    abi_dump::TemplateInfo *ti, const abi_util::TemplatedArtifactIR *ta) {
  for (auto &&template_element : ta->GetTemplateElements()) {
    abi_dump::TemplateElement *added_element = ti->add_elements();
    if (!added_element) {
      return false;
    }
    added_element->set_referenced_type(template_element.GetReferencedType());
  }
  return true;
}

bool IRToProtobufConverter::AddTypeInfo(
    abi_dump::BasicNamedAndTypedDecl *type_info,
    const TypeIR *typep) {
  if (!type_info || !typep) {
    return false;
  }
  type_info->set_linker_set_key(typep->GetLinkerSetKey());
  type_info->set_source_file(typep->GetSourceFile());
  type_info->set_name(typep->GetName());
  type_info->set_size(typep->GetSize());
  type_info->set_alignment(typep->GetAlignment());
  type_info->set_referenced_type(typep->GetReferencedType());
  return true;
}

bool IRToProtobufConverter::AddRecordFields(abi_dump::RecordType *record_protobuf,
                                       const RecordTypeIR *record_ir) {
  // Iterate through the fields and create corresponding ones for the protobuf
  // record
  for (auto &&field_ir : record_ir->GetFields()) {
    abi_dump::RecordFieldDecl *added_field = record_protobuf->add_fields();
    if (!added_field) {
      llvm::errs() << "Couldn't add record field\n";
    }
    added_field->set_field_name(std::move(field_ir.GetName()));
    added_field->set_referenced_type(std::move(field_ir.GetReferencedType()));
    added_field->set_access(AccessIRToProtobuf(field_ir.GetAccess()));
    added_field->set_field_offset(field_ir.GetOffset());
  }
  return true;
}

bool IRToProtobufConverter::AddBaseSpecifiers(abi_dump::RecordType *record_protobuf,
                                         const RecordTypeIR *record_ir) {
  for (auto &&base_ir : record_ir->GetBases()) {
    abi_dump::CXXBaseSpecifier *added_base =
        record_protobuf->add_base_specifiers();
    if (!added_base) {
      return false;
    }
    added_base->set_referenced_type(base_ir.GetReferencedType());
    added_base->set_is_virtual(base_ir.IsVirtual());
  }
  return true;
}

bool IRToProtobufConverter::AddVTableLayout(abi_dump::RecordType *record_protobuf,
                                       const RecordTypeIR *record_ir) {
  // If there are no entries in the vtable, just return.
  if (record_ir->GetVTableNumEntries() == 0) {
    return true;
  }
  const VTableLayoutIR &vtable_layout_ir = record_ir->GetVTableLayout();
  abi_dump::VTableLayout *vtable_protobuf =
      record_protobuf->mutable_vtable_layout();
  if (!vtable_protobuf) {
    return false;
  }
  for (auto &&vtable_component_ir : vtable_layout_ir.GetVTableComponents()) {
    abi_dump::VTableComponent *added_vtable_component =
        vtable_protobuf->add_vtable_components();
    if (!added_vtable_component) {
      return false;
    }
    added_vtable_component->set_kind(
        VTableComponentKindIRToProtobuf(vtable_component_ir.GetKind()));
    added_vtable_component->set_component_value(vtable_component_ir.GetValue());
    added_vtable_component->set_mangled_component_name(
        vtable_component_ir.GetName());
  }
  return true;
}

abi_dump::RecordType IRToProtobufConverter::ConvertRecordTypeIR(
    const RecordTypeIR *recordp) {
  abi_dump::RecordType added_record_type;
  added_record_type.set_access(AccessIRToProtobuf(recordp->GetAccess()));
  if (!AddTypeInfo(added_record_type.mutable_type_info(), recordp) ||
      !AddRecordFields(&added_record_type, recordp) ||
      !AddBaseSpecifiers(&added_record_type, recordp) ||
      !AddVTableLayout(&added_record_type, recordp) ||
      !(recordp->GetTemplateElements().size() ?
       AddTemplateInformation(added_record_type.mutable_template_info(),
                              recordp) : true)) {
    ::exit(1);
  }
  return added_record_type;
}


bool IRToProtobufConverter::AddFunctionParameters(
    abi_dump::FunctionDecl *function_protobuf,
    const FunctionIR *function_ir) {
  for (auto &&parameter : function_ir->GetParameters()) {
    abi_dump::ParamDecl *added_parameter = function_protobuf->add_parameters();
    if (!added_parameter) {
      return false;
    }
    added_parameter->set_referenced_type(
        parameter.GetReferencedType());
    added_parameter->set_default_arg(parameter.GetIsDefault());
  }
  return true;
}

abi_dump::FunctionDecl IRToProtobufConverter::ConvertFunctionIR(
    const FunctionIR *functionp) {
  abi_dump::FunctionDecl added_function;
  added_function.set_access(AccessIRToProtobuf(functionp->GetAccess()));
  added_function.set_linker_set_key(functionp->GetLinkerSetKey());
  added_function.set_source_file(functionp->GetSourceFile());
  added_function.set_referenced_type(functionp->GetReferencedType());
  if (!AddFunctionParameters(&added_function, functionp) ||
      !(functionp->GetTemplateElements().size() ?
      AddTemplateInformation(added_function.mutable_template_info(), functionp)
      : true)) {
    ::exit(1);
  }
  return added_function;
}

bool IRToProtobufConverter::AddEnumFields(abi_dump::EnumType *enum_protobuf,
                                     const EnumTypeIR *enum_ir) {
  for (auto &&field : enum_ir->GetFields()) {
    abi_dump::EnumFieldDecl *enum_fieldp = enum_protobuf->add_enum_fields();
    if (!enum_fieldp) {
      return false;
    }
    // TODO:  Do we need to add underlying type at all ?
    enum_fieldp->set_name(field.GetName());
    enum_fieldp->set_enum_field_value(field.GetValue());
  }
  return true;
}


abi_dump::EnumType IRToProtobufConverter::ConvertEnumTypeIR(
    const EnumTypeIR *enump) {
  abi_dump::EnumType added_enum_type;
  added_enum_type.set_access(AccessIRToProtobuf(enump->GetAccess()));
  added_enum_type.set_underlying_type(enump->GetUnderlyingType());
  if (!AddTypeInfo(added_enum_type.mutable_type_info(), enump) ||
      !AddEnumFields(&added_enum_type, enump)) {
    ::exit(1);
  }
  return added_enum_type;
}

abi_dump::GlobalVarDecl IRToProtobufConverter::ConvertGlobalVarIR(
    const GlobalVarIR *global_varp) {
  abi_dump::GlobalVarDecl added_global_var;
  added_global_var.set_referenced_type(global_varp->GetReferencedType());
  added_global_var.set_source_file(global_varp->GetSourceFile());
  added_global_var.set_name(global_varp->GetName());
  added_global_var.set_linker_set_key(global_varp->GetLinkerSetKey());
  added_global_var.set_access(
      AccessIRToProtobuf(global_varp->GetAccess()));
  return added_global_var;
}

abi_dump::PointerType IRToProtobufConverter::ConvertPointerTypeIR(
    const PointerTypeIR *pointerp) {
  abi_dump::PointerType added_pointer_type;
  if (!AddTypeInfo(added_pointer_type.mutable_type_info(), pointerp)) {
    ::exit(1);
  }
  return added_pointer_type;
}

abi_dump::QualifiedType IRToProtobufConverter::ConvertQualifiedTypeIR(
    const QualifiedTypeIR *qualtypep) {
  abi_dump::QualifiedType added_qualified_type;
  if (!AddTypeInfo(added_qualified_type.mutable_type_info(), qualtypep)) {
    ::exit(1);
  }
  return added_qualified_type;
}

abi_dump::BuiltinType IRToProtobufConverter::ConvertBuiltinTypeIR(
    const BuiltinTypeIR *builtin_typep) {
  abi_dump::BuiltinType added_builtin_type;
  added_builtin_type.set_is_unsigned(builtin_typep->IsUnsigned());
  if (!AddTypeInfo(added_builtin_type.mutable_type_info(), builtin_typep)) {
    ::exit(1);
  }
  return added_builtin_type;
}

abi_dump::ArrayType IRToProtobufConverter::ConvertArrayTypeIR(
    const ArrayTypeIR *array_typep) {
  abi_dump::ArrayType added_array_type;
  if (!AddTypeInfo(added_array_type.mutable_type_info(), array_typep)) {
    ::exit(1);
  }
  return added_array_type;
}

abi_dump::LvalueReferenceType
IRToProtobufConverter::ConvertLvalueReferenceTypeIR(
    const LvalueReferenceTypeIR *lvalue_reference_typep) {
  abi_dump::LvalueReferenceType added_lvalue_reference_type;
  if (!AddTypeInfo(added_lvalue_reference_type.mutable_type_info(),
                   lvalue_reference_typep)) {
    ::exit(1);
  }
  return added_lvalue_reference_type;
}

abi_dump::RvalueReferenceType
IRToProtobufConverter::ConvertRvalueReferenceTypeIR(
    const RvalueReferenceTypeIR *rvalue_reference_typep) {
  abi_dump::RvalueReferenceType added_rvalue_reference_type;
  if (!AddTypeInfo(added_rvalue_reference_type.mutable_type_info(),
                   rvalue_reference_typep)) {
    ::exit(1);
  }
  return added_rvalue_reference_type;
}

bool ProtobufIRDumper::AddLinkableMessageIR (const LinkableMessageIR *lm) {
  // No RTTI
  switch (lm->GetKind()) {
    case RecordTypeKind:
      return AddRecordTypeIR(static_cast<const RecordTypeIR *>(lm));
    case EnumTypeKind:
      return AddEnumTypeIR(static_cast<const EnumTypeIR *>(lm));
    case PointerTypeKind:
      return AddPointerTypeIR(static_cast<const PointerTypeIR *>(lm));
    case QualifiedTypeKind:
      return AddQualifiedTypeIR(static_cast<const QualifiedTypeIR *>(lm));
    case ArrayTypeKind:
      return AddArrayTypeIR(static_cast<const ArrayTypeIR *>(lm));
    case LvalueReferenceTypeKind:
      return AddLvalueReferenceTypeIR(
          static_cast<const LvalueReferenceTypeIR *>(lm));
    case RvalueReferenceTypeKind:
      return AddRvalueReferenceTypeIR(
          static_cast<const RvalueReferenceTypeIR*>(lm));
    case BuiltinTypeKind:
      return AddBuiltinTypeIR(static_cast<const BuiltinTypeIR*>(lm));
    case GlobalVarKind:
      return AddGlobalVarIR(static_cast<const GlobalVarIR*>(lm));
    case FunctionKind:
      return AddFunctionIR(static_cast<const FunctionIR*>(lm));
  }
  return false;
}

bool ProtobufIRDumper::AddRecordTypeIR(const RecordTypeIR *recordp) {
  abi_dump::RecordType *added_record_type = tu_ptr_->add_record_types();
  if (!added_record_type) {
    return false;
  }
  *added_record_type = ConvertRecordTypeIR(recordp);
  return true;
}

bool ProtobufIRDumper::AddFunctionIR(const FunctionIR *functionp) {
  abi_dump::FunctionDecl *added_function = tu_ptr_->add_functions();
  if (!added_function) {
    return false;
  }
  *added_function = ConvertFunctionIR(functionp);
  return true;
}

bool ProtobufIRDumper::AddEnumTypeIR(const EnumTypeIR *enump) {
  abi_dump::EnumType *added_enum_type = tu_ptr_->add_enum_types();
  if (!added_enum_type) {
    return false;
  }
  *added_enum_type = ConvertEnumTypeIR(enump);
  return true;
}

bool ProtobufIRDumper::AddGlobalVarIR(const GlobalVarIR *global_varp) {
  abi_dump::GlobalVarDecl *added_global_var = tu_ptr_->add_global_vars();
  if (!added_global_var) {
    return false;
  }
  *added_global_var = ConvertGlobalVarIR(global_varp);
  return true;
}

bool ProtobufIRDumper::AddPointerTypeIR(const PointerTypeIR *pointerp) {
  abi_dump::PointerType *added_pointer_type = tu_ptr_->add_pointer_types();
  if (!added_pointer_type) {
    return false;
  }
  *added_pointer_type = ConvertPointerTypeIR(pointerp);
  return true;
}

bool ProtobufIRDumper::AddQualifiedTypeIR(const QualifiedTypeIR *qualtypep) {
  abi_dump::QualifiedType *added_qualified_type =
      tu_ptr_->add_qualified_types();
  if (!added_qualified_type) {
    return false;
  }
  *added_qualified_type = ConvertQualifiedTypeIR(qualtypep);
  return true;
}

bool ProtobufIRDumper::AddBuiltinTypeIR(const BuiltinTypeIR *builtin_typep) {
  abi_dump::BuiltinType *added_builtin_type =
      tu_ptr_->add_builtin_types();
  if (!added_builtin_type) {
    return false;
  }
  *added_builtin_type = ConvertBuiltinTypeIR(builtin_typep);
  return true;
}

bool ProtobufIRDumper::AddArrayTypeIR(const ArrayTypeIR *array_typep) {
  abi_dump::ArrayType *added_array_type =
      tu_ptr_->add_array_types();
  if (!added_array_type) {
    return false;
  }
  *added_array_type = ConvertArrayTypeIR(array_typep);
  return true;
}

bool ProtobufIRDumper::AddLvalueReferenceTypeIR(
    const LvalueReferenceTypeIR *lvalue_reference_typep) {
  abi_dump::LvalueReferenceType *added_lvalue_reference_type =
      tu_ptr_->add_lvalue_reference_types();
  if (!added_lvalue_reference_type) {
    return false;
  }
  *added_lvalue_reference_type =
      ConvertLvalueReferenceTypeIR(lvalue_reference_typep);
  return true;
}

bool ProtobufIRDumper::AddRvalueReferenceTypeIR(
    const RvalueReferenceTypeIR *rvalue_reference_typep) {
  abi_dump::RvalueReferenceType *added_rvalue_reference_type =
      tu_ptr_->add_rvalue_reference_types();
  if (!added_rvalue_reference_type) {
    return false;
  }
  *added_rvalue_reference_type =
      ConvertRvalueReferenceTypeIR(rvalue_reference_typep);
  return true;
}

bool ProtobufIRDumper::Dump() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  assert( tu_ptr_.get() != nullptr);
  std::ofstream text_output(dump_path_);
  google::protobuf::io::OstreamOutputStream text_os(&text_output);
  return google::protobuf::TextFormat::Print(*tu_ptr_.get(), &text_os);
}

void ProtobufIRDiffDumper::AddLibNameIR(const std::string &name) {
  diff_tu_->set_lib_name(name);
}

void ProtobufIRDiffDumper::AddArchIR(const std::string &arch) {
  diff_tu_->set_arch(arch);
}

void ProtobufIRDiffDumper::AddCompatibilityStatusIR(
    CompatibilityStatusIR status) {
  diff_tu_->set_compatibility_status(CompatibilityStatusIRToProtobuf(status));
}

bool ProtobufIRDiffDumper::AddLinkableMessagesIR(
    const LinkableMessageIR *old_message,
    const LinkableMessageIR *new_message,
    const std::string &type_stack) {
  assert(old_message->GetKind() == new_message->GetKind());
  switch (old_message->GetKind()) {
    case RecordTypeKind:
      return AddRecordTypeDiffIR(
          static_cast<const RecordTypeIR *>(old_message),
          static_cast<const RecordTypeIR *>(new_message),
          type_stack);
    case EnumTypeKind:
      return AddEnumTypeDiffIR(
          static_cast<const EnumTypeIR *>(old_message),
          static_cast<const EnumTypeIR *>(new_message),
          type_stack);
    case GlobalVarKind:
      return AddGlobalVarDiffIR(
          static_cast<const GlobalVarIR*>(old_message),
          static_cast<const GlobalVarIR*>(new_message),
          type_stack);
    case FunctionKind:
      return AddFunctionDiffIR(
          static_cast<const FunctionIR*>(old_message),
          static_cast<const FunctionIR*>(new_message),
          type_stack);
    default:
      break;
  }
  llvm::errs() << "Dump Diff attempted on something not a user defined type" <<
                   "/ function / global variable\n";
  return false;
}

bool ProtobufIRDiffDumper::AddLinkableMessageIR(
    const LinkableMessageIR *message) {
  //TODO: Add a parameter for added / removed.
  switch (message->GetKind()) {
    case RecordTypeKind:
      return AddLoneRecordTypeDiffIR(
          static_cast<const RecordTypeIR *>(message),
          diff_tu_->add_record_types_removed());
    case EnumTypeKind:
      return AddLoneEnumTypeDiffIR(
          static_cast<const EnumTypeIR *>(message),
          diff_tu_->add_enum_types_removed());
    case GlobalVarKind:
      return AddLoneGlobalVarDiffIR(
          static_cast<const GlobalVarIR*>(message),
          diff_tu_->add_global_vars_removed());
    case FunctionKind:
      return AddLoneFunctionDiffIR(
          static_cast<const FunctionIR*>(message),
          diff_tu_->add_functions_removed());
    default:
      break;
  }
  llvm::errs() << "Dump Diff attempted on something not a user defined type" <<
                   "/ function / global variable\n";
  return false;
}

bool ProtobufIRDiffDumper::AddLoneRecordTypeDiffIR(
    const RecordTypeIR *recordp,
    abi_dump::RecordType *abi_dump_record) {
  if (!abi_dump_record) {
    return false;
  }
  *abi_dump_record = ConvertRecordTypeIR(recordp);
  return true;
}

bool ProtobufIRDiffDumper::AddLoneFunctionDiffIR(
    const FunctionIR *functionp,
    abi_dump::FunctionDecl *abi_dump_function) {
  if (!abi_dump_function) {
    return false;
  }
  *abi_dump_function = ConvertFunctionIR(functionp);
  return true;

}

bool ProtobufIRDiffDumper::AddLoneEnumTypeDiffIR(
    const EnumTypeIR *enump, abi_dump::EnumType *abi_dump_enum) {
  if (!abi_dump_enum) {
    return false;
  }
  *abi_dump_enum = ConvertEnumTypeIR(enump);
  return true;

}

bool ProtobufIRDiffDumper::AddLoneGlobalVarDiffIR(
    const GlobalVarIR *global_varp, abi_dump::GlobalVarDecl *abi_dump_globvar) {
  if (!abi_dump_globvar) {
    return false;
  }
  *abi_dump_globvar = ConvertGlobalVarIR(global_varp);
  return true;
}


bool ProtobufIRDiffDumper::AddRecordTypeDiffIR(
    const RecordTypeIR *old_recordp, const RecordTypeIR *new_recordp,
    const std::string &type_stack) {
  abi_diff::RecordTypeDiff *added_record_diff_type =
      diff_tu_->add_unsafe_record_type_diffs();
  if (!added_record_diff_type) {
    return false;
  }
  abi_dump::RecordType *old_record = added_record_diff_type->mutable_old();
  abi_dump::RecordType *new_record = added_record_diff_type->mutable_new_();
  if (!old_record || !new_record) {
    llvm::errs() << "Could not create Record diff\n";
    return false;
  }
  *old_record = ConvertRecordTypeIR(old_recordp);
  *new_record = ConvertRecordTypeIR(new_recordp);
  added_record_diff_type->set_type_stack(type_stack);
  return true;
}

bool ProtobufIRDiffDumper::AddFunctionDiffIR(const FunctionIR *old_functionp,
                                             const FunctionIR *new_functionp,
                                             const std::string &type_stack) {
  abi_diff::FunctionDeclDiff *added_function_diff =
      diff_tu_->add_unsafe_function_diffs();
  if (!added_function_diff) {
    return false;
  }
  abi_dump::FunctionDecl *old_function =
      added_function_diff->mutable_old();
  abi_dump::FunctionDecl *new_function =
      added_function_diff->mutable_new_();
  if (!old_function || !new_function) {
    llvm::errs() << "Could not create Function diff\n";
    return false;
  }
  *old_function = ConvertFunctionIR(old_functionp);
  *new_function = ConvertFunctionIR(new_functionp);
  added_function_diff->set_type_stack(type_stack);
  return true;
}

bool ProtobufIRDiffDumper::AddEnumTypeDiffIR(const EnumTypeIR *old_enump,
                                             const EnumTypeIR *new_enump,
                                             const std::string &type_stack) {
  abi_diff::EnumTypeDiff *added_enum_diff_type =
      diff_tu_->add_unsafe_enum_type_diffs();
  if (!added_enum_diff_type) {
    return false;
  }
  abi_dump::EnumType *old_enum = added_enum_diff_type->mutable_old();
  abi_dump::EnumType *new_enum = added_enum_diff_type->mutable_new_();
  if (!old_enum || !new_enum) {
    llvm::errs() << "Could not create Enum diff\n";
    return false;
  }
  *old_enum = ConvertEnumTypeIR(old_enump);
  *new_enum = ConvertEnumTypeIR(new_enump);
  added_enum_diff_type->set_type_stack(type_stack);
  return true;
}

bool ProtobufIRDiffDumper::AddGlobalVarDiffIR(
    const GlobalVarIR *old_global_varp,
    const GlobalVarIR *new_global_varp,
    const std::string &type_stack) {
  abi_diff::GlobalVarDeclDiff *added_global_var_diff =
      diff_tu_->add_unsafe_global_var_diffs();
  if (!added_global_var_diff) {
    return false;
  }
  abi_dump::GlobalVarDecl *old_global_var =
      added_global_var_diff->mutable_old();
  abi_dump::GlobalVarDecl *new_global_var =
      added_global_var_diff->mutable_new_();
  if (!old_global_var || !new_global_var) {
    llvm::errs() << "Could not create GlobalVar diff\n";
    return false;
  }
  *old_global_var = ConvertGlobalVarIR(old_global_varp);
  *new_global_var = ConvertGlobalVarIR(new_global_varp);
  added_global_var_diff->set_type_stack(type_stack);
  return true;
}

bool ProtobufIRDiffDumper::Dump() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  assert(diff_tu_.get() != nullptr);
  std::ofstream text_output(dump_path_);
  google::protobuf::io::OstreamOutputStream text_os(&text_output);
  return google::protobuf::TextFormat::Print(*diff_tu_.get(), &text_os);
}

} //abi_util

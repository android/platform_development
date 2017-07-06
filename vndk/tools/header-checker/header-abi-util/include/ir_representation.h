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
#ifndef IR_
#define IR_

#include <llvm/Support/raw_ostream.h>

#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <memory>

// Classes which act as middle-men between clang AST parsing routines and
// message format specific dumpers.
namespace abi_util {

enum CompatibilityStatusIR {
  COMPATIBLE = 0,
  EXTENSION = 1,
  INCOMPATIBLE = 4
};

enum AccessSpecifierIR {
  public_access = 1,
  protected_access = 2,
  private_access = 3
};

enum LinkableMessageKind {
  RecordTypeKind,
  EnumTypeKind,
  PointerTypeKind,
  QualifiedTypeKind,
  ArrayTypeKind,
  LvalueReferenceTypeKind,
  RvalueReferenceTypeKind,
  BuiltinTypeKind,
  FunctionKind,
  GlobalVarKind
};

class LinkableMessageIR {
 public:
  const std::string &GetLinkerSetKey() const {
    return linker_set_key_;
  }
  void SetSourceFile(const std::string &source_file) {
    source_file_ = source_file;
  }
  void SetLinkerSetKey(const std::string &linker_set_key) {
    linker_set_key_ = linker_set_key;
  }
  const std::string &GetSourceFile() const {
    return source_file_;
  }
  virtual LinkableMessageKind GetKind() const = 0;
  virtual ~LinkableMessageIR() {}
 protected:
 // The source file where this message comes from. This will be an empty string
 // for built-in types
  std::string source_file_;
  std::string linker_set_key_;
};

class BasicTypeInfoIR {
 public:
  BasicTypeInfoIR(const std::string &name, const std::string &type,
                  const std::string linker_set_key, uint64_t size,
                  uint32_t alignment)
      : name_(name), referenced_type_(type), linker_set_key_(linker_set_key) ,
      size_(size), alignment_(alignment) { }
  BasicTypeInfoIR() { }
  std::string GetLinkerSetKey() {
    return linker_set_key_;
  }
 protected:
  std::string name_;
  std::string referenced_type_;
  std::string linker_set_key_;
  uint64_t size_;
  uint32_t alignment_;
};

// TODO: Break this up into types with sizes and those without types ?
class TypeIR : public LinkableMessageIR {
 public:
  void SetName(const std::string &name) {
    name_ = name;
  }
  const std::string &GetName() const {
    return name_;
  }
  void SetReferencedType(const std::string &type) {
    referenced_type_ = type;
  }
  const std::string &GetReferencedType() const {
    return referenced_type_;
  }
  void SetSize(uint64_t size) {
    size_ = size;
  }
  uint64_t GetSize() const {
   return size_;
  }
  void SetAlignment(uint32_t alignment) {
    alignment_ = alignment;
  }
  uint32_t GetAlignment() const {
    return alignment_;
  }
  virtual ~TypeIR() {}
 protected:
  std::string name_;
  std::string referenced_type_;
  uint64_t size_;
  uint32_t alignment_;
};

class VTableComponentIR {
 public:
  enum Kind {
    VCallOffset = 0,
    VBaseOffset = 1,
    OffsetToTop = 2,
    RTTI = 3,
    FunctionPointer = 4,
    CompleteDtorPointer = 5,
    DeletingDtorPointer = 6,
    UnusedFunctionPointer = 7
  };
  VTableComponentIR(const std::string &name, Kind kind, int64_t value)
      : component_name_(name), kind_(kind), value_(value) { }
  VTableComponentIR() { }
  void SetComponentName(std::string &name);
  void SetKind(Kind kind);
  Kind GetKind() const {
    return kind_;
  }
  int64_t GetValue() const {
    return value_;
  }
  const std::string &GetName() const {
    return component_name_;
  }
  void SetValue(int64_t value);
 protected:
  std::string component_name_;
  Kind kind_;
  int64_t value_;
};

class VTableLayoutIR {
 public:
  void AddVTableComponent(VTableComponentIR &&vtable_component) {
    vtable_components_.emplace_back(std::move(vtable_component));
  }
  const std::vector<VTableComponentIR> &GetVTableComponents() const {
    return vtable_components_;
  }
  uint64_t GetVTableNumEntries() const {
    return vtable_components_.size();
  }
 protected:
  std::vector<VTableComponentIR> vtable_components_;
};

class CXXBaseSpecifierIR {
 public:
  CXXBaseSpecifierIR(const std::string &type, bool is_virtual,
                     AccessSpecifierIR access) :
    referenced_type_(type), is_virtual_(is_virtual), access_(access) { }
  CXXBaseSpecifierIR() { }
  const std::string &GetReferencedType() const {
    return referenced_type_;
  }
  bool IsVirtual() const {
    return is_virtual_;
  }
  AccessSpecifierIR GetAccess() const {
    return access_;
  }
 protected:
  std::string referenced_type_;
  bool is_virtual_;
  AccessSpecifierIR access_;
};

class TemplateElementIR {
 public:
  TemplateElementIR(std::string &&type)
      :  referenced_type_(std::move(type)) { }
  TemplateElementIR(const std::string &type)
      :  referenced_type_(type) { }
  TemplateElementIR() { }
  const std::string &GetReferencedType() const {
    return referenced_type_;
  }
 protected:
  std::string referenced_type_;
};

class TemplateInfoIR {
 public:
  void AddTemplateElement(TemplateElementIR &&element) {
    template_elements_.emplace_back(element);
  }

  const std::vector<TemplateElementIR> &GetTemplateElements() const {
    return template_elements_;
  }
 protected:
  std::vector<TemplateElementIR> template_elements_;
};

class TemplatedArtifactIR {
 public:
  void SetTemplateInfo(TemplateInfoIR &&template_info) {
    template_info_ = std::move(template_info);
  }
  const std::vector<TemplateElementIR> &GetTemplateElements() const {
    return template_info_.GetTemplateElements();
  }
 protected:
  TemplateInfoIR template_info_;
};

class RecordFieldIR {
 public:
  RecordFieldIR(const std::string &name, const std::string &type,
                uint64_t offset, AccessSpecifierIR access)
      : name_(name), referenced_type_(type), offset_(offset),
      access_(access) { }
  RecordFieldIR() { }

  const std::string &GetName() const {
    return name_;
  }
  const std::string &GetReferencedType() const {
    return referenced_type_;
  }
  uint64_t GetOffset() const {
    return offset_;
  }
  AccessSpecifierIR GetAccess() const {
    return access_;
  }
 protected:
  std::string name_;
  std::string referenced_type_;
  uint64_t offset_;
  AccessSpecifierIR access_;
};

class RecordTypeIR: public TypeIR, public TemplatedArtifactIR {
 public:
  // Add Methods to get information from the IR.
  void AddRecordField(RecordFieldIR &&field) {
    fields_.emplace_back(std::move(field));
  }
  void SetRecordFields(std::vector<RecordFieldIR> &&fields) {
    fields_ = std::move(fields);
  }
  void SetVTableLayout(VTableLayoutIR &&vtable_layout) {
    vtable_layout_ = std::move(vtable_layout);
  }
  const VTableLayoutIR &GetVTableLayout() const {
    return vtable_layout_;
  }
  void AddCXXBaseSpecifier(CXXBaseSpecifierIR &&base_specifier) {
    bases_.emplace_back(std::move(base_specifier));
  };
  void SetCXXBaseSpecifiers(std::vector<CXXBaseSpecifierIR> &&bases) {
    bases_ = std::move(bases);
  }
  const std::vector<CXXBaseSpecifierIR> &GetBases() const {
    return bases_;
  }

  void SetAccess(AccessSpecifierIR access) { access_ = access;}

  AccessSpecifierIR GetAccess() const {
    return access_;
  }

  const std::vector<RecordFieldIR> &GetFields() const {
    return fields_;
  }

  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::RecordTypeKind;
  }

  uint64_t GetVTableNumEntries() const {
    return vtable_layout_.GetVTableNumEntries();
  }
 protected:
  std::vector<RecordFieldIR> fields_;
  VTableLayoutIR vtable_layout_;
  std::vector<CXXBaseSpecifierIR> bases_;
  AccessSpecifierIR access_;
};

class EnumFieldIR {
 public:
  EnumFieldIR(const std::string &name, int value)
      : name_(name), value_(value) { }
  const std::string &GetName() const {
    return name_;
  }

  int GetValue() const {
    return value_;
  }
 protected:
  std::string name_;
  int value_;
};

class EnumTypeIR : public TypeIR {
 public:
  // Add Methods to get information from the IR.
  void AddEnumField(EnumFieldIR &&field) {
    fields_.emplace_back(std::move(field));
  }
  void SetAccess(AccessSpecifierIR access) { access_ = access;}
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::EnumTypeKind;
  }
  AccessSpecifierIR GetAccess() const {
    return access_;
  }
  void SetUnderlyingType(std::string &&underlying_type) {
    underlying_type_ = std::move(underlying_type);
  }
  void SetUnderlyingType(const std::string &underlying_type) {
    underlying_type_ = underlying_type;
  }
  const std::string &GetUnderlyingType() const {
    return underlying_type_;
  }
  void SetFields(std::vector<EnumFieldIR> &&fields) {
    fields_ = std::move(fields);
  }
  const std::vector<EnumFieldIR> &GetFields() const {
    return fields_;
  }
 protected:
  std::vector<EnumFieldIR> fields_;
  std::string underlying_type_;
  AccessSpecifierIR access_;
};

class ArrayTypeIR : public TypeIR {
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::ArrayTypeKind;
  }
};

class PointerTypeIR : public TypeIR {
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::PointerTypeKind;
  }
};

class BuiltinTypeIR : public TypeIR {
 public:
  void SetSignedness(bool is_unsigned) {
    is_unsigned_ = is_unsigned;
  }
  bool IsUnsigned() const {
    return is_unsigned_;
  }
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::BuiltinTypeKind;
  }
 protected:
  bool is_unsigned_;
};

class LvalueReferenceTypeIR : public TypeIR {
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::LvalueReferenceTypeKind;
  }
};

class RvalueReferenceTypeIR : public TypeIR {
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::RvalueReferenceTypeKind;
  }
};

class QualifiedTypeIR : public TypeIR {
 public:
  void SetConstness(bool is_const) {
    is_const_ = is_const;
  }
  bool IsConst() const {
    return is_const_;
  }
  void SetRestrictedness(bool is_restricted) {
    is_restricted_ = is_restricted;
  }
  bool IsRestricted() const {
    return is_restricted_;
  }
  void SetVolatility(bool is_volatile) {
    is_volatile_ = is_volatile;
  }
  bool IsVolatile() const {
    return is_volatile_;
  }
 public:
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::QualifiedTypeKind;
  }
 protected:
  bool is_const_;
  bool is_restricted_;
  bool is_volatile_;
};

class GlobalVarIR: public LinkableMessageIR {
 public:
  // Add Methods to get information from the IR.
  void SetReferencedType(const std::string &type) {
    referenced_type_ =  type;
  }

  const std::string &GetReferencedType() const {
    return referenced_type_;
  }

  void SetName(std::string &&name) {
    name_ = std::move(name);
  }

  void SetName(const std::string &name) {
    name_ = name;
  }

  const std::string &GetName() const {
    return name_;
  }

  AccessSpecifierIR GetAccess() const {
    return access_;
  }

  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::GlobalVarKind;
  }

 protected:
  std::string referenced_type_; // underlying type
  std::string name_;
  AccessSpecifierIR access_;
};

class ParamIR {
 public:
  ParamIR(const std::string &type, bool is_default) :
    referenced_type_(type) , is_default_(is_default) {}

  const std::string &GetReferencedType() const {
    return referenced_type_;
  }

  bool GetIsDefault() const {
    return is_default_;
  }

 protected:
  std::string referenced_type_;
  bool is_default_;
};

class FunctionIR : public LinkableMessageIR, public TemplatedArtifactIR {
 public:
  void SetReferencedType(const std::string &type) {
    referenced_type_ = type;
  }
  const std::string &GetReferencedType() const {
    return referenced_type_;
  }
  void AddParameter(ParamIR &&parameter) {
    parameters_.emplace_back(std::move(parameter));
  }
  void SetAccess(AccessSpecifierIR access) {
    access_ = access;
  }
  AccessSpecifierIR GetAccess() const {
    return access_;
  }
  LinkableMessageKind GetKind() const override {
    return LinkableMessageKind::FunctionKind;
  }
  const std::vector<ParamIR> &GetParameters() const {
    return parameters_;
  }
 protected:
    std::string referenced_type_; // return type reference
    std::string linkage_name_;
    std::string qualified_name_;
    std::vector<ParamIR> parameters_;
    AccessSpecifierIR access_;
};

class IRDumper {
 public:
  IRDumper(const std::string &dump_path) : dump_path_(dump_path) { }

  static std::unique_ptr<IRDumper> CreateIRDumper(const std::string &type,
                                                  const std::string &dump_path);

  virtual bool AddLinkableMessageIR(const LinkableMessageIR *) = 0;

  virtual bool Dump() = 0;

  virtual ~IRDumper() {}

 protected:
  const std::string &dump_path_;
};

class TextFormatToIRReader {
 public:
  TextFormatToIRReader(const std::string &dump_path) : dump_path_(dump_path) { }
  const std::vector<FunctionIR> &GetFunctions() const {
    return functions_;
  }
  const std::vector<GlobalVarIR> &GetGlobalVariables() const {
    return global_variables_;
  }
  const std::vector<RecordTypeIR> &GetRecordTypes() const {
    return record_types_;
  }
  const std::vector<EnumTypeIR> &GetEnumTypes() const {
    return enum_types_;
  }
  const std::vector<LvalueReferenceTypeIR> &GetLvalueReferenceTypes() const {
    return lvalue_reference_types_;
  }
  const std::vector<RvalueReferenceTypeIR> &GetRvalueReferenceTypes() const {
    return rvalue_reference_types_;
  }
  const std::vector<QualifiedTypeIR> &GetQualifiedTypes() const {
    return qualified_types_;
  }
  const std::vector<ArrayTypeIR> &GetArrayTypes() const {
    return array_types_;
  }
  const std::vector<PointerTypeIR> &GetPointerTypes() const {
    return pointer_types_;
  }
  const std::vector<BuiltinTypeIR> &GetBuiltinTypes() const {
    return builtin_types_;
  }
  virtual bool ReadDump() = 0;
  virtual ~TextFormatToIRReader() {}

  static std::unique_ptr<TextFormatToIRReader> CreateTextFormatToIRReader(
      const std::string &text_format, const std::string &dump_path);
 protected:
  const std::string &dump_path_;

  std::vector<FunctionIR> functions_;
  std::vector<GlobalVarIR> global_variables_;
  std::vector<RecordTypeIR> record_types_;
  std::vector<EnumTypeIR> enum_types_;
  std::vector<PointerTypeIR> pointer_types_;
  std::vector<LvalueReferenceTypeIR> lvalue_reference_types_;
  std::vector<RvalueReferenceTypeIR> rvalue_reference_types_;
  std::vector<ArrayTypeIR> array_types_;
  std::vector<BuiltinTypeIR> builtin_types_;
  std::vector<QualifiedTypeIR> qualified_types_;
};

class IRDiffDumper {
 public:
  IRDiffDumper(const std::string &dump_path) : dump_path_(dump_path) { }
  virtual bool AddLinkableMessagesIR(const LinkableMessageIR *,
                                     const LinkableMessageIR *,
                                     const std::string &type_stack) = 0;
  virtual bool AddLinkableMessageIR(const LinkableMessageIR *) = 0;

  virtual bool Dump() = 0;

  virtual void AddLibNameIR(const std::string &name) = 0;

  virtual void AddArchIR(const std::string &arch) = 0;

  virtual void AddCompatibilityStatusIR(CompatibilityStatusIR status) = 0;

  virtual ~IRDiffDumper() {}
  static std::unique_ptr<IRDiffDumper> CreateIRDiffDumper(
      const std::string &type, const std::string &dump_path);
 protected:
  const std::string &dump_path_;
};

} // namespace abi_util

#endif // IR_

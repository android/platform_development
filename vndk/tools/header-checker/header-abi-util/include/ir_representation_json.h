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
#ifndef IR_JSON_
#define IR_JSON_

#include <ir_representation.h>

#include <json/value.h>

// Classes which act as middle-men between clang AST parsing routines and
// message format specific dumpers.
namespace abi_util {

enum AccessSpecifier {
  public_access = 1,
  private_access = 2,
  protected_access = 3,
};

enum RecordKind {
  struct_kind = 1,
  class_kind = 2,
  union_kind = 3,
};

enum VTableComponentKind {
  VCallOffset = 0,
  VBaseOffset = 1,
  OffsetToTop = 2,
  RTTI = 3,
  FunctionPointer = 4,
  CompleteDtorPointer = 5,
  DeletingDtorPointer = 6,
  UnusedFunctionPointer = 7,
};

inline AccessSpecifier AccessIRToJson(AccessSpecifierIR access) {
  switch (access) {
  case AccessSpecifierIR::PublicAccess:
    return AccessSpecifier::public_access;

  case AccessSpecifierIR::ProtectedAccess:
    return AccessSpecifier::protected_access;

  case AccessSpecifierIR::PrivateAccess:
    return AccessSpecifier::private_access;

  default:
    return AccessSpecifier::public_access;
  }
  // Should not be reached
  assert(false);
}

inline AccessSpecifierIR AccessJsonToIR(int64_t access) {
  switch (access) {
  case AccessSpecifier::public_access:
    return AccessSpecifierIR::PublicAccess;

  case AccessSpecifier::protected_access:
    return AccessSpecifierIR::ProtectedAccess;

  case AccessSpecifier::private_access:
    return AccessSpecifierIR::PrivateAccess;

  default:
    return AccessSpecifierIR::PublicAccess;
  }
  // Should not be reached
  assert(false);
}

inline RecordKind RecordKindIRToJson(RecordTypeIR::RecordKind kind) {
  switch (kind) {
  case RecordTypeIR::RecordKind::struct_kind:
    return RecordKind::struct_kind;

  case RecordTypeIR::RecordKind::class_kind:
    return RecordKind::class_kind;

  case RecordTypeIR::RecordKind::union_kind:
    return RecordKind::union_kind;

  default:
    return RecordKind::struct_kind;
  }
  // Should not be reached
  assert(false);
}

inline RecordTypeIR::RecordKind RecordKindJsonToIR(int64_t kind) {
  switch (kind) {
  case RecordKind::struct_kind:
    return RecordTypeIR::struct_kind;

  case RecordKind::class_kind:
    return RecordTypeIR::class_kind;

  case RecordKind::union_kind:
    return RecordTypeIR::union_kind;

  default:
    return RecordTypeIR::struct_kind;
  }
  // Should not be reached
  assert(false);
}

inline VTableComponentKind
VTableComponentKindIRToJson(VTableComponentIR::Kind kind) {
  switch (kind) {
  case VTableComponentIR::Kind::VCallOffset:
    return VTableComponentKind::VCallOffset;

  case VTableComponentIR::Kind::VBaseOffset:
    return VTableComponentKind::VBaseOffset;

  case VTableComponentIR::Kind::OffsetToTop:
    return VTableComponentKind::OffsetToTop;

  case VTableComponentIR::Kind::RTTI:
    return VTableComponentKind::RTTI;

  case VTableComponentIR::Kind::FunctionPointer:
    return VTableComponentKind::FunctionPointer;

  case VTableComponentIR::Kind::CompleteDtorPointer:
    return VTableComponentKind::CompleteDtorPointer;

  case VTableComponentIR::Kind::DeletingDtorPointer:
    return VTableComponentKind::DeletingDtorPointer;

  case VTableComponentIR::Kind::UnusedFunctionPointer:
    return VTableComponentKind::UnusedFunctionPointer;

  default:
    return VTableComponentKind::UnusedFunctionPointer;
  }
  // Should not be reached
  assert(false);
}

inline VTableComponentIR::Kind VTableComponentKindJsonToIR(int64_t kind) {
  switch (kind) {
  case VTableComponentKind::VCallOffset:
    return VTableComponentIR::Kind::VCallOffset;

  case VTableComponentKind::VBaseOffset:
    return VTableComponentIR::Kind::VBaseOffset;

  case VTableComponentKind::OffsetToTop:
    return VTableComponentIR::Kind::OffsetToTop;

  case VTableComponentKind::RTTI:
    return VTableComponentIR::Kind::RTTI;

  case VTableComponentKind::FunctionPointer:
    return VTableComponentIR::Kind::FunctionPointer;

  case VTableComponentKind::CompleteDtorPointer:
    return VTableComponentIR::Kind::CompleteDtorPointer;

  case VTableComponentKind::DeletingDtorPointer:
    return VTableComponentIR::Kind::DeletingDtorPointer;

  case VTableComponentKind::UnusedFunctionPointer:
    return VTableComponentIR::Kind::UnusedFunctionPointer;

  default:
    return VTableComponentIR::Kind::UnusedFunctionPointer;
  }
  // Should not be reached
  assert(false);
}

// Classes that wrap constructor of Json::Value.
class JsonArray : public Json::Value {
 public:
  JsonArray() : Json::Value(Json::ValueType::arrayValue) {}
};

class JsonObject : public Json::Value {
 public:
  JsonObject() : Json::Value(Json::ValueType::objectValue) {}
};

class IRToJsonConverter {
 private:
  static void AddTemplateInfo(JsonObject &type_decl,
                              const abi_util::TemplatedArtifactIR *template_ir);

  // BasicNamedAndTypedDecl
  static void AddTypeInfo(JsonObject &type_decl, const TypeIR *type_ir);

  static void AddRecordFields(JsonObject &record_type,
                              const RecordTypeIR *record_ir);

  static void AddBaseSpecifiers(JsonObject &record_type,
                                const RecordTypeIR *record_ir);

  static void AddVTableLayout(JsonObject &record_type,
                              const RecordTypeIR *record_ir);

  static void AddTagTypeInfo(JsonObject &tag_type,
                             const TagTypeIR *tag_type_ir);

  static void AddEnumFields(JsonObject &enum_type, const EnumTypeIR *enum_ir);

 public:
  static JsonObject ConvertEnumTypeIR(const EnumTypeIR *enump);

  static JsonObject ConvertRecordTypeIR(const RecordTypeIR *recordp);

  static JsonObject ConvertFunctionTypeIR(const FunctionTypeIR *function_typep);

  static void AddFunctionParametersAndSetReturnType(
      JsonObject &function, const CFunctionLikeIR *cfunction_like_ir);

  static void AddFunctionParameters(JsonObject &function,
                                    const CFunctionLikeIR *cfunction_like_ir);

  static JsonObject ConvertFunctionIR(const FunctionIR *functionp);

  static JsonObject ConvertGlobalVarIR(const GlobalVarIR *global_varp);

  static JsonObject ConvertPointerTypeIR(const PointerTypeIR *pointerp);

  static JsonObject ConvertQualifiedTypeIR(const QualifiedTypeIR *qualtypep);

  static JsonObject ConvertBuiltinTypeIR(const BuiltinTypeIR *builtin_typep);

  static JsonObject ConvertArrayTypeIR(const ArrayTypeIR *array_typep);

  static JsonObject ConvertLvalueReferenceTypeIR(
      const LvalueReferenceTypeIR *lvalue_reference_typep);

  static JsonObject ConvertRvalueReferenceTypeIR(
      const RvalueReferenceTypeIR *rvalue_reference_typep);

  static JsonObject ConvertElfFunctionIR(const ElfFunctionIR *elf_function_ir);

  static JsonObject ConvertElfObjectIR(const ElfObjectIR *elf_object_ir);
};

class JsonIRDumper : public IRDumper, public IRToJsonConverter {
 public:
  JsonIRDumper(const std::string &dump_path);

  bool AddLinkableMessageIR(const LinkableMessageIR *) override;

  bool AddElfSymbolMessageIR(const ElfSymbolIR *) override;

  bool Dump() override;

  ~JsonIRDumper() override {}

 private:
  JsonObject translation_unit_;
};

template <typename T> class JsonArrayRef;

// The class that loads values from a read-only JSON object.
class JsonObjectRef {
 public:
  // The constructor sets ok to false if json_value is not an object.
  JsonObjectRef(const Json::Value &json_value, bool &ok);

  // This method gets a value from the object and checks the type.
  // If the type mismatches, it sets ok_ to false and returns default value.
  // If the key doesn't exist, it doesn't change ok_ and returns default value.
  // Default to false.
  bool GetBool(const std::string key) const;

  // Default to 0.
  int64_t GetInt(const std::string &key) const;

  // Default to 0.
  uint64_t GetUint(const std::string &key) const;

  // Default to "".
  std::string GetString(const std::string &key) const;

  // Default to {}.
  JsonObjectRef GetObject(const std::string &key) const;

  // Default to [].
  JsonArrayRef<JsonObjectRef> GetObjects(const std::string &key) const;

 private:
  const Json::Value &Get(const std::string &key,
                         const Json::Value &default_value,
                         bool (Json::Value::*is_expected_type)() const) const;

  const Json::Value &object_;
  bool &ok_;
};

// The class that loads elements as type T from a read-only JSON array.
template <typename T> class JsonArrayRef {
 public:
  class Iterator {
  public:
    Iterator(const Json::Value &json_value, bool &ok, int index)
        : array_(json_value), ok_(ok), index_(index) {}

    Iterator &operator++() {
      ++index_;
      return *this;
    }

    bool operator!=(const Iterator &other) const {
      return index_ != other.index_;
    }

    T operator*() const;

  private:
    const Json::Value &array_;
    bool &ok_;
    int index_;
  };

  // The caller ensures json_value.isArray() == true.
  JsonArrayRef(const Json::Value &json_value, bool &ok)
      : array_(json_value), ok_(ok) {}

  Iterator begin() const { return Iterator(array_, ok_, 0); }

  Iterator end() const { return Iterator(array_, ok_, array_.size()); }

 private:
  const Json::Value &array_;
  bool &ok_;
};

template <>
JsonObjectRef JsonArrayRef<JsonObjectRef>::Iterator::operator*() const;

class JsonToIRReader : public TextFormatToIRReader {
 public:
  JsonToIRReader(const std::set<std::string> *exported_headers)
      : TextFormatToIRReader(exported_headers) {}

  bool ReadDump(const std::string &dump_file) override;

 private:
  void ReadFunctions(const JsonObjectRef &tu);

  void ReadGlobalVariables(const JsonObjectRef &tu);

  void ReadEnumTypes(const JsonObjectRef &tu);

  void ReadRecordTypes(const JsonObjectRef &tu);

  void ReadFunctionTypes(const JsonObjectRef &tu);

  void ReadPointerTypes(const JsonObjectRef &tu);

  void ReadBuiltinTypes(const JsonObjectRef &tu);

  void ReadQualifiedTypes(const JsonObjectRef &tu);

  void ReadArrayTypes(const JsonObjectRef &tu);

  void ReadLvalueReferenceTypes(const JsonObjectRef &tu);

  void ReadRvalueReferenceTypes(const JsonObjectRef &tu);

  void ReadElfFunctions(const JsonObjectRef &tu);

  void ReadElfObjects(const JsonObjectRef &tu);

  static void ReadTypeInfo(const JsonObjectRef &type_info, TypeIR *type_ir);

  static void ReadTagTypeInfo(const JsonObjectRef &tag_type,
                              TagTypeIR *tag_type_ir);

  static void ReadFunctionParametersAndReturnType(const JsonObjectRef &function,
                                                  CFunctionLikeIR *function_ir);

  static FunctionIR FunctionJsonToIR(const JsonObjectRef &function);

  static FunctionTypeIR
  FunctionTypeJsonToIR(const JsonObjectRef &function_type);

  static RecordTypeIR RecordTypeJsonToIR(const JsonObjectRef &record_type);

  static std::vector<RecordFieldIR>
  RecordFieldsJsonToIR(const JsonArrayRef<JsonObjectRef> &fields);

  static std::vector<CXXBaseSpecifierIR>
  BaseSpecifiersJsonToIR(const JsonArrayRef<JsonObjectRef> &base_specifiers);

  static std::vector<EnumFieldIR>
  EnumFieldsJsonToIR(const JsonArrayRef<JsonObjectRef> &enum_fields);

  static EnumTypeIR EnumTypeJsonToIR(const JsonObjectRef &enum_type);

  static VTableLayoutIR
  VTableLayoutJsonToIR(const JsonObjectRef &vtable_layout);

  static TemplateInfoIR
  TemplateInfoJsonToIR(const JsonObjectRef &template_info);
};
} // namespace abi_util

#endif // IR_JSON_

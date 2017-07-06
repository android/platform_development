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

#include "abi_wrappers.h"
#include "QualTypeNames.h"

#include <header_abi_util.h>

#include <limits.h>
#include <stdlib.h>
#include <clang/Tooling/Core/QualTypeNames.h>
#include <clang/Index/CodegenNameGenerator.h>

#include <string>

using namespace abi_wrapper;

ABIWrapper::ABIWrapper(
    clang::MangleContext *mangle_contextp,
    clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *cip,
    std::set<std::string> *type_cache,
    abi_util::IRDumper *ir_dumper,
    std::map<const clang::Decl *, std::string> &decl_to_source_cache)
  : cip_(cip),
    mangle_contextp_(mangle_contextp),
    ast_contextp_(ast_contextp),
    type_cache_(type_cache),
    ir_dumper_(ir_dumper),
    decl_to_source_file_cache_(decl_to_source_cache) { }

std::string ABIWrapper::GetCachedDeclSourceFile(
    const clang::Decl *decl, const clang::CompilerInstance *cip) {
  assert(decl != nullptr);
  auto result = decl_to_source_file_cache_.find(decl);
  if (result == decl_to_source_file_cache_.end()) {
    return GetDeclSourceFile(decl, cip);
  }
  return result->second;
}

std::string ABIWrapper::GetDeclSourceFile(const clang::Decl *decl,
                                          const clang::CompilerInstance *cip) {
  clang::SourceManager &sm = cip->getSourceManager();
  clang::SourceLocation location = decl->getLocation();
  // We need to use the expansion location to identify whether we should recurse
  // into the AST Node or not. For eg: macros specifying LinkageSpecDecl can
  // have their spelling location defined somewhere outside a source / header
  // file belonging to a library. This should not allow the AST node to be
  // skipped. Its expansion location will still be the source-file / header
  // belonging to the library.
  clang::SourceLocation expansion_location = sm.getExpansionLoc(location);
  llvm::StringRef file_name = sm.getFilename(expansion_location);
  std::string file_name_adjusted = "";
  char file_abs_path[PATH_MAX];
  if (realpath(file_name.str().c_str(), file_abs_path) == nullptr) {
    return "";
  }
  return file_abs_path;
}

static abi_util::AccessSpecifierIR AccessClangToIR(
    const clang::AccessSpecifier sp)  {
  switch (sp) {
    case clang::AS_private: {
      return abi_util::AccessSpecifierIR::private_access;
      break;
    }
    case clang::AS_protected: {
      return abi_util::AccessSpecifierIR::protected_access;
      break;
    }
    default: {
      return abi_util::AccessSpecifierIR::public_access;
      break;
    }
  }
}
// Get type 'referenced' by qual_type. Referenced type implies, in order:
// 1) Strip off all qualifiers if qual_type has CVR qualifiers.
// 2) Strip off a pointer level if qual_type is a pointer.
// 3) Strip off the reference if qual_type is a reference.
// Note: qual_type is expected to be a canonical type.
clang::QualType ABIWrapper::GetReferencedType(const clang::QualType qual_type) {
  const clang::Type *type_ptr = qual_type.getTypePtr();
  if (qual_type.hasLocalQualifiers()) {
    return qual_type.getLocalUnqualifiedType();
  }
  if (type_ptr->isPointerType()) {
    return type_ptr->getPointeeType();
  }
  if (type_ptr->isArrayType()) {
    return type_ptr->getArrayElementTypeNoTypeQual()->getCanonicalTypeInternal();
  }
  return qual_type.getNonReferenceType();
}

bool ABIWrapper::CreateExtendedType(
    clang::QualType qual_type,
    abi_util::TypeIR *typep) {
  std::string type_name = QualTypeToString(qual_type);
  if (!type_cache_->insert(type_name).second) {
    return true;
  }
  const clang::QualType canonical_type = qual_type.getCanonicalType();
  return CreateBasicNamedAndTypedDecl(canonical_type, typep);
}

//This overload takes in a qualtype and adds its information to the abi-dump on
//its own.
bool ABIWrapper::CreateBasicNamedAndTypedDecl(clang::QualType qual_type) {
  std::string type_name = QualTypeToString(qual_type);
  const clang::QualType canonical_type = qual_type.getCanonicalType();
  const clang::Type *base_type = canonical_type.getTypePtr();
  bool is_ptr = base_type->isPointerType();
  bool is_reference = base_type->isReferenceType();
  bool is_array = base_type->isArrayType();
  // TODO: Check API usage. Make different varaible for local qualifiers
  bool has_referenced_type =
      is_ptr || is_reference || is_array || canonical_type.hasLocalQualifiers();
  if (!has_referenced_type || !type_cache_->insert(type_name).second) {
    return true;
  }
  // Do something similar to what is being done right now. Create an object
  // extending Type and return a pointer to that and pass it to CreateBasic...
  // CreateBasic...(qualtype, Type *) fills in size, alignemnt etc.
  std::unique_ptr<abi_util::TypeIR> typep = SetTypeKind(canonical_type);
  if (!base_type->isVoidType() && !typep) {
    return false;
  }
  return CreateBasicNamedAndTypedDecl(canonical_type, typep.get()) &&
      ir_dumper_->AddLinkableMessageIR(typep.get());
}

// CreateBasicNamedAndTypedDecl creates a BasicNamedAndTypedDecl : that'll
// include all the generic information a basic type will have:
// abi_dump::BasicNamedAndTypedDecl. Other methods fill in more specific
// information, eg: RecordDecl, EnumDecl.
bool ABIWrapper::CreateBasicNamedAndTypedDecl(
    clang::QualType canonical_type,
    abi_util::TypeIR *typep) {
  // Cannot determine the size and alignment for template parameter dependent
  // types as well as incomplete types.
  const clang::Type *base_type = canonical_type.getTypePtr();
  assert(base_type != nullptr);
  clang::Type::TypeClass type_class = base_type->getTypeClass();
  // Temporary Hack for auto type sizes. Not determinable.
  if ((type_class != clang::Type::Auto) && !base_type->isIncompleteType() &&
      !(base_type->isDependentType())) {
    std::pair<clang::CharUnits, clang::CharUnits> size_and_alignment =
    ast_contextp_->getTypeInfoInChars(canonical_type);
    size_t size = size_and_alignment.first.getQuantity();
    size_t alignment = size_and_alignment.second.getQuantity();
    typep->SetSize(size);
    typep->SetAlignment(alignment);
  }
  // TODO: Fix this repeated use.
  typep->SetName(QualTypeToString(canonical_type));
  typep->SetLinkerSetKey(QualTypeToString(canonical_type));
  // default values are false, we don't set them since explicitly doing that
  // makes the abi dumps more verbose.
  // This type has a reference type if its a pointer / reference OR it has CVR
  // qualifiers.
  //TODO: Fill in CVR qualifiers.
  clang::QualType referenced_type = GetReferencedType(canonical_type);
  typep->SetReferencedType(QualTypeToString(referenced_type));
  // Create the type for referenced type.
  return CreateBasicNamedAndTypedDecl(referenced_type);
}

std::string ABIWrapper::GetTypeLinkageName(const clang::Type *typep)  {
  assert(typep != nullptr);
  clang::QualType qt = typep->getCanonicalTypeInternal();
  return QualTypeToString(qt);
}

std::unique_ptr<abi_util::TypeIR> ABIWrapper::SetTypeKind(
    const clang::QualType canonical_type) {
  // TODO: Fill in Qualifiers before returning.
  if (canonical_type.hasLocalQualifiers()) {
    return std::unique_ptr<abi_util::TypeIR>(new abi_util::QualifiedTypeIR());
  }
  const clang::Type *type_ptr = canonical_type.getTypePtr();
  if (type_ptr->isPointerType()) {
    return std::unique_ptr<abi_util::TypeIR>(new abi_util::PointerTypeIR());
  }
  if (type_ptr->isLValueReferenceType()) {
    return std::unique_ptr<abi_util::TypeIR>(
        new abi_util::LvalueReferenceTypeIR());
  }
  if (type_ptr->isRValueReferenceType()) {
    return std::unique_ptr<abi_util::TypeIR>(
        new abi_util::RvalueReferenceTypeIR());
  }
  if (type_ptr->isArrayType()) {
    return std::unique_ptr<abi_util::TypeIR>(new abi_util::ArrayTypeIR());
  }
  if (type_ptr->isEnumeralType()) {
    return std::unique_ptr<abi_util::TypeIR>(new abi_util::EnumTypeIR());
  }
  if (type_ptr->isRecordType()) {
    return std::unique_ptr<abi_util::TypeIR>(new abi_util::RecordTypeIR());
  }
  if (type_ptr->isBuiltinType()) {
    auto builtin_type =
        std::unique_ptr<abi_util::BuiltinTypeIR>(new abi_util::BuiltinTypeIR());
    builtin_type->SetSignedness(type_ptr->isUnsignedIntegerType());
    return builtin_type;
  }
  return nullptr;
}

std::string ABIWrapper::GetMangledNameDecl(
    const clang::NamedDecl *decl, clang::MangleContext *mangle_contextp) {
  if (!mangle_contextp->shouldMangleDeclName(decl)) {
    clang::IdentifierInfo *identifier = decl->getIdentifier();
    return identifier ? identifier->getName() : "";
  }
  std::string mangled_name;
  llvm::raw_string_ostream ostream(mangled_name);
  mangle_contextp->mangleName(decl, ostream);
  ostream.flush();
  return mangled_name;
}

std::string ABIWrapper::GetTagDeclQualifiedName(
    const clang::TagDecl *decl) {
  if (decl->getTypedefNameForAnonDecl()) {
    return decl->getTypedefNameForAnonDecl()->getQualifiedNameAsString();
  }
  return decl->getQualifiedNameAsString();
}

bool ABIWrapper::SetupTemplateArguments(
    const clang::TemplateArgumentList *tl,
    abi_util::TemplatedArtifactIR *ta) {
  abi_util::TemplateInfoIR template_info;
  for (int i = 0; i < tl->size(); i++) {
    const clang::TemplateArgument &arg = (*tl)[i];
    //TODO: More comprehensive checking needed.
    if (arg.getKind() != clang::TemplateArgument::Type) {
      continue;
    }
    clang::QualType type = arg.getAsType();
    template_info.AddTemplateElement(
        abi_util::TemplateElementIR(QualTypeToString(type)));
  }
  ta->SetTemplateInfo(std::move(template_info));
  // TODO: Add CreateBasicNamedAndTypedDecl and return that status as a bool
  return true;
}

std::string ABIWrapper::QualTypeToString(
    const clang::QualType &sweet_qt) {
  const clang::QualType salty_qt = sweet_qt.getCanonicalType();
  // clang::TypeName::getFullyQualifiedName removes the part of the type related
  // to it being a template parameter. Don't use it for dependent types.
  if (salty_qt.getTypePtr()->isDependentType()) {
    return salty_qt.getAsString();
  }
  return clang::TypeName::getFullyQualifiedTypeName(salty_qt, *ast_contextp_);
}

FunctionDeclWrapper::FunctionDeclWrapper(
    clang::MangleContext *mangle_contextp,
    clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::FunctionDecl *decl,
    std::set<std::string> *type_cache,
    abi_util::IRDumper *ir_dumper,
    std::map<const clang::Decl *, std::string> &decl_to_source_cache)
  : ABIWrapper(mangle_contextp, ast_contextp, compiler_instance_p,
               type_cache, ir_dumper, decl_to_source_cache),
    function_decl_(decl) { }

bool FunctionDeclWrapper::SetupFunctionParameters(
    abi_util::FunctionIR *functionp) {
  clang::FunctionDecl::param_const_iterator param_it =
      function_decl_->param_begin();
  while (param_it != function_decl_->param_end()) {
    // The linker set key is blank since that shows up in the mangled name.
    bool has_default_arg = (*param_it)->hasDefaultArg();
    clang::QualType param_qt = (*param_it)->getType();
    if (!CreateBasicNamedAndTypedDecl((*param_it)->getType())) {
      return false;
    }
    functionp->AddParameter(abi_util::ParamIR(QualTypeToString(param_qt),
                                              has_default_arg));
    param_it++;
  }
  return true;
}

bool FunctionDeclWrapper::SetupFunction(abi_util::FunctionIR *functionp,
                                        const std::string &source_file)  {
  // Go through all the parameters in the method and add them to the fields.
  // Also get the fully qualfied name.
  functionp->SetSourceFile(source_file);
  // Combine the function name and return type to form a NamedAndTypedDecl
  clang::QualType return_type = function_decl_->getReturnType();

  functionp->SetReferencedType(QualTypeToString(return_type));
  functionp->SetAccess(AccessClangToIR(function_decl_->getAccess()));
  return CreateBasicNamedAndTypedDecl(return_type) &&
      SetupFunctionParameters(functionp) && SetupTemplateInfo(functionp);
}

bool FunctionDeclWrapper::SetupTemplateInfo(
    abi_util::FunctionIR *functionp)  {
  switch (function_decl_->getTemplatedKind()) {
    case clang::FunctionDecl::TK_FunctionTemplateSpecialization: {
      const clang::TemplateArgumentList *arg_list =
          function_decl_->getTemplateSpecializationArgs();
      if (arg_list && !SetupTemplateArguments(arg_list, functionp)) {
        return false;
      }
      break;
    }
    default: {
      break;
    }
  }
  return true;
}

std::unique_ptr<abi_util::FunctionIR> FunctionDeclWrapper::GetFunctionDecl() {
  std::unique_ptr<abi_util::FunctionIR> abi_decl(
      new abi_util::FunctionIR());
  std::string source_file = GetCachedDeclSourceFile(function_decl_, cip_);
  if (!SetupFunction(abi_decl.get(), source_file)) {
    return nullptr;
  }
  return abi_decl;
}

RecordDeclWrapper::RecordDeclWrapper(
    clang::MangleContext *mangle_contextp,
    clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::RecordDecl *decl,
    std::set<std::string> *type_cache,
    abi_util::IRDumper *ir_dumper,
    std::map<const clang::Decl *, std::string> &decl_to_source_cache)
  : ABIWrapper(mangle_contextp, ast_contextp, compiler_instance_p,
               type_cache, ir_dumper, decl_to_source_cache),
    record_decl_(decl) { }

bool RecordDeclWrapper::SetupRecordFields(abi_util::RecordTypeIR *recordp) {
  clang::RecordDecl::field_iterator field = record_decl_->field_begin();
  int field_index = 0;
  const clang::ASTRecordLayout &record_layout =
      ast_contextp_->getASTRecordLayout(record_decl_);
  while (field != record_decl_->field_end()) {
    clang::QualType field_type = field->getType();
    if (!CreateBasicNamedAndTypedDecl(field_type)) {
      llvm::errs() << "Creation of Type failed\n";
      return false;
    }
    uint64_t field_offset = record_layout.getFieldOffset(field_index);
    recordp->AddRecordField(abi_util::RecordFieldIR(
        field->getName(), QualTypeToString(field_type), field_offset,
        AccessClangToIR(field->getAccess())));
    field++;
    field_index++;
  }
  return true;
}

bool RecordDeclWrapper::SetupCXXBases(
    abi_util::RecordTypeIR *cxxp,
    const clang::CXXRecordDecl *cxx_record_decl) {
  if (!cxx_record_decl || !cxxp) {
    return false;
  }
  clang::CXXRecordDecl::base_class_const_iterator base_class =
      cxx_record_decl->bases_begin();
  while (base_class != cxx_record_decl->bases_end()) {
    std::string name = QualTypeToString(base_class->getType());
    bool is_virtual = base_class->isVirtual();
    abi_util::AccessSpecifierIR access =
        AccessClangToIR(base_class->getAccessSpecifier());
    // TODO: Remove this is not required since a base will always be a Record.
    if (!CreateBasicNamedAndTypedDecl(base_class->getType())) {
      return false;
    }
    cxxp->AddCXXBaseSpecifier(abi_util::CXXBaseSpecifierIR(name, is_virtual,
                                                           access));
    base_class++;
  }
  return true;
}

bool RecordDeclWrapper::SetupRecordVTable(
    abi_util::RecordTypeIR *record_declp,
    const clang::CXXRecordDecl *cxx_record_decl) {
  if (!cxx_record_decl || !record_declp) {
    return false;
  }
  clang::VTableContextBase *base_vtable_contextp =
      ast_contextp_->getVTableContext();
  const clang::Type *typep = cxx_record_decl->getTypeForDecl();
  if (!base_vtable_contextp || !typep) {
    return false;
  }
  // Skip Microsoft ABI.
  clang::ItaniumVTableContext *itanium_vtable_contextp =
        llvm::dyn_cast<clang::ItaniumVTableContext>(base_vtable_contextp);
  if (!itanium_vtable_contextp || !cxx_record_decl->isPolymorphic() ||
      typep->isDependentType() || typep->isIncompleteType()) {
    return true;
  }
  const clang::VTableLayout &vtable_layout =
      itanium_vtable_contextp->getVTableLayout(cxx_record_decl);
  abi_util::VTableLayoutIR vtable_ir_layout;
  for (const auto &vtable_component : vtable_layout.vtable_components()) {
    abi_util::VTableComponentIR added_component=
        SetupRecordVTableComponent(vtable_component);
    vtable_ir_layout.AddVTableComponent(std::move(added_component));
  }
  record_declp->SetVTableLayout(std::move(vtable_ir_layout));
  return true;
}

abi_util::VTableComponentIR RecordDeclWrapper::SetupRecordVTableComponent(
    const clang::VTableComponent &vtable_component) {
  abi_util::VTableComponentIR::Kind kind =
      abi_util::VTableComponentIR::Kind::RTTI;
  std::string mangled_component_name = "";
  llvm::raw_string_ostream ostream(mangled_component_name);
  int64_t value = 0;
  clang::VTableComponent::Kind clang_component_kind =
      vtable_component.getKind();
    switch (clang_component_kind) {
      case clang::VTableComponent::CK_VCallOffset:
        kind =  abi_util::VTableComponentIR::Kind::VCallOffset;
        value = vtable_component.getVCallOffset().getQuantity();
        break;
      case clang::VTableComponent::CK_VBaseOffset:
        kind =  abi_util::VTableComponentIR::Kind::VBaseOffset;
        value = vtable_component.getVBaseOffset().getQuantity();
        break;
      case clang::VTableComponent::CK_OffsetToTop:
        kind =  abi_util::VTableComponentIR::Kind::OffsetToTop;
        value = vtable_component.getOffsetToTop().getQuantity();
        break;
      case clang::VTableComponent::CK_RTTI:
        {
          kind =  abi_util::VTableComponentIR::Kind::RTTI;
          const clang::CXXRecordDecl *rtti_decl =
              vtable_component.getRTTIDecl();
          assert(rtti_decl != nullptr);
          mangled_component_name =
              ABIWrapper::GetTypeLinkageName(rtti_decl->getTypeForDecl());
        }
        break;
      case clang::VTableComponent::CK_FunctionPointer:
      case clang::VTableComponent::CK_CompleteDtorPointer:
      case clang::VTableComponent::CK_DeletingDtorPointer:
      case clang::VTableComponent::CK_UnusedFunctionPointer:
        {
          const clang::CXXMethodDecl *method_decl =
              vtable_component.getFunctionDecl();
          assert(method_decl != nullptr);
          switch (clang_component_kind) {
            case clang::VTableComponent::CK_FunctionPointer:
              kind =  abi_util::VTableComponentIR::Kind::FunctionPointer;
              mangled_component_name = GetMangledNameDecl(method_decl,
                                                          mangle_contextp_);
              break;
            case clang::VTableComponent::CK_CompleteDtorPointer:
              kind =  abi_util::VTableComponentIR::Kind::CompleteDtorPointer;
              mangle_contextp_->mangleCXXDtor(
                  vtable_component.getDestructorDecl(),
                  clang::CXXDtorType::Dtor_Complete, ostream);
              ostream.flush();

              break;
            case clang::VTableComponent::CK_DeletingDtorPointer:
              kind =  abi_util::VTableComponentIR::Kind::DeletingDtorPointer;
              mangle_contextp_->mangleCXXDtor(
                  vtable_component.getDestructorDecl(),
                  clang::CXXDtorType::Dtor_Deleting, ostream);
              ostream.flush();
              break;
            case clang::VTableComponent::CK_UnusedFunctionPointer:
              kind =  abi_util::VTableComponentIR::Kind::UnusedFunctionPointer;
            default:
              break;
          }
        }
        break;
      default:
        // TODO: Fix this
        break;
    }
   return abi_util::VTableComponentIR(mangled_component_name, kind, value);
}

// TODO: Add CreateBasicNamedAndTypedDecl on Template Parameters as well.
bool RecordDeclWrapper::SetupTemplateInfo(
    abi_util::RecordTypeIR *record_declp,
    const clang::CXXRecordDecl *cxx_record_decl) {
  // TODO: Experimental, remove this.
 assert(cxx_record_decl != nullptr);
 const clang::ClassTemplateSpecializationDecl *specialization_decl =
     clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(
         cxx_record_decl);
  if(specialization_decl) {
    const clang::TemplateArgumentList *arg_list =
        &specialization_decl->getTemplateArgs();
    if (arg_list &&
        !SetupTemplateArguments(arg_list, record_declp)) {
      return false;
    }
  }
  return true;
}

bool RecordDeclWrapper::SetupRecordInfo(abi_util::RecordTypeIR *record_declp,
                                        const std::string &source_file) {
  if (!record_declp) {
    return false;
  }
  const clang::Type *basic_type = nullptr;
  if (!(basic_type = record_decl_->getTypeForDecl())) {
    return false;
  }
  clang::QualType qual_type = basic_type->getCanonicalTypeInternal();
  if (!CreateExtendedType(qual_type, record_declp)) {
    return false;
  }
  record_declp->SetSourceFile(source_file);
  record_declp->SetLinkerSetKey((QualTypeToString(qual_type)));
  record_declp->SetAccess(AccessClangToIR(record_decl_->getAccess()));
  return SetupRecordFields(record_declp) &&  SetupCXXRecordInfo(record_declp);
}

bool RecordDeclWrapper::SetupCXXRecordInfo(
    abi_util::RecordTypeIR *record_declp) {
  const clang::CXXRecordDecl *cxx_record_decl =
      clang::dyn_cast<clang::CXXRecordDecl>(record_decl_);
  if (!cxx_record_decl) {
    return true;
  }
  return SetupTemplateInfo(record_declp, cxx_record_decl) &&
      SetupCXXBases(record_declp, cxx_record_decl) &&
      SetupRecordVTable(record_declp, cxx_record_decl);
}

bool RecordDeclWrapper::GetRecordDecl() {
  std::unique_ptr<abi_util::RecordTypeIR> abi_decl(
      new abi_util::RecordTypeIR());
  std::string source_file = GetCachedDeclSourceFile(record_decl_, cip_);
  if (!SetupRecordInfo(abi_decl.get(), source_file)) {
    llvm::errs() << "Setting up CXX Bases / Template Info failed\n";
    return false;
  }
  return ir_dumper_->AddLinkableMessageIR(abi_decl.get());
}

EnumDeclWrapper::EnumDeclWrapper(
    clang::MangleContext *mangle_contextp,
    clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::EnumDecl *decl,
    std::set<std::string> *type_cache,
    abi_util::IRDumper *ir_dumper,
    std::map<const clang::Decl *, std::string> &decl_to_source_cache)
  : ABIWrapper(mangle_contextp, ast_contextp, compiler_instance_p,
               type_cache, ir_dumper, decl_to_source_cache),
    enum_decl_(decl) { }

bool EnumDeclWrapper::SetupEnumFields(abi_util::EnumTypeIR *enump) {
  if (!enump) {
    return false;
  }
  clang::EnumDecl::enumerator_iterator enum_it = enum_decl_->enumerator_begin();
  while (enum_it != enum_decl_->enumerator_end()) {
    std::string name = enum_it->getQualifiedNameAsString();
    uint64_t field_value = enum_it->getInitVal().getExtValue();
    enump->AddEnumField(abi_util::EnumFieldIR(name, field_value));
    enum_it++;
  }
  return true;
}

bool EnumDeclWrapper::SetupEnum(abi_util::EnumTypeIR *enum_type,
                                const std::string &source_file) {
  std::string enum_name = GetTagDeclQualifiedName(enum_decl_);
  // TODO: Change this to TypeForDecl
  clang::QualType enum_qual_type =
      enum_decl_->getTypeForDecl()->getCanonicalTypeInternal();
  if (!CreateExtendedType(enum_qual_type, enum_type)) {
    return false;
  }
  enum_type->SetSourceFile(source_file);
  enum_type->SetUnderlyingType(QualTypeToString(enum_decl_->getIntegerType()));
  enum_type->SetAccess(AccessClangToIR(enum_decl_->getAccess()));
  return SetupEnumFields(enum_type);
}

bool EnumDeclWrapper::GetEnumDecl()  {
  std::unique_ptr<abi_util::EnumTypeIR> abi_decl(new abi_util::EnumTypeIR());
  std::string source_file = GetCachedDeclSourceFile(enum_decl_, cip_);

  if (!SetupEnum(abi_decl.get(), source_file)) {
    llvm::errs() << "Setting up Enum failed\n";
    return false;
  }
  return ir_dumper_->AddLinkableMessageIR(abi_decl.get());
}

GlobalVarDeclWrapper::GlobalVarDeclWrapper(
    clang::MangleContext *mangle_contextp,
    clang::ASTContext *ast_contextp,
    const clang::CompilerInstance *compiler_instance_p,
    const clang::VarDecl *decl,std::set<std::string> *type_cache,
    abi_util::IRDumper *ir_dumper,
    std::map<const clang::Decl *, std::string> &decl_to_source_cache)
  : ABIWrapper(mangle_contextp, ast_contextp, compiler_instance_p, type_cache,
               ir_dumper, decl_to_source_cache),
    global_var_decl_(decl) { }

bool GlobalVarDeclWrapper::SetupGlobalVar(
    abi_util::GlobalVarIR *global_varp,
    const std::string &source_file) {
  // Temporary fix : clang segfaults on trying to mangle global variable which
  // is a dependent sized array type.
  std::string mangled_name =
      GetMangledNameDecl(global_var_decl_, mangle_contextp_);
  if (!CreateBasicNamedAndTypedDecl(global_var_decl_->getType())) {
    return false;
  }
  global_varp->SetSourceFile(source_file);
  global_varp->SetName(global_var_decl_->getQualifiedNameAsString());
  global_varp->SetLinkerSetKey(mangled_name);
  global_varp->SetReferencedType(
      QualTypeToString(global_var_decl_->getType()));
  return true;
}

bool GlobalVarDeclWrapper::GetGlobalVarDecl() {
  std::unique_ptr<abi_util::GlobalVarIR>
      abi_decl(new abi_util::GlobalVarIR);
  std::string source_file = GetCachedDeclSourceFile(global_var_decl_, cip_);
  return SetupGlobalVar(abi_decl.get(), source_file) &&
      ir_dumper_->AddLinkableMessageIR(abi_decl.get());
}

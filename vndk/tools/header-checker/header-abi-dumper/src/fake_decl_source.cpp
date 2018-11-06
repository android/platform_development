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

#include "fake_decl_source.h"

#include <clang/Sema/Lookup.h>

FakeDeclSource::FakeDeclSource(const clang::CompilerInstance &ci) : ci_(ci) {}

clang::RecordDecl *
FakeDeclSource::CreateRecordDecl(const clang::DeclarationName &name,
                                 clang::DeclContext *decl_context) {
  clang::ASTContext &ast = ci_.getASTContext();
  clang::RecordDecl *prev_decl = nullptr;

  clang::RecordDecl *record_decl = clang::RecordDecl::Create(
      ast, clang::TTK_Struct, decl_context, clang::SourceLocation(),
      clang::SourceLocation(), name.getAsIdentifierInfo(), prev_decl);
  record_decl->setInvalidDecl(true);

  std::string name_string = name.getAsString();
  decl_context->addDecl(record_decl);
  return record_decl;
}

clang::ClassTemplateDecl *
FakeDeclSource::CreateClassTemplateDecl(clang::NamedDecl *record_decl,
                                        clang::DeclContext *decl_context) {
  clang::ASTContext &ast = ci_.getASTContext();

  std::vector<clang::NamedDecl *> param_array;
  clang::TemplateParameterList *param_list =
      clang::TemplateParameterList::Create(
          ast, clang::SourceLocation(), clang::SourceLocation(), param_array,
          clang::SourceLocation(), /* AssociatedConstraints */ nullptr);

  clang::ClassTemplateDecl *class_template_decl =
      clang::ClassTemplateDecl::Create(ast, decl_context,
                                       clang::SourceLocation(),
                                       record_decl->getDeclName(), param_list,
                                       (record_decl ? nullptr : record_decl),
                                       /* AssociatedConstraints */ nullptr);
  class_template_decl->setInvalidDecl(true);
  return class_template_decl;
}

clang::NamespaceDecl *
FakeDeclSource::CreateNamespaceDecl(const clang::DeclarationName &name,
                                    clang::DeclContext *decl_context) {
  clang::ASTContext &ast = ci_.getASTContext();
  clang::NamespaceDecl *prev_decl = nullptr;

  clang::NamespaceDecl *namespace_decl = clang::NamespaceDecl::Create(
      ast, decl_context, /* Inline */ false, clang::SourceLocation(),
      clang::SourceLocation(), name.getAsIdentifierInfo(), prev_decl);
  namespace_decl->setInvalidDecl(true);

  std::string name_string = name.getAsString();
  decl_context->addDecl(namespace_decl);
  return namespace_decl;
}

clang::NamedDecl *FakeDeclSource::CreateDecl(clang::Sema::LookupNameKind kind,
                                             const clang::DeclarationName &name,
                                             clang::DeclContext *decl_context) {
  if (name.getNameKind() != clang::DeclarationName::Identifier) {
    return nullptr;
  }
  if (kind == clang::Sema::LookupTagName ||
      kind == clang::Sema::LookupOrdinaryName) {
    clang::RecordDecl *record_decl = CreateRecordDecl(name, decl_context);
    return CreateClassTemplateDecl(record_decl, decl_context);
  }

  if (kind == clang::Sema::LookupNestedNameSpecifierName) {
    return CreateNamespaceDecl(name, decl_context);
  }

  return nullptr;
}

clang::TypoCorrection FakeDeclSource::CorrectTypo(
    const clang::DeclarationNameInfo &typo, int lookup_kind,
    clang::Scope *scope, clang::CXXScopeSpec *scope_spec,
    clang::CorrectionCandidateCallback &ccc, clang::DeclContext *member_context,
    bool entering_context, const clang::ObjCObjectPointerType *opt) {
  clang::DeclarationName name = typo.getName();
  const std::string lookup_name_string = name.getAsString();

  clang::NestedNameSpecifier *nns = nullptr;
  if (scope_spec != nullptr && !scope_spec->isEmpty()) {
    nns = scope_spec->getScopeRep();
  }

  clang::ASTContext &ast = ci_.getASTContext();
  clang::DeclContext *decl_context = ast.getTranslationUnitDecl();
  if (member_context != nullptr) {
    decl_context = member_context;
  } else if (nns != nullptr && nns->getAsNamespace() != nullptr) {
    decl_context = nns->getAsNamespace();
  }

  clang::NamedDecl *decl =
      CreateDecl(clang::Sema::LookupNameKind(lookup_kind), name, decl_context);
  if (decl != nullptr) {
    return clang::TypoCorrection(decl, nns);
  }
  return clang::TypoCorrection();
}

bool FakeDeclSource::LookupUnqualified(clang::LookupResult &result,
                                       clang::Scope *scope) {
  if (result.isForRedeclaration()) {
    return false;
  }

  clang::Sema::LookupNameKind kind = result.getLookupKind();
  clang::DeclarationName name = result.getLookupName();
  clang::ASTContext &ast = ci_.getASTContext();
  clang::DeclContext *decl_context = ast.getTranslationUnitDecl();

  clang::NamedDecl *decl = CreateDecl(kind, name, decl_context);
  if (decl == nullptr) {
    return false;
  }

  result.addDecl(decl);
  result.resolveKind();
  return true;
}

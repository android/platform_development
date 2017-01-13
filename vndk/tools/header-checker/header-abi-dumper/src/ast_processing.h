#include "ABI.pb.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Mangle.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

using namespace headerchecker::headerabidumper;

class HeaderASTVisitor
    : public clang::RecursiveASTVisitor<HeaderASTVisitor> {
 public:
  HeaderASTVisitor(
      src::TranslationUnit *TUPtr,
      clang::MangleContext *mangle_contextp,
      const clang::ASTContext *AST_contextp,
      const clang::CompilerInstance *compiler_instance_p);
  bool VisitRecordDecl(const clang::RecordDecl *decl);
  bool VisitFunctionDecl(const clang::FunctionDecl *decl);
 private:
  src::TranslationUnit *TUp_;
  bool SetupFunction(
      src::FunctionDecl *methodp,
      const clang::FunctionDecl *decl);
  bool SetupClassFields(
      src::RecordDecl *classp,
      const clang::RecordDecl *decl);
  std::string GetDeclSourceFile(const clang::NamedDecl *decl);
  std::string GetMangledNameDecl(const clang::NamedDecl *decl);
  clang::MangleContext *mangle_contextp_;
  const clang::ASTContext *AST_contextp_;
  const clang::CompilerInstance *cip_;
};

class HeaderASTConsumer : public clang::ASTConsumer {
 public:
  HeaderASTConsumer(
      std::string file_name,
      clang::CompilerInstance *compiler_instancep,
      std::string out_dump_name);
  void HandleTranslationUnit(clang::ASTContext &ctx) override;
  void HandleVTable(clang::CXXRecordDecl *crd) override;
 private:
  std::string file_name_;
  clang::CompilerInstance *cip_;
  std::string out_dump_name_;
};

class HeaderASTPPCallbacks : public clang::PPCallbacks {
 private:
   llvm::StringRef ToString(const clang::Token &tok);

 public:
  void MacroDefined(const clang::Token &macro_name_tok,
                    const clang::MacroDirective *) override;
 };

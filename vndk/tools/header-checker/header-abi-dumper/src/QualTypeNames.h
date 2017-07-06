//===------- QualTypeNames.cpp - Generate Complete QualType Names ---------===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


// This is a replica of
// external/clang/include/clang/Tooling/Core/QualTypeNames.h since there are
// some static methods needed by getFullyQualifiedTypeName not exported by the
// library in order to change the printing policy.

#ifndef _QUALTYPENAMES_H
#define _QUALTYPENAMES_H

#include "clang/AST/ASTContext.h"

namespace clang {
namespace TypeName {
/// \brief Get the fully qualified name for a type. This includes full
/// qualification of all template parameters etc.
///
/// \param[in] QT - the type for which the fully qualified name will be
/// returned.
/// \param[in] Ctx - the ASTContext to be used.
/// \param[in] WithGlobalNsPrefix - If true, then the global namespace
/// specifier "::" will be prepended to the fully qualified name.
std::string getFullyQualifiedTypeName(QualType QT,
                                      const ASTContext &Ctx,
                                      bool WithGlobalNsPrefix = false);
}  // end namespace TypeName
}  // end namespace clang
#endif  // LLVM_CLANG_TOOLING_CORE_QUALTYPENAMES_H

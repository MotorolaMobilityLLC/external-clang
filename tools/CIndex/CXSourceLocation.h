//===- CXSourceLocation.h - CXSourceLocations Utilities ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines routines for manipulating CXSourceLocations.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_CXSOURCELOCATION_H
#define LLVM_CLANG_CXSOURCELOCATION_H

#include "clang-c/Index.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/AST/ASTContext.h"

namespace clang {
      
class ASTContext;

namespace cxloc {

/// \brief Translate a Clang source location into a CIndex source location.
static inline CXSourceLocation 
translateSourceLocation(const SourceManager &SM, const LangOptions &LangOpts,
                        SourceLocation Loc) {
  CXSourceLocation Result = { { (void*) &SM, (void*) &LangOpts, },
                              Loc.getRawEncoding() };
  return Result;
}
  
/// \brief Translate a Clang source location into a CIndex source location.
static inline CXSourceLocation translateSourceLocation(ASTContext &Context,
                                                       SourceLocation Loc) {
  return translateSourceLocation(Context.getSourceManager(),
                                 Context.getLangOptions(),
                                 Loc);
}

/// \brief Translate a Clang source range into a CIndex source range.
///
/// Clang internally represents ranges where the end location points to the
/// start of the token at the end. However, for external clients it is more
/// useful to have a CXSourceRange be a proper half-open interval. This routine
/// does the appropriate translation.
CXSourceRange translateSourceRange(const SourceManager &SM, 
                                   const LangOptions &LangOpts,
                                   SourceRange R);
  
/// \brief Translate a Clang source range into a CIndex source range.
static inline CXSourceRange translateSourceRange(ASTContext &Context,
                                                 SourceRange R) {
  return translateSourceRange(Context.getSourceManager(),
                              Context.getLangOptions(),
                              R);
}

static inline SourceLocation translateSourceLocation(CXSourceLocation L) {
  return SourceLocation::getFromRawEncoding(L.int_data);
}

static inline SourceRange translateSourceRange(CXSourceRange R) {
  return SourceRange(SourceLocation::getFromRawEncoding(R.begin_int_data),
                     SourceLocation::getFromRawEncoding(R.end_int_data));
}


}} // end namespace: clang::cxloc

#endif

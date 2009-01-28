//===--- DiagnosticSema.h - Diagnostics for libsema -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_DIAGNOSTICSEMA_H
#define LLVM_CLANG_DIAGNOSTICSEMA_H

#include "clang/Basic/Diagnostic.h"

namespace clang {
  namespace diag { 
    enum {
#define DIAG(ENUM,FLAGS,DESC) ENUM,
#include "DiagnosticCommonKinds.def"
#define SEMASTART
#include "DiagnosticSemaKinds.def"
      NUM_BUILTIN_SEMA_DIAGNOSTICS
    };
  }  // end namespace diag
}  // end namespace clang

#endif

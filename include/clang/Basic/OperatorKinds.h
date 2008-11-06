//===--- OperatorKinds.h - C++ Overloaded Operators -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines C++ overloaded operators.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_OPERATOR_KINDS_H
#define LLVM_CLANG_BASIC_OPERATOR_KINDS_H

namespace clang {

/// OverloadedOperatorKind - Enumeration specifying the different kinds of
/// C++ overloaded operators.
enum OverloadedOperatorKind {
  OO_None,                //< Not an overloaded operator
#define OVERLOADED_OPERATOR(Name,Spelling,Token) OO_##Name,
#include "clang/Basic/OperatorKinds.def"
  NUM_OVERLOADED_OPERATORS
};


} // end namespace clang

#endif

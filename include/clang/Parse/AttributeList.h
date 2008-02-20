//===--- AttributeList.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AttributeList class interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ATTRLIST_H
#define LLVM_CLANG_ATTRLIST_H

#include "clang/Parse/Action.h"
#include <cassert>

namespace clang {
  
/// AttributeList - Represents GCC's __attribute__ declaration. There are
/// 4 forms of this construct...they are:
///
/// 1: __attribute__(( const )). ParmName/Args/NumArgs will all be unused.
/// 2: __attribute__(( mode(byte) )). ParmName used, Args/NumArgs unused.
/// 3: __attribute__(( format(printf, 1, 2) )). ParmName/Args/NumArgs all used.
/// 4: __attribute__(( aligned(16) )). ParmName is unused, Args/Num used.
///
class AttributeList {
  IdentifierInfo *AttrName;
  SourceLocation AttrLoc;
  IdentifierInfo *ParmName;
  SourceLocation ParmLoc;
  Action::ExprTy **Args;
  unsigned NumArgs;
  AttributeList *Next;
public:
  AttributeList(IdentifierInfo *AttrName, SourceLocation AttrLoc,
                IdentifierInfo *ParmName, SourceLocation ParmLoc,
                Action::ExprTy **args, unsigned numargs, AttributeList *Next);
  ~AttributeList();
  
  enum Kind {
    UnknownAttribute,
    AT_vector_size,
    AT_ocu_vector_type,
    AT_address_space,
    AT_aligned,
    AT_packed
  };
  
  IdentifierInfo *getName() const { return AttrName; }
  SourceLocation getLoc() const { return AttrLoc; }
  IdentifierInfo *getParameterName() const { return ParmName; }
  
  Kind getKind() const { return getKind(getName()); }
  static Kind getKind(const IdentifierInfo *Name);
  
  AttributeList *getNext() const { return Next; }
  void setNext(AttributeList *N) { Next = N; }
  
  void addAttributeList(AttributeList *alist) {
    assert((alist != 0) && "addAttributeList(): alist is null");
    AttributeList *next = this, *prev;
    do {
      prev = next;
      next = next->getNext();
    } while (next);
    prev->setNext(alist);
  }

  /// getNumArgs - Return the number of actual arguments to this attribute.
  unsigned getNumArgs() const { return NumArgs; }
  
  /// getArg - Return the specified argument.
  Action::ExprTy *getArg(unsigned Arg) const {
    assert(Arg < NumArgs && "Arg access out of range!");
    return Args[Arg];
  }
};

}  // end namespace clang

#endif

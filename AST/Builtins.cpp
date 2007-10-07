//===--- Builtins.cpp - Builtin function implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Chris Lattner and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements various things for builtin functions.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Builtins.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/TargetInfo.h"
using namespace clang;

static const Builtin::Info BuiltinInfo[] = {
  { "not a builtin function", 0, 0 },
#define BUILTIN(ID, TYPE, ATTRS) { #ID, TYPE, ATTRS },
#include "clang/AST/Builtins.def"
};

const Builtin::Info &Builtin::Context::GetRecord(unsigned ID) const {
  if (ID < Builtin::FirstTSBuiltin)
    return BuiltinInfo[ID];
  assert(ID - Builtin::FirstTSBuiltin < NumTSRecords && "Invalid builtin ID!");
  return TSRecords[ID - Builtin::FirstTSBuiltin];
}


/// InitializeBuiltins - Mark the identifiers for all the builtins with their
/// appropriate builtin ID # and mark any non-portable builtin identifiers as
/// such.
void Builtin::Context::InitializeBuiltins(IdentifierTable &Table,
                                          const TargetInfo &Target) {
  // Step #1: mark all target-independent builtins with their ID's.
  for (unsigned i = Builtin::NotBuiltin+1; i != Builtin::FirstTSBuiltin; ++i)
    Table.get(BuiltinInfo[i].Name).setBuiltinID(i);
  
  // Step #2: handle target builtins.
  std::vector<const char *> NonPortableBuiltins;
  Target.getTargetBuiltins(TSRecords, NumTSRecords, NonPortableBuiltins);

  // Step #2a: Register target-specific builtins.
  for (unsigned i = 0, e = NumTSRecords; i != e; ++i)
    Table.get(TSRecords[i].Name).setBuiltinID(i+Builtin::FirstTSBuiltin);
  
  // Step #2b: Mark non-portable builtins as such.
  for (unsigned i = 0, e = NonPortableBuiltins.size(); i != e; ++i)
    Table.get(NonPortableBuiltins[i]).setNonPortableBuiltin(true);
}

/// DecodeTypeFromStr - This decodes one type descriptor from Str, advancing the
/// pointer over the consumed characters.  This returns the resultant type.
static QualType DecodeTypeFromStr(const char *&Str, ASTContext &Context) {
  // Modifiers.
  bool Long = false, LongLong = false, Signed = false, Unsigned = false;
  
  // Read the modifiers first.
  bool Done = false;
  while (!Done) {
    switch (*Str++) {
    default: Done = true; --Str; break; 
    case 'S':
      assert(!Unsigned && "Can't use both 'S' and 'U' modifiers!");
      assert(!Signed && "Can't use 'S' modifier multiple times!");
      Signed = true;
      break;
    case 'U':
      assert(!Signed && "Can't use both 'S' and 'U' modifiers!");
      assert(!Unsigned && "Can't use 'S' modifier multiple times!");
      Unsigned = true;
      break;
    case 'L':
      assert(!LongLong && "Can't have LLL modifier");
      if (Long) 
        LongLong = true;
      else
        Long = true;
      break;
    }
  }

  QualType Type;
  
  // Read the base type.
  switch (*Str++) {
  default: assert(0 && "Unknown builtin type letter!");
  case 'v':
    assert(!Long && !Signed && !Unsigned && "Bad modifiers used with 'v'!");
    Type = Context.VoidTy;
    break;
  case 'f':
    assert(!Long && !Signed && !Unsigned && "Bad modifiers used with 'f'!");
    Type = Context.FloatTy;
    break;
  case 'd':
    assert(!LongLong && !Signed && !Unsigned && "Bad modifiers used with 'd'!");
    if (Long)
      Type = Context.LongDoubleTy;
    else
      Type = Context.DoubleTy;
    break;
  case 's':
    assert(!LongLong && "Bad modifiers used with 's'!");
    if (Unsigned)
      Type = Context.UnsignedShortTy;
    else
      Type = Context.ShortTy;
      break;
  case 'i':
    if (Long)
      Type = Unsigned ? Context.UnsignedLongTy : Context.LongTy;
    else if (LongLong)
      Type = Unsigned ? Context.UnsignedLongLongTy : Context.LongLongTy;
    else if (Unsigned)
      Type = Context.UnsignedIntTy;
    else 
      Type = Context.IntTy; // default is signed.
    break;
  case 'c':
    assert(!Long && !LongLong && "Bad modifiers used with 'c'!");
    if (Signed)
      Type = Context.SignedCharTy;
    else if (Unsigned)
      Type = Context.UnsignedCharTy;
    else
      Type = Context.CharTy;
    break;
  case 'F':
    Type = Context.getCFConstantStringType();
    break;
  }
  
  Done = false;
  while (!Done) {
    switch (*Str++) {
      default: Done = true; --Str; break; 
      case '*':
        Type = Context.getPointerType(Type);
        break;
      case 'C':
        Type = Type.getQualifiedType(QualType::Const);
        break;
    }
  }
  
  return Type;
}

/// GetBuiltinType - Return the type for the specified builtin.
QualType Builtin::Context::GetBuiltinType(unsigned id, ASTContext &Context)const{
  const char *TypeStr = GetRecord(id).Type;
  
  llvm::SmallVector<QualType, 8> ArgTypes;
  
  QualType ResType = DecodeTypeFromStr(TypeStr, Context);
  while (TypeStr[0] && TypeStr[0] != '.')
    ArgTypes.push_back(DecodeTypeFromStr(TypeStr, Context));
  
  assert((TypeStr[0] != '.' || TypeStr[1] == 0) &&
         "'.' should only occur at end of builtin type list!");

  // handle untyped/variadic arguments "T c99Style();" or "T cppStyle(...);".
  if (ArgTypes.size() == 0 && TypeStr[0] == '.')
    return Context.getFunctionTypeNoProto(ResType);
  return Context.getFunctionType(ResType, &ArgTypes[0], ArgTypes.size(),
                                 TypeStr[0] == '.');
}

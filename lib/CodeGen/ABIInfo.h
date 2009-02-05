//===----- ABIInfo.h - ABI information access & encapsulation ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_CODEGEN_ABIINFO_H
#define CLANG_CODEGEN_ABIINFO_H

namespace llvm {
  class Type;
}

namespace clang {
  class ASTContext;

  // FIXME: This is a layering issue if we want to move ABIInfo
  // down. Fortunately CGFunctionInfo has no real tie to CodeGen.
  namespace CodeGen {
    class CGFunctionInfo;
  }

  /* FIXME: All of this stuff should be part of the target interface
     somehow. It is currently here because it is not clear how to factor
     the targets to support this, since the Targets currently live in a
     layer below types n'stuff.
  */

  /// ABIArgInfo - Helper class to encapsulate information about how a
  /// specific C type should be passed to or returned from a function.
  class ABIArgInfo {
  public:
    enum Kind {
      Direct,    /// Pass the argument directly using the normal
                 /// converted LLVM type. Complex and structure types
                 /// are passed using first class aggregates.
  
      Indirect,  /// Pass the argument indirectly via a hidden pointer
                 /// with the specified alignment (0 indicates default
                 /// alignment).
  
      Ignore,    /// Ignore the argument (treat as void). Useful for
                 /// void and empty structs.
  
      Coerce,    /// Only valid for aggregate return types, the argument
                 /// should be accessed by coercion to a provided type.
  
      Expand,    /// Only valid for aggregate argument types. The
                 /// structure should be expanded into consecutive
                 /// arguments for its constituent fields. Currently
                 /// expand is only allowed on structures whose fields
                 /// are all scalar types or are themselves expandable
                 /// types.
  
      KindFirst=Direct, KindLast=Expand
    };
  
  private:
    Kind TheKind;
    const llvm::Type *TypeData;
    unsigned UIntData;
  
    ABIArgInfo(Kind K, const llvm::Type *TD=0,
               unsigned UI=0) : TheKind(K),
                                TypeData(TD),
                                UIntData(UI) {}
  public:
    ABIArgInfo() : TheKind(Direct), TypeData(0), UIntData(0) {}

    static ABIArgInfo getDirect() { 
      return ABIArgInfo(Direct); 
    }
    static ABIArgInfo getIgnore() {
      return ABIArgInfo(Ignore);
    }
    static ABIArgInfo getCoerce(const llvm::Type *T) { 
      return ABIArgInfo(Coerce, T);
    }
    static ABIArgInfo getIndirect(unsigned Alignment) {
      return ABIArgInfo(Indirect, 0, Alignment);
    }
    static ABIArgInfo getExpand() {
      return ABIArgInfo(Expand);
    }
  
    Kind getKind() const { return TheKind; }
    bool isDirect() const { return TheKind == Direct; }
    bool isIgnore() const { return TheKind == Ignore; }
    bool isCoerce() const { return TheKind == Coerce; }
    bool isIndirect() const { return TheKind == Indirect; }
    bool isExpand() const { return TheKind == Expand; }
  
    // Coerce accessors
    const llvm::Type *getCoerceToType() const {
      assert(TheKind == Coerce && "Invalid kind!");
      return TypeData;
    }
  
    // ByVal accessors
    unsigned getIndirectAlign() const {
      assert(TheKind == Indirect && "Invalid kind!");
      return UIntData;
    }

    void dump() const;
  };

  /// ABIInfo - Target specific hooks for defining how a type should be
  /// passed or returned from functions.
  class ABIInfo {
  public:
    virtual ~ABIInfo();

    virtual void computeInfo(CodeGen::CGFunctionInfo &FI,
                             ASTContext &Ctx) const = 0;
  };
}  // end namespace clang

#endif

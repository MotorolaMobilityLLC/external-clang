//===--- Type.h - C Language Family Type Representation ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Type interface and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_TYPE_H
#define LLVM_CLANG_AST_TYPE_H

#include "clang/Basic/Diagnostic.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Bitcode/SerializationFwd.h"
using llvm::isa;
using llvm::cast;
using llvm::cast_or_null;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;

namespace clang {
  class ASTContext;
  class Type;
  class TypedefDecl;
  class TemplateDecl;
  class TemplateTypeParmDecl;
  class NonTypeTemplateParmDecl;
  class TemplateTemplateParamDecl;
  class TagDecl;
  class RecordDecl;
  class CXXRecordDecl;
  class EnumDecl;
  class FieldDecl;
  class ObjCInterfaceDecl;
  class ObjCProtocolDecl;
  class ObjCMethodDecl;
  class Expr;
  class Stmt;
  class SourceLocation;
  class StmtIteratorBase;
  class TemplateArgument;
  class QualifiedNameType;

  // Provide forward declarations for all of the *Type classes
#define TYPE(Class, Base) class Class##Type;
#include "clang/AST/TypeNodes.def"

/// QualType - For efficiency, we don't store CVR-qualified types as nodes on
/// their own: instead each reference to a type stores the qualifiers.  This
/// greatly reduces the number of nodes we need to allocate for types (for
/// example we only need one for 'int', 'const int', 'volatile int',
/// 'const volatile int', etc).
///
/// As an added efficiency bonus, instead of making this a pair, we just store
/// the three bits we care about in the low bits of the pointer.  To handle the
/// packing/unpacking, we make QualType be a simple wrapper class that acts like
/// a smart pointer.
class QualType {
  llvm::PointerIntPair<Type*, 3> Value;
public:
  enum TQ {   // NOTE: These flags must be kept in sync with DeclSpec::TQ.
    Const    = 0x1,
    Restrict = 0x2,
    Volatile = 0x4,
    CVRFlags = Const|Restrict|Volatile
  };
  
  enum GCAttrTypes {
    GCNone = 0,
    Weak,
    Strong
  };
  
  QualType() {}
  
  QualType(const Type *Ptr, unsigned Quals)
    : Value(const_cast<Type*>(Ptr), Quals) {}

  unsigned getCVRQualifiers() const { return Value.getInt(); }
  void setCVRQualifiers(unsigned Quals) { Value.setInt(Quals); }
  Type *getTypePtr() const { return Value.getPointer(); }
  
  void *getAsOpaquePtr() const { return Value.getOpaqueValue(); }
  static QualType getFromOpaquePtr(void *Ptr) {
    QualType T;
    T.Value.setFromOpaqueValue(Ptr);
    return T;
  }
  
  Type &operator*() const {
    return *getTypePtr();
  }

  Type *operator->() const {
    return getTypePtr();
  }
  
  /// isNull - Return true if this QualType doesn't point to a type yet.
  bool isNull() const {
    return getTypePtr() == 0;
  }

  bool isConstQualified() const {
    return (getCVRQualifiers() & Const) ? true : false;
  }
  bool isVolatileQualified() const {
    return (getCVRQualifiers() & Volatile) ? true : false;
  }
  bool isRestrictQualified() const {
    return (getCVRQualifiers() & Restrict) ? true : false;
  }

  bool isConstant(ASTContext& Ctx) const;
  
  /// addConst/addVolatile/addRestrict - add the specified type qual to this
  /// QualType.
  void addConst()    { Value.setInt(Value.getInt() | Const); }
  void addVolatile() { Value.setInt(Value.getInt() | Volatile); }
  void addRestrict() { Value.setInt(Value.getInt() | Restrict); }

  void removeConst()    { Value.setInt(Value.getInt() & ~Const); }
  void removeVolatile() { Value.setInt(Value.getInt() & ~Volatile); }
  void removeRestrict() { Value.setInt(Value.getInt() & ~Restrict); }

  QualType getQualifiedType(unsigned TQs) const {
    return QualType(getTypePtr(), TQs);
  }
  QualType getWithAdditionalQualifiers(unsigned TQs) const {
    return QualType(getTypePtr(), TQs|getCVRQualifiers());
  }

  QualType withConst() const { return getWithAdditionalQualifiers(Const); }
  QualType withVolatile() const { return getWithAdditionalQualifiers(Volatile);}
  QualType withRestrict() const { return getWithAdditionalQualifiers(Restrict);}
  
  QualType getUnqualifiedType() const;
  bool isMoreQualifiedThan(QualType Other) const;
  bool isAtLeastAsQualifiedAs(QualType Other) const;
  QualType getNonReferenceType() const;
  
  /// getDesugaredType - Return the specified type with any "sugar" removed from
  /// the type.  This takes off typedefs, typeof's etc.  If the outer level of
  /// the type is already concrete, it returns it unmodified.  This is similar
  /// to getting the canonical type, but it doesn't remove *all* typedefs.  For
  /// example, it returns "T*" as "T*", (not as "int*"), because the pointer is
  /// concrete.
  QualType getDesugaredType() const;

  /// operator==/!= - Indicate whether the specified types and qualifiers are
  /// identical.
  bool operator==(const QualType &RHS) const {
    return Value == RHS.Value;
  }
  bool operator!=(const QualType &RHS) const {
    return Value != RHS.Value;
  }
  std::string getAsString() const {
    std::string S;
    getAsStringInternal(S);
    return S;
  }
  void getAsStringInternal(std::string &Str) const;
  
  void dump(const char *s) const;
  void dump() const;
  
  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddPointer(getAsOpaquePtr());
  }

public:
  
  /// getAddressSpace - Return the address space of this type.
  inline unsigned getAddressSpace() const;
  
  /// GCAttrTypesAttr - Returns gc attribute of this type.
  inline QualType::GCAttrTypes getObjCGCAttr() const;

  /// isObjCGCWeak true when Type is objc's weak.
  bool isObjCGCWeak() const {
    return getObjCGCAttr() == Weak;
  }

  /// isObjCGCStrong true when Type is objc's strong.
  bool isObjCGCStrong() const {
    return getObjCGCAttr() == Strong;
  }
  
  /// Emit - Serialize a QualType to Bitcode.
  void Emit(llvm::Serializer& S) const;
  
  /// Read - Deserialize a QualType from Bitcode.
  static QualType ReadVal(llvm::Deserializer& D);
  
  void ReadBackpatch(llvm::Deserializer& D);
};

} // end clang.

namespace llvm {
/// Implement simplify_type for QualType, so that we can dyn_cast from QualType
/// to a specific Type class.
template<> struct simplify_type<const ::clang::QualType> {
  typedef ::clang::Type* SimpleType;
  static SimpleType getSimplifiedValue(const ::clang::QualType &Val) {
    return Val.getTypePtr();
  }
};
template<> struct simplify_type< ::clang::QualType>
  : public simplify_type<const ::clang::QualType> {};
  
} // end namespace llvm

namespace clang {

/// Type - This is the base class of the type hierarchy.  A central concept
/// with types is that each type always has a canonical type.  A canonical type
/// is the type with any typedef names stripped out of it or the types it
/// references.  For example, consider:
///
///  typedef int  foo;
///  typedef foo* bar;
///    'int *'    'foo *'    'bar'
///
/// There will be a Type object created for 'int'.  Since int is canonical, its
/// canonicaltype pointer points to itself.  There is also a Type for 'foo' (a
/// TypedefType).  Its CanonicalType pointer points to the 'int' Type.  Next
/// there is a PointerType that represents 'int*', which, like 'int', is
/// canonical.  Finally, there is a PointerType type for 'foo*' whose canonical
/// type is 'int*', and there is a TypedefType for 'bar', whose canonical type
/// is also 'int*'.
///
/// Non-canonical types are useful for emitting diagnostics, without losing
/// information about typedefs being used.  Canonical types are useful for type
/// comparisons (they allow by-pointer equality tests) and useful for reasoning
/// about whether something has a particular form (e.g. is a function type),
/// because they implicitly, recursively, strip all typedefs out of a type.
///
/// Types, once created, are immutable.
///
class Type {
public:
  enum TypeClass {
#define TYPE(Class, Base) Class,
#define ABSTRACT_TYPE(Class, Base)
#include "clang/AST/TypeNodes.def"
    TagFirst = Record, TagLast = Enum
  };
  
private:
  QualType CanonicalType;

  /// Dependent - Whether this type is a dependent type (C++ [temp.dep.type]).
  bool Dependent : 1;

  /// TypeClass bitfield - Enum that specifies what subclass this belongs to.
  /// Note that this should stay at the end of the ivars for Type so that
  /// subclasses can pack their bitfields into the same word.
  unsigned TC : 5;

protected:
  // silence VC++ warning C4355: 'this' : used in base member initializer list
  Type *this_() { return this; }
  Type(TypeClass tc, QualType Canonical, bool dependent)
    : CanonicalType(Canonical.isNull() ? QualType(this_(), 0) : Canonical),
      Dependent(dependent), TC(tc) {}
  virtual ~Type() {}
  virtual void Destroy(ASTContext& C);
  friend class ASTContext;
  
  void EmitTypeInternal(llvm::Serializer& S) const;
  void ReadTypeInternal(llvm::Deserializer& D);
  
public:
  TypeClass getTypeClass() const { return static_cast<TypeClass>(TC); }
  
  bool isCanonical() const { return CanonicalType.getTypePtr() == this; }

  /// Types are partitioned into 3 broad categories (C99 6.2.5p1): 
  /// object types, function types, and incomplete types.
  
  /// isObjectType - types that fully describe objects. An object is a region
  /// of memory that can be examined and stored into (H&S).
  bool isObjectType() const;

  /// isIncompleteType - Return true if this is an incomplete type.
  /// A type that can describe objects, but which lacks information needed to
  /// determine its size (e.g. void, or a fwd declared struct). Clients of this
  /// routine will need to determine if the size is actually required.  
  bool isIncompleteType() const;

  /// isIncompleteOrObjectType - Return true if this is an incomplete or object
  /// type, in other words, not a function type.
  bool isIncompleteOrObjectType() const {
    return !isFunctionType();
  }

  /// isPODType - Return true if this is a plain-old-data type (C++ 3.9p10).
  bool isPODType() const;

  /// isVariablyModifiedType (C99 6.7.5.2p2) - Return true for variable array
  /// types that have a non-constant expression. This does not include "[]".
  bool isVariablyModifiedType() const;
  
  /// Helper methods to distinguish type categories. All type predicates
  /// operate on the canonical type, ignoring typedefs and qualifiers.

  /// isSpecificBuiltinType - Test for a particular builtin type.
  bool isSpecificBuiltinType(unsigned K) const;
  
  /// isIntegerType() does *not* include complex integers (a GCC extension).
  /// isComplexIntegerType() can be used to test for complex integers.
  bool isIntegerType() const;     // C99 6.2.5p17 (int, char, bool, enum)
  bool isEnumeralType() const;
  bool isBooleanType() const;
  bool isCharType() const;
  bool isWideCharType() const;
  bool isIntegralType() const;
  
  /// Floating point categories.
  bool isRealFloatingType() const; // C99 6.2.5p10 (float, double, long double)
  /// isComplexType() does *not* include complex integers (a GCC extension).
  /// isComplexIntegerType() can be used to test for complex integers.
  bool isComplexType() const;      // C99 6.2.5p11 (complex)
  bool isAnyComplexType() const;   // C99 6.2.5p11 (complex) + Complex Int.
  bool isFloatingType() const;     // C99 6.2.5p11 (real floating + complex)
  bool isRealType() const;         // C99 6.2.5p17 (real floating + integer)
  bool isArithmeticType() const;   // C99 6.2.5p18 (integer + floating)
  bool isVoidType() const;         // C99 6.2.5p19
  bool isDerivedType() const;      // C99 6.2.5p20
  bool isScalarType() const;       // C99 6.2.5p21 (arithmetic + pointers)
  bool isAggregateType() const;
  
  // Type Predicates: Check to see if this type is structurally the specified
  // type, ignoring typedefs and qualifiers.
  bool isFunctionType() const;
  bool isPointerType() const;
  bool isBlockPointerType() const;
  bool isReferenceType() const;
  bool isLValueReferenceType() const;
  bool isRValueReferenceType() const;
  bool isFunctionPointerType() const;
  bool isMemberPointerType() const;
  bool isMemberFunctionPointerType() const;
  bool isArrayType() const;
  bool isConstantArrayType() const;
  bool isIncompleteArrayType() const;
  bool isVariableArrayType() const;
  bool isDependentSizedArrayType() const;
  bool isRecordType() const;
  bool isClassType() const;   
  bool isStructureType() const;   
  bool isUnionType() const;
  bool isComplexIntegerType() const;            // GCC _Complex integer type.
  bool isVectorType() const;                    // GCC vector type.
  bool isExtVectorType() const;                 // Extended vector type.
  bool isObjCInterfaceType() const;             // NSString or NSString<foo>
  bool isObjCQualifiedInterfaceType() const;    // NSString<foo>
  bool isObjCQualifiedIdType() const;           // id<foo>
  bool isTemplateTypeParmType() const;          // C++ template type parameter

  /// isDependentType - Whether this type is a dependent type, meaning
  /// that its definition somehow depends on a template parameter 
  /// (C++ [temp.dep.type]).
  bool isDependentType() const { return Dependent; }
  bool isOverloadableType() const;

  /// hasPointerRepresentation - Whether this type is represented
  /// natively as a pointer; this includes pointers, references, block
  /// pointers, and Objective-C interface, qualified id, and qualified
  /// interface types.
  bool hasPointerRepresentation() const;

  /// hasObjCPointerRepresentation - Whether this type can represent
  /// an objective pointer type for the purpose of GC'ability
  bool hasObjCPointerRepresentation() const; 

  // Type Checking Functions: Check to see if this type is structurally the
  // specified type, ignoring typedefs and qualifiers, and return a pointer to
  // the best type we can.
  const BuiltinType *getAsBuiltinType() const;
  const FunctionType *getAsFunctionType() const;
  const FunctionNoProtoType *getAsFunctionNoProtoType() const;
  const FunctionProtoType *getAsFunctionProtoType() const;
  const PointerType *getAsPointerType() const;
  const BlockPointerType *getAsBlockPointerType() const;
  const ReferenceType *getAsReferenceType() const;
  const LValueReferenceType *getAsLValueReferenceType() const;
  const RValueReferenceType *getAsRValueReferenceType() const;
  const MemberPointerType *getAsMemberPointerType() const;
  const TagType *getAsTagType() const;
  const RecordType *getAsRecordType() const;
  const RecordType *getAsStructureType() const;
  /// NOTE: getAs*ArrayType are methods on ASTContext.
  const TypedefType *getAsTypedefType() const;
  const RecordType *getAsUnionType() const;
  const EnumType *getAsEnumType() const;
  const VectorType *getAsVectorType() const; // GCC vector type.
  const ComplexType *getAsComplexType() const;
  const ComplexType *getAsComplexIntegerType() const; // GCC complex int type.
  const ExtVectorType *getAsExtVectorType() const; // Extended vector type.
  const ObjCInterfaceType *getAsObjCInterfaceType() const;
  const ObjCQualifiedInterfaceType *getAsObjCQualifiedInterfaceType() const;
  const ObjCQualifiedIdType *getAsObjCQualifiedIdType() const;
  const TemplateTypeParmType *getAsTemplateTypeParmType() const;

  const ClassTemplateSpecializationType *
    getAsClassTemplateSpecializationType() const;
  
  /// getAsPointerToObjCInterfaceType - If this is a pointer to an ObjC
  /// interface, return the interface type, otherwise return null.
  const ObjCInterfaceType *getAsPointerToObjCInterfaceType() const;

  /// getArrayElementTypeNoTypeQual - If this is an array type, return the
  /// element type of the array, potentially with type qualifiers missing.
  /// This method should never be used when type qualifiers are meaningful.
  const Type *getArrayElementTypeNoTypeQual() const;
  
  /// getDesugaredType - Return the specified type with any "sugar" removed from
  /// the type.  This takes off typedefs, typeof's etc.  If the outer level of
  /// the type is already concrete, it returns it unmodified.  This is similar
  /// to getting the canonical type, but it doesn't remove *all* typedefs.  For
  /// example, it returns "T*" as "T*", (not as "int*"), because the pointer is
  /// concrete.
  QualType getDesugaredType() const;
  
  /// More type predicates useful for type checking/promotion
  bool isPromotableIntegerType() const; // C99 6.3.1.1p2

  /// isSignedIntegerType - Return true if this is an integer type that is
  /// signed, according to C99 6.2.5p4 [char, signed char, short, int, long..],
  /// an enum decl which has a signed representation, or a vector of signed
  /// integer element type.
  bool isSignedIntegerType() const;

  /// isUnsignedIntegerType - Return true if this is an integer type that is
  /// unsigned, according to C99 6.2.5p6 [which returns true for _Bool], an enum
  /// decl which has an unsigned representation, or a vector of unsigned integer
  /// element type.
  bool isUnsignedIntegerType() const;

  /// isConstantSizeType - Return true if this is not a variable sized type,
  /// according to the rules of C99 6.7.5p3.  It is not legal to call this on
  /// incomplete types.
  bool isConstantSizeType() const;

  QualType getCanonicalTypeInternal() const { return CanonicalType; }
  void dump() const;
  virtual void getAsStringInternal(std::string &InnerString) const = 0;
  static bool classof(const Type *) { return true; }
  
protected:  
  /// Emit - Emit a Type to bitcode.  Used by ASTContext.
  void Emit(llvm::Serializer& S) const;
  
  /// Create - Construct a Type from bitcode.  Used by ASTContext.
  static void Create(ASTContext& Context, unsigned i, llvm::Deserializer& S);
  
  /// EmitImpl - Subclasses must implement this method in order to
  ///  be serialized.
  // FIXME: Make this abstract once implemented.
  virtual void EmitImpl(llvm::Serializer& S) const {
    assert(false && "Serialization for type not supported.");
  }
};

/// ExtQualType - TR18037 (C embedded extensions) 6.2.5p26 
/// This supports all kinds of type attributes; including,
/// address space qualified types, objective-c's __weak and
/// __strong attributes.
///
class ExtQualType : public Type, public llvm::FoldingSetNode {
  /// BaseType - This is the underlying type that this qualifies.  All CVR
  /// qualifiers are stored on the QualType that references this type, so we
  /// can't have any here.
  Type *BaseType;

  /// Address Space ID - The address space ID this type is qualified with.
  unsigned AddressSpace;
  /// GC __weak/__strong attributes
  QualType::GCAttrTypes GCAttrType;
  
  ExtQualType(Type *Base, QualType CanonicalPtr, unsigned AddrSpace,
              QualType::GCAttrTypes gcAttr) :
      Type(ExtQual, CanonicalPtr, Base->isDependentType()), BaseType(Base),
      AddressSpace(AddrSpace), GCAttrType(gcAttr) {
    assert(!isa<ExtQualType>(BaseType) &&
           "Cannot have ExtQualType of ExtQualType");
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  Type *getBaseType() const { return BaseType; }
  QualType::GCAttrTypes getObjCGCAttr() const { return GCAttrType; }
  unsigned getAddressSpace() const { return AddressSpace; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getBaseType(), AddressSpace, GCAttrType);
  }
  static void Profile(llvm::FoldingSetNodeID &ID, Type *Base, 
                      unsigned AddrSpace, QualType::GCAttrTypes gcAttr) {
    ID.AddPointer(Base);
    ID.AddInteger(AddrSpace);
    ID.AddInteger(gcAttr);
  }
  
  static bool classof(const Type *T) { return T->getTypeClass() == ExtQual; }
  static bool classof(const ExtQualType *) { return true; }
  
protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};


/// BuiltinType - This class is used for builtin types like 'int'.  Builtin
/// types are always canonical and have a literal name field.
class BuiltinType : public Type {
public:
  enum Kind {
    Void,
    
    Bool,     // This is bool and/or _Bool.
    Char_U,   // This is 'char' for targets where char is unsigned.
    UChar,    // This is explicitly qualified unsigned char.
    UShort,
    UInt,
    ULong,
    ULongLong,
    
    Char_S,   // This is 'char' for targets where char is signed.
    SChar,    // This is explicitly qualified signed char.
    WChar,    // This is 'wchar_t' for C++.
    Short,
    Int,
    Long,
    LongLong,
    
    Float, Double, LongDouble,

    Overload,  // This represents the type of an overloaded function declaration.
    Dependent  // This represents the type of a type-dependent expression.
  };
private:
  Kind TypeKind;
public:
  BuiltinType(Kind K) 
    : Type(Builtin, QualType(), /*Dependent=*/(K == Dependent)), 
      TypeKind(K) {}
  
  Kind getKind() const { return TypeKind; }
  const char *getName() const;
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  static bool classof(const Type *T) { return T->getTypeClass() == Builtin; }
  static bool classof(const BuiltinType *) { return true; }
};

/// FixedWidthIntType - Used for arbitrary width types that we either don't
/// want to or can't map to named integer types.  These always have a lower
/// integer rank than builtin types of the same width.
class FixedWidthIntType : public Type {
private:
  unsigned Width;
  bool Signed;
public:
  FixedWidthIntType(unsigned W, bool S) : Type(FixedWidthInt, QualType(), false),
                                          Width(W), Signed(S) {}
  
  unsigned getWidth() const { return Width; }
  bool isSigned() const { return Signed; }
  const char *getName() const;
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  static bool classof(const Type *T) { return T->getTypeClass() == FixedWidthInt; }
  static bool classof(const FixedWidthIntType *) { return true; }
};

/// ComplexType - C99 6.2.5p11 - Complex values.  This supports the C99 complex
/// types (_Complex float etc) as well as the GCC integer complex extensions.
///
class ComplexType : public Type, public llvm::FoldingSetNode {
  QualType ElementType;
  ComplexType(QualType Element, QualType CanonicalPtr) :
    Type(Complex, CanonicalPtr, Element->isDependentType()), 
    ElementType(Element) {
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  QualType getElementType() const { return ElementType; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;
    
  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getElementType());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Element) {
    ID.AddPointer(Element.getAsOpaquePtr());
  }
  
  static bool classof(const Type *T) { return T->getTypeClass() == Complex; }
  static bool classof(const ComplexType *) { return true; }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// PointerType - C99 6.7.5.1 - Pointer Declarators.
///
class PointerType : public Type, public llvm::FoldingSetNode {
  QualType PointeeType;

  PointerType(QualType Pointee, QualType CanonicalPtr) :
    Type(Pointer, CanonicalPtr, Pointee->isDependentType()), PointeeType(Pointee) {
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  QualType getPointeeType() const { return PointeeType; }

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getPointeeType());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Pointee) {
    ID.AddPointer(Pointee.getAsOpaquePtr());
  }
  
  static bool classof(const Type *T) { return T->getTypeClass() == Pointer; }
  static bool classof(const PointerType *) { return true; }

protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// BlockPointerType - pointer to a block type.
/// This type is to represent types syntactically represented as
/// "void (^)(int)", etc. Pointee is required to always be a function type.
///
class BlockPointerType : public Type, public llvm::FoldingSetNode {
  QualType PointeeType;  // Block is some kind of pointer type
  BlockPointerType(QualType Pointee, QualType CanonicalCls) :
    Type(BlockPointer, CanonicalCls, Pointee->isDependentType()), 
    PointeeType(Pointee) {
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  
  // Get the pointee type. Pointee is required to always be a function type.
  QualType getPointeeType() const { return PointeeType; }

  virtual void getAsStringInternal(std::string &InnerString) const;
    
  void Profile(llvm::FoldingSetNodeID &ID) {
      Profile(ID, getPointeeType());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Pointee) {
      ID.AddPointer(Pointee.getAsOpaquePtr());
  }
    
  static bool classof(const Type *T) { 
    return T->getTypeClass() == BlockPointer; 
  }
  static bool classof(const BlockPointerType *) { return true; }

  protected:  
    virtual void EmitImpl(llvm::Serializer& S) const;
    static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
    friend class Type;
};

/// ReferenceType - Base for LValueReferenceType and RValueReferenceType
///
class ReferenceType : public Type, public llvm::FoldingSetNode {
  QualType PointeeType;

protected:
  ReferenceType(TypeClass tc, QualType Referencee, QualType CanonicalRef) :
    Type(tc, CanonicalRef, Referencee->isDependentType()),
    PointeeType(Referencee) {
  }
public:
  QualType getPointeeType() const { return PointeeType; }

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getPointeeType());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Referencee) {
    ID.AddPointer(Referencee.getAsOpaquePtr());
  }

  static bool classof(const Type *T) {
    return T->getTypeClass() == LValueReference ||
           T->getTypeClass() == RValueReference;
  }
  static bool classof(const ReferenceType *) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
};

/// LValueReferenceType - C++ [dcl.ref] - Lvalue reference
///
class LValueReferenceType : public ReferenceType {
  LValueReferenceType(QualType Referencee, QualType CanonicalRef) :
    ReferenceType(LValueReference, Referencee, CanonicalRef) {
  }
  friend class ASTContext; // ASTContext creates these
public:
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) {
    return T->getTypeClass() == LValueReference;
  }
  static bool classof(const LValueReferenceType *) { return true; }

protected:
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// RValueReferenceType - C++0x [dcl.ref] - Rvalue reference
///
class RValueReferenceType : public ReferenceType {
  RValueReferenceType(QualType Referencee, QualType CanonicalRef) :
    ReferenceType(RValueReference, Referencee, CanonicalRef) {
  }
  friend class ASTContext; // ASTContext creates these
public:
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) {
    return T->getTypeClass() == RValueReference;
  }
  static bool classof(const RValueReferenceType *) { return true; }

protected:
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// MemberPointerType - C++ 8.3.3 - Pointers to members
///
class MemberPointerType : public Type, public llvm::FoldingSetNode {
  QualType PointeeType;
  /// The class of which the pointee is a member. Must ultimately be a
  /// RecordType, but could be a typedef or a template parameter too.
  const Type *Class;

  MemberPointerType(QualType Pointee, const Type *Cls, QualType CanonicalPtr) :
    Type(MemberPointer, CanonicalPtr,
         Cls->isDependentType() || Pointee->isDependentType()),
    PointeeType(Pointee), Class(Cls) {
  }
  friend class ASTContext; // ASTContext creates these.
public:

  QualType getPointeeType() const { return PointeeType; }

  const Type *getClass() const { return Class; }

  virtual void getAsStringInternal(std::string &InnerString) const;

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getPointeeType(), getClass());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Pointee,
                      const Type *Class) {
    ID.AddPointer(Pointee.getAsOpaquePtr());
    ID.AddPointer(Class);
  }

  static bool classof(const Type *T) {
    return T->getTypeClass() == MemberPointer;
  }
  static bool classof(const MemberPointerType *) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// ArrayType - C99 6.7.5.2 - Array Declarators.
///
class ArrayType : public Type, public llvm::FoldingSetNode {
public:
  /// ArraySizeModifier - Capture whether this is a normal array (e.g. int X[4])
  /// an array with a static size (e.g. int X[static 4]), or an array
  /// with a star size (e.g. int X[*]).
  /// 'static' is only allowed on function parameters.
  enum ArraySizeModifier {
    Normal, Static, Star
  };
private:
  /// ElementType - The element type of the array.
  QualType ElementType;
  
  // NOTE: VC++ treats enums as signed, avoid using the ArraySizeModifier enum
  /// NOTE: These fields are packed into the bitfields space in the Type class.
  unsigned SizeModifier : 2;
  
  /// IndexTypeQuals - Capture qualifiers in declarations like:
  /// 'int X[static restrict 4]'. For function parameters only.
  unsigned IndexTypeQuals : 3;
  
protected:
  // C++ [temp.dep.type]p1:
  //   A type is dependent if it is...
  //     - an array type constructed from any dependent type or whose
  //       size is specified by a constant expression that is
  //       value-dependent,
  ArrayType(TypeClass tc, QualType et, QualType can,
            ArraySizeModifier sm, unsigned tq)
    : Type(tc, can, et->isDependentType() || tc == DependentSizedArray),
      ElementType(et), SizeModifier(sm), IndexTypeQuals(tq) {}

  friend class ASTContext;  // ASTContext creates these.
public:
  QualType getElementType() const { return ElementType; }
  ArraySizeModifier getSizeModifier() const {
    return ArraySizeModifier(SizeModifier);
  }
  unsigned getIndexTypeQualifier() const { return IndexTypeQuals; }
  
  static bool classof(const Type *T) {
    return T->getTypeClass() == ConstantArray ||
           T->getTypeClass() == VariableArray ||
           T->getTypeClass() == IncompleteArray ||
           T->getTypeClass() == DependentSizedArray;
  }
  static bool classof(const ArrayType *) { return true; }
};

/// ConstantArrayType - This class represents C arrays with a specified constant
/// size.  For example 'int A[100]' has ConstantArrayType where the element type
/// is 'int' and the size is 100.
class ConstantArrayType : public ArrayType {
  llvm::APInt Size; // Allows us to unique the type.
  
  ConstantArrayType(QualType et, QualType can, const llvm::APInt &size,
                    ArraySizeModifier sm, unsigned tq)
    : ArrayType(ConstantArray, et, can, sm, tq), Size(size) {}
  friend class ASTContext;  // ASTContext creates these.
public:
  const llvm::APInt &getSize() const { return Size; }
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getElementType(), getSize(), 
            getSizeModifier(), getIndexTypeQualifier());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType ET,
                      const llvm::APInt &ArraySize, ArraySizeModifier SizeMod,
                      unsigned TypeQuals) {
    ID.AddPointer(ET.getAsOpaquePtr());
    ID.AddInteger(ArraySize.getZExtValue());
    ID.AddInteger(SizeMod);
    ID.AddInteger(TypeQuals);
  }
  static bool classof(const Type *T) { 
    return T->getTypeClass() == ConstantArray; 
  }
  static bool classof(const ConstantArrayType *) { return true; }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// IncompleteArrayType - This class represents C arrays with an unspecified
/// size.  For example 'int A[]' has an IncompleteArrayType where the element
/// type is 'int' and the size is unspecified.
class IncompleteArrayType : public ArrayType {
  IncompleteArrayType(QualType et, QualType can,
                    ArraySizeModifier sm, unsigned tq)
    : ArrayType(IncompleteArray, et, can, sm, tq) {}
  friend class ASTContext;  // ASTContext creates these.
public:

  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) { 
    return T->getTypeClass() == IncompleteArray; 
  }
  static bool classof(const IncompleteArrayType *) { return true; }
  
  friend class StmtIteratorBase;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getElementType(), getSizeModifier(), getIndexTypeQualifier());
  }
  
  static void Profile(llvm::FoldingSetNodeID &ID, QualType ET,
                      ArraySizeModifier SizeMod, unsigned TypeQuals) {
    ID.AddPointer(ET.getAsOpaquePtr());
    ID.AddInteger(SizeMod);
    ID.AddInteger(TypeQuals);
  }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// VariableArrayType - This class represents C arrays with a specified size
/// which is not an integer-constant-expression.  For example, 'int s[x+foo()]'.
/// Since the size expression is an arbitrary expression, we store it as such.
///
/// Note: VariableArrayType's aren't uniqued (since the expressions aren't) and
/// should not be: two lexically equivalent variable array types could mean
/// different things, for example, these variables do not have the same type
/// dynamically:
///
/// void foo(int x) {
///   int Y[x];
///   ++x;
///   int Z[x];
/// }
///
class VariableArrayType : public ArrayType {
  /// SizeExpr - An assignment expression. VLA's are only permitted within 
  /// a function block. 
  Stmt *SizeExpr;
  
  VariableArrayType(QualType et, QualType can, Expr *e,
                    ArraySizeModifier sm, unsigned tq)
    : ArrayType(VariableArray, et, can, sm, tq), SizeExpr((Stmt*) e) {}
  friend class ASTContext;  // ASTContext creates these.
  virtual void Destroy(ASTContext& C);

public:
  Expr *getSizeExpr() const { 
    // We use C-style casts instead of cast<> here because we do not wish
    // to have a dependency of Type.h on Stmt.h/Expr.h.
    return (Expr*) SizeExpr;
  }
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  static bool classof(const Type *T) { 
    return T->getTypeClass() == VariableArray; 
  }
  static bool classof(const VariableArrayType *) { return true; }
  
  friend class StmtIteratorBase;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    assert(0 && "Cannnot unique VariableArrayTypes.");
  }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// DependentSizedArrayType - This type represents an array type in
/// C++ whose size is a value-dependent expression. For example:
/// @code
/// template<typename T, int Size> 
/// class array {
///   T data[Size];
/// };
/// @endcode
/// For these types, we won't actually know what the array bound is
/// until template instantiation occurs, at which point this will
/// become either a ConstantArrayType or a VariableArrayType.
class DependentSizedArrayType : public ArrayType {
  /// SizeExpr - An assignment expression that will instantiate to the
  /// size of the array.
  Stmt *SizeExpr;
  
  DependentSizedArrayType(QualType et, QualType can, Expr *e,
			  ArraySizeModifier sm, unsigned tq)
    : ArrayType(DependentSizedArray, et, can, sm, tq), SizeExpr((Stmt*) e) {}
  friend class ASTContext;  // ASTContext creates these.
  virtual void Destroy(ASTContext& C);

public:
  Expr *getSizeExpr() const { 
    // We use C-style casts instead of cast<> here because we do not wish
    // to have a dependency of Type.h on Stmt.h/Expr.h.
    return (Expr*) SizeExpr;
  }
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  static bool classof(const Type *T) { 
    return T->getTypeClass() == DependentSizedArray; 
  }
  static bool classof(const DependentSizedArrayType *) { return true; }
  
  friend class StmtIteratorBase;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    assert(0 && "Cannnot unique DependentSizedArrayTypes.");
  }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// VectorType - GCC generic vector type. This type is created using
/// __attribute__((vector_size(n)), where "n" specifies the vector size in 
/// bytes. Since the constructor takes the number of vector elements, the 
/// client is responsible for converting the size into the number of elements.
class VectorType : public Type, public llvm::FoldingSetNode {
protected:
  /// ElementType - The element type of the vector.
  QualType ElementType;
  
  /// NumElements - The number of elements in the vector.
  unsigned NumElements;
  
  VectorType(QualType vecType, unsigned nElements, QualType canonType) :
    Type(Vector, canonType, vecType->isDependentType()), 
    ElementType(vecType), NumElements(nElements) {} 
  VectorType(TypeClass tc, QualType vecType, unsigned nElements, 
             QualType canonType) 
    : Type(tc, canonType, vecType->isDependentType()), ElementType(vecType), 
      NumElements(nElements) {} 
  friend class ASTContext;  // ASTContext creates these.
public:
    
  QualType getElementType() const { return ElementType; }
  unsigned getNumElements() const { return NumElements; } 

  virtual void getAsStringInternal(std::string &InnerString) const;
  
  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getElementType(), getNumElements(), getTypeClass());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType ElementType, 
                      unsigned NumElements, TypeClass TypeClass) {
    ID.AddPointer(ElementType.getAsOpaquePtr());
    ID.AddInteger(NumElements);
    ID.AddInteger(TypeClass);
  }
  static bool classof(const Type *T) { 
    return T->getTypeClass() == Vector || T->getTypeClass() == ExtVector; 
  }
  static bool classof(const VectorType *) { return true; }
};

/// ExtVectorType - Extended vector type. This type is created using
/// __attribute__((ext_vector_type(n)), where "n" is the number of elements.
/// Unlike vector_size, ext_vector_type is only allowed on typedef's. This
/// class enables syntactic extensions, like Vector Components for accessing
/// points, colors, and textures (modeled after OpenGL Shading Language).
class ExtVectorType : public VectorType {
  ExtVectorType(QualType vecType, unsigned nElements, QualType canonType) :
    VectorType(ExtVector, vecType, nElements, canonType) {} 
  friend class ASTContext;  // ASTContext creates these.
public:
  static int getPointAccessorIdx(char c) {
    switch (c) {
    default: return -1;
    case 'x': return 0;
    case 'y': return 1;
    case 'z': return 2;
    case 'w': return 3;
    }
  }
  static int getNumericAccessorIdx(char c) {
    switch (c) {
      default: return -1;
      case '0': return 0;
      case '1': return 1;
      case '2': return 2;
      case '3': return 3;
      case '4': return 4;
      case '5': return 5;
      case '6': return 6;
      case '7': return 7;
      case '8': return 8;
      case '9': return 9;
      case 'a': return 10;
      case 'b': return 11;
      case 'c': return 12;
      case 'd': return 13;
      case 'e': return 14;
      case 'f': return 15;
    }
  }
  
  static int getAccessorIdx(char c) {
    if (int idx = getPointAccessorIdx(c)+1) return idx-1;
    return getNumericAccessorIdx(c);
  }
  
  bool isAccessorWithinNumElements(char c) const {
    if (int idx = getAccessorIdx(c)+1)
      return unsigned(idx-1) < NumElements;
    return false;
  }
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) { 
    return T->getTypeClass() == ExtVector; 
  }
  static bool classof(const ExtVectorType *) { return true; }
};

/// FunctionType - C99 6.7.5.3 - Function Declarators.  This is the common base
/// class of FunctionNoProtoType and FunctionProtoType.
///
class FunctionType : public Type {
  /// SubClassData - This field is owned by the subclass, put here to pack
  /// tightly with the ivars in Type.
  bool SubClassData : 1;

  /// TypeQuals - Used only by FunctionProtoType, put here to pack with the
  /// other bitfields.
  /// The qualifiers are part of FunctionProtoType because...
  ///
  /// C++ 8.3.5p4: The return type, the parameter type list and the
  /// cv-qualifier-seq, [...], are part of the function type.
  ///
  unsigned TypeQuals : 3;
  
  // The type returned by the function.
  QualType ResultType;
protected:
  FunctionType(TypeClass tc, QualType res, bool SubclassInfo,
               unsigned typeQuals, QualType Canonical, bool Dependent)
    : Type(tc, Canonical, Dependent),
      SubClassData(SubclassInfo), TypeQuals(typeQuals), ResultType(res) {}
  bool getSubClassData() const { return SubClassData; }
  unsigned getTypeQuals() const { return TypeQuals; }
public:
  
  QualType getResultType() const { return ResultType; }

  
  static bool classof(const Type *T) {
    return T->getTypeClass() == FunctionNoProto ||
           T->getTypeClass() == FunctionProto;
  }
  static bool classof(const FunctionType *) { return true; }
};

/// FunctionNoProtoType - Represents a K&R-style 'int foo()' function, which has
/// no information available about its arguments.
class FunctionNoProtoType : public FunctionType, public llvm::FoldingSetNode {
  FunctionNoProtoType(QualType Result, QualType Canonical)
    : FunctionType(FunctionNoProto, Result, false, 0, Canonical, 
                   /*Dependent=*/false) {}
  friend class ASTContext;  // ASTContext creates these.
public:
  // No additional state past what FunctionType provides.
  
  virtual void getAsStringInternal(std::string &InnerString) const;

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getResultType());
  }
  static void Profile(llvm::FoldingSetNodeID &ID, QualType ResultType) {
    ID.AddPointer(ResultType.getAsOpaquePtr());
  }
  
  static bool classof(const Type *T) {
    return T->getTypeClass() == FunctionNoProto;
  }
  static bool classof(const FunctionNoProtoType *) { return true; }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// FunctionProtoType - Represents a prototype with argument type info, e.g.
/// 'int foo(int)' or 'int foo(void)'.  'void' is represented as having no
/// arguments, not as having a single void argument.
class FunctionProtoType : public FunctionType, public llvm::FoldingSetNode {
  /// hasAnyDependentType - Determine whether there are any dependent
  /// types within the arguments passed in.
  static bool hasAnyDependentType(const QualType *ArgArray, unsigned numArgs) {
    for (unsigned Idx = 0; Idx < numArgs; ++Idx)
      if (ArgArray[Idx]->isDependentType())
    return true;

    return false;
  }

  FunctionProtoType(QualType Result, const QualType *ArgArray, unsigned numArgs,
                    bool isVariadic, unsigned typeQuals, QualType Canonical)
    : FunctionType(FunctionProto, Result, isVariadic, typeQuals, Canonical,
                   (Result->isDependentType() || 
                    hasAnyDependentType(ArgArray, numArgs))),
      NumArgs(numArgs) {
    // Fill in the trailing argument array.
    QualType *ArgInfo = reinterpret_cast<QualType *>(this+1);;
    for (unsigned i = 0; i != numArgs; ++i)
      ArgInfo[i] = ArgArray[i];
  }
  
  /// NumArgs - The number of arguments this function has, not counting '...'.
  unsigned NumArgs;
  
  /// ArgInfo - There is an variable size array after the class in memory that
  /// holds the argument types.
  
  friend class ASTContext;  // ASTContext creates these.

public:
  unsigned getNumArgs() const { return NumArgs; }
  QualType getArgType(unsigned i) const {
    assert(i < NumArgs && "Invalid argument number!");
    return arg_type_begin()[i];
  }
    
  bool isVariadic() const { return getSubClassData(); }
  unsigned getTypeQuals() const { return FunctionType::getTypeQuals(); }
  
  typedef const QualType *arg_type_iterator;
  arg_type_iterator arg_type_begin() const {
    return reinterpret_cast<const QualType *>(this+1);
  }
  arg_type_iterator arg_type_end() const { return arg_type_begin()+NumArgs; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) {
    return T->getTypeClass() == FunctionProto;
  }
  static bool classof(const FunctionProtoType *) { return true; }
  
  void Profile(llvm::FoldingSetNodeID &ID);
  static void Profile(llvm::FoldingSetNodeID &ID, QualType Result,
                      arg_type_iterator ArgTys, unsigned NumArgs,
                      bool isVariadic, unsigned TypeQuals);

protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};


class TypedefType : public Type {
  TypedefDecl *Decl;
protected:
  TypedefType(TypeClass tc, TypedefDecl *D, QualType can) 
    : Type(tc, can, can->isDependentType()), Decl(D) {
    assert(!isa<TypedefType>(can) && "Invalid canonical type");
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  
  TypedefDecl *getDecl() const { return Decl; }
  
  /// LookThroughTypedefs - Return the ultimate type this typedef corresponds to
  /// potentially looking through *all* consecutive typedefs.  This returns the
  /// sum of the type qualifiers, so if you have:
  ///   typedef const int A;
  ///   typedef volatile A B;
  /// looking through the typedefs for B will give you "const volatile A".
  QualType LookThroughTypedefs() const;
    
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) { return T->getTypeClass() == Typedef; }
  static bool classof(const TypedefType *) { return true; }
  
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context,llvm::Deserializer& D);
  friend class Type;
};

/// TypeOfExprType (GCC extension).
class TypeOfExprType : public Type {
  Expr *TOExpr;
  TypeOfExprType(Expr *E, QualType can);
  friend class ASTContext;  // ASTContext creates these.
public:
  Expr *getUnderlyingExpr() const { return TOExpr; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) { return T->getTypeClass() == TypeOfExpr; }
  static bool classof(const TypeOfExprType *) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// TypeOfType (GCC extension).
class TypeOfType : public Type {
  QualType TOType;
  TypeOfType(QualType T, QualType can) 
    : Type(TypeOf, can, T->isDependentType()), TOType(T) {
    assert(!isa<TypedefType>(can) && "Invalid canonical type");
  }
  friend class ASTContext;  // ASTContext creates these.
public:
  QualType getUnderlyingType() const { return TOType; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;

  static bool classof(const Type *T) { return T->getTypeClass() == TypeOf; }
  static bool classof(const TypeOfType *) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

class TagType : public Type {
  /// Stores the TagDecl associated with this type. The decl will
  /// point to the TagDecl that actually defines the entity (or is a
  /// definition in progress), if there is such a definition. The
  /// single-bit value will be non-zero when this tag is in the
  /// process of being defined.
  mutable llvm::PointerIntPair<TagDecl *, 1> decl;
  friend class ASTContext;
  friend class TagDecl;

protected:
  // FIXME: We'll need the user to pass in information about whether
  // this type is dependent or not, because we don't have enough
  // information to compute it here.
  TagType(TypeClass TC, TagDecl *D, QualType can) 
    : Type(TC, can, /*Dependent=*/false), decl(D, 0) {}

public:   
  TagDecl *getDecl() const { return decl.getPointer(); }
  
  /// @brief Determines whether this type is in the process of being
  /// defined. 
  bool isBeingDefined() const { return decl.getInt(); }
  void setBeingDefined(bool Def) { decl.setInt(Def? 1 : 0); }

  virtual void getAsStringInternal(std::string &InnerString) const;
  void getAsStringInternal(std::string &InnerString,
                           bool SuppressTagKind) const;

  static bool classof(const Type *T) { 
    return T->getTypeClass() >= TagFirst && T->getTypeClass() <= TagLast;
  }
  static bool classof(const TagType *) { return true; }
  static bool classof(const RecordType *) { return true; }
  static bool classof(const EnumType *) { return true; }
 
protected:  
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// RecordType - This is a helper class that allows the use of isa/cast/dyncast
/// to detect TagType objects of structs/unions/classes.
class RecordType : public TagType {
protected:
  explicit RecordType(RecordDecl *D)
    : TagType(Record, reinterpret_cast<TagDecl*>(D), QualType()) { }
  explicit RecordType(TypeClass TC, RecordDecl *D)
    : TagType(TC, reinterpret_cast<TagDecl*>(D), QualType()) { }
  friend class ASTContext;   // ASTContext creates these.
public:
    
  RecordDecl *getDecl() const {
    return reinterpret_cast<RecordDecl*>(TagType::getDecl());
  }
  
  // FIXME: This predicate is a helper to QualType/Type. It needs to 
  // recursively check all fields for const-ness. If any field is declared
  // const, it needs to return false. 
  bool hasConstFields() const { return false; }

  // FIXME: RecordType needs to check when it is created that all fields are in
  // the same address space, and return that.
  unsigned getAddressSpace() const { return 0; }
  
  static bool classof(const TagType *T);
  static bool classof(const Type *T) {
    return isa<TagType>(T) && classof(cast<TagType>(T));
  }
  static bool classof(const RecordType *) { return true; }
};

/// EnumType - This is a helper class that allows the use of isa/cast/dyncast
/// to detect TagType objects of enums.
class EnumType : public TagType {
  explicit EnumType(EnumDecl *D)
    : TagType(Enum, reinterpret_cast<TagDecl*>(D), QualType()) { }
  friend class ASTContext;   // ASTContext creates these.
public:
    
  EnumDecl *getDecl() const {
    return reinterpret_cast<EnumDecl*>(TagType::getDecl());
  }
  
  static bool classof(const TagType *T);
  static bool classof(const Type *T) {
    return isa<TagType>(T) && classof(cast<TagType>(T));
  }
  static bool classof(const EnumType *) { return true; }
};

class TemplateTypeParmType : public Type, public llvm::FoldingSetNode {
  unsigned Depth : 16;
  unsigned Index : 16;
  IdentifierInfo *Name;

  TemplateTypeParmType(unsigned D, unsigned I, IdentifierInfo *N, 
                       QualType Canon) 
    : Type(TemplateTypeParm, Canon, /*Dependent=*/true),
      Depth(D), Index(I), Name(N) { }

  TemplateTypeParmType(unsigned D, unsigned I) 
    : Type(TemplateTypeParm, QualType(this, 0), /*Dependent=*/true),
      Depth(D), Index(I), Name(0) { }

  friend class ASTContext;  // ASTContext creates these

public:
  unsigned getDepth() const { return Depth; }
  unsigned getIndex() const { return Index; }
  IdentifierInfo *getName() const { return Name; }
  
  virtual void getAsStringInternal(std::string &InnerString) const;

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, Depth, Index, Name);
  }

  static void Profile(llvm::FoldingSetNodeID &ID, unsigned Depth, 
                      unsigned Index, IdentifierInfo *Name) {
    ID.AddInteger(Depth);
    ID.AddInteger(Index);
    ID.AddPointer(Name);
  }

  static bool classof(const Type *T) { 
    return T->getTypeClass() == TemplateTypeParm; 
  }
  static bool classof(const TemplateTypeParmType *T) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// \brief Represents the type of a class template specialization as
/// written in the source code.
///
/// Class template specialization types represent the syntactic form
/// of a template-id that refers to a type, e.g., @c vector<int>. All
/// class template specialization types are syntactic sugar, whose
/// canonical type will point to some other type node that represents
/// the instantiation or class template specialization. For example, a
/// class template specialization type of @c vector<int> will refer to
/// a tag type for the instantiation
/// @c std::vector<int, std::allocator<int>>.
class ClassTemplateSpecializationType 
  : public Type, public llvm::FoldingSetNode {

  // FIXME: Do we want templates to have a representation in the type
  // system? It will probably help with dependent templates and
  // possibly with template-names preceded by a nested-name-specifier.
  TemplateDecl *Template;

  /// \brief - The number of template arguments named in this class
  /// template specialization.
  unsigned NumArgs;

  ClassTemplateSpecializationType(TemplateDecl *T, 
                                  const TemplateArgument *Args,
                                  unsigned NumArgs, QualType Canon);

  virtual void Destroy(ASTContext& C);

  friend class ASTContext;  // ASTContext creates these

public:
  /// \brief Determine whether any of the given template arguments are
  /// dependent.
  static bool anyDependentTemplateArguments(const TemplateArgument *Args,
                                            unsigned NumArgs);  

  /// \brief Print a template argument list, including the '<' and '>'
  /// enclosing the template arguments.
  static std::string PrintTemplateArgumentList(const TemplateArgument *Args,
                                               unsigned NumArgs);

  typedef const TemplateArgument * iterator;

  iterator begin() const { return getArgs(); }
  iterator end() const;

  /// \brief Retrieve the template that we are specializing.
  TemplateDecl *getTemplate() const { return Template; }

  /// \brief Retrieve the template arguments.
  const TemplateArgument *getArgs() const { 
    return reinterpret_cast<const TemplateArgument *>(this + 1);
  }

  /// \brief Retrieve the number of template arguments.
  unsigned getNumArgs() const { return NumArgs; }

  /// \brief Retrieve a specific template argument as a type.
  /// \precondition @c isArgType(Arg)
  const TemplateArgument &getArg(unsigned Idx) const;

  virtual void getAsStringInternal(std::string &InnerString) const;

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, Template, getArgs(), NumArgs);
  }

  static void Profile(llvm::FoldingSetNodeID &ID, TemplateDecl *T,
                      const TemplateArgument *Args, unsigned NumArgs);

  static bool classof(const Type *T) { 
    return T->getTypeClass() == ClassTemplateSpecialization; 
  }
  static bool classof(const ClassTemplateSpecializationType *T) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// \brief Represents a type that was referred to via a qualified
/// name, e.g., N::M::type.
///
/// This type is used to keep track of a type name as written in the
/// source code, including any nested-name-specifiers.
class QualifiedNameType : public Type, public llvm::FoldingSetNode {
  /// \brief The number of components in the qualified name, not
  /// counting the final type.
  unsigned NumComponents;

  /// \brief The type that this qualified name refers to.
  QualType NamedType;

  QualifiedNameType(const NestedNameSpecifier *Components,
                    unsigned NumComponents, QualType NamedType,
                    QualType CanonType);

  friend class ASTContext;  // ASTContext creates these

public:
  typedef const NestedNameSpecifier * iterator;

  iterator begin() const { return getComponents(); }
  iterator end() const { return getComponents() + getNumComponents(); }

  /// \brief Retrieve the array of nested-name-specifier components.
  const NestedNameSpecifier *getComponents() const { 
    return reinterpret_cast<const NestedNameSpecifier *>(this + 1);
  }

  /// \brief Retrieve the number of nested-name-specifier components.
  unsigned getNumComponents() const { return NumComponents; }

  /// \brief Retrieve the type named by the qualified-id.
  QualType getNamedType() const { return NamedType; }

  virtual void getAsStringInternal(std::string &InnerString) const;

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getComponents(), NumComponents, NamedType);
  }

  static void Profile(llvm::FoldingSetNodeID &ID, 
                      const NestedNameSpecifier *Components,
                      unsigned NumComponents, 
                      QualType NamedType);

  static bool classof(const Type *T) { 
    return T->getTypeClass() == QualifiedName; 
  }
  static bool classof(const QualifiedNameType *T) { return true; }

protected:
  virtual void EmitImpl(llvm::Serializer& S) const;
  static Type* CreateImpl(ASTContext& Context, llvm::Deserializer& D);
  friend class Type;
};

/// ObjCInterfaceType - Interfaces are the core concept in Objective-C for
/// object oriented design.  They basically correspond to C++ classes.  There
/// are two kinds of interface types, normal interfaces like "NSString" and
/// qualified interfaces, which are qualified with a protocol list like
/// "NSString<NSCopyable, NSAmazing>".  Qualified interface types are instances
/// of ObjCQualifiedInterfaceType, which is a subclass of ObjCInterfaceType.
class ObjCInterfaceType : public Type {
  ObjCInterfaceDecl *Decl;
protected:
  ObjCInterfaceType(TypeClass tc, ObjCInterfaceDecl *D) : 
    Type(tc, QualType(), /*Dependent=*/false), Decl(D) { }
  friend class ASTContext;  // ASTContext creates these.
public:
  
  ObjCInterfaceDecl *getDecl() const { return Decl; }
  
  /// qual_iterator and friends: this provides access to the (potentially empty)
  /// list of protocols qualifying this interface.  If this is an instance of
  /// ObjCQualifiedInterfaceType it returns the list, otherwise it returns an
  /// empty list if there are no qualifying protocols.
  typedef llvm::SmallVector<ObjCProtocolDecl*, 8>::const_iterator qual_iterator;
  inline qual_iterator qual_begin() const;
  inline qual_iterator qual_end() const;
  bool qual_empty() const { return getTypeClass() != ObjCQualifiedInterface; }

  /// getNumProtocols - Return the number of qualifying protocols in this
  /// interface type, or 0 if there are none.
  inline unsigned getNumProtocols() const;
  
  /// getProtocol - Return the specified qualifying protocol.
  inline ObjCProtocolDecl *getProtocol(unsigned i) const;
  
  
  virtual void getAsStringInternal(std::string &InnerString) const;
  static bool classof(const Type *T) { 
    return T->getTypeClass() == ObjCInterface ||
           T->getTypeClass() == ObjCQualifiedInterface; 
  }
  static bool classof(const ObjCInterfaceType *) { return true; }
};

/// ObjCQualifiedInterfaceType - This class represents interface types 
/// conforming to a list of protocols, such as INTF<Proto1, Proto2, Proto1>.
///
/// Duplicate protocols are removed and protocol list is canonicalized to be in
/// alphabetical order.
class ObjCQualifiedInterfaceType : public ObjCInterfaceType, 
                                   public llvm::FoldingSetNode {
                                     
  // List of protocols for this protocol conforming object type
  // List is sorted on protocol name. No protocol is enterred more than once.
  llvm::SmallVector<ObjCProtocolDecl*, 4> Protocols;

  ObjCQualifiedInterfaceType(ObjCInterfaceDecl *D,
                             ObjCProtocolDecl **Protos, unsigned NumP) : 
    ObjCInterfaceType(ObjCQualifiedInterface, D), 
    Protocols(Protos, Protos+NumP) { }
  friend class ASTContext;  // ASTContext creates these.
public:
  
  ObjCProtocolDecl *getProtocol(unsigned i) const {
    return Protocols[i];
  }
  unsigned getNumProtocols() const {
    return Protocols.size();
  }

  qual_iterator qual_begin() const { return Protocols.begin(); }
  qual_iterator qual_end() const   { return Protocols.end(); }
                                     
  virtual void getAsStringInternal(std::string &InnerString) const;
  
  void Profile(llvm::FoldingSetNodeID &ID);
  static void Profile(llvm::FoldingSetNodeID &ID, 
                      const ObjCInterfaceDecl *Decl,
                      ObjCProtocolDecl **protocols, unsigned NumProtocols);
 
  static bool classof(const Type *T) { 
    return T->getTypeClass() == ObjCQualifiedInterface; 
  }
  static bool classof(const ObjCQualifiedInterfaceType *) { return true; }
};
  
inline ObjCInterfaceType::qual_iterator ObjCInterfaceType::qual_begin() const {
  if (const ObjCQualifiedInterfaceType *QIT = 
         dyn_cast<ObjCQualifiedInterfaceType>(this))
    return QIT->qual_begin();
  return 0;
}
inline ObjCInterfaceType::qual_iterator ObjCInterfaceType::qual_end() const {
  if (const ObjCQualifiedInterfaceType *QIT = 
         dyn_cast<ObjCQualifiedInterfaceType>(this))
    return QIT->qual_end();
  return 0;
}

/// getNumProtocols - Return the number of qualifying protocols in this
/// interface type, or 0 if there are none.
inline unsigned ObjCInterfaceType::getNumProtocols() const {
  if (const ObjCQualifiedInterfaceType *QIT = 
        dyn_cast<ObjCQualifiedInterfaceType>(this))
    return QIT->getNumProtocols();
  return 0;
}

/// getProtocol - Return the specified qualifying protocol.
inline ObjCProtocolDecl *ObjCInterfaceType::getProtocol(unsigned i) const {
  return cast<ObjCQualifiedInterfaceType>(this)->getProtocol(i);
}
  
  

/// ObjCQualifiedIdType - to represent id<protocol-list>.
///
/// Duplicate protocols are removed and protocol list is canonicalized to be in
/// alphabetical order.
class ObjCQualifiedIdType : public Type,
                            public llvm::FoldingSetNode {
  // List of protocols for this protocol conforming 'id' type
  // List is sorted on protocol name. No protocol is enterred more than once.
  llvm::SmallVector<ObjCProtocolDecl*, 8> Protocols;
    
  ObjCQualifiedIdType(ObjCProtocolDecl **Protos, unsigned NumP)
    : Type(ObjCQualifiedId, QualType()/*these are always canonical*/,
           /*Dependent=*/false), 
  Protocols(Protos, Protos+NumP) { }
  friend class ASTContext;  // ASTContext creates these.
public:
    
  ObjCProtocolDecl *getProtocols(unsigned i) const {
    return Protocols[i];
  }
  unsigned getNumProtocols() const {
    return Protocols.size();
  }
  ObjCProtocolDecl **getReferencedProtocols() {
    return &Protocols[0];
  }
                              
  typedef llvm::SmallVector<ObjCProtocolDecl*, 8>::const_iterator qual_iterator;
  qual_iterator qual_begin() const { return Protocols.begin(); }
  qual_iterator qual_end() const   { return Protocols.end(); }
    
  virtual void getAsStringInternal(std::string &InnerString) const;
    
  void Profile(llvm::FoldingSetNodeID &ID);
  static void Profile(llvm::FoldingSetNodeID &ID,
                      ObjCProtocolDecl **protocols, unsigned NumProtocols);
    
  static bool classof(const Type *T) { 
    return T->getTypeClass() == ObjCQualifiedId; 
  }
  static bool classof(const ObjCQualifiedIdType *) { return true; }
    
};
  
/// ObjCQualifiedClassType - to represent Class<protocol-list>.
///
/// Duplicate protocols are removed and protocol list is canonicalized to be in
/// alphabetical order.
class ObjCQualifiedClassType : public Type,
                               public llvm::FoldingSetNode {
  // List of protocols for this protocol conforming 'id' type
  // List is sorted on protocol name. No protocol is enterred more than once.
  llvm::SmallVector<ObjCProtocolDecl*, 8> Protocols;
    
  ObjCQualifiedClassType(ObjCProtocolDecl **Protos, unsigned NumP)
    : Type(ObjCQualifiedClass, QualType()/*these are always canonical*/,
           /*Dependent=*/false), 
  Protocols(Protos, Protos+NumP) { }
  friend class ASTContext;  // ASTContext creates these.
public:
    
  ObjCProtocolDecl *getProtocols(unsigned i) const {
    return Protocols[i];
  }
  unsigned getNumProtocols() const {
    return Protocols.size();
  }
  ObjCProtocolDecl **getReferencedProtocols() {
    return &Protocols[0];
  }
                              
  typedef llvm::SmallVector<ObjCProtocolDecl*, 8>::const_iterator qual_iterator;
  qual_iterator qual_begin() const { return Protocols.begin(); }
  qual_iterator qual_end() const   { return Protocols.end(); }
    
  virtual void getAsStringInternal(std::string &InnerString) const;
    
  void Profile(llvm::FoldingSetNodeID &ID);
  static void Profile(llvm::FoldingSetNodeID &ID,
                      ObjCProtocolDecl **protocols, unsigned NumProtocols);
    
  static bool classof(const Type *T) { 
    return T->getTypeClass() == ObjCQualifiedClass; 
  }
  static bool classof(const ObjCQualifiedClassType *) { return true; }
    
};

// Inline function definitions.

/// getUnqualifiedType - Return the type without any qualifiers.
inline QualType QualType::getUnqualifiedType() const {
  Type *TP = getTypePtr();
  if (const ExtQualType *EXTQT = dyn_cast<ExtQualType>(TP))
    TP = EXTQT->getBaseType();
  return QualType(TP, 0);
}

/// getAddressSpace - Return the address space of this type.
inline unsigned QualType::getAddressSpace() const {
  QualType CT = getTypePtr()->getCanonicalTypeInternal();
  if (const ArrayType *AT = dyn_cast<ArrayType>(CT))
    return AT->getElementType().getAddressSpace();
  if (const RecordType *RT = dyn_cast<RecordType>(CT))
    return RT->getAddressSpace();
  if (const ExtQualType *EXTQT = dyn_cast<ExtQualType>(CT))
    return EXTQT->getAddressSpace();
  return 0;
}

/// getObjCGCAttr - Return the gc attribute of this type.
inline QualType::GCAttrTypes QualType::getObjCGCAttr() const {
  QualType CT = getTypePtr()->getCanonicalTypeInternal();
  if (const ArrayType *AT = dyn_cast<ArrayType>(CT))
      return AT->getElementType().getObjCGCAttr();
  if (const ExtQualType *EXTQT = dyn_cast<ExtQualType>(CT))
    return EXTQT->getObjCGCAttr();
  if (const PointerType *PT = CT->getAsPointerType())
    return PT->getPointeeType().getObjCGCAttr(); 
  return GCNone;
}
  
/// isMoreQualifiedThan - Determine whether this type is more
/// qualified than the Other type. For example, "const volatile int"
/// is more qualified than "const int", "volatile int", and
/// "int". However, it is not more qualified than "const volatile
/// int".
inline bool QualType::isMoreQualifiedThan(QualType Other) const {
  // FIXME: Handle address spaces
  unsigned MyQuals = this->getCVRQualifiers();
  unsigned OtherQuals = Other.getCVRQualifiers();
  assert(this->getAddressSpace() == 0 && "Address space not checked");
  assert(Other.getAddressSpace() == 0 && "Address space not checked");
  return MyQuals != OtherQuals && (MyQuals | OtherQuals) == MyQuals;
}

/// isAtLeastAsQualifiedAs - Determine whether this type is at last
/// as qualified as the Other type. For example, "const volatile
/// int" is at least as qualified as "const int", "volatile int",
/// "int", and "const volatile int".
inline bool QualType::isAtLeastAsQualifiedAs(QualType Other) const {
  // FIXME: Handle address spaces
  unsigned MyQuals = this->getCVRQualifiers();
  unsigned OtherQuals = Other.getCVRQualifiers();
  assert(this->getAddressSpace() == 0 && "Address space not checked");
  assert(Other.getAddressSpace() == 0 && "Address space not checked");
  return (MyQuals | OtherQuals) == MyQuals;
}

/// getNonReferenceType - If Type is a reference type (e.g., const
/// int&), returns the type that the reference refers to ("const
/// int"). Otherwise, returns the type itself. This routine is used
/// throughout Sema to implement C++ 5p6:
///
///   If an expression initially has the type "reference to T" (8.3.2,
///   8.5.3), the type is adjusted to "T" prior to any further
///   analysis, the expression designates the object or function
///   denoted by the reference, and the expression is an lvalue.
inline QualType QualType::getNonReferenceType() const {
  if (const ReferenceType *RefType = (*this)->getAsReferenceType())
    return RefType->getPointeeType();
  else
    return *this;
}

inline const TypedefType* Type::getAsTypedefType() const {
  return dyn_cast<TypedefType>(this);
}
inline const ObjCInterfaceType *Type::getAsPointerToObjCInterfaceType() const {
  if (const PointerType *PT = getAsPointerType())
    return PT->getPointeeType()->getAsObjCInterfaceType();
  return 0;
}
  
// NOTE: All of these methods use "getUnqualifiedType" to strip off address
// space qualifiers if present.
inline bool Type::isFunctionType() const {
  return isa<FunctionType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isPointerType() const {
  return isa<PointerType>(CanonicalType.getUnqualifiedType()); 
}
inline bool Type::isBlockPointerType() const {
    return isa<BlockPointerType>(CanonicalType); 
}
inline bool Type::isReferenceType() const {
  return isa<ReferenceType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isLValueReferenceType() const {
  return isa<LValueReferenceType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isRValueReferenceType() const {
  return isa<RValueReferenceType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isFunctionPointerType() const {
  if (const PointerType* T = getAsPointerType())
    return T->getPointeeType()->isFunctionType();
  else
    return false;
}
inline bool Type::isMemberPointerType() const {
  return isa<MemberPointerType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isMemberFunctionPointerType() const {
  if (const MemberPointerType* T = getAsMemberPointerType())
    return T->getPointeeType()->isFunctionType();
  else
    return false;
}
inline bool Type::isArrayType() const {
  return isa<ArrayType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isConstantArrayType() const {
  return isa<ConstantArrayType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isIncompleteArrayType() const {
  return isa<IncompleteArrayType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isVariableArrayType() const {
  return isa<VariableArrayType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isDependentSizedArrayType() const {
  return isa<DependentSizedArrayType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isRecordType() const {
  return isa<RecordType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isAnyComplexType() const {
  return isa<ComplexType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isVectorType() const {
  return isa<VectorType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isExtVectorType() const {
  return isa<ExtVectorType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isObjCInterfaceType() const {
  return isa<ObjCInterfaceType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isObjCQualifiedInterfaceType() const {
  return isa<ObjCQualifiedInterfaceType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isObjCQualifiedIdType() const {
  return isa<ObjCQualifiedIdType>(CanonicalType.getUnqualifiedType());
}
inline bool Type::isTemplateTypeParmType() const {
  return isa<TemplateTypeParmType>(CanonicalType.getUnqualifiedType());
}

inline bool Type::isSpecificBuiltinType(unsigned K) const {
  if (const BuiltinType *BT = getAsBuiltinType())
    if (BT->getKind() == (BuiltinType::Kind) K)
      return true;
  return false;
}

/// \brief Determines whether this is a type for which one can define
/// an overloaded operator.
inline bool Type::isOverloadableType() const {
  return isDependentType() || isRecordType() || isEnumeralType();
}

inline bool Type::hasPointerRepresentation() const {
  return (isPointerType() || isReferenceType() || isBlockPointerType() ||
          isObjCInterfaceType() || isObjCQualifiedIdType() || 
          isObjCQualifiedInterfaceType());
}

inline bool Type::hasObjCPointerRepresentation() const {
  return (isObjCInterfaceType() || isObjCQualifiedIdType() || 
          isObjCQualifiedInterfaceType());
}

/// Insertion operator for diagnostics.  This allows sending QualType's into a
/// diagnostic with <<.
inline const DiagnosticBuilder &operator<<(const DiagnosticBuilder &DB,
                                           QualType T) {
  DB.AddTaggedVal(reinterpret_cast<intptr_t>(T.getAsOpaquePtr()),
                  Diagnostic::ak_qualtype);
  return DB;
}

}  // end namespace clang

#endif

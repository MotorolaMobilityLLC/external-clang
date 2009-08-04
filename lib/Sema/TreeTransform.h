//===------- TreeTransform.h - Semantic Tree Transformation ---------------===/
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===/
//
//  This file implements a semantic tree transformation that takes a given
//  AST and rebuilds it, possibly transforming some nodes in the process.
//
//===----------------------------------------------------------------------===/
#ifndef LLVM_CLANG_SEMA_TREETRANSFORM_H
#define LLVM_CLANG_SEMA_TREETRANSFORM_H

#include "Sema.h"
#include <algorithm>

namespace clang {
  
/// \brief A semantic tree transformation that allows one to transform one
/// abstract syntax tree into another.
///
/// A new tree transformation is defined by creating a new subclass \c X of 
/// \c TreeTransform<X> and then overriding certain operations to provide 
/// behavior specific to that transformation. For example, template 
/// instantiation is implemented as a tree transformation where the
/// transformation of TemplateTypeParmType nodes involves substituting the
/// template arguments for their corresponding template parameters; a similar
/// transformation is performed for non-type template parameters and
/// template template parameters.
///
/// This tree-transformation template uses static polymorphism to allow
/// subclasses to customize any of its operations. Thus, a subclass can 
/// override any of the transformation or rebuild operators by providing an
/// operation with the same signature as the default implementation. The
/// overridding function should not be virtual.
///
/// Semantic tree transformations are split into two stages, either of which
/// can be replaced by a subclass. The "transform" step transforms an AST node
/// or the parts of an AST node using the various transformation functions,
/// then passes the pieces on to the "rebuild" step, which constructs a new AST
/// node of the appropriate kind from the pieces. The default transformation
/// routines recursively transform the operands to composite AST nodes (e.g.,
/// the pointee type of a PointerType node) and, if any of those operand nodes
/// were changed by the transformation, invokes the rebuild operation to create
/// a new AST node.
///
/// Subclasses can customize the transformation at various levels. The 
/// most course-grained transformations involve replacing TransformType(),
/// TransformExpr(), TransformDecl(), TransformNestedNameSpecifier(),
/// TransformTemplateName(), or TransformTemplateArgument() with entirely
/// new implementations.
///
/// For more fine-grained transformations, subclasses can replace any of the
/// \c TransformXXX functions (where XXX is the name of an AST node, e.g.,
/// PointerType) to alter the transformation. As mentioned previously,
/// replacing TransformTemplateTypeParmType() allows template instantiation
/// to substitute template arguments for their corresponding template 
/// parameters. Additionally, subclasses can override the \c RebuildXXX
/// functions to control how AST nodes are rebuilt when their operands change.
/// By default, \c TreeTransform will invoke semantic analysis to rebuild
/// AST nodes. However, certain other tree transformations (e.g, cloning) may
/// be able to use more efficient rebuild steps.
///
/// There are a handful of other functions that can be overridden, allowing one
/// to avoid traversing nodes that don't need any transformation 
/// (\c AlreadyTransformed()), force rebuilding AST nodes even when their
/// operands have not changed (\c AlwaysRebuild()), and customize the
/// default locations and entity names used for type-checking
/// (\c getBaseLocation(), \c getBaseEntity()).
///
/// FIXME: In the future, TreeTransform will support transformation of
/// statements and expressions as well as types.
template<typename Derived>
class TreeTransform {
protected:
  Sema &SemaRef;
  
public:  
  /// \brief Initializes a new tree transformer.
  TreeTransform(Sema &SemaRef) : SemaRef(SemaRef) { }
          
  /// \brief Retrieves a reference to the derived class.
  Derived &getDerived() { return static_cast<Derived&>(*this); }

  /// \brief Retrieves a reference to the derived class.
  const Derived &getDerived() const { 
    return static_cast<const Derived&>(*this); 
  }

  /// \brief Retrieves a reference to the semantic analysis object used for
  /// this tree transform.
  Sema &getSema() const { return SemaRef; }
  
  /// \brief Whether the transformation should always rebuild AST nodes, even
  /// if none of the children have changed.
  ///
  /// Subclasses may override this function to specify when the transformation
  /// should rebuild all AST nodes.
  bool AlwaysRebuild() { return false; }
  
  /// \brief Returns the location of the entity being transformed, if that
  /// information was not available elsewhere in the AST.
  ///
  /// By default, returns no source-location information. Subclasses can 
  /// provide an alternative implementation that provides better location
  /// information.
  SourceLocation getBaseLocation() { return SourceLocation(); }
  
  /// \brief Returns the name of the entity being transformed, if that
  /// information was not available elsewhere in the AST.
  ///
  /// By default, returns an empty name. Subclasses can provide an alternative
  /// implementation with a more precise name.
  DeclarationName getBaseEntity() { return DeclarationName(); }

  /// \brief Determine whether the given type \p T has already been 
  /// transformed.
  ///
  /// Subclasses can provide an alternative implementation of this routine
  /// to short-circuit evaluation when it is known that a given type will 
  /// not change. For example, template instantiation need not traverse
  /// non-dependent types.
  bool AlreadyTransformed(QualType T) {
    return T.isNull();
  }

  /// \brief Transforms the given type into another type.
  ///
  /// By default, this routine transforms a type by delegating to the
  /// appropriate TransformXXXType to build a new type, then applying 
  /// the qualifiers on \p T to the resulting type with AddTypeQualifiers. 
  /// Subclasses may override this function (to take over all type 
  /// transformations), some set of the TransformXXXType functions, or
  /// the AddTypeQualifiers function to alter the transformation.
  ///
  /// \returns the transformed type.
  QualType TransformType(QualType T);
  
  /// \brief Transform the given type by adding the given set of qualifiers
  /// and returning the result.
  ///
  /// FIXME: By default, this routine adds type qualifiers only to types that
  /// can have qualifiers, and silently suppresses those qualifiers that are
  /// not permitted (e.g., qualifiers on reference or function types). This
  /// is the right thing for template instantiation, but probably not for
  /// other clients.
  QualType AddTypeQualifiers(QualType T, unsigned CVRQualifiers);
       
  /// \brief Transform the given expression.
  ///
  /// FIXME: At the moment, subclasses must override this.
  Sema::OwningExprResult TransformExpr(Expr *E);
  
  /// \brief Transform the given declaration, which is referenced from a type
  /// or expression.
  ///
  /// Subclasses must override this.
  Decl *TransformDecl(Decl *D);
  
  /// \brief Transform the given nested-name-specifier.
  ///
  /// Subclasses must override this.
  NestedNameSpecifier *TransformNestedNameSpecifier(NestedNameSpecifier *NNS,
                                                    SourceRange Range);
  
  /// \brief Transform the given template name.
  /// 
  /// FIXME: At the moment, subclasses must override this.
  TemplateName TransformTemplateName(TemplateName Template);
  
  /// \brief Transform the given template argument.
  ///
  /// FIXME: At the moment, subclasses must override this.
  TemplateArgument TransformTemplateArgument(const TemplateArgument &Arg);
  
#define ABSTRACT_TYPE(CLASS, PARENT)
#define TYPE(CLASS, PARENT)                                   \
  QualType Transform##CLASS##Type(const CLASS##Type *T);
#include "clang/AST/TypeNodes.def"      

  /// \brief Build a new pointer type given its pointee type.
  ///
  /// By default, performs semantic analysis when building the pointer type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildPointerType(QualType PointeeType);

  /// \brief Build a new block pointer type given its pointee type.
  ///
  /// By default, performs semantic analysis when building the block pointer 
  /// type. Subclasses may override this routine to provide different behavior.
  QualType RebuildBlockPointerType(QualType PointeeType);

  /// \brief Build a new lvalue reference type given the type it references.
  ///
  /// By default, performs semantic analysis when building the lvalue reference
  /// type. Subclasses may override this routine to provide different behavior.
  QualType RebuildLValueReferenceType(QualType ReferentType);

  /// \brief Build a new rvalue reference type given the type it references.
  ///
  /// By default, performs semantic analysis when building the rvalue reference
  /// type. Subclasses may override this routine to provide different behavior.
  QualType RebuildRValueReferenceType(QualType ReferentType);
  
  /// \brief Build a new member pointer type given the pointee type and the
  /// class type it refers into.
  ///
  /// By default, performs semantic analysis when building the member pointer
  /// type. Subclasses may override this routine to provide different behavior.
  QualType RebuildMemberPointerType(QualType PointeeType, QualType ClassType);
  
  /// \brief Build a new array type given the element type, size
  /// modifier, size of the array (if known), size expression, and index type
  /// qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  /// Also by default, all of the other Rebuild*Array 
  QualType RebuildArrayType(QualType ElementType,
                            ArrayType::ArraySizeModifier SizeMod,
                            const llvm::APInt *Size,
                            Expr *SizeExpr,
                            unsigned IndexTypeQuals,
                            SourceRange BracketsRange);
  
  /// \brief Build a new constant array type given the element type, size
  /// modifier, (known) size of the array, and index type qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildConstantArrayType(QualType ElementType, 
                                    ArrayType::ArraySizeModifier SizeMod,
                                    const llvm::APInt &Size,
                                    unsigned IndexTypeQuals);

  /// \brief Build a new constant array type given the element type, size
  /// modifier, (known) size of the array, size expression, and index type 
  /// qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildConstantArrayWithExprType(QualType ElementType, 
                                            ArrayType::ArraySizeModifier SizeMod,
                                            const llvm::APInt &Size,
                                            Expr *SizeExpr,
                                            unsigned IndexTypeQuals,
                                            SourceRange BracketsRange);

  /// \brief Build a new constant array type given the element type, size
  /// modifier, (known) size of the array, and index type qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildConstantArrayWithoutExprType(QualType ElementType, 
                                               ArrayType::ArraySizeModifier SizeMod,
                                               const llvm::APInt &Size,
                                               unsigned IndexTypeQuals);

  /// \brief Build a new incomplete array type given the element type, size
  /// modifier, and index type qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildIncompleteArrayType(QualType ElementType, 
                                      ArrayType::ArraySizeModifier SizeMod,
                                      unsigned IndexTypeQuals);

  /// \brief Build a new variable-length array type given the element type, 
  /// size modifier, size expression, and index type qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildVariableArrayType(QualType ElementType, 
                                    ArrayType::ArraySizeModifier SizeMod,
                                    Sema::ExprArg SizeExpr,
                                    unsigned IndexTypeQuals,
                                    SourceRange BracketsRange);

  /// \brief Build a new dependent-sized array type given the element type, 
  /// size modifier, size expression, and index type qualifiers.
  ///
  /// By default, performs semantic analysis when building the array type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildDependentSizedArrayType(QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                          Sema::ExprArg SizeExpr,
                                          unsigned IndexTypeQuals,
                                          SourceRange BracketsRange);

  /// \brief Build a new vector type given the element type and
  /// number of elements.
  ///
  /// By default, performs semantic analysis when building the vector type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildVectorType(QualType ElementType, unsigned NumElements);
  
  /// \brief Build a new extended vector type given the element type and
  /// number of elements.
  ///
  /// By default, performs semantic analysis when building the vector type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildExtVectorType(QualType ElementType, unsigned NumElements,
                                SourceLocation AttributeLoc);
  
  /// \brief Build a new potentially dependently-sized extended vector type 
  /// given the element type and number of elements.
  ///
  /// By default, performs semantic analysis when building the vector type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildDependentSizedExtVectorType(QualType ElementType, 
                                              Sema::ExprArg SizeExpr,
                                              SourceLocation AttributeLoc);
    
  /// \brief Build a new function type.
  ///
  /// By default, performs semantic analysis when building the function type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildFunctionProtoType(QualType T,
                                    QualType *ParamTypes, 
                                    unsigned NumParamTypes,
                                    bool Variadic, unsigned Quals);
    
  /// \brief Build a new typedef type.
  QualType RebuildTypedefType(TypedefDecl *Typedef) {
    return SemaRef.Context.getTypeDeclType(Typedef);
  }

  /// \brief Build a new class/struct/union type.
  QualType RebuildRecordType(RecordDecl *Record) {
    return SemaRef.Context.getTypeDeclType(Record);
  }

  /// \brief Build a new Enum type.
  QualType RebuildEnumType(EnumDecl *Enum) {
    return SemaRef.Context.getTypeDeclType(Enum);
  }
  
  /// \brief Build a new typeof(expr) type. 
  ///
  /// By default, performs semantic analysis when building the typeof type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildTypeOfExprType(Sema::ExprArg Underlying);

  /// \brief Build a new typeof(type) type. 
  ///
  /// By default, builds a new TypeOfType with the given underlying type.
  QualType RebuildTypeOfType(QualType Underlying);

  /// \brief Build a new C++0x decltype type. 
  ///
  /// By default, performs semantic analysis when building the decltype type.
  /// Subclasses may override this routine to provide different behavior.
  QualType RebuildDecltypeType(Sema::ExprArg Underlying);
  
  /// \brief Build a new template specialization type.
  ///
  /// By default, performs semantic analysis when building the template
  /// specialization type. Subclasses may override this routine to provide
  /// different behavior.
  QualType RebuildTemplateSpecializationType(TemplateName Template,
                                             const TemplateArgument *Args,
                                             unsigned NumArgs);
  
  /// \brief Build a new qualified name type.
  ///
  /// By default, builds a new QualifiedNameType type from the 
  /// nested-name-specifier and the named type. Subclasses may override 
  /// this routine to provide different behavior.
  QualType RebuildQualifiedNameType(NestedNameSpecifier *NNS, QualType Named) {
    return SemaRef.Context.getQualifiedNameType(NNS, Named);
  }  

  /// \brief Build a new typename type that refers to a template-id.
  ///
  /// By default, builds a new TypenameType type from the nested-name-specifier
  /// and the given type. Subclasses may override this routine to provide 
  /// different behavior.
  QualType RebuildTypenameType(NestedNameSpecifier *NNS, QualType T) {
    if (NNS->isDependent())
      return SemaRef.Context.getTypenameType(NNS, 
                                          cast<TemplateSpecializationType>(T));
    
    return SemaRef.Context.getQualifiedNameType(NNS, T);
  }  

  /// \brief Build a new typename type that refers to an identifier.
  ///
  /// By default, performs semantic analysis when building the typename type
  /// (or qualified name type). Subclasses may override this routine to provide 
  /// different behavior.
  QualType RebuildTypenameType(NestedNameSpecifier *NNS, 
                               const IdentifierInfo *Id) {
    return SemaRef.CheckTypenameType(NNS, *Id,
                                  SourceRange(getDerived().getBaseLocation()));
  }    
};
  
//===----------------------------------------------------------------------===//
// Type transformation
//===----------------------------------------------------------------------===//

template<typename Derived>
QualType TreeTransform<Derived>::TransformType(QualType T) {
  if (getDerived().AlreadyTransformed(T))
    return T;
  
  QualType Result;
  switch (T->getTypeClass()) {
#define ABSTRACT_TYPE(CLASS, PARENT) 
#define TYPE(CLASS, PARENT)                                                  \
    case Type::CLASS:                                                        \
      Result = getDerived().Transform##CLASS##Type(                          \
                                  static_cast<CLASS##Type*>(T.getTypePtr())); \
      break;
#include "clang/AST/TypeNodes.def"      
  }
  
  if (Result.isNull() || T == Result)
    return Result;
  
  return getDerived().AddTypeQualifiers(Result, T.getCVRQualifiers());
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::AddTypeQualifiers(QualType T, unsigned CVRQualifiers) {
  if (CVRQualifiers && !T->isFunctionType() && !T->isReferenceType())
    return T.getWithAdditionalQualifiers(CVRQualifiers);
  
  return T;
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformExtQualType(const ExtQualType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformBuiltinType(const BuiltinType *T) { 
  // Nothing to do
  return QualType(T, 0); 
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformFixedWidthIntType(
                                                  const FixedWidthIntType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformComplexType(const ComplexType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformPointerType(const PointerType *T) {
  QualType PointeeType = getDerived().TransformType(T->getPointeeType());
  if (PointeeType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      PointeeType == T->getPointeeType())
    return QualType(T, 0);

  return getDerived().RebuildPointerType(PointeeType);
}
  
template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformBlockPointerType(const BlockPointerType *T) { 
  QualType PointeeType = getDerived().TransformType(T->getPointeeType());
  if (PointeeType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      PointeeType == T->getPointeeType())
    return QualType(T, 0);
  
  return getDerived().RebuildBlockPointerType(PointeeType);
}

template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformLValueReferenceType(
                                               const LValueReferenceType *T) { 
  QualType PointeeType = getDerived().TransformType(T->getPointeeType());
  if (PointeeType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      PointeeType == T->getPointeeType())
    return QualType(T, 0);
  
  return getDerived().RebuildLValueReferenceType(PointeeType);
}

template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformRValueReferenceType(
                                              const RValueReferenceType *T) { 
  QualType PointeeType = getDerived().TransformType(T->getPointeeType());
  if (PointeeType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      PointeeType == T->getPointeeType())
    return QualType(T, 0);
  
  return getDerived().RebuildRValueReferenceType(PointeeType);
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::TransformMemberPointerType(const MemberPointerType *T) { 
  QualType PointeeType = getDerived().TransformType(T->getPointeeType());
  if (PointeeType.isNull())
    return QualType();
  
  QualType ClassType = getDerived().TransformType(QualType(T->getClass(), 0));
  if (ClassType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      PointeeType == T->getPointeeType() &&
      ClassType == QualType(T->getClass(), 0))
    return QualType(T, 0);

  return getDerived().RebuildMemberPointerType(PointeeType, ClassType);
}

template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformConstantArrayType(const ConstantArrayType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);
  
  return getDerived().RebuildConstantArrayType(ElementType, 
                                               T->getSizeModifier(),
                                               T->getSize(),
                                               T->getIndexTypeQualifier());
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::TransformConstantArrayWithExprType(
                                      const ConstantArrayWithExprType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);
  
  return getDerived().RebuildConstantArrayWithExprType(ElementType, 
                                                       T->getSizeModifier(),
                                                       T->getSize(),
                                        /*FIXME: Transform?*/T->getSizeExpr(),
                                                   T->getIndexTypeQualifier(),
                                                       T->getBracketsRange());
}
  
template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformConstantArrayWithoutExprType(
                                      const ConstantArrayWithoutExprType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);
  
  return getDerived().RebuildConstantArrayWithoutExprType(ElementType, 
                                                       T->getSizeModifier(),
                                                       T->getSize(),
                                                    T->getIndexTypeQualifier());
}

template<typename Derived> 
QualType TreeTransform<Derived>::TransformIncompleteArrayType(
                                              const IncompleteArrayType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);

  return getDerived().RebuildIncompleteArrayType(ElementType,
                                                 T->getSizeModifier(),
                                                 T->getIndexTypeQualifier());
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformVariableArrayType(
                                                  const VariableArrayType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  Sema::OwningExprResult Size = getDerived().TransformExpr(T->getSizeExpr());
  if (Size.isInvalid())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType() &&
      Size.get() == T->getSizeExpr()) {
    Size.take();
    return QualType(T, 0);
  }
  
  return getDerived().RebuildVariableArrayType(ElementType, 
                                               T->getSizeModifier(),
                                               move(Size),
                                               T->getIndexTypeQualifier(),
                                               T->getBracketsRange());
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformDependentSizedArrayType(
                                          const DependentSizedArrayType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  Sema::OwningExprResult Size = getDerived().TransformExpr(T->getSizeExpr());
  if (Size.isInvalid())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType() &&
      Size.get() == T->getSizeExpr()) {
    Size.take();
    return QualType(T, 0);
  }
  
  return getDerived().RebuildDependentSizedArrayType(ElementType, 
                                                     T->getSizeModifier(),
                                                     move(Size),
                                                     T->getIndexTypeQualifier(),
                                                     T->getBracketsRange());
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformDependentSizedExtVectorType(
                                      const DependentSizedExtVectorType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  Sema::OwningExprResult Size = getDerived().TransformExpr(T->getSizeExpr());
  if (Size.isInvalid())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType() &&
      Size.get() == T->getSizeExpr()) {
    Size.take();
    return QualType(T, 0);
  }
  
  return getDerived().RebuildDependentSizedExtVectorType(ElementType, 
                                                         move(Size),
                                                         T->getAttributeLoc());
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformVectorType(const VectorType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();

  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);
  
  return getDerived().RebuildVectorType(ElementType, T->getNumElements());
}
  
template<typename Derived> 
QualType 
TreeTransform<Derived>::TransformExtVectorType(const ExtVectorType *T) { 
  QualType ElementType = getDerived().TransformType(T->getElementType());
  if (ElementType.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      ElementType == T->getElementType())
    return QualType(T, 0);
  
  return getDerived().RebuildExtVectorType(ElementType, T->getNumElements(),
                                           /*FIXME*/SourceLocation());
}

template<typename Derived> 
QualType TreeTransform<Derived>::TransformFunctionProtoType(
                                                  const FunctionProtoType *T) { 
  QualType ResultType = getDerived().TransformType(T->getResultType());
  if (ResultType.isNull())
    return QualType();
  
  llvm::SmallVector<QualType, 4> ParamTypes;
  for (FunctionProtoType::arg_type_iterator Param = T->arg_type_begin(),
                                         ParamEnd = T->arg_type_end(); 
       Param != ParamEnd; ++Param) {
    QualType P = getDerived().TransformType(*Param);
    if (P.isNull())
      return QualType();
    
    ParamTypes.push_back(P);
  }
  
  if (!getDerived().AlwaysRebuild() &&
      ResultType == T->getResultType() &&
      std::equal(T->arg_type_begin(), T->arg_type_end(), ParamTypes.begin()))
    return QualType(T, 0);
  
  return getDerived().RebuildFunctionProtoType(ResultType, ParamTypes.data(), 
                                               ParamTypes.size(), T->isVariadic(),
                                               T->getTypeQuals());
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformFunctionNoProtoType(
                                                const FunctionNoProtoType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformTypedefType(const TypedefType *T) { 
  TypedefDecl *Typedef
    = cast_or_null<TypedefDecl>(getDerived().TransformDecl(T->getDecl()));
  if (!Typedef)
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      Typedef == T->getDecl())
    return QualType(T, 0);
  
  return getDerived().RebuildTypedefType(Typedef);
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformTypeOfExprType(
                                                    const TypeOfExprType *T) { 
  Sema::OwningExprResult E = getDerived().TransformExpr(T->getUnderlyingExpr());
  if (E.isInvalid())
    return QualType();

  if (!getDerived().AlwaysRebuild() &&
      E.get() == T->getUnderlyingExpr()) {
    E.take();
    return QualType(T, 0);
  }
  
  return getDerived().RebuildTypeOfExprType(move(E));
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformTypeOfType(const TypeOfType *T) { 
  QualType Underlying = getDerived().TransformType(T->getUnderlyingType());
  if (Underlying.isNull())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      Underlying == T->getUnderlyingType())
    return QualType(T, 0);
  
  return getDerived().RebuildTypeOfType(Underlying);
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformDecltypeType(const DecltypeType *T) { 
  Sema::OwningExprResult E = getDerived().TransformExpr(T->getUnderlyingExpr());
  if (E.isInvalid())
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      E.get() == T->getUnderlyingExpr()) {
    E.take();
    return QualType(T, 0);
  }
  
  return getDerived().RebuildDecltypeType(move(E));
}

template<typename Derived>
QualType TreeTransform<Derived>::TransformRecordType(const RecordType *T) { 
  RecordDecl *Record
  = cast_or_null<RecordDecl>(getDerived().TransformDecl(T->getDecl()));
  if (!Record)
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      Record == T->getDecl())
    return QualType(T, 0);
  
  return getDerived().RebuildRecordType(Record);
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformEnumType(const EnumType *T) { 
  EnumDecl *Enum
  = cast_or_null<EnumDecl>(getDerived().TransformDecl(T->getDecl()));
  if (!Enum)
    return QualType();
  
  if (!getDerived().AlwaysRebuild() &&
      Enum == T->getDecl())
    return QualType(T, 0);
  
  return getDerived().RebuildEnumType(Enum);
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformTemplateTypeParmType(
                                              const TemplateTypeParmType *T) { 
  // Nothing to do
  return QualType(T, 0); 
}

template<typename Derived> 
QualType TreeTransform<Derived>::TransformTemplateSpecializationType(
                                        const TemplateSpecializationType *T) { 
  TemplateName Template 
    = getDerived().TransformTemplateName(T->getTemplateName());
  if (Template.isNull())
    return QualType();
  
  llvm::SmallVector<TemplateArgument, 4> NewTemplateArgs;
  NewTemplateArgs.reserve(T->getNumArgs());
  for (TemplateSpecializationType::iterator Arg = T->begin(), ArgEnd = T->end();
       Arg != ArgEnd; ++Arg) {
    TemplateArgument NewArg = getDerived().TransformTemplateArgument(*Arg);
    if (NewArg.isNull())
      return QualType();
    
    NewTemplateArgs.push_back(NewArg);
  }
  
  // FIXME: early abort if all of the template arguments and such are the
  // same.
  
  // FIXME: We're missing the locations of the template name, '<', and '>'.
  return getDerived().RebuildTemplateSpecializationType(Template,
                                                        NewTemplateArgs.data(),
                                                        NewTemplateArgs.size());
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformQualifiedNameType(
                                                  const QualifiedNameType *T) {
  NestedNameSpecifier *NNS
    = getDerived().TransformNestedNameSpecifier(T->getQualifier(),
                                                SourceRange());
  if (!NNS)
    return QualType();
  
  QualType Named = getDerived().TransformType(T->getNamedType());
  if (Named.isNull())
    return QualType();
                      
  if (!getDerived().AlwaysRebuild() &&
      NNS == T->getQualifier() &&
      Named == T->getNamedType())
    return QualType(T, 0);

  return getDerived().RebuildQualifiedNameType(NNS, Named);
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformTypenameType(const TypenameType *T) {
  NestedNameSpecifier *NNS
    = getDerived().TransformNestedNameSpecifier(T->getQualifier(),
                                SourceRange(getDerived().getBaseLocation()));
  if (!NNS)
    return QualType();
  
  if (const TemplateSpecializationType *TemplateId = T->getTemplateId()) {
    QualType NewTemplateId 
      = getDerived().TransformType(QualType(TemplateId, 0));
    if (NewTemplateId.isNull())
      return QualType();
    
    if (!getDerived().AlwaysRebuild() &&
        NNS == T->getQualifier() &&
        NewTemplateId == QualType(TemplateId, 0))
      return QualType(T, 0);
    
    return getDerived().RebuildTypenameType(NNS, NewTemplateId);
  }

  return getDerived().RebuildTypenameType(NNS, T->getIdentifier());
}
  
template<typename Derived>
QualType TreeTransform<Derived>::TransformObjCInterfaceType(
                                                  const ObjCInterfaceType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}
  
template<typename Derived> 
QualType TreeTransform<Derived>::TransformObjCObjectPointerType(
                                             const ObjCObjectPointerType *T) { 
  // FIXME: Implement
  return QualType(T, 0); 
}

//===----------------------------------------------------------------------===//
// Type reconstruction
//===----------------------------------------------------------------------===//

template<typename Derived> 
QualType TreeTransform<Derived>::RebuildPointerType(QualType PointeeType) {
  return SemaRef.BuildPointerType(PointeeType, 0, 
                                  getDerived().getBaseLocation(),
                                  getDerived().getBaseEntity());
}

template<typename Derived> 
QualType TreeTransform<Derived>::RebuildBlockPointerType(QualType PointeeType) {
  return SemaRef.BuildBlockPointerType(PointeeType, 0, 
                                       getDerived().getBaseLocation(),
                                       getDerived().getBaseEntity());
}

template<typename Derived> 
QualType 
TreeTransform<Derived>::RebuildLValueReferenceType(QualType ReferentType) {
  return SemaRef.BuildReferenceType(ReferentType, true, 0, 
                                    getDerived().getBaseLocation(),
                                    getDerived().getBaseEntity());
}

template<typename Derived> 
QualType 
TreeTransform<Derived>::RebuildRValueReferenceType(QualType ReferentType) {
  return SemaRef.BuildReferenceType(ReferentType, false, 0, 
                                    getDerived().getBaseLocation(),
                                    getDerived().getBaseEntity());
}

template<typename Derived>
QualType TreeTransform<Derived>::RebuildMemberPointerType(QualType PointeeType, 
                                                          QualType ClassType) {
  return SemaRef.BuildMemberPointerType(PointeeType, ClassType, 0, 
                                        getDerived().getBaseLocation(),
                                        getDerived().getBaseEntity());
}

template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildArrayType(QualType ElementType,
                                         ArrayType::ArraySizeModifier SizeMod,
                                         const llvm::APInt *Size,
                                         Expr *SizeExpr,
                                         unsigned IndexTypeQuals,
                                         SourceRange BracketsRange) {
  if (SizeExpr || !Size)
    return SemaRef.BuildArrayType(ElementType, SizeMod, SizeExpr,
                                  IndexTypeQuals, BracketsRange,
                                  getDerived().getBaseEntity());
  
  QualType Types[] = { 
    SemaRef.Context.UnsignedCharTy, SemaRef.Context.UnsignedShortTy, 
    SemaRef.Context.UnsignedIntTy, SemaRef.Context.UnsignedLongTy, 
    SemaRef.Context.UnsignedLongLongTy, SemaRef.Context.UnsignedInt128Ty 
  };
  const unsigned NumTypes = sizeof(Types) / sizeof(QualType);
  QualType SizeType;
  for (unsigned I = 0; I != NumTypes; ++I)
    if (Size->getBitWidth() == SemaRef.Context.getIntWidth(Types[I])) {
      SizeType = Types[I];
      break;
    }
  
  if (SizeType.isNull())
    SizeType = SemaRef.Context.getFixedWidthIntType(Size->getBitWidth(), false);
  
  IntegerLiteral ArraySize(*Size, SizeType, /*FIXME*/BracketsRange.getBegin());
  return SemaRef.BuildArrayType(ElementType, SizeMod, &ArraySize, 
                                IndexTypeQuals, BracketsRange,
                                getDerived().getBaseEntity());    
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildConstantArrayType(QualType ElementType, 
                                                 ArrayType::ArraySizeModifier SizeMod,
                                                 const llvm::APInt &Size,
                                                 unsigned IndexTypeQuals) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, &Size, 0, 
                                        IndexTypeQuals, SourceRange());
}

template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildConstantArrayWithExprType(QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                                      const llvm::APInt &Size,
                                                         Expr *SizeExpr,
                                                      unsigned IndexTypeQuals,
                                                    SourceRange BracketsRange) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, &Size, SizeExpr, 
                                       IndexTypeQuals, BracketsRange);
}

template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildConstantArrayWithoutExprType(
                                                        QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                                       const llvm::APInt &Size,
                                                     unsigned IndexTypeQuals) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, &Size, 0, 
                                       IndexTypeQuals, SourceRange());
}

template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildIncompleteArrayType(QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                                 unsigned IndexTypeQuals) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, 0, 0, 
                                       IndexTypeQuals, SourceRange());
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildVariableArrayType(QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                                 Sema::ExprArg SizeExpr,
                                                 unsigned IndexTypeQuals,
                                                 SourceRange BracketsRange) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, 0, 
                                       SizeExpr.takeAs<Expr>(),
                                       IndexTypeQuals, BracketsRange);
}

template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildDependentSizedArrayType(QualType ElementType, 
                                          ArrayType::ArraySizeModifier SizeMod,
                                                       Sema::ExprArg SizeExpr,
                                                       unsigned IndexTypeQuals,
                                                   SourceRange BracketsRange) {
  return getDerived().RebuildArrayType(ElementType, SizeMod, 0, 
                                       SizeExpr.takeAs<Expr>(),
                                       IndexTypeQuals, BracketsRange);
}

template<typename Derived>
QualType TreeTransform<Derived>::RebuildVectorType(QualType ElementType,
                                                   unsigned NumElements) {
  // FIXME: semantic checking!
  return SemaRef.Context.getVectorType(ElementType, NumElements);
}
  
template<typename Derived>
QualType TreeTransform<Derived>::RebuildExtVectorType(QualType ElementType,
                                                      unsigned NumElements,
                                                 SourceLocation AttributeLoc) {
  llvm::APInt numElements(SemaRef.Context.getIntWidth(SemaRef.Context.IntTy),
                          NumElements, true);
  IntegerLiteral *VectorSize
    = new (SemaRef.Context) IntegerLiteral(numElements, SemaRef.Context.IntTy, 
                                           AttributeLoc);
  return SemaRef.BuildExtVectorType(ElementType, SemaRef.Owned(VectorSize),
                                    AttributeLoc);
}
  
template<typename Derived>
QualType 
TreeTransform<Derived>::RebuildDependentSizedExtVectorType(QualType ElementType, 
                                                         Sema::ExprArg SizeExpr,
                                                  SourceLocation AttributeLoc) {
  return SemaRef.BuildExtVectorType(ElementType, move(SizeExpr), AttributeLoc);
}
 
template<typename Derived>
QualType TreeTransform<Derived>::RebuildFunctionProtoType(QualType T,
                                                          QualType *ParamTypes, 
                                                        unsigned NumParamTypes,
                                                          bool Variadic, 
                                                          unsigned Quals) {
  return SemaRef.BuildFunctionType(T, ParamTypes, NumParamTypes, Variadic, 
                                   Quals,
                                   getDerived().getBaseLocation(),
                                   getDerived().getBaseEntity());
}
  
template<typename Derived>
QualType TreeTransform<Derived>::RebuildTypeOfExprType(Sema::ExprArg E) {
  return SemaRef.BuildTypeofExprType(E.takeAs<Expr>());
}

template<typename Derived>
QualType TreeTransform<Derived>::RebuildTypeOfType(QualType Underlying) {
  return SemaRef.Context.getTypeOfType(Underlying);
}

template<typename Derived>
QualType TreeTransform<Derived>::RebuildDecltypeType(Sema::ExprArg E) {
  return SemaRef.BuildDecltypeType(E.takeAs<Expr>());
}

template<typename Derived>
QualType TreeTransform<Derived>::RebuildTemplateSpecializationType(
                                                        TemplateName Template,
                                                 const TemplateArgument *Args,
                                                           unsigned NumArgs) {
  // FIXME: Missing source locations for the template name, <, >.
  return SemaRef.CheckTemplateIdType(Template, getDerived().getBaseLocation(),
                                     SourceLocation(), Args, NumArgs, 
                                     SourceLocation());  
}
  
} // end namespace clang

#endif // LLVM_CLANG_SEMA_TREETRANSFORM_H

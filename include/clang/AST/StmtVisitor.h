//===--- StmtVisitor.h - Visitor for Stmt subclasses ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Chris Lattner and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the StmtVisitor interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_STMTVISITOR_H
#define LLVM_CLANG_AST_STMTVISITOR_H

#include "clang/AST/ExprCXX.h"

namespace clang {
  
#define DISPATCH(NAME, CLASS) \
  return static_cast<ImplClass*>(this)->Visit ## NAME(static_cast<CLASS*>(S))
  
/// StmtVisitor - This class implements a simple visitor for Stmt subclasses.
/// Since Expr derives from Stmt, this also includes support for visiting Exprs.
template<typename ImplClass, typename RetTy=void>
class StmtVisitor {
public:
  RetTy Visit(Stmt *S) {
    
    // If we have a binary expr, dispatch to the subcode of the binop.  A smart
    // optimizer (e.g. LLVM) will fold this comparison into the switch stmt
    // below.
    if (BinaryOperator *BinOp = dyn_cast<BinaryOperator>(S)) {
      switch (BinOp->getOpcode()) {
      default: assert(0 && "Unknown binary operator!");
      case BinaryOperator::Mul:       DISPATCH(BinMul,       BinaryOperator);
      case BinaryOperator::Div:       DISPATCH(BinDiv,       BinaryOperator);
      case BinaryOperator::Rem:       DISPATCH(BinRem,       BinaryOperator);
      case BinaryOperator::Add:       DISPATCH(BinAdd,       BinaryOperator);
      case BinaryOperator::Sub:       DISPATCH(BinSub,       BinaryOperator);
      case BinaryOperator::Shl:       DISPATCH(BinShl,       BinaryOperator);
      case BinaryOperator::Shr:       DISPATCH(BinShr,       BinaryOperator);

      case BinaryOperator::LT:        DISPATCH(BinLT,        BinaryOperator);
      case BinaryOperator::GT:        DISPATCH(BinGT,        BinaryOperator);
      case BinaryOperator::LE:        DISPATCH(BinLE,        BinaryOperator);
      case BinaryOperator::GE:        DISPATCH(BinGE,        BinaryOperator);
      case BinaryOperator::EQ:        DISPATCH(BinEQ,        BinaryOperator);
      case BinaryOperator::NE:        DISPATCH(BinNE,        BinaryOperator);
        
      case BinaryOperator::And:       DISPATCH(BinAnd,       BinaryOperator);
      case BinaryOperator::Xor:       DISPATCH(BinXor,       BinaryOperator);
      case BinaryOperator::Or :       DISPATCH(BinOr,        BinaryOperator);
      case BinaryOperator::LAnd:      DISPATCH(BinLAnd,      BinaryOperator);
      case BinaryOperator::LOr :      DISPATCH(BinLOr,       BinaryOperator);
      case BinaryOperator::Assign:    DISPATCH(BinAssign,    BinaryOperator);
      case BinaryOperator::MulAssign: DISPATCH(BinMulAssign, BinaryOperator);
      case BinaryOperator::DivAssign: DISPATCH(BinDivAssign, BinaryOperator);
      case BinaryOperator::RemAssign: DISPATCH(BinRemAssign, BinaryOperator);
      case BinaryOperator::AddAssign: DISPATCH(BinAddAssign, BinaryOperator);
      case BinaryOperator::SubAssign: DISPATCH(BinSubAssign, BinaryOperator);
      case BinaryOperator::ShlAssign: DISPATCH(BinShlAssign, BinaryOperator);
      case BinaryOperator::ShrAssign: DISPATCH(BinShrAssign, BinaryOperator);
      case BinaryOperator::AndAssign: DISPATCH(BinAndAssign, BinaryOperator);
      case BinaryOperator::OrAssign:  DISPATCH(BinOrAssign,  BinaryOperator);
      case BinaryOperator::XorAssign: DISPATCH(BinXorAssign, BinaryOperator);
      case BinaryOperator::Comma:     DISPATCH(BinComma,     BinaryOperator);
      }
    } else if (UnaryOperator *UnOp = dyn_cast<UnaryOperator>(S)) {
      switch (UnOp->getOpcode()) {
      default: assert(0 && "Unknown unary operator!");
      case UnaryOperator::PostInc:      DISPATCH(UnaryPostInc,   UnaryOperator);
      case UnaryOperator::PostDec:      DISPATCH(UnaryPostDec,   UnaryOperator);
      case UnaryOperator::PreInc:       DISPATCH(UnaryPreInc,    UnaryOperator);
      case UnaryOperator::PreDec:       DISPATCH(UnaryPreDec,    UnaryOperator);
      case UnaryOperator::AddrOf:       DISPATCH(UnaryAddrOf,    UnaryOperator);
      case UnaryOperator::Deref:        DISPATCH(UnaryDeref,     UnaryOperator);
      case UnaryOperator::Plus:         DISPATCH(UnaryPlus,      UnaryOperator);
      case UnaryOperator::Minus:        DISPATCH(UnaryMinus,     UnaryOperator);
      case UnaryOperator::Not:          DISPATCH(UnaryNot,       UnaryOperator);
      case UnaryOperator::LNot:         DISPATCH(UnaryLNot,      UnaryOperator);
      case UnaryOperator::SizeOf:       DISPATCH(UnarySizeOf,    UnaryOperator);
      case UnaryOperator::AlignOf:      DISPATCH(UnaryAlignOf,   UnaryOperator);
      case UnaryOperator::Real:         DISPATCH(UnaryReal,      UnaryOperator);
      case UnaryOperator::Imag:         DISPATCH(UnaryImag,      UnaryOperator);
      case UnaryOperator::Extension:    DISPATCH(UnaryExtension, UnaryOperator);
      }          
    }
    
    // Top switch stmt: dispatch to VisitFooStmt for each FooStmt.
    switch (S->getStmtClass()) {
    default: assert(0 && "Unknown stmt kind!");
#define STMT(N, CLASS, PARENT)                              \
    case Stmt::CLASS ## Class: DISPATCH(CLASS, CLASS);
#include "clang/AST/StmtNodes.def"
    }
  }
  
  // If the implementation chooses not to implement a certain visit method, fall
  // back on VisitExpr or whatever else is the superclass.
#define STMT(N, CLASS, PARENT)                                   \
  RetTy Visit ## CLASS(CLASS *S) { DISPATCH(PARENT, PARENT); }
#include "clang/AST/StmtNodes.def"

  // If the implementation doesn't implement binary operator methods, fall back
  // on VisitBinaryOperator.
#define BINOP_FALLBACK(NAME) \
  RetTy VisitBin ## NAME(BinaryOperator *S) { \
    DISPATCH(BinaryOperator, BinaryOperator); \
  }
  BINOP_FALLBACK(Mul)   BINOP_FALLBACK(Div)  BINOP_FALLBACK(Rem)
  BINOP_FALLBACK(Add)   BINOP_FALLBACK(Sub)  BINOP_FALLBACK(Shl)
  BINOP_FALLBACK(Shr)
  
  BINOP_FALLBACK(LT)    BINOP_FALLBACK(GT)   BINOP_FALLBACK(LE)
  BINOP_FALLBACK(GE)    BINOP_FALLBACK(EQ)   BINOP_FALLBACK(NE)
  BINOP_FALLBACK(And)   BINOP_FALLBACK(Xor)  BINOP_FALLBACK(Or)
  BINOP_FALLBACK(LAnd)  BINOP_FALLBACK(LOr)

  BINOP_FALLBACK(Assign)
  BINOP_FALLBACK(MulAssign) BINOP_FALLBACK(DivAssign) BINOP_FALLBACK(RemAssign)
  BINOP_FALLBACK(AddAssign) BINOP_FALLBACK(SubAssign) BINOP_FALLBACK(ShlAssign)
  BINOP_FALLBACK(ShrAssign) BINOP_FALLBACK(AndAssign) BINOP_FALLBACK(OrAssign)
  BINOP_FALLBACK(XorAssign)
  
  BINOP_FALLBACK(Comma)
#undef BINOP_FALLBACK
  
  // If the implementation doesn't implement unary operator methods, fall back
  // on VisitUnaryOperator.
#define UNARYOP_FALLBACK(NAME) \
  RetTy VisitUnary ## NAME(UnaryOperator *S) { \
    DISPATCH(UnaryOperator, UnaryOperator);    \
  }
  UNARYOP_FALLBACK(PostInc)   UNARYOP_FALLBACK(PostDec)
  UNARYOP_FALLBACK(PreInc)    UNARYOP_FALLBACK(PreDec)
  UNARYOP_FALLBACK(AddrOf)    UNARYOP_FALLBACK(Deref)
  
  UNARYOP_FALLBACK(Plus)      UNARYOP_FALLBACK(Minus)
  UNARYOP_FALLBACK(Not)       UNARYOP_FALLBACK(LNot)
  UNARYOP_FALLBACK(SizeOf)    UNARYOP_FALLBACK(AlignOf)
  UNARYOP_FALLBACK(Real)      UNARYOP_FALLBACK(Imag)
  UNARYOP_FALLBACK(Extension)
#undef UNARYOP_FALLBACK
  
  // Base case, ignore it. :)
  RetTy VisitStmt(Stmt *Node) { return RetTy(); }
};

#undef DISPATCH

}  // end namespace clang

#endif

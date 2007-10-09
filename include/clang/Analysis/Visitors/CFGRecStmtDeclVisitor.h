//= CFGRecStmtDeclVisitor - Recursive visitor of CFG stmts/decls -*- C++ --*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by Ted Kremenek and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the template class CFGRecStmtDeclVisitor, which extends
// CFGRecStmtVisitor by implementing (typed) visitation of decls.
//
// FIXME: This may not be fully complete.  We currently explore only subtypes
//        of ScopedDecl.
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_CFG_REC_STMT_DECL_VISITOR_H
#define LLVM_CLANG_ANALYSIS_CFG_REC_STMT_DECL_VISITOR_H

#include "clang/Analysis/Visitors/CFGRecStmtVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclObjC.h"

#define DISPATCH_CASE(CASE,CLASS) \
case Decl::CASE: \
static_cast<ImplClass*>(this)->Visit##CLASS(static_cast<CLASS*>(D));\
break;

#define DEFAULT_DISPATCH(CLASS) void Visit##CLASS(CLASS* D) {}
#define DEFAULT_DISPATCH_VARDECL(CLASS) void Visit##CLASS(CLASS* D)\
  { static_cast<ImplClass*>(this)->VisitVarDecl(D); }

  
namespace clang {
template <typename ImplClass>
class CFGRecStmtDeclVisitor : public CFGRecStmtVisitor<ImplClass> {
public:  

  void VisitDeclRefExpr(DeclRefExpr* DR) {
    for (ScopedDecl* D = DR->getDecl(); D != NULL; D = D->getNextDeclarator())
      static_cast<ImplClass*>(this)->VisitScopedDecl(D); 
  }
  
  void VisitDeclStmt(DeclStmt* DS) {
    for (ScopedDecl* D = DS->getDecl(); D != NULL; D = D->getNextDeclarator()) {
      static_cast<ImplClass*>(this)->VisitScopedDecl(D); 
      // Visit the initializer.
      if (VarDecl* VD = dyn_cast<VarDecl>(D))
        if (Expr* I = VD->getInit())
          static_cast<ImplClass*>(this)->Visit(I);
    }
  }
    
  void VisitScopedDecl(ScopedDecl* D) {
    switch (D->getKind()) {
        DISPATCH_CASE(Function,FunctionDecl)
        DISPATCH_CASE(BlockVar,BlockVarDecl) // FIXME:Refine. VisitVarDecl?
        DISPATCH_CASE(FileVar,FileVarDecl)   // FIXME: (same)
        DISPATCH_CASE(ParmVar,ParmVarDecl)       // FIXME: (same)
        DISPATCH_CASE(EnumConstant,EnumConstantDecl)
        DISPATCH_CASE(Typedef,TypedefDecl)
        DISPATCH_CASE(Struct,RecordDecl)    // FIXME: Refine.  VisitStructDecl?
        DISPATCH_CASE(Union,RecordDecl)     // FIXME: Refine.
        DISPATCH_CASE(Class,RecordDecl)     // FIXME: Refine. 
        DISPATCH_CASE(Enum,EnumDecl)
        DISPATCH_CASE(ObjcInterface,ObjcInterfaceDecl)
      default:
        assert(false && "Subtype of ScopedDecl not handled.");
    }
  }
  
  DEFAULT_DISPATCH(VarDecl)
  DEFAULT_DISPATCH(FunctionDecl)
  DEFAULT_DISPATCH_VARDECL(BlockVarDecl)
  DEFAULT_DISPATCH_VARDECL(FileVarDecl)
  DEFAULT_DISPATCH_VARDECL(ParmVarDecl)
  DEFAULT_DISPATCH(EnumConstantDecl)
  DEFAULT_DISPATCH(TypedefDecl)
  DEFAULT_DISPATCH(RecordDecl)
  DEFAULT_DISPATCH(EnumDecl)
  DEFAULT_DISPATCH(ObjcInterfaceDecl)
  DEFAULT_DISPATCH(ObjcClassDecl)
  DEFAULT_DISPATCH(ObjcMethodDecl)
  DEFAULT_DISPATCH(ObjcProtocolDecl)
  DEFAULT_DISPATCH(ObjcCategoryDecl)
};

} // end namespace clang

#undef DISPATCH_CASE
#undef DEFAULT_DISPATCH
#endif

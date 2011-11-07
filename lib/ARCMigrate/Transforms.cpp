//===--- Tranforms.cpp - Tranformations to ARC mode -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Transforms.h"
#include "Internals.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Analysis/DomainSpecific/CocoaConventions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/DenseSet.h"
#include <map>

using namespace clang;
using namespace arcmt;
using namespace trans;

ASTTraverser::~ASTTraverser() { }

//===----------------------------------------------------------------------===//
// Helpers.
//===----------------------------------------------------------------------===//

/// \brief True if the class is one that does not support weak.
static bool isClassInWeakBlacklist(ObjCInterfaceDecl *cls) {
  if (!cls)
    return false;

  bool inList = llvm::StringSwitch<bool>(cls->getName())
                 .Case("NSColorSpace", true)
                 .Case("NSFont", true)
                 .Case("NSFontPanel", true)
                 .Case("NSImage", true)
                 .Case("NSLazyBrowserCell", true)
                 .Case("NSWindow", true)
                 .Case("NSWindowController", true)
                 .Case("NSMenuView", true)
                 .Case("NSPersistentUIWindowInfo", true)
                 .Case("NSTableCellView", true)
                 .Case("NSATSTypeSetter", true)
                 .Case("NSATSGlyphStorage", true)
                 .Case("NSLineFragmentRenderingContext", true)
                 .Case("NSAttributeDictionary", true)
                 .Case("NSParagraphStyle", true)
                 .Case("NSTextTab", true)
                 .Case("NSSimpleHorizontalTypesetter", true)
                 .Case("_NSCachedAttributedString", true)
                 .Case("NSStringDrawingTextStorage", true)
                 .Case("NSTextView", true)
                 .Case("NSSubTextStorage", true)
                 .Default(false);

  if (inList)
    return true;

  return isClassInWeakBlacklist(cls->getSuperClass());
}

bool trans::canApplyWeak(ASTContext &Ctx, QualType type,
                         bool AllowOnUnknownClass) {
  if (!Ctx.getLangOptions().ObjCRuntimeHasWeak)
    return false;

  QualType T = type;
  if (T.isNull())
    return false;

  while (const PointerType *ptr = T->getAs<PointerType>())
    T = ptr->getPointeeType();
  if (const ObjCObjectPointerType *ObjT = T->getAs<ObjCObjectPointerType>()) {
    ObjCInterfaceDecl *Class = ObjT->getInterfaceDecl();
    if (!AllowOnUnknownClass && (!Class || Class->getName() == "NSObject"))
      return false; // id/NSObject is not safe for weak.
    if (!AllowOnUnknownClass && Class->isForwardDecl())
      return false; // forward classes are not verifiable, therefore not safe.
    if (Class->isArcWeakrefUnavailable())
      return false;
    if (isClassInWeakBlacklist(Class))
      return false;
  }

  return true;
}

/// \brief 'Loc' is the end of a statement range. This returns the location
/// immediately after the semicolon following the statement.
/// If no semicolon is found or the location is inside a macro, the returned
/// source location will be invalid.
SourceLocation trans::findLocationAfterSemi(SourceLocation loc,
                                            ASTContext &Ctx) {
  SourceLocation SemiLoc = findSemiAfterLocation(loc, Ctx);
  if (SemiLoc.isInvalid())
    return SourceLocation();
  return SemiLoc.getLocWithOffset(1);
}

/// \brief \arg Loc is the end of a statement range. This returns the location
/// of the semicolon following the statement.
/// If no semicolon is found or the location is inside a macro, the returned
/// source location will be invalid.
SourceLocation trans::findSemiAfterLocation(SourceLocation loc,
                                            ASTContext &Ctx) {
  SourceManager &SM = Ctx.getSourceManager();
  if (loc.isMacroID()) {
    if (!Lexer::isAtEndOfMacroExpansion(loc, SM, Ctx.getLangOptions()))
      return SourceLocation();
    loc = SM.getExpansionRange(loc).second;
  }
  loc = Lexer::getLocForEndOfToken(loc, /*Offset=*/0, SM, Ctx.getLangOptions());

  // Break down the source location.
  std::pair<FileID, unsigned> locInfo = SM.getDecomposedLoc(loc);

  // Try to load the file buffer.
  bool invalidTemp = false;
  StringRef file = SM.getBufferData(locInfo.first, &invalidTemp);
  if (invalidTemp)
    return SourceLocation();

  const char *tokenBegin = file.data() + locInfo.second;

  // Lex from the start of the given location.
  Lexer lexer(SM.getLocForStartOfFile(locInfo.first),
              Ctx.getLangOptions(),
              file.begin(), tokenBegin, file.end());
  Token tok;
  lexer.LexFromRawLexer(tok);
  if (tok.isNot(tok::semi))
    return SourceLocation();

  return tok.getLocation();
}

bool trans::hasSideEffects(Expr *E, ASTContext &Ctx) {
  if (!E || !E->HasSideEffects(Ctx))
    return false;

  E = E->IgnoreParenCasts();
  ObjCMessageExpr *ME = dyn_cast<ObjCMessageExpr>(E);
  if (!ME)
    return true;
  switch (ME->getMethodFamily()) {
  case OMF_autorelease:
  case OMF_dealloc:
  case OMF_release:
  case OMF_retain:
    switch (ME->getReceiverKind()) {
    case ObjCMessageExpr::SuperInstance:
      return false;
    case ObjCMessageExpr::Instance:
      return hasSideEffects(ME->getInstanceReceiver(), Ctx);
    default:
      break;
    }
    break;
  default:
    break;
  }

  return true;
}

bool trans::isGlobalVar(Expr *E) {
  E = E->IgnoreParenCasts();
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E))
    return DRE->getDecl()->getDeclContext()->isFileContext() &&
           DRE->getDecl()->getLinkage() == ExternalLinkage;
  if (ConditionalOperator *condOp = dyn_cast<ConditionalOperator>(E))
    return isGlobalVar(condOp->getTrueExpr()) &&
           isGlobalVar(condOp->getFalseExpr());

  return false;  
}

StringRef trans::getNilString(ASTContext &Ctx) {
  if (Ctx.Idents.get("nil").hasMacroDefinition())
    return "nil";
  else
    return "0";
}

namespace {

class ReferenceClear : public RecursiveASTVisitor<ReferenceClear> {
  ExprSet &Refs;
public:
  ReferenceClear(ExprSet &refs) : Refs(refs) { }
  bool VisitDeclRefExpr(DeclRefExpr *E) { Refs.erase(E); return true; }
  bool VisitBlockDeclRefExpr(BlockDeclRefExpr *E) { Refs.erase(E); return true; }
};

class ReferenceCollector : public RecursiveASTVisitor<ReferenceCollector> {
  ValueDecl *Dcl;
  ExprSet &Refs;

public:
  ReferenceCollector(ValueDecl *D, ExprSet &refs)
    : Dcl(D), Refs(refs) { }

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    if (E->getDecl() == Dcl)
      Refs.insert(E);
    return true;
  }

  bool VisitBlockDeclRefExpr(BlockDeclRefExpr *E) {
    if (E->getDecl() == Dcl)
      Refs.insert(E);
    return true;
  }
};

class RemovablesCollector : public RecursiveASTVisitor<RemovablesCollector> {
  ExprSet &Removables;

public:
  RemovablesCollector(ExprSet &removables)
  : Removables(removables) { }
  
  bool shouldWalkTypesOfTypeLocs() const { return false; }
  
  bool TraverseStmtExpr(StmtExpr *E) {
    CompoundStmt *S = E->getSubStmt();
    for (CompoundStmt::body_iterator
        I = S->body_begin(), E = S->body_end(); I != E; ++I) {
      if (I != E - 1)
        mark(*I);
      TraverseStmt(*I);
    }
    return true;
  }
  
  bool VisitCompoundStmt(CompoundStmt *S) {
    for (CompoundStmt::body_iterator
        I = S->body_begin(), E = S->body_end(); I != E; ++I)
      mark(*I);
    return true;
  }
  
  bool VisitIfStmt(IfStmt *S) {
    mark(S->getThen());
    mark(S->getElse());
    return true;
  }
  
  bool VisitWhileStmt(WhileStmt *S) {
    mark(S->getBody());
    return true;
  }
  
  bool VisitDoStmt(DoStmt *S) {
    mark(S->getBody());
    return true;
  }
  
  bool VisitForStmt(ForStmt *S) {
    mark(S->getInit());
    mark(S->getInc());
    mark(S->getBody());
    return true;
  }
  
private:
  void mark(Stmt *S) {
    if (!S) return;
    
    while (LabelStmt *Label = dyn_cast<LabelStmt>(S))
      S = Label->getSubStmt();
    S = S->IgnoreImplicit();
    if (Expr *E = dyn_cast<Expr>(S))
      Removables.insert(E);
  }
};

} // end anonymous namespace

void trans::clearRefsIn(Stmt *S, ExprSet &refs) {
  ReferenceClear(refs).TraverseStmt(S);
}

void trans::collectRefs(ValueDecl *D, Stmt *S, ExprSet &refs) {
  ReferenceCollector(D, refs).TraverseStmt(S);
}

void trans::collectRemovables(Stmt *S, ExprSet &exprs) {
  RemovablesCollector(exprs).TraverseStmt(S);
}

//===----------------------------------------------------------------------===//
// MigrationContext
//===----------------------------------------------------------------------===//

namespace {

class ASTTransform : public RecursiveASTVisitor<ASTTransform> {
  MigrationContext &MigrateCtx;
  typedef RecursiveASTVisitor<ASTTransform> base;

public:
  ASTTransform(MigrationContext &MigrateCtx) : MigrateCtx(MigrateCtx) { }

  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool TraverseObjCImplementationDecl(ObjCImplementationDecl *D) {
    ObjCImplementationContext ImplCtx(MigrateCtx, D);
    for (MigrationContext::traverser_iterator
           I = MigrateCtx.traversers_begin(),
           E = MigrateCtx.traversers_end(); I != E; ++I)
      (*I)->traverseObjCImplementation(ImplCtx);

    return base::TraverseObjCImplementationDecl(D);
  }

  bool TraverseStmt(Stmt *rootS) {
    if (!rootS)
      return true;

    BodyContext BodyCtx(MigrateCtx, rootS);
    for (MigrationContext::traverser_iterator
           I = MigrateCtx.traversers_begin(),
           E = MigrateCtx.traversers_end(); I != E; ++I)
      (*I)->traverseBody(BodyCtx);

    return true;
  }
};

}

MigrationContext::~MigrationContext() {
  for (traverser_iterator
         I = traversers_begin(), E = traversers_end(); I != E; ++I)
    delete *I;
}

bool MigrationContext::isGCOwnedNonObjC(QualType T) {
  while (!T.isNull()) {
    if (const AttributedType *AttrT = T->getAs<AttributedType>()) {
      if (AttrT->getAttrKind() == AttributedType::attr_objc_ownership)
        return !AttrT->getModifiedType()->isObjCRetainableType();
    }

    if (T->isArrayType())
      T = Pass.Ctx.getBaseElementType(T);
    else if (const PointerType *PT = T->getAs<PointerType>())
      T = PT->getPointeeType();
    else if (const ReferenceType *RT = T->getAs<ReferenceType>())
      T = RT->getPointeeType();
    else
      break;
  }

  return false;
}

bool MigrationContext::rewritePropertyAttribute(StringRef fromAttr,
                                                StringRef toAttr,
                                                SourceLocation atLoc) {
  if (atLoc.isMacroID())
    return false;

  SourceManager &SM = Pass.Ctx.getSourceManager();

  // Break down the source location.
  std::pair<FileID, unsigned> locInfo = SM.getDecomposedLoc(atLoc);

  // Try to load the file buffer.
  bool invalidTemp = false;
  StringRef file = SM.getBufferData(locInfo.first, &invalidTemp);
  if (invalidTemp)
    return false;

  const char *tokenBegin = file.data() + locInfo.second;

  // Lex from the start of the given location.
  Lexer lexer(SM.getLocForStartOfFile(locInfo.first),
              Pass.Ctx.getLangOptions(),
              file.begin(), tokenBegin, file.end());
  Token tok;
  lexer.LexFromRawLexer(tok);
  if (tok.isNot(tok::at)) return false;
  lexer.LexFromRawLexer(tok);
  if (tok.isNot(tok::raw_identifier)) return false;
  if (StringRef(tok.getRawIdentifierData(), tok.getLength())
        != "property")
    return false;
  lexer.LexFromRawLexer(tok);
  if (tok.isNot(tok::l_paren)) return false;
  
  Token BeforeTok = tok;
  Token AfterTok;
  AfterTok.startToken();
  SourceLocation AttrLoc;
  
  lexer.LexFromRawLexer(tok);
  if (tok.is(tok::r_paren))
    return false;

  while (1) {
    if (tok.isNot(tok::raw_identifier)) return false;
    StringRef ident(tok.getRawIdentifierData(), tok.getLength());
    if (ident == fromAttr) {
      if (!toAttr.empty()) {
        Pass.TA.replaceText(tok.getLocation(), fromAttr, toAttr);
        return true;
      }
      // We want to remove the attribute.
      AttrLoc = tok.getLocation();
    }

    do {
      lexer.LexFromRawLexer(tok);
      if (AttrLoc.isValid() && AfterTok.is(tok::unknown))
        AfterTok = tok;
    } while (tok.isNot(tok::comma) && tok.isNot(tok::r_paren));
    if (tok.is(tok::r_paren))
      break;
    if (AttrLoc.isInvalid())
      BeforeTok = tok;
    lexer.LexFromRawLexer(tok);
  }

  if (toAttr.empty() && AttrLoc.isValid() && AfterTok.isNot(tok::unknown)) {
    // We want to remove the attribute.
    if (BeforeTok.is(tok::l_paren) && AfterTok.is(tok::r_paren)) {
      Pass.TA.remove(SourceRange(BeforeTok.getLocation(),
                                 AfterTok.getLocation()));
    } else if (BeforeTok.is(tok::l_paren) && AfterTok.is(tok::comma)) {
      Pass.TA.remove(SourceRange(AttrLoc, AfterTok.getLocation()));
    } else {
      Pass.TA.remove(SourceRange(BeforeTok.getLocation(), AttrLoc));
    }

    return true;
  }
  
  return false;
}

void MigrationContext::traverse(TranslationUnitDecl *TU) {
  for (traverser_iterator
         I = traversers_begin(), E = traversers_end(); I != E; ++I)
    (*I)->traverseTU(*this);

  ASTTransform(*this).TraverseDecl(TU);
}

//===----------------------------------------------------------------------===//
// getAllTransformations.
//===----------------------------------------------------------------------===//

static void traverseAST(MigrationPass &pass) {
  MigrationContext MigrateCtx(pass);

  if (pass.isGCMigration()) {
    MigrateCtx.addTraverser(new GCCollectableCallsTraverser);
    MigrateCtx.addTraverser(new GCAttrsTraverser());
  }
  MigrateCtx.addTraverser(new PropertyRewriteTraverser());

  MigrateCtx.traverse(pass.Ctx.getTranslationUnitDecl());
}

static void independentTransforms(MigrationPass &pass) {
  rewriteAutoreleasePool(pass);
  removeRetainReleaseDeallocFinalize(pass);
  rewriteUnusedInitDelegate(pass);
  removeZeroOutPropsInDeallocFinalize(pass);
  makeAssignARCSafe(pass);
  rewriteUnbridgedCasts(pass);
  rewriteBlockObjCVariable(pass);
  checkAPIUses(pass);
  traverseAST(pass);
}

std::vector<TransformFn> arcmt::getAllTransformations(
                                               LangOptions::GCMode OrigGCMode) {
  std::vector<TransformFn> transforms;

  transforms.push_back(independentTransforms);
  // This depends on previous transformations removing various expressions.
  transforms.push_back(removeEmptyStatementsAndDeallocFinalize);

  return transforms;
}

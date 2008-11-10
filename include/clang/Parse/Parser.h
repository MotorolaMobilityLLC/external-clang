//===--- Parser.h - C Language Parser ---------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Parser interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_PARSE_PARSER_H
#define LLVM_CLANG_PARSE_PARSER_H

#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/Action.h"
#include "clang/Parse/DeclSpec.h"
#include <stack>

namespace clang {
  class AttributeList;
  class DeclSpec;
  class Declarator;
  class FieldDeclarator;
  class ObjCDeclSpec;
  class PragmaHandler;
  class Scope;

/// Parser - This implements a parser for the C family of languages.  After
/// parsing units of the grammar, productions are invoked to handle whatever has
/// been read.
///
class Parser {
  Preprocessor &PP;
  
  /// Tok - The current token we are peeking ahead.  All parsing methods assume
  /// that this is valid.
  Token Tok;
  
  unsigned short ParenCount, BracketCount, BraceCount;

  /// Actions - These are the callbacks we invoke as we parse various constructs
  /// in the file.  This refers to the common base class between MinimalActions
  /// and SemaActions for those uses that don't matter.
  Action &Actions;
  
  Scope *CurScope;
  Diagnostic &Diags;
  
  /// ScopeCache - Cache scopes to reduce malloc traffic.
  enum { ScopeCacheSize = 16 };
  unsigned NumCachedScopes;
  Scope *ScopeCache[ScopeCacheSize];

  /// Ident_super - IdentifierInfo for "super", to support fast
  /// comparison.
  IdentifierInfo *Ident_super;

  PragmaHandler *PackHandler;

public:
  Parser(Preprocessor &PP, Action &Actions);
  ~Parser();

  const LangOptions &getLang() const { return PP.getLangOptions(); }
  TargetInfo &getTargetInfo() const { return PP.getTargetInfo(); }
  Action &getActions() const { return Actions; }
  
  // Type forwarding.  All of these are statically 'void*', but they may all be
  // different actual classes based on the actions in place.
  typedef Action::ExprTy ExprTy;
  typedef Action::StmtTy StmtTy;
  typedef Action::DeclTy DeclTy;
  typedef Action::TypeTy TypeTy;
  typedef Action::BaseTy BaseTy;
  typedef Action::MemInitTy MemInitTy;
  typedef Action::CXXScopeTy CXXScopeTy;

  // Parsing methods.
  
  /// ParseTranslationUnit - All in one method that initializes parses, and
  /// shuts down the parser.
  void ParseTranslationUnit();
  
  /// Initialize - Warm up the parser.
  ///
  void Initialize();
  
  /// ParseTopLevelDecl - Parse one top-level declaration. Returns true if 
  /// the EOF was encountered.
  bool ParseTopLevelDecl(DeclTy*& Result);
  
private:
  //===--------------------------------------------------------------------===//
  // Low-Level token peeking and consumption methods.
  //
  
  /// isTokenParen - Return true if the cur token is '(' or ')'.
  bool isTokenParen() const {
    return Tok.getKind() == tok::l_paren || Tok.getKind() == tok::r_paren;
  }
  /// isTokenBracket - Return true if the cur token is '[' or ']'.
  bool isTokenBracket() const {
    return Tok.getKind() == tok::l_square || Tok.getKind() == tok::r_square;
  }
  /// isTokenBrace - Return true if the cur token is '{' or '}'.
  bool isTokenBrace() const {
    return Tok.getKind() == tok::l_brace || Tok.getKind() == tok::r_brace;
  }
  
  /// isTokenStringLiteral - True if this token is a string-literal.
  ///
  bool isTokenStringLiteral() const {
    return Tok.getKind() == tok::string_literal ||
           Tok.getKind() == tok::wide_string_literal;
  }

  /// isTokenCXXScopeSpecifier - True if this token is '::', or identifier with
  /// '::' as next token, or a 'C++ scope annotation' token.
  /// When not in C++, always returns false.
  ///
  bool isTokenCXXScopeSpecifier() {
    return getLang().CPlusPlus &&
           (Tok.is(tok::coloncolon)     ||
            Tok.is(tok::annot_cxxscope) ||
            (Tok.is(tok::identifier) && NextToken().is(tok::coloncolon)));
  }
  
  /// isTokenUnqualifiedId - True if token is the start of C++ unqualified-id
  /// or an identifier in C.
  ///
  ///       unqualified-id:
  ///         identifier
  /// [C++]   operator-function-id
  /// [C++]   conversion-function-id
  /// [C++]   '~' class-name
  /// [C++]   template-id      [TODO]
  ///
  bool isTokenUnqualifiedId() const {
    return Tok.is(tok::identifier)  ||   // identifier or template-id 
           Tok.is(tok::kw_operator) ||   // operator/conversion-function-id or
                                         // template-id
           (Tok.is(tok::tilde) && getLang().CPlusPlus); // '~' class-name
  }

  /// ConsumeToken - Consume the current 'peek token' and lex the next one.
  /// This does not work with all kinds of tokens: strings and specific other
  /// tokens must be consumed with custom methods below.  This returns the
  /// location of the consumed token.
  SourceLocation ConsumeToken() {
    assert(!isTokenStringLiteral() && !isTokenParen() && !isTokenBracket() &&
           !isTokenBrace() &&
           "Should consume special tokens with Consume*Token");
    SourceLocation L = Tok.getLocation();
    PP.Lex(Tok);
    return L;
  }
  
  /// ConsumeAnyToken - Dispatch to the right Consume* method based on the
  /// current token type.  This should only be used in cases where the type of
  /// the token really isn't known, e.g. in error recovery.
  SourceLocation ConsumeAnyToken() {
    if (isTokenParen())
      return ConsumeParen();
    else if (isTokenBracket())
      return ConsumeBracket();
    else if (isTokenBrace())
      return ConsumeBrace();
    else if (isTokenStringLiteral())
      return ConsumeStringToken();
    else
      return ConsumeToken();
  }
  
  /// ConsumeParen - This consume method keeps the paren count up-to-date.
  ///
  SourceLocation ConsumeParen() {
    assert(isTokenParen() && "wrong consume method");
    if (Tok.getKind() == tok::l_paren)
      ++ParenCount;
    else if (ParenCount)
      --ParenCount;       // Don't let unbalanced )'s drive the count negative.
    SourceLocation L = Tok.getLocation();
    PP.Lex(Tok);
    return L;
  }
  
  /// ConsumeBracket - This consume method keeps the bracket count up-to-date.
  ///
  SourceLocation ConsumeBracket() {
    assert(isTokenBracket() && "wrong consume method");
    if (Tok.getKind() == tok::l_square)
      ++BracketCount;
    else if (BracketCount)
      --BracketCount;     // Don't let unbalanced ]'s drive the count negative.
    
    SourceLocation L = Tok.getLocation();
    PP.Lex(Tok);
    return L;
  }
      
  /// ConsumeBrace - This consume method keeps the brace count up-to-date.
  ///
  SourceLocation ConsumeBrace() {
    assert(isTokenBrace() && "wrong consume method");
    if (Tok.getKind() == tok::l_brace)
      ++BraceCount;
    else if (BraceCount)
      --BraceCount;     // Don't let unbalanced }'s drive the count negative.
    
    SourceLocation L = Tok.getLocation();
    PP.Lex(Tok);
    return L;
  }
  
  /// ConsumeStringToken - Consume the current 'peek token', lexing a new one
  /// and returning the token kind.  This method is specific to strings, as it
  /// handles string literal concatenation, as per C99 5.1.1.2, translation
  /// phase #6.
  SourceLocation ConsumeStringToken() {
    assert(isTokenStringLiteral() &&
           "Should only consume string literals with this method");
    SourceLocation L = Tok.getLocation();
    PP.Lex(Tok);
    return L;
  }
  
  /// GetLookAheadToken - This peeks ahead N tokens and returns that token
  /// without consuming any tokens.  LookAhead(0) returns 'Tok', LookAhead(1)
  /// returns the token after Tok, etc.
  ///
  /// Note that this differs from the Preprocessor's LookAhead method, because
  /// the Parser always has one token lexed that the preprocessor doesn't.
  ///
  const Token &GetLookAheadToken(unsigned N) {
    if (N == 0 || Tok.is(tok::eof)) return Tok;
    return PP.LookAhead(N-1);
  }

  /// NextToken - This peeks ahead one token and returns it without
  /// consuming it.
  const Token &NextToken() {
    return PP.LookAhead(0);
  }

  /// TryAnnotateTypeOrScopeToken - If the current token position is on a
  /// typename (possibly qualified in C++) or a C++ scope specifier not followed
  /// by a typename, TryAnnotateTypeOrScopeToken will replace one or more tokens
  /// with a single annotation token representing the typename or C++ scope
  /// respectively.
  /// This simplifies handling of C++ scope specifiers and allows efficient
  /// backtracking without the need to re-parse and resolve nested-names and
  /// typenames.
  void TryAnnotateTypeOrScopeToken();

  /// TryAnnotateScopeToken - Like TryAnnotateTypeOrScopeToken but only
  /// annotates C++ scope specifiers.
  void TryAnnotateScopeToken();

  /// TentativeParsingAction - An object that is used as a kind of "tentative
  /// parsing transaction". It gets instantiated to mark the token position and
  /// after the token consumption is done, Commit() or Revert() is called to
  /// either "commit the consumed tokens" or revert to the previously marked
  /// token position. Example:
  ///
  ///   TentativeParsingAction TPA;
  ///   ConsumeToken();
  ///   ....
  ///   TPA.Revert();
  ///
  class TentativeParsingAction {
    Parser &P;
    Token PrevTok;
    bool isActive;

  public:
    explicit TentativeParsingAction(Parser& p) : P(p) {
      PrevTok = P.Tok;
      P.PP.EnableBacktrackAtThisPos();
      isActive = true;
    }
    void Commit() {
      assert(isActive && "Parsing action was finished!");
      P.PP.CommitBacktrackedTokens();
      isActive = false;
    }
    void Revert() {
      assert(isActive && "Parsing action was finished!");
      P.PP.Backtrack();
      P.Tok = PrevTok;
      isActive = false;
    }
    ~TentativeParsingAction() {
      assert(!isActive && "Forgot to call Commit or Revert!");
    }
  };
  
  
  /// MatchRHSPunctuation - For punctuation with a LHS and RHS (e.g. '['/']'),
  /// this helper function matches and consumes the specified RHS token if
  /// present.  If not present, it emits the specified diagnostic indicating
  /// that the parser failed to match the RHS of the token at LHSLoc.  LHSName
  /// should be the name of the unmatched LHS token.  This returns the location
  /// of the consumed token.
  SourceLocation MatchRHSPunctuation(tok::TokenKind RHSTok,
                                     SourceLocation LHSLoc);
  
  /// ExpectAndConsume - The parser expects that 'ExpectedTok' is next in the
  /// input.  If so, it is consumed and false is returned.
  ///
  /// If the input is malformed, this emits the specified diagnostic.  Next, if
  /// SkipToTok is specified, it calls SkipUntil(SkipToTok).  Finally, true is
  /// returned.
  bool ExpectAndConsume(tok::TokenKind ExpectedTok, unsigned Diag,
                        const char *DiagMsg = "",
                        tok::TokenKind SkipToTok = tok::unknown);

  //===--------------------------------------------------------------------===//
  // Scope manipulation
  
  /// EnterScope - Start a new scope.
  void EnterScope(unsigned ScopeFlags);
  
  /// ExitScope - Pop a scope off the scope stack.
  void ExitScope();

  //===--------------------------------------------------------------------===//
  // Diagnostic Emission and Error recovery.
    
  bool Diag(SourceLocation Loc, unsigned DiagID,
            const std::string &Msg = std::string());
  bool Diag(SourceLocation Loc, unsigned DiagID, const SourceRange &R);
  bool Diag(SourceLocation Loc, unsigned DiagID, const std::string &Msg,
            const SourceRange& R1);
  bool Diag(const Token &Tok, unsigned DiagID,
            const std::string &M = std::string()) {
    return Diag(Tok.getLocation(), DiagID, M);
  }
  
  /// SkipUntil - Read tokens until we get to the specified token, then consume
  /// it (unless DontConsume is true).  Because we cannot guarantee that the
  /// token will ever occur, this skips to the next token, or to some likely
  /// good stopping point.  If StopAtSemi is true, skipping will stop at a ';'
  /// character.
  /// 
  /// If SkipUntil finds the specified token, it returns true, otherwise it
  /// returns false.  
  bool SkipUntil(tok::TokenKind T, bool StopAtSemi = true,
                 bool DontConsume = false) {
    return SkipUntil(&T, 1, StopAtSemi, DontConsume);
  }
  bool SkipUntil(tok::TokenKind T1, tok::TokenKind T2, bool StopAtSemi = true,
                 bool DontConsume = false) {
    tok::TokenKind TokArray[] = {T1, T2};
    return SkipUntil(TokArray, 2, StopAtSemi, DontConsume);
  }
  bool SkipUntil(const tok::TokenKind *Toks, unsigned NumToks,
                 bool StopAtSemi = true, bool DontConsume = false);
 
  typedef Action::ExprResult    ExprResult;
  typedef Action::StmtResult    StmtResult;
  typedef Action::BaseResult    BaseResult;
  typedef Action::MemInitResult MemInitResult;

  //===--------------------------------------------------------------------===//
  // Lexing and parsing of C++ inline methods.

  typedef llvm::SmallVector<Token, 32> TokensTy;
  struct LexedMethod {
    Action::DeclTy *D;
    TokensTy Toks;
    explicit LexedMethod(Action::DeclTy *MD) : D(MD) {}
  };

  /// LexedMethodsForTopClass - During parsing of a top (non-nested) C++ class,
  /// its inline method definitions and the inline method definitions of its
  /// nested classes are lexed and stored here.
  typedef std::stack<LexedMethod> LexedMethodsForTopClass;

  /// TopClassStacks - This is initialized with one LexedMethodsForTopClass used
  /// for lexing all top classes, until a local class in an inline method is
  /// encountered, at which point a new LexedMethodsForTopClass is pushed here
  /// and used until the parsing of that local class is finished.
  std::stack<LexedMethodsForTopClass> TopClassStacks;

  LexedMethodsForTopClass &getCurTopClassStack() {
    assert(!TopClassStacks.empty() && "No lexed method stacks!");
    return TopClassStacks.top();
  }

  void PushTopClassStack() {
    TopClassStacks.push(LexedMethodsForTopClass());
  }
  void PopTopClassStack() { TopClassStacks.pop(); }

  DeclTy *ParseCXXInlineMethodDef(AccessSpecifier AS, Declarator &D);
  void ParseLexedMethodDefs();
  bool ConsumeAndStoreUntil(tok::TokenKind T, TokensTy &Toks,
                            tok::TokenKind EarlyAbortIf = tok::unknown);

  //===--------------------------------------------------------------------===//
  // C99 6.9: External Definitions.
  DeclTy *ParseExternalDeclaration();
  DeclTy *ParseDeclarationOrFunctionDefinition();
  DeclTy *ParseFunctionDefinition(Declarator &D);
  void ParseKNRParamDeclarations(Declarator &D);
  ExprResult ParseSimpleAsm();
  ExprResult ParseAsmStringLiteral();

  // Objective-C External Declarations
  DeclTy *ParseObjCAtDirectives(); 
  DeclTy *ParseObjCAtClassDeclaration(SourceLocation atLoc);
  DeclTy *ParseObjCAtInterfaceDeclaration(SourceLocation atLoc, 
                                          AttributeList *prefixAttrs = 0);
  void ParseObjCClassInstanceVariables(DeclTy *interfaceDecl, 
                                       SourceLocation atLoc);
  bool ParseObjCProtocolReferences(llvm::SmallVectorImpl<Action::DeclTy*> &P,
                                   bool WarnOnDeclarations, 
                                   SourceLocation &EndProtoLoc);
  void ParseObjCInterfaceDeclList(DeclTy *interfaceDecl,
                                  tok::ObjCKeywordKind contextKey);
  DeclTy *ParseObjCAtProtocolDeclaration(SourceLocation atLoc,
                                         AttributeList *prefixAttrs = 0);
  
  DeclTy *ObjCImpDecl;

  DeclTy *ParseObjCAtImplementationDeclaration(SourceLocation atLoc);
  DeclTy *ParseObjCAtEndDeclaration(SourceLocation atLoc);
  DeclTy *ParseObjCAtAliasDeclaration(SourceLocation atLoc);
  DeclTy *ParseObjCPropertySynthesize(SourceLocation atLoc);
  DeclTy *ParseObjCPropertyDynamic(SourceLocation atLoc);
  
  IdentifierInfo *ParseObjCSelector(SourceLocation &MethodLocation);
  // Definitions for Objective-c context sensitive keywords recognition.
  enum ObjCTypeQual {
    objc_in=0, objc_out, objc_inout, objc_oneway, objc_bycopy, objc_byref,
    objc_NumQuals
  };
  IdentifierInfo *ObjCTypeQuals[objc_NumQuals];
  
  bool isTokIdentifier_in() const;

  TypeTy *ParseObjCTypeName(ObjCDeclSpec &DS);
  void ParseObjCMethodRequirement();
  DeclTy *ParseObjCMethodPrototype(DeclTy *classOrCat,
            tok::ObjCKeywordKind MethodImplKind = tok::objc_not_keyword);
  DeclTy *ParseObjCMethodDecl(SourceLocation mLoc, tok::TokenKind mType,
                              DeclTy *classDecl,
            tok::ObjCKeywordKind MethodImplKind = tok::objc_not_keyword);
  void ParseObjCPropertyAttribute(ObjCDeclSpec &DS);
  
  DeclTy *ParseObjCMethodDefinition();
  
  //===--------------------------------------------------------------------===//
  // C99 6.5: Expressions.

  ExprResult ParseExpression();
  ExprResult ParseConstantExpression();
  ExprResult ParseAssignmentExpression();  // Expr that doesn't include commas.
  
  ExprResult ParseExpressionWithLeadingAt(SourceLocation AtLoc);

  ExprResult ParseRHSOfBinaryExpression(ExprResult LHS, unsigned MinPrec);
  ExprResult ParseCastExpression(bool isUnaryExpression);
  ExprResult ParsePostfixExpressionSuffix(ExprResult LHS);
  ExprResult ParseSizeofAlignofExpression();
  ExprResult ParseBuiltinPrimaryExpression();

  typedef llvm::SmallVector<ExprTy*, 8> ExprListTy;
  typedef llvm::SmallVector<SourceLocation, 8> CommaLocsTy;

  /// ParseExpressionList - Used for C/C++ (argument-)expression-list.
  bool ParseExpressionList(ExprListTy &Exprs, CommaLocsTy &CommaLocs);
  
  /// ParenParseOption - Control what ParseParenExpression will parse.
  enum ParenParseOption {
    SimpleExpr,      // Only parse '(' expression ')'
    CompoundStmt,    // Also allow '(' compound-statement ')'
    CompoundLiteral, // Also allow '(' type-name ')' '{' ... '}'
    CastExpr         // Also allow '(' type-name ')' <anything>
  };
  ExprResult ParseParenExpression(ParenParseOption &ExprType, TypeTy *&CastTy,
                                  SourceLocation &RParenLoc);
  
  ExprResult ParseSimpleParenExpression() {  // Parse SimpleExpr only.
    SourceLocation RParenLoc;
    return ParseSimpleParenExpression(RParenLoc);
  }
  ExprResult ParseSimpleParenExpression(SourceLocation &RParenLoc) {
    ParenParseOption Op = SimpleExpr;
    TypeTy *CastTy;
    return ParseParenExpression(Op, CastTy, RParenLoc);
  }
  ExprResult ParseStringLiteralExpression();

  //===--------------------------------------------------------------------===//
  // C++ Expressions
  ExprResult ParseCXXIdExpression();
  void ParseCXXScopeSpecifier(CXXScopeSpec &SS);
  
  //===--------------------------------------------------------------------===//
  // C++ 5.2p1: C++ Casts
  ExprResult ParseCXXCasts();

  //===--------------------------------------------------------------------===//
  // C++ 9.3.2: C++ 'this' pointer
  ExprResult ParseCXXThis();

  //===--------------------------------------------------------------------===//
  // C++ 15: C++ Throw Expression
  ExprResult ParseThrowExpression();

  //===--------------------------------------------------------------------===//
  // C++ 2.13.5: C++ Boolean Literals
  ExprResult ParseCXXBoolLiteral();

  //===--------------------------------------------------------------------===//
  // C++ 5.2.3: Explicit type conversion (functional notation)
  ExprResult ParseCXXTypeConstructExpression(const DeclSpec &DS);

  /// ParseCXXSimpleTypeSpecifier - [C++ 7.1.5.2] Simple type specifiers.
  /// This should only be called when the current token is known to be part of
  /// simple-type-specifier.
  void ParseCXXSimpleTypeSpecifier(DeclSpec &DS);

  bool ParseCXXTypeSpecifierSeq(DeclSpec &DS);

  //===--------------------------------------------------------------------===//
  // C++ if/switch/while/for condition expression.
  ExprResult ParseCXXCondition();

  //===--------------------------------------------------------------------===//
  // C++ types

  //===--------------------------------------------------------------------===//
  // C99 6.7.8: Initialization.
  
  /// ParseInitializer
  ///       initializer: [C99 6.7.8]
  ///         assignment-expression
  ///         '{' ...
  ExprResult ParseInitializer() {
    if (Tok.isNot(tok::l_brace))
      return ParseAssignmentExpression();
    return ParseBraceInitializer();
  }
  ExprResult ParseBraceInitializer();
  ExprResult ParseInitializerWithPotentialDesignator(InitListDesignations &D,
                                                     unsigned InitNum);
  
  //===--------------------------------------------------------------------===//
  // clang Expressions
  
  ExprResult ParseBlockLiteralExpression();  // ^{...}

  //===--------------------------------------------------------------------===//
  // Objective-C Expressions
  
  bool isTokObjCMessageIdentifierReceiver() const {
    if (!Tok.is(tok::identifier))
      return false;
    
    if (Actions.isTypeName(*Tok.getIdentifierInfo(), CurScope))
      return true;
    
    return Tok.getIdentifierInfo() == Ident_super;
  }
  
  ExprResult ParseObjCAtExpression(SourceLocation AtLocation);
  ExprResult ParseObjCStringLiteral(SourceLocation AtLoc);
  ExprResult ParseObjCEncodeExpression(SourceLocation AtLoc);
  ExprResult ParseObjCSelectorExpression(SourceLocation AtLoc);
  ExprResult ParseObjCProtocolExpression(SourceLocation AtLoc);
  ExprResult ParseObjCMessageExpression();
  ExprResult ParseObjCMessageExpressionBody(SourceLocation LBracloc,
                                            IdentifierInfo *ReceiverName,
                                            ExprTy *ReceiverExpr);
  ExprResult ParseAssignmentExprWithObjCMessageExprStart(SourceLocation LBracloc,
                                                         IdentifierInfo *ReceiverName,
                                                         ExprTy *ReceiverExpr);
    
  //===--------------------------------------------------------------------===//
  // C99 6.8: Statements and Blocks.
  
  StmtResult ParseStatement() { return ParseStatementOrDeclaration(true); }
  StmtResult ParseStatementOrDeclaration(bool OnlyStatement = false);
  StmtResult ParseLabeledStatement();
  StmtResult ParseCaseStatement();
  StmtResult ParseDefaultStatement();
  StmtResult ParseCompoundStatement(bool isStmtExpr = false);
  StmtResult ParseCompoundStatementBody(bool isStmtExpr = false);
  StmtResult ParseIfStatement();
  StmtResult ParseSwitchStatement();
  StmtResult ParseWhileStatement();
  StmtResult ParseDoStatement();
  StmtResult ParseForStatement();
  StmtResult ParseGotoStatement();
  StmtResult ParseContinueStatement();
  StmtResult ParseBreakStatement();
  StmtResult ParseReturnStatement();
  StmtResult ParseAsmStatement(bool &msAsm);
  StmtResult FuzzyParseMicrosoftAsmStatement();
  StmtResult ParseObjCAtStatement(SourceLocation atLoc);
  StmtResult ParseObjCTryStmt(SourceLocation atLoc);
  StmtResult ParseObjCThrowStmt(SourceLocation atLoc);
  StmtResult ParseObjCSynchronizedStmt(SourceLocation atLoc);
  bool ParseAsmOperandsOpt(llvm::SmallVectorImpl<std::string> &Names,
                           llvm::SmallVectorImpl<ExprTy*> &Constraints,
                           llvm::SmallVectorImpl<ExprTy*> &Exprs);


  //===--------------------------------------------------------------------===//
  // C99 6.7: Declarations.
  
  DeclTy *ParseDeclaration(unsigned Context);
  DeclTy *ParseSimpleDeclaration(unsigned Context);
  DeclTy *ParseInitDeclaratorListAfterFirstDeclarator(Declarator &D);
  DeclTy *ParseFunctionStatementBody(DeclTy *Decl, 
                                     SourceLocation L, SourceLocation R);
  void ParseDeclarationSpecifiers(DeclSpec &DS);
  bool MaybeParseTypeSpecifier(DeclSpec &DS, int &isInvalid, 
                               const char *&PrevSpec);
  void ParseSpecifierQualifierList(DeclSpec &DS);
  
  void ParseObjCTypeQualifierList(ObjCDeclSpec &DS);

  void ParseEnumSpecifier(DeclSpec &DS);
  void ParseEnumBody(SourceLocation StartLoc, DeclTy *TagDecl);
  void ParseStructUnionBody(SourceLocation StartLoc, unsigned TagType,
                            DeclTy *TagDecl);
  void ParseStructDeclaration(DeclSpec &DS,
                              llvm::SmallVectorImpl<FieldDeclarator> &Fields);
                              
  bool isDeclarationSpecifier();
  bool isTypeSpecifierQualifier();
  bool isTypeQualifier() const;

  /// isDeclarationStatement - Disambiguates between a declaration or an
  /// expression statement, when parsing function bodies.
  /// Returns true for declaration, false for expression.
  bool isDeclarationStatement() {
    if (getLang().CPlusPlus)
      return isCXXDeclarationStatement();
    return isDeclarationSpecifier();
  }

  /// isSimpleDeclaration - Disambiguates between a declaration or an
  /// expression, mainly used for the C 'clause-1' or the C++
  // 'for-init-statement' part of a 'for' statement.
  /// Returns true for declaration, false for expression.
  bool isSimpleDeclaration() {
    if (getLang().CPlusPlus)
      return isCXXSimpleDeclaration();
    return isDeclarationSpecifier();
  }

  /// isTypeIdInParens - Assumes that a '(' was parsed and now we want to know
  /// whether the parens contain an expression or a type-id.
  /// Returns true for a type-id and false for an expression.
  bool isTypeIdInParens() {
    if (getLang().CPlusPlus)
      return isCXXTypeIdInParens();
    return isTypeSpecifierQualifier();
  }

  /// isCXXDeclarationStatement - C++-specialized function that disambiguates
  /// between a declaration or an expression statement, when parsing function
  /// bodies. Returns true for declaration, false for expression.
  bool isCXXDeclarationStatement();

  /// isCXXSimpleDeclaration - C++-specialized function that disambiguates
  /// between a simple-declaration or an expression-statement.
  /// If during the disambiguation process a parsing error is encountered,
  /// the function returns true to let the declaration parsing code handle it.
  /// Returns false if the statement is disambiguated as expression.
  bool isCXXSimpleDeclaration();

  /// isCXXFunctionDeclarator - Disambiguates between a function declarator or
  /// a constructor-style initializer, when parsing declaration statements.
  /// Returns true for function declarator and false for constructor-style
  /// initializer. If 'warnIfAmbiguous' is true a warning will be emitted to
  /// indicate that the parens were disambiguated as function declarator.
  /// If during the disambiguation process a parsing error is encountered,
  /// the function returns true to let the declaration parsing code handle it.
  bool isCXXFunctionDeclarator(bool warnIfAmbiguous);

  /// isCXXConditionDeclaration - Disambiguates between a declaration or an
  /// expression for a condition of a if/switch/while/for statement.
  /// If during the disambiguation process a parsing error is encountered,
  /// the function returns true to let the declaration parsing code handle it.
  bool isCXXConditionDeclaration();

  /// isCXXTypeIdInParens - Assumes that a '(' was parsed and now we want to
  /// know whether the parens contain an expression or a type-id.
  /// Returns true for a type-id and false for an expression.
  /// If during the disambiguation process a parsing error is encountered,
  /// the function returns true to let the declaration parsing code handle it.
  bool isCXXTypeIdInParens();

  /// TPResult - Used as the result value for functions whose purpose is to
  /// disambiguate C++ constructs by "tentatively parsing" them.
  /// This is a class instead of a simple enum because the implicit enum-to-bool
  /// conversions may cause subtle bugs.
  class TPResult {
    enum Result {
      TPR_true,
      TPR_false,
      TPR_ambiguous,
      TPR_error
    };
    Result Res;
    TPResult(Result result) : Res(result) {}
  public:
    static TPResult True() { return TPR_true; }
    static TPResult False() { return TPR_false; }
    static TPResult Ambiguous() { return TPR_ambiguous; }
    static TPResult Error() { return TPR_error; }

    bool operator==(const TPResult &RHS) const { return Res == RHS.Res; }
    bool operator!=(const TPResult &RHS) const { return Res != RHS.Res; }
  };

  /// isCXXDeclarationSpecifier - Returns TPResult::True() if it is a
  /// declaration specifier, TPResult::False() if it is not,
  /// TPResult::Ambiguous() if it could be either a decl-specifier or a
  /// function-style cast, and TPResult::Error() if a parsing error was
  /// encountered.
  /// Doesn't consume tokens.
  TPResult isCXXDeclarationSpecifier();

  // "Tentative parsing" functions, used for disambiguation. If a parsing error
  // is encountered they will return TPResult::Error().
  // Returning TPResult::True()/False() indicates that the ambiguity was
  // resolved and tentative parsing may stop. TPResult::Ambiguous() indicates
  // that more tentative parsing is necessary for disambiguation.
  // They all consume tokens, so backtracking should be used after calling them.

  TPResult TryParseDeclarationSpecifier();
  TPResult TryParseSimpleDeclaration();
  TPResult TryParseTypeofSpecifier();
  TPResult TryParseInitDeclaratorList();
  TPResult TryParseDeclarator(bool mayBeAbstract, bool mayHaveIdentifier=true);
  TPResult TryParseParameterDeclarationClause();
  TPResult TryParseFunctionDeclarator();
  TPResult TryParseBracketDeclarator();


  TypeTy *ParseTypeName();
  AttributeList *ParseAttributes();
  void ParseTypeofSpecifier(DeclSpec &DS);

  /// DeclaratorScopeObj - RAII object used in Parser::ParseDirectDeclarator to
  /// enter a new C++ declarator scope and exit it when the function is
  /// finished.
  class DeclaratorScopeObj {
    Parser &P;
    CXXScopeSpec &SS;
  public:
    DeclaratorScopeObj(Parser &p, CXXScopeSpec &ss) : P(p), SS(ss) {}

    void EnterDeclaratorScope() {
      if (SS.isSet())
        P.Actions.ActOnCXXEnterDeclaratorScope(P.CurScope, SS);
    }

    ~DeclaratorScopeObj() {
      if (SS.isSet())
        P.Actions.ActOnCXXExitDeclaratorScope(SS);
    }
  };
  
  /// ParseDeclarator - Parse and verify a newly-initialized declarator.
  void ParseDeclarator(Declarator &D);
  void ParseDeclaratorInternal(Declarator &D, bool PtrOperator = false);
  void ParseTypeQualifierListOpt(DeclSpec &DS);
  void ParseDirectDeclarator(Declarator &D);
  void ParseParenDeclarator(Declarator &D);
  void ParseFunctionDeclarator(SourceLocation LParenLoc, Declarator &D,
                               AttributeList *AttrList = 0,
                               bool RequiresArg = false);
  void ParseFunctionDeclaratorIdentifierList(SourceLocation LParenLoc,
                                             Declarator &D);
  void ParseBracketDeclarator(Declarator &D);
  
  //===--------------------------------------------------------------------===//
  // C++ 7: Declarations [dcl.dcl]
  
  DeclTy *ParseNamespace(unsigned Context);
  DeclTy *ParseLinkage(unsigned Context);

  //===--------------------------------------------------------------------===//
  // C++ 9: classes [class] and C structs/unions.
  TypeTy *ParseClassName(const CXXScopeSpec *SS = 0);
  void ParseClassSpecifier(DeclSpec &DS);
  void ParseCXXMemberSpecification(SourceLocation StartLoc, unsigned TagType,
                                   DeclTy *TagDecl);
  DeclTy *ParseCXXClassMemberDeclaration(AccessSpecifier AS);
  void ParseConstructorInitializer(DeclTy *ConstructorDecl);
  MemInitResult ParseMemInitializer(DeclTy *ConstructorDecl);

  //===--------------------------------------------------------------------===//
  // C++ 10: Derived classes [class.derived]
  void ParseBaseClause(DeclTy *ClassDecl);
  BaseResult ParseBaseSpecifier(DeclTy *ClassDecl);
  AccessSpecifier getAccessSpecifierIfPresent() const;

  //===--------------------------------------------------------------------===//
  // C++ 13.5: Overloaded operators [over.oper]
  IdentifierInfo *MaybeParseOperatorFunctionId();
  TypeTy *ParseConversionFunctionId();
};

}  // end namespace clang

#endif

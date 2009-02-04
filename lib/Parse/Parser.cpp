//===--- Parser.cpp - C Language Family Parser ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the Parser interfaces.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/DeclSpec.h"
#include "clang/Parse/Scope.h"
#include "ExtensionRAIIObject.h"
#include "ParsePragma.h"
using namespace clang;

Parser::Parser(Preprocessor &pp, Action &actions)
  : PP(pp), Actions(actions), Diags(PP.getDiagnostics()) {
  Tok.setKind(tok::eof);
  CurScope = 0;
  NumCachedScopes = 0;
  ParenCount = BracketCount = BraceCount = 0;
  ObjCImpDecl = 0;

  // Add #pragma handlers. These are removed and destroyed in the
  // destructor.
  PackHandler =
    new PragmaPackHandler(&PP.getIdentifierTable().get("pack"), actions);
  PP.AddPragmaHandler(0, PackHandler);

  // Instantiate a LexedMethodsForTopClass for all the non-nested classes.
  PushTopClassStack();
}

///  Out-of-line virtual destructor to provide home for Action class.
ActionBase::~ActionBase() {}

///  Out-of-line virtual destructor to provide home for Action class.
Action::~Action() {}

// Defined out-of-line here because of dependecy on AttributeList
Action::DeclTy *Action::ActOnUsingDirective(Scope *CurScope,
                                            SourceLocation UsingLoc,
                                            SourceLocation NamespcLoc,
                                            const CXXScopeSpec &SS,
                                            SourceLocation IdentLoc,
                                            IdentifierInfo *NamespcName,
                                            AttributeList *AttrList) {

  // FIXME: Parser seems to assume that Action::ActOn* takes ownership over
  // passed AttributeList, however other actions don't free it, is it
  // temporary state or bug?
  delete AttrList;
  return 0;
}

DiagnosticBuilder Parser::Diag(SourceLocation Loc, unsigned DiagID) {
  return Diags.Report(FullSourceLoc(Loc,PP.getSourceManager()), DiagID);
}

DiagnosticBuilder Parser::Diag(const Token &Tok, unsigned DiagID) {
  return Diag(Tok.getLocation(), DiagID);
}

/// MatchRHSPunctuation - For punctuation with a LHS and RHS (e.g. '['/']'),
/// this helper function matches and consumes the specified RHS token if
/// present.  If not present, it emits the specified diagnostic indicating
/// that the parser failed to match the RHS of the token at LHSLoc.  LHSName
/// should be the name of the unmatched LHS token.
SourceLocation Parser::MatchRHSPunctuation(tok::TokenKind RHSTok,
                                           SourceLocation LHSLoc) {

  if (Tok.is(RHSTok))
    return ConsumeAnyToken();

  SourceLocation R = Tok.getLocation();
  const char *LHSName = "unknown";
  diag::kind DID = diag::err_parse_error;
  switch (RHSTok) {
  default: break;
  case tok::r_paren : LHSName = "("; DID = diag::err_expected_rparen; break;
  case tok::r_brace : LHSName = "{"; DID = diag::err_expected_rbrace; break;
  case tok::r_square: LHSName = "["; DID = diag::err_expected_rsquare; break;
  case tok::greater:  LHSName = "<"; DID = diag::err_expected_greater; break;
  }
  Diag(Tok, DID);
  Diag(LHSLoc, diag::note_matching) << LHSName;
  SkipUntil(RHSTok);
  return R;
}

/// ExpectAndConsume - The parser expects that 'ExpectedTok' is next in the
/// input.  If so, it is consumed and false is returned.
///
/// If the input is malformed, this emits the specified diagnostic.  Next, if
/// SkipToTok is specified, it calls SkipUntil(SkipToTok).  Finally, true is
/// returned.
bool Parser::ExpectAndConsume(tok::TokenKind ExpectedTok, unsigned DiagID,
                              const char *Msg, tok::TokenKind SkipToTok) {
  if (Tok.is(ExpectedTok)) {
    ConsumeAnyToken();
    return false;
  }

  Diag(Tok, DiagID) << Msg;
  if (SkipToTok != tok::unknown)
    SkipUntil(SkipToTok);
  return true;
}

//===----------------------------------------------------------------------===//
// Error recovery.
//===----------------------------------------------------------------------===//

/// SkipUntil - Read tokens until we get to the specified token, then consume
/// it (unless DontConsume is true).  Because we cannot guarantee that the
/// token will ever occur, this skips to the next token, or to some likely
/// good stopping point.  If StopAtSemi is true, skipping will stop at a ';'
/// character.
///
/// If SkipUntil finds the specified token, it returns true, otherwise it
/// returns false.
bool Parser::SkipUntil(const tok::TokenKind *Toks, unsigned NumToks,
                       bool StopAtSemi, bool DontConsume) {
  // We always want this function to skip at least one token if the first token
  // isn't T and if not at EOF.
  bool isFirstTokenSkipped = true;
  while (1) {
    // If we found one of the tokens, stop and return true.
    for (unsigned i = 0; i != NumToks; ++i) {
      if (Tok.is(Toks[i])) {
        if (DontConsume) {
          // Noop, don't consume the token.
        } else {
          ConsumeAnyToken();
        }
        return true;
      }
    }

    switch (Tok.getKind()) {
    case tok::eof:
      // Ran out of tokens.
      return false;

    case tok::l_paren:
      // Recursively skip properly-nested parens.
      ConsumeParen();
      SkipUntil(tok::r_paren, false);
      break;
    case tok::l_square:
      // Recursively skip properly-nested square brackets.
      ConsumeBracket();
      SkipUntil(tok::r_square, false);
      break;
    case tok::l_brace:
      // Recursively skip properly-nested braces.
      ConsumeBrace();
      SkipUntil(tok::r_brace, false);
      break;

    // Okay, we found a ']' or '}' or ')', which we think should be balanced.
    // Since the user wasn't looking for this token (if they were, it would
    // already be handled), this isn't balanced.  If there is a LHS token at a
    // higher level, we will assume that this matches the unbalanced token
    // and return it.  Otherwise, this is a spurious RHS token, which we skip.
    case tok::r_paren:
      if (ParenCount && !isFirstTokenSkipped)
        return false;  // Matches something.
      ConsumeParen();
      break;
    case tok::r_square:
      if (BracketCount && !isFirstTokenSkipped)
        return false;  // Matches something.
      ConsumeBracket();
      break;
    case tok::r_brace:
      if (BraceCount && !isFirstTokenSkipped)
        return false;  // Matches something.
      ConsumeBrace();
      break;

    case tok::string_literal:
    case tok::wide_string_literal:
      ConsumeStringToken();
      break;
    case tok::semi:
      if (StopAtSemi)
        return false;
      // FALL THROUGH.
    default:
      // Skip this token.
      ConsumeToken();
      break;
    }
    isFirstTokenSkipped = false;
  }
}

//===----------------------------------------------------------------------===//
// Scope manipulation
//===----------------------------------------------------------------------===//

/// EnterScope - Start a new scope.
void Parser::EnterScope(unsigned ScopeFlags) {
  if (NumCachedScopes) {
    Scope *N = ScopeCache[--NumCachedScopes];
    N->Init(CurScope, ScopeFlags);
    CurScope = N;
  } else {
    CurScope = new Scope(CurScope, ScopeFlags);
  }
}

/// ExitScope - Pop a scope off the scope stack.
void Parser::ExitScope() {
  assert(CurScope && "Scope imbalance!");

  // Inform the actions module that this scope is going away if there are any
  // decls in it.
  if (!CurScope->decl_empty())
    Actions.ActOnPopScope(Tok.getLocation(), CurScope);

  Scope *OldScope = CurScope;
  CurScope = OldScope->getParent();

  if (NumCachedScopes == ScopeCacheSize)
    delete OldScope;
  else
    ScopeCache[NumCachedScopes++] = OldScope;
}




//===----------------------------------------------------------------------===//
// C99 6.9: External Definitions.
//===----------------------------------------------------------------------===//

Parser::~Parser() {
  // If we still have scopes active, delete the scope tree.
  delete CurScope;

  // Free the scope cache.
  for (unsigned i = 0, e = NumCachedScopes; i != e; ++i)
    delete ScopeCache[i];

  // Remove the pragma handlers we installed.
  PP.RemovePragmaHandler(0, PackHandler);
  delete PackHandler;
}

/// Initialize - Warm up the parser.
///
void Parser::Initialize() {
  // Prime the lexer look-ahead.
  ConsumeToken();

  // Create the translation unit scope.  Install it as the current scope.
  assert(CurScope == 0 && "A scope is already active?");
  EnterScope(Scope::DeclScope);
  Actions.ActOnTranslationUnitScope(Tok.getLocation(), CurScope);

  if (Tok.is(tok::eof) &&
      !getLang().CPlusPlus)  // Empty source file is an extension in C
    Diag(Tok, diag::ext_empty_source_file);

  // Initialization for Objective-C context sensitive keywords recognition.
  // Referenced in Parser::ParseObjCTypeQualifierList.
  if (getLang().ObjC1) {
    ObjCTypeQuals[objc_in] = &PP.getIdentifierTable().get("in");
    ObjCTypeQuals[objc_out] = &PP.getIdentifierTable().get("out");
    ObjCTypeQuals[objc_inout] = &PP.getIdentifierTable().get("inout");
    ObjCTypeQuals[objc_oneway] = &PP.getIdentifierTable().get("oneway");
    ObjCTypeQuals[objc_bycopy] = &PP.getIdentifierTable().get("bycopy");
    ObjCTypeQuals[objc_byref] = &PP.getIdentifierTable().get("byref");
  }

  Ident_super = &PP.getIdentifierTable().get("super");
}

/// ParseTopLevelDecl - Parse one top-level declaration, return whatever the
/// action tells us to.  This returns true if the EOF was encountered.
bool Parser::ParseTopLevelDecl(DeclTy*& Result) {
  Result = 0;
  if (Tok.is(tok::eof)) {
    Actions.ActOnEndOfTranslationUnit();
    return true;
  }

  Result = ParseExternalDeclaration();
  return false;
}

/// ParseTranslationUnit:
///       translation-unit: [C99 6.9]
///         external-declaration
///         translation-unit external-declaration
void Parser::ParseTranslationUnit() {
  Initialize();

  DeclTy *Res;
  while (!ParseTopLevelDecl(Res))
    /*parse them all*/;
  
  ExitScope();
  assert(CurScope == 0 && "Scope imbalance!");
}

/// ParseExternalDeclaration:
///
///       external-declaration: [C99 6.9], declaration: [C++ dcl.dcl]
///         function-definition
///         declaration
/// [EXT]   ';'
/// [GNU]   asm-definition
/// [GNU]   __extension__ external-declaration
/// [OBJC]  objc-class-definition
/// [OBJC]  objc-class-declaration
/// [OBJC]  objc-alias-declaration
/// [OBJC]  objc-protocol-definition
/// [OBJC]  objc-method-definition
/// [OBJC]  @end
/// [C++]   linkage-specification
/// [GNU] asm-definition:
///         simple-asm-expr ';'
///
Parser::DeclTy *Parser::ParseExternalDeclaration() {
  switch (Tok.getKind()) {
  case tok::semi:
    Diag(Tok, diag::ext_top_level_semi);
    ConsumeToken();
    // TODO: Invoke action for top-level semicolon.
    return 0;
  case tok::r_brace:
    Diag(Tok, diag::err_expected_external_declaration);
    ConsumeBrace();
    return 0;
  case tok::eof:
    Diag(Tok, diag::err_expected_external_declaration);
    return 0;
  case tok::kw___extension__: {
    // __extension__ silences extension warnings in the subexpression.
    ExtensionRAIIObject O(Diags);  // Use RAII to do this.
    ConsumeToken();
    return ParseExternalDeclaration();
  }
  case tok::kw_asm: {
    OwningExprResult Result(ParseSimpleAsm());

    ExpectAndConsume(tok::semi, diag::err_expected_semi_after,
                     "top-level asm block");

    if (!Result.isInvalid())
      return Actions.ActOnFileScopeAsmDecl(Tok.getLocation(), move_arg(Result));
    return 0;
  }
  case tok::at:
    // @ is not a legal token unless objc is enabled, no need to check.
    return ParseObjCAtDirectives();
  case tok::minus:
  case tok::plus:
    if (getLang().ObjC1)
      return ParseObjCMethodDefinition();
    else {
      Diag(Tok, diag::err_expected_external_declaration);
      ConsumeToken();
    }
    return 0;
  case tok::kw_using:
  case tok::kw_namespace:
  case tok::kw_typedef:
  case tok::kw_template:
  case tok::kw_export:    // As in 'export template'
    // A function definition cannot start with a these keywords.
    return ParseDeclaration(Declarator::FileContext);
       
  default:
    // We can't tell whether this is a function-definition or declaration yet.
    return ParseDeclarationOrFunctionDefinition();
  }
}

/// ParseDeclarationOrFunctionDefinition - Parse either a function-definition or
/// a declaration.  We can't tell which we have until we read up to the
/// compound-statement in function-definition. TemplateParams, if
/// non-NULL, provides the template parameters when we're parsing a
/// C++ template-declaration. 
///
///       function-definition: [C99 6.9.1]
///         decl-specs      declarator declaration-list[opt] compound-statement
/// [C90] function-definition: [C99 6.7.1] - implicit int result
/// [C90]   decl-specs[opt] declarator declaration-list[opt] compound-statement
///
///       declaration: [C99 6.7]
///         declaration-specifiers init-declarator-list[opt] ';'
/// [!C99]  init-declarator-list ';'                   [TODO: warn in c99 mode]
/// [OMP]   threadprivate-directive                              [TODO]
///
Parser::DeclTy *
Parser::ParseDeclarationOrFunctionDefinition(
                                  TemplateParameterLists *TemplateParams) {
  // Parse the common declaration-specifiers piece.
  DeclSpec DS;
  ParseDeclarationSpecifiers(DS, TemplateParams);

  // C99 6.7.2.3p6: Handle "struct-or-union identifier;", "enum { X };"
  // declaration-specifiers init-declarator-list[opt] ';'
  if (Tok.is(tok::semi)) {
    ConsumeToken();
    return Actions.ParsedFreeStandingDeclSpec(CurScope, DS);
  }

  // ObjC2 allows prefix attributes on class interfaces and protocols.
  // FIXME: This still needs better diagnostics. We should only accept
  // attributes here, no types, etc.
  if (getLang().ObjC2 && Tok.is(tok::at)) {
    SourceLocation AtLoc = ConsumeToken(); // the "@"
    if (!Tok.isObjCAtKeyword(tok::objc_interface) && 
        !Tok.isObjCAtKeyword(tok::objc_protocol)) {
      Diag(Tok, diag::err_objc_unexpected_attr);
      SkipUntil(tok::semi); // FIXME: better skip?
      return 0;
    }
    const char *PrevSpec = 0;
    if (DS.SetTypeSpecType(DeclSpec::TST_unspecified, AtLoc, PrevSpec))
      Diag(AtLoc, diag::err_invalid_decl_spec_combination) << PrevSpec;
    if (Tok.isObjCAtKeyword(tok::objc_protocol))
      return ParseObjCAtProtocolDeclaration(AtLoc, DS.getAttributes());
    return ParseObjCAtInterfaceDeclaration(AtLoc, DS.getAttributes());
  }

  // If the declspec consisted only of 'extern' and we have a string
  // literal following it, this must be a C++ linkage specifier like
  // 'extern "C"'.
  if (Tok.is(tok::string_literal) && getLang().CPlusPlus &&
      DS.getStorageClassSpec() == DeclSpec::SCS_extern &&
      DS.getParsedSpecifiers() == DeclSpec::PQ_StorageClassSpecifier)
    return ParseLinkage(Declarator::FileContext);

  // Parse the first declarator.
  Declarator DeclaratorInfo(DS, Declarator::FileContext);
  ParseDeclarator(DeclaratorInfo);
  // Error parsing the declarator?
  if (!DeclaratorInfo.hasName()) {
    // If so, skip until the semi-colon or a }.
    SkipUntil(tok::r_brace, true, true);
    if (Tok.is(tok::semi))
      ConsumeToken();
    return 0;
  }

  // If the declarator is the start of a function definition, handle it.
  if (Tok.is(tok::equal) ||           // int X()=  -> not a function def
      Tok.is(tok::comma) ||           // int X(),  -> not a function def
      Tok.is(tok::semi)  ||           // int X();  -> not a function def
      Tok.is(tok::kw_asm) ||          // int X() __asm__ -> not a function def
      Tok.is(tok::kw___attribute) ||  // int X() __attr__ -> not a function def
      (getLang().CPlusPlus &&
       Tok.is(tok::l_paren)) ) {      // int X(0) -> not a function def [C++]
    // FALL THROUGH.
  } else if (DeclaratorInfo.isFunctionDeclarator() &&
             (Tok.is(tok::l_brace) ||             // int X() {}
              ( !getLang().CPlusPlus &&
                isDeclarationSpecifier() ))) {    // int X(f) int f; {}
    if (DS.getStorageClassSpec() == DeclSpec::SCS_typedef) {
      Diag(Tok, diag::err_function_declared_typedef);

      if (Tok.is(tok::l_brace)) {
        // This recovery skips the entire function body. It would be nice
        // to simply call ParseFunctionDefinition() below, however Sema
        // assumes the declarator represents a function, not a typedef.
        ConsumeBrace();
        SkipUntil(tok::r_brace, true);
      } else {
        SkipUntil(tok::semi);
      }
      return 0;
    }
    return ParseFunctionDefinition(DeclaratorInfo);
  } else {
    if (DeclaratorInfo.isFunctionDeclarator())
      Diag(Tok, diag::err_expected_fn_body);
    else
      Diag(Tok, diag::err_expected_after_declarator);
    SkipUntil(tok::semi);
    return 0;
  }

  // Parse the init-declarator-list for a normal declaration.
  return ParseInitDeclaratorListAfterFirstDeclarator(DeclaratorInfo);
}

/// ParseFunctionDefinition - We parsed and verified that the specified
/// Declarator is well formed.  If this is a K&R-style function, read the
/// parameters declaration-list, then start the compound-statement.
///
///       function-definition: [C99 6.9.1]
///         decl-specs      declarator declaration-list[opt] compound-statement
/// [C90] function-definition: [C99 6.7.1] - implicit int result
/// [C90]   decl-specs[opt] declarator declaration-list[opt] compound-statement
/// [C++] function-definition: [C++ 8.4]
///         decl-specifier-seq[opt] declarator ctor-initializer[opt] function-body
/// [C++] function-definition: [C++ 8.4]
///         decl-specifier-seq[opt] declarator function-try-block [TODO]
///
Parser::DeclTy *Parser::ParseFunctionDefinition(Declarator &D) {
  const DeclaratorChunk &FnTypeInfo = D.getTypeObject(0);
  assert(FnTypeInfo.Kind == DeclaratorChunk::Function &&
         "This isn't a function declarator!");
  const DeclaratorChunk::FunctionTypeInfo &FTI = FnTypeInfo.Fun;

  // If this is C90 and the declspecs were completely missing, fudge in an
  // implicit int.  We do this here because this is the only place where
  // declaration-specifiers are completely optional in the grammar.
  if (getLang().ImplicitInt && D.getDeclSpec().getParsedSpecifiers() == 0) {
    const char *PrevSpec;
    D.getMutableDeclSpec().SetTypeSpecType(DeclSpec::TST_int,
                                           D.getIdentifierLoc(),
                                           PrevSpec);
  }

  // If this declaration was formed with a K&R-style identifier list for the
  // arguments, parse declarations for all of the args next.
  // int foo(a,b) int a; float b; {}
  if (!FTI.hasPrototype && FTI.NumArgs != 0)
    ParseKNRParamDeclarations(D);

  // We should have either an opening brace or, in a C++ constructor,
  // we may have a colon.
  // FIXME: In C++, we might also find the 'try' keyword.
  if (Tok.isNot(tok::l_brace) && Tok.isNot(tok::colon)) {
    Diag(Tok, diag::err_expected_fn_body);

    // Skip over garbage, until we get to '{'.  Don't eat the '{'.
    SkipUntil(tok::l_brace, true, true);

    // If we didn't find the '{', bail out.
    if (Tok.isNot(tok::l_brace))
      return 0;
  }

  // Enter a scope for the function body.
  ParseScope BodyScope(this, Scope::FnScope|Scope::DeclScope);

  // Tell the actions module that we have entered a function definition with the
  // specified Declarator for the function.
  DeclTy *Res = Actions.ActOnStartOfFunctionDef(CurScope, D);

  // If we have a colon, then we're probably parsing a C++
  // ctor-initializer.
  if (Tok.is(tok::colon))
    ParseConstructorInitializer(Res);

  SourceLocation BraceLoc = Tok.getLocation();
  return ParseFunctionStatementBody(Res, BraceLoc, BraceLoc);
}

/// ParseKNRParamDeclarations - Parse 'declaration-list[opt]' which provides
/// types for a function with a K&R-style identifier list for arguments.
void Parser::ParseKNRParamDeclarations(Declarator &D) {
  // We know that the top-level of this declarator is a function.
  DeclaratorChunk::FunctionTypeInfo &FTI = D.getTypeObject(0).Fun;

  // Enter function-declaration scope, limiting any declarators to the
  // function prototype scope, including parameter declarators.
  ParseScope PrototypeScope(this, Scope::FunctionPrototypeScope|Scope::DeclScope);

  // Read all the argument declarations.
  while (isDeclarationSpecifier()) {
    SourceLocation DSStart = Tok.getLocation();

    // Parse the common declaration-specifiers piece.
    DeclSpec DS;
    ParseDeclarationSpecifiers(DS);

    // C99 6.9.1p6: 'each declaration in the declaration list shall have at
    // least one declarator'.
    // NOTE: GCC just makes this an ext-warn.  It's not clear what it does with
    // the declarations though.  It's trivial to ignore them, really hard to do
    // anything else with them.
    if (Tok.is(tok::semi)) {
      Diag(DSStart, diag::err_declaration_does_not_declare_param);
      ConsumeToken();
      continue;
    }

    // C99 6.9.1p6: Declarations shall contain no storage-class specifiers other
    // than register.
    if (DS.getStorageClassSpec() != DeclSpec::SCS_unspecified &&
        DS.getStorageClassSpec() != DeclSpec::SCS_register) {
      Diag(DS.getStorageClassSpecLoc(),
           diag::err_invalid_storage_class_in_func_decl);
      DS.ClearStorageClassSpecs();
    }
    if (DS.isThreadSpecified()) {
      Diag(DS.getThreadSpecLoc(),
           diag::err_invalid_storage_class_in_func_decl);
      DS.ClearStorageClassSpecs();
    }

    // Parse the first declarator attached to this declspec.
    Declarator ParmDeclarator(DS, Declarator::KNRTypeListContext);
    ParseDeclarator(ParmDeclarator);

    // Handle the full declarator list.
    while (1) {
      DeclTy *AttrList;
      // If attributes are present, parse them.
      if (Tok.is(tok::kw___attribute))
        // FIXME: attach attributes too.
        AttrList = ParseAttributes();

      // Ask the actions module to compute the type for this declarator.
      Action::DeclTy *Param =
        Actions.ActOnParamDeclarator(CurScope, ParmDeclarator);

      if (Param &&
          // A missing identifier has already been diagnosed.
          ParmDeclarator.getIdentifier()) {

        // Scan the argument list looking for the correct param to apply this
        // type.
        for (unsigned i = 0; ; ++i) {
          // C99 6.9.1p6: those declarators shall declare only identifiers from
          // the identifier list.
          if (i == FTI.NumArgs) {
            Diag(ParmDeclarator.getIdentifierLoc(), diag::err_no_matching_param)
              << ParmDeclarator.getIdentifier();
            break;
          }

          if (FTI.ArgInfo[i].Ident == ParmDeclarator.getIdentifier()) {
            // Reject redefinitions of parameters.
            if (FTI.ArgInfo[i].Param) {
              Diag(ParmDeclarator.getIdentifierLoc(),
                   diag::err_param_redefinition)
                 << ParmDeclarator.getIdentifier();
            } else {
              FTI.ArgInfo[i].Param = Param;
            }
            break;
          }
        }
      }

      // If we don't have a comma, it is either the end of the list (a ';') or
      // an error, bail out.
      if (Tok.isNot(tok::comma))
        break;

      // Consume the comma.
      ConsumeToken();

      // Parse the next declarator.
      ParmDeclarator.clear();
      ParseDeclarator(ParmDeclarator);
    }

    if (Tok.is(tok::semi)) {
      ConsumeToken();
    } else {
      Diag(Tok, diag::err_parse_error);
      // Skip to end of block or statement
      SkipUntil(tok::semi, true);
      if (Tok.is(tok::semi))
        ConsumeToken();
    }
  }

  // The actions module must verify that all arguments were declared.
  Actions.ActOnFinishKNRParamDeclarations(CurScope, D);
}


/// ParseAsmStringLiteral - This is just a normal string-literal, but is not
/// allowed to be a wide string, and is not subject to character translation.
///
/// [GNU] asm-string-literal:
///         string-literal
///
Parser::OwningExprResult Parser::ParseAsmStringLiteral() {
  if (!isTokenStringLiteral()) {
    Diag(Tok, diag::err_expected_string_literal);
    return ExprError();
  }

  OwningExprResult Res(ParseStringLiteralExpression());
  if (Res.isInvalid()) return move(Res);

  // TODO: Diagnose: wide string literal in 'asm'

  return move(Res);
}

/// ParseSimpleAsm
///
/// [GNU] simple-asm-expr:
///         'asm' '(' asm-string-literal ')'
///
Parser::OwningExprResult Parser::ParseSimpleAsm() {
  assert(Tok.is(tok::kw_asm) && "Not an asm!");
  SourceLocation Loc = ConsumeToken();

  if (Tok.isNot(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << "asm";
    return ExprError();
  }

  ConsumeParen();

  OwningExprResult Result(ParseAsmStringLiteral());

  if (Result.isInvalid())
    SkipUntil(tok::r_paren);
  else
    MatchRHSPunctuation(tok::r_paren, Loc);

  return move(Result);
}

/// TryAnnotateTypeOrScopeToken - If the current token position is on a
/// typename (possibly qualified in C++) or a C++ scope specifier not followed
/// by a typename, TryAnnotateTypeOrScopeToken will replace one or more tokens
/// with a single annotation token representing the typename or C++ scope
/// respectively.
/// This simplifies handling of C++ scope specifiers and allows efficient
/// backtracking without the need to re-parse and resolve nested-names and
/// typenames.
/// It will mainly be called when we expect to treat identifiers as typenames
/// (if they are typenames). For example, in C we do not expect identifiers
/// inside expressions to be treated as typenames so it will not be called
/// for expressions in C.
/// The benefit for C/ObjC is that a typename will be annotated and
/// Actions.getTypeName will not be needed to be called again (e.g. getTypeName
/// will not be called twice, once to check whether we have a declaration
/// specifier, and another one to get the actual type inside
/// ParseDeclarationSpecifiers).
///
/// This returns true if the token was annotated.
/// 
/// Note that this routine emits an error if you call it with ::new or ::delete
/// as the current tokens, so only call it in contexts where these are invalid.
bool Parser::TryAnnotateTypeOrScopeToken() {
  assert((Tok.is(tok::identifier) || Tok.is(tok::coloncolon)) &&
         "Cannot be a type or scope token!");
  
  // FIXME: Implement template-ids
  CXXScopeSpec SS;
  if (getLang().CPlusPlus)
    ParseOptionalCXXScopeSpecifier(SS);

  if (Tok.is(tok::identifier)) {
    // Determine whether the identifier is a type name.
    if (TypeTy *Ty = Actions.getTypeName(*Tok.getIdentifierInfo(), 
                                         Tok.getLocation(), CurScope, &SS)) {
      // This is a typename. Replace the current token in-place with an
      // annotation type token.
      Tok.setKind(tok::annot_typename);
      Tok.setAnnotationValue(Ty);
      Tok.setAnnotationEndLoc(Tok.getLocation());
      if (SS.isNotEmpty()) // it was a C++ qualified type name.
        Tok.setLocation(SS.getBeginLoc());
      
      // In case the tokens were cached, have Preprocessor replace
      // them with the annotation token.
      PP.AnnotateCachedTokens(Tok);
      return true;
    } else if (!getLang().CPlusPlus) {
      // If we're in C, we can't have :: tokens at all (the lexer won't return
      // them).  If the identifier is not a type, then it can't be scope either,
      // just early exit. 
      return false;
    }
    
    // If this is a template-id, annotate the template-id token.
    if (NextToken().is(tok::less))
      if (DeclTy *Template =
          Actions.isTemplateName(*Tok.getIdentifierInfo(), CurScope, &SS))
        AnnotateTemplateIdToken(Template, &SS);

    // We either have an identifier that is not a type name or we have
    // just created a template-id that might be a type name. Both
    // cases will be handled below.
  }

  // FIXME: check for a template-id token here, and look it up if it
  // names a type.

  if (SS.isEmpty())
    return false;
  
  // A C++ scope specifier that isn't followed by a typename.
  // Push the current token back into the token stream (or revert it if it is
  // cached) and use an annotation scope token for current token.
  if (PP.isBacktrackEnabled())
    PP.RevertCachedTokens(1);
  else
    PP.EnterToken(Tok);
  Tok.setKind(tok::annot_cxxscope);
  Tok.setAnnotationValue(SS.getScopeRep());
  Tok.setAnnotationRange(SS.getRange());

  // In case the tokens were cached, have Preprocessor replace them with the
  // annotation token.
  PP.AnnotateCachedTokens(Tok);
  return true;
}

/// TryAnnotateScopeToken - Like TryAnnotateTypeOrScopeToken but only
/// annotates C++ scope specifiers.  This returns true if the token was
/// annotated.
/// 
/// Note that this routine emits an error if you call it with ::new or ::delete
/// as the current tokens, so only call it in contexts where these are invalid.
bool Parser::TryAnnotateCXXScopeToken() {
  assert(getLang().CPlusPlus &&
         "Call sites of this function should be guarded by checking for C++");
  assert((Tok.is(tok::identifier) || Tok.is(tok::coloncolon)) &&
         "Cannot be a type or scope token!");

  CXXScopeSpec SS;
  if (!ParseOptionalCXXScopeSpecifier(SS))
    return false;

  // Push the current token back into the token stream (or revert it if it is
  // cached) and use an annotation scope token for current token.
  if (PP.isBacktrackEnabled())
    PP.RevertCachedTokens(1);
  else
    PP.EnterToken(Tok);
  Tok.setKind(tok::annot_cxxscope);
  Tok.setAnnotationValue(SS.getScopeRep());
  Tok.setAnnotationRange(SS.getRange());

  // In case the tokens were cached, have Preprocessor replace them with the
  // annotation token.
  PP.AnnotateCachedTokens(Tok);
  return true;
}

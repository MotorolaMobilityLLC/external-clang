//===--- PTHLexer.cpp - Lex from a token stream ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the PTHLexer interface.
//
//===----------------------------------------------------------------------===//

#include "clang/Lex/PTHLexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/TokenKinds.h"
using namespace clang;

PTHLexer::PTHLexer(Preprocessor& pp, SourceLocation fileloc,
                   const Token *TokArray, unsigned NumTokens)
  : PreprocessorLexer(&pp, fileloc),
    Tokens(TokArray),
    LastTokenIdx(NumTokens - 1),
    CurTokenIdx(0) {

  assert(NumTokens >= 1);
  assert(Tokens[LastTokenIdx].is(tok::eof));
}

void PTHLexer::Lex(Token& Tok) {
LexNextToken:
  if (AtLastToken()) {
    if (ParsingPreprocessorDirective) {
      ParsingPreprocessorDirective = false;
      Tok = GetToken();
      Tok.setKind(tok::eom);
      MIOpt.ReadToken();
      return;
    }
    
    assert(!LexingRawMode && "PTHLexer cannot lex in raw mode.");
    
    // FIXME: Issue diagnostics similar to Lexer.
    PP->HandleEndOfFile(Tok, false);    
    return;
  }

  Tok = GetToken();
  
  // Don't advance to the next token yet.  Check if we are at the
  // start of a new line and we're processing a directive.  If so, we
  // consume this token twice, once as an tok::eom.
  if (Tok.isAtStartOfLine() && ParsingPreprocessorDirective) {
    ParsingPreprocessorDirective = false;
    Tok.setKind(tok::eom);
    MIOpt.ReadToken();
    return;
  }
  
  // Advance to the next token.
  AdvanceToken();
    
  if (Tok.is(tok::hash)) {    
    if (Tok.isAtStartOfLine() && !LexingRawMode) {
      PP->HandleDirective(Tok);

      if (PP->isCurrentLexer(this))
        goto LexNextToken;

      return PP->Lex(Tok);
    }
  }

  MIOpt.ReadToken();
  
  if (Tok.is(tok::identifier)) {
    if (LexingRawMode) return;
    return PP->HandleIdentifier(Tok);
  }  
}

void PTHLexer::setEOF(Token& Tok) {
  Tok = Tokens[LastTokenIdx];
}

void PTHLexer::DiscardToEndOfLine() {
  assert(ParsingPreprocessorDirective && ParsingFilename == false &&
         "Must be in a preprocessing directive!");

  // Already at end-of-file?
  if (AtLastToken())
    return;

  // Find the first token that is not the start of the *current* line.
  for (AdvanceToken(); !AtLastToken(); AdvanceToken())
    if (GetToken().isAtStartOfLine())
      return;
}

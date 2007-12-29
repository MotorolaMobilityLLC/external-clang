//===--- MacroExpander.h - Lex from a macro expansion -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MacroExpander and MacroArgs interfaces.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_MACROEXPANDER_H
#define LLVM_CLANG_MACROEXPANDER_H

#include "clang/Basic/SourceLocation.h"
#include <vector>

namespace clang {
  class MacroInfo;
  class Preprocessor;
  class Token;

/// MacroArgs - An instance of this class captures information about
/// the formal arguments specified to a function-like macro invocation.
class MacroArgs {
  /// NumUnexpArgTokens - The number of raw, unexpanded tokens for the
  /// arguments.  All of the actual argument tokens are allocated immediately
  /// after the MacroArgs object in memory.  This is all of the arguments
  /// concatenated together, with 'EOF' markers at the end of each argument.
  unsigned NumUnexpArgTokens;

  /// PreExpArgTokens - Pre-expanded tokens for arguments that need them.  Empty
  /// if not yet computed.  This includes the EOF marker at the end of the
  /// stream.
  std::vector<std::vector<Token> > PreExpArgTokens;

  /// StringifiedArgs - This contains arguments in 'stringified' form.  If the
  /// stringified form of an argument has not yet been computed, this is empty.
  std::vector<Token> StringifiedArgs;

  /// VarargsElided - True if this is a C99 style varargs macro invocation and
  /// there was no argument specified for the "..." argument.  If the argument
  /// was specified (even empty) or this isn't a C99 style varargs function, or
  /// if in strict mode and the C99 varargs macro had only a ... argument, this
  /// is false.
  bool VarargsElided;
  
  MacroArgs(unsigned NumToks, bool varargsElided)
    : NumUnexpArgTokens(NumToks), VarargsElided(varargsElided) {}
  ~MacroArgs() {}
public:
  /// MacroArgs ctor function - Create a new MacroArgs object with the specified
  /// macro and argument info.
  static MacroArgs *create(const MacroInfo *MI,
                           const Token *UnexpArgTokens,
                           unsigned NumArgTokens, bool VarargsElided);
  
  /// destroy - Destroy and deallocate the memory for this object.
  ///
  void destroy();
  
  /// ArgNeedsPreexpansion - If we can prove that the argument won't be affected
  /// by pre-expansion, return false.  Otherwise, conservatively return true.
  bool ArgNeedsPreexpansion(const Token *ArgTok, Preprocessor &PP) const;
  
  /// getUnexpArgument - Return a pointer to the first token of the unexpanded
  /// token list for the specified formal.
  ///
  const Token *getUnexpArgument(unsigned Arg) const;
  
  /// getArgLength - Given a pointer to an expanded or unexpanded argument,
  /// return the number of tokens, not counting the EOF, that make up the
  /// argument.
  static unsigned getArgLength(const Token *ArgPtr);
  
  /// getPreExpArgument - Return the pre-expanded form of the specified
  /// argument.
  const std::vector<Token> &
    getPreExpArgument(unsigned Arg, Preprocessor &PP);  
  
  /// getStringifiedArgument - Compute, cache, and return the specified argument
  /// that has been 'stringified' as required by the # operator.
  const Token &getStringifiedArgument(unsigned ArgNo, Preprocessor &PP);
  
  /// getNumArguments - Return the number of arguments passed into this macro
  /// invocation.
  unsigned getNumArguments() const { return NumUnexpArgTokens; }
  
  
  /// isVarargsElidedUse - Return true if this is a C99 style varargs macro
  /// invocation and there was no argument specified for the "..." argument.  If
  /// the argument was specified (even empty) or this isn't a C99 style varargs
  /// function, or if in strict mode and the C99 varargs macro had only a ...
  /// argument, this returns false.
  bool isVarargsElidedUse() const { return VarargsElided; }
};

  
/// MacroExpander - This implements a lexer that returns token from a macro body
/// or token stream instead of lexing from a character buffer.
///
class MacroExpander {
  /// Macro - The macro we are expanding from.  This is null if expanding a
  /// token stream.
  ///
  MacroInfo *Macro;

  /// ActualArgs - The actual arguments specified for a function-like macro, or
  /// null.  The MacroExpander owns the pointed-to object.
  MacroArgs *ActualArgs;

  /// PP - The current preprocessor object we are expanding for.
  ///
  Preprocessor &PP;

  /// MacroTokens - This is the pointer to an array of tokens that the macro is
  /// defined to, with arguments expanded for function-like macros.  If this is
  /// a token stream, these are the tokens we are returning.
  const Token *MacroTokens;
  
  /// NumMacroTokens - This is the length of the MacroTokens array.
  ///
  unsigned NumMacroTokens;
  
  /// CurToken - This is the next token that Lex will return.
  ///
  unsigned CurToken;
  
  /// InstantiateLoc - The source location where this macro was instantiated.
  ///
  SourceLocation InstantiateLoc;
  
  /// Lexical information about the expansion point of the macro: the identifier
  /// that the macro expanded from had these properties.
  bool AtStartOfLine : 1;
  bool HasLeadingSpace : 1;
  
  /// OwnsMacroTokens - This is true if this macroexpander allocated the
  /// MacroTokens array, and thus needs to free it when destroyed.
  bool OwnsMacroTokens : 1;
  
  MacroExpander(const MacroExpander&);  // DO NOT IMPLEMENT
  void operator=(const MacroExpander&); // DO NOT IMPLEMENT
public:
  /// Create a macro expander for the specified macro with the specified actual
  /// arguments.  Note that this ctor takes ownership of the ActualArgs pointer.
  MacroExpander(Token &Tok, MacroArgs *ActualArgs, Preprocessor &pp)
    : Macro(0), ActualArgs(0), PP(pp), OwnsMacroTokens(false) {
    Init(Tok, ActualArgs);
  }
  
  /// Init - Initialize this macro expander to expand from the specified macro
  /// with the specified argument information.  Note that this ctor takes
  /// ownership of the ActualArgs pointer.
  void Init(Token &Tok, MacroArgs *ActualArgs);
  
  /// Create a macro expander for the specified token stream.  This does not
  /// take ownership of the specified token vector.
  MacroExpander(const Token *TokArray, unsigned NumToks, Preprocessor &pp)
    : Macro(0), ActualArgs(0), PP(pp), OwnsMacroTokens(false) {
    Init(TokArray, NumToks);
  }
  
  /// Init - Initialize this macro expander with the specified token stream.
  /// This does not take ownership of the specified token vector.
  void Init(const Token *TokArray, unsigned NumToks);
  
  ~MacroExpander() { destroy(); }
  
  /// isNextTokenLParen - If the next token lexed will pop this macro off the
  /// expansion stack, return 2.  If the next unexpanded token is a '(', return
  /// 1, otherwise return 0.
  unsigned isNextTokenLParen() const;
  
  /// Lex - Lex and return a token from this macro stream.
  void Lex(Token &Tok);
  
private:
  void destroy();
  
  /// isAtEnd - Return true if the next lex call will pop this macro off the
  /// include stack.
  bool isAtEnd() const {
    return CurToken == NumMacroTokens;
  }
  
  /// PasteTokens - Tok is the LHS of a ## operator, and CurToken is the ##
  /// operator.  Read the ## and RHS, and paste the LHS/RHS together.  If there
  /// are is another ## after it, chomp it iteratively.  Return the result as
  /// Tok.
  void PasteTokens(Token &Tok);
  
  /// Expand the arguments of a function-like macro so that we can quickly
  /// return preexpanded tokens from MacroTokens.
  void ExpandFunctionArguments();
};

}  // end namespace clang

#endif

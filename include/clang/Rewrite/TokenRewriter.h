//===--- TokenRewriter.h - Token-based Rewriter -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the TokenRewriter class, which is used for code
//  transformations.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOKENREWRITER_H
#define LLVM_CLANG_TOKENREWRITER_H

#include "clang/Basic/SourceLocation.h"
#include <list>
#include <map>

namespace clang {
  class Token;
  class LangOptions;
  
  class TokenRewriter {
    /// TokenList - This is the list of raw tokens that make up this file.  Each
    /// of these tokens has a unique SourceLocation, which is a FileID.
    std::list<Token> TokenList;

    /// TokenRefTy - This is the type used to refer to a token in the TokenList.
    typedef std::list<Token>::iterator TokenRefTy;
    
    /// TokenAtLoc - This map indicates which token exists at a specific
    /// SourceLocation.  Since each token has a unique SourceLocation, this is a
    /// one to one map.  The token can return its own location directly, to map
    /// backwards.
    std::map<SourceLocation, TokenRefTy> TokenAtLoc;
    
  public:
    /// TokenRewriter - This creates a TokenRewriter for the file with the
    /// specified FileID.
    TokenRewriter(unsigned FileID, SourceManager &SM, const LangOptions &LO);
    
    
    typedef std::list<Token>::const_iterator token_iterator;
    token_iterator token_begin() const { return TokenList.begin(); }
    token_iterator token_end() const { return TokenList.end(); }
    
  private:
    /// AddToken - Add the specified token into the Rewriter before the other
    /// position.
    void AddToken(const Token &T, TokenRefTy Where);
  };
  
  
  
} // end namespace clang

#endif

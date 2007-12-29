//===--- ScratchBuffer.h - Scratch space for forming tokens -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the ScratchBuffer interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SCRATCHBUFFER_H
#define LLVM_CLANG_SCRATCHBUFFER_H

namespace clang {
  class SourceManager;
  class SourceLocation;

/// ScratchBuffer - This class exposes a simple interface for the dynamic
/// construction of tokens.  This is used for builtin macros (e.g. __LINE__) as
/// well as token pasting, etc.
class ScratchBuffer {
  SourceManager &SourceMgr;
  char *CurBuffer;
  unsigned FileID;
  unsigned BytesUsed;
public:
  ScratchBuffer(SourceManager &SM);
  
  /// getToken - Splat the specified text into a temporary MemoryBuffer and
  /// return a SourceLocation that refers to the token.  The SourceLoc value
  /// gives a virtual location that the token will appear to be from.
  SourceLocation getToken(const char *Buf, unsigned Len,
                          SourceLocation SourceLoc);
  
  /// getToken - Splat the specified text into a temporary MemoryBuffer and
  /// return a SourceLocation that refers to the token.  This is just like the
  /// previous method, but returns a location that indicates the physloc of the
  /// token.
  SourceLocation getToken(const char *Buf, unsigned Len);
                          
private:
  void AllocScratchBuffer(unsigned RequestLen);
};

} // end namespace clang

#endif

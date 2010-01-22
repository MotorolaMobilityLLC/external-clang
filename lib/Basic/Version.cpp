//===- Version.cpp - Clang Version Number -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines several version-related utility functions for Clang.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include <cstring>
#include <cstdlib>

using namespace std;

namespace clang {
  
llvm::StringRef getClangRepositoryPath() {
  static const char *Path = 0;
  if (Path)
    return Path;
  
  static char URL[] = "$URL$";
  char *End = strstr(URL, "/lib/Basic");
  if (End)
    *End = 0;
  
  End = strstr(URL, "/clang/tools/clang");
  if (End)
    *End = 0;
  
  char *Begin = strstr(URL, "cfe/");
  if (Begin) {
    Path = Begin + 4;
    return Path;
  }
  
  Path = URL;
  return Path;
}


llvm::StringRef getClangRevision() {
#ifndef SVN_REVISION
  // Subversion was not available at build time?
  return llvm::StringRef();
#else
  static std::string revision;
  if (revision.empty()) {
    llvm::raw_string_ostream Out(revision);
    Out << strtol(SVN_REVISION, 0, 10);
  }
  return revision;
#endif
}

llvm::StringRef getClangFullRepositoryVersion() {
  static std::string buf;
  if (buf.empty()) {
    llvm::raw_string_ostream Out(buf);
    Out << getClangRepositoryPath();
    llvm::StringRef Revision = getClangRevision();
    if (!Revision.empty())
      Out << ' ' << Revision;
  }
  return buf;
}
  
} // end namespace clang

// RUN: clang-cc %s -fsyntax-only -pedantic -verify
// PR4290

// The following declaration is compatible with vfprintf, so we shouldn't
// warn.
int vfprintf();

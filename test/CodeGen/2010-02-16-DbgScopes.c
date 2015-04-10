// RUN: %clang_cc1 -emit-llvm -g < %s | FileCheck %s
// Test to check number of lexical scope identified in debug info.
// CHECK: !MDLexicalBlock(
// CHECK: !MDLexicalBlock(
// CHECK: !MDLexicalBlock(
// CHECK: !MDLexicalBlock(

extern int bar();
extern void foobar();
void foo(int s) {
  unsigned loc = 0;
  if (s) {
    if (bar()) {
      foobar();
    }
  } else {
    loc = 1;
    if (bar()) {
      loc = 2;
    }
  }
}

// RUN: %clang_cc1 %s -emit-llvm -o - | opt -O3 | llc | \
// RUN:    not grep _foo2

void foo() __asm__("foo2");

void bar() {
  foo();
}

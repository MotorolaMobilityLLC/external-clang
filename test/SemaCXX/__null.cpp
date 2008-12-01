// RUN: clang %s -fsyntax-only -verify

void f() {
  int* i = __null;
  i = __null;
  int i2 = __null;

  // Verify statically that __null is the right size
  int a[sizeof(typeof(__null)) == sizeof(void*)? 1 : -1];
}

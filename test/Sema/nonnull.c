// RUN: clang -fsyntax-only -verify %s

int f1(int x) __attribute__((nonnull));
int f2(int *x) __attribute__ ((nonnull (1)));
int f3(int *x) __attribute__ ((nonnull (0))); // expected-error {{'nonnull' attribute parameter 1 is out of bounds}}
int f4(int *x, int *y) __attribute__ ((nonnull (1,2)));
int f5(int *x, int *y) __attribute__ ((nonnull (2,1)));


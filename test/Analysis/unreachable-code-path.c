// RUN: %clang_cc1 -analyze -analyzer-experimental-checks -analyzer-check-objc-mem -analyzer-check-dead-stores -verify -analyzer-opt-analyze-nested-blocks %s

extern void foo(int a);

// The first few tests are non-path specific - we should be able to find them

void test(unsigned a) {
  switch (a) {
    a += 5; // expected-warning{{never executed}}
  case 2:
    a *= 10;
  case 3:
    a %= 2;
  }
  foo(a);
}

void test2(unsigned a) {
 help:
  if (a > 0)
    return;
  if (a == 0)
    return;
  foo(a); // expected-warning{{never executed}}
  goto help;
}

void test3(unsigned a) {
  while(1);
  if (a > 5) { // expected-warning{{never executed}}
    return;
  }
}

// These next tests are path-sensitive

void test4() {
  int a = 5;

  while (a > 1)
    a -= 2;

  if (a > 1) {
    a = a + 56; // expected-warning{{never executed}}
  }

  foo(a);
}

extern void bar(char c);

void test5(const char *c) {
  foo(c[0]);

  if (!c) {
    bar(1); // expected-warning{{never executed}}
  }
}

// These next tests are false positives and should not generate warnings

void test6(const char *c) {
  if (c) return;
  if (!c) return;
  __builtin_unreachable(); // no-warning
}

// Compile-time constant false positives
#define CONSTANT 0
enum test_enum { Off, On };
void test7() {
  if (CONSTANT)
    return; // no-warning

  if (sizeof(int))
    return; // no-warning

  if (Off)
    return; // no-warning
}

void test8() {
  static unsigned a = 0;

  if (a)
    a = 123; // no-warning

  a = 5;
}


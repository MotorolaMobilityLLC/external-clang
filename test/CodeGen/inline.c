// RUN: echo "C89 tests:" &&
// RUN: clang %s -emit-llvm -S -o %t -std=c89 &&
// RUN: grep "define available_externally i32 @ei()" %t &&
// RUN: grep "define i32 @foo()" %t &&
// RUN: grep "define i32 @bar()" %t &&
// RUN: grep "define void @unreferenced1()" %t &&
// RUN: not grep unreferenced2 %t &&
// RUN: grep "define void @gnu_inline()" %t &&
// RUN: grep "define available_externally void @gnu_ei_inline()" %t &&
// RUN: grep "define void @test3()" %t &&
// RUN: grep "define i32 @test1" %t &&
// RUN: grep "define i32 @test2" %t &&

// RUN: echo "\nC99 tests:" &&
// RUN: clang %s -emit-llvm -S -o %t -std=c99 &&
// RUN: grep "define i32 @ei()" %t &&
// RUN: grep "define available_externally i32 @foo()" %t &&
// RUN: grep "define i32 @bar()" %t &&
// RUN: not grep unreferenced1 %t &&
// RUN: grep "define void @unreferenced2()" %t &&
// RUN: grep "define void @gnu_inline()" %t &&
// RUN: grep "define available_externally void @gnu_ei_inline()" %t &&
// RUN: grep "define i32 @test1" %t &&
// RUN: grep "define i32 @test2" %t &&

// RUN: echo "\nC++ tests:" &&
// RUN: clang %s -emit-llvm -S -o %t -std=c++98 &&
// RUN: grep "define linkonce_odr i32 @_Z2eiv()" %t &&
// RUN: grep "define linkonce_odr i32 @_Z3foov()" %t &&
// RUN: grep "define i32 @_Z3barv()" %t &&
// RUN: not grep unreferenced %t &&
// RUN: grep "define void @_Z10gnu_inlinev()" %t &&
// RUN: grep "define available_externally void @_Z13gnu_ei_inlinev()" %t

extern inline int ei() { return 123; }

inline int foo() {
  return ei();
}

int bar() { return foo(); }


inline void unreferenced1() {}
extern inline void unreferenced2() {}

__inline __attribute((__gnu_inline__)) void gnu_inline() {}

// PR3988
extern inline __attribute__((gnu_inline)) void gnu_ei_inline() {}
void (*P)() = gnu_ei_inline;

// <rdar://problem/6818429>
int test1();
inline int test1() { return 4; }
inline int test2() { return 5; }
inline int test2();
int test2();

void test_test1() { test1(); }
void test_test2() { test2(); }

// PR3989
extern inline void test3() __attribute__((gnu_inline));
inline void test3()  {}

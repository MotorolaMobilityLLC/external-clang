// RUN: %clang_cc1 -triple i386-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple i386-unknown-linux-gnu -O2 -emit-llvm -o - %s | FileCheck %s

int foo(int) __attribute__ ((ifunc("foo_ifunc")));

static int f1(int i) {
  return i + 1;
}

static int f2(int i) {
  return i + 2;
}

typedef int (*foo_t)(int);

int global;

static foo_t foo_ifunc() {
  return global ? f1 : f2;
}

int bar() {
  return foo(1);
}

extern void goo(void);

void bar2(void) {
  goo();
}

extern void goo(void) __attribute__ ((ifunc("goo_ifunc")));

void* goo_ifunc(void) {
  return 0;
}
// CHECK: @foo = ifunc i32 (i32), bitcast (i32 (i32)* ()* @foo_ifunc to i32 (i32)*)
// CHECK: @goo = ifunc void (), bitcast (i8* ()* @goo_ifunc to void ()*)

// CHECK: call i32 @foo(i32
// CHECK: call void @goo()

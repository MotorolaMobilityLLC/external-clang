// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -S -emit-llvm -std=c++0x -include %S/ser.h %s -o - | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-pch -o %t-ser.pch -std=c++0x -x c++ %S/ser.h
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -S -emit-llvm -std=c++0x -include-pch %t-ser.pch %s -o - | FileCheck %s

struct D {
  ~D() throw();
};
struct E {
  ~E() throw();
};

void test() {
  bool b;
  // CHECK: store i8 1, i8* %b, align 1
  b = noexcept(0);
  // CHECK: store i8 0, i8* %b, align 1
  b = noexcept(throw 0);
  b = f1();
  b = f2();

  // CHECK-NOT: call void @_ZN1ED1Ev
  // CHECK: call void @_ZN1DD1Ev
  D(), noexcept(E());
}
// CHECK: ret i1 true
// CHECK: ret i1 false

// RUN: %clang_cc1 %s -emit-llvm -o - -triple=x86_64-apple-darwin10 | FileCheck %s
// RUN: %clang_cc1 %s -emit-llvm -o - -triple=x86_64-apple-darwin10 -O3 | FileCheck --check-prefix=CHECK-O3 %s
struct A { int a; int b; };
struct B { int b; };
struct C : B, A { };

// Zero init.
namespace ZeroInit {
  // CHECK: @_ZN8ZeroInit1aE = global i64 -1
  int A::* a;
  
  // CHECK: @_ZN8ZeroInit2aaE = global [2 x i64] [i64 -1, i64 -1]
  int A::* aa[2];
  
  // CHECK: @_ZN8ZeroInit3aaaE = global [2 x [2 x i64]] {{\[}}[2 x i64] [i64 -1, i64 -1], [2 x i64] [i64 -1, i64 -1]]
  int A::* aaa[2][2];
  
  // CHECK: @_ZN8ZeroInit1bE = global i64 -1,
  int A::* b = 0;

  // CHECK: @_ZN8ZeroInit2saE = global %struct.anon { i64 -1 }
  struct {
    int A::*a;
  } sa;
  
  // CHECK: @_ZN8ZeroInit3ssaE = 
  // CHECK: [2 x i64] [i64 -1, i64 -1]
  struct {
    int A::*aa[2];
  } ssa[2];
  
  // CHECK: @_ZN8ZeroInit2ssE = global %1 { %struct.anon { i64 -1 } }
  struct {
    struct {
      int A::*pa;
    } s;
  } ss;
  
  struct A {
    int A::*a;
    int b;
  };

  struct B {
    A a[10];
    char c;
    int B::*b;
  };

  struct C : A, B { int j; };
  // CHECK: @_ZN8ZeroInit1cE = global %"struct.ZeroInit::C" { [16 x i8] c"\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00", [176 x i8] c"\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\FF\FF\FF\FF\FF\FF\FF\FF", i32 0, [4 x i8] zeroinitializer }
  C c;
}

// PR5674
namespace PR5674 {
  // CHECK: @_ZN6PR56742pbE = global i64 4
  int A::*pb = &A::b;
}

// Casts.
namespace Casts {

int A::*pa;
int C::*pc;

void f() {
  // CHECK: store i64 -1, i64* @_ZN5Casts2paE
  pa = 0;

  // CHECK: [[ADJ:%[a-zA-Z0-9\.]+]] = add i64 {{.*}}, 4
  // CHECK: store i64 [[ADJ]], i64* @_ZN5Casts2pcE
  pc = pa;

  // CHECK: [[ADJ:%[a-zA-Z0-9\.]+]] = sub i64 {{.*}}, 4
  // CHECK: store i64 [[ADJ]], i64* @_ZN5Casts2paE
  pa = static_cast<int A::*>(pc);
}

}

// Comparisons
namespace Comparisons {
  void f() {
    int A::*a;

    // CHECK: icmp ne i64 {{.*}}, -1
    if (a) { }

    // CHECK: icmp ne i64 {{.*}}, -1
    if (a != 0) { }
    
    // CHECK: icmp ne i64 -1, {{.*}}
    if (0 != a) { }

    // CHECK: icmp eq i64 {{.*}}, -1
    if (a == 0) { }

    // CHECK: icmp eq i64 -1, {{.*}}
    if (0 == a) { }
  }
}

namespace ValueInit {

struct A {
  int A::*a;

  char c;

  A();
};

// CHECK: define void @_ZN9ValueInit1AC2Ev
// CHECK: store i64 -1, i64*
// CHECK: ret void
A::A() : a() {}

}

namespace PR7139 {

struct pair {
  int first;
  int second;
};

typedef int pair::*ptr_to_member_type;

struct ptr_to_member_struct { 
  ptr_to_member_type data;
  int i;
};

struct A {
  ptr_to_member_struct a;

  A() : a() {}
};

// CHECK-O3: define zeroext i1 @_ZN6PR71395checkEv() nounwind readnone
bool check() {
  // CHECK-O3: ret i1 true
  return A().a.data == 0;
}

// CHECK-O3: define zeroext i1 @_ZN6PR71396check2Ev() nounwind readnone
bool check2() {
  // CHECK-O3: ret i1 true
  return ptr_to_member_type() == 0;
}

}

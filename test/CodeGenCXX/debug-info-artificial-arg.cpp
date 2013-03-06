// RUN: %clang_cc1 -emit-llvm -g -triple x86_64-apple-darwin %s -o - | FileCheck %s

template<class X> class B {
public:
  explicit B(X* p = 0);
};

class A
{
public:
  A(int value) : m_a_value(value) {};
  A(int value, A* client_A) : m_a_value (value), m_client_A (client_A) {}

  virtual ~A() {}

private:
  int m_a_value;
  B<A> m_client_A;
};

int main(int argc, char **argv) {
  A reallyA (500);
}

// FIXME: The numbers are truly awful.
// CHECK: ![[ARTARG:.*]] = metadata !{i32 786447, i32 0, metadata !"", i32 0, i32 0, i64 64, i64 64, i64 0, i32 1088, metadata ![[CLASSTYPE:.*]]} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from A]
// CHECK: ![[CLASSTYPE]] = metadata !{i32 {{.*}}, null, metadata !"A", metadata !{{.*}}, i32 8, i64 128, i64 64, i32 0, i32 0, null, metadata !{{.*}}, i32 0, metadata ![[CLASSTYPE]], null} ; [ DW_TAG_class_type ]
// CHECK: metadata ![[CLASSTYPE]], metadata !"A", metadata !"A", metadata !"", metadata !{{.*}}, i32 12, metadata !{{.*}}, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !{{.*}}, i32 12} ; [ DW_TAG_subprogram ]
// CHECK: metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata ![[FUNCTYPE:.*]], i32 0, i32 0} ; [ DW_TAG_subroutine_type ]
// CHECK: ![[FUNCTYPE]] = metadata !{null, metadata ![[ARTARG]], metadata !{{.*}}, metadata !{{.*}}}

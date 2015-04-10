// RUN: %clang_cc1 -emit-llvm -g %s -o - | FileCheck %s
// Test that the line table info for Foo<T>::bar() is pointing to the
// right header file.
// CHECK: define{{.*}}bar
// CHECK-NOT: define
// CHECK: ret {{.*}}, !dbg [[DBG:.*]]
// CHECK: [[HPP:.*]] = !MDFile(filename: "./template.hpp",
// CHECK: [[SP:.*]] = !MDSubprogram(name: "bar",
// CHECK-SAME:                      file: [[HPP]], line: 22
// CHECK-SAME:                      isDefinition: true
// We shouldn't need a lexical block for this function.
// CHECK: [[DBG]] = !MDLocation(line: 23, scope: [[SP]])


# 1 "./template.h" 1
template <typename T>
class Foo {
public:
  int bar();
};
# 21 "./template.hpp"
template <typename T>
int Foo<T>::bar() {
  return 23;
}
int main (int argc, const char * argv[])
{
  Foo<int> f;
  return f.bar();
}

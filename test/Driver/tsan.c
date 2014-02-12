// RUN: %clang     -target x86_64-unknown-linux -fsanitize=thread %s -S -emit-llvm -o - | FileCheck %s
// RUN: %clang -O1 -target x86_64-unknown-linux -fsanitize=thread %s -S -emit-llvm -o - | FileCheck %s
// RUN: %clang -O2 -target x86_64-unknown-linux -fsanitize=thread %s -S -emit-llvm -o - | FileCheck %s
// RUN: %clang -O3 -target x86_64-unknown-linux -fsanitize=thread %s -S -emit-llvm -o - | FileCheck %s
// RUN: %clang     -target x86_64-unknown-linux -fsanitize=thread  %s -S -emit-llvm -o - | FileCheck %s
// Verify that -fsanitize=thread invokes tsan instrumentation.

int foo(int *a) { return *a; }
// CHECK: __tsan_init

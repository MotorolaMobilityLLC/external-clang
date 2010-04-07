// RUN: %clang_cc1 -triple i686-pc-win32 -fms-extensions -fsyntax-only %s 2>&1 | FileCheck -check-prefix=CHECK-NEG %s
// RUN: %clang_cc1 -triple i686-pc-win32 -fsyntax-only %s 2>&1 | FileCheck -check-prefix=CHECK-POS %s

// Do not report an error with including dllimport in the typedef when -fms-extensions is specified.
// Addresses <rdar://problem/7653870>.
typedef __declspec(dllimport) int CB(void);

// This function is added just to trigger a diagnostic.  This way we can test how many
// diagnostics we expect.
void bar() { return 1; }

// CHECK-NEG: warning: void function 'bar' should not return a value
// CHECK-NEG: 1 warning generated
// CHECK-POS: warning: 'dllimport' attribute only applies to variable and function type
// CHECK-POS: warning: void function 'bar' should not return a value
// CHECK-POS: 2 warnings generated


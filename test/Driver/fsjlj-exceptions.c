// RUN: %clang -target armv7-apple-ios -fexceptions -c %s -o /dev/null -### 2>&1 | FileCheck -check-prefix CHECK-IOS %s
// RUN: %clang -target i686-windows-gnu -fexceptions -c %s -o /dev/null -### 2>&1 | FileCheck -check-prefix CHECK-MINGW-DEFAULT %s
// RUN: %clang -target i686-windows-gnu -fexceptions -fsjlj-exceptions -c %s -o /dev/null -### 2>&1 | FileCheck -check-prefix CHECK-MINGW-SJLJ %s

// CHECK-IOS: -fsjlj-exceptions
// CHECK-MINGW-DEFAULT-NOT: -fsjlj-exceptions
// CHECK-MINGW-SJLJ: -fsjlj-exceptions


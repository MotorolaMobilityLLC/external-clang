// RUN: clang-cc -fsyntax-only -verify %s
int x __attribute__((sentinel)); //expected-warning{{'sentinel' attribute only applies to function or method types}}

void f1(int a, ...) __attribute__ ((sentinel));
void f2(int a, ...) __attribute__ ((sentinel(1)));

void f3(int a, ...) __attribute__ ((sentinel("hello"))); //expected-error{{'sentinel' attribute requires parameter 1 to be an integer constant}}
void f4(int a, ...) __attribute__ ((sentinel(1, 2, 3))); //expected-error{{attribute requires 0, 1 or 2 argument(s)}}
void f4(int a, ...) __attribute__ ((sentinel(-1))); //expected-error{{parameter 1 less than zero}}
void f4(int a, ...) __attribute__ ((sentinel(0, 2))); // expected-error{{parameter 2 not 0 or 1}}

void f5(int a) __attribute__ ((sentinel)); //expected-warning{{'sentinel' attribute only supported for variadic functions}}


void f6() __attribute__((__sentinel__));  // expected-warning {{'sentinel' attribute requires named arguments}}

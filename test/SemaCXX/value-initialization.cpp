// RUN: %clang_cc1 -fsyntax-only -verify %s -std=c++0x

struct A { // expected-error {{implicit default constructor for 'A' must explicitly initialize the const member 'i'}}
     const int i;	// expected-note {{declared at}}
     virtual void f() { } 
};

int main () {
      (void)A(); // expected-note {{first required here}}
}

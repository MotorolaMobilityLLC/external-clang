// RUN: clang -fsyntax-only -verify %s

@compatibility_alias alias4 foo; // expected-warning {{cannot find interface declaration for 'foo'}}

@class class2;
@class class3;

typedef int I;  // expected-warning {{previous declaration is here}}

@compatibility_alias alias1 I;  // expected-warning {{cannot find interface declaration for 'I'}}

@compatibility_alias alias class2;    // expected-warning {{previous declaration is here}} 
@compatibility_alias alias class3;   // expected-warning {{previously declared alias is ignored}}


typedef int alias2;	// expected-error {{previous declaration is here}}
@compatibility_alias alias2 class3;  // expected-error {{conflicting types for alias alias2'}}

alias *p;
class2 *p2;

int foo ()
{

	if (p == p2) {
	  int alias = 1;
	}

	alias *p3;
	return p3 == p2;
}

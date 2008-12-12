// RUN: clang -fsyntax-only -verify -pedantic %s

// PR2241
float test2241[] = { 
  1e,            // expected-error {{exponent}}
  1ee0           // expected-error {{exponent}}
};


// Testcase derived from PR2692
static char *f (char * (*g) (char **, int), char **p, ...) {
    char *s;
    va_list v;                              // expected-error {{identifier}}
    s = g (p, __builtin_va_arg(v, int));    // expected-error {{identifier}} expected-warning {{extension}}
}


// PR3172
} // expected-error {{expected external declaration}}


// rdar://6094870
int test(int) {
  struct { int i; } x;
  
  if (x.hello)   // expected-error {{no member named 'hello'}}
    test(0);
  else
    ;
  
  if (x.hello == 0)   // expected-error {{no member named 'hello'}}
    test(0);
  else
    ;
  
  if ((x.hello == 0))   // expected-error {{no member named 'hello'}}
    test(0);
  else
    ;
  
  if (x.i == 0))   // expected-error {{expected expression}}
    test(0);
  else
    ;
}



char ((((                       /* expected-note {{to match this '('}} */
         *X x ] ))));                    /* expected-error {{expected ')'}} */

;   // expected-warning {{ISO C does not allow an extra ';' outside of a function}}




struct S { void *X, *Y; };

struct S A = {
&BADIDENT, 0     /* expected-error {{use of undeclared identifier}} */
};

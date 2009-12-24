// RUN: %clang_cc1 -fsyntax-only -verify %s

// PR5868
struct T0 {
  int x;
  union {
    void *m0;
  };
};
template <typename T>
struct T1 : public T0, public T { 
  void f0() { 
    m0 = 0; // expected-error{{ambiguous conversion}}
  } 
};

struct A : public T0 { };

void f1(T1<A> *S) { S->f0(); } // expected-note{{instantiation of member function}}

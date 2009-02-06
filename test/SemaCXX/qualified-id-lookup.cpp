// RUN: clang -fsyntax-only -verify %s 
namespace Ns {
  int f(); // expected-note{{previous declaration is here}}

  enum E {
    Enumerator
  };
}
namespace Ns {
  double f(); // expected-error{{functions that differ only in their return type cannot be overloaded}}

  int x = Enumerator;
}

namespace Ns2 {
  float f();
}

int y = Ns::Enumerator;

namespace Ns2 {
  float f(int); // expected-note{{previous declaration is here}}
}

namespace Ns2 {
  double f(int); // expected-error{{functions that differ only in their return type cannot be overloaded}}
}

namespace N {
  int& f1();
}

namespace N {
  struct f1 {
    static int member;

    typedef int type;

    void foo(type);
  };

  void test_f1() {
    int &i1 = f1();
  }
}

void N::f1::foo(int i) { 
  f1::member = i; 
  f1::type &ir = i;
}

namespace N {
  float& f1(int x) {
    N::f1::type& i1 = x;
    f1::type& i2 = x;
  }

  struct f2 {
    static int member;
  };
  void f2();
}

int i1 = N::f1::member;
typedef struct N::f1 type1;
int i2 = N::f2::member;
typedef struct N::f2 type2;

void test_f1(int i) {
  int &v1 = N::f1();
  float &v2 = N::f1(i);
  int v3 = ::i1;
  int v4 = N::f1::member;
}

typedef int f2_type;
namespace a {
  typedef int f2_type(int, int);

  void test_f2() {
    ::f2_type(1, 2); // expected-error {{function-style cast to a builtin type can only take one argument}}
  }
}

// PR clang/3291
namespace a {  
  namespace a {   // A1
    namespace a { // A2
      int i;
    }
  }
}

void test_a() {
  a::a::i = 3; // expected-error{{no member named 'i'}}
  a::a::a::i = 4;
}
  
struct Undef { // expected-note{{definition of 'struct Undef' is not complete until the closing '}'}}
  typedef int type;

  Undef::type member;

  static int size = sizeof(Undef); // expected-error{{invalid application of 'sizeof' to an incomplete type 'struct Undef'}}

  int f();
};

int Undef::f() {
  return sizeof(Undef);
}

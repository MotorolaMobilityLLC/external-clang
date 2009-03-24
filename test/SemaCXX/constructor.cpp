// RUN: clang-cc -fsyntax-only -verify %s 
typedef int INT;

class Foo {
  Foo();
  (Foo)(float) { }
  explicit Foo(int); // expected-note {{previous declaration is here}}
  Foo(const Foo&);

  ((Foo))(INT); // expected-error{{cannot be redeclared}}

  Foo(Foo foo, int i = 17, int j = 42); // expected-error {{copy constructor must pass its first argument by reference}}

  static Foo(short, short); // expected-error{{constructor cannot be declared 'static'}}
  virtual Foo(double); // expected-error{{constructor cannot be declared 'virtual'}}
  Foo(long) const; // expected-error{{'const' qualifier is not allowed on a constructor}}

  int Foo(int, int); // expected-error{{constructor cannot have a return type}}
};

Foo::Foo(const Foo&) { }

typedef struct {
  int version;
} Anon;
extern const Anon anon;
extern "C" const Anon anon2;

// PR3188: The extern declaration complained about not having an appropriate
// constructor.
struct x;
extern x a;

// A similar case.
struct y {
  y(int);
};
extern y b;

struct Length {
  Length l() const { return *this; }
};

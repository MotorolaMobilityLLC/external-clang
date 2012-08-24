// RUN: %clang_cc1 -analyze -analyzer-checker=core,unix.Malloc,debug.ExprInspection -analyzer-store region -analyzer-ipa=inlining -cfg-add-implicit-dtors -Wno-null-dereference -verify %s

void clang_analyzer_eval(bool);

class A {
public:
  ~A() { 
    int *x = 0;
    *x = 3; // expected-warning{{Dereference of null pointer}}
  }
};

int main() {
  A a;
}


typedef __typeof(sizeof(int)) size_t;
void *malloc(size_t);
void free(void *);

class SmartPointer {
  void *X;
public:
  SmartPointer(void *x) : X(x) {}
  ~SmartPointer() {
    free(X);
  }
};

void testSmartPointer() {
  char *mem = (char*)malloc(4);
  {
    SmartPointer Deleter(mem);
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}


void doSomething();
void testSmartPointer2() {
  char *mem = (char*)malloc(4);
  {
    SmartPointer Deleter(mem);
    // Remove dead bindings...
    doSomething();
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}


class Subclass : public SmartPointer {
public:
  Subclass(void *x) : SmartPointer(x) {}
};

void testSubclassSmartPointer() {
  char *mem = (char*)malloc(4);
  {
    Subclass Deleter(mem);
    // Remove dead bindings...
    doSomething();
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}


class MultipleInheritance : public Subclass, public SmartPointer {
public:
  MultipleInheritance(void *a, void *b) : Subclass(a), SmartPointer(b) {}
};

void testMultipleInheritance1() {
  char *mem = (char*)malloc(4);
  {
    MultipleInheritance Deleter(mem, 0);
    // Remove dead bindings...
    doSomething();
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}

void testMultipleInheritance2() {
  char *mem = (char*)malloc(4);
  {
    MultipleInheritance Deleter(0, mem);
    // Remove dead bindings...
    doSomething();
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}

void testMultipleInheritance3() {
  char *mem = (char*)malloc(4);
  {
    MultipleInheritance Deleter(mem, mem);
    // Remove dead bindings...
    doSomething();
    // destructor called here
    // expected-warning@27 {{Attempt to free released memory}}
  }
}


class SmartPointerMember {
  SmartPointer P;
public:
  SmartPointerMember(void *x) : P(x) {}
};

void testSmartPointerMember() {
  char *mem = (char*)malloc(4);
  {
    SmartPointerMember Deleter(mem);
    // Remove dead bindings...
    doSomething();
    // destructor called here
  }
  *mem = 0; // expected-warning{{Use of memory after it is freed}}
}


struct IntWrapper {
  IntWrapper() : x(0) {}
  ~IntWrapper();
  int *x;
};

void testArrayInvalidation() {
  int i = 42;
  int j = 42;

  {
    IntWrapper arr[2];

    // There should be no undefined value warnings here.
    // Eventually these should be TRUE as well, but right now
    // we can't handle array constructors.
    clang_analyzer_eval(arr[0].x == 0); // expected-warning{{UNKNOWN}}
    clang_analyzer_eval(arr[1].x == 0); // expected-warning{{UNKNOWN}}

    arr[0].x = &i;
    arr[1].x = &j;
    clang_analyzer_eval(*arr[0].x == 42); // expected-warning{{TRUE}}
    clang_analyzer_eval(*arr[1].x == 42); // expected-warning{{TRUE}}
  }

  // The destructors should have invalidated i and j.
  clang_analyzer_eval(i == 42); // expected-warning{{UNKNOWN}}
  clang_analyzer_eval(j == 42); // expected-warning{{UNKNOWN}}
}



// Don't crash on a default argument inside an initializer.
struct DefaultArg {
  DefaultArg(int x = 0) {}
  ~DefaultArg();
};

struct InheritsDefaultArg : DefaultArg {
  InheritsDefaultArg() {}
  virtual ~InheritsDefaultArg();
};

void testDefaultArg() {
  InheritsDefaultArg a;
  // Force a bug to be emitted.
  *(char *)0 = 1; // expected-warning{{Dereference of null pointer}}
}


namespace DestructorVirtualCalls {
  class A {
  public:
    int *out1, *out2, *out3;

    virtual int get() { return 1; }

    ~A() {
      *out1 = get();
    }
  };

  class B : public A {
  public:
    virtual int get() { return 2; }

    ~B() {
      *out2 = get();
    }
  };

  class C : public B {
  public:
    virtual int get() { return 3; }

    ~C() {
      *out3 = get();
    }
  };

  void test() {
    int a, b, c;

    // New scope for the C object.
    {
      C obj;
      clang_analyzer_eval(obj.get() == 3); // expected-warning{{TRUE}}

      // Sanity check for devirtualization.
      A *base = &obj;
      clang_analyzer_eval(base->get() == 3); // expected-warning{{TRUE}}

      obj.out1 = &a;
      obj.out2 = &b;
      obj.out3 = &c;
    }

    clang_analyzer_eval(a == 1); // expected-warning{{TRUE}}
    clang_analyzer_eval(b == 2); // expected-warning{{TRUE}}
    clang_analyzer_eval(c == 3); // expected-warning{{TRUE}}
  }
}

// RUN: clang-cc -fsyntax-only -verify %s

// ---------------------------------------------------------------------
// C++ Functional Casts
// ---------------------------------------------------------------------
template<int N>
struct ValueInit0 {
  int f() {
    return int();
  }
};

template struct ValueInit0<5>;

template<int N>
struct FunctionalCast0 {
  int f() {
    return int(N);
  }
};

template struct FunctionalCast0<5>;

struct X { // expected-note 2 {{candidate function}}
  X(int, int); // expected-note 2 {{candidate function}}
};

template<int N, int M>
struct BuildTemporary0 {
  X f() {
    return X(N, M);
  }
};

template struct BuildTemporary0<5, 7>;

template<int N, int M>
struct Temporaries0 {
  void f() {
    (void)X(N, M);
  }
};

template struct Temporaries0<5, 7>;

// ---------------------------------------------------------------------
// new/delete expressions
// ---------------------------------------------------------------------
struct Y { };

template<typename T>
struct New0 {
  T* f(bool x) {
    if (x)
      return new T; // expected-error{{no matching}}
    else
      return new T();
  }
};

template struct New0<int>;
template struct New0<Y>;
template struct New0<X>; // expected-note{{instantiation}}

template<typename T, typename Arg1>
struct New1 {
  T* f(bool x, Arg1 a1) {
    return new T(a1); // expected-error{{no matching}}
  }
};

template struct New1<int, float>;
template struct New1<Y, Y>;
template struct New1<X, Y>; // expected-note{{instantiation}}

template<typename T, typename Arg1, typename Arg2>
struct New2 {
  T* f(bool x, Arg1 a1, Arg2 a2) {
    return new T(a1, a2); // expected-error{{no matching}}
  }
};

template struct New2<X, int, float>;
template struct New2<X, int, int*>; // expected-note{{instantiation}}
// FIXME: template struct New2<int, int, float>;

template<typename T>
struct Delete0 {
  void f(T t) {
    delete t; // expected-error{{cannot delete}}
    ::delete [] t;
  }
};

template struct Delete0<int*>;
template struct Delete0<X*>;
template struct Delete0<int>; // expected-note{{instantiation}}

// ---------------------------------------------------------------------
// throw expressions
// ---------------------------------------------------------------------
template<typename T>
struct Throw1 {
  void f(T t) {
    throw;
    throw t; // expected-error{{incomplete type}}
  }
};

struct Incomplete; // expected-note{{forward}}

template struct Throw1<int>;
template struct Throw1<int*>;
template struct Throw1<Incomplete*>; // expected-note{{instantiation}}

// ---------------------------------------------------------------------
// typeid expressions
// ---------------------------------------------------------------------

// FIXME: This should really include <typeinfo>, but we don't have that yet.
namespace std {
  class type_info;
}

template<typename T>
struct TypeId0 {
  const std::type_info &f(T* ptr) {
    if (ptr)
      return typeid(ptr);
    else
      return typeid(T);
  }
};

struct Abstract {
  virtual void f() = 0;
};

template struct TypeId0<int>;
template struct TypeId0<Incomplete>;
template struct TypeId0<Abstract>;

// ---------------------------------------------------------------------
// type traits
// ---------------------------------------------------------------------
template<typename T>
struct is_pod {
  static const bool value = __is_pod(T);
};

static const int is_pod0[is_pod<X>::value? -1 : 1];
static const int is_pod1[is_pod<Y>::value? 1 : -1];

// ---------------------------------------------------------------------
// initializer lists
// ---------------------------------------------------------------------
template<typename T, typename Val1>
struct InitList1 {
  void f(Val1 val1) { 
    T x = { val1 };
  }
};

struct APair {
  int *x;
  const float *y;
};

template struct InitList1<int[1], float>;
template struct InitList1<APair, int*>;

template<typename T, typename Val1, typename Val2>
struct InitList2 {
  void f(Val1 val1, Val2 val2) { 
    T x = { val1, val2 }; // expected-error{{incompatible}}
  }
};

template struct InitList2<APair, int*, float*>;
template struct InitList2<APair, int*, double*>; // expected-note{{instantiation}}

// ---------------------------------------------------------------------
// member references
// ---------------------------------------------------------------------
template<typename T, typename Result>
struct DotMemRef0 {
  void f(T t) {
    Result result = t.m; // expected-error{{cannot be initialized}}
  }
};

struct MemInt {
  int m;
};

struct InheritsMemInt : MemInt { };

struct MemIntFunc {
  static int m(int);
};

template struct DotMemRef0<MemInt, int&>;
template struct DotMemRef0<InheritsMemInt, int&>;
template struct DotMemRef0<MemIntFunc, int (*)(int)>;
template struct DotMemRef0<MemInt, float&>; // expected-note{{instantiation}}

template<typename T, typename Result>
struct ArrowMemRef0 {
  void f(T t) {
    Result result = t->m; // expected-error 2{{cannot be initialized}}
  }
};

template<typename T>
struct ArrowWrapper {
  T operator->();
};

template struct ArrowMemRef0<MemInt*, int&>;
template struct ArrowMemRef0<InheritsMemInt*, int&>;
template struct ArrowMemRef0<MemIntFunc*, int (*)(int)>;
template struct ArrowMemRef0<MemInt*, float&>; // expected-note{{instantiation}}

template struct ArrowMemRef0<ArrowWrapper<MemInt*>, int&>;
template struct ArrowMemRef0<ArrowWrapper<InheritsMemInt*>, int&>;
template struct ArrowMemRef0<ArrowWrapper<MemIntFunc*>, int (*)(int)>;
template struct ArrowMemRef0<ArrowWrapper<MemInt*>, float&>; // expected-note{{instantiation}}
template struct ArrowMemRef0<ArrowWrapper<ArrowWrapper<MemInt*> >, int&>;

// FIXME: we should be able to return a MemInt without the reference!
MemInt &createMemInt(int);

template<int N>
struct NonDepMemberExpr0 {
  void f() {
    createMemInt(N).m = N;
  }
};

template struct NonDepMemberExpr0<0>; 

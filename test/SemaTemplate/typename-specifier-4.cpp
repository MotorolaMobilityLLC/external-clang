// RUN: %clang_cc1 -fsyntax-only -verify %s
template<typename T, typename U> 
struct is_same {
  static const bool value = false;
};

template<typename T>
struct is_same<T, T> {
  static const bool value = true;
};

template<typename MetaFun, typename T1, typename T2>
struct metafun_apply2 {
  typedef typename MetaFun::template apply<T1, T2> inner;
  typedef typename inner::type type;
};

template<typename T, typename U> struct pair;

struct make_pair {
  template<typename T1, typename T2>
  struct apply {
    typedef pair<T1, T2> type;
  };
};

int a0[is_same<metafun_apply2<make_pair, int, float>::type, 
               pair<int, float> >::value? 1 : -1];
int a1[is_same<
         typename make_pair::template apply<int, float>, // expected-warning{{'template' keyword outside of a template}} \
       // expected-warning{{'typename' occurs outside of a template}}
         make_pair::apply<int, float>
       >::value? 1 : -1];

template<typename MetaFun>
struct swap_and_apply2 {
  template<typename T1, typename T2>
  struct apply {
    typedef typename MetaFun::template apply<T2, T1> new_metafun;
    typedef typename new_metafun::type type;
  };
};

int a2[is_same<swap_and_apply2<make_pair>::apply<int, float>::type, 
               pair<float, int> >::value? 1 : -1];

template<typename MetaFun>
struct swap_and_apply2b {
  template<typename T1, typename T2>
  struct apply {
    typedef typename MetaFun::template apply<T2, T1>::type type;
  };
};

int a3[is_same<swap_and_apply2b<make_pair>::apply<int, float>::type, 
               pair<float, int> >::value? 1 : -1];

template<typename T>
struct X0 {
  template<typename U, typename V>
  struct Inner;
  
  void f0(X0<T>::Inner<T*, T&>); // expected-note{{here}}
  void f0(typename X0<T>::Inner<T*, T&>); // expected-error{{redecl}}

  void f1(X0<T>::Inner<T*, T&>); // expected-note{{here}}
  void f1(typename X0<T>::template Inner<T*, T&>); // expected-error{{redecl}}

  void f2(typename X0<T>::Inner<T*, T&>::type); // expected-note{{here}}
  void f2(typename X0<T>::template Inner<T*, T&>::type); // expected-error{{redecl}}
};

namespace PR6236 {
  template<typename T, typename U> struct S { };
  
  template<typename T> struct S<T, T> {
    template<typename U> struct K { };
    
    void f() {
      typedef typename S<T, T>::template K<T> Foo;
    }
  };
}

namespace PR6268 {
  template <typename T>
  struct Outer {
    template <typename U>
    struct Inner {};

    template <typename U>
    typename Outer<T>::template Inner<U>
    foo(typename Outer<T>::template Inner<U>);
  };

  template <typename T>
  template <typename U>
  typename Outer<T>::template Inner<U>
  Outer<T>::foo(typename Outer<T>::template Inner<U>) {
    return Inner<U>();
  }
}

namespace PR6463 {
  struct B { typedef int type; }; // expected-note 2{{member found by ambiguous name lookup}}
  struct C { typedef int type; }; // expected-note 2{{member found by ambiguous name lookup}}

  template<typename T>
  struct A : B, C { 
    type& a(); // expected-error{{found in multiple base classes}}
    int x; 
  };

  // FIXME: Improve source location info here.
  template<typename T>
  typename A<T>::type& A<T>::a() { // expected-error{{found in multiple base classes}}
    return x;
  }
}

namespace PR7419 {
  template <typename T> struct S {
    typedef typename T::Y T2;
    typedef typename T2::Z T3;
    typedef typename T3::W T4;
    T4 *f();

    typedef typename T::template Y<int> TT2;
    typedef typename TT2::template Z<float> TT3;
    typedef typename TT3::template W<double> TT4;
    TT4 g();
  };

  template <typename T> typename T::Y::Z::W *S<T>::f() { }
  template <typename T> typename T::template Y<int>::template Z<float>::template W<double> S<T>::g() { }
}

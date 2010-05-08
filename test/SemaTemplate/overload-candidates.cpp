// RUN: %clang_cc1 -fsyntax-only -verify %s

template<typename T>
const T& min(const T&, const T&); // expected-note{{candidate template ignored: deduced conflicting types for parameter 'T' ('int' vs. 'long')}}

void test_min() {
  (void)min(1, 2l); // expected-error{{no matching function for call to 'min'}}
}

template<typename R, typename T>
R *dyn_cast(const T&); // expected-note{{candidate template ignored: couldn't infer template argument 'R'}}

void test_dyn_cast(int* ptr) {
  (void)dyn_cast(ptr); // expected-error{{no matching function for call to 'dyn_cast'}}
}

template<int I, typename T> 
  void get(const T&); // expected-note{{candidate template ignored: invalid explicitly-specified argument for template parameter 'I'}}
template<template<class T> class, typename T> 
  void get(const T&); // expected-note{{candidate template ignored: invalid explicitly-specified argument for 1st template parameter}}

void test_get(void *ptr) {
  get<int>(ptr); // expected-error{{no matching function for call to 'get'}}
}

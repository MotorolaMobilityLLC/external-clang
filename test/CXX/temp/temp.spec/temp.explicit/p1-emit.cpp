// RUN: clang-cc -emit-llvm -triple x86_64-apple-darwin10 -o - %s | FileCheck %s
template<typename T>
struct X {
  static T member1;
  static T member2;
  static T member3;
};

template<typename T>
T X<T>::member1;

template<typename T>
T X<T>::member2 = 17;

// CHECK: @_ZN1XIiE7member1E = global i32 0
template int X<int>::member1;

// CHECK: @_ZN1XIiE7member2E = global i32 17
template int X<int>::member2;

// For implicit instantiation of 
long& get(bool Cond) {
  // CHECK: @_ZN1XIlE7member1E = weak global i64 0
  // CHECK: @_ZN1XIlE7member2E = weak global i64 17
  return Cond? X<long>::member1 : X<long>::member2;
}
// RUN: clang-cc -emit-llvm -o %t %s
// RUN: grep "@pipe()" %t | count 0
// RUN: grep '_thisIsNotAPipe' %t | count 3
// RUN: grep 'g0' %t | count 0
// RUN: grep '_renamed' %t | count 2
// RUN: clang-cc -DUSE_DEF -emit-llvm -o %t %s
// RUN: grep "@pipe()" %t | count 0
// RUN: grep '_thisIsNotAPipe' %t | count 3
// <rdr://6116729>

void pipe() asm("_thisIsNotAPipe");

void f0() {
  pipe();
}

void pipe(int);

void f1() {
  pipe(1);
}

#ifdef USE_DEF
void pipe(int arg) {
  int x = 10;
}
#endif

// PR3698
extern int g0 asm("_renamed");
int f2() {
  return g0;
}

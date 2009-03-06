// RUN: clang %s -emit-llvm -o %t -fblocks -f__block &&
// RUN: grep "_Block_object_dispose" %t | count 4
// RUN: grep "__copy_helper_block_" %t | count 2
// RUN: grep "__destroy_helper_block_" %t | count 2
#include <stdio.h>

void test1() {
  __block int a;
  int b=2;
  a=1;
  printf("a is %d, b is %d\n", a, b);
  ^{ a = 10; printf("a is %d, b is %d\n", a, b); }();
  printf("a is %d, b is %d\n", a, b);
  a = 1;
  printf("a is %d, b is %d\n", a, b);
}


void test2() {
  __block int a;
  a=1;
  printf("a is %d\n", a);
  ^{
    ^{
      a = 10;
    }();
  }();
  printf("a is %d\n", a);
  a = 1;
  printf("a is %d\n", a);
}

void test3() {
  __block int (^j)(int);
  ^{j=0;}();
}

int main() {
  test1();
  test2();
  test3();
  return 0;
}

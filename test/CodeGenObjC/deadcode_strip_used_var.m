// RUN: clang-cc %s -emit-llvm -o %t -triple i386-apple-darwin10 &&
// RUN: grep "llvm.used" %t | count 1 &&
// RUN: clang-cc %s -emit-llvm -o %t -triple x86_64-apple-darwin10 &&
// RUN: grep "llvm.used" %t | count 1 


__attribute__((used)) static int  XXXXXX  __attribute__ ((section ("__DATA,__Xinterpose"))) ;
__attribute__((used)) static int  YYYY  __attribute__ ((section ("__DATA,__Xinterpose"))) ;


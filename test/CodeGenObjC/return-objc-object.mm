// RUN: %clang_cc1 -triple x86_64 -emit-llvm -o - %s | FileCheck %s

@protocol P1 @end
@interface NSOperationQueue 
{
  char ch[64];
  double d;
}
@end

NSOperationQueue &f();
NSOperationQueue<P1> &f1();

void call_once() {
  f();
  f1();
}
// CHECK: call %0* @_Z1fv()
// CHECK: call %0* @_Z2f1v()  

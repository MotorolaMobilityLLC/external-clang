// RUN: clang-cc -emit-llvm -triple=i686-apple-darwin8 -o %t %s -O2 &&
// RUN: grep 'ret i32' %t | count 1 &&
// RUN: grep 'ret i32 1' %t | count 1

#include <stdio.h>

@interface MyClass
{
}
- (void)method;
@end

@implementation MyClass

- (void)method
{
	@synchronized(self)
	{
		NSLog(@"sync");
	}
}

@end

void foo(id a) {
  @synchronized(a) {
    printf("Swimming? No.");
    return;
  }
}

int f0(id a) {
  int x = 0;
  @synchronized((x++, a)) {    
  }
  return x; // ret i32 1
}

void f1(id a) {
  // The trick here is that the return shouldn't go through clean up,
  // but there isn't a simple way to check this property.
  @synchronized(({ return; }), a) {
    return;
  }
}

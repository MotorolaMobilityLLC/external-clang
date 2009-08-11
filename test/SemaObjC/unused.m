// RUN: clang-cc %s -verify -Wunused -fsyntax-only
#include <stdio.h>

@interface Greeter
+ (void) hello;
@end

@implementation Greeter
+ (void) hello {
    fprintf(stdout, "Hello, World!\n");
}
@end


int test1(void) {
  [Greeter hello];
  return 0;
}



@interface NSObject @end
@interface NSString : NSObject 
- (int)length;
@end

void test2() {
  // No unused warning: rdar://7126285
  @"pointless example call for test purposes".length;
}





@interface foo
- (int)meth: (int)x: (int)y: (int)z ;
@end

@implementation foo
- (int) meth: (int)x: 
(int)y: // expected-warning{{unused}} 
(int) __attribute__((unused))z { return x; }
@end

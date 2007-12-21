// RUN: clang -fsyntax-only -verify %s
extern void foo();
#include <objc/objc.h>

@protocol MyProtocolA
- (void) methodA;
@end

@protocol MyProtocolB
- (void) methodB;
@end

@protocol MyProtocolAB <MyProtocolA, MyProtocolB>
@end

@protocol MyProtocolAC <MyProtocolA>
- (void) methodC;
@end

int main()
{
  id<MyProtocolA> obj_a = nil;
  id<MyProtocolB> obj_b = nil;
  id<MyProtocolAB> obj_ab = nil;
  id<MyProtocolAC> obj_ac = nil;

  obj_a = obj_b;  // expected-error {{incompatible types assigning 'id<MyProtocolB>' to 'id<MyProtocolA>'}}
  obj_a = obj_ab; /* Ok */
  obj_a = obj_ac; /* Ok */
  
  obj_b = obj_a;  // expected-error {{incompatible types assigning 'id<MyProtocolA>' to 'id<MyProtocolB>'}}
  obj_b = obj_ab; /* Ok */
  obj_b = obj_ac; // expected-error {{incompatible types assigning 'id<MyProtocolAC>' to 'id<MyProtocolB>'}}
  
  obj_ab = obj_a;  // expected-error {{incompatible types assigning 'id<MyProtocolA>' to 'id<MyProtocolAB>'}}
  obj_ab = obj_b;  // expected-error {{incompatible types assigning 'id<MyProtocolB>' to 'id<MyProtocolAB>'}}
  obj_ab = obj_ac; // expected-error {{incompatible types assigning 'id<MyProtocolAC>' to 'id<MyProtocolAB>'}}
  
  obj_ac = obj_a;  // expected-error {{incompatible types assigning 'id<MyProtocolA>' to 'id<MyProtocolAC>'}}
  obj_ac = obj_b;  // expected-error {{incompatible types assigning 'id<MyProtocolB>' to 'id<MyProtocolAC>'}}
  obj_ac = obj_ab; // expected-error {{incompatible types assigning 'id<MyProtocolAB>' to 'id<MyProtocolAC>'}}

  if (obj_a == obj_b) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolA>' and 'id<MyProtocolB>')}}
  if (obj_b == obj_a) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolB>' and 'id<MyProtocolA>')}}

  if (obj_a == obj_ab) foo (); /* Ok */
  if (obj_ab == obj_a) foo (); /* Ok */ 

  if (obj_a == obj_ac) foo (); /* Ok */ 
  if (obj_ac == obj_a) foo (); /* Ok */ 

  if (obj_b == obj_ab) foo (); /* Ok */ 
  if (obj_ab == obj_b) foo (); /* Ok */ 

  if (obj_b == obj_ac) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolB>' and 'id<MyProtocolAC>')}} 
  if (obj_ac == obj_b) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolAC>' and 'id<MyProtocolB>')}} 

  if (obj_ab == obj_ac) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolAB>' and 'id<MyProtocolAC>')}} 
  if (obj_ac == obj_ab) foo (); // expected-error {{invalid operands to binary expression ('id<MyProtocolAC>' and 'id<MyProtocolAB>')}} 

  return 0;
}

// RUN: %clang_cc1 -triple i386-apple-darwin9 -fsyntax-only -verify %s

@interface I 
{
	int IVAR; // expected-note{{ivar is declared here}}
	int name;
}
@property int d1;
@property id  prop_id; // expected-warning {{no 'assign', 'retain', or 'copy' attribute is specified - 'assign' is assumed}}, expected-warning {{default property attribute 'assign' not appropriate for non-gc object}}
@property int name;
@end

@interface I(CAT)
@property int d1;	// expected-warning {{property 'd1' requires method 'd1' to be defined }} \
			// expected-warning {{property 'd1' requires method 'setD1:' to be defined }}
@end

@implementation I
@synthesize d1;		// expected-error {{synthesized property 'd1' must either be named the same as}}
@dynamic    bad;	// expected-error {{property implementation must have its declaration in interface 'I'}}
@synthesize prop_id;	// expected-error {{synthesized property 'prop_id' must either be named the same}}  // expected-note {{previous declaration is here}}
@synthesize prop_id = IVAR;	// expected-error {{type of property 'prop_id' ('id') does not match type of ivar 'IVAR' ('int')}} // expected-error {{property 'prop_id' is already implemented}}
@synthesize name;	// OK! property with same name as an accessible ivar of same name
@end

@implementation I(CAT)  // expected-note 2 {{implementation is here}}
@synthesize d1;		// expected-error {{@synthesize not allowed in a category's implementation}}
@dynamic bad;		// expected-error {{property implementation must have its declaration in the category 'CAT'}}
@end

@implementation E	// expected-warning {{cannot find interface declaration for 'E'}}
@dynamic d;		// expected-error {{property implementation must have its declaration in interface 'E'}}
@end

@implementation Q(MYCAT)  // expected-error {{cannot find interface declaration for 'Q'}}
@dynamic d;		// expected-error {{property implementation in a category with no category declaration}}
@end

@interface Foo
@property double bar;
@end

int func1() {
   id foo;
   double bar = [foo bar];
   return 0;
}

// PR3932
typedef id BYObjectIdentifier;
@interface Foo1  {
  void *isa;
}
@property(copy) BYObjectIdentifier identifier;
@end

@interface Foo2 
{
  int ivar;
}
@property int treeController;  // expected-note {{property declared here}}
@property int ivar;	// OK
@property int treeController;  // expected-error {{property has a previous declaration}}
@end


// RUN: clang -warn-objc-missing-dealloc %s -verify

// Tests for the checker which checks missing/extra ivar 'release' calls 
// in dealloc.

@interface NSObject
- (void)release;
@end

@interface MyClass : NSObject {
@private
  id _X;
  id _Y;
  id _Z;
  id _K;
  id _N;
  id _M;
  id _V;
  id _W;
}
@property(retain) id X;
@property(retain) id Y;
@property(assign) id Z;
@property(assign) id K;
@property(assign, readonly) id N;
@property(retain) id M;
@property(retain) id V;
@property(retain) id W;
@end

@implementation MyClass
@synthesize X = _X;
@synthesize Y = _Y; // expected-warning{{The '_Y' instance variable was retained by a synthesized property but wasn't released in 'dealloc'}}
@synthesize Z = _Z; // expected-warning{{The '_Z' instance variable was not retained by a synthesized property but was released in 'dealloc'}}
@synthesize K = _K;
@synthesize N = _N;
@synthesize M = _M;
@synthesize V = _V;
@synthesize W = _W; // expected-warning{{The '_W' instance variable was retained by a synthesized property but wasn't released in 'dealloc'}}

- (void)dealloc
{
  [_X release];
  [_Z release];
  [_N release];
  
  self.M = 0; // This will release '_M'
  [self setV:0]; // This will release '_V'
  [self setW:@"newW"]; // This will release '_W', but retain the new value
  
  [super dealloc];
}

@end


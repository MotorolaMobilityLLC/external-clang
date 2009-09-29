// RUN: clang-cc -triple i386-apple-darwin9 -analyze -checker-cfref --analyzer-store=region --verify -fblocks %s &&
// RUN: clang-cc -triple x86_64-apple-darwin9 -analyze -checker-cfref --analyzer-store=region --verify -fblocks %s

typedef struct objc_selector *SEL;
typedef signed char BOOL;
typedef int NSInteger;
typedef unsigned int NSUInteger;
typedef struct _NSZone NSZone;
@class NSInvocation, NSMethodSignature, NSCoder, NSString, NSEnumerator;
@protocol NSObject  - (BOOL)isEqual:(id)object; @end
@protocol NSCopying  - (id)copyWithZone:(NSZone *)zone; @end
@protocol NSMutableCopying  - (id)mutableCopyWithZone:(NSZone *)zone; @end
@protocol NSCoding  - (void)encodeWithCoder:(NSCoder *)aCoder; @end
@interface NSObject <NSObject> {} - (id)init; @end
extern id NSAllocateObject(Class aClass, NSUInteger extraBytes, NSZone *zone);
@interface NSString : NSObject <NSCopying, NSMutableCopying, NSCoding>
- (NSUInteger)length;
+ (id)stringWithUTF8String:(const char *)nullTerminatedCString;
@end extern NSString * const NSBundleDidLoadNotification;
@interface NSAssertionHandler : NSObject {}
+ (NSAssertionHandler *)currentHandler;
- (void)handleFailureInMethod:(SEL)selector object:(id)object file:(NSString *)fileName lineNumber:(NSInteger)line description:(NSString *)format,...;
@end
extern NSString * const NSConnectionReplyMode;


//---------------------------------------------------------------------------
// Test case 'checkaccess_union' differs for region store and basic store.
// The basic store doesn't reason about compound literals, so the code
// below won't fire an "uninitialized value" warning.
//---------------------------------------------------------------------------

// PR 2948 (testcase; crash on VisitLValue for union types)
// http://llvm.org/bugs/show_bug.cgi?id=2948
void checkaccess_union() {
  int ret = 0, status;
  // Since RegionStore doesn't handle unions yet,
  // this branch condition won't be triggered
  // as involving an uninitialized value.  
  if (((((__extension__ (((union {  // no-warning
    __typeof (status) __in; int __i;}
    )
    {
      .__in = (status)}
      ).__i))) & 0xff00) >> 8) == 1)
        ret = 1;
}

// Check our handling of fields being invalidated by function calls.
struct test2_struct { int x; int y; char* s; };
void test2_helper(struct test2_struct* p);

char test2() {
  struct test2_struct s;
  test2_help(&s);
  char *p = 0;
  
  if (s.x > 1) {
    if (s.s != 0) {
      p = "hello";
    }
  }
  
  if (s.x > 1) {
    if (s.s != 0) {
      return *p;
    }
  }

  return 'a';
}

// BasicStore handles this case incorrectly because it doesn't reason about
// the value pointed to by 'x' and thus creates different symbolic values
// at the declarations of 'a' and 'b' respectively.  RegionStore handles
// it correctly. See the companion test in 'misc-ps-basic-store.m'.
void test_trivial_symbolic_comparison_pointer_parameter(int *x) {
  int a = *x;
  int b = *x;
  if (a != b) {
    int *p = 0;
    *p = 0xDEADBEEF;     // no-warning
  }
}

// This is a modified test from 'misc-ps.m'.  Here we have the extra
// NULL dereferences which are pruned out by RegionStore's symbolic reasoning
// of fields.
typedef struct _BStruct { void *grue; } BStruct;
void testB_aux(void *ptr);

void testB(BStruct *b) {
  {
    int *__gruep__ = ((int *)&((b)->grue));
    int __gruev__ = *__gruep__;
    int __gruev2__ = *__gruep__;
    if (__gruev__ != __gruev2__) {
      int *p = 0;
      *p = 0xDEADBEEF; // no-warning
    }

    testB_aux(__gruep__);
  }
  {
    int *__gruep__ = ((int *)&((b)->grue));
    int __gruev__ = *__gruep__;
    int __gruev2__ = *__gruep__;
    if (__gruev__ != __gruev2__) {
      int *p = 0;
      *p = 0xDEADBEEF; // no-warning
    }

    if (~0 != __gruev__) {}
  }
}

void testB_2(BStruct *b) {
  {
    int **__gruep__ = ((int **)&((b)->grue));
    int *__gruev__ = *__gruep__;
    testB_aux(__gruep__);
  }
  {
    int **__gruep__ = ((int **)&((b)->grue));
    int *__gruev__ = *__gruep__;
    if ((int*)~0 != __gruev__) {}
  }
}

// This test case is a reduced case of a caching bug discovered by an
// assertion failure in RegionStoreManager::BindArray.  Essentially the
// DeclStmt is evaluated twice, but on the second loop iteration the
// engine caches out.  Previously a false transition would cause UnknownVal
// to bind to the variable, firing an assertion failure.  This bug was fixed
// in r76262.
void test_declstmt_caching() {
again:
  {
    const char a[] = "I like to crash";
    goto again;
  }
}

// Reduced test case from <rdar://problem/7114618>.
// Basically a null check is performed on the field value, which is then
// assigned to a variable and then checked again.
struct s_7114618 { int *p; };
void test_rdar_7114618(struct s_7114618 *s) {
  if (s->p) {
    int *p = s->p;
    if (!p) {
      // Infeasible
      int *dead = 0;
      *dead = 0xDEADBEEF; // no-warning
    }
  }
}

// Test pointers increment correctly.
void f() {
  int a[2];
  a[1] = 3;
  int *p = a;
  p++;
  if (*p != 3) {
    int *q = 0;
    *q = 3; // no-warning
  }
}

// <rdar://problem/7185607>
// Bit-fields of a struct should be invalidated when blasting the entire
// struct with an integer constant.
struct test_7185607 {
  int x : 10;
  int y : 22;
};
int rdar_test_7185607() {
  struct test_7185607 s; // Uninitialized.
  *((unsigned *) &s) = 0U;
  return s.x; // no-warning
}

// <rdar://problem/7242006> [RegionStore] compound literal assignment with
//  floats not honored
// This test case is mirrored in misc-ps.m, but this case is a negative.
typedef float CGFloat;
typedef struct _NSSize {
    CGFloat width;
    CGFloat height;
} NSSize;

CGFloat rdar7242006_negative(CGFloat x) {
  NSSize y;
  return y.width; // expected-warning{{garbage}}
}

// <rdar://problem/7249340> - Allow binding of values to symbolic regions.
// This test case shows how RegionStore tracks the value bound to 'x'
// after the assignment.
typedef int* ptr_rdar_7249340;
void rdar_7249340(ptr_rdar_7249340 x) {
  *x = 1;
  if (*x)
    return;
  int *p = 0;   // This is unreachable.
  *p = 0xDEADBEEF; // no-warning
}

// <rdar://problem/7249327> - This test case tests both value tracking of
// array values and that we handle symbolic values that are casted
// between different integer types.  Note the assignment 'n = *a++'; here
// 'n' is and 'int' and '*a' is 'unsigned'.  Previously we got a false positive
// at 'x += *b++' (undefined value) because we got a false path.
int rdar_7249327_aux(void);

void rdar_7249327(unsigned int A[2*32]) {
  int B[2*32];
  int *b;
  unsigned int *a;
  int x = 0;
  
  int n;
  
  a = A;
  b = B;
  
  n = *a++;
  if (n)
    *b++ = rdar_7249327_aux();

  a = A;
  b = B;
  
  n = *a++;
  if (n)
    x += *b++; // no-warning
}

// <rdar://problem/6914474> - Check that 'x' is invalidated because its
// address is passed in as a value to a struct.
struct doodad_6914474 { int *v; };
extern void prod_6914474(struct doodad_6914474 *d);
int rdar_6914474(void) {
  int x;
  struct doodad_6914474 d;
  d.v = &x;
  prod_6914474(&d);
  return x; // no-warning
}

// Test invalidation of a single field.
struct s_test_field_invalidate {
  int x;
};
extern void test_invalidate_field(int *x);
int test_invalidate_field_test() {
  struct s_test_field_invalidate y;
  test_invalidate_field(&y.x);
  return y.x; // no-warning
}
int test_invalidate_field_test_positive() {
  struct s_test_field_invalidate y;
  return y.x; // expected-warning{{garbage}}
}

// This test case illustrates how a typeless array of bytes casted to a
// struct should be treated as initialized.  RemoveDeadBindings previously
// had a bug that caused 'x' to lose its default symbolic value after the
// assignment to 'p', thus causing 'p->z' to evaluate to "undefined".
struct ArrayWrapper { unsigned char y[16]; };
struct WrappedStruct { unsigned z; };

int test_handle_array_wrapper() {
  struct ArrayWrapper x;
  test_handle_array_wrapper(&x);
  struct WrappedStruct *p = (struct WrappedStruct*) x.y;
  return p->z;  // no-warning
}

// <rdar://problem/7261075> [RegionStore] crash when 
//   handling load: '*((unsigned int *)"????")'
int rdar_7261075(void) {
  unsigned int var = 0;
  if (var == *((unsigned int *)"????"))
    return 1;
  return 0;
}


// RUN: clang -checker-simple -verify %s

#include<stdint.h>

void f1(int *p) {  
  if (p) *p = 1;
  else *p = 0; // expected-warning{{ereference}}
}

struct foo_struct {
  int x;
};

int f2(struct foo_struct* p) {
  
  if (p)
    p->x = 1;
    
  return p->x++; // expected-warning{{Dereference of null pointer.}}
}

int f3(char* x) {
  
  int i = 2;
  
  if (x)
    return x[i - 1];
  
  return x[i+1]; // expected-warning{{Dereference of null pointer.}}
}

int f3_b(char* x) {
  
  int i = 2;
  
  if (x)
    return x[i - 1];
  
  return x[i+1]++; // expected-warning{{Dereference of null pointer.}}
}

int f4(int *p) {
  
  uintptr_t x = p;
  
  if (x)
    return 1;
    
  int *q = (int*) x;
  return *q; // expected-warning{{Dereference of null pointer.}}
}

int f5() {
  
  char *s = "hello world";
  return s[0]; // no-warning
}

int bar(int* p) __attribute__((nonnull));

int f6(int *p) { 
  return !p ? bar(p) : *p; // expected-warning {{Null pointer passed as an argument to a 'nonnull' parameter}}
} 

int* qux();

int f7(int x) {
  
  int* p = 0;
  
  if (0 == x)
    p = qux();
  
  if (0 == x)
    *p = 1; // no-warning
    
  return x;
}



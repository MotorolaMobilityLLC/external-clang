// RUN: %clang_cc1 -fsyntax-only -verify %s
int *use_new(int N) {
  if (N == 1)
    return new int;
  
  return new int [N];
}

void use_delete(int* ip, int N) {
  if (N == 1)
    delete ip;
  else
    delete [] ip;
}

namespace std {
  class bad_alloc { };
  
  typedef __SIZE_TYPE__ size_t;
}

void* operator new(std::size_t) throw(std::bad_alloc); 
void* operator new[](std::size_t) throw(std::bad_alloc); 
void operator delete(void*) throw(); 
void operator delete[](void*) throw();

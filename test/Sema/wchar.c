// RUN: clang-cc %s -fsyntax-only -verify &&
// RUN: clang-cc %s -fsyntax-only -fshort-wchar -verify -DSHORT_WCHAR
 
#include <wchar.h>
 
#if defined(_WIN32) || defined(_M_IX86) || defined(__CYGWIN__) \
 || defined(_M_X64) || defined(SHORT_WCHAR)
  #define WCHAR_T_TYPE unsigned short
#else
  #define WCHAR_T_TYPE int
#endif
 
int check_wchar_size[sizeof(*L"") == sizeof(wchar_t) ? 1 : -1];
 
void foo() {
  WCHAR_T_TYPE t1[] = L"x";
  wchar_t tab[] = L"x";
  WCHAR_T_TYPE t2[] = "x";     // expected-error {{initialization}}
  char t3[] = L"x";   // expected-error {{initialization}}
}

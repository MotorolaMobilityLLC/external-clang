// RUN: clang-cc -triple x86_64-apple-darwin -S %s -o - | FileCheck %s

#define strcpy(dest, src) \
  ((__builtin_object_size(dest, 0) != -1ULL) \
   ? __builtin___strcpy_chk (dest, src, __builtin_object_size(dest, 1)) \
   : __inline_strcpy_chk(dest, src))

static char *__inline_strcpy_chk (char *dest, const char *src) {
  return __builtin___strcpy_chk(dest, src, __builtin_object_size(dest, 1));
}

char gbuf[63];
char *gp;

void test1() {
  // CHECK:       movabsq $59, %rdx
  // CHECK-NEXT:  movq    %rax, %rdi
  // CHECK-NEXT:  movq    %rcx, %rsi
  // CHECK-NEXT:  call    ___strcpy_chk
  strcpy(&gbuf[4], "Hi there");
}

void test2() {
  // CHECK:       movabsq $63, %rdx
  // CHECK-NEXT:  movq    %rax, %rdi
  // CHECK-NEXT:  movq    %rcx, %rsi
  // CHECK-NEXT:  call    ___strcpy_chk
  strcpy(gbuf, "Hi there");
}

void test3() {
  // CHECK:       movabsq $0, %rdx
  // CHECK-NEXT:  movq    %rax, %rdi
  // CHECK-NEXT:  movq    %rcx, %rsi
  // CHECK-NEXT:  call    ___strcpy_chk
  strcpy(&gbuf[100], "Hi there");
}

void test4() {
  // CHECK:       movabsq $0, %rdx
  // CHECK-NEXT:  movq    %rax, %rdi
  // CHECK-NEXT:  movq    %rcx, %rsi
  // CHECK-NEXT:  call    ___strcpy_chk
  strcpy((char*)(void*)&gbuf[-1], "Hi there");
}

void test5() {
  // CHECK:       call    ___inline_strcpy_chk
  strcpy(gp, "Hi there");
}

void test6() {
  int i;
  // CHECK:       call    ___inline_strcpy_chk
  strcpy((++i, gbuf), "Hi there");
}

void test7() {
  char buf[57];

  // CHECK:       movabsq $53, %rdx
  // CHECK-NEXT:  movq    %rax, %rdi
  // CHECK-NEXT:  movq    %rcx, %rsi
  // CHECK-NEXT:  call    ___strcpy_chk
  strcpy(&buf[4], "Hi there");
}

// RUN: clang -verify -fsyntax-only -std=c90 %s &&
// RUN: clang -verify -fsyntax-only -std=c99 %s

struct s
{
  int a;
};

int a[__builtin_offsetof(struct s, a) == 0];

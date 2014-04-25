// RUN: %clang_cc1 -ffreestanding -fsyntax-only -std=c++11 %s
// DISABLED: %clang_cc1 -ffreestanding -fsyntax-only -std=c++11 -fmodules %s

// FIXME: This test fails on OS X 10.9 with headers in /usr/include, see PR18322

#include <stdalign.h>

#if defined alignas
#error alignas should not be defined in C++
#endif

#if defined alignof
#error alignof should not be defined in C++
#endif

static_assert(__alignas_is_defined, "");
static_assert(__alignof_is_defined, "");


#include <stdint.h>

#ifndef SIZE_MAX
#error SIZE_MAX should be defined in C++
#endif

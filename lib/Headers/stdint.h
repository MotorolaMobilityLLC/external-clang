/*===---- stdint.h - Standard header for sized integer types --------------===*\
 *
 * Copyright (c) 2009 Chris Lattner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
\*===----------------------------------------------------------------------===*/

#ifndef __CLANG_STDINT_H
#define __CLANG_STDINT_H

/* If we're hosted, fall back to the system's stdint.h, which might have
 * additional definitions.
 */
#if __STDC_HOSTED__ && \
    defined(__has_include_next) && __has_include_next(<stdint.h>)
# include_next <stdint.h>
#else

/* C99 7.18.1.1 Exact-width integer types.
 * C99 7.18.1.2 Minimum-width integer types.
 * C99 7.18.1.3 Fastest minimum-width integer types.
 *
 * The standard requires that exact-width type be defined for 8-, 16-, 32-, and 
 * 64-bit types if they are implemented. Other exact width types are optional.
 * This implementation defines an exact-width types for every integer width
 * that is represented in the standard integer types.
 *
 * The standard also requires minimum-width types be defined for 8-, 16-, 32-,
 * and 64-bit widths regardless of whether there are corresponding exact-width
 * types. 
 *
 * To accomodate targets that are missing types that are exactly 8, 16, 32, or
 * 64 bits wide, this implementation takes an approach of cascading
 * redefintions, redefining __int_leastN_t to successively smaller exact-width
 * types. It is therefore important that the types are defined in order of
 * descending widths.
 *
 * We currently assume that the minimum-width types and the fastest
 * minimum-width types are the same. This is allowed by the standard, but is
 * suboptimal.
 *
 * In violation of the standard, some targets do not implement a type that is
 * wide enough to represent all of the required widths (8-, 16-, 32-, 64-bit).  
 * To accomodate these targets, a required minimum-width type is only
 * defined if there exists an exact-width type of equal or greater width.
 */

#ifdef __INT64_TYPE__
# ifndef __int8_t_defined /* glibc sys/types.h also defines int64_t*/
typedef signed __INT64_TYPE__ int64_t;
# endif /* __int8_t_defined */
typedef unsigned __INT64_TYPE__ uint64_t;
# define __int_least64_t int64_t
# define __uint_least64_t uint64_t
# define __int_least32_t int64_t
# define __uint_least32_t uint64_t
# define __int_least16_t int64_t
# define __uint_least16_t uint64_t
# define __int_least8_t int64_t
# define __uint_least8_t uint64_t
#endif /* __INT64_TYPE__ */

#ifdef __int_least64_t
typedef __int_least64_t int_least64_t;
typedef __uint_least64_t uint_least64_t;
typedef __int_least64_t int_fast64_t;
typedef __uint_least64_t uint_fast64_t;
#endif /* __int_least64_t */

#ifdef __INT56_TYPE__
typedef signed __INT56_TYPE__ int56_t;
typedef unsigned __INT56_TYPE__ uint56_t;
typedef int56_t int_least56_t;
typedef uint56_t uint_least56_t;
typedef int56_t int_fast56_t;
typedef uint56_t uint_fast56_t;
# define __int_least32_t int56_t
# define __uint_least32_t uint56_t
# define __int_least16_t int56_t
# define __uint_least16_t uint56_t
# define __int_least8_t int56_t
# define __uint_least8_t uint56_t
#endif /* __INT56_TYPE__ */


#ifdef __INT48_TYPE__
typedef signed __INT48_TYPE__ int48_t;
typedef unsigned __INT48_TYPE__ uint48_t;
typedef int48_t int_least48_t;
typedef uint48_t uint_least48_t;
typedef int48_t int_fast48_t;
typedef uint48_t uint_fast48_t;
# define __int_least32_t int48_t
# define __uint_least32_t uint48_t
# define __int_least16_t int48_t
# define __uint_least16_t uint48_t
# define __int_least8_t int48_t
# define __uint_least8_t uint48_t
#endif /* __INT48_TYPE__ */


#ifdef __INT40_TYPE__
typedef signed __INT40_TYPE__ int40_t;
typedef unsigned __INT40_TYPE__ uint40_t;
typedef int40_t int_least40_t;
typedef uint40_t uint_least40_t;
typedef int40_t int_fast40_t;
typedef uint40_t uint_fast40_t;
# define __int_least32_t int40_t
# define __uint_least32_t uint40_t
# define __int_least16_t int40_t
# define __uint_least16_t uint40_t
# define __int_least8_t int40_t
# define __uint_least8_t uint40_t
#endif /* __INT40_TYPE__ */


#ifdef __INT32_TYPE__

# ifndef __int8_t_defined /* glibc sys/types.h also defines int32_t*/
typedef signed __INT32_TYPE__ int32_t;
# endif /* __int8_t_defined */

# ifndef __uint32_t_defined  /* more glibc compatibility */
# define __uint32_t_defined
typedef unsigned __INT32_TYPE__ uint32_t;
# endif /* __uint32_t_defined */

# define __int_least32_t int32_t
# define __uint_least32_t uint32_t
# define __int_least16_t int32_t
# define __uint_least16_t uint32_t
# define __int_least8_t int32_t
# define __uint_least8_t uint32_t
#endif /* __INT32_TYPE__ */

#ifdef __int_least32_t
typedef __int_least32_t int_least32_t;
typedef __uint_least32_t uint_least32_t;
typedef __int_least32_t int_fast32_t;
typedef __uint_least32_t uint_fast32_t;
#endif /* __int_least32_t */

#ifdef __INT24_TYPE__
typedef signed __INT24_TYPE__ int24_t;
typedef unsigned __INT24_TYPE__ uint24_t;
typedef int24_t int_least24_t;
typedef uint24_t uint_least24_t;
typedef int24_t int_fast24_t;
typedef uint24_t uint_fast24_t;
# define __int_least16_t int24_t
# define __uint_least16_t uint24_t
# define __int_least8_t int24_t
# define __uint_least8_t uint24_t
#endif /* __INT24_TYPE__ */

#ifdef __INT16_TYPE__
#ifndef __int8_t_defined /* glibc sys/types.h also defines int16_t*/
typedef signed __INT16_TYPE__ int16_t;
#endif /* __int8_t_defined */
typedef unsigned __INT16_TYPE__ uint16_t;
# define __int_least16_t int16_t
# define __uint_least16_t uint16_t
# define __int_least8_t int16_t
# define __uint_least8_t uint16_t
#endif /* __INT16_TYPE__ */

#ifdef __int_least16_t
typedef __int_least16_t int_least16_t;
typedef __uint_least16_t uint_least16_t;
typedef __int_least16_t int_fast16_t;
typedef __uint_least16_t uint_fast16_t;
#endif /* __int_least16_t */


#ifdef __INT8_TYPE__
#ifndef __int8_t_defined  /* glibc sys/types.h also defines int8_t*/
typedef signed __INT8_TYPE__ int8_t;
#endif /* __int8_t_defined */
typedef unsigned __INT8_TYPE__ uint8_t;
# define __int_least8_t int8_t
# define __uint_least8_t uint8_t
#endif /* __INT8_TYPE__ */

#ifdef __int_least8_t
typedef __int_least8_t int_least8_t;
typedef __uint_least8_t uint_least8_t;
typedef __int_least8_t int_fast8_t;
typedef __uint_least8_t uint_fast8_t;
#endif /* __int_least8_t */

/* prevent glibc sys/types.h from defining conflicting types */
#ifndef __int8_t_defined  
# define __int8_t_defined
#endif /* __int8_t_defined */

/* C99 7.18.1.4 Integer types capable of holding object pointers.
 */
#ifndef __intptr_t_defined
typedef __INTPTR_TYPE__          intptr_t;
#define __intptr_t_defined
#endif
typedef unsigned __INTPTR_TYPE__ uintptr_t;

/* C99 7.18.1.5 Greatest-width integer types.
 */
typedef __INTMAX_TYPE__   intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

/* C99 7.18.4 Macros for minimum-width integer constants.
 *
 * The standard requires that integer constant macros be defined for all the
 * minimum-width types defined above. As 8-, 16-, 32-, and 64-bit minimum-width
 * types are required, the corresponding integer constant macros are defined 
 * here. This implementation also defines minimum-width types for every other
 * integer width that the target implements, so corresponding macros are 
 * defined below, too.
 *
 * These macros are defined using the same successive-shrinking approach as
 * the type definitions above. It is likewise important that macros are defined
 * in order of decending width.
 *
 * Note that C++ should not check __STDC_CONSTANT_MACROS here, contrary to the
 * claims of the C standard (see C++ 18.3.1p2, [cstdint.syn]).
 */

#define __int_c_join(a, b) a ## b
#define __int_c(v, suffix) __int_c_join(v, suffix)
#define __uint_c(v, suffix) __int_c_join(v##U, suffix)


#ifdef __INT64_TYPE__
# ifdef __INT64_C_SUFFIX__
#  define __int64_c_suffix __INT64_C_SUFFIX__
#  define __int32_c_suffix __INT64_C_SUFFIX__
#  define __int16_c_suffix __INT64_C_SUFFIX__
#  define  __int8_c_suffix __INT64_C_SUFFIX__
# else
#  undef __int64_c_suffix
#  undef __int32_c_suffix
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT64_C_SUFFIX__ */
#endif /* __INT64_TYPE__ */

#ifdef __int_least64_t
# ifdef __int64_c_suffix
#  define INT64_C(v) __int_c(v, __int64_c_suffix)
#  define UINT64_C(v) __uint_c(v, __int64_c_suffix)
# else
#  define INT64_C(v) v
#  define UINT64_C(v) v ## U
# endif /* __int64_c_suffix */
#endif /* __int_least64_t */


#ifdef __INT56_TYPE__
# ifdef __INT56_C_SUFFIX__
#  define INT56_C(v) __int_c(v, __INT56_C_SUFFIX__)
#  define UINT56_C(v) __uint_c(v, __INT56_C_SUFFIX__)
#  define __int32_c_suffix __INT56_C_SUFFIX__
#  define __int16_c_suffix __INT56_C_SUFFIX__
#  define __int8_c_suffix  __INT56_C_SUFFIX__
# else
#  define INT56_C(v) v
#  define UINT56_C(v) v ## U
#  undef __int32_c_suffix
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT56_C_SUFFIX__ */
#endif /* __INT56_TYPE__ */


#ifdef __INT48_TYPE__
# ifdef __INT48_C_SUFFIX__
#  define INT48_C(v) __int_c(v, __INT48_C_SUFFIX__)
#  define UINT48_C(v) __uint_c(v, __INT48_C_SUFFIX__)
#  define __int32_c_suffix __INT48_C_SUFFIX__
#  define __int16_c_suffix __INT48_C_SUFFIX__
#  define __int8_c_suffix  __INT48_C_SUFFIX__
# else
#  define INT48_C(v) v
#  define UINT48_C(v) v ## U
#  undef __int32_c_suffix
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT48_C_SUFFIX__ */
#endif /* __INT48_TYPE__ */


#ifdef __INT40_TYPE__
# ifdef __INT40_C_SUFFIX__
#  define INT40_C(v) __int_c(v, __INT40_C_SUFFIX__)
#  define UINT40_C(v) __uint_c(v, __INT40_C_SUFFIX__)
#  define __int32_c_suffix __INT40_C_SUFFIX__
#  define __int16_c_suffix __INT40_C_SUFFIX__
#  define __int8_c_suffix  __INT40_C_SUFFIX__
# else
#  define INT40_C(v) v
#  define UINT40_C(v) v ## U
#  undef __int32_c_suffix
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT40_C_SUFFIX__ */
#endif /* __INT40_TYPE__ */


#ifdef __INT32_TYPE__
# ifdef __INT32_C_SUFFIX__
#  define __int32_c_suffix __INT32_C_SUFFIX__
#  define __int16_c_suffix __INT32_C_SUFFIX__
#  define __int8_c_suffix  __INT32_C_SUFFIX__
#else
#  undef __int32_c_suffix
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT32_C_SUFFIX__ */
#endif /* __INT32_TYPE__ */

#ifdef __int_least32_t
# ifdef __int32_c_suffix
#  define INT32_C(v) __int_c(v, __int32_c_suffix)
#  define UINT32_C(v) __uint_c(v, __int32_c_suffix)
# else
#  define INT32_C(v) v
#  define UINT32_C(v) v ## U
# endif /* __int32_c_suffix */
#endif /* __int_least32_t */


#ifdef __INT24_TYPE__
# ifdef __INT24_C_SUFFIX__
#  define INT24_C(v) __int_c(v, __INT24_C_SUFFIX__)
#  define UINT24_C(v) __uint_c(v, __INT24_C_SUFFIX__)
#  define __int16_c_suffix __INT24_C_SUFFIX__
#  define __int8_c_suffix  __INT24_C_SUFFIX__
# else
#  define INT24_C(v) v
#  define UINT24_C(v) v ## U
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT24_C_SUFFIX__ */
#endif /* __INT24_TYPE__ */


#ifdef __INT16_TYPE__
# ifdef __INT16_C_SUFFIX__
#  define __int16_c_suffix __INT16_C_SUFFIX__
#  define __int8_c_suffix  __INT16_C_SUFFIX__
#else
#  undef __int16_c_suffix
#  undef  __int8_c_suffix
# endif /* __INT16_C_SUFFIX__ */
#endif /* __INT16_TYPE__ */

#ifdef __int_least16_t
# ifdef __int16_c_suffix
#  define INT16_C(v) __int_c(v, __int16_c_suffix)
#  define UINT16_C(v) __uint_c(v, __int16_c_suffix)
# else
#  define INT16_C(v) v
#  define UINT16_C(v) v ## U
# endif /* __int16_c_suffix */
#endif /* __int_least16_t */


#ifdef __INT8_TYPE__
# ifdef __INT8_C_SUFFIX__
#  define __int8_c_suffix __INT8_C_SUFFIX__
#else
#  undef  __int8_c_suffix
# endif /* __INT8_C_SUFFIX__ */
#endif /* __INT8_TYPE__ */

#ifdef __int_least8_t
# ifdef __int8_c_suffix
#  define INT8_C(v) __int_c(v, __int8_c_suffix)
#  define UINT8_C(v) __uint_c(v, __int8_c_suffix)
# else
#  define INT8_C(v) v
#  define UINT8_C(v) v ## U
# endif /* __int8_c_suffix */
#endif /* __int_least8_t */


/* C99 7.18.2.1 Limits of exact-width integer types. 
 * C99 7.18.2.2 Limits of minimum-width integer types.
 * C99 7.18.2.3 Limits of fastest minimum-width integer types.
 *
 * The presence of limit macros are completely optional in C99.  This
 * implementation defines limits for all of the types (exact- and
 * minimum-width) that it defines above, using the limits of the minimum-width
 * type for any types that do not have exact-width representations.
 *
 * As in the type definitions, this section takes an approach of
 * successive-shrinking to determine which limits to use for the standard (8,
 * 16, 32, 64) bit widths when they don't have exact representations. It is
 * therefore important that the defintions be kept in order of decending
 * widths.
 *
 * Note that C++ should not check __STDC_LIMIT_MACROS here, contrary to the
 * claims of the C standard (see C++ 18.3.1p2, [cstdint.syn]).
 */

#ifdef __INT64_TYPE__
# define INT64_MAX           INT64_C( 9223372036854775807)
# define INT64_MIN         (-INT64_C( 9223372036854775807)-1)
# define UINT64_MAX         UINT64_C(18446744073709551615)
# define __INT_LEAST64_MIN   INT64_MIN
# define __INT_LEAST64_MAX   INT64_MAX
# define __UINT_LEAST64_MAX UINT64_MAX
# define __INT_LEAST32_MIN   INT64_MIN
# define __INT_LEAST32_MAX   INT64_MAX
# define __UINT_LEAST32_MAX UINT64_MAX
# define __INT_LEAST16_MIN   INT64_MIN
# define __INT_LEAST16_MAX   INT64_MAX
# define __UINT_LEAST16_MAX UINT64_MAX
# define __INT_LEAST8_MIN    INT64_MIN
# define __INT_LEAST8_MAX    INT64_MAX
# define __UINT_LEAST8_MAX  UINT64_MAX
#endif /* __INT64_TYPE__ */

#ifdef __INT_LEAST64_MIN
# define INT_LEAST64_MIN   __INT_LEAST64_MIN
# define INT_LEAST64_MAX   __INT_LEAST64_MAX
# define UINT_LEAST64_MAX __UINT_LEAST64_MAX
# define INT_FAST64_MIN    __INT_LEAST64_MIN
# define INT_FAST64_MAX    __INT_LEAST64_MAX
# define UINT_FAST64_MAX  __UINT_LEAST64_MAX
#endif /* __INT_LEAST64_MIN */


#ifdef __INT56_TYPE__
# define INT56_MAX           INT56_C(36028797018963967)
# define INT56_MIN         (-INT56_C(36028797018963967)-1)
# define UINT56_MAX         UINT56_C(72057594037927935)
# define INT_LEAST56_MIN     INT56_MIN
# define INT_LEAST56_MAX     INT56_MAX
# define UINT_LEAST56_MAX   UINT56_MAX
# define INT_FAST56_MIN      INT56_MIN
# define INT_FAST56_MAX      INT56_MAX
# define UINT_FAST56_MAX    UINT56_MAX
# define __INT_LEAST32_MIN   INT56_MIN
# define __INT_LEAST32_MAX   INT56_MAX
# define __UINT_LEAST32_MAX UINT56_MAX
# define __INT_LEAST16_MIN   INT56_MIN
# define __INT_LEAST16_MAX   INT56_MAX
# define __UINT_LEAST16_MAX UINT56_MAX
# define __INT_LEAST8_MIN    INT56_MIN
# define __INT_LEAST8_MAX    INT56_MAX
# define __UINT_LEAST8_MAX  UINT56_MAX
#endif /* __INT56_TYPE__ */


#ifdef __INT48_TYPE__
# define INT48_MAX           INT48_C(140737488355327)
# define INT48_MIN         (-INT48_C(140737488355327)-1)
# define UINT48_MAX         UINT48_C(281474976710655)
# define INT_LEAST48_MIN     INT48_MIN
# define INT_LEAST48_MAX     INT48_MAX
# define UINT_LEAST48_MAX   UINT48_MAX
# define INT_FAST48_MIN      INT48_MIN
# define INT_FAST48_MAX      INT48_MAX
# define UINT_FAST48_MAX    UINT48_MAX
# define __INT_LEAST32_MIN   INT48_MIN
# define __INT_LEAST32_MAX   INT48_MAX
# define __UINT_LEAST32_MAX UINT48_MAX
# define __INT_LEAST16_MIN   INT48_MIN
# define __INT_LEAST16_MAX   INT48_MAX
# define __UINT_LEAST16_MAX UINT48_MAX
# define __INT_LEAST8_MIN    INT48_MIN
# define __INT_LEAST8_MAX    INT48_MAX
# define __UINT_LEAST8_MAX  UINT48_MAX
#endif /* __INT48_TYPE__ */


#ifdef __INT40_TYPE__
# define INT40_MAX           INT40_C(549755813887)
# define INT40_MIN         (-INT40_C(549755813887)-1)
# define UINT40_MAX         UINT40_C(1099511627775)
# define INT_LEAST40_MIN     INT40_MIN
# define INT_LEAST40_MAX     INT40_MAX
# define UINT_LEAST40_MAX   UINT40_MAX
# define INT_FAST40_MIN      INT40_MIN
# define INT_FAST40_MAX      INT40_MAX
# define UINT_FAST40_MAX    UINT40_MAX
# define __INT_LEAST32_MIN   INT40_MIN
# define __INT_LEAST32_MAX   INT40_MAX
# define __UINT_LEAST32_MAX UINT40_MAX
# define __INT_LEAST16_MIN   INT40_MIN
# define __INT_LEAST16_MAX   INT40_MAX
# define __UINT_LEAST16_MAX UINT40_MAX
# define __INT_LEAST8_MIN    INT40_MIN
# define __INT_LEAST8_MAX    INT40_MAX
# define __UINT_LEAST8_MAX  UINT40_MAX
#endif /* __INT40_TYPE__ */


#ifdef __INT32_TYPE__
# define INT32_MAX           INT32_C(2147483647)
# define INT32_MIN         (-INT32_C(2147483647)-1)
# define UINT32_MAX         UINT32_C(4294967295)
# define __INT_LEAST32_MIN   INT32_MIN
# define __INT_LEAST32_MAX   INT32_MAX
# define __UINT_LEAST32_MAX UINT32_MAX
# define __INT_LEAST16_MIN   INT32_MIN
# define __INT_LEAST16_MAX   INT32_MAX
# define __UINT_LEAST16_MAX UINT32_MAX
# define __INT_LEAST8_MIN    INT32_MIN
# define __INT_LEAST8_MAX    INT32_MAX
# define __UINT_LEAST8_MAX  UINT32_MAX
#endif /* __INT32_TYPE__ */

#ifdef __INT_LEAST32_MIN
# define INT_LEAST32_MIN   __INT_LEAST32_MIN
# define INT_LEAST32_MAX   __INT_LEAST32_MAX
# define UINT_LEAST32_MAX __UINT_LEAST32_MAX
# define INT_FAST32_MIN    __INT_LEAST32_MIN
# define INT_FAST32_MAX    __INT_LEAST32_MAX
# define UINT_FAST32_MAX  __UINT_LEAST32_MAX
#endif /* __INT_LEAST32_MIN */


#ifdef __INT24_TYPE__
# define INT24_MAX           INT24_C(8388607)
/* FIXME: argument of 8388608 for INT24_C is outside of legal range */
# define INT24_MIN         (-INT24_C(8388608))
# define UINT24_MAX         UINT24_C(16777215)
# define INT_LEAST24_MIN     INT24_MIN
# define INT_LEAST24_MAX     INT24_MAX
# define UINT_LEAST24_MAX   UINT24_MAX
# define INT_FAST24_MIN      INT24_MIN
# define INT_FAST24_MAX      INT24_MAX
# define UINT_FAST24_MAX    UINT24_MAX
# define __INT_LEAST16_MIN   INT24_MIN
# define __INT_LEAST16_MAX   INT24_MAX
# define __UINT_LEAST16_MAX UINT24_MAX
# define __INT_LEAST8_MIN    INT24_MIN
# define __INT_LEAST8_MAX    INT24_MAX
# define __UINT_LEAST8_MAX  UINT24_MAX
#endif /* __INT24_TYPE__ */


#ifdef __INT16_TYPE__
#define INT16_MAX            INT16_C(32767)
/* FIXME: argument of 32768 for INT16_C is outside of legal range */
#define INT16_MIN          (-INT16_C(32768))
#define UINT16_MAX          UINT16_C(65535)
# define __INT_LEAST16_MIN   INT16_MIN
# define __INT_LEAST16_MAX   INT16_MAX
# define __UINT_LEAST16_MAX UINT16_MAX
# define __INT_LEAST8_MIN    INT16_MIN
# define __INT_LEAST8_MAX    INT16_MAX
# define __UINT_LEAST8_MAX  UINT16_MAX
#endif /* __INT16_TYPE__ */

#ifdef __INT_LEAST16_MIN
# define INT_LEAST16_MIN   __INT_LEAST16_MIN
# define INT_LEAST16_MAX   __INT_LEAST16_MAX
# define UINT_LEAST16_MAX __UINT_LEAST16_MAX
# define INT_FAST16_MIN    __INT_LEAST16_MIN
# define INT_FAST16_MAX    __INT_LEAST16_MAX
# define UINT_FAST16_MAX  __UINT_LEAST16_MAX
#endif /* __INT_LEAST16_MIN */


#ifdef __INT8_TYPE__
# define INT8_MAX            INT8_C(127)
/* FIXME: argument of 128 for INT8_C is outside of legal range */
# define INT8_MIN          (-INT8_C(128))
# define UINT8_MAX          UINT8_C(255)
# define __INT_LEAST8_MIN    INT8_MIN
# define __INT_LEAST8_MAX    INT8_MAX
# define __UINT_LEAST8_MAX  UINT8_MAX
#endif /* __INT8_TYPE__ */

#ifdef __INT_LEAST8_MIN
# define INT_LEAST8_MIN   __INT_LEAST8_MIN
# define INT_LEAST8_MAX   __INT_LEAST8_MAX
# define UINT_LEAST8_MAX __UINT_LEAST8_MAX
# define INT_FAST8_MIN    __INT_LEAST8_MIN
# define INT_FAST8_MAX    __INT_LEAST8_MAX
# define UINT_FAST8_MAX  __UINT_LEAST8_MAX
#endif /* __INT_LEAST8_MIN */

/* C99 7.18.2.4 Limits of integer types capable of holding object pointers. */
/* C99 7.18.3 Limits of other integer types. */

#if __POINTER_WIDTH__ == 64

#define  INTPTR_MIN  INT64_MIN
#define  INTPTR_MAX  INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#define PTRDIFF_MIN  INT64_MIN
#define PTRDIFF_MAX  INT64_MAX
#define SIZE_MAX    UINT64_MAX

#elif __POINTER_WIDTH__ == 32

#define  INTPTR_MIN  INT32_MIN
#define  INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define PTRDIFF_MIN  INT32_MIN
#define PTRDIFF_MAX  INT32_MAX
#define SIZE_MAX    UINT32_MAX

#elif __POINTER_WIDTH__ == 16

#define  INTPTR_MIN  INT16_MIN
#define  INTPTR_MAX  INT16_MAX
#define UINTPTR_MAX UINT16_MAX
#define PTRDIFF_MIN  INT16_MIN
#define PTRDIFF_MAX  INT16_MAX
#define SIZE_MAX    UINT16_MAX

#else
#error "unknown or unset pointer width!"
#endif

/* C99 7.18.2.5 Limits of greatest-width integer types. */
#define INTMAX_MIN  (-__INTMAX_MAX__-1)
#define INTMAX_MAX   __INTMAX_MAX__
#define UINTMAX_MAX (__INTMAX_MAX__*2ULL+1ULL)

/* C99 7.18.3 Limits of other integer types. */
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX
#define WINT_MIN       INT32_MIN
#define WINT_MAX       INT32_MAX

/* FIXME: if we ever support a target with unsigned wchar_t, this should be
 * 0 .. Max.
 */
#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX__
#endif
#ifndef WCHAR_MIN
#define WCHAR_MIN (-__WCHAR_MAX__-1)
#endif

/* 7.18.4.2 Macros for greatest-width integer constants. */
#define INTMAX_C(v)  v##LL
#define UINTMAX_C(v) v##ULL

#endif /* __STDC_HOSTED__ */
#endif /* __CLANG_STDINT_H */

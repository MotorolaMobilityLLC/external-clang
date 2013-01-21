// RUN: %clang -target mipsel-unknown-linux -O3 -S -o - -emit-llvm %s | FileCheck %s -check-prefix=O32
// RUN: %clang -target mips64el-unknown-linux -O3 -S -mabi=n64 -o - -emit-llvm %s | FileCheck %s -check-prefix=N64

// check that
// 1. vector arguments are passed in integer registers
// 2. argument alignment is no larger than 8-byte for O32 and 16-byte for N64.

typedef float  v4sf __attribute__ ((__vector_size__ (16)));
typedef int v4i32 __attribute__ ((__vector_size__ (16)));

// O32: define void @test_v4sf(i32 %a1.coerce0, i32 %a1.coerce1, i32 %a1.coerce2, i32 %a1.coerce3, i32 %a2, i32, i32 %a3.coerce0, i32 %a3.coerce1, i32 %a3.coerce2, i32 %a3.coerce3) nounwind 
// O32: declare i32 @test_v4sf_2(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
// N64: define void @test_v4sf(i64 %a1.coerce0, i64 %a1.coerce1, i32 %a2, i64, i64 %a3.coerce0, i64 %a3.coerce1) nounwind
// N64: declare i32 @test_v4sf_2(i64, i64, i32, i64, i64, i64)
extern test_v4sf_2(v4sf, int, v4sf);
void test_v4sf(v4sf a1, int a2, v4sf a3) {
  test_v4sf_2(a3, a2, a1);
}

// O32: define void @test_v4i32(i32 %a1.coerce0, i32 %a1.coerce1, i32 %a1.coerce2, i32 %a1.coerce3, i32 %a2, i32, i32 %a3.coerce0, i32 %a3.coerce1, i32 %a3.coerce2, i32 %a3.coerce3) nounwind 
// O32: declare i32 @test_v4i32_2(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
// N64: define void @test_v4i32(i64 %a1.coerce0, i64 %a1.coerce1, i32 %a2, i64, i64 %a3.coerce0, i64 %a3.coerce1) nounwind
// N64: declare i32 @test_v4i32_2(i64, i64, i32, i64, i64, i64)
extern test_v4i32_2(v4i32, int, v4i32);
void test_v4i32(v4i32 a1, int a2, v4i32 a3) {
  test_v4i32_2(a3, a2, a1);
}


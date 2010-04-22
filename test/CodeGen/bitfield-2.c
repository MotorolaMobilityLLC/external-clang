// RUN: %clang_cc1 -emit-llvm -triple x86_64 -O3 -o %t.opt.ll %s \
// RUN:   -fdump-record-layouts 2> %t.dump.txt
// RUN: FileCheck -check-prefix=CHECK-RECORD < %t.dump.txt %s
// RUN: FileCheck -check-prefix=CHECK-OPT < %t.opt.ll %s

/****/

// Check that we don't read off the end a packed 24-bit structure.
// PR6176

// CHECK-RECORD: *** Dumping IRgen Record Layout
// CHECK-RECORD: Record: struct s0
// CHECK-RECORD: Layout: <CGRecordLayout
// CHECK-RECORD:   LLVMType:<{ [3 x i8] }>
// CHECK-RECORD:   ContainsPointerToDataMember:0
// CHECK-RECORD:   BitFields:[
// CHECK-RECORD:     <CGBitFieldInfo Size:24 IsSigned:1
// CHECK-RECORD:                     NumComponents:2 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:0 FieldBitStart:0 AccessWidth:16
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:0 TargetBitWidth:16>
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:2 FieldBitStart:0 AccessWidth:8
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:16 TargetBitWidth:8>
struct __attribute((packed)) s0 {
  int f0 : 24;
};

struct s0 g0 = { 0xdeadbeef };

int f0_load(struct s0 *a0) {
  int size_check[sizeof(struct s0) == 3 ? 1 : -1];
  return a0->f0;
}
int f0_store(struct s0 *a0) {
  return (a0->f0 = 1);
}
int f0_reload(struct s0 *a0) {
  return (a0->f0 += 1);
}

// CHECK-OPT: define i64 @test_0()
// CHECK-OPT:  ret i64 1
// CHECK-OPT: }
unsigned long long test_0() {
  struct s0 g0 = { 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g0.f0;
  res ^= f0_load(&g0) ^ f0_store(&g0) ^ f0_reload(&g0);
  res ^= g0.f0;
  return res;
}

/****/

// PR5591

// CHECK-RECORD: *** Dumping IRgen Record Layout
// CHECK-RECORD: Record: struct s1
// CHECK-RECORD: Layout: <CGRecordLayout
// CHECK-RECORD:   LLVMType:<{ [2 x i8], i8 }>
// CHECK-RECORD:   ContainsPointerToDataMember:0
// CHECK-RECORD:   BitFields:[
// CHECK-RECORD:     <CGBitFieldInfo Size:10 IsSigned:1
// CHECK-RECORD:                     NumComponents:1 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:0 FieldBitStart:0 AccessWidth:16
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:0 TargetBitWidth:10>
// CHECK-RECORD:     ]>
// CHECK-RECORD:     <CGBitFieldInfo Size:10 IsSigned:1
// CHECK-RECORD:                     NumComponents:2 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:0 FieldBitStart:10 AccessWidth:16
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:0 TargetBitWidth:6>
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:2 FieldBitStart:0 AccessWidth:8
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:6 TargetBitWidth:4>

#pragma pack(push)
#pragma pack(1)
struct __attribute((packed)) s1 {
  signed f0 : 10;
  signed f1 : 10;
};
#pragma pack(pop)

struct s1 g1 = { 0xdeadbeef, 0xdeadbeef };

int f1_load(struct s1 *a0) {
  int size_check[sizeof(struct s1) == 3 ? 1 : -1];
  return a0->f1;
}
int f1_store(struct s1 *a0) {
  return (a0->f1 = 1234);
}
int f1_reload(struct s1 *a0) {
  return (a0->f1 += 1234);
}

// CHECK-OPT: define i64 @test_1()
// CHECK-OPT:  ret i64 210
// CHECK-OPT: }
unsigned long long test_1() {
  struct s1 g1 = { 0xdeadbeef, 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g1.f0 ^ g1.f1;
  res ^= f1_load(&g1) ^ f1_store(&g1) ^ f1_reload(&g1);
  res ^= g1.f0 ^ g1.f1;
  return res;
}

/****/

// Check that we don't access beyond the bounds of a union.
//
// PR5567

// CHECK-RECORD: *** Dumping IRgen Record Layout
// CHECK-RECORD: Record: union u2
// CHECK-RECORD: Layout: <CGRecordLayout
// CHECK-RECORD:   LLVMType:<{ i8 }>
// CHECK-RECORD:   ContainsPointerToDataMember:0
// CHECK-RECORD:   BitFields:[
// CHECK-RECORD:     <CGBitFieldInfo Size:3 IsSigned:0
// CHECK-RECORD:                     NumComponents:1 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:0 FieldBitStart:0 AccessWidth:8
// CHECK-RECORD:                     AccessAlignment:1 TargetBitOffset:0 TargetBitWidth:3>

union __attribute__((packed)) u2 {
  unsigned long long f0 : 3;
};

union u2 g2 = { 0xdeadbeef };

int f2_load(union u2 *a0) {
  return a0->f0;
}
int f2_store(union u2 *a0) {
  return (a0->f0 = 1234);
}
int f2_reload(union u2 *a0) {
  return (a0->f0 += 1234);
}

// CHECK-OPT: define i64 @test_2()
// CHECK-OPT:  ret i64 2
// CHECK-OPT: }
unsigned long long test_2() {
  union u2 g2 = { 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g2.f0;
  res ^= f2_load(&g2) ^ f2_store(&g2) ^ f2_reload(&g2);
  res ^= g2.f0;
  return res;
}

/***/

// PR5039

struct s3 {
  long long f0 : 32;
  long long f1 : 32;
};

struct s3 g3 = { 0xdeadbeef, 0xdeadbeef };

int f3_load(struct s3 *a0) {
  a0->f0 = 1;
  return a0->f0;
}
int f3_store(struct s3 *a0) {
  a0->f0 = 1;
  return (a0->f0 = 1234);
}
int f3_reload(struct s3 *a0) {
  a0->f0 = 1;
  return (a0->f0 += 1234);
}

// CHECK-OPT: define i64 @test_3()
// CHECK-OPT:  ret i64 -559039940
// CHECK-OPT: }
unsigned long long test_3() {
  struct s3 g3 = { 0xdeadbeef, 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g3.f0 ^ g3.f1;
  res ^= f3_load(&g3) ^ f3_store(&g3) ^ f3_reload(&g3);
  res ^= g3.f0 ^ g3.f1;
  return res;
}

/***/

// This is a case where the bitfield access will straddle an alignment boundary
// of its underlying type.

struct s4 {
  unsigned f0 : 16;
  unsigned f1 : 28 __attribute__ ((packed));
};

struct s4 g4 = { 0xdeadbeef, 0xdeadbeef };

int f4_load(struct s4 *a0) {
  return a0->f0 ^ a0->f1;
}
int f4_store(struct s4 *a0) {
  return (a0->f0 = 1234) ^ (a0->f1 = 5678);
}
int f4_reload(struct s4 *a0) {
  return (a0->f0 += 1234) ^ (a0->f1 += 5678);
}

// CHECK-OPT: define i64 @test_4()
// CHECK-OPT:  ret i64 4860
// CHECK-OPT: }
unsigned long long test_4() {
  struct s4 g4 = { 0xdeadbeef, 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g4.f0 ^ g4.f1;
  res ^= f4_load(&g4) ^ f4_store(&g4) ^ f4_reload(&g4);
  res ^= g4.f0 ^ g4.f1;
  return res;
}

/***/

struct s5 {
  unsigned f0 : 2;
  _Bool f1 : 1;
  _Bool f2 : 1;
};

struct s5 g5 = { 0xdeadbeef, 0xdeadbeef };

int f5_load(struct s5 *a0) {
  return a0->f0 ^ a0->f1;
}
int f5_store(struct s5 *a0) {
  return (a0->f0 = 0xF) ^ (a0->f1 = 0xF) ^ (a0->f2 = 0xF);
}
int f5_reload(struct s5 *a0) {
  return (a0->f0 += 0xF) ^ (a0->f1 += 0xF) ^ (a0->f2 += 0xF);
}

// CHECK-OPT: define i64 @test_5()
// CHECK-OPT:  ret i64 2
// CHECK-OPT: }
unsigned long long test_5() {
  struct s5 g5 = { 0xdeadbeef, 0xdeadbeef, 0xdeadbeef };
  unsigned long long res = 0;
  res ^= g5.f0 ^ g5.f1 ^ g5.f2;
  res ^= f5_load(&g5) ^ f5_store(&g5) ^ f5_reload(&g5);
  res ^= g5.f0 ^ g5.f1 ^ g5.f2;
  return res;
}

/***/

struct s6 {
  _Bool f0 : 2;
};

struct s6 g6 = { 0xF };

int f6_load(struct s6 *a0) {
  return a0->f0;
}
int f6_store(struct s6 *a0) {
  return a0->f0 = 0x0;
}
int f6_reload(struct s6 *a0) {
  return (a0->f0 += 0xF);
}

// CHECK-OPT: define zeroext i1 @test_6()
// CHECK-OPT:  ret i1 true
// CHECK-OPT: }
_Bool test_6() {
  struct s6 g6 = { 0xF };
  unsigned long long res = 0;
  res ^= g6.f0;
  res ^= f6_load(&g6);
  res ^= g6.f0;
  return res;
}

/***/

// Check that we compute the best alignment possible for each access.
//
// CHECK-RECORD: *** Dumping IRgen Record Layout
// CHECK-RECORD: Record: struct s7
// CHECK-RECORD: Layout: <CGRecordLayout
// CHECK-RECORD:   LLVMType:{ i32, i32, i32, i8, [3 x i8], [4 x i8], [12 x i8] }
// CHECK-RECORD:   ContainsPointerToDataMember:0
// CHECK-RECORD:   BitFields:[
// CHECK-RECORD:     <CGBitFieldInfo Size:5 IsSigned:1
// CHECK-RECORD:                     NumComponents:1 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:12 FieldBitStart:0 AccessWidth:32
// CHECK-RECORD:                     AccessAlignment:4 TargetBitOffset:0 TargetBitWidth:5>
// CHECK-RECORD:     ]>
// CHECK-RECORD:     <CGBitFieldInfo Size:29 IsSigned:1
// CHECK-RECORD:                     NumComponents:1 Components: [
// CHECK-RECORD:         <AccessInfo FieldIndex:0 FieldByteOffset:16 FieldBitStart:0 AccessWidth:32
// CHECK-RECORD:                     AccessAlignment:16 TargetBitOffset:0 TargetBitWidth:29>

struct __attribute__((aligned(16))) s7 {
  int a, b, c;
  int f0 : 5;
  int f1 : 29;
};

int f7_load(struct s7 *a0) {
  return a0->f0;
}

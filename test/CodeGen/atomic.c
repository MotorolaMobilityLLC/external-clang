// RUN: clang-cc %s -emit-llvm -o - > %t1 &&
// RUN: grep @llvm.atomic.load.add.i32 %t1 | count 3 &&
// RUN: grep @llvm.atomic.load.sub.i32 %t1 | count 3 &&
// RUN: grep @llvm.atomic.load.min.i32 %t1 &&
// RUN: grep @llvm.atomic.load.max.i32 %t1 &&
// RUN: grep @llvm.atomic.load.umin.i32 %t1 &&
// RUN: grep @llvm.atomic.load.umax.i32 %t1 &&
// RUN: grep @llvm.atomic.swap.i32 %t1 &&
// RUN: grep @llvm.atomic.cmp.swap.i32 %t1 | count 3 &&
// RUN: grep @llvm.atomic.load.and.i32 %t1 | count 3 &&
// RUN: grep @llvm.atomic.load.or.i32 %t1 | count 3 &&
// RUN: grep @llvm.atomic.load.xor.i32 %t1 | count 3


int atomic(void)
{
  // nonsenical test for sync functions
  int old;
  int val = 1;
  unsigned int uval = 1;
  int cmp = 0;

  old = __sync_fetch_and_add(&val, 1);
  old = __sync_fetch_and_sub(&val, 2);
  old = __sync_fetch_and_min(&val, 3);
  old = __sync_fetch_and_max(&val, 4);
  old = __sync_fetch_and_umin(&uval, 5u);
  old = __sync_fetch_and_umax(&uval, 6u);
  old = __sync_lock_test_and_set(&val, 7);
  old = __sync_val_compare_and_swap(&val, 4, 1976);
  old = __sync_bool_compare_and_swap(&val, 4, 1976);
  old = __sync_fetch_and_and(&val, 0x9);
  old = __sync_fetch_and_or(&val, 0xa);
  old = __sync_fetch_and_xor(&val, 0xb);

  old = __sync_add_and_fetch(&val, 1);
  old = __sync_sub_and_fetch(&val, 2);
  old = __sync_and_and_fetch(&val, 3);
  old = __sync_or_and_fetch(&val, 4);
  old = __sync_xor_and_fetch(&val, 5);

  return old;
}

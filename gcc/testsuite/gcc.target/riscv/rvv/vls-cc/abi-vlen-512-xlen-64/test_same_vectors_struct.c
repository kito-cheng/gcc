/* { dg-do compile } */
/* { dg-options "-march=rv64gcv_zvl512b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 512

#include "../common/test_same_vectors_struct.h"
// Function under test:
//	struct_two_same_vectors_t test_same_vectors_struct(struct_two_same_vectors_t s)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg:V4SI \d+\)[[:space:]]+\(reg:V4SI \d+ v8 \[ s \]\)\)} "expand" } } */
// Check vector argument 2 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg:V4SI \d+\)[[:space:]]+\(reg:V4SI \d+ v9 \[ s\+16 \]\)\)} "expand" } } */

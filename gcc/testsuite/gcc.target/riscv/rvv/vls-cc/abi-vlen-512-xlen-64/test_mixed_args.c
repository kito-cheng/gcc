/* { dg-do compile } */
/* { dg-options "-march=rv64gcv_zvl512b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 512

#include "../common/test_mixed_args.h"
// Function under test:
//	int test_mixed_args(int scalar1, int32x4_t vec1, float scalar2, int32x4_t vec2)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V4SI \d+ \[ vec1 \]\)[[:space:]]+\(reg:V4SI \d+ v8 \[ vec1 \]\)\)} "expand" } } */
// Check vector argument 2 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V4SI \d+ \[ vec2 \]\)[[:space:]]+\(reg:V4SI \d+ v9 \[ vec2 \]\)\)} "expand" } } */

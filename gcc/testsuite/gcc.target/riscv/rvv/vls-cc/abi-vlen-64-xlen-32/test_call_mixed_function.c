/* { dg-do compile } */
/* { dg-options "-march=rv32gc_zve64d_zvl64b -mabi=ilp32d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 64

#include "../common/test_call_mixed_function.h"
// Function under test:
//	int helper_mixed_function(int i, int32x4_t v, float f)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V4SI \d+ \[ v \]\)[[:space:]]+\(reg:V4SI \d+ v8 \[ v \]\)\)} "expand" } } */

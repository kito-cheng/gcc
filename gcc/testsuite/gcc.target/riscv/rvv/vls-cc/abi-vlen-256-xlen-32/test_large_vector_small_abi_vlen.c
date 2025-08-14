/* { dg-do compile } */
/* { dg-options "-march=rv32gcv_zvl256b -mabi=ilp32d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 256

#include "../common/test_large_vector_small_abi_vlen.h"
// Function under test:
//	int32x16_t test_large_vector_small_abi_vlen(int32x16_t vec1, int32x16_t vec2)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V16SI \d+ \[ vec1 \]\)[[:space:]]+\(reg:V16SI \d+ v8 \[ vec1 \]\)\)} "expand" } } */
// Check argument 2 register assignment
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V16SI \d+ \[ vec2 \]\)[[:space:]]+\(reg:V16SI \d+ v10 \[ vec2 \]\)\)} "expand" } } */

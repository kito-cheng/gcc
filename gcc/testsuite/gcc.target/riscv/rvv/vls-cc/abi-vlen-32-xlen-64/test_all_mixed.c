/* { dg-do compile } */
/* { dg-options "-march=rv64gc_zve32f_zvl32b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 32

#include "../common/test_all_mixed.h"
// Function under test:
//	double test_all_mixed(int i, float f, int32x4_t vec1, double d, float32x4_t vec2, int j)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V4SI \d+ \[ vec1 \]\)[[:space:]]+\(reg:V4SI \d+ v8 \[ vec1 \]\)\)} "expand" } } */
// Check argument 2 register assignment
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V4SF \d+ \[ vec2 \]\)[[:space:]]+\(reg:V4SF \d+ v12 \[ vec2 \]\)\)} "expand" } } */

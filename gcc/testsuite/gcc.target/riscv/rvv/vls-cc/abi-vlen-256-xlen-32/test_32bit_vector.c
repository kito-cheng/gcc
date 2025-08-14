/* { dg-do compile } */
/* { dg-options "-march=rv32gcv_zvl256b -mabi=ilp32d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 256

#include "../common/test_32bit_vector.h"
// Function under test:
//	int16x2_t test_32bit_vector(int16x2_t vec1, int16x2_t vec2)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V2HI \d+ \[ vec1 \]\)[[:space:]]+\(reg:V2HI \d+ v8 \[ vec1 \]\)\)} "expand" } } */
// Check vector argument 2 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V2HI \d+ \[ vec2 \]\)[[:space:]]+\(reg:V2HI \d+ v9 \[ vec2 \]\)\)} "expand" } } */
// Check return value passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg/i:V2HI \d+ v8\)[[:space:]]+\(reg:V2HI \d+ \[ <retval>.*\]\)\)} "expand" } } */

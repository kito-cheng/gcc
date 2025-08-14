/* { dg-do compile } */
/* { dg-options "-march=rv32gc_zve64d_zvl64b -mabi=ilp32d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 64

#include "../common/test_vector_array_struct.h"
// Function under test:
//	struct_vector_array_t test_vector_array_struct(struct_vector_array_t s)
// Check argument 1 register assignment
/* { dg-final { scan-rtl-dump {\(set \(reg/v/f:SI \d+.*\).*\(reg:SI \d+ a1\)\)} "expand" } } */
// Check return value passed via pointer (result_ptr)
/* { dg-final { scan-rtl-dump {\(set \(reg/f:SI \d+ \[ \.result_ptr \]\).*\(reg:SI \d+ a0 \[ \.result_ptr \]\)\)} "expand" } } */
// Check return value passed via pointer (result_ptr)
/* { dg-final { scan-rtl-dump {\(set \(reg/i:SI \d+ a0\).*\(reg/f:SI \d+ \[ \.result_ptr \]\)\)} "expand" } } */

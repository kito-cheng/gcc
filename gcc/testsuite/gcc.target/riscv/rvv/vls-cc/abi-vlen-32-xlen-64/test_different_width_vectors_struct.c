/* { dg-do compile } */
/* { dg-options "-march=rv64gc_zve32f_zvl32b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 32

#include "../common/test_different_width_vectors_struct.h"
// Function under test:
//	different_width_vectors_struct_t test_different_width_vectors_struct(different_width_vectors_struct_t s)
// Check argument 1 register assignment
/* { dg-final { scan-rtl-dump {\(set \(reg/v/f:DI \d+.*\).*\(reg:DI \d+ a1\)\)} "expand" } } */
// Check return value passed via pointer (result_ptr)
/* { dg-final { scan-rtl-dump {\(set \(reg/f:DI \d+ \[ \.result_ptr \]\).*\(reg:DI \d+ a0 \[ \.result_ptr \]\)\)} "expand" } } */
// Check return value passed via pointer (result_ptr)
/* { dg-final { scan-rtl-dump {\(set \(reg/i:DI \d+ a0\).*\(reg/f:DI \d+ \[ \.result_ptr \]\)\)} "expand" } } */

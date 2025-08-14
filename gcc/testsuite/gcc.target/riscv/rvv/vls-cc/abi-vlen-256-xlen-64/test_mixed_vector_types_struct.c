/* { dg-do compile } */
/* { dg-options "-march=rv64gcv_zvl256b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 256

#include "../common/test_mixed_vector_types_struct.h"
// Function under test:
//	mixed_vector_types_struct_t test_mixed_vector_types_struct(mixed_vector_types_struct_t s)
// Check vector argument 1 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg:V4SI \d+\)[[:space:]]+\(reg:V4SI \d+ v8 \[ s \]\)\)} "expand" } } */
// Check vector argument 2 passed in vector register
/* { dg-final { scan-rtl-dump {\(set \(reg:V4SF \d+\)[[:space:]]+\(reg:V4SF \d+ v9 \[ s\+16 \]\)\)} "expand" } } */

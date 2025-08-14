/* { dg-do compile } */
/* { dg-options "-march=rv64gc_zve32f_zvl32b -mabi=lp64d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

#define ABI_VLEN 32

#include "../common/test_multiple_unions.h"
// Function under test:
//	union_vector_t test_multiple_unions(union_vector_t u1, union_vector_t u2, union_vector_t u3)
// Check argument 1 register assignment
/* { dg-final { scan-rtl-dump {\(set \(subreg:DI \(reg/v:TI \d+ \[[^\]]+\]\) 0\).*\(reg:DI \d+ a0 \[[^\]]+\]\)\)} "expand" } } */
// Check argument 2 register assignment
/* { dg-final { scan-rtl-dump {\(set \(subreg:DI \(reg/v:TI \d+ \[[^\]]+\]\) [0-9]+\).*\(reg:DI \d+ a1 \[[^\]]+\]\)\)} "expand" } } */
// Check return value passed via integer registers using subreg
/* { dg-final { scan-rtl-dump {\(set \(reg:DI \d+ a0\).*\(subreg:DI \(reg:TI \d+ \[.*<retval>.*\]\) 0\)\)} "expand" } } */
// Check return value passed via integer registers using subreg
/* { dg-final { scan-rtl-dump {\(set \(reg:DI \d+ a1 \[[^\]]*\]\).*\(subreg:DI \(reg:TI \d+ \[.*<retval>.*\]\) \d+\)\)} "expand" } } */

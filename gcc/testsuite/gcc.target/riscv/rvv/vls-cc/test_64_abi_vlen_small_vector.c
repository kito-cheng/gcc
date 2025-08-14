/* { dg-do compile } */
/* { dg-options "-march=rv32gcv_zvl512b -mabi=ilp32d -fdump-rtl-expand -O2" } */
/* { dg-skip-if "" { *-*-* } { "-O0" } } */

// Test: Small vector with ABI_VLEN=64

#include "vls-cc-common.h"

int32x2_t __attribute__((riscv_vls_cc(64))) test_64_abi_vlen_small_vector(int32x2_t vec) {
    // 64-bit vector with ABI_VLEN=64 -> uses 1 register
    return vec;
}
/* { dg-final { scan-rtl-dump {\(set \(reg/v:V2SI \d+ \[ vec \]\)[[:space:]]+\(reg:V2SI \d+ v8 \[ vec \]\)\)} "expand" } } */
/* { dg-final { scan-rtl-dump {\(set \(reg/i:V2SI \d+ v8\)[[:space:]]+\(reg:V2SI \d+ \[ <retval>.*\]\)\)} "expand" } } */

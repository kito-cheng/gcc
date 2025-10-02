/* { dg-do compile } */
/* { dg-options "-march=rv64gcv -mabi=lp64d -Ofast -ftree-vectorize -mrvv-vector-bits=scalable" } */

#include <stdint-gcc.h>
#include <stddef.h>

int32_t sum(uint8_t *x, uint8_t *y, size_t n){
    int32_t s = 0;
    for (int i=0;i<n;++i)
      s += x[i] * y[i];
    return s;
}

/* { dg-final { scan-assembler {vqdotu\.vv} } } */

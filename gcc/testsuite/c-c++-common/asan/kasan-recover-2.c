/* { dg-do compile } */
/* { dg-options "-fno-sanitize=address -fsanitize=kernel-address" } */

void
foo (int *p)
{
  *p = 0;
}

/* { dg-final { scan-assembler "__asan_store4_noabort" } } */


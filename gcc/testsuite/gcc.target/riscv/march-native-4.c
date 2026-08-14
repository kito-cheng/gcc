/* An explicit -march must survive -mtune=native.  Rewriting the native
   options is a driver self spec, so getting the ordering wrong would let
   the detected architecture overwrite what the user asked for.  */

/* { dg-do compile } */
/* { dg-require-effective-target riscv_native_cpu_detect } */
/* { dg-require-effective-target rv64 } */
/* { dg-options "-mtune=native -march=rv64gc -mabi=lp64d" } */

#ifdef __riscv_v
#error "-mtune=native overrode an explicit -march"
#endif

int
main (void)
{
  return 0;
}

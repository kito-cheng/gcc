/* { dg-do compile } */
/* { dg-options "-march=rv64gc_zclsd -mabi=ilp32d" } */
int foo()
{
}
/* { missing " for " dg-error 6 ".'error: '-march=rv64gc_zclsd': zclsd extension supports in rv32 only " } */

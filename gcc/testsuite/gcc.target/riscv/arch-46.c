/* { dg-do compile } */
/* { dg-options "-march=rv64gc_zilsd -mabi=ilp32d" } */
int foo()
{
}
/* { missing " for " dg-error 6 ".'error: '-march=rv64gc_zilsd': zilsd extension supports in rv32 only " } */

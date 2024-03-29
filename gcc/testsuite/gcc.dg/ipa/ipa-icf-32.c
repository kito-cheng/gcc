/* { dg-do run } */
/* { dg-options "-O0 -fipa-icf -fdump-ipa-icf-details" } */

int
__attribute__((optimize("O0"), noinline, noclone))
foo(int a)
{
  return a * a;
}

__attribute__ ((noinline, noclone))
int bar(int b)
{
  return b * b;
}

int main()
{
  return foo (0) + bar (0);
}

/* { dg-final { scan-ipa-dump "optimization flags are different" "icf"  } } */
/* { dg-final { scan-ipa-dump "Equal symbols: 0" "icf"  } } */
/* { dg-final { cleanup-ipa-dump "icf" } } */

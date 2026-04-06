/* { dg-do compile } */
/* { dg-options "-O2" } */

__attribute__ ((noinline, noclone))
long
huge_diff (__huge unsigned char *p, __huge unsigned char *q)
{
  return q - p;
}

/* Huge subtraction still uses full-width linear arithmetic.  */
/* { dg-final { scan-assembler "(adcw|sbbw)" } } */
/* { dg-do compile } */
/* { dg-options "-O2" } */

__attribute__ ((noinline, noclone))
int
far_diff (__far unsigned char *p, __far unsigned char *q)
{
  return (int) (q - p);
}

/* Plain far subtraction must not propagate borrow across the segment word.  */
/* { dg-final { scan-assembler-not "adcw" } } */
/* { dg-final { scan-assembler-not "sbbw" } } */
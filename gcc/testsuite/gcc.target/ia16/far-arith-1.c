/* { dg-do compile } */
/* { dg-options "-O2" } */

__far char *
far_add (__far char *p, unsigned long n)
{
  return p + n;
}

__far char *
far_sub (__far char *p, unsigned long n)
{
  return p - n;
}

/* Plain far arithmetic must not propagate carry or borrow into the segment.  */
/* { dg-final { scan-assembler-not "adcw" } } */
/* { dg-final { scan-assembler-not "sbbw" } } */
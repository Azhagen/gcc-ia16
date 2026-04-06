/* { dg-do compile } */
/* { dg-options "-O2" } */

__huge char *
huge_add (__huge char *p, unsigned long n)
{
  return p + n;
}

__huge char *
huge_sub (__huge char *p, unsigned long n)
{
  return p - n;
}

/* Huge pointers still use full-width linear arithmetic.  Subtraction may
   canonicalize to addition of a negated offset, so accept either carry or
   borrow propagation in the high word.  */
/* { dg-final { scan-assembler-times "(adcw|sbbw)" 2 } } */
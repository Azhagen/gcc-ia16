/* { dg-do compile } */
/* { dg-options "-O2 -mmedium" } */

extern void ext_proc (void) __attribute__ ((noinline, noclone));
extern int ext_add1 (int) __attribute__ ((noinline, noclone));

void
caller_void (void)
{
  ext_proc ();
}

int
caller_value (int x)
{
  return ext_add1 (x);
}

/* Medium-model direct calls must use far call/return instructions.  */
/* { dg-final { scan-assembler-times "\\blcall\\b" 2 } } */
/* { dg-final { scan-assembler-times "\\blret\\b" 2 } } */
/* { dg-final { scan-assembler-not "\tcall\t" } } */
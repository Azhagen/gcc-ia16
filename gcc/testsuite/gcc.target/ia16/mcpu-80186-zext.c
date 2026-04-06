/* { dg-do assemble } */
/* { dg-options "-O2 -mcpu=80186" } */

unsigned int
zext_80186 (unsigned char x)
{
  return x;
}

/* The backend should select the assembler's accepted 80186 spelling.  */
/* { dg-final { scan-assembler "\\.arch i186" } } */

/* 80186 does not support movzbw, and zero-extension should not clear the
   high byte with xor behind the RTL's back.  */
/* { dg-final { scan-assembler-not "movzbw" } } */
/* { dg-final { scan-assembler-not "xorb\\t%[abcd]h, %[abcd]h" } } */

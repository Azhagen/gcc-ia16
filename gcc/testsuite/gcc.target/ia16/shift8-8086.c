/* { dg-do assemble } */
/* { dg-options "-O2 -mcpu=8086" } */

unsigned short
lshr8_8086 (unsigned short x)
{
  return x >> 8;
}

unsigned short
ashl8_8086 (unsigned short x)
{
  return x << 8;
}

short
ashr8_8086 (short x)
{
  return x >> 8;
}

/* Fixed count-8 shifts should not bounce through CL on 8086.  */
/* { dg-final { scan-assembler-not "movb\\t\\$8, %cl" } } */
/* { dg-final { scan-assembler-not "shlw\\t\\$8" } } */
/* { dg-final { scan-assembler-not "shrw\\t\\$8" } } */
/* { dg-final { scan-assembler-not "sarw\\t\\$8" } } */

/* AX byte shuffles should handle the logical count-8 cases.  */
/* { dg-final { scan-assembler "movb\\t%ah, %al" } } */
/* { dg-final { scan-assembler "movb\\t%al, %ah" } } */

/* Signed right shift by 8 in AX can use CBW after moving AH into AL.  */
/* { dg-final { scan-assembler "cbw" } } */
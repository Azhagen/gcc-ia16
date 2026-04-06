/* { dg-do compile } */
/* { dg-options "-Os" } */

extern unsigned char buffer[64];

unsigned char
read_huge (void)
{
  __huge unsigned char *p = (__huge unsigned char *) &buffer[14];

  return p[4];
}
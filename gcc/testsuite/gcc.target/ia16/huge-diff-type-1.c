/* { dg-do compile } */

typedef int check_huge_ptrdiff_size
  [sizeof (((__huge unsigned char *) 0) - ((__huge unsigned char *) 0))
   == sizeof (long)
   ? 1 : -1];

long
huge_diff_long (__huge unsigned char *p, __huge unsigned char *q)
{
  return q - p;
}
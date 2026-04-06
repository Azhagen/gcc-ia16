/* { dg-do compile } */

using huge_diff_t
  = decltype (((__huge unsigned char *) 0) - ((__huge unsigned char *) 0));

static_assert (sizeof (huge_diff_t) == sizeof (long), "huge ptrdiff should be long");

long
huge_diff_long (__huge unsigned char *p, __huge unsigned char *q)
{
  return q - p;
}
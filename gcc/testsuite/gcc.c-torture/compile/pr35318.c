/* { dg-skip-if "" { pdp11-*-* } } */
/* { dg-skip-if "Not enough 64-bit registers" { ia16-*-* } } */
/* PR target/35318 */

void
foo ()
{
  double x = 4, y;
  __asm__ volatile ("" : "=r,r" (x), "=r,r" (y) : "%0,0" (x), "m,r" (8));
}

/* PR c/123365 */
/* { dg-skip-if "No 64-bit registers" { ia16-*-* } } */

void
foo ()
{
  __asm__ volatile ("" : "+r" ((long long[]) { 0 }));
}

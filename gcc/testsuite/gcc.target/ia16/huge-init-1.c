/* { dg-do compile } */

extern unsigned char anchor[32];

__huge unsigned char *hp = (__huge unsigned char *) &anchor[14];

/* Huge pointers use a linear 32-bit initializer, not segmented @OFF/@SEG
   emission.  */
/* { dg-final { scan-assembler "\\.long[ \t]+_anchor\\+14" } } */

/* { dg-do compile } */

extern int ex;
int gx;
static int sx;

__far int *pgx = &gx;
__far int *pex = &ex;
__far int *psx = &sx;

/* { dg-final { scan-assembler-times "@OFF" 3 } } */
/* { dg-final { scan-assembler-times "@SEG" 3 } } */

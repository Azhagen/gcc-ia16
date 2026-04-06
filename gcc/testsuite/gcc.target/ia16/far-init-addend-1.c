/* { dg-do compile } */

extern int exa[8];
static int sa[8];

struct pair
{
  int a;
  int b;
};

static struct pair sp;

__far int *pex = &exa[3];
__far int *psa = &sa[3];
__far int *psb = &sp.b;

/* { dg-final { scan-assembler "_exa@OFF\\+6" } } */
/* { dg-final { scan-assembler "_sa@OFF\\+6" } } */
/* { dg-final { scan-assembler "_sp@OFF\\+2" } } */
/* { dg-final { scan-assembler-times "@SEG" 3 } } */
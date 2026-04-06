/* { dg-do compile } */

__huge unsigned char huge_buffer[70000];

unsigned char
read_last (void)
{
  return huge_buffer[69999L];
}

/* { dg-final { scan-assembler "\\.section[ \t]+\\.bss\\.huge" } } */
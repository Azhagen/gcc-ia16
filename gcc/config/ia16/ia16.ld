/* Linker script for ia16-elf (16-bit x86, bare metal / .COM style).
   Copyright (C) 2026 Free Software Foundation, Inc.

   Code starts at 0x0100 (DOS .COM convention) or 0x0000 for bare metal.
   All sections fit within a single 64KB segment.  */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
  . = 0x0000;

  .text : {
    *(.text .text.*)
    *(.rodata .rodata.*)
    _etext = .;
  }

  .data : {
    _data = .;
    *(.data .data.*)
    _edata = .;
  }

  .bss : {
    _bss = .;
    *(.bss .bss.*)
    *(COMMON)
    _ebss = .;
  }

  _end = .;

  /* Stack grows down from top of 64KB segment.  */
  _stack_top = 0xFFFE;

  /DISCARD/ : {
    *(.comment)
    *(.note.*)
    *(.eh_frame)
    *(.eh_frame_hdr)
  }
}

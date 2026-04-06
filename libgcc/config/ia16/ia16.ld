/* Linker script for ia16-elf (16-bit x86, bare metal / .COM style).
   Copyright (C) 2026 Free Software Foundation, Inc.

  Code starts at 0x0100 (DOS .COM convention) or 0x0000 for bare metal.
  Near sections stay within a single 64KB segment; huge data sections may
  extend beyond that and are placed separately.  */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
  . = 0x0000;

  . = ALIGN(16);
  .text : SUBALIGN(16) {
    *(.text .text.*)
    *(.rodata .rodata.[!h]*)
    _etext = .;
  }

  . = ALIGN(16);
  .rodata.huge : SUBALIGN(16) {
    *(.rodata.huge .rodata.huge.*)
    *(.gnu.linkonce.r.huge.*)
  }

  . = ALIGN(16);
  .data.huge : SUBALIGN(16) {
    *(.data.huge .data.huge.*)
    *(.data.rel.huge .data.rel.huge.*)
    *(.data.rel.local.huge .data.rel.local.huge.*)
    *(.data.rel.ro.huge .data.rel.ro.huge.*)
    *(.data.rel.ro.local.huge .data.rel.ro.local.huge.*)
    *(.persistent.huge .persistent.huge.*)
    *(.gnu.linkonce.d.huge.*)
    *(.gnu.linkonce.d.rel.huge.*)
    *(.gnu.linkonce.d.rel.local.huge.*)
    *(.gnu.linkonce.d.rel.ro.huge.*)
    *(.gnu.linkonce.d.rel.ro.local.huge.*)
    *(.gnu.linkonce.p.huge.*)
  }

  . = ALIGN(16);
  .data : SUBALIGN(16) {
    _data = .;
    *(.data .data.*)
    _edata = .;
  }

  . = ALIGN(16);
  .bss : SUBALIGN(16) {
    _bss = .;
    *(.bss .bss.[!h]*)
    *(COMMON)
    _ebss = .;
  }

  . = ALIGN(16);
  .bss.huge : SUBALIGN(16) {
    _bss_huge = .;
    _bss_huge_seg = _bss_huge >> 4;
    _bss_huge_off = _bss_huge & 15;
    *(.bss.huge .bss.huge.*)
    *(.gnu.linkonce.b.huge.*)
    _ebss_huge = .;
    _ebss_huge_seg = _ebss_huge >> 4;
    _ebss_huge_off = _ebss_huge & 15;
  }

  . = ALIGN(16);
  .noinit.huge : SUBALIGN(16) {
    *(.noinit.huge .noinit.huge.*)
    *(.gnu.linkonce.n.huge.*)
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

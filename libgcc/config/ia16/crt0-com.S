/* Minimal C runtime startup for DOS .COM executables.
   Copyright (C) 2026 Free Software Foundation, Inc.

   DOS loads .COM at CS:0100h with CS=DS=ES=SS.
   SP is set to 0xFFFE by DOS.  */

	.code16
	.arch i8086

	.section .text.startup, "ax"
	.global _start
_start:
	/* Clear regular BSS.  */
	movw	$_bss, %di
	movw	$_ebss, %cx
	subw	%di, %cx
	movw	%cx, %bx
	shrw	$1, %cx
	xorw	%ax, %ax
	cld
	rep	stosw
	testw	$1, %bx
	jz	.Lclear_huge_bss
	stosb

.Lclear_huge_bss:
	/* Clear huge BSS without touching .noinit.huge.  */
	movw	$_bss_huge_seg, %si
	movw	%si, %es
	movw	$_bss_huge_off, %di
	movw	$_ebss_huge_seg, %bx
	movw	$_ebss_huge_off, %dx
	cmpw	%bx, %si
	jne	.Lclear_huge_loop
	cmpw	%dx, %di
	je	.Lclear_bss_done

.Lclear_huge_loop:
	movw	$16, %cx
	subw	%di, %cx
	cmpw	%bx, %si
	jne	.Lclear_huge_chunk
	movw	%dx, %cx
	subw	%di, %cx

.Lclear_huge_chunk:
	rep	stosb
	cmpw	%bx, %si
	jne	.Ladvance_huge
	cmpw	%dx, %di
	je	.Lclear_bss_done

.Ladvance_huge:
	incw	%si
	movw	%si, %es
	xorw	%di, %di
	jmp	.Lclear_huge_loop

.Lclear_bss_done:
	movw	%ds, %ax
	movw	%ax, %es

	/* Call main().  */
	call	_main

	/* Exit to DOS: INT 21h, AH=4Ch, AL=return code.  */
	movb	%al, %al	/* Return value already in AL.  */
	movb	$0x4c, %ah
	int	$0x21

	.size	_start, .-_start

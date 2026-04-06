/* Shift helpers for the GCC runtime library on IA-16.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

	.code16
	.arch i8086

	.section .text.__ashlsi3, "ax"
	.global ___ashlsi3
	.type ___ashlsi3, @function
___ashlsi3:
	pushw	%bp
	movw	%sp, %bp
	pushw	%bx
	movw	4(%bp), %ax
	movw	6(%bp), %dx
	movw	8(%bp), %cx
	orw		%cx, %cx
	jz	.Lashlsi3_done
	cmpw	$32, %cx
	jb	.Lashlsi3_small
	xorw	%ax, %ax
	xorw	%dx, %dx
	jmp	.Lashlsi3_done
.Lashlsi3_small:
	cmpw	$16, %cx
	jb	.Lashlsi3_lt16
	subw	$16, %cx
	movw	%ax, %dx
	xorw	%ax, %ax
	jz	.Lashlsi3_done
	shlw	%cl, %dx
	jmp	.Lashlsi3_done
.Lashlsi3_lt16:
	movw	%ax, %bx
	movb	%cl, %ch
	shlw	%cl, %ax
	movb	$16, %cl
	subb	%ch, %cl
	shrw	%cl, %bx
	movb	%ch, %cl
	shlw	%cl, %dx
	orw		%bx, %dx
.Lashlsi3_done:
	popw	%bx
	popw	%bp
	ret
	.size ___ashlsi3, .-___ashlsi3

	.section .text.__ashrsi3, "ax"
	.global ___ashrsi3
	.type ___ashrsi3, @function
___ashrsi3:
	pushw	%bp
	movw	%sp, %bp
	pushw	%bx
	movw	4(%bp), %ax
	movw	6(%bp), %dx
	movw	8(%bp), %cx
	orw		%cx, %cx
	jz	.Lashrsi3_done
	cmpw	$32, %cx
	jb	.Lashrsi3_small
	movw	%dx, %ax
	cwd
	jmp	.Lashrsi3_done
.Lashrsi3_small:
	cmpw	$16, %cx
	jb	.Lashrsi3_lt16
	subw	$16, %cx
	movw	%dx, %ax
	cwd
	jz	.Lashrsi3_done
	sarw	%cl, %ax
	jmp	.Lashrsi3_done
.Lashrsi3_lt16:
	movw	%dx, %bx
	movb	%cl, %ch
	sarw	%cl, %dx
	movb	$16, %cl
	subb	%ch, %cl
	shlw	%cl, %bx
	movb	%ch, %cl
	shrw	%cl, %ax
	orw		%bx, %ax
.Lashrsi3_done:
	popw	%bx
	popw	%bp
	ret
	.size ___ashrsi3, .-___ashrsi3

	.section .text.__lshrsi3, "ax"
	.global ___lshrsi3
	.type ___lshrsi3, @function
___lshrsi3:
	pushw	%bp
	movw	%sp, %bp
	pushw	%bx
	movw	4(%bp), %ax
	movw	6(%bp), %dx
	movw	8(%bp), %cx
	orw		%cx, %cx
	jz	.Llshrsi3_done
	cmpw	$32, %cx
	jb	.Llshrsi3_small
	xorw	%ax, %ax
	xorw	%dx, %dx
	jmp	.Llshrsi3_done
.Llshrsi3_small:
	cmpw	$16, %cx
	jb	.Llshrsi3_lt16
	subw	$16, %cx
	movw	%dx, %ax
	xorw	%dx, %dx
	jz	.Llshrsi3_done
	shrw	%cl, %ax
	jmp	.Llshrsi3_done
.Llshrsi3_lt16:
	movw	%dx, %bx
	movb	%cl, %ch
	shrw	%cl, %dx
	movb	$16, %cl
	subb	%ch, %cl
	shlw	%cl, %bx
	movb	%ch, %cl
	shrw	%cl, %ax
	orw		%bx, %ax
.Llshrsi3_done:
	popw	%bx
	popw	%bp
	ret
	.size ___lshrsi3, .-___lshrsi3
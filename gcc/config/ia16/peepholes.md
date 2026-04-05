;;  Peephole optimizations for the Intel 16-bit x86 (ia16) backend.
;;  Copyright (C) 2026 Free Software Foundation, Inc.
;;
;;  This file is part of GCC.
;;
;;  GCC is free software; you can redistribute it and/or modify
;;  it under the terms of the GNU General Public License as published by
;;  the Free Software Foundation; either version 3, or (at your option)
;;  any later version.
;;
;;  GCC is distributed in the hope that it will be useful,
;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;  GNU General Public License for more details.
;;
;;  You should have received a copy of the GNU General Public License
;;  along with GCC; see the file COPYING3.  If not see
;;  <http://www.gnu.org/licenses/>.

;; Collapse "movb $8, %cl; shift %cl, reg" back to a constant shift so the
;; 8086 backend can use byte moves instead of routing everything through CL.
(define_peephole2
  [(set (match_operand:QI 2 "register_operand")
	(const_int 8))
   (parallel [(set (match_operand:HI 0 "register_operand")
		   (ashift:HI (match_operand:HI 1 "register_operand")
			      (match_dup 2)))
	      (clobber (reg:CC CC_REG))])]
  "TARGET_8086
   && REGNO (operands[2]) == CX_REG
   && REGNO (operands[0]) == REGNO (operands[1])
   && (REGNO (operands[0]) == AX_REG
       || REGNO (operands[0]) == BX_REG
       || REGNO (operands[0]) == CX_REG
       || REGNO (operands[0]) == DX_REG)
   && peep2_reg_dead_p (2, operands[2])"
  [(parallel [(set (match_dup 0)
		   (ashift:HI (match_dup 1) (const_int 8)))
	      (clobber (reg:CC CC_REG))])])

(define_peephole2
  [(set (match_operand:QI 2 "register_operand")
	(const_int 8))
   (parallel [(set (match_operand:HI 0 "register_operand")
		   (ashiftrt:HI (match_operand:HI 1 "register_operand")
				(match_dup 2)))
	      (clobber (reg:CC CC_REG))])]
  "TARGET_8086
   && REGNO (operands[2]) == CX_REG
   && REGNO (operands[0]) == REGNO (operands[1])
   && REGNO (operands[0]) == AX_REG
   && peep2_reg_dead_p (2, operands[2])"
  [(parallel [(set (match_dup 0)
		   (ashiftrt:HI (match_dup 1) (const_int 8)))
	      (clobber (reg:CC CC_REG))])])

(define_peephole2
  [(set (match_operand:QI 2 "register_operand")
	(const_int 8))
   (parallel [(set (match_operand:HI 0 "register_operand")
		   (lshiftrt:HI (match_operand:HI 1 "register_operand")
				(match_dup 2)))
	      (clobber (reg:CC CC_REG))])]
  "TARGET_8086
   && REGNO (operands[2]) == CX_REG
   && REGNO (operands[0]) == REGNO (operands[1])
   && (REGNO (operands[0]) == AX_REG
       || REGNO (operands[0]) == BX_REG
       || REGNO (operands[0]) == CX_REG
       || REGNO (operands[0]) == DX_REG)
   && peep2_reg_dead_p (2, operands[2])"
  [(parallel [(set (match_dup 0)
		   (lshiftrt:HI (match_dup 1) (const_int 8)))
	      (clobber (reg:CC CC_REG))])])

;; Convert "movw SRC, DST; andw $255, DST" into a byte zero-extension that
;; preserves flags.  Split the register and memory cases so peephole2 never
;; needs to invent a new pseudo for the QImode source.
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "register_operand"))
   (parallel [(set (match_dup 0)
		   (and:HI (match_dup 0) (const_int 255)))
	      (clobber (reg:CC CC_REG))])]
  "REG_P (operands[0])
   && REG_P (operands[1])
   && REGNO (operands[0]) < FIRST_PSEUDO_REGISTER
   && REGNO (operands[1]) < FIRST_PSEUDO_REGISTER
   && (REGNO (operands[0]) == AX_REG
       || REGNO (operands[0]) == BX_REG
       || REGNO (operands[0]) == CX_REG
       || REGNO (operands[0]) == DX_REG)
   && (REGNO (operands[1]) == AX_REG
       || REGNO (operands[1]) == BX_REG
       || REGNO (operands[1]) == CX_REG
       || REGNO (operands[1]) == DX_REG)
   && lowpart_subreg (QImode, operands[1], HImode) != NULL_RTX"
  [(parallel [(set (match_dup 0)
		   (zero_extend:HI (match_dup 2)))
	      (clobber (reg:CC CC_REG))])]
{
  operands[2] = lowpart_subreg (QImode, operands[1], HImode);
  gcc_assert (operands[2] != NULL_RTX);
})

(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "memory_operand"))
   (parallel [(set (match_dup 0)
		   (and:HI (match_dup 0) (const_int 255)))
	      (clobber (reg:CC CC_REG))])]
  "REG_P (operands[0])
   && REGNO (operands[0]) < FIRST_PSEUDO_REGISTER
   && (REGNO (operands[0]) == AX_REG
       || REGNO (operands[0]) == BX_REG
       || REGNO (operands[0]) == CX_REG
       || REGNO (operands[0]) == DX_REG)"
  [(parallel [(set (match_dup 0)
		   (zero_extend:HI (match_dup 2)))
	      (clobber (reg:CC CC_REG))])]
{
  operands[2] = adjust_address (operands[1], QImode, 0);
})

;; Remove redundant load after store to the same location.
;; mov %reg, MEM ; mov MEM, %reg  ->  mov %reg, MEM
(define_peephole2
  [(set (match_operand:HI 0 "memory_operand")
	(match_operand:HI 1 "register_operand"))
   (set (match_operand:HI 2 "register_operand")
	(match_dup 0))]
  "REGNO (operands[1]) == REGNO (operands[2])
   && peep2_reg_dead_p (2, operands[2])"
  [(set (match_dup 0) (match_dup 1))])

;; Same for QI mode.
(define_peephole2
  [(set (match_operand:QI 0 "memory_operand")
	(match_operand:QI 1 "register_operand"))
   (set (match_operand:QI 2 "register_operand")
	(match_dup 0))]
  "REGNO (operands[1]) == REGNO (operands[2])
   && peep2_reg_dead_p (2, operands[2])"
  [(set (match_dup 0) (match_dup 1))])

;; Eliminate store followed by load of same value when reg is still live.
;; mov %reg, MEM ; mov MEM, %same_reg  ->  mov %reg, MEM
(define_peephole2
  [(set (match_operand:HI 0 "memory_operand")
	(match_operand:HI 1 "register_operand"))
   (set (match_operand:HI 2 "register_operand")
	(match_dup 0))]
  "REGNO (operands[1]) == REGNO (operands[2])"
  [(set (match_dup 0) (match_dup 1))])

;; XOR reg, reg is shorter than MOV $0, reg for zeroing.
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(const_int 0))]
  ""
  [(parallel [(set (match_dup 0)
		   (xor:HI (match_dup 0) (match_dup 0)))
	      (clobber (reg:CC CC_REG))])])

;; Replace movw %ax, %dx; sarw $N, %dx (where N fills with sign bit)
;; with CWD when result is in DX and source is AX.
;; This pattern arises from extendhisi2 (sign-extend HI to SI).
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "register_operand"))
   (parallel [(set (match_dup 0)
		   (ashiftrt:HI (match_dup 0)
				(match_operand:QI 2 "const_int_operand")))
	      (clobber (reg:CC CC_REG))])]
  "REGNO (operands[0]) == DX_REG
   && REGNO (operands[1]) == AX_REG
   && INTVAL (operands[2]) == 15"
  [(set (reg:HI DX_REG)
	(ashiftrt:HI (reg:HI AX_REG) (const_int 15)))]
  "")

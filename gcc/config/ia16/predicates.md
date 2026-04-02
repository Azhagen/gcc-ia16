;;  Predicate definitions for the ia16 backend.
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

;; Return true if OP is a valid general operand for ia16.
(define_predicate "ia16_general_operand"
  (match_operand 0 "general_operand"))

;; Return true if OP is a nonimmediate operand.
(define_predicate "ia16_nonimmediate_operand"
  (match_operand 0 "nonimmediate_operand"))

;; Return true if OP is an immediate operand fitting in 8 bits.
(define_predicate "const_byte_operand"
  (and (match_code "const_int")
       (match_test "IN_RANGE (INTVAL (op), 0, 255)")))

;; Return true if OP is a comparison operator we support.
(define_predicate "ia16_comparison_operator"
  (match_code "eq,ne,lt,ltu,gt,gtu,le,leu,ge,geu"))

;; Return true if OP is a register or memory operand suitable as a
;; destination.
(define_predicate "ia16_dst_operand"
  (match_code "reg,subreg,mem"))

;; Return true if OP is a valid shift count.
(define_predicate "ia16_shift_operand"
  (ior (and (match_code "const_int")
	    (match_test "IN_RANGE (INTVAL (op), 1, 16)"))
       (and (match_code "reg")
	    (match_test "REGNO (op) == CL_REG || REGNO (op) == CX_REG"))))

;; Return true for constants that can appear as an operand in a 16-bit
;; instruction (used for immediate operand optimization).
(define_predicate "ia16_const_int_operand"
  (match_code "const_int"))

;; Return true for a symbol reference.
(define_predicate "ia16_symbolic_operand"
  (match_code "symbol_ref,label_ref,const"))

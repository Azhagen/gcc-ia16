;;  Constraint definitions for the ia16 backend.
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

;; Register constraints.

(define_register_constraint "a" "AREG"
  "The AX register.")

(define_register_constraint "b" "INDEX_REGS"
  "BX, SI, or DI register (valid for base addressing).")

(define_register_constraint "c" "CREG"
  "The CX register (used for shift counts and loop).")

(define_register_constraint "d" "DREG"
  "The DX register.")

(define_register_constraint "S" "SIREG"
  "The SI register.")

(define_register_constraint "D" "DIREG"
  "The DI register.")

(define_register_constraint "q" "QI_REGS"
  "Any register accessible as an 8-bit value (AX, BX, CX, DX).")

(define_register_constraint "e" "SEG_REGS"
  "A segment register (ES).")

;; Immediate-value constraints.

(define_constraint "I"
  "Integer constant 0-255 (unsigned byte)."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 0, 255)")))

(define_constraint "J"
  "Integer constant 0-65535 (unsigned word)."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 0, 65535)")))

(define_constraint "K"
  "Integer constant -128 to 127 (signed byte)."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, -128, 127)")))

(define_constraint "L"
  "Integer constant -32768 to 32767 (signed word)."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, -32768, 32767)")))

(define_constraint "M"
  "Integer constant 1-16 (for shift counts)."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 1, 16)")))

(define_constraint "N"
  "Integer constant 0 (zero)."
  (and (match_code "const_int")
       (match_test "ival == 0")))

(define_constraint "O"
  "Integer constant 1."
  (and (match_code "const_int")
       (match_test "ival == 1")))

(define_constraint "P"
  "Integer constant -1 (all ones)."
  (and (match_code "const_int")
       (match_test "ival == -1")))

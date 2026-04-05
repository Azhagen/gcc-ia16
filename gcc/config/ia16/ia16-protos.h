/* Exported function prototypes for the ia16 backend.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_IA16_PROTOS_H
#define GCC_IA16_PROTOS_H

extern void ia16_expand_prologue (void);
extern void ia16_expand_epilogue (bool);
extern int ia16_initial_elimination_offset (int, int);
extern void ia16_print_operand (FILE *, rtx, int);
extern void ia16_print_operand_address (FILE *, machine_mode, rtx);
extern const char *ia16_output_addsub_insn (machine_mode, bool, rtx *);
extern const char *ia16_output_move_insn (rtx *, machine_mode);
extern const char *ia16_output_shift_insn (const char *, rtx *);
extern bool ia16_expand_const_shift_si (enum rtx_code, rtx *);
extern enum reg_class ia16_regno_reg_class (int);
extern bool ia16_hard_regno_mode_ok (unsigned int, machine_mode);
extern unsigned int ia16_hard_regno_nregs (unsigned int, machine_mode);
extern int ia16_register_move_cost (machine_mode, reg_class_t, reg_class_t);
extern int ia16_memory_move_cost (machine_mode, reg_class_t, bool);
extern bool ia16_can_eliminate (int, int);
extern bool ia16_legitimize_reload_address (rtx *, machine_mode, int, int,
					    int);
extern rtx ia16_complex_part (rtx, machine_mode, unsigned int);
extern rtx ia16_subword (rtx, machine_mode, unsigned int);
extern void ia16_emit_huge_address_parts (rtx, rtx, rtx);
extern rtx ia16_force_huge_address (rtx);
extern void ia16_emit_multiword_move (rtx *, machine_mode);
extern void ia16_emit_multiword_push (rtx, machine_mode);
extern rtx ia16_split_si_half (rtx, int);
extern void ia16_split_movsi (rtx *, rtx *);
extern bool ia16_can_use_return_insn_p (void);
extern poly_int64 ia16_push_rounding (poly_int64);

#endif /* GCC_IA16_PROTOS_H */

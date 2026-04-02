/* Subroutines used for code generation on Intel 16-bit x86 (ia16).
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

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "df.h"
#include "memmodel.h"
#include "tm_p.h"
#include "regs.h"
#include "emit-rtl.h"
#include "varasm.h"
#include "diagnostic-core.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "calls.h"
#include "output.h"
#include "explow.h"
#include "expr.h"
#include "langhooks.h"
#include "builtins.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "recog.h"
#include "reload.h"

/* This file should be included last.  */
#include "target-def.h"

#include "gt-ia16.h"

/* --------------------------------------------------------------------------
   Frame / Machine Function
   -------------------------------------------------------------------------- */

struct GTY(()) machine_function
{
  int computed;
  int callee_saved_reg_size;
  int local_vars_size;
  int frame_size;
  bool uses_frame_pointer;
};

static struct machine_function *
ia16_init_machine_status (void)
{
  return ggc_cleared_alloc<machine_function> ();
}

static void
ia16_compute_frame_info (void)
{
  struct machine_function *m = cfun->machine;

  if (m->computed)
    return;

  m->callee_saved_reg_size = 0;
  m->local_vars_size = get_frame_size ();
  m->uses_frame_pointer = frame_pointer_needed;

  /* Count callee-saved registers that need saving.  */
  for (int i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      if (df_regs_ever_live_p (i)
	  && !call_used_or_fixed_reg_p (i)
	  && !fixed_regs[i])
	m->callee_saved_reg_size += 2;
    }

  m->frame_size = m->callee_saved_reg_size + m->local_vars_size;
  m->computed = 1;
}

/* --------------------------------------------------------------------------
   Option Override
   -------------------------------------------------------------------------- */

static void
ia16_option_override (void)
{
  init_machine_status = ia16_init_machine_status;

  /* ENTER/LEAVE requires 80186 or later.  */
  if (TARGET_ENTER_LEAVE && TARGET_8086)
    {
      warning (0, "%<-menter-leave%> requires 80186 or later; ignoring");
      target_flags &= ~MASK_ENTER_LEAVE;
    }
}

#undef  TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE ia16_option_override

/* --------------------------------------------------------------------------
   Register Info
   -------------------------------------------------------------------------- */

enum reg_class
ia16_regno_reg_class (int regno)
{
  switch (regno)
    {
    case AX_REG: return AREG;
    case DX_REG: return DREG;
    case CX_REG: return CREG;
    case BX_REG: return INDEX_REGS;
    case SI_REG: return SIREG;
    case DI_REG: return DIREG;
    case BP_REG: return BASE_REGS;
    case SP_REG: return GENERAL_REGS;
    case ES_REG: return SEG_REGS;
    case CC_REG: return NO_REGS;
    case AP_REG: return GENERAL_REGS;
    case AL_REG: return ABREG;
    case DL_REG: return ABREG;
    case CL_REG: return ABREG;
    case BL_REG: return ABREG;
    default:     return NO_REGS;
    }
}

/* Implement TARGET_HARD_REGNO_NREGS.  */
static unsigned int
ia16_hard_regno_nregs_hook (unsigned int regno, machine_mode mode)
{
  return ia16_hard_regno_nregs (regno, mode);
}

unsigned int
ia16_hard_regno_nregs (unsigned int regno, machine_mode mode)
{
  /* Byte registers hold one QImode value.  */
  if (regno >= AL_REG && regno <= BL_REG)
    return 1;

  /* 16-bit registers: 1 reg per 16 bits.  */
  unsigned int size = GET_MODE_SIZE (mode);
  return (size + 1) / 2;
}

#undef  TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS ia16_hard_regno_nregs_hook

/* Implement TARGET_HARD_REGNO_MODE_OK.  */
static bool
ia16_hard_regno_mode_ok_hook (unsigned int regno, machine_mode mode)
{
  return ia16_hard_regno_mode_ok (regno, mode);
}

bool
ia16_hard_regno_mode_ok (unsigned int regno, machine_mode mode)
{
  /* Byte sub-registers can only hold QI mode.  */
  if (regno >= AL_REG && regno <= BL_REG)
    return GET_MODE_SIZE (mode) == 1;

  /* CC register only holds CCmode.  */
  if (regno == CC_REG)
    return mode == CCmode;

  /* Segment register: only HImode.  */
  if (regno == ES_REG)
    return mode == HImode;

  /* SP and AP: only pointer-sized values.  */
  if (regno == SP_REG || regno == AP_REG)
    return mode == HImode || mode == Pmode;

  /* General 16-bit registers (AX=0 .. BP=6): can hold HImode and QImode.  */
  if (GET_MODE_SIZE (mode) <= 2)
    return true;

  /* SImode (32-bit) can be held in even-numbered register pairs.
     AX:DX (0:1), CX:BX (2:3), SI:DI (4:5).  */
  if (GET_MODE_SIZE (mode) == 4)
    return (regno == AX_REG || regno == CX_REG || regno == SI_REG);

  return false;
}

#undef  TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK ia16_hard_regno_mode_ok_hook

/* Implement TARGET_MODES_TIEABLE_P.  */
static bool
ia16_modes_tieable_p (machine_mode mode1, machine_mode mode2)
{
  if (mode1 == mode2)
    return true;
  /* Integer modes of the same or smaller size are tieable.  */
  if (GET_MODE_CLASS (mode1) == MODE_INT
      && GET_MODE_CLASS (mode2) == MODE_INT
      && GET_MODE_SIZE (mode1) <= 2
      && GET_MODE_SIZE (mode2) <= 2)
    return true;
  return false;
}

#undef  TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P ia16_modes_tieable_p

/* --------------------------------------------------------------------------
   Frame Pointer / Elimination
   -------------------------------------------------------------------------- */

static bool
ia16_frame_pointer_required (void)
{
  return TARGET_FRAME_POINTER || cfun->calls_alloca;
}

#undef  TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED ia16_frame_pointer_required

bool
ia16_can_eliminate (int from, int to)
{
  if (to == STACK_POINTER_REGNUM && frame_pointer_needed)
    return false;
  return true;
}

#undef  TARGET_CAN_ELIMINATE
#define TARGET_CAN_ELIMINATE ia16_can_eliminate

/* Stack frame layout (after prologue, near call):

   High addresses
     arg 1            AP+2   =  BP+6
     arg 0            AP+0   =  BP+4
     return address            BP+2
     saved BP                  BP+0   <-- hard frame pointer (BP)
     saved regs                BP-2 .. BP-(2+saved_regs)
     local vars                BP-(2+saved_regs) .. BP-(2+saved_regs+locals)
                               <-- SP

   With FRAME_GROWS_DOWNWARD, GCC's virtual frame pointer (FP)
   addresses locals as FP+0, FP+2, ...  We eliminate FP to BP
   by subtracting the callee-saved area + locals size so that
   FP+0 maps to the lowest local variable slot.  */

int
ia16_initial_elimination_offset (int from, int to)
{
  ia16_compute_frame_info ();

  struct machine_function *m = cfun->machine;
  int ret_addr_size = TARGET_FAR_CODE ? 4 : 2;

  if (from == ARG_POINTER_REGNUM && to == FRAME_POINTER_REGNUM)
    {
      /* AP is above the return address and saved BP.
	 AP+0 = BP + saved_bp(2) + ret_addr.  */
      return ret_addr_size + 2;
    }

  if (from == ARG_POINTER_REGNUM && to == STACK_POINTER_REGNUM)
    {
      /* AP to SP: skip ret_addr + saved_bp + saved_regs + locals.  */
      return ret_addr_size + 2 + m->callee_saved_reg_size
	     + m->local_vars_size;
    }

  if (from == FRAME_POINTER_REGNUM && to == STACK_POINTER_REGNUM)
    {
      /* Virtual FP to SP.  With FRAME_GROWS_DOWNWARD, GCC places
	 locals at FP+0, FP+2, etc.  SP is at the bottom, so
	 FP+0 should map to the lowest local slot.
	 FP + offset = SP, so offset = 0 (locals sit just above SP).  */
      return 0;
    }

  gcc_unreachable ();
}

/* --------------------------------------------------------------------------
   Addressing Modes
   -------------------------------------------------------------------------- */

/* On the 8086, valid addressing modes are:
     [BX], [SI], [DI], [BP],
     [BX+SI], [BX+DI], [BP+SI], [BP+DI],
     [BX+disp], [SI+disp], [DI+disp], [BP+disp],
     [BX+SI+disp], [BX+DI+disp], [BP+SI+disp], [BP+DI+disp],
     [disp] (direct addressing)

   We simplify: base can be BX, BP, SI, DI, SP.
   Index can be SI, DI (only with BX or BP as base).
   Displacement can be any constant.  */

static bool
ia16_legitimate_address_p (machine_mode mode ATTRIBUTE_UNUSED,
			   rtx addr, bool strict,
			   code_helper ch ATTRIBUTE_UNUSED)
{
  rtx base = NULL_RTX;
  rtx index = NULL_RTX;
  rtx disp = NULL_RTX;

  /* Direct address (symbolic or constant).  */
  if (CONSTANT_P (addr))
    return true;

  /* Single register.  */
  if (REG_P (addr))
    {
      int regno = REGNO (addr);
      if (!strict || regno < FIRST_PSEUDO_REGISTER)
	return REGNO_OK_FOR_BASE_P (regno);
      return true; /* Pseudo will be allocated to a valid base.  */
    }

  if (GET_CODE (addr) != PLUS)
    return false;

  rtx op0 = XEXP (addr, 0);
  rtx op1 = XEXP (addr, 1);

  /* REG + CONST.  */
  if (REG_P (op0) && CONSTANT_P (op1))
    {
      base = op0;
      disp = op1;
    }
  else if (REG_P (op1) && CONSTANT_P (op0))
    {
      base = op1;
      disp = op0;
    }
  /* REG + REG.  */
  else if (REG_P (op0) && REG_P (op1))
    {
      base = op0;
      index = op1;
    }
  /* (REG + REG) + CONST or REG + (REG + CONST).  */
  else if (GET_CODE (op0) == PLUS && CONSTANT_P (op1))
    {
      if (REG_P (XEXP (op0, 0)) && REG_P (XEXP (op0, 1)))
	{
	  base = XEXP (op0, 0);
	  index = XEXP (op0, 1);
	  disp = op1;
	}
      else
	return false;
    }
  else
    return false;

  /* Validate base register.  */
  if (base)
    {
      int regno = REGNO (base);
      if (strict && regno >= FIRST_PSEUDO_REGISTER)
	return false;
      if (strict && !REGNO_OK_FOR_BASE_P (regno))
	return false;
    }

  /* Validate index register.  On 8086, only SI and DI can be index
     registers, and only with BX or BP as base.  */
  if (index)
    {
      int iregno = REGNO (index);
      if (strict && iregno >= FIRST_PSEUDO_REGISTER)
	return false;
      if (strict && !REGNO_OK_FOR_INDEX_P (iregno))
	return false;
    }

  return true;
}

#undef  TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P ia16_legitimate_address_p

static bool
ia16_legitimate_constant_p (machine_mode mode ATTRIBUTE_UNUSED, rtx x)
{
  return CONSTANT_P (x);
}

#undef  TARGET_LEGITIMATE_CONSTANT_P
#define TARGET_LEGITIMATE_CONSTANT_P ia16_legitimate_constant_p

/* --------------------------------------------------------------------------
   Calling Convention

   All arguments are passed on the stack, right to left (cdecl).
   Return values:  8-bit in AL, 16-bit in AX, 32-bit in DX:AX.
   Caller cleans the stack.
   BX, SI, DI, BP are callee-saved.
   -------------------------------------------------------------------------- */

void
ia16_init_cumulative_args (CUMULATIVE_ARGS *cum,
			   tree fntype ATTRIBUTE_UNUSED,
			   rtx libname ATTRIBUTE_UNUSED,
			   tree fndecl ATTRIBUTE_UNUSED,
			   int n_named_args ATTRIBUTE_UNUSED)
{
  cum->nregs = 0;
  cum->bytes = 0;
}

/* Round bytes up to 16-bit word boundary for stack pushes.  */
poly_int64
ia16_push_rounding (poly_int64 bytes)
{
  return (bytes + 1) & ~(HOST_WIDE_INT)1;
}

/* All args go on the stack for cdecl.  */
static rtx
ia16_function_arg (cumulative_args_t cum_v ATTRIBUTE_UNUSED,
		   const function_arg_info &arg ATTRIBUTE_UNUSED)
{
  return NULL_RTX; /* Pass on the stack.  */
}

static void
ia16_function_arg_advance (cumulative_args_t cum_v,
			   const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  int bytes = arg.promoted_size_in_bytes ();
  /* Round up to 16-bit boundary.  */
  cum->bytes += (bytes + 1) & ~1;
}

#undef  TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG ia16_function_arg

#undef  TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE ia16_function_arg_advance

/* Return values.  */
bool
ia16_function_value_regno_p (unsigned int regno)
{
  return regno == AX_REG;
}

#undef  TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P ia16_function_value_regno_p

/* Return the RTX for the return value.  */
rtx
ia16_function_value (const_tree ret_type,
		     const_tree fn_decl_or_type ATTRIBUTE_UNUSED,
		     bool outgoing ATTRIBUTE_UNUSED)
{
  machine_mode mode = TYPE_MODE (ret_type);
  /* 8-bit values in AL, 16-bit in AX, 32-bit in DX:AX.  */
  return gen_rtx_REG (mode, AX_REG);
}

#undef  TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE ia16_function_value

rtx
ia16_libcall_value (machine_mode mode,
		    const_rtx fun ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (mode, AX_REG);
}

#undef  TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE ia16_libcall_value

static bool
ia16_return_in_memory (const_tree type,
		       const_tree fntype ATTRIBUTE_UNUSED)
{
  /* Return values larger than 4 bytes (32 bits) in memory.  */
  return int_size_in_bytes (type) > 4;
}

#undef  TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY ia16_return_in_memory

static bool
ia16_pass_by_reference (cumulative_args_t cum_v ATTRIBUTE_UNUSED,
			const function_arg_info &arg)
{
  /* Pass aggregates larger than 4 bytes by reference.  */
  return arg.aggregate_type_p () && arg.promoted_size_in_bytes () > 4;
}

#undef  TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE ia16_pass_by_reference

/* --------------------------------------------------------------------------
   Costs
   -------------------------------------------------------------------------- */

int
ia16_register_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
			 reg_class_t from ATTRIBUTE_UNUSED,
			 reg_class_t to ATTRIBUTE_UNUSED)
{
  return 2;
}

#undef  TARGET_REGISTER_MOVE_COST
#define TARGET_REGISTER_MOVE_COST ia16_register_move_cost

int
ia16_memory_move_cost (machine_mode mode ATTRIBUTE_UNUSED,
		       reg_class_t rclass ATTRIBUTE_UNUSED,
		       bool in ATTRIBUTE_UNUSED)
{
  return 4;
}

#undef  TARGET_MEMORY_MOVE_COST
#define TARGET_MEMORY_MOVE_COST ia16_memory_move_cost

static bool
ia16_rtx_costs (rtx x, machine_mode mode, int outer_code ATTRIBUTE_UNUSED,
		int opno ATTRIBUTE_UNUSED, int *total, bool speed)
{
  int code = GET_CODE (x);

  switch (code)
    {
    case CONST_INT:
      if (INTVAL (x) == 0)
	*total = 0;
      else if (IN_RANGE (INTVAL (x), -128, 127))
	*total = 1;
      else
	*total = 2;
      return true;

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
      *total = 2;
      return true;

    case PLUS:
    case MINUS:
    case AND:
    case IOR:
    case XOR:
      if (mode == QImode)
	*total = speed ? 2 : 1;
      else if (mode == HImode)
	*total = speed ? 2 : 2;
      else if (mode == SImode)
	*total = speed ? 6 : 4;
      else
	*total = speed ? 12 : 8;
      return false;

    case MULT:
      /* 8086 has no MUL instruction that GCC can use directly for
	 signed/unsigned multiply easily; it's expensive.  */
      *total = speed ? 30 : 8;
      return false;

    case DIV:
    case UDIV:
    case MOD:
    case UMOD:
      *total = speed ? 50 : 10;
      return false;

    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
      if (CONST_INT_P (XEXP (x, 1)))
	{
	  int count = INTVAL (XEXP (x, 1));
	  /* On 8086, only shift by 1 or by CL.  For constant shifts,
	     we need to emit N individual shifts or load CL.  */
	  if (TARGET_8086)
	    *total = speed ? 2 * count : count + 1;
	  else
	    /* 80186+ has immediate shift counts.  */
	    *total = speed ? 4 : 2;
	}
      else
	*total = speed ? 8 : 3;
      return false;

    case NEG:
    case NOT:
      *total = speed ? 2 : 1;
      return false;

    case MEM:
      *total = speed ? 4 : 2;
      return true;

    default:
      return false;
    }
}

#undef  TARGET_RTX_COSTS
#define TARGET_RTX_COSTS ia16_rtx_costs

/* Use classic reload, not LRA.  LRA support for a new target is complex.  */
#undef  TARGET_LRA_P
#define TARGET_LRA_P hook_bool_void_false

/* --------------------------------------------------------------------------
   Prologue / Epilogue
   -------------------------------------------------------------------------- */

void
ia16_expand_prologue (void)
{
  ia16_compute_frame_info ();
  struct machine_function *m = cfun->machine;
  rtx insn;

  /* Push BP and set up frame pointer if needed.  */
  if (m->uses_frame_pointer)
    {
      insn = emit_insn (gen_push (gen_rtx_REG (HImode, BP_REG)));
      RTX_FRAME_RELATED_P (insn) = 1;

      insn = emit_move_insn (gen_rtx_REG (HImode, BP_REG),
			     gen_rtx_REG (HImode, SP_REG));
      RTX_FRAME_RELATED_P (insn) = 1;
    }

  /* Save callee-saved registers.  */
  for (int i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      if (df_regs_ever_live_p (i)
	  && !call_used_or_fixed_reg_p (i)
	  && !fixed_regs[i]
	  && i != BP_REG)
	{
	  insn = emit_insn (gen_push (gen_rtx_REG (HImode, i)));
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
    }

  /* Allocate local variables.  */
  if (m->local_vars_size > 0)
    {
      insn = emit_insn (gen_addhi3 (gen_rtx_REG (HImode, SP_REG),
				    gen_rtx_REG (HImode, SP_REG),
				    GEN_INT (-m->local_vars_size)));
      RTX_FRAME_RELATED_P (insn) = 1;
    }
}

void
ia16_expand_epilogue (bool is_sibcall ATTRIBUTE_UNUSED)
{
  ia16_compute_frame_info ();
  struct machine_function *m = cfun->machine;

  /* Deallocate locals.  */
  if (m->local_vars_size > 0)
    {
      emit_insn (gen_addhi3 (gen_rtx_REG (HImode, SP_REG),
			     gen_rtx_REG (HImode, SP_REG),
			     GEN_INT (m->local_vars_size)));
    }

  /* Restore callee-saved registers (reverse order).  */
  for (int i = FIRST_PSEUDO_REGISTER - 1; i >= 0; i--)
    {
      if (df_regs_ever_live_p (i)
	  && !call_used_or_fixed_reg_p (i)
	  && !fixed_regs[i]
	  && i != BP_REG)
	{
	  emit_insn (gen_pop (gen_rtx_REG (HImode, i)));
	}
    }

  /* Restore frame pointer.  */
  if (m->uses_frame_pointer)
    {
      emit_insn (gen_pop (gen_rtx_REG (HImode, BP_REG)));
    }

  /* The actual RET instruction is emitted by the epilogue pattern
     in the .md file.  */
}

/* --------------------------------------------------------------------------
   Assembly Output
   -------------------------------------------------------------------------- */

static void
ia16_asm_file_start (void)
{
  fprintf (asm_out_file, "\t.code16\n");
  default_file_start ();

  switch (ia16_arch)
    {
    case IA16_CPU_8086:
      fprintf (asm_out_file, "\t.arch i8086\n");
      break;
    case IA16_CPU_80186:
      fprintf (asm_out_file, "\t.arch i80186\n");
      break;
    case IA16_CPU_80286:
      fprintf (asm_out_file, "\t.arch i286\n");
      break;
    }
}

#undef  TARGET_ASM_FILE_START
#define TARGET_ASM_FILE_START ia16_asm_file_start

/* Print an operand to the assembler.  */
void
ia16_print_operand (FILE *file, rtx x, int code)
{
  switch (code)
    {
    case 'h':
      /* Print high byte register name.  */
      if (REG_P (x))
	{
	  switch (REGNO (x))
	    {
	    case AX_REG: fputs ("%ah", file); return;
	    case BX_REG: fputs ("%bh", file); return;
	    case CX_REG: fputs ("%ch", file); return;
	    case DX_REG: fputs ("%dh", file); return;
	    default: break;
	    }
	}
      break;

    case 'l':
      /* Print low byte register name.  */
      if (REG_P (x))
	{
	  switch (REGNO (x))
	    {
	    case AX_REG: fputs ("%al", file); return;
	    case BX_REG: fputs ("%bl", file); return;
	    case CX_REG: fputs ("%cl", file); return;
	    case DX_REG: fputs ("%dl", file); return;
	    default: break;
	    }
	}
      break;

    case 'b':
      /* Print byte-size operand suffix.  */
      fputc ('b', file);
      return;

    case 'w':
      /* Print word-size operand suffix.  */
      fputc ('w', file);
      return;

    case 0:
      /* Default: print operand normally.  */
      break;

    default:
      output_operand_lossage ("invalid operand code '%c'", code);
      return;
    }

  if (REG_P (x))
    fputs (reg_names[REGNO (x)], file);
  else if (MEM_P (x))
    {
      ia16_print_operand_address (file, GET_MODE (x), XEXP (x, 0));
    }
  else if (CONST_INT_P (x))
    fprintf (file, "$%d", (int) INTVAL (x));
  else
    output_addr_const (file, x);
}

/* Print a memory address.  AT&T syntax: displacement(%base,%index).  */
void
ia16_print_operand_address (FILE *file, machine_mode mode ATTRIBUTE_UNUSED,
			    rtx addr)
{
  rtx base = NULL_RTX;
  rtx index = NULL_RTX;
  rtx disp = NULL_RTX;

  if (REG_P (addr))
    {
      base = addr;
    }
  else if (CONSTANT_P (addr))
    {
      disp = addr;
    }
  else if (GET_CODE (addr) == PLUS)
    {
      rtx op0 = XEXP (addr, 0);
      rtx op1 = XEXP (addr, 1);

      if (REG_P (op0) && CONSTANT_P (op1))
	{
	  base = op0;
	  disp = op1;
	}
      else if (REG_P (op1) && CONSTANT_P (op0))
	{
	  base = op1;
	  disp = op0;
	}
      else if (REG_P (op0) && REG_P (op1))
	{
	  base = op0;
	  index = op1;
	}
      else if (GET_CODE (op0) == PLUS && CONSTANT_P (op1))
	{
	  if (REG_P (XEXP (op0, 0)) && REG_P (XEXP (op0, 1)))
	    {
	      base = XEXP (op0, 0);
	      index = XEXP (op0, 1);
	      disp = op1;
	    }
	}
    }

  if (disp)
    output_addr_const (file, disp);

  if (base || index)
    {
      fputc ('(', file);
      if (base)
	fputs (reg_names[REGNO (base)], file);
      if (index)
	{
	  fputc (',', file);
	  fputs (reg_names[REGNO (index)], file);
	}
      fputc (')', file);
    }
}

#undef  TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND ia16_print_operand

#undef  TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS ia16_print_operand_address

/* Helper for move instruction output.  */
const char *
ia16_output_move_insn (rtx *operands, machine_mode mode)
{
  if (mode == QImode)
    return "movb\t%1, %0";
  else
    return "movw\t%1, %0";
}

/* Split a SImode move into two HImode moves.  OPERANDS[0..1] are the
   original dest/src; RESULT[0..3] are filled with lo_dest, lo_src,
   hi_dest, hi_src.  */

void
ia16_split_movsi (rtx *operands, rtx *result)
{
  rtx dest = operands[0];
  rtx src = operands[1];

  /* Low half.  */
  result[0] = gen_lowpart (HImode, dest);
  result[1] = gen_lowpart (HImode, src);
  /* High half.  */
  result[2] = gen_highpart (HImode, dest);
  result[3] = gen_highpart (HImode, src);
}

/* Return true if the current function can use a simple RET.
   This is the case when no epilogue work is needed.  */

bool
ia16_can_use_return_insn_p (void)
{
  ia16_compute_frame_info ();
  struct machine_function *m = cfun->machine;

  if (m->frame_size != 0)
    return false;
  if (m->uses_frame_pointer)
    return false;

  /* Check if any callee-saved registers need restoring.  */
  for (int i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    {
      if (df_regs_ever_live_p (i)
	  && !call_used_or_fixed_reg_p (i)
	  && !fixed_regs[i])
	return false;
    }

  return true;
}

/* --------------------------------------------------------------------------
   Target Hook Struct
   -------------------------------------------------------------------------- */

struct gcc_target targetm = TARGET_INITIALIZER;

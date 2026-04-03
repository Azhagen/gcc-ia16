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

  /* Count callee-saved hard registers that need saving.  */
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
    case BX_REG: return BASE_REGS;
    case SI_REG: return SIREG;
    case DI_REG: return DIREG;
    case BP_REG: return BASE_REGS;
    case SP_REG: return GENERAL_REGS;
    case ES_REG: return SEG_REGS;
    case CC_REG: return NO_REGS;
    case AP_REG: return GENERAL_REGS;
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
  /* Treat singleton and otherwise invalid hard-reg/mode combinations as
     occupying exactly one hard register.  LRA can query this hook while
     building hard-reg spans without first checking HARD_REGNO_MODE_OK, and
     returning a multi-register span for CC/AP/ES makes it walk into
     unrelated adjacent hard registers.  */
  if (!ia16_hard_regno_mode_ok (regno, mode)
      || regno == SP_REG
      || regno == ES_REG
      || regno == CC_REG
      || regno == AP_REG)
    return 1;

  /* General 16-bit registers use one hard reg per word.  */
  unsigned int size = GET_MODE_SIZE (mode);
  return (size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
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
  unsigned int size;

  /* CC register only holds CCmode.  */
  if (regno == CC_REG)
    return mode == CCmode;

  /* Segment register: only HImode.  */
  if (regno == ES_REG)
    return mode == HImode;

  /* SP and AP: only pointer-sized values.  */
  if (regno == SP_REG || regno == AP_REG)
    return mode == HImode || mode == Pmode;

  /* Only AX/BX/CX/DX have byte-addressable subregs.  */
  if (mode == QImode)
    return regno == AX_REG || regno == DX_REG
	   || regno == CX_REG || regno == BX_REG;

  size = GET_MODE_SIZE (mode);

  /* General registers AX..BP can carry scalar values as contiguous 16-bit
     words, but multiword values must not spill into SP/ES/CC/AP.  */
  if (size <= UNITS_PER_WORD)
    return regno < SP_REG;

  if (size > 8)
    return false;

  return regno + ((size + UNITS_PER_WORD - 1) / UNITS_PER_WORD) <= SP_REG;
}

#undef  TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK ia16_hard_regno_mode_ok_hook

/* Implement TARGET_MODES_TIEABLE_P.  */
static bool
ia16_modes_tieable_p (machine_mode mode1, machine_mode mode2)
{
  if (mode1 == mode2)
    return true;

  /* QImode is only available in AX/BX/CX/DX, so it is not generally
     tieable with wider integer modes.  */
  if (mode1 == QImode || mode2 == QImode)
    return false;

  /* Scalar and complex modes that occupy the same number of 16-bit words
     use the same hard-register layouts.  */
  if (GET_MODE_CLASS (mode1) != MODE_CC
      && GET_MODE_CLASS (mode2) != MODE_CC
      && GET_MODE_SIZE (mode1) == GET_MODE_SIZE (mode2)
      && GET_MODE_SIZE (mode1) <= 8)
    return true;
  return false;
}

#undef  TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P ia16_modes_tieable_p

/* Implement TARGET_CAN_CHANGE_MODE_CLASS.  QImode is only a view of
   AX/BX/CX/DX, so refuse QI mode changes for classes that include any
   other hard register.  */
static bool
ia16_can_change_mode_class (machine_mode from, machine_mode to,
			    reg_class_t rclass)
{
  if (from == to)
    return true;

  if (from == QImode || to == QImode)
    return reg_class_subset_p (rclass, QI_REGS);

  return true;
}

#undef  TARGET_CAN_CHANGE_MODE_CLASS
#define TARGET_CAN_CHANGE_MODE_CLASS ia16_can_change_mode_class

/* The assembler accepts explicit 16-bit and 32-bit integer directives.
   Provide the 32-bit form so DWARF can emit real 4-byte lengths and
   addresses instead of splitting them into overflowing 16-bit pieces.  */
#undef  TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\t.word\t"

#undef  TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.long\t"

#undef  TARGET_ASM_UNALIGNED_HI_OP
#define TARGET_ASM_UNALIGNED_HI_OP TARGET_ASM_ALIGNED_HI_OP

#undef  TARGET_ASM_UNALIGNED_SI_OP
#define TARGET_ASM_UNALIGNED_SI_OP TARGET_ASM_ALIGNED_SI_OP

/* --------------------------------------------------------------------------
   Frame Pointer / Elimination
   -------------------------------------------------------------------------- */

static bool
ia16_frame_pointer_required (void)
{
  /* SP is not a valid base register for ordinary memory operands on ia16.
     As soon as GCC chooses SP as the frame base, reload can form illegal
     stack references like [sp-4].  Keep BP as the hard frame pointer and
     solve the original register-pressure issue via reload-address
     legalization instead.  */
  return true;
}

#undef  TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED ia16_frame_pointer_required

bool
ia16_can_eliminate (int from ATTRIBUTE_UNUSED, int to)
{
  if (to == STACK_POINTER_REGNUM)
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

/* Decompose an address RTX into base, index, and displacement
   components.  Normalizes the base/index order for 8086 addressing:
   base must be BX/BP, index must be SI/DI.
   Returns true if the address could be decomposed.  */

static bool
ia16_decompose_address (rtx addr, rtx *base_out, rtx *index_out,
			rtx *disp_out)
{
  rtx base = NULL_RTX;
  rtx index = NULL_RTX;
  rtx disp = NULL_RTX;

  if (CONSTANT_P (addr))
    {
      disp = addr;
    }
  else if (REG_P (addr))
    {
      base = addr;
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
	  else
	    return false;
	}
      else
	return false;
    }
  else
    return false;

  /* Normalize base/index order for 8086.  On 8086, only SI and DI
     can be index registers.  If the current assignment has a non-index
     register as index, swap base and index.  */
  if (base && index
      && (unsigned) REGNO (base) < FIRST_PSEUDO_REGISTER
      && (unsigned) REGNO (index) < FIRST_PSEUDO_REGISTER
      && !REGNO_OK_FOR_INDEX_P (REGNO (index)))
    {
      rtx tmp = base;
      base = index;
      index = tmp;
    }

  *base_out = base;
  *index_out = index;
  *disp_out = disp;
  return true;
}

static bool
ia16_legitimate_address_p (machine_mode mode ATTRIBUTE_UNUSED,
			   rtx addr, bool strict,
			   code_helper ch ATTRIBUTE_UNUSED)
{
  rtx base, index, disp;

  if (!ia16_decompose_address (addr, &base, &index, &disp))
    return false;

  /* Direct address (constant only, no registers).  */
  if (!base && !index)
    return true;

  /* Validate base register.  Reject hard registers that are not valid
     8086 base registers (BX, SI, DI, BP) in both strict and non-strict
     modes, so GCC allocates base registers correctly from the start.  */
  if (base)
    {
      int regno = REGNO (base);
      if (strict && (unsigned) regno >= FIRST_PSEUDO_REGISTER)
	return false;
      if ((unsigned) regno < FIRST_PSEUDO_REGISTER
	  && !REGNO_OK_FOR_BASE_P (regno))
	return false;
    }

  /* Validate index register and base+index combination.
     On 8086, only these base+index pairs are valid:
      BX+SI, BX+DI, BP+SI, BP+DI.  */
  if (index)
    {
      /* An address cannot use the same register as both base and index.
	 Reject this even for pseudos so combine does not keep forms like
	 `sym + i + i' as a decomposed address.  */
      if (base && rtx_equal_p (base, index))
	return false;

      int iregno = REGNO (index);
      if (strict && (unsigned) iregno >= FIRST_PSEUDO_REGISTER)
	return false;
      if ((unsigned) iregno < FIRST_PSEUDO_REGISTER)
	{
	  if (iregno != SI_REG && iregno != DI_REG)
	    return false;
	  if (base && (unsigned) REGNO (base) < FIRST_PSEUDO_REGISTER)
	    {
	      int bregno = REGNO (base);
	      if (bregno != BX_REG && bregno != BP_REG)
		return false;
	      if (bregno == iregno)
		return false;
	    }
	}

      /* ia16 is better off materializing unresolved pseudo base+index
	 addresses into a single temporary address register than trying to
	 preserve separate base and index terms through allocation.  If we
	 accept pseudo base+index here, IRA/LRA pins pointer pseudos into
	 BASE_REGS and quickly runs out of legal combinations for cases like
	 `*(p0 + x0) cmp *(p1 + x1)'.  */
      if (!strict
	  && ((unsigned) REGNO (base) >= FIRST_PSEUDO_REGISTER
	      || (unsigned) REGNO (index) >= FIRST_PSEUDO_REGISTER))
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

/* Try to rewrite an invalid address into a valid one.
   The 8086 has very limited addressing modes, so complex addresses
   need to be decomposed into register loads.  */

static rtx
ia16_legitimize_address (rtx x, rtx oldx ATTRIBUTE_UNUSED,
			 machine_mode mode ATTRIBUTE_UNUSED)
{
  /* (plus (mem ...) REG) or (plus REG (mem ...)) — memory-indirect.
     Load the memory part into a register.  */
  if (GET_CODE (x) == PLUS)
    {
      rtx op0 = XEXP (x, 0);
      rtx op1 = XEXP (x, 1);

      if (MEM_P (op0))
	return gen_rtx_PLUS (Pmode, force_reg (Pmode, op0), op1);
      if (MEM_P (op1))
	return gen_rtx_PLUS (Pmode, op0, force_reg (Pmode, op1));

      /* (plus (plus REG REG) CONST).  ia16 does not want unresolved
	 pseudo base+index addresses to survive into IRA/LRA, so compute
	 REG+REG into a register whenever either term is still a pseudo or
	 the eventual hard-reg pair would be invalid.  */
      if ((GET_CODE (op0) == PLUS && CONSTANT_P (op1))
	  || (GET_CODE (op1) == PLUS && CONSTANT_P (op0)))
	{
	  rtx inner = GET_CODE (op0) == PLUS ? op0 : op1;
	  rtx disp = GET_CODE (op0) == PLUS ? op1 : op0;
	  rtx inner0 = XEXP (inner, 0);
	  rtx inner1 = XEXP (inner, 1);
	  if (REG_P (inner0) && REG_P (inner1))
	    {
	      if (rtx_equal_p (inner0, inner1)
		  || (unsigned) REGNO (inner0) >= FIRST_PSEUDO_REGISTER
		  || (unsigned) REGNO (inner1) >= FIRST_PSEUDO_REGISTER)
		return gen_rtx_PLUS (Pmode, force_reg (Pmode, inner), disp);

	      rtx base, index;
	      base = inner0;
	      index = inner1;
	      /* Normalize order.  */
	      if ((unsigned) REGNO (base) < FIRST_PSEUDO_REGISTER
		  && (unsigned) REGNO (index) < FIRST_PSEUDO_REGISTER
		  && !REGNO_OK_FOR_INDEX_P (REGNO (index)))
		{
		  base = inner1;
		  index = inner0;
		}
	      /* If still not a valid pair, fold into a register.  */
	      if ((unsigned) REGNO (base) < FIRST_PSEUDO_REGISTER
		  && (unsigned) REGNO (index) < FIRST_PSEUDO_REGISTER)
		{
		  int bregno = REGNO (base);
		  int iregno = REGNO (index);
		  if ((iregno != SI_REG && iregno != DI_REG)
		      || (bregno != BX_REG && bregno != BP_REG)
		      || bregno == iregno)
		    return gen_rtx_PLUS (Pmode,
					force_reg (Pmode, inner), disp);
		}
	    }
	}

      /* (plus REG REG).  Materialize unresolved pseudo pairs, or invalid
	 hard-reg pairs, into a temporary address register.  */
      if (REG_P (op0) && REG_P (op1))
	{
	  if (rtx_equal_p (op0, op1))
	    return force_reg (Pmode, x);

	  int r0 = REGNO (op0);
	  int r1 = REGNO (op1);
	  if ((unsigned) r0 >= FIRST_PSEUDO_REGISTER
	      || (unsigned) r1 >= FIRST_PSEUDO_REGISTER)
	    return force_reg (Pmode, x);

	  if ((unsigned) r0 < FIRST_PSEUDO_REGISTER
	      && (unsigned) r1 < FIRST_PSEUDO_REGISTER)
	    {
	      /* Check if either ordering works.  */
	      bool ok = false;
	      if (REGNO_OK_FOR_BASE_P (r0) && REGNO_OK_FOR_INDEX_P (r1))
		ok = true;
	      if (REGNO_OK_FOR_BASE_P (r1) && REGNO_OK_FOR_INDEX_P (r0))
		ok = true;
	      if (!ok)
		return force_reg (Pmode, x);
	    }
	}
    }

  return x;
}

#undef  TARGET_LEGITIMIZE_ADDRESS
#define TARGET_LEGITIMIZE_ADDRESS ia16_legitimize_address

/* Worker for LEGITIMIZE_RELOAD_ADDRESS.

   If reload keeps an ia16 base+index address in decomposed form, each
   address can consume two hard registers: one BASE_INDEX_REGS register
   and one INDEX_REGS register.  That is enough to trigger reload
   failures in cases like `*(p0 + x0) cmp *(p1 + x1)' when BP is also
   tied up as the hard frame pointer.  Force such addresses through a
   single BASE_REGS reload instead.  */
bool
ia16_legitimize_reload_address (rtx *x, machine_mode mode ATTRIBUTE_UNUSED,
				int opnum, int itype,
				int ind_levels ATTRIBUTE_UNUSED)
{
  enum reload_type type = (enum reload_type) itype;
  rtx addr = *x;

  if (type == RELOAD_OTHER)
    type = RELOAD_FOR_OTHER_ADDRESS;

  if (GET_CODE (addr) == PLUS)
    {
      rtx op0 = XEXP (addr, 0);
      rtx op1 = XEXP (addr, 1);

      if ((REG_P (op0) && REG_P (op1))
	  || (GET_CODE (op0) == PLUS
	      && REG_P (XEXP (op0, 0))
	      && REG_P (XEXP (op0, 1))
	      && CONSTANT_P (op1))
	  || (GET_CODE (op1) == PLUS
	      && REG_P (XEXP (op1, 0))
	      && REG_P (XEXP (op1, 1))
	      && CONSTANT_P (op0)))
	{
	  push_reload (addr, NULL_RTX, x, NULL,
		       BASE_REGS, Pmode, VOIDmode, 0, 0, opnum, type);
	  return true;
	}
    }

  return false;
}

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

  /* Save callee-saved hard registers.  */
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

  /* Restore callee-saved hard registers in reverse order.  */
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
  rtx reg = x;

  /* Lowpart subregs are how reload models byte views of wider pseudos.  */
  if (SUBREG_P (reg)
      && REG_P (SUBREG_REG (reg))
      && subreg_lowpart_p (reg))
    reg = SUBREG_REG (reg);

  switch (code)
    {
    case 'H':
      /* Print high byte register name.  */
      if (REG_P (reg))
	{
	  switch (REGNO (reg))
	    {
	    case AX_REG: fputs ("%ah", file); return;
	    case BX_REG: fputs ("%bh", file); return;
	    case CX_REG: fputs ("%ch", file); return;
	    case DX_REG: fputs ("%dh", file); return;
	    default: break;
	    }
	}
      break;

    case 'L':
      /* Print low byte register name.  */
      if (REG_P (reg))
	{
	  switch (REGNO (reg))
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

    case 'P':
      /* Print operand without '$' prefix (for call/jmp targets).  */
      break;

    case 0:
      /* Default: print operand normally.  */
      break;

    default:
      output_operand_lossage ("invalid operand code '%c'", code);
      return;
    }

  if (REG_P (reg))
    {
      int regno = REGNO (reg);
      /* In QImode, use byte register names for AX/DX/CX/BX.  */
      if (GET_MODE (x) == QImode)
	{
	  switch (regno)
	    {
	    case AX_REG: fputs ("%al", file); return;
	    case DX_REG: fputs ("%dl", file); return;
	    case CX_REG: fputs ("%cl", file); return;
	    case BX_REG: fputs ("%bl", file); return;
	    default: break;
	    }
	}
      fputs (reg_names[regno], file);
    }
  else if (MEM_P (x))
    {
      ia16_print_operand_address (file, GET_MODE (x), XEXP (x, 0));
    }
  else
    {
      /* In AT&T syntax, immediates need a '$' prefix.
	 The 'P' modifier suppresses '$' (for call/jmp targets).  */
      if (code != 'P'
	  && (CONST_INT_P (x) || SYMBOL_REF_P (x)
	      || LABEL_REF_P (x) || GET_CODE (x) == CONST))
	fputc ('$', file);

      if (CONST_INT_P (x))
	fprintf (file, HOST_WIDE_INT_PRINT_DEC, INTVAL (x));
      else
	output_addr_const (file, x);
    }
}

/* Print a memory address.  AT&T syntax: displacement(%base,%index).  */
void
ia16_print_operand_address (FILE *file, machine_mode mode ATTRIBUTE_UNUSED,
			    rtx addr)
{
  rtx base, index, disp;

  ia16_decompose_address (addr, &base, &index, &disp);

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

/* Return complex part PART of X in MODE as an inner-mode operand.  */
rtx
ia16_complex_part (rtx x, machine_mode mode, unsigned int part)
{
  machine_mode inner_mode = GET_MODE_INNER (mode);
  rtx result;

  gcc_assert (COMPLEX_MODE_P (mode));
  gcc_assert (part < 2);

  if (GET_CODE (x) == CONCAT)
    {
      gcc_assert (GET_MODE (x) == mode);
      return XEXP (x, part);
    }

  result = simplify_gen_subreg (inner_mode, x, mode,
				part * GET_MODE_SIZE (inner_mode));
  gcc_assert (result != NULL_RTX);
  return result;
}

/* Return word WORD of X in MODE as an HImode operand.  */
rtx
ia16_subword (rtx x, machine_mode mode, unsigned int word)
{
  if (GET_CODE (x) == CONCAT && COMPLEX_MODE_P (mode))
    {
      machine_mode inner_mode = GET_MODE_INNER (mode);
      unsigned int inner_words =
	(GET_MODE_SIZE (inner_mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

      if (word < inner_words)
	return ia16_subword (XEXP (x, 0), inner_mode, word);
      return ia16_subword (XEXP (x, 1), inner_mode, word - inner_words);
    }

  rtx part = operand_subword (x, word, 1, mode);

  gcc_assert (part != NULL_RTX);
  return part;
}

rtx
ia16_split_si_half (rtx x, int high)
{
  return ia16_subword (x, SImode, high);
}

/* Emit a multiword move as one HImode move per word.  */
void
ia16_emit_multiword_move (rtx *operands, machine_mode mode)
{
  const unsigned int nwords =
    (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

  for (unsigned int i = 0; i < nwords; ++i)
    emit_move_insn (ia16_subword (operands[0], mode, i),
		    ia16_subword (operands[1], mode, i));
}

/* Push a multiword value one HImode chunk at a time, highest word first.  */
void
ia16_emit_multiword_push (rtx op, machine_mode mode)
{
  const unsigned int nwords =
    (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

  for (unsigned int i = nwords; i-- > 0; )
    {
      rtx part = ia16_subword (op, mode, i);

      if (TARGET_8086 && CONSTANT_P (part))
	part = force_reg (HImode, part);

      emit_insn (gen_push (part));
    }
}

void
ia16_split_movsi (rtx *operands, rtx *result)
{
  result[0] = ia16_subword (operands[0], SImode, 0);
  result[1] = ia16_subword (operands[1], SImode, 0);
  result[2] = ia16_subword (operands[0], SImode, 1);
  result[3] = ia16_subword (operands[1], SImode, 1);
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

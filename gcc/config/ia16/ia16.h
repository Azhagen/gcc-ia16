/* GCC backend definitions for the Intel 16-bit x86 (ia16) target.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_IA16_H
#define GCC_IA16_H

/* Run-time Target Specification.  */

#define TARGET_CPU_CPP_BUILTINS()			\
  do							\
    {							\
      builtin_define ("__ia16__");			\
      builtin_define ("__IA16__");			\
      builtin_assert ("cpu=ia16");			\
      builtin_assert ("machine=ia16");			\
      if (TARGET_8086)					\
	builtin_define ("__8086__");			\
      else if (TARGET_80186)				\
	{						\
	  builtin_define ("__80186__");			\
	  builtin_define ("__186__");			\
	}						\
      else if (TARGET_80286)				\
	{						\
	  builtin_define ("__80286__");			\
	  builtin_define ("__286__");			\
	}						\
      switch (ia16_memmodel)			\
	{						\
	case IA16_MODEL_TINY:				\
	  builtin_define ("__TINY__");			\
	  break;					\
	case IA16_MODEL_SMALL:				\
	  builtin_define ("__SMALL__");			\
	  break;					\
	case IA16_MODEL_MEDIUM:				\
	  builtin_define ("__MEDIUM__");		\
	  break;					\
	case IA16_MODEL_COMPACT:			\
	  builtin_define ("__COMPACT__");		\
	  break;					\
	case IA16_MODEL_LARGE:				\
	  builtin_define ("__LARGE__");			\
	  break;					\
	case IA16_MODEL_HUGE:				\
	  builtin_define ("__HUGE__");			\
	  break;					\
	}						\
      if (ia16_memmodel >= IA16_MODEL_MEDIUM)	\
	builtin_define ("__FAR_CODE__");		\
      if (ia16_memmodel >= IA16_MODEL_COMPACT	\
	  && ia16_memmodel != IA16_MODEL_MEDIUM)	\
	builtin_define ("__FAR_DATA__");		\
    }							\
  while (0)

/* Target CPU flags.  */
#define TARGET_8086	(ia16_arch == IA16_CPU_8086)
#define TARGET_80186	(ia16_arch == IA16_CPU_80186)
#define TARGET_80286	(ia16_arch == IA16_CPU_80286)

/* Memory model flags.  */
#define TARGET_FAR_CODE	\
  (ia16_memmodel >= IA16_MODEL_MEDIUM)
#define TARGET_FAR_DATA	\
  (ia16_memmodel >= IA16_MODEL_COMPACT \
   && ia16_memmodel != IA16_MODEL_MEDIUM)

/* Spec strings.  */
#undef  STARTFILE_SPEC
#define STARTFILE_SPEC "%{!nostartfiles:crt0.o%s}"

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC ""

/* The backend emits a .arch directive itself; the ia16 assembler does not
   accept -mcpu=... command-line options.  */
#define ASM_SPEC ""

#define LINK_SPEC "%{!T*:-T ia16.ld%s}"

#undef  LIB_SPEC
#define LIB_SPEC "%{!nostdlib:-lgcc}"

/* --------------------------------------------------------------------------
   Storage Layout
   -------------------------------------------------------------------------- */

#define BITS_BIG_ENDIAN		0
#define BYTES_BIG_ENDIAN	0
#define WORDS_BIG_ENDIAN	0

#define UNITS_PER_WORD		2

/* libgcc2.c uses LIBGCC2_UNITS_PER_WORD to size its Wtype (word)
   and DWtype (double-word).  With LIBGCC2_UNITS_PER_WORD=2, Wtype
   is HImode (16-bit) and DWtype is SImode (32-bit), so libgcc2
   generates the SI routines we need (__mulsi3, __divsi3, etc.).  */
#define LIBGCC2_UNITS_PER_WORD	2

#define SHORT_TYPE_SIZE		16
#define INT_TYPE_SIZE		16
#define LONG_TYPE_SIZE		32
#define LONG_LONG_TYPE_SIZE	64

#define DEFAULT_SIGNED_CHAR	1

#define STRICT_ALIGNMENT	0
#define FUNCTION_BOUNDARY	8
#define BIGGEST_ALIGNMENT	16
#define STACK_BOUNDARY		16
#define PARM_BOUNDARY		16
#define PCC_BITFIELD_TYPE_MATTERS 1

#define STACK_GROWS_DOWNWARD	1
#define FRAME_GROWS_DOWNWARD	1
#define FIRST_PARM_OFFSET(FNDECL) 0

#define MAX_REGS_PER_ADDRESS	2

/* Pointer size is 16 bits for near pointers, 32 bits for far pointers.  */
#define Pmode		HImode
#define POINTER_SIZE	16
#define POINTERS_EXTEND_UNSIGNED 1

/* Far pointer mode: segment:offset = 32 bits.  */
#define ADDR_SPACE_FAR		1

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)	\
  if (GET_MODE_CLASS (MODE) == MODE_INT		\
      && GET_MODE_SIZE (MODE) < 2)		\
    (MODE) = HImode;

/* --------------------------------------------------------------------------
   Layout of Source Language Data Types
   -------------------------------------------------------------------------- */

#undef  SIZE_TYPE
#define SIZE_TYPE		"unsigned int"
#undef  PTRDIFF_TYPE
#define PTRDIFF_TYPE		"int"
#undef  WCHAR_TYPE
#define WCHAR_TYPE		"int"
#undef  WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE		16

#define FUNCTION_MODE		HImode
#define CASE_VECTOR_MODE	HImode
#define HAS_LONG_COND_BRANCH	0
#define HAS_LONG_UNCOND_BRANCH	0

#define BRANCH_COST(speed_p, predictable_p) 3
#define LOGICAL_OP_NON_SHORT_CIRCUIT 0

#define LOAD_EXTEND_OP(M)	ZERO_EXTEND
#define WORD_REGISTER_OPERATIONS 1

#define MOVE_MAX		2
#define SLOW_BYTE_ACCESS	0

/* --------------------------------------------------------------------------
   Register Usage

   The ia16 has the following registers:

   Word registers are numbered consecutively so that adjacent
   registers form valid SImode pairs (e.g., AX:DX = regs 0:1).

   Register  Number  Name     Purpose
   --------  ------  ----     -------
   AX        0       %ax      Accumulator, return value (lo)
   DX        1       %dx      Data register, return value (hi)
   CX        2       %cx      Count register
   BX        3       %bx      Base register
   SI        4       %si      Source index
   DI        5       %di      Destination index
   BP        6       %bp      Hard frame pointer
   SP        7       %sp      Stack pointer
   ES        8       %es      Extra segment
   CC        9       cc       Condition codes (virtual)
   AP       10       argp     Arg pointer (virtual)
  FP       11       frame    Frame pointer (virtual)

  Byte names like %al/%ah are assembler aliases for AX/DX/CX/BX views,
  not separate hard registers.
   -------------------------------------------------------------------------- */

/* Hard register numbers.  */
#define AX_REG		0
#define DX_REG		1
#define CX_REG		2
#define BX_REG		3
#define SI_REG		4
#define DI_REG		5
#define BP_REG		6
#define SP_REG		7
#define ES_REG		8
#define CC_REG		9
#define AP_REG		10  /* Arg pointer (virtual).  */
#define FP_REG		11  /* Frame pointer (virtual).  */

#define FIRST_PSEUDO_REGISTER 12

/* Register names for the assembler.  */
#define REGISTER_NAMES						\
{								\
  "%ax", "%dx", "%cx", "%bx", "%si", "%di", "%bp", "%sp",	\
  "%es", "cc", "argp", "frame"				\
}

/* AX, BX, CX, and DX have byte-addressable low and high halves.
   GCC models those as QImode views of the parent HImode hard registers,
   rather than as separate hard registers.  */

#define ADDITIONAL_REGISTER_NAMES					\
{									\
  { "al", AX_REG }, { "ah", AX_REG },					\
  { "dl", DX_REG }, { "dh", DX_REG },					\
  { "cl", CX_REG }, { "ch", CX_REG },					\
  { "bl", BX_REG }, { "bh", BX_REG }					\
}

enum reg_class
{
  NO_REGS,
  AREG,		/* AX only.  */
  ABREG,	/* AX, DX, CX, BX (registers with byte subregs).  */
  CREG,		/* CX only (for shifts).  */
  DREG,		/* DX only (for multiply/divide).  */
  ADREG,	/* AX + DX (for 32-bit return).  */
  SIREG,	/* SI only (string ops source).  */
  DIREG,	/* DI only (string ops dest).  */
  INDEX_REGS,	/* SI, DI (index registers for addressing).  */
  BASE_INDEX_REGS, /* BX, BP, AP, FP (base when index is present).  */
  BASE_REGS,	/* BX, SI, DI, BP, AP, FP (valid base for addressing).  */
  QI_REGS,	/* AX, DX, CX, BX (byte-capable).  */
  GENERAL_REGS,	/* AX, BX, CX, DX, SI, DI, BP, SP, AP, FP.  */
  SEG_REGS,	/* ES.  */
  ALL_REGS,
  LIM_REG_CLASSES
};

#define N_REG_CLASSES	((int) LIM_REG_CLASSES)

#define REG_CLASS_NAMES				\
{						\
  "NO_REGS",					\
  "AREG",					\
  "ABREG",					\
  "CREG",					\
  "DREG",					\
  "ADREG",					\
  "SIREG",					\
  "DIREG",					\
  "INDEX_REGS",				\
  "BASE_INDEX_REGS",				\
  "BASE_REGS",					\
  "QI_REGS",					\
  "GENERAL_REGS",				\
  "SEG_REGS",					\
  "ALL_REGS"					\
}

/*  Registers:        FP AP CC ES SP BP DI SI BX CX DX AX
    Bit positions:    11 10  9  8  7  6  5  4  3  2  1  0  */
#define REG_CLASS_CONTENTS						\
{									\
  { 0x0000 },	/* NO_REGS     */					\
  { 0x0001 },	/* AREG        - AX */					\
  { 0x000F },	/* ABREG       - AX, DX, CX, BX */			\
  { 0x0004 },	/* CREG        - CX */					\
  { 0x0002 },	/* DREG        - DX */					\
  { 0x0003 },	/* ADREG       - AX, DX */				\
  { 0x0010 },	/* SIREG       - SI */					\
  { 0x0020 },	/* DIREG       - DI */					\
  { 0x0030 },	/* INDEX_REGS  - SI, DI */				\
  { 0x0C48 },	/* BASE_INDEX_REGS - BX, BP, AP, FP */			\
  { 0x0C78 },	/* BASE_REGS   - BX, SI, DI, BP, AP, FP */		\
  { 0x000F },	/* QI_REGS     - AX, DX, CX, BX */			\
  { 0x0CFF },	/* GENERAL_REGS - AX, DX, CX, BX, SI, DI, BP, SP, AP, FP */ \
  { 0x0100 },	/* SEG_REGS    - ES */					\
  { 0x0FFF }	/* ALL_REGS    */					\
}

#define GENERAL_REGS	GENERAL_REGS

/* When an index register is present, the final hard-reg base must be
   BX or BP on 8086.  During LRA, however, unresolved base+index pseudos
   are already rejected by ia16_legitimate_address_p, so using
   BASE_INDEX_REGS here only over-constrains intermediate address
   temporaries.  Let LRA use BASE_REGS for those temporaries and leave
   final hard-reg legality to the address predicate.  */
#define MODE_CODE_BASE_REG_CLASS(MODE, AS, OUTER, INDEX)	\
  (((INDEX) == REG && !lra_in_progress) ? BASE_INDEX_REGS : BASE_REGS)

/* Validate a specific register number as base, depending on whether
   an index register is present.  */
#define REGNO_MODE_CODE_OK_FOR_BASE_P(REGNO, MODE, AS, OUTER, INDEX)	\
  (((INDEX) == REG)							\
  ? ((REGNO) == BX_REG || (REGNO) == BP_REG				\
    || (REGNO) == AP_REG || (REGNO) == FP_REG			\
      || (unsigned)(REGNO) >= FIRST_PSEUDO_REGISTER)			\
   : REGNO_OK_FOR_BASE_P (REGNO))

#define INDEX_REG_CLASS	INDEX_REGS

#define STACK_POINTER_REGNUM	SP_REG
#define HARD_FRAME_POINTER_REGNUM BP_REG
#define FRAME_POINTER_REGNUM	FP_REG
#define ARG_POINTER_REGNUM	AP_REG
#define STATIC_CHAIN_REGNUM	CX_REG

#define PC_REGNUM		(-1)

/* 1 = register is fixed and cannot be used by the allocator.  */
#define FIXED_REGISTERS						\
{								\
  /* AX DX CX BX SI DI BP SP ES CC AP FP */		\
     0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1		\
}

/* 1 = register is clobbered by function calls.  */
#define CALL_USED_REGISTERS					\
{								\
  /* AX DX CX BX SI DI BP SP ES CC AP FP */		\
     1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1		\
}

/* Allocate caller-saved regs first, then callee-saved.  */
#define REG_ALLOC_ORDER						\
  { AX_REG, DX_REG, CX_REG, BX_REG, SI_REG, DI_REG, BP_REG,	\
  ES_REG, SP_REG, CC_REG, AP_REG, FP_REG }

/* On 8086, only BX, SI, DI, BP can be used as base registers
  for memory addressing.  AP and FP are virtual registers that will be
  eliminated to BP, so they are also valid bases.  */
#define REGNO_OK_FOR_BASE_P(REGNO)				\
  (((unsigned)(REGNO) < FIRST_PSEUDO_REGISTER			\
    && ((REGNO) == BX_REG					\
	|| (REGNO) == SI_REG					\
	|| (REGNO) == DI_REG					\
	|| (REGNO) == BP_REG					\
	|| (REGNO) == AP_REG					\
  || (REGNO) == FP_REG))				\
   || (unsigned)(REGNO) >= FIRST_PSEUDO_REGISTER)

/* On 8086, only SI and DI can be index registers (used with
   BX or BP as base in base+index addressing).  */
#define REGNO_OK_FOR_INDEX_P(REGNO)				\
  (((unsigned)(REGNO) < FIRST_PSEUDO_REGISTER			\
    && ((REGNO) == SI_REG					\
	|| (REGNO) == DI_REG))					\
   || (unsigned)(REGNO) >= FIRST_PSEUDO_REGISTER)

#define REGNO_REG_CLASS(REGNO) ia16_regno_reg_class (REGNO)

/* Map from register class to a letter used in constraints.  */
#define TRAMPOLINE_SIZE		10
#define TRAMPOLINE_ALIGNMENT	16

#define ELIMINABLE_REGS						\
{{ ARG_POINTER_REGNUM, STACK_POINTER_REGNUM },			\
 { ARG_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM },		\
 { FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM },			\
 { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM }}

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET)		\
  (OFFSET) = ia16_initial_elimination_offset ((FROM), (TO))

/* Reload can otherwise end up spending one hard reg on the base and one
   on the index for each `p + x' address.  Allow the backend to collapse
   such sums into one legal base register when that reduces pressure.  */
#define LEGITIMIZE_RELOAD_ADDRESS(X, MODE, OPNUM, TYPE, INDL, WIN)	\
  do									\
    {									\
      if (ia16_legitimize_reload_address (&(X), (MODE), (OPNUM),		\
					  (TYPE), (INDL)))		\
	goto WIN;							\
    }									\
  while (0)

#define FUNCTION_ARG_REGNO_P(N)	 0
#define DEFAULT_PCC_STRUCT_RETURN 1

/* AX:DX pair is used for 32-bit return values.  */
typedef struct
{
  int nregs;		/* Number of registers used so far.  */
  int bytes;		/* Number of bytes used for argument passing.  */
} CUMULATIVE_ARGS;

extern void ia16_init_cumulative_args (CUMULATIVE_ARGS *, tree, rtx, tree, int);

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS) \
  ia16_init_cumulative_args (&(CUM), (FNTYPE), (LIBNAME), (FNDECL), \
			     (N_NAMED_ARGS))

/* --------------------------------------------------------------------------
   Return Value
   -------------------------------------------------------------------------- */

#define INCOMING_RETURN_ADDR_RTX					\
  gen_rtx_MEM (HImode, gen_rtx_REG (HImode, SP_REG))

#define RETURN_ADDR_RTX(COUNT, FRAME)					\
  ((COUNT) == 0 ? INCOMING_RETURN_ADDR_RTX : NULL_RTX)

#define INCOMING_FRAME_SP_OFFSET (TARGET_FAR_CODE ? 4 : 2)

/* --------------------------------------------------------------------------
   Profiling
   -------------------------------------------------------------------------- */

#define NO_PROFILE_COUNTERS	1
#define PROFILE_BEFORE_PROLOGUE	1

#define FUNCTION_PROFILER(FILE, LABELNO)		\
  fprintf (FILE, "\tcall\t_mcount\n");

/* --------------------------------------------------------------------------
   Assembler Format
   -------------------------------------------------------------------------- */

#define TEXT_SECTION_ASM_OP	".text"
#define DATA_SECTION_ASM_OP	".data"
#define BSS_SECTION_ASM_OP	"\t.section .bss"

#define ASM_COMMENT_START	"#"
#define ASM_APP_ON		"#APP\n"
#define ASM_APP_OFF		"#NO_APP\n"
#define LOCAL_LABEL_PREFIX	".L"
#undef  USER_LABEL_PREFIX
#define USER_LABEL_PREFIX	"_"

#define GLOBAL_ASM_OP		"\t.global\t"

/* How to refer to registers in assembler output.  */
#define REGISTER_PREFIX		"%"

#define ASM_OUTPUT_ALIGN(STREAM, LOG)			\
  do							\
    {							\
      if ((LOG) != 0)					\
	fprintf (STREAM, "\t.balign %d\n", 1 << (LOG));\
    }							\
  while (0)

#define JUMP_TABLES_IN_TEXT_SECTION 1

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE) \
  fprintf (FILE, "\t.word .L%d\n", VALUE)

#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL) \
  fprintf (FILE, "\t.word .L%d-.L%d\n", VALUE, REL)

#undef  DWARF2_ADDR_SIZE
/* Use 32-bit DWARF addresses and ranges even though near data pointers are
   16-bit.  Large functions and debug line/aranges tables can easily exceed
   64 KiB, and emitting 16-bit DWARF address-sized fields makes gas reject
   perfectly valid debug info at -g.  */
#define DWARF2_ADDR_SIZE	4

#undef  PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DWARF2_DEBUG

#define DWARF2_ASM_LINE_DEBUG_INFO 1

#define ACCUMULATE_OUTGOING_ARGS 0

/* Use push instructions for argument passing.  The pushhi1 expander
   forces immediates into registers on 8086 (no push-immediate).  */
#define PUSH_ARGS	1
#define PUSH_ROUNDING(BYTES) ia16_push_rounding (BYTES)
#define PUSH_ARGS_REVERSED 1

/* --------------------------------------------------------------------------
   Exception Handling
   -------------------------------------------------------------------------- */

#define EH_RETURN_DATA_REGNO(N) \
  ((N) < 2 ? (N) == 0 ? AX_REG : DX_REG : INVALID_REGNUM)

#define EH_RETURN_STACKADJ_RTX gen_rtx_REG (HImode, CX_REG)

#define ASM_PREFERRED_EH_DATA_FORMAT(CODE, GLOBAL) DW_EH_PE_udata2

/* --------------------------------------------------------------------------
   Misc
   -------------------------------------------------------------------------- */

/* Costs.  */
#define SLOW_BYTE_ACCESS	0
#define NO_FUNCTION_CSE		1

/* The 8086 has a 16-bit data bus; moving 16-bit values is cheap.  */
#define MOVE_RATIO(speed) ((speed) ? 4 : 2)

#endif /* GCC_IA16_H */

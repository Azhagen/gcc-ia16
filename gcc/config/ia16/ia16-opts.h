/* Option-related definitions for the ia16 backend.
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

#ifndef IA16_OPTS_H
#define IA16_OPTS_H

enum ia16_cpu_type
{
  IA16_CPU_8086,
  IA16_CPU_80186,
  IA16_CPU_80286
};

enum ia16_memory_model
{
  IA16_MODEL_TINY,
  IA16_MODEL_SMALL,
  IA16_MODEL_MEDIUM,
  IA16_MODEL_COMPACT,
  IA16_MODEL_LARGE,
  IA16_MODEL_HUGE
};

#endif /* IA16_OPTS_H */

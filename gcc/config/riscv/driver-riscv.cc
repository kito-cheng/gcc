/* Native CPU detection for RISC-V.
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
#define INCLUDE_STRING
#include "system.h"
#include "coretypes.h"
#include "tm.h"

/* -march=native describes the core the compiler is running on, in the same
   way that -march=native on x86 describes whatever core executed CPUID.
   That matters on RISC-V because heterogeneous systems are common: asking
   the kernel about every online CPU makes it answer -1 for mvendorid,
   marchid and mimpid as soon as two of them disagree, which leaves nothing
   to derive -mtune from.  So every query below is restricted to the CPU
   returned by getcpu.  Users who care which core is described should pin
   the compiler with taskset, or spell out -march=.  */

#ifdef __linux__

/* Linux syscall numbers.  These come from the asm-generic ABI, which is
   what RISC-V uses, and are replicated here so that the driver depends
   neither on the kernel headers nor on a glibc new enough to provide
   <sys/hwprobe.h>.  */
#define RISCV_NR_GETCPU			168
#define RISCV_NR_HWPROBE		258

/* hwprobe keys, likewise a stable kernel ABI.  */
#define RISCV_HWPROBE_KEY_BASE_BEHAVIOR	3
#define RISCV_HWPROBE_BASE_BEHAVIOR_IMA	(1ULL << 0)
#define RISCV_HWPROBE_KEY_IMA_EXT_0	4
#define RISCV_HWPROBE_KEY_IMA_EXT_1	16

/* Flags for the fourth field of RISCV_HWPROBE_EXT.  */
#define RISCV_HWPROBE_XLEN32		(1U << 0)

struct riscv_hwprobe
{
  long long key;
  unsigned long long value;
};

/* One entry per extension the kernel can report.  */

struct riscv_hwprobe_ext
{
  const char *name;
  int key;
  int bit;
  unsigned int flags;
};

static const struct riscv_hwprobe_ext riscv_hwprobe_exts[] = {
#define RISCV_HWPROBE_EXT(NAME, KEY, BIT, FLAGS) { NAME, KEY, BIT, FLAGS },
#include "riscv-hwprobe.def"
};

/* Issue a five argument syscall.  Doing this by hand rather than through
   glibc keeps the driver buildable against any libc, and mirrors what
   libgcc/config/riscv/feature_bits.c already does.  */

static long
riscv_syscall_5 (long number, long arg1, long arg2, long arg3, long arg4,
		 long arg5)
{
  register long a7 __asm__ ("a7") = number;
  register long a0 __asm__ ("a0") = arg1;
  register long a1 __asm__ ("a1") = arg2;
  register long a2 __asm__ ("a2") = arg3;
  register long a3 __asm__ ("a3") = arg4;
  register long a4 __asm__ ("a4") = arg5;
  __asm__ __volatile__ ("ecall\n\t"
			: "=r" (a0)
			: "r" (a7), "r" (a0), "r" (a1), "r" (a2), "r" (a3),
			  "r" (a4)
			: "memory");
  return a0;
}

/* Return the number of the CPU this process is currently running on, or -1
   if it cannot be determined.  */

static int
riscv_current_cpu (void)
{
  unsigned int cpu = 0;

  if (riscv_syscall_5 (RISCV_NR_GETCPU, (long) &cpu, 0, 0, 0, 0) != 0)
    return -1;

  return (int) cpu;
}

/* The CPU bitmap handed to hwprobe.  1024 CPUs matches the size glibc uses
   for cpu_set_t, which the kernel is known to accept.  */
#define RISCV_CPU_SET_BITS	(8 * sizeof (unsigned long))
#define RISCV_CPU_SET_WORDS	16

/* Ask the kernel to fill in PAIRS, restricting the query to the CPU we are
   running on.  Return true on success.  */

static bool
riscv_hwprobe (struct riscv_hwprobe *pairs, size_t npairs)
{
  unsigned long cpus[RISCV_CPU_SET_WORDS];
  unsigned long *cpuset = NULL;
  size_t cpusetsize = 0;
  int cpu = riscv_current_cpu ();

  if (cpu >= 0 && (size_t) cpu < RISCV_CPU_SET_WORDS * RISCV_CPU_SET_BITS)
    {
      memset (cpus, 0, sizeof (cpus));
      cpus[cpu / RISCV_CPU_SET_BITS] |= 1UL << (cpu % RISCV_CPU_SET_BITS);
      cpuset = cpus;
      cpusetsize = sizeof (cpus);
    }

  /* With a null CPU set the kernel answers for every online CPU.  The
     bitmask keys are still safe there, since it intersects them, but the
     identification keys degrade to -1.  */
  return riscv_syscall_5 (RISCV_NR_HWPROBE, (long) pairs, (long) npairs,
			  (long) cpusetsize, (long) cpuset, 0) == 0;
}

/* Return the value the kernel filled in for KEY.  The kernel rewrites the
   key of any pair it does not recognise to -1, so a key added after the
   running kernel was built reads back as zero rather than as an error.  */

static unsigned long long
riscv_hwprobe_value (const struct riscv_hwprobe *pairs, size_t npairs,
		     int key)
{
  for (size_t i = 0; i < npairs; i++)
    if (pairs[i].key == key)
      return pairs[i].value;

  return 0;
}

/* Build the ISA string of the CPU we are running on, or return NULL if it
   cannot be determined.  */

static const char *
riscv_native_arch (void)
{
  struct riscv_hwprobe pairs[] = {
    { RISCV_HWPROBE_KEY_BASE_BEHAVIOR, 0 },
    { RISCV_HWPROBE_KEY_IMA_EXT_0, 0 },
    { RISCV_HWPROBE_KEY_IMA_EXT_1, 0 }
  };
  size_t npairs = ARRAY_SIZE (pairs);

  if (!riscv_hwprobe (pairs, npairs))
    return NULL;

  /* Everything below assumes the base ISA behaves as IMA.  Anything else is
     a machine we have no way to describe.  */
  if (!(riscv_hwprobe_value (pairs, npairs, RISCV_HWPROBE_KEY_BASE_BEHAVIOR)
	& RISCV_HWPROBE_BASE_BEHAVIOR_IMA))
    return NULL;

  /* The driver runs on the machine it is describing, so its own XLEN is
     the one to report.  */
  const int xlen = __riscv_xlen;
  std::string isa = xlen == 32 ? "rv32ima" : "rv64ima";

  for (size_t i = 0; i < ARRAY_SIZE (riscv_hwprobe_exts); i++)
    {
      const struct riscv_hwprobe_ext *ext = &riscv_hwprobe_exts[i];
      unsigned long long value;

      /* Skip extensions that do not exist for this XLEN.  A kernel should
	 not report them, but -march= rejects them outright, so a stray bit
	 must not be allowed to break -march=native entirely.  */
      if ((ext->flags & RISCV_HWPROBE_XLEN32) != 0 && xlen != 32)
	continue;

      value = riscv_hwprobe_value (pairs, npairs, ext->key);
      if (value & (1ULL << ext->bit))
	{
	  isa += '_';
	  isa += ext->name;
	}
    }

  return xstrdup (isa.c_str ());
}

#else /* !__linux__ */

static const char *
riscv_native_arch (void)
{
  return NULL;
}

#endif /* __linux__ */

/* Implement the local_cpu_detect spec function.  ARGV[0] selects what to
   report: "arch" for an -march= option, "tune" for an -mtune= option.
   Returning NULL leaves the command line alone, which makes the compiler
   fall back on whatever it was configured with.  */

const char *
host_detect_local_cpu (int argc, const char **argv)
{
  if (argc < 1 || argv[0] == NULL)
    return NULL;

  if (strcmp (argv[0], "arch") == 0)
    {
      const char *isa = riscv_native_arch ();

      if (isa == NULL)
	return NULL;

      return concat ("-march=", isa, NULL);
    }

  return NULL;
}

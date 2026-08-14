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
#define RISCV_HWPROBE_KEY_MVENDORID		0
#define RISCV_HWPROBE_KEY_MARCHID		1
#define RISCV_HWPROBE_KEY_MIMPID		2
#define RISCV_HWPROBE_KEY_BASE_BEHAVIOR		3
#define RISCV_HWPROBE_BASE_BEHAVIOR_IMA		(1ULL << 0)
#define RISCV_HWPROBE_KEY_IMA_EXT_0		4
#define RISCV_HWPROBE_KEY_ZICBOZ_BLOCK_SIZE	6
#define RISCV_HWPROBE_KEY_ZICBOM_BLOCK_SIZE	12
#define RISCV_HWPROBE_KEY_IMA_EXT_1		16

/* The handful of IMA_EXT_0 bits this file needs to test directly rather
   than just turn into an extension name.  */
#define RISCV_HWPROBE_IMA_V			(1ULL << 2)
#define RISCV_HWPROBE_EXT_ZICBOZ		(1ULL << 6)
#define RISCV_HWPROBE_EXT_ZVE32X		(1ULL << 37)
#define RISCV_HWPROBE_EXT_ZVE64X		(1ULL << 39)
#define RISCV_HWPROBE_EXT_ZICBOM		(1ULL << 55)

/* Any of these means the machine can execute vector instructions.  */
#define RISCV_HWPROBE_ANY_VECTOR					\
  (RISCV_HWPROBE_IMA_V | RISCV_HWPROBE_EXT_ZVE32X			\
   | RISCV_HWPROBE_EXT_ZVE64X)

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

/* One entry per core we can recognise from its identification registers.  */

struct riscv_core_id
{
  const char *name;
  unsigned long long mvendorid;
  unsigned long long marchid;
  unsigned long long mimpid;
};

/* Matches whatever the hardware reports for that register.  */
#define RISCV_CORE_ID_ANY (~0ULL)

static const struct riscv_core_id riscv_core_ids[] = {
#define RISCV_CORE_ID(NAME, MVENDORID, MARCHID, MIMPID)			\
  { NAME, MVENDORID, MARCHID, MIMPID },
#include "riscv-cores.def"
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

/* Append the Zvl extension describing this machine's vector register
   width to ISA.  Neither hwprobe nor /proc/cpuinfo reports VLEN, so the
   vlenb CSR is the only place it can be read.  Doing so needs the vector
   unit enabled, which Linux only does lazily, but its illegal instruction
   handler recognises a read of CSR_VLENB and enables it, so having
   established that the machine has vectors at all is guard enough.  */

static void
riscv_add_vlen (std::string &isa)
{
  unsigned long vlenb;
  unsigned long vlen;
  char buf[32];

  /* Spelled numerically because not every assembler knows the name.  */
  __asm__ volatile ("csrr %0, 0xc22" : "=r" (vlenb));

  vlen = vlenb * 8;

  /* A valid VLEN is a power of two, at least 32 and at most 65536.  */
  if (vlen < 32 || vlen > 65536 || (vlen & (vlen - 1)) != 0)
    return;

  snprintf (buf, sizeof (buf), "_zvl%lub", vlen);
  isa += buf;
}

/* Build the ISA string of the CPU we are running on, or return NULL if it
   cannot be determined.  */

static const char *
riscv_native_arch (void)
{
  struct riscv_hwprobe pairs[] = {
    { RISCV_HWPROBE_KEY_BASE_BEHAVIOR, 0 },
    { RISCV_HWPROBE_KEY_IMA_EXT_0, 0 },
    { RISCV_HWPROBE_KEY_IMA_EXT_1, 0 },
    { RISCV_HWPROBE_KEY_ZICBOZ_BLOCK_SIZE, 0 },
    { RISCV_HWPROBE_KEY_ZICBOM_BLOCK_SIZE, 0 }
  };
  size_t npairs = ARRAY_SIZE (pairs);
  unsigned long long ima0;
  bool have_zicboz, have_zicbom, zic64b;

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

  ima0 = riscv_hwprobe_value (pairs, npairs, RISCV_HWPROBE_KEY_IMA_EXT_0);

  /* Zic64b promises that every cache block acted on by Zicbom, Zicbop and
     Zicboz is 64 bytes wide, which is what lets memset expand to cbo.zero.
     The kernel reports the two block sizes it knows about separately, so
     claim Zic64b only when each one that applies really is 64.  */
  have_zicboz = (ima0 & RISCV_HWPROBE_EXT_ZICBOZ) != 0;
  have_zicbom = (ima0 & RISCV_HWPROBE_EXT_ZICBOM) != 0;
  zic64b = have_zicboz || have_zicbom;

  if (have_zicboz
      && riscv_hwprobe_value (pairs, npairs,
			      RISCV_HWPROBE_KEY_ZICBOZ_BLOCK_SIZE) != 64)
    zic64b = false;
  if (have_zicbom
      && riscv_hwprobe_value (pairs, npairs,
			      RISCV_HWPROBE_KEY_ZICBOM_BLOCK_SIZE) != 64)
    zic64b = false;

  if (zic64b)
    isa += "_zic64b";

  if (ima0 & RISCV_HWPROBE_ANY_VECTOR)
    riscv_add_vlen (isa);

  return xstrdup (isa.c_str ());
}

/* Return the -mtune= option naming the core we are running on, or NULL if
   it is not one we have hardware identification for.  */

static const char *
riscv_native_tune (void)
{
  struct riscv_hwprobe pairs[] = {
    { RISCV_HWPROBE_KEY_MVENDORID, 0 },
    { RISCV_HWPROBE_KEY_MARCHID, 0 },
    { RISCV_HWPROBE_KEY_MIMPID, 0 }
  };
  size_t npairs = ARRAY_SIZE (pairs);
  unsigned long long mvendorid, marchid, mimpid;

  if (!riscv_hwprobe (pairs, npairs))
    return NULL;

  mvendorid = riscv_hwprobe_value (pairs, npairs,
				   RISCV_HWPROBE_KEY_MVENDORID);
  marchid = riscv_hwprobe_value (pairs, npairs, RISCV_HWPROBE_KEY_MARCHID);
  mimpid = riscv_hwprobe_value (pairs, npairs, RISCV_HWPROBE_KEY_MIMPID);

  /* The kernel answers -1 for a register it cannot pin down to one value,
     which is what happens when the query spans cores that disagree.  It
     should not happen for the single CPU asked about here, but an
     all-ones vendor or architecture ID does not name a real core either.  */
  if (mvendorid == ~0ULL || marchid == ~0ULL)
    return NULL;

  for (size_t i = 0; i < ARRAY_SIZE (riscv_core_ids); i++)
    {
      const struct riscv_core_id *core = &riscv_core_ids[i];

      if (core->mvendorid != RISCV_CORE_ID_ANY
	  && core->mvendorid != mvendorid)
	continue;
      if (core->marchid != RISCV_CORE_ID_ANY && core->marchid != marchid)
	continue;
      if (core->mimpid != RISCV_CORE_ID_ANY && core->mimpid != mimpid)
	continue;

      return concat ("-mtune=", core->name, NULL);
    }

  return NULL;
}

#else /* !__linux__ */

static const char *
riscv_native_arch (void)
{
  return NULL;
}

static const char *
riscv_native_tune (void)
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

  if (strcmp (argv[0], "tune") == 0)
    return riscv_native_tune ();

  return NULL;
}

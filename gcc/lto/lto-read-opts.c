/* Language-dependent hooks for LTO.
   Copyright (C) 2009-2014 Free Software Foundation, Inc.
   Contributed by CodeSourcery, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "flags.h"
#include "tm.h"
#include "tree.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "target.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "debug.h"
#include "opts.h"
#include "lto-tree.h"
#include "lto.h"
#include "tree-inline.h"
#include "basic-block.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimple.h"
#include "diagnostic-core.h"
#include "toplev.h"
#include "lto-streamer.h"
#include "cilk.h"
#include "simple-object.h"
#include "lto-section-names.h"

static void
decode_option_string (const char *options,
		      struct cl_decoded_option **decoded_options,
		      unsigned int *decoded_options_count)
{
  struct obstack argv_obstack;
  char *argv_storage;
  const char **argv;
  int j, k, argc;

  argv_storage = xstrdup (options);
  obstack_init (&argv_obstack);
  /* First option will ignore, just stub a empty string.  */
  obstack_ptr_grow (&argv_obstack, "");

  for (j = 0, k = 0; argv_storage[j] != '\0'; ++j)
    {
      if (argv_storage[j] == '\'')
	{
	  obstack_ptr_grow (&argv_obstack, &argv_storage[k]);
	  ++j;
	  do
	    {
	      if (argv_storage[j] == '\0')
		fatal_error ("malformed COLLECT_GCC_OPTIONS");
	      else if (strncmp (&argv_storage[j], "'\\''", 4) == 0)
		{
		  argv_storage[k++] = '\'';
		  j += 4;
		}
	      else if (argv_storage[j] == '\'')
		break;
	      else
		argv_storage[k++] = argv_storage[j++];
	    }
	  while (1);
	  argv_storage[k++] = '\0';
	}
    }

  obstack_ptr_grow (&argv_obstack, NULL);
  argc = obstack_object_size (&argv_obstack) / sizeof (void *) - 1;
  argv = XOBFINISH (&argv_obstack, const char **);

  decode_cmdline_options_to_array (argc, (const char **)argv,
				   CL_LANG_ALL | CL_TARGET | CL_COMMON,
//				   CL_LANG_ALL,
				   decoded_options, decoded_options_count);
  obstack_free (&argv_obstack, NULL);

}

/* Append OPTION to the options array DECODED_OPTIONS with size
   DECODED_OPTIONS_COUNT.  */

static void
append_option (struct cl_decoded_option **decoded_options,
	       unsigned int *decoded_options_count,
	       struct cl_decoded_option *option)
{
  ++*decoded_options_count;
  *decoded_options
    = (struct cl_decoded_option *)
	xrealloc (*decoded_options,
		  (*decoded_options_count
		   * sizeof (struct cl_decoded_option)));
  memcpy (&(*decoded_options)[*decoded_options_count - 1], option,
	  sizeof (struct cl_decoded_option));
}

/* Try to merge and complain about options FDECODED_OPTIONS when applied
   ontop of DECODED_OPTIONS.  */

static void
merge_and_complain (struct cl_decoded_option **decoded_options,
		    unsigned int *decoded_options_count,
		    struct cl_decoded_option *fdecoded_options,
		    unsigned int fdecoded_options_count)
{
  unsigned int i, j;

  /* ???  Merge options from files.  Most cases can be
     handled by either unioning or intersecting
     (for example -fwrapv is a case for unioning,
     -ffast-math is for intersection).  Most complaints
     about real conflicts between different options can
     be deferred to the compiler proper.  Options that
     we can neither safely handle by intersection nor
     unioning would need to be complained about here.
     Ideally we'd have a flag in the opt files that
     tells whether to union or intersect or reject.
     In absence of that it's unclear what a good default is.
     It's also difficult to get positional handling correct.  */

  /* The following does what the old LTO option code did,
     union all target and a selected set of common options.  */
  for (i = 0; i < fdecoded_options_count; ++i)
    {
      struct cl_decoded_option *foption = &fdecoded_options[i];
      switch (foption->opt_index)
	{
	case OPT_SPECIAL_unknown:
	case OPT_SPECIAL_ignore:
	case OPT_SPECIAL_program_name:
	case OPT_SPECIAL_input_file:
	  break;

	/* Don't write out it since it's used for WAP mode only.  */
	case OPT_fresolution_:
	case OPT_fltrans_output_list_:
	  break;

	default:
	  if (!(cl_options[foption->opt_index].flags & CL_TARGET | CL_COMMON))
	    break;

	  /* Fallthru.  */
	case OPT_fPIC:
	case OPT_fpic:
	case OPT_fPIE:
	case OPT_fpie:
	case OPT_fcommon:
	case OPT_fexceptions:
	case OPT_fnon_call_exceptions:
	case OPT_fgnu_tm:
	  /* Do what the old LTO code did - collect exactly one option
	     setting per OPT code, we pick the first we encounter.
	     ???  This doesn't make too much sense, but when it doesn't
	     then we should complain.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  break;

	case OPT_ftrapv:
	case OPT_fstrict_overflow:
	case OPT_ffp_contract_:
	  /* For selected options we can merge conservatively.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  /* FP_CONTRACT_OFF < FP_CONTRACT_ON < FP_CONTRACT_FAST,
	     -fno-trapv < -ftrapv,
	     -fno-strict-overflow < -fstrict-overflow  */
	  else if (foption->value < (*decoded_options)[j].value)
	    (*decoded_options)[j] = *foption;
	  break;

	case OPT_fwrapv:
	  /* For selected options we can merge conservatively.  */
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  /* -fwrapv > -fno-wrapv.  */
	  else if (foption->value > (*decoded_options)[j].value)
	    (*decoded_options)[j] = *foption;
	  break;

	case OPT_freg_struct_return:
	case OPT_fpcc_struct_return:
	case OPT_fshort_double:
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == foption->opt_index)
	      break;
	  if (j == *decoded_options_count)
	    fatal_error ("Option %s not used consistently in all LTO input"
			 " files", foption->orig_option_with_args_text);
	  break;

	case OPT_O:
	case OPT_Ofast:
	case OPT_Og:
	case OPT_Os:
	  for (j = 0; j < *decoded_options_count; ++j)
	    if ((*decoded_options)[j].opt_index == OPT_O
		|| (*decoded_options)[j].opt_index == OPT_Ofast
		|| (*decoded_options)[j].opt_index == OPT_Og
		|| (*decoded_options)[j].opt_index == OPT_Os)
	      break;
	  if (j == *decoded_options_count)
	    append_option (decoded_options, decoded_options_count, foption);
	  else if ((*decoded_options)[j].opt_index == foption->opt_index
		   && foption->opt_index != OPT_O)
	    /* Exact same options get merged.  */
	    ;
	  else
	    {
	      /* For mismatched option kinds preserve the optimization
	         level only, thus merge it as -On.  This also handles
		 merging of same optimization level -On.  */
	      int level = 0;
	      switch (foption->opt_index)
		{
		case OPT_O:
		  if (foption->arg[0] == '\0')
		    level = MAX (level, 1);
		  else
		    level = MAX (level, atoi (foption->arg));
		  break;
		case OPT_Ofast:
		  level = MAX (level, 3);
		  break;
		case OPT_Og:
		  level = MAX (level, 1);
		  break;
		case OPT_Os:
		  level = MAX (level, 2);
		  break;
		default:
		  gcc_unreachable ();
		}
	      switch ((*decoded_options)[j].opt_index)
		{
		case OPT_O:
		  if ((*decoded_options)[j].arg[0] == '\0')
		    level = MAX (level, 1);
		  else
		    level = MAX (level, atoi ((*decoded_options)[j].arg));
		  break;
		case OPT_Ofast:
		  level = MAX (level, 3);
		  break;
		case OPT_Og:
		  level = MAX (level, 1);
		  break;
		case OPT_Os:
		  level = MAX (level, 2);
		  break;
		default:
		  gcc_unreachable ();
		}
	      (*decoded_options)[j].opt_index = OPT_O;
	      char *tem;
	      asprintf (&tem, "-O%d", level);
	      (*decoded_options)[j].arg = &tem[2];
	      (*decoded_options)[j].canonical_option[0] = tem;
	      (*decoded_options)[j].value = 1;
	    }
	  break;
	}
    }
}

#define DEBUG
#undef DEBUG

void debug_options (struct cl_decoded_option *opts, unsigned int optcount)
{
  unsigned i, j;
  fprintf (stderr, "Options:\n");
  for (i = 1; i < optcount; ++i)
    {
      struct cl_decoded_option *option = &opts[i];
      for (j = 0; j < option->canonical_option_num_elements; ++j)
	fprintf (stderr, "%s ", option->canonical_option[j]);
      fprintf (stderr, "  error : %d\n", option->errors);
    }
  fprintf (stderr, "\n");

}

extern void process_options (void);
void read_option_from_files (unsigned nfiles, const char **fnames)
{
  unsigned i;
#ifdef DEBUG
  unsigned j;
  const char *pfx = (flag_lto) ? "LTO" : (flag_wpa) ? "WPA" : "LTRANS";
#endif
  //gcc_assert(flag_lto || flag_wpa);
#ifdef DEBUG
  fprintf (stderr, "Now in %s mode\n", pfx);
#endif

  struct cl_decoded_option *final_decoded_options = NULL;
  unsigned final_decoded_options_count;

  for (i = 0; i < nfiles; ++i)
    {
      int fd;
      simple_object_read *sobj;
      off_t file_offset = 0; /* Need to handle obj in *.a  */
      off_t offset, length;
      const char *errmsg;
      int err;
      char *data, *fopts;
      struct cl_decoded_option *decoded_options;
      unsigned decoded_options_count;

      fd = open (fnames[i], O_RDONLY);

      if (fd == -1)
	abort();

      sobj = simple_object_start_read (fd, file_offset, "__GNU_LTO",
    				   &errmsg, &err);

      if (!sobj)
	{
	  close (fd);
	  return;
	}

      if (!simple_object_find_section (sobj, LTO_SECTION_NAME_PREFIX "." "opts",
    				   &offset, &length, &errmsg, &err))
	{
	  simple_object_release_read (sobj);
	  close (fd);
	  return;
	}
      lseek (fd, file_offset + offset, SEEK_SET);
      data = (char *)xmalloc (length);
      read (fd, data, length);
      fopts = data;
      decode_option_string (fopts, &decoded_options, &decoded_options_count);

      if (!final_decoded_options)
	{
	  final_decoded_options = decoded_options;
	  final_decoded_options_count = decoded_options_count;
	}
      else
	merge_and_complain (&final_decoded_options,
			    &final_decoded_options_count,
			    decoded_options,
			    decoded_options_count);
#ifdef DEBUG
      fprintf (stderr, "Option for `%s` : \"%s\"\n", fnames[i], fopts);
#endif
    }
#ifdef DEBUG
  fprintf (stderr, "Saved Option:\n");
  for (i = 1; i < save_decoded_options_count; ++i)
    {
      struct cl_decoded_option *option = &save_decoded_options[i];
      for (j = 0; j < option->canonical_option_num_elements; ++j)
	fprintf (stderr, "%s ", option->canonical_option[j]);
    }
  fprintf (stderr, "\n");
#endif

#ifdef DEBUG
  fprintf (stderr, "Final Option:\n");
  for (i = 1; i < final_decoded_options_count; ++i)
    {
      struct cl_decoded_option *option = &final_decoded_options[i];
      for (j = 0; j < option->canonical_option_num_elements; ++j)
	fprintf (stderr, "%s ", option->canonical_option[j]);
    }
  fprintf (stderr, "\n");
#endif

  merge_and_complain (
    &final_decoded_options,
    &final_decoded_options_count,
    save_decoded_options,
    save_decoded_options_count);
#ifdef DEBUG
  fprintf (stderr, "Final Option merge with cmdline:\n");
  for (i = 1; i < final_decoded_options_count; ++i)
    {
      struct cl_decoded_option *option = &final_decoded_options[i];
      for (j = 0; j < option->canonical_option_num_elements; ++j)
	fprintf (stderr, "%s ", option->canonical_option[j]);
      fprintf (stderr, "  error : %d\n", option->errors);
    }
  fprintf (stderr, "\n");
#endif
  if (flag_wpa)
    lto_set_write_options (final_decoded_options, final_decoded_options_count);

  decode_options (&global_options, &global_options_set,
		  final_decoded_options, final_decoded_options_count,
		  UNKNOWN_LOCATION, global_dc);
  handle_common_deferred_options ();
  process_options ();
}

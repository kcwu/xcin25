/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define if the debug option is enabled. */
#undef DEBUG

/* Define for building the dynamic loadable IM-modules */
#undef USE_DYNAMIC

/* Define for dlopen() function (ELF) */
#undef HAVE_DLOPEN

/* Define for shl_load() function (HP-UX) */
#undef HAVE_SHL_LOAD

/* Define for gettext() function */
#undef HAVE_GETTEXT

/* Define for iconv() function */
#undef HAVE_ICONV

/* Define for XCIN_DEFAULT_DIR variable */
#undef XCIN_DEFAULT_DIR

/* Define for XCIN_DEFAULT_RCDIR variable */
#undef XCIN_DEFAULT_RCDIR

/* Define for XCIN_MSGLOCAT variable */
#undef XCIN_MSGLOCAT

/* Define for Linux system */
#undef LINUX

/* Define for FreeBSD system */
#undef FREEBSD

/* Define for OpenBSD system */
#undef OPENBSD

/* Define for NETBSD system */
#undef NETBSD

/* Define for Solaris system */
#undef SOLARIS

/* Define for HP-UX system */
#undef HPUX

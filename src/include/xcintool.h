/*
    Copyright (C) 1999 by  XCIN TEAM

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    For any question or suggestion, please mail to xcin mailing-list:
    xcin@linux.org.tw, or the maintainer Tung-Han Hsieh: thhsieh@linux.org.tw


			**** ADDITIONAL NOTE ****

    This file, xcintool.h, defines the interface to the xcintool library,
    which provides utilities to make the programming of xcin to be more
    convenient and the behavior of the components of the xcin system are
    more consistant. Although this file, as a whole, is distributed under
    the terms of GPL (GNU General Public License) with xcin, everyone is 
    freely/optionally to adopt the contents there if he/she wants to write 
    a new XCIN MODULE for xcin. The adoption of any part of the contents 
    below will not affect the license terms of your program.

    Please note that the freedom of adoption of this file DOES NOT mean
    that you can statically link your module with ANY functions declared
    there if your module is a proprietary (not under GPL, LGPL, BSD, or other
    free software license terms) program. In fact, this is not necessary to 
    do any links, since when xcin loads your module and execute it, the 
    functions your module calls will automatically linked by the system. So 
    please don't worry about any statically or dynamically linking problem. 
    Just compile your module to be a shared library (with suffix .so) and 
    everything will go.
*/      


#ifndef _XCINTOOL_H
#define _XCINTOOL_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global status variables. */
extern int verbose, errstatus;

#if !defined(True) || !defined(False)
#undef  True
#undef  False
#define True  1
#define False 0
#endif

/* Integer types. */
typedef unsigned int	xmode_t;
typedef signed char	x_int8;
typedef unsigned char	x_uint8;

#if (SIZEOF_SHORT == 2)
typedef short		x_int16;
typedef unsigned short	x_uint16;
#else
#if (SIZEOF_INT == 2)
typedef int		x_int16;
typedef unsigned int	x_uint16;
#endif
#endif

#if (SIZEOF_INT == 4)
typedef int		x_int32;
typedef unsigned int	x_uint32;
#else
#if (SIZEOF_LONG == 4)
typedef long		x_int32;
typedef unsigned long	x_uint32;
#else
#if (SIZEOF_SHORT == 4)
typedef short		x_int32;
typedef unsigned short	x_uint32;
#endif
#endif
#endif

/* For general message level. */
#define XCINMSG_NORMAL		 0		/* normal		*/
#define XCINMSG_WARNING		 1		/* warning		*/
#define XCINMSG_IWARNING	 2		/* internal warnning	*/
#define XCINMSG_ERROR		-1		/* error		*/
#define XCINMSG_IERROR		-2		/* internal error	*/
#define XCINMSG_EMPTY		 3		/* pure message printed */

/* For international message output (gettext) */
#ifdef HAVE_GETTEXT
#  include <libintl.h>
#  define _(STRING) gettext(STRING)
#else
#  define _(STRING) STRING
#endif
#ifndef N_
#  define N_(STRING) STRING
#endif


/* General char type: xcin internal representation. */
#define N_ENCODING_ENTRY	256
#define N_XCH_SLEN		65535

#define XCH_TYPE_MBYTES		0		
#define XCH_TYPE_WUCS4		1

typedef struct {
    x_uint8  type;		/* character representation type */
    x_uint8  encid;		/* encoding id */
    x_uint16 len;		/* string length */
    union {
	char    *s;		/* pointer to multi-bytes string */
	wchar_t *w;		/* pointer to wide-character string */
    } c;
} xch_t;

/*
 *  Reference:  CJKV Information Processing
 *              author:         Ken Lunde
 *              publisher:      O'Reilly, 1999
 *              ISBN:           1-56592-224-7
 */
enum charset_type {
    CHSET_MODAL,                /* ISO-2022 */
    CHSET_NOMODAL,              /* Big5, GB2312, EUC, Shift-JIS, UTF-8 */
    CHSET_FIXLEN                /* UCS2, UCS4 */
};

#define N_ASCII                 95
#define UINT32BIT               0x80000000

typedef struct {
    char *subset_name;          /* sub-charset names */
    int n_char;                 /* # of chars in this sub-charset */
    int mblen;                  /* max # of bytes in multi-byte format */
    int code_start;             /* coding start into xcin char system */
} sub_csdata_t;

typedef struct {
    char *charset_name;         /* charset name */
    int charset_type;           /* type of the charset */
    int n_subset;               /* # of sub-charsets */
    sub_csdata_t *sch;          /* sub-charset list */

    int n_char;                 /* total # of chars */
    int n_chcoded;              /* total # of coded chars */
    int max_mblen;              /* max mblen of all the sub-charset */

    xch_t imname_en;            /* English input method name */
    xch_t imname_sb;            /* Single-byte input mode name */
    xch_t imname_wc;            /* Wide-char input mode name */
    xch_t wc_ascii;		/* Wide-char ascii list */
} csdata_t;


/* File type for check_file_exist(); */
enum ftype {
    FTYPE_FILE,			/* Regular file */
    FTYPE_DIR,			/* Directory */
    FTYPE_NONE
};

/* Input data type for set_data(); */
enum rctype {
    RC_BFLAG,
    RC_SFLAG,
    RC_IFLAG,
    RC_LFLAG,
    RC_BYTE,
    RC_UBYTE,
    RC_SHORT,
    RC_USHORT,
    RC_INT,
    RC_UINT,
    RC_LONG,
    RC_ULONG,
    RC_FLOAT,
    RC_DOUBLE,
    RC_STRING,
    RC_STRARR,
    RC_NONE
};

/* XCIN global resources. */
typedef struct {
    char *lc_ctype;
    char *lc_messages;
    char *encoding;
    int encid, locid;
} locale_t;

typedef struct {
    int argc;			/* Command line arguement list */
    char **argv;
    locale_t locale;            /* Locale name. */
    char *usrhome;		/* User home directory. */
    char *default_dir;          /* Default module directory. */
    char *user_dir;             /* User data directory. */
    char *rcfile;               /* rcfile name. */
    void *rclist;		/* resource list read from the rcfile */
} xcin_rc_t;

/* 
 * General module definition.
 */
enum mtype {
    MTYPE_IM			/* IM module */
};

#define MODULE_ID_SIZE 20	/* module id buffer size */
typedef struct {		/* common module header */
    int module_type;
    char *name;
    char *version;
    char *comments;
} mod_header_t;


/* 
 * Replacement of the standard libc functions.
 */
#ifdef HAVE_MERGESORT
#  define stable_sort  mergesort
#else
#  define stable_sort  xcin_mergesort
   extern void xcin_mergesort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
#endif

#ifndef HAVE_SNPRINTF
#  define snprintf xcin_snprintf
   extern int xcin_snprintf(char *str, size_t size, const char *format, ...);
#endif

#define DebugLog(defverb, action) \
	if (defverb <= verbose) perr_debug action

/* xcintool functions. */
extern void set_perr(char *error_head);
extern void perr(int exitcode, const char *fmt, ...);
extern void perr_debug(const char *fmt, ...);
extern void *xcin_malloc(size_t n_bytes, int reset);
extern void *xcin_realloc(void *pt, size_t n_bytes);
extern int set_lc_ctype(char *loc_name, char *loc_return, int loc_size,
		char *enc_return, int enc_size, int exitcode);
extern int set_lc_messages(char *loc_name, char *loc_return, int loc_size);
extern int set_lc_ctype_env(char *loc_name, char *loc_return, int loc_size, 
		char *enc_return, int enc_size, int exitcode);

extern FILE *open_file(char *fn, char *md, int exitcode);
extern int check_file_exist(char *path, int type);
extern int check_datafile(char *fn, char *sub_path,
		xcin_rc_t *xrc, char *true_fn, int true_fnsize);
extern void check_xcin_path(xcin_rc_t *xrc, int exitcode);
extern mod_header_t *load_module(char *modname, int mod_type,
		char *version, xcin_rc_t *xrc, char *sub_path);
extern void unload_module(mod_header_t *imodule);
extern void module_comment(mod_header_t *modp, char *mod_name);

extern void read_xcinrc(xcin_rc_t *xrc, char *rcfile);
extern int get_resource(xcin_rc_t *xrc, char **cmd_list, char *value,
		int v_size, int n_cmd_list);
extern int get_resource_long(xcin_rc_t *xrc, char **cmd_list, char *value,
		int v_size, int n_cmd_list, int sep);

extern int get_line(char *str, int str_size, FILE *f, int *lineno, 
		char *ignore_ch);
extern int get_word(char **line, char *word, int word_size, char *token);
extern void set_data(void *ref, int type, char *value, 
		unsigned long flag_mask, int bufsize);
extern int strcmp_wild(char *s1, char *s2);

extern int xcin_newenc(char *encoding, xcin_rc_t *xrc);
extern int xcin_enc2id(char *encoding);
extern char *xcin_id2enc(int encid);
extern void xcin_enclist_clean(void);
extern int connect_iconv(int encid, int to_wch);
extern int xcin_wcs2mbs(xch_t *mbs, xch_t *wcs);
extern int xcin_mbs2wcs(xch_t *wcs, xch_t *mbs, int wcstype);


/*
 * The following is absolute .....
 */

/* General char type: mbs encoding
 *
 * Note: In Linux, if wch_t.s = "a1 a4", then wch_t.wch = 0xa4a1, i.e.,
 *       the order reversed. This might not be the general case for all
 *       plateforms.
 */
#ifndef WCH_SIZE
#define WCH_SIZE  4
typedef union { 
    unsigned char s[WCH_SIZE];
    wchar_t wch;
} wch_t;
#endif

typedef signed char byte_t;
typedef unsigned char ubyte_t;
typedef unsigned short xtype_t;

int wchs_to_mbs(char *mbs, wch_t *wchs, int size);
int nwchs_to_mbs(char *mbs, wch_t *wchs, int n_wchs, int size);

#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif

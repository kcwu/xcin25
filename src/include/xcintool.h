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


#ifndef _TOOLFUNC_H
#define _TOOLFUNC_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* For general message level. */
#define XCINMSG_NORMAL		 0		/* normal		*/
#define XCINMSG_WARNING		 1		/* warning		*/
#define XCINMSG_IWARNING	 2		/* internal warnning	*/
#define XCINMSG_ERROR		-1		/* error		*/
#define XCINMSG_IERROR		-2		/* internal error	*/
#define XCINMSG_EMPTY		 3		/* pure message printed */

#ifndef N_
#define N_(msg)	(msg)
#endif

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
typedef unsigned int xmode_t;
typedef unsigned short xtype_t;

/* Choose a suitable sorting function */
#ifdef HAVE_MERGESORT
#  define stable_sort  mergesort
#else
#  define stable_sort  my_merge_sort
   extern void my_merge_sort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
#endif

#ifndef HAVE_SNPRINTF
#  define snprintf xcin_snprintf
   extern int xcin_snprintf(char *str, size_t size, const char *format, ...);
#endif


/* File type for check_file_exist(); */
enum ftype {
    FTYPE_FILE,
    FTYPE_DIR,
    NONE
};

/* Input data type  */
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

#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

extern void set_perr(char *error_head);
extern void perr(int exitcode, const char *fmt,...);
extern void locale_setting(char **lc_ctype, char **lc_messages, 
		char **encoding, int exitcode);

extern FILE *open_file(char *fn, char *md, int exitcode);
extern void set_open_data(char *default_path, char *user_path,
		char *default_sub_path, char *locale_sub_path);
extern FILE *open_data(char *fn, char *md, char *sub_path,
		char *true_fn, int true_size, int exitcode);
extern void copy_file(char *fn1, char *fn2, int exitcode);
extern int check_file_exist(char *path, int type);
extern int check_version(char *vaild_version, char *version, int const_str);

extern int get_line(char *str, int str_size, FILE *f, int *lineno, 
		char *ignore_ch);
extern int get_word(char **line, char *word, int word_size, char *token);
extern void set_data(void *ref, int type, char *value, 
		unsigned long flag_mask, int bufsize);
extern int strcmp_wild(char *s1, char *s2);
extern int wchs_to_mbs(char *mbs, wch_t *wchs, int size);
extern int nwchs_to_mbs(char *mbs, wch_t *wchs, int n_wchs, int size);
extern int wchs_len(wch_t *wchs);
extern int wch_mblen(wch_t *wch);
extern void extract_char(char *s, char *buf, int buflen);

extern void read_resource(char *rc_fn);
extern char *read_xcinrc(char *rcfn, char *user_home);
extern int get_resource(char **cmd_list, char *value, int v_size, 
		int n_cmd_list);
extern int get_resource_long(char **cmd_list, char *value, int v_size, 
		int n_cmd_list, int sep);

extern void DebugLog(int deflevel, int inplevel, char *fmt, ...);

#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif

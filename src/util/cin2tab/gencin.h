#define KCWU
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
    xcin@linux.org.tw.
*/      

#ifndef _GENCIN_H
#define _GENCIN_H

#include <stdlib.h>
#include "constant.h"
#include "module.h"
/* KhoGuan rev
#define GENCIN_VERSION  "20000827" */
#ifdef KCWU
#define GENCIN_VERSION  "20040928"
#else
#define GENCIN_VERSION  "20040422"
#endif

/* For input-code char definition. */
#ifdef KCWU
typedef unsigned int	icode_t;
#else
/* KhoGuan rev
typedef unsigned int	icode_t;
*/
typedef unsigned long long	icode_t;
#endif
/* KhoGuan rev
typedef unsigned short	icode_idx_t;
*/
typedef int		icode_idx_t;  /* linear codepoint for a zi or tsi */
typedef icode_idx_t 	ichar_t;      /* index into icode1/icode2 array */

#define VERLEN			20	/* Version buffer size */
#define ENCLEN			15	/* Encoding name buffer size */
#define CIN_ENAME_LENGTH	20      /* English name buffer size */
#define CIN_CNAME_LENGTH	20      /* Chinese name buffer size */
#ifdef KCWU
#define ICODE_IDX_NOTEXIST	65535
#else
/* KhoGuan rev
#define ICODE_IDX_NOTEXIST	65535 */
#define ICODE_IDX_NOTEXIST	2147483647	
#endif

#ifdef KCWU
#else
#define ICODE_MODE1		1	/* one icode & one icode_idx */
#define ICODE_MODE2		2	/* two icode & one icode_idx */
#endif

#ifdef KCWU
#define  INP_CODE_LENGTH        20 	/* max # of keys in a keystroke */
#define  MAX_ICODE_MODE		((INP_CODE_LENGTH+4)/5)
#else
/* KhoGuan rev
#define  INP_CODE_LENGTH        10 */	/* max # of keys in a keystroke */
#define  INP_CODE_LENGTH        20 	/* max # of keys in a keystroke */
#endif
#define  END_KEY_LENGTH         10      /* max # of end keys */

/* KhoGuan add */
#include<limits.h>
typedef signed char tsisize_t;        /* max length of a "tsi" is 127 "zi" */
/*#define  MAX_TSI_LEN         SCHAR_MAX*/
#define  MAX_TSI_LEN         30		/* smaller length for less memory consumption before fix */

#define  INVALID_ICODE_IDX      -128
#define  FIRST_TSI_NUM          200000
/* icode_idx_t ranges:
   -128                         => INVALID_ICDOE_IDX
   -127 ~ -1                    => ASCII
      0 ~ 199999                => single multibyte char
   FIRST_TSI_NUM ~ INT_MAX      => valid tsi_num
*/

/* Tab file header for general cin */
typedef struct {
    char version[VERLEN];		/* version number. */
    char encoding[ENCLEN];		/* table encoding. */
    char ename[CIN_ENAME_LENGTH];	/* English name. */
    char cname[CIN_CNAME_LENGTH];	/* Chinese name. */

    wch_t keyname[N_KEYCODE];		/* key name define. */
    char selkey[SELECT_KEY_LENGTH + 1];	/* select keys. */
    char endkey[END_KEY_LENGTH + 1];	/* end keys. */

    unsigned int n_ichar;		/* # of total chars. */
    unsigned int n_icode;		/* # of total keystroke code */
    unsigned char n_keyname;		/* # of keyname needed. */
    unsigned char n_selkey;		/* # of select keys. */
    unsigned char n_endkey;		/* # of end keys. */
    unsigned char n_max_keystroke;	/* max # of keystroke for each char */
    char icode_mode;
/* KhoGuan add */
    int n_tsi;				/* # of total tsi, 
					 * only count multi-zi tsi */
} cintab_head_t;

#endif

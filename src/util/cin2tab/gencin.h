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
*/      

#ifndef _GENCIN_H
#define _GENCIN_H

#include <stdlib.h>
#include "constant.h"
#include "module.h"

#define GENCIN_VERSION  "20040102"

/* For input-code char definition. */
typedef unsigned int	icode_t;
// Modify by Firefly(firefly@firefly.idv.tw)
typedef unsigned int	icode_idx_t;
typedef icode_idx_t 	ichar_t;

#define VERLEN			20	/* Version buffer size */
#define ENCLEN			15	/* Encoding name buffer size */
#define CIN_ENAME_LENGTH	20      /* English name buffer size */
#define CIN_CNAME_LENGTH	20      /* Chinese name buffer size */

#define ICODE_IDX_NOTEXIST	65535

#define ICODE_MODE1		1	/* one icode & one icode_idx */
#define ICODE_MODE2		2	/* two icode & one icode_idx */

#define  INP_CODE_LENGTH        10	/* max # of keys in a keystroke */
#define  END_KEY_LENGTH         10      /* max # of end keys */


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
} cintab_head_t;

#endif

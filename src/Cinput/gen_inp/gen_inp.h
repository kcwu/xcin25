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

#ifndef _GEN_INP_H
#define _GEN_INP_H

#include <stdlib.h>
#include "../../util/cin2tab/gencin.h"

#define INP_MODE_AUTOSELECT  0x00000001 /* Auto-select mode on. */
#define INP_MODE_AUTOCOMPOSE 0x00000002 /* Auto-compose mode on. */
#define INP_MODE_AUTOUPCHAR  0x00000004 /* Auto-up-char mode on. */
#define INP_MODE_AUTOFULLUP  0x00000008 /* Auto-full-up mode on. */
#define INP_MODE_SPACEAUTOUP 0x00000010 /* Space key can auto-up-char */
#define INP_MODE_SELKEYSHIFT 0x00000020 /* selkey shift mode on. */
#define INP_MODE_SPACEIGNOR  0x00000040 /* Ignore the space after a char. */
#define INP_MODE_WILDON      0x00000080 /* Enable the wild mode. */
#define INP_MODE_ENDKEY      0x00000100 /* Enable the end key mode. */
#define INP_MODE_SINMDLINE1  0x00000200 /* Enable sinmd in line1 mode. */
#define INP_MODE_SPACERESET  0x00000400 /* Enable space reset error mode. */
#define INP_MODE_AUTORESET   0x00000800 /* Enable auto reset error mode. */
#define INP_MODE_HINTSEL     0x00001000 /* Enable hint selection. */
#define INP_MODE_HINTTSI     0x00002000 /* Enable hint tsi. */
#define INP_MODE_BEEPWRONG   0x00010000 /* Beap when type a wrong char. */
#define INP_MODE_BEEPDUP     0x00020000 /* Beap when exists duplet chars. */

#define INPINFO_MODE_MCCH	0x0001
#define INPINFO_MODE_SPACE	0x0002
#define INPINFO_MODE_INWILD     0x0004
#define INPINFO_MODE_WRONG      0x0008

typedef struct {
    char keystroke[INP_CODE_LENGTH+1];
    wch_t wch;
} kremap_t;

typedef struct {
    char *inp_cname;		/* IM Chinese name */
    char *inp_ename;		/* IM English name */
    char *tabfn;		/* IM tab full path */
    unsigned int mode;		/* IM mode flag */
    cintab_head_t header;	/* cin-tab file header */
    ccode_info_t ccinfo;	/* info of current encoding */
    ubyte_t modesc;		/* Modifier escape mode */
    char *disable_sel_list;	/* List of keys to disable selection keys */
    int n_kremap;		/* Number of keystroke remapping */
    kremap_t *kremap;		/* Keystroke remapping list */

    icode_t *ic1;		/* icode & idx for different memory models */
    icode_t *ic2;
    icode_idx_t *icidx;
    ichar_t *ichar;

#ifdef HAVE_LIBTABE
    struct TsiDB *tsidb;	/* tsi db */
#endif
} gen_inp_conf_t;

#define HINTSZ	100

typedef struct {
    char keystroke[INP_CODE_LENGTH+1];
    unsigned short mode;
    wch_t *mcch_list;
    int *mkey_list;
    unsigned int n_mcch_list, mcch_hidx, mcch_eidx, n_mkey_list;

#ifdef HAVE_LIBTABE
    char commithistory[HINTSZ];	/* committed words */
    int showtsiflag;		/* show tsi flag */
    int nreltsi;
    char reltsi[HINTSZ];	/* related tsi */
    int tsiindex[HINTSZ];	/* index of tsi */
    ubyte_t tsigroup[HINTSZ];	/* group of tsi */
#endif
} gen_inp_iccf_t;


#endif

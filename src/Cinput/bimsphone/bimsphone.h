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

#ifndef _PHONE_H
#define _PHONE_H

#include "module.h"
#include "../../util/cin2tab/bimscin.h"

#define N_KEYMAPS		4
#define N_SELMAPS		2
#define N_MAX_KEYCODE_ZUYIN	4
#define N_MAX_KEYCODE_PINYIN	7
#define N_MAX_KEYCODE		7
#define N_MAX_SELECTION		10
#define MCCH_BUFSIZE		30

#define BIMSPH_MODE_SPACESEL	0x0001		/* space selection on */
#define BIMSPH_MODE_AUTOSEL	0x0002		/* auto-selection on */	
#define BIMSPH_MODE_PHRASESEL	0x0004		/* phrase selection on */
#define BIMSPH_MODE_AUTOUPCH	0x0008		/* auto-up char on */
#define BIMSPH_MODE_USRDB	0x0010		/* user db file on */
#define BIMSPH_MODE_PINYIN	0x1000		/* pinyin mode on */

#define BIMSPH_PGKEY1		0x01		/* page key <> */
#define BIMSPH_PGKEY2		0x02		/* page key ,. */
#define BIMSPH_PGKEY3		0x04		/* page key (Right)(Left) */

typedef struct ipinyin_s {
    int pinno;
    unsigned char tone[6];
    unsigned char zhu[83];
    wch_t ntone[5];		/* pinyin tone number. */
    wch_t stone[5];		/* zhuyin tone symbol. */
    pinpho_t *pinpho;		/* pinyin index: sorting in pin_idx order. */
    pinpho_t *phopin;		/* pinyin index: sorting in phone_idx order. */
} ipinyin_t;

typedef struct phone_conf_s {
    char *inp_cname;		/* IM Chinese name */
    char *inp_ename;		/* IM English name */
    byte_t setkey;		/* Set key number (for switch to this IM) */
    xmode_t mode;		/* IM mode flag */

    ubyte_t modesc;		/* Modifier escape mode */
    byte_t n_selkey;		/* number of selection keys */
    byte_t n_selphr;		/* number of phrase selection */
    byte_t keymap;		/* 0: zozy, 1: eten, 2: hsu */
    byte_t selmap;		/* 0: number, 1: home ("asdfghjkl;") */
    ubyte_t page_key;		/* 1:<> 2:,. 4:(Rt)(Lt) */

    ipinyin_t *pinyin;		/* pinyin special structer. */
} phone_conf_t;


#define ICCF_MODE_COMPOSEDOK	0x0001		/* bims has composed a char */

typedef struct phone_iccf_s {
    unsigned short lcch_size;    	/* Buffer size of lcch list. */
    unsigned short lcch_max_len;	/* Allowed max length of lcch list. */
    unsigned short lcchg_size;		/* Buffer size of lcch grouping */
    wch_t mcch[MCCH_BUFSIZE];
    char mcch_grouping[N_MAX_SELECTION+1];
    wch_t s_keystroke[N_MAX_KEYCODE+1];
    wch_t s_selkey[N_MAX_SELECTION];
    wch_t s_skeystroke[N_MAX_KEYCODE+1];
    xmode_t mode;			/* IM mode per IC. */
    char pin_keystroke[N_MAX_KEYCODE_PINYIN+1];
} phone_iccf_t;

typedef struct {
    char *s_selkey;
    KeySym keysym[10];
} selkey_list_t;

#endif

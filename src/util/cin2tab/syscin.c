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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include "xcintool.h"
#include "syscin.h"
#include "module.h"
#include "cin2tab.h"


/*--------------------------------------------------------------------------

	syscin function.

--------------------------------------------------------------------------*/

static char *sys_str[] = {
    "%charcode",
    "INPN_ENGLISH",
    "INPN_SBYTE",
    "INPN_2BYTES",
    "ASCII_space",
    "ASCII_exclam",
    "ASCII_quotedbl",
    "ASCII_numbersign",
    "ASCII_dollar",
    "ASCII_percent",
    "ASCII_ampersand",
    "ASCII_apostrophe",
    "ASCII_parenleft",
    "ASCII_parenright",
    "ASCII_asterisk",
    "ASCII_plus",
    "ASCII_comma",
    "ASCII_minus",
    "ASCII_period",
    "ASCII_slash",
    "ASCII_0",
    "ASCII_1",
    "ASCII_2",
    "ASCII_3",
    "ASCII_4",
    "ASCII_5",
    "ASCII_6",
    "ASCII_7",
    "ASCII_8",
    "ASCII_9",
    "ASCII_colon",
    "ASCII_semicolon",
    "ASCII_less",
    "ASCII_equal",
    "ASCII_greater",
    "ASCII_question",
    "ASCII_at",
    "ASCII_A",
    "ASCII_B",
    "ASCII_C",
    "ASCII_D",
    "ASCII_E",
    "ASCII_F",
    "ASCII_G",
    "ASCII_H",
    "ASCII_I",
    "ASCII_J",
    "ASCII_K",
    "ASCII_L",
    "ASCII_M",
    "ASCII_N",
    "ASCII_O",
    "ASCII_P",
    "ASCII_Q",
    "ASCII_R",
    "ASCII_S",
    "ASCII_T",
    "ASCII_U",
    "ASCII_V",
    "ASCII_W",
    "ASCII_X",
    "ASCII_Y",
    "ASCII_Z",
    "ASCII_bracketleft",
    "ASCII_backslash",
    "ASCII_bracketright",
    "ASCII_asciicircum",
    "ASCII_underscore",
    "ASCII_grave",
    "ASCII_a",
    "ASCII_b",
    "ASCII_c",
    "ASCII_d",
    "ASCII_e",
    "ASCII_f",
    "ASCII_g",
    "ASCII_h",
    "ASCII_i",
    "ASCII_j",
    "ASCII_k",
    "ASCII_l",
    "ASCII_m",
    "ASCII_n",
    "ASCII_o",
    "ASCII_p",
    "ASCII_q",
    "ASCII_r",
    "ASCII_s",
    "ASCII_t",
    "ASCII_u",
    "ASCII_v",
    "ASCII_w",
    "ASCII_x",
    "ASCII_y",
    "ASCII_z",
    "ASCII_braceleft",
    "ASCII_bar",
    "ASCII_braceright",
    "ASCII_asciitilde",
    NULL
};

static char *plane_str[] = {
    "plane1",
    "plane2",
    "plane3",
    "plane4",
    NULL
};

static int
check_hex_num(char *numbuf, int *num)
{
    char *err_num=NULL;

    if (strncasecmp("0x", numbuf, 2))
	return -1;
    *num = strtoul(numbuf+2, &err_num, 16);
    if (err_num[0] != '\0')
	return -1;
    return 0;
}

static void
read_encoding(charcode_t *ccp, cintab_t *cintab)
{
    int i, num=0, idx;
    char cmd[80], arg[80], numbuf[20], **p, *s;

    while(cmd_arg(cmd, 80, arg, 80, NULL)) {
	if (! cmd[0] || ! arg[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): \"cmd arg\" pair expected.\n"),
		cintab->fname_cin, cintab->lineno);

	p = plane_str;
	while (*p && strcmp(*p, cmd))
	    p ++;
	if (! strcmp("%charcode", cmd) && ! strcmp("end", arg))
	    return;
	else if (! *p)
	    perr(XCINMSG_ERROR, N_("%s(%d): unexpected syscin key: %s.\n"),
		cintab->fname_cin, cintab->lineno, cmd);

	i = (int)(p-plane_str);
	if (ccp[i].n >= N_CCODE_RULE)
	    perr(XCINMSG_WARNING, 
		N_("%s(%d): too many rules for char plane %d, ignore.\n"),
		cintab->fname_cin, cintab->lineno, i+1);

	s = arg;
	idx = ccp[i].n;
	if (! get_word(&s, numbuf, 20, "-") || 
	    check_hex_num(numbuf, &num) == -1)
	    perr(XCINMSG_ERROR, N_("%s(%d): hex number expected.\n"),
		cintab->fname_cin, cintab->lineno);
	ccp[i].begin[idx] = (ubyte_t)num;
	s ++;
	if (! get_word(&s, numbuf, 20, "-") || 
	    check_hex_num(numbuf, &num) == -1)
	    perr(XCINMSG_ERROR, N_("%s(%d): hex number expected.\n"),
		cintab->fname_cin, cintab->lineno);
	ccp[i].end[idx] = (ubyte_t)num;
	ccp[i].n ++;
    }
}

void
syscin(cintab_t *cintab)
{
    char inpn_english[CIN_CNAME_LENGTH];
    char inpn_sbyte[CIN_CNAME_LENGTH];
    char inpn_2bytes[CIN_CNAME_LENGTH];
    char cmd[30], arg[15];
    wch_t ascii[N_ASCII_KEY];
    charcode_t ccp[WCH_SIZE];
    int i;

    memset(inpn_english, 0, CIN_CNAME_LENGTH);
    memset(inpn_sbyte, 0, CIN_CNAME_LENGTH);
    memset(inpn_2bytes, 0, CIN_CNAME_LENGTH);
    memset(ascii, 0, sizeof(ascii));

    while (cmd_arg(cmd, 30, arg, 15, NULL)) {
	if (! cmd[0] || ! arg[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): \"cmd arg\" pair expected.\n"),
		cintab->fname_cin, cintab->lineno);

	for (i=0; i<N_ASCII_KEY+5 && strcmp(cmd, sys_str[i]); i++);
	if (i >= N_ASCII_KEY+5)
	    perr(XCINMSG_ERROR, N_("%s(%d): unknown syscin key: %s\n"),
		cintab->fname_cin, cintab->lineno, cmd);
	if (! arg[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): %s: argument expected.\n"),
		cintab->fname_cin, cintab->lineno, cmd);

	if (i == 0) {
	    if (strcmp("begin", arg))
		perr(XCINMSG_ERROR, 
			N_("%s(%d): %s: argument \"begin\" expected.\n"),
			cintab->fname_cin, cintab->lineno, cmd);
	    for (i=0; i<WCH_SIZE; i++)
		memset(ccp+i, 0, sizeof(charcode_t));
	    read_encoding(ccp, cintab);
	}
	else if (i < 4) {
	    switch (i) {
	    case 1:
		strncpy(inpn_english, arg, CIN_CNAME_LENGTH);
		inpn_english[CIN_CNAME_LENGTH - 1] = '\0';
		break;
	    case 2:
		strncpy(inpn_sbyte, arg, CIN_CNAME_LENGTH);
		inpn_sbyte[CIN_CNAME_LENGTH - 1] = '\0';
		break;
	    case 3:
		strncpy(inpn_2bytes, arg, CIN_CNAME_LENGTH);
		inpn_2bytes[CIN_CNAME_LENGTH - 1] = '\0';
		break;
	    }
	}
	else {
	    if (! read_hexwch(ascii[i-4].s, arg))
	        strncpy((char *)ascii[i-4].s, arg, WCH_SIZE);
	}
    }

    fwrite(SYSCIN_VERSION, sizeof(SYSCIN_VERSION), 1, cintab->fw);
    fwrite(inpn_english, sizeof(char), CIN_CNAME_LENGTH, cintab->fw);
    fwrite(inpn_sbyte, sizeof(char), CIN_CNAME_LENGTH, cintab->fw);
    fwrite(inpn_2bytes, sizeof(char), CIN_CNAME_LENGTH, cintab->fw);
    fwrite(ascii, sizeof(wch_t), N_ASCII_KEY, cintab->fw);
    fwrite(ccp, sizeof(charcode_t), WCH_SIZE, cintab->fw);
}

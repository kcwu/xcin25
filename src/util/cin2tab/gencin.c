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
#include "gencin.h"
#include "cin2tab.h"

static ccode_info_t ccinfo;

static cintab_head_t th;
static icode_idx_t *icode_idx;
static ichar_t *ichar;
static icode_t *icode1;
static icode_t *icode2;
static int n_ignore;


/*--------------------------------------------------------------------------

	Normal cin2tab functions.

--------------------------------------------------------------------------*/

static void
cin_ename(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"), 
		cintab->fname_cin, cintab->lineno);
    strncpy(th.ename, arg, CIN_ENAME_LENGTH);
    th.ename[CIN_ENAME_LENGTH - 1] = '\0';
}

static void
cin_cname(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    strncpy(th.cname, arg, CIN_CNAME_LENGTH);
    th.cname[CIN_CNAME_LENGTH - 1] = '\0';
}

static void
cin_selkey(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    th.n_selkey = strlen(arg);
    if (th.n_selkey > SELECT_KEY_LENGTH)
	perr(XCINMSG_ERROR, N_("%s(%d): too many selection keys defined.\n"),
	     cintab->fname_cin, cintab->lineno);
    strcpy(th.selkey, arg);
}

static void
cin_endkey(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    th.n_endkey = strlen(arg);
    if (th.n_endkey > END_KEY_LENGTH)
	perr(XCINMSG_ERROR, N_("%s(%d): too many end keys defined.\n"),
	     cintab->fname_cin, cintab->lineno);
    strcpy(th.endkey, arg);
}

static void
cin_keyname(char *arg, cintab_t *cintab)
{
    char cmd1[64], arg1[64];
    int k;

    if (! arg[0] || strcmp(arg, "begin") != 0)
	perr(XCINMSG_ERROR, N_("%s(%d): arguement \"begin\" expected.\n"),
	     cintab->fname_cin, cintab->lineno);

    while (cmd_arg(cmd1, 64, arg1, 64, NULL)) {
        if (! arg1[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	if (! strcmp("%keyname", cmd1) && ! strcmp("end", arg1))
	    break;

	if (! (k = key2code(cmd1[0])))
	    perr(XCINMSG_ERROR, N_("%s(%d): illegal key \"%c\" specified.\n"),
		 cintab->fname_cin, cintab->lineno, cmd1[0]);
	if (th.keyname[k].wch)
	    perr(XCINMSG_ERROR, N_("%s(%d): key \"%c\" is already in used.\n"),
		 cintab->fname_cin, cintab->lineno, cmd1[0]);
	if (! read_hexwch(th.keyname[k].s, arg1))
	    strncpy((char *)th.keyname[k].s, arg1, WCH_SIZE);
	th.n_keyname++;
    }
}

/*------------------------------------------------------------------------*/

typedef struct {
    int char_idx;
    unsigned int key[2];
    byte_t mark;
} cin_char_t;

static int
icode_cmp(const void *a, const void *b)
{
    cin_char_t *aa=(cin_char_t *)a, *bb=(cin_char_t *)b;

    if (aa->key[0] == bb->key[0]) {
	if (aa->key[1] == bb->key[1])
	    return 0;
	else if (aa->key[1] > bb->key[1])
	    return 1;
	else
	    return -1;
    }
    else if (aa->key[0] > bb->key[0])
	return 1;
    else
	return -1;
}

static void
cin_chardef(char *arg, cintab_t *cintab)
{
    char cmd1[64], arg1[64], arg2[2];
    byte_t *idx;
    wch_t ch;
    cin_char_t *cchar, *cch;
    int len, ret, cch_size;
    unsigned int i, j;

    if (! arg[0] || strcmp(arg, "begin") != 0)
	perr(XCINMSG_ERROR, N_("%s(%d): arguement \"begin\" expected.\n"),
	     cintab->fname_cin, cintab->lineno);

    cch_size = ccinfo.total_char;
    cch = cchar = xcin_malloc(cch_size*sizeof(cin_char_t), 1);
    idx = xcin_malloc(cch_size*sizeof(byte_t), 1);

    while ((ret=cmd_arg(cmd1, 64, arg1, 64, arg2, 2, NULL))) {
        if (! arg1[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	if (th.n_icode >= cch_size) {
	    cch_size += ccinfo.total_char;
	    cchar = xcin_realloc(cchar, cch_size * sizeof(cin_char_t));
	    cch = cchar + th.n_icode;
	}
	if (! strcmp("%chardef", cmd1) && ! strcmp("end", arg1))
	    break;

	/*
	 *  Fill in the cin_char_t *cch unit.
	 */
	ch.wch = (wchar_t)0;
	if (! read_hexwch(ch.s, arg1))
	    strncpy((char *)ch.s, arg1, WCH_SIZE);
	if ((i = ccode_to_idx(&ch)) == -1) {
	    n_ignore ++;
	    continue;
	}
	cch->char_idx = i;
	keys2codes(cch->key, 2, cmd1);
	cch->mark = (ret==3 && arg2[0]=='*') ? 1 : 0;

	if (! idx[i])
	    th.n_ichar ++;
	else
	    idx[i] = 1;
	th.n_icode ++;
	cch ++;

	len = strlen(cmd1);
	if (th.n_max_keystroke < len)
	    th.n_max_keystroke = len;
    }

    /*
     *  Determine the memory model.
     */
    ret = (th.n_max_keystroke <= 5) ? ICODE_MODE1 : ICODE_MODE2;
    th.icode_mode = ret;

    /*
     *  Fill in the ichar, icode_idx and icode1, icode2 arrays.
     */
    stable_sort(cchar, th.n_icode, sizeof(cin_char_t), icode_cmp);

    ichar = xcin_malloc(cch_size * sizeof(ichar_t), 1);
    icode_idx = xcin_malloc(sizeof(icode_idx_t)*th.n_icode, 1);
    icode1 = xcin_malloc(th.n_icode*sizeof(icode_t), 1);
    if (ret == ICODE_MODE2)
        icode2 = xcin_malloc(th.n_icode*sizeof(icode_t), 1);
    memset(idx, 0, ccinfo.total_char);

    for (i=0; i<cch_size; i++)
	ichar[i] = (ichar_t)ICODE_IDX_NOTEXIST;
    for (i=0, cch=cchar; i<th.n_icode; i++, cch++) {
	icode_idx[i] = j = (icode_idx_t)(cch->char_idx);
	icode1[i] = (icode_t)(cch->key[0]);
        if (ret == ICODE_MODE2)
	    icode2[i] = (icode_t)(cch->key[1]);

	if (! idx[j] || cch->mark) {
	    ichar[j] = (ichar_t)i;
	    idx[j] = 1;
	}
    }
    free(cchar);
    free(idx);
}


/*----------------------------------------------------------------------------

	Main Functions.

----------------------------------------------------------------------------*/

struct genfunc_s {
    char *name;
    void (*func) (char *, cintab_t *);
    byte_t gotit;
};

static struct genfunc_s genfunc[] = {
    {"%ename", cin_ename, 0},
    {"%cname", cin_cname, 0},
    {"%keyname", cin_keyname, 0},
    {"%selkey", cin_selkey, 0},
    {"%endkey", cin_endkey, 1},
    {"%chardef", cin_chardef, 0},
    {NULL, NULL, 0}
};

void
gencin(cintab_t *cintab)
{
    int i;
    char cmd[64], arg[64];

    if (cintab->sysfn == NULL)
	check_xcin_path(&(cintab->xrc), XCINMSG_ERROR);

    strncpy(th.version, GENCIN_VERSION, VERLEN);
    strncpy(th.encoding, cintab->xrc.locale.encoding, ENCLEN);
    load_systab(cintab->sysfn, &(cintab->xrc));
    ccode_info(&ccinfo);

    while (cmd_arg(cmd, 64, arg, 64, NULL)) {
	for (i=0; genfunc[i].name != NULL; i++) {
	    if (! strcmp(genfunc[i].name, cmd)) {
		genfunc[i].func(arg, cintab);
		genfunc[i].gotit = 1;
		break;
	    }
	}
	if (genfunc[i].name == NULL)
	    perr(XCINMSG_ERROR, N_("%s(%d): unknown command: %s.\n"),
		 cintab->fname_cin, cintab->lineno, cmd);
    }

    for (i=0; genfunc[i].name != NULL; i++) {
	if (genfunc[i].gotit == 0)
	    perr(XCINMSG_ERROR, N_("%s: command \"%s\" not specified.\n"),
		 cintab->fname_cin, genfunc[i].name);
    }
    perr(XCINMSG_NORMAL,
	N_("number of keyname: %d\n"), th.n_keyname);
    perr(XCINMSG_NORMAL, 
	N_("max length of keystroke: %d\n"), th.n_max_keystroke);
    perr(XCINMSG_NORMAL, 
	N_("total char number of this encoding: %d\n"), ccinfo.total_char);
    perr(XCINMSG_NORMAL, 
	N_("number of char defined: %d\n"), th.n_ichar);
    perr(XCINMSG_NORMAL, 
	N_("number of keystroke code defined: %d\n"), th.n_icode);
    perr(XCINMSG_NORMAL, 
	N_("number of defined char ignored: %d\n"), n_ignore);
    perr(XCINMSG_NORMAL, 
	N_("memory model: %d\n"), th.icode_mode);

    fwrite(&th, sizeof(cintab_head_t), 1, cintab->fw);
    fwrite(icode_idx, sizeof(icode_idx_t), th.n_icode, cintab->fw);
    fwrite(ichar, sizeof(ichar_t), ccinfo.total_char, cintab->fw);
    fwrite(icode1, sizeof(icode_t), th.n_icode, cintab->fw);
    if (th.icode_mode == ICODE_MODE2)
	fwrite(icode2, sizeof(icode_t), th.n_icode, cintab->fw);
}


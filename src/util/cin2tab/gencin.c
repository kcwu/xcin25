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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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
static icode_t *icode[MAX_ICODE_MODE];
static int n_ignore;

/* KhoGuan add */
/* tsi_idx[i]: accumulated length as index into tsi_list */ 
static unsigned int *tsi_idx = NULL; 
static icode_idx_t *tsi_list = NULL;

typedef struct {
/* KhoGuan rev
    int char_idx;
    unsigned int key[2];
*/
    icode_idx_t char_idx;
    icode_t key[MAX_ICODE_MODE];
    byte_t mark;
/* KhoGuan add */
/* tsi_len and tsi will be used for multi-zi tsi only. Otherwise, they are kept 0. */
    tsisize_t tsi_len;
    icode_idx_t tsi[MAX_TSI_LEN];
} cin_char_t;

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

static int
icode_cmp(const void *a, const void *b)
{
    int i;
    cin_char_t *aa=(cin_char_t *)a, *bb=(cin_char_t *)b;

    for(i=0; i<MAX_ICODE_MODE; i++)
	if(aa->key[i] > bb->key[i])
	    return 1;
	else if(aa->key[i] < bb->key[i])
	    return -1;
    return 0;
}

/* A "phrase" as processed in parse_phrase() may consist of one or 
   several multibyte characters with the same encoding such as Big5.
   Even singlebyte ASCII isgraph() character(s) can combine with it/them.
   All of them will be parsed into zi cells.
*/
icode_idx_t
parse_phrase(char arg[], icode_idx_t tsi[], tsisize_t* tsi_len_p) 
{
    char *cp = arg;
    static icode_idx_t tsi_num = FIRST_TSI_NUM;
    int len;
    icode_idx_t ic;
    wch_t wch;

    *tsi_len_p = 0;

    while (*cp && *tsi_len_p < MAX_TSI_LEN) {
        len=read_hexch_enc(&wch, cp);
	if(len==0)
	    len=mbtowch(&wch, cp, WCH_SIZE);
	if(len) {
	    ic = ccode_to_idx(&wch);
	    if (ic != -1) {
		tsi[(*tsi_len_p)] = ic;
		(*tsi_len_p)++;
		cp += len;
		continue;
	    } 
        }
	if(isgraph(*cp)) {
	    tsi[(*tsi_len_p)] = (icode_idx_t)((unsigned)*cp * -1);
	    (*tsi_len_p)++;
	    cp++;
	} else {
	    *tsi_len_p = 0;
	    break;
        }
    }

    if (*tsi_len_p == 0)
	return (tsi[0] = INVALID_ICODE_IDX);
    else if(*tsi_len_p == 1)
	return tsi[0];
    else if(*tsi_len_p == MAX_TSI_LEN && *cp)
	perr(XCINMSG_WARNING, N_("phrase too long\n"));
    else
	return tsi_num++;
}
        
static void
cin_chardef(char *arg, cintab_t *cintab)
{
    int arg1_len = MAX_TSI_LEN * WCH_SIZE + 1;
    char cmd1[64], arg1[arg1_len], arg2[2];
    byte_t *idx;
    wch_t ch;
    cin_char_t *cchar, *cch;
    int len, ret, cch_size;
    icode_idx_t ic;
    unsigned int i, j; 

/* KhoGuan rev */
    icode_idx_t tsi[MAX_TSI_LEN];
    tsisize_t tsi_len = 0;
    unsigned int tsi_accu_len = 0;
    unsigned int* tsi_idx_cur = NULL;
    icode_idx_t* tsi_list_cur = NULL;

    th.n_tsi = 0;
    memset(tsi, 0, sizeof(tsi));

    if (! arg[0] || strcmp(arg, "begin") != 0)
	perr(XCINMSG_ERROR, N_("%s(%d): arguement \"begin\" expected.\n"),
	     cintab->fname_cin, cintab->lineno);
/* KhoGuan: since we don't know n_icode in advance, we use total_char
   of the encoding as the starting buffer size */
    cch_size = ccinfo.total_char;
    cch = cchar = xcin_malloc(cch_size*sizeof(cin_char_t), 1);
    idx = xcin_malloc(cch_size*sizeof(byte_t), 1);


    while ((ret=cmd_arg(cmd1, 64, arg1, arg1_len, arg2, 2, NULL))) {

        if (! arg1[0])
	    perr(XCINMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
/* KhoGuan: afterwards, if we find n_icode is larger than total_char,
   we resize our buffer */
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
/* KhoGuan rev */
        ic = parse_phrase(arg1, tsi, &tsi_len); /* ic is the codepoint */
        if (tsi_len == 0) {
            perr(XCINMSG_WARNING, N_("%s(line %d): 0x"), cintab->fname_cin, cintab->lineno);
            for (i = 0; i < arg1[i]; i++)
                printf("%hhx", arg1[i]);
            printf(" invalid \"zi\" or \"tsi\".\n");

            n_ignore ++;
            continue;
        }
        else if (tsi_len == 1) {        /* single multibyte char */
            if (! idx[ic]) {
                th.n_ichar++;
                idx[ic] = 1;
            }
            cch->tsi_len = 1;
        }
        else {        /* tsi_len > 1, for multi-zi tsi*/
	    cch->tsi_len = tsi_len;
	    for (i = 0; i < tsi_len; i++) {
		cch->tsi[i] = tsi[i];
		if (tsi[i] >= (icode_idx_t)0 && 
			tsi[i] < (icode_idx_t)ccinfo.total_char) {
		    if (! idx[tsi[i]]) {
			th.n_ichar++;
			idx[tsi[i]] = 1;
		    }
		}
	    }
	    tsi_accu_len += tsi_len;
	    th.n_tsi++;
	}

	cch->char_idx = ic;
	keys2codes(cch->key, 2, cmd1);
	cch->mark = (ret==3 && arg2[0]=='*') ? 1 : 0;

	th.n_icode ++;
	cch ++;

	len = strlen(cmd1);
	if (th.n_max_keystroke < len)
	    th.n_max_keystroke = len;
    }
    /*
     *  Determine the memory model.
     */
/* KhoGuan rev
    ret = (th.n_max_keystroke <= 5) ? ICODE_MODE1 : ICODE_MODE2;
*/
    ret = (th.n_max_keystroke+4)/5;
    th.icode_mode = ret;

    /*
     *  Fill in the ichar, icode_idx and icode1, icode2 arrays.
     */

    if (th.n_tsi > 0) {
        tsi_idx_cur = tsi_idx = xcin_malloc(th.n_tsi * sizeof(unsigned int), 1);
        tsi_list_cur = tsi_list = xcin_malloc(tsi_accu_len * sizeof(icode_idx_t), 1);
    }

    for (i=0, cch=cchar; i<th.n_icode; i++, cch++) {
        if (cch->tsi_len > 1) {
            *tsi_idx_cur = ((tsi_idx_cur == tsi_idx)? 
                             cch->tsi_len : cch->tsi_len + *(tsi_idx_cur-1));
            tsi_idx_cur++;
            for (j = 0; j < cch->tsi_len; j++)
                *(tsi_list_cur + j) = cch->tsi[j];
            tsi_list_cur += j;
        }
    }

    stable_sort(cchar, th.n_icode, sizeof(cin_char_t), icode_cmp);

/* KhoGuan debug: cch_size => ccinfo.total_char
    ichar = xcin_malloc(cch_size * sizeof(ichar_t), 1); */
    ichar = xcin_malloc(ccinfo.total_char * sizeof(ichar_t), 1);
    icode_idx = xcin_malloc(sizeof(icode_idx_t)*th.n_icode, 1);
    for (i=0; i<ret; i++) {
      icode[i] = xcin_malloc(th.n_icode*sizeof(icode_t), 1);
    }
    memset(idx, 0, ccinfo.total_char);

    for (i=0; i<ccinfo.total_char; i++)
        ichar[i] = (ichar_t)ICODE_IDX_NOTEXIST;
    for (i=0, cch=cchar; i<th.n_icode; i++, cch++) {
	icode_idx[i] = ic = (icode_idx_t)(cch->char_idx);
	for(j=0; j<ret; j++)
	  icode[j][i] = (icode_t)(cch->key[j]);

	if (ic >= (icode_idx_t)0 && ic < (icode_idx_t)ccinfo.total_char) { 
	    if (! idx[ic] || cch->mark) {
		ichar[ic] = (ichar_t)i;
		idx[ic] = 1;
            }
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
    free(icode_idx);
    fwrite(ichar, sizeof(ichar_t), ccinfo.total_char, cintab->fw);
    free(ichar);
    for(i=0; i<th.icode_mode; i++) {
	fwrite(icode[i], sizeof(icode_t), th.n_icode, cintab->fw);
	free(icode[i]);
    }
/* KhoGuan add */
    if (th.n_tsi > 0) {
        perr(XCINMSG_NORMAL, 
             N_("number of phrase defined: %d\n"), th.n_tsi);

        fwrite(tsi_idx, sizeof(unsigned int), th.n_tsi, cintab->fw);
        fwrite(tsi_list, sizeof(icode_idx_t), tsi_idx[th.n_tsi-1], cintab->fw);
        free(tsi_idx);
        free(tsi_list);
    }
}


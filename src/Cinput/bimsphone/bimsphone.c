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
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "xcintool.h"
#include "module.h"
#include "bims.h"
#include "bimsphone.h"

static selkey_list_t sel_num = {"1234567890", 
	{XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9, XK_0}};

static selkey_list_t sel_home = {"asdfghjkl;",
	{XK_a, XK_s, XK_d, XK_f, XK_g, XK_h, XK_j, XK_k, XK_l, XK_semicolon}};

static selkey_list_t *sel[2] = {&sel_num, &sel_home};

static int keymaplist[N_KEYMAPS]={
	BC_KEYMAP_ZO, BC_KEYMAP_ETEN, BC_KEYMAP_ETEN26, BC_KEYMAP_HSU};

int load_pinyin_data(FILE *fp, char *truefn, phone_conf_t *cf);
char *pho2pinyinw(ipinyin_t *pf, char *phostring);
int pinyin_keystroke(phone_conf_t *cf, phone_iccf_t *iccf,
		inpinfo_t *inpinfo, keyinfo_t *keyinfo, int *rval2);

#ifdef DEBUG
extern int verbose;
#endif

/*----------------------------------------------------------------------------

	phone_init()

----------------------------------------------------------------------------*/

static void
phone_resource(phone_conf_t *cf, xcin_rc_t *xrc, char *objname,
	       char *ftsi, char *fyin, char *fpinyin)
{
    char *cmd[2], value[256];

    cmd[0] = objname;
    cmd[1] = "INP_CNAME";                               /* inp_names */
    if (get_resource(xrc, cmd, value, 256, 2)) {
	if (cf->inp_cname)
	    free(cf->inp_cname);
        cf->inp_cname = (char *)strdup(value);
    }

    cmd[1] = "N_SELECTION_KEY";
    if (get_resource(xrc, cmd, value, 256, 2)) {
	int n_selkey = atoi(value);
	if (n_selkey >= N_MAX_SELECTION/2 && n_selkey <= N_MAX_SELECTION)
	    cf->n_selkey = (byte_t)n_selkey;
    }
    cmd[1] = "SELECTION_KEYS";
    if (get_resource(xrc, cmd, value, 256, 2)) {
	int selnum;
	selnum = atoi(value);
	if (selnum >= 0 && selnum < N_SELMAPS)
	    cf->selmap = (byte_t)selnum;
    }
    cmd[1] = "PAGE_KEYS";
    if (get_resource(xrc, cmd, value, 256, 2))
	cf->page_key = (ubyte_t)(atoi(value) % 256);
    cmd[1] = "QPHRASE_MODE";
    if (get_resource(xrc, cmd, value, 256, 2))
	cf->modesc = (ubyte_t)(atoi(value) % 256);

    cmd[1] = "AUTO_SELECTION";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, BIMSPH_MODE_AUTOSEL, 0);
    cmd[1] = "KEYMAP";
    if (get_resource(xrc, cmd, value, 256, 2)) {
	int mapnum = atoi(value);
	if (mapnum >= 0 && mapnum < N_KEYMAPS)
	    cf->keymap = (byte_t)mapnum;
    }
    cmd[1] = "PINPHO_MAP";
    if (get_resource(xrc, cmd, value, 256, 2)) {
	char *s = strrchr(value, '.');
	if (! s || strcmp(s+1, "tab"))
	    strncat(value, ".tab", 256);
	strcpy(fpinyin, value);
    }
    cmd[1] = "TSI_FNAME";
    if (get_resource(xrc, cmd, value, 256, 2))
	strcpy(ftsi, value);
    cmd[1] = "YIN_FNAME";
    if (get_resource(xrc, cmd, value, 256, 2))
	strcpy(fyin, value);

    cmd[1] = "SPACE_SELECTION";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, BIMSPH_MODE_SPACESEL, 0);
    cmd[1] = "PHRASE_SELECTION";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, BIMSPH_MODE_PHRASESEL, 0);
    cmd[1] = "N_SELECTION_PHR";
    if (get_resource(xrc, cmd, value, 256, 2)) {
	int n_selphr = atoi(value);
	if (n_selphr <= cf->n_selkey)
	    cf->n_selphr = n_selphr;
    }

    cmd[1] = "AUTO_UPCHAR";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, BIMSPH_MODE_AUTOUPCH, 0);
}

static int
phone_init(void *conf, char *objname, xcin_rc_t *xrc)
{
    phone_conf_t *cf = (phone_conf_t *)conf, cfd;
    objenc_t objenc;
    char ftsi[256], fyin[256], fpinyin[256];
    FILE *fp;

    memset(&cfd, 0, sizeof(phone_conf_t));
    cfd.n_selkey = 9;
    cfd.n_selphr = 9;
    cfd.page_key = (ubyte_t)BIMSPH_PGKEY3;
    ftsi[0] = fyin[0] = fpinyin[0] = '\0';

    if (get_objenc(objname, &objenc) == -1)
	return False;
    if (! strncmp(objenc.objname, "bimspinyin", 10)) {
	cf->mode |= BIMSPH_MODE_PINYIN;
	cfd.inp_cname = (char *)strdup("«÷­µ");
    }
    else
	cfd.inp_cname = (char *)strdup("ª`­µ");
    phone_resource(&cfd, xrc, "bimsphone_default", ftsi, fyin, fpinyin);
    phone_resource(&cfd, xrc, objenc.objloadname, ftsi, fyin, fpinyin);

/*
 *  Global setup.
 */
    cf->inp_ename = (char *)strdup(objenc.objname);
    cf->inp_cname = cfd.inp_cname;
    cf->n_selkey  = cfd.n_selkey;
    cf->selmap    = cfd.selmap;
    cf->page_key  = cfd.page_key;
    cf->modesc    = cfd.modesc;
/*
 *  Special options for ZuYin / PinYin modes.
 */
    if (! (cf->mode & BIMSPH_MODE_PINYIN)) {
	if ((cfd.mode & BIMSPH_MODE_AUTOSEL))
	    cf->mode |= BIMSPH_MODE_AUTOSEL;
	cf->keymap = cfd.keymap;
    }
    else {
	char truefn[256], sub_path[256];

	/* For PinYin mode, always fix to Auto-Selection. */
	cf->mode |= BIMSPH_MODE_AUTOSEL;
	/* Only fixed to Zozy */
	cf->keymap = (byte_t)0;

	snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
	if (check_datafile(fpinyin, sub_path, xrc, truefn, 256) == True)
	    fp = open_file(truefn, "r", XCINMSG_WARNING);
	else
	    fp = NULL;
	if (fp == NULL) {
	    perr(XCINMSG_WARNING, 
		N_("bimsphone: %s: cannot open pinyin data file: %s.\n"),
		objenc.objloadname, fpinyin);
	    return False;
	}
	if (load_pinyin_data(fp, truefn, cf) == False)
	    return False;
    }
/*
 *  Further auto-selection setup.
 */
    if ((cf->mode & BIMSPH_MODE_AUTOSEL)) {
	char tsi_fname[256], yin_fname[256], sub_path[256];

	snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
	if (check_datafile(ftsi, sub_path, xrc, tsi_fname, 256) == False) {
	    perr(XCINMSG_WARNING, 
		N_("bimsphone: %s: cannot open data file: %s\n"),
		objenc.objloadname, ftsi);
	    return False;
	}
	if (check_datafile(fyin, sub_path, xrc, yin_fname, 256) == False) {
	    perr(XCINMSG_WARNING, 
		N_("bimsphone: %s: cannot open data file: %s\n"),
		objenc.objloadname, fyin);
	    return False;
	}

	if ((cfd.mode & BIMSPH_MODE_SPACESEL))
	    cf->mode |= BIMSPH_MODE_SPACESEL;
	if ((cfd.mode & BIMSPH_MODE_PHRASESEL)) {
	    cf->mode |= BIMSPH_MODE_PHRASESEL;
	    cf->n_selphr = cfd.n_selphr;
	}
	else
	    cf->n_selphr = 0;

	if (bimsInit(tsi_fname, yin_fname) == 0)
	    return True;
	else
	    return False;
    }
    else {
	if ((cfd.mode & BIMSPH_MODE_AUTOUPCH))
	    cf->mode |= BIMSPH_MODE_AUTOUPCH;
	cf->n_selphr = 0;
	return True;
    }
}


/*----------------------------------------------------------------------------

	Tool Functions.

----------------------------------------------------------------------------*/

static int
big5_mbs_wcs(wch_t *wcs, char *mbs, int wcs_len)
{
    int len;
    char *s;

    for (s=mbs, len=0; *s && len<wcs_len-1; s+=2, len++) {
	wcs[len].wch = (wchar_t)0;
	wcs[len].s[0] = *s;
	wcs[len].s[1] = *(s+1);
    }
    wcs[len].wch = (wchar_t)0;

    return len;
}

static void
commit_string(inpinfo_t *inpinfo, phone_iccf_t *iccf, int n_chars)
{
    static char *str=NULL;

    if (str)
	free(str);
    str = (char *)bimsFetchText(inpinfo->imid, n_chars);
    inpinfo->cch = str;
}

static void
s_commit_string(inpinfo_t *inpinfo, char *s)
{
    static char str[3];

    str[0] = s[0];
    str[1] = s[1];
    str[2] = '\0';
    inpinfo->cch = str;
}

static void
check_winsize(inpinfo_t *inpinfo, phone_iccf_t *iccf)
{
    int size;

    size = (inpinfo->xcin_wlen > 0) ? inpinfo->xcin_wlen/2 - 1 : 15;
    if (iccf->lcch_size != size) {
	bimsSetMaxLen(inpinfo->imid, size-1);
	iccf->lcch_max_len = size-1;
    }
    if (iccf->lcch_size <= size) {
	if (inpinfo->lcch)
	    free(inpinfo->lcch);
	iccf->lcch_size = size+1;
        inpinfo->lcch = calloc(iccf->lcch_size, sizeof(wch_t));
    }
}

static void
publish_composed_cch(phone_conf_t *cf, inpinfo_t *inpinfo, wch_t *wch)
{
    char *str=NULL, *str1=NULL;

    inpinfo->cch_publish.wch = wch->wch;
    if ((str = (char *)bimsQueryLastZuYinString(inpinfo->imid))) {
	str1 = ((cf->mode & BIMSPH_MODE_PINYIN)) ? 
		pho2pinyinw(cf->pinyin, str) : str;
	if (str1)
	    big5_mbs_wcs(inpinfo->suggest_skeystroke, str1, N_MAX_KEYCODE+1);
	free(str);
    }
}

static void
editing_status(phone_conf_t *cf, inpinfo_t *inpinfo, phone_iccf_t *iccf)
{
    char *str;

    if (! (cf->mode & BIMSPH_MODE_PINYIN)) {
	str = (char *)bimsQueryZuYinString(inpinfo->imid);
	inpinfo->keystroke_len =
        	big5_mbs_wcs(inpinfo->s_keystroke, str, N_MAX_KEYCODE+1);
	free(str);
    }

    if ((cf->mode & BIMSPH_MODE_AUTOSEL)) {
        int len, *seg;
	str = (char *)bimsQueryInternalText(inpinfo->imid);
	len = strlen(str) / 2;
	if (len >= iccf->lcch_size) {
	    iccf->lcch_size = len+1;
	    inpinfo->lcch = realloc(inpinfo->lcch, (len+1)*sizeof(wch_t));
	}
	inpinfo->n_lcch = big5_mbs_wcs(inpinfo->lcch, str, len+1);
	free(str);

	if (! inpinfo->keystroke_len && inpinfo->n_lcch)
	    iccf->mode |= ICCF_MODE_COMPOSEDOK;
	else
	    iccf->mode &= ~(ICCF_MODE_COMPOSEDOK);

	inpinfo->edit_pos = len = bimsQueryPos(inpinfo->imid);
	if ((iccf->mode & ICCF_MODE_COMPOSEDOK)) {
	    if (len == inpinfo->n_lcch && len > 0)
		len --;
	    publish_composed_cch(cf, inpinfo, inpinfo->lcch+len);
	}

	seg = bimsQueryYinSeg(inpinfo->imid);
	if (iccf->lcchg_size <= seg[0]) {
	    free(inpinfo->lcch_grouping);
	    iccf->lcchg_size = seg[0] + 1;
	    inpinfo->lcch_grouping = malloc(iccf->lcch_size * sizeof(ubyte_t));
	}
	for (len=0; len<seg[0]+1; len++)
	    inpinfo->lcch_grouping[len] = (ubyte_t)seg[len];
	free(seg);
    }
}

static int
determine_selection(phone_conf_t *cf, inpinfo_t *inpinfo, phone_iccf_t *iccf, 
		int state, KeySym key, wch_t *ch_return)
/*
 *  Return: 0: change back, no selection.
 *	    n: change back, get selection (n-1).
 *	   -1: no change back, ignore key.
 */
{
    int i, j, base, num, cpnum, lpnum, pgstate=0;
    unsigned char **selection;

    selection = bimsQuerySelectionText(inpinfo->imid);
    num  = bimsQuerySelectionNumber(inpinfo->imid);
    base = bimsQuerySelectionBase(inpinfo->imid);

/*  Determine selection number  */
    if (state == BC_STATE_SELECTION_ZHI) {
	cpnum = cf->n_selkey;
	lpnum = cf->n_selkey;
    } 
    else {
	int word;
	for (i=base, j=0, word=0; j<cf->n_selphr && i<num; i++, j++) {
	    word += (strlen((char *)selection[i]) / 2);
	    if (word >= MCCH_BUFSIZE) 
		break;
	}
	cpnum = j;

	for (i=base-1, j=0, word=0; j<cf->n_selphr && i>=0; i--, j++) {
	    word += (strlen((char *)selection[i]) / 2);
	    if (word >= MCCH_BUFSIZE) 
		break;
	}
	lpnum = j;
    }

/*  Determine key action  */
    if (key) {
        switch(key) {
        case XK_Escape:
            return 0;
	case XK_space:
	    pgstate = 1;
	    break;
        case XK_less:
	    if ((cf->page_key & BIMSPH_PGKEY1))
		pgstate = -1;
	    else
		return -1;
	    break;
	case XK_comma:
	    if ((cf->page_key & BIMSPH_PGKEY2))
		pgstate = -1;
	    else
		return -1;
            break;
	case XK_Left:
	    if ((cf->page_key & BIMSPH_PGKEY3))
		pgstate = -1;
	    else
		return -1;
            break;
        case XK_greater:
	    if ((cf->page_key & BIMSPH_PGKEY1))
		pgstate = 1;
	    else
		return -1;
            break;
	case XK_period:
	    if ((cf->page_key & BIMSPH_PGKEY2))
		pgstate = 1;
	    else
		return -1;
            break;
	case XK_Right:
	    if ((cf->page_key & BIMSPH_PGKEY3))
		pgstate = 1;
	    else
		return -1;
            break;
	case XK_BackSpace:
	    return -1;
        default:
	    for (i=0; i<cpnum; i++) {
		if (key == sel[cf->selmap]->keysym[i]) {
		    if (i + base < num) {
			if (ch_return)
			    strcpy((char *)ch_return->s, 
				   (char *)selection[base+i]);
			return base+i+1;
		    }
		}
	    }
	    if ((cf->mode & BIMSPH_MODE_AUTOUPCH)) {
		if (ch_return)
		    strcpy((char *)ch_return->s, (char *)selection[base]);
		return base+1;
	    }
            break;
        }
    }

/*  Shift base when page changing  */
    if (pgstate == -1) {
        if (base - lpnum >= 0)
            base -= lpnum;
	else if (inpinfo->mcch_pgstate == MCCH_BEGIN &&
		 state == BC_STATE_SELECTION_ZHI) {
	    i = num % cpnum;
	    base = num - ((i == 0) ? cpnum : i);
	}
    }
    else if (pgstate == 1) {
        if (base + cpnum < num)
            base += cpnum;
	else if (inpinfo->mcch_pgstate == MCCH_END)
	    base = 0;
    }

    bimsSetSelectionBase(inpinfo->imid, base);
    if (base > 0) {
        if (base + cpnum < num)
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
	else
            inpinfo->mcch_pgstate = MCCH_END;
    }
    else if (base + cpnum < num)
	inpinfo->mcch_pgstate = MCCH_BEGIN;
    else 
	inpinfo->mcch_pgstate = MCCH_ONEPG;

/*  Prepare for mcch display  */
    if (state==BC_STATE_SELECTION_ZHI) {
	inpinfo->mcch_grouping[0] = (char)0;
	for (i=0; i<cpnum && i+base < num; i++) {
	    inpinfo->mcch[i].wch = (wchar_t)0;
	    strncpy((char *)inpinfo->mcch[i].s, (char *)selection[base+i], 2);
	}
	inpinfo->n_mcch = i;
    } 
    else {
	int word, tmp;
	char *str;
	for (i=base, j=0, word=0; j<cf->n_selphr && i<num; i++, j++) {
	    str = (char *)selection[i];
	    tmp = inpinfo->mcch_grouping[j+1] = strlen(str)/2;
	    if (word+tmp >= MCCH_BUFSIZE) 
		break;
	    for (; *str; word++, str+=2) {
		inpinfo->mcch[word].wch = (wchar_t)0;
		strncpy((char *)inpinfo->mcch[word].s, str, 2);
	    }
	}
	inpinfo->n_mcch = word;
	inpinfo->mcch_grouping[0] = j;
    }
    iccf->mode &= ~(ICCF_MODE_COMPOSEDOK);

    return -1;
}

static unsigned int
modifier_escape(phone_conf_t *cf, inpinfo_t *inpinfo, 
		keyinfo_t *keyinfo, int *gotit)
{
    unsigned int ret=0;
    int ctrl_alt=0;

    *gotit = 0;
    if ((keyinfo->keystate & ControlMask) == ControlMask) {
	if ((cf->modesc & QPHR_CTRL))
	    ret |= IMKEY_CTRLPHR;
	else
	    ret = (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
	ctrl_alt = 1;
	*gotit = 1;
    }
    else if ((keyinfo->keystate & Mod1Mask) == Mod1Mask) {
	if ((cf->modesc & QPHR_ALT))
	    ret |= IMKEY_ALTPHR;
	else
	    ret = (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
	ctrl_alt = 1;
	*gotit = 1;
    }
    else if ((keyinfo->keystate & ShiftMask) == ShiftMask) {
	if ((cf->modesc & QPHR_SHIFT))
	    ret |= (IMKEY_SHIFTPHR | IMKEY_SHIFTESC);
	else
	    ret = (inpinfo->n_lcch) ? IMKEY_ABSORB :
			((keyinfo->keystr_len == 1) ? 
				IMKEY_SHIFTESC : IMKEY_IGNORE);
	*gotit = 1;
    }
    if ((keyinfo->keystate & LockMask) == LockMask) {
	if (ctrl_alt == 0 && (keyinfo->keystr_len == 1) &&
	    (inpinfo->guimode & GUIMOD_LISTCHAR))
	    ret |= IMKEY_SHIFTESC;
	else
	    ret |= IMKEY_IGNORE;
	*gotit = 1;
    }

    if (ret != IMKEY_ABSORB && ret != IMKEY_IGNORE) {
	if (inpinfo->n_lcch) {
	    commit_string(inpinfo, inpinfo->iccf, inpinfo->n_lcch);
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    editing_status(cf, inpinfo, (phone_iccf_t *)inpinfo->iccf);
	    ret |= IMKEY_COMMIT;
	}
    }
    return ret;
}

static unsigned int
check_qphr_fallback(phone_conf_t *cf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    if ((cf->modesc & QPHR_FALLBACK) &&
	(keyinfo->keystr_len == 1) && (inpinfo->guimode & GUIMOD_LISTCHAR))
	return IMKEY_FALLBACKPHR;
    else
        return (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
}

/*----------------------------------------------------------------------------

	phone_xim_init() & phone_xim_end()

----------------------------------------------------------------------------*/

static int
phone_xim_init(void *conf, inpinfo_t *inpinfo)
{
    phone_conf_t *cf = (phone_conf_t *)conf;
    phone_iccf_t *iccf;
    int i, selmap = (int)(cf->selmap);

    inpinfo->iccf = calloc(1, sizeof(phone_iccf_t));
    iccf = (phone_iccf_t *)inpinfo->iccf;

    inpinfo->inp_cname = cf->inp_cname;
    inpinfo->inp_ename = cf->inp_ename;
    if (! (cf->mode & BIMSPH_MODE_PINYIN))
	inpinfo->area3_len = N_MAX_KEYCODE_ZUYIN * 2 + 2;
    else
	inpinfo->area3_len = N_MAX_KEYCODE_PINYIN + 2;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = iccf->s_keystroke;
    inpinfo->suggest_skeystroke = iccf->s_skeystroke;

    inpinfo->n_selkey = cf->n_selkey;
    inpinfo->s_selkey = iccf->s_selkey;
    for (i=0; i<inpinfo->n_selkey; i++) {
	inpinfo->s_selkey[i].wch = (wchar_t)0;
	inpinfo->s_selkey[i].s[0] = sel[selmap]->s_selkey[i];
    }
    inpinfo->n_mcch = 0;
    inpinfo->mcch = iccf->mcch;
    inpinfo->mcch_grouping = (ubyte_t *)iccf->mcch_grouping;

    inpinfo->n_lcch = 0;
    inpinfo->edit_pos = 0;
    inpinfo->cch_publish.wch = (wchar_t)0;
    if ((cf->mode & BIMSPH_MODE_AUTOSEL)) {
	inpinfo->guimode = GUIMOD_LISTCHAR;
	check_winsize(inpinfo, iccf);
	iccf->lcchg_size = iccf->lcch_size + 1;
        inpinfo->lcch_grouping = calloc(iccf->lcchg_size, sizeof(ubyte_t));
    }
    else {
	inpinfo->guimode = 0;
	inpinfo->lcch = NULL;
        inpinfo->lcch_grouping = NULL;
    }

    if ((cf->mode & BIMSPH_MODE_AUTOSEL))
	bimsToggleSmartEditing(inpinfo->imid);
    else
	bimsToggleNoSmartEditing(inpinfo->imid);
    if (bimsSetKeyMap(inpinfo->imid, keymaplist[(int)(cf->keymap)]) == 0)
	return True;
    else {
	free(inpinfo->iccf);
	inpinfo->iccf = NULL;
	if (inpinfo->lcch)
	    free(inpinfo->lcch);
	return False;
    }
}

static unsigned int
phone_xim_end(void *conf, inpinfo_t *inpinfo)
{
    unsigned int ret;

    if (inpinfo->n_lcch) {
	commit_string(inpinfo, inpinfo->iccf, inpinfo->n_lcch);
	ret = IMKEY_COMMIT;
    }
    else
	ret = IMKEY_ABSORB;

    bimsFreeBC(inpinfo->imid);
    free(inpinfo->iccf);
    if (inpinfo->lcch)
	free(inpinfo->lcch);
    if (inpinfo->lcch_grouping);
	free(inpinfo->lcch_grouping);
    inpinfo->iccf = NULL;
    inpinfo->s_keystroke = NULL;
    inpinfo->suggest_skeystroke = NULL;
    inpinfo->s_selkey = NULL;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;

    return ret;
}


/*----------------------------------------------------------------------------

	phone_keystroke() & phone_show_keystroke()

----------------------------------------------------------------------------*/

static int
enter_selection(phone_conf_t *cf, unsigned int bimsid)
{
    if ((cf->mode & BIMSPH_MODE_PHRASESEL)) {
	int state = bimsQueryState(bimsid);

	if (state != BC_STATE_SELECTION_TSI) {
	    if (bimsToggleTsiSelection(bimsid) == 0)
		return 1;
        }
	if (bimsToggleZhiSelection(bimsid) == 0)
	    return 1;
    }
    else if (bimsToggleZhiSelection(bimsid) == 0)
	return 1;

    return 0;
}

static unsigned int
bims_keystroke(phone_conf_t *cf, phone_iccf_t *iccf, 
	       inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    KeySym keysym = keyinfo->keysym;
    int state, rval;

    check_winsize(inpinfo, iccf);
    if (keysym == XK_Up) {
	if ((keyinfo->keystate & LockMask) == LockMask)
	    return IMKEY_IGNORE;
	if (enter_selection(cf, inpinfo->imid) == 0)
	    return (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
	inpinfo->guimode &= ~GUIMOD_LISTCHAR;
	keysym = 0;
    }
    else if (keysym == XK_Down) {
	if ((keyinfo->keystate & LockMask) == LockMask)
	    return IMKEY_IGNORE;
	if (bimsToggleZhiSelection(inpinfo->imid))
	    return (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
	inpinfo->guimode &= ~GUIMOD_LISTCHAR;
	keysym = 0;
    }
    else if (keysym == XK_Return) {
	if ((keyinfo->keystate & LockMask) == LockMask)
	    return IMKEY_IGNORE;
	else if (! (inpinfo->guimode & GUIMOD_LISTCHAR))
	    return IMKEY_ABSORB;
	else if (inpinfo->n_lcch) {
	    commit_string(inpinfo, iccf, inpinfo->n_lcch);
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    editing_status(cf, inpinfo, iccf);
	    return IMKEY_COMMIT;
	}
	else
	    return IMKEY_IGNORE;
    }
    else if (keysym == XK_space) {
	if ((keyinfo->keystate & LockMask) == LockMask)
	    return IMKEY_IGNORE;
	else if ((iccf->mode & ICCF_MODE_COMPOSEDOK)) {
	    if ((cf->mode & BIMSPH_MODE_SPACESEL)) {
		if (enter_selection(cf, inpinfo->imid)==0)
		    return (inpinfo->n_lcch) ? IMKEY_ABSORB : IMKEY_IGNORE;
		inpinfo->guimode &= ~GUIMOD_LISTCHAR;
		keysym = 0;
	    }
	    else if ((inpinfo->guimode & GUIMOD_LISTCHAR)) {
		commit_string(inpinfo, iccf, inpinfo->n_lcch);
		inpinfo->cch_publish.wch = (wchar_t)0;
		editing_status(cf, inpinfo, iccf);
		return IMKEY_COMMIT;
	    }
	}
	else if (! inpinfo->n_lcch && ! inpinfo->keystroke_len)
	    return IMKEY_IGNORE;
    }

    state = bimsQueryState(inpinfo->imid);
    switch(state) {
    case BC_STATE_EDITING:
	inpinfo->guimode &= ~GUIMOD_SELKEYSPOT;
	if (keyinfo->keystate) {
	    int ret, gotit;
	    ret = modifier_escape(cf, inpinfo, keyinfo, &gotit);
	    if (gotit)
		return ret;
	}

	inpinfo->n_mcch = 0;
	inpinfo->mcch[0].wch = (wchar_t)0;
	if ((cf->mode & BIMSPH_MODE_PINYIN)) {
	    int rval2;
	    rval = pinyin_keystroke(cf, iccf, inpinfo, keyinfo, &rval2);
	    if (rval2 != IMKEY_IGNORE)
		return rval2;
	}
	else
	    rval = bimsFeedKey(inpinfo->imid, keysym);

        switch (rval) {
        case BC_VAL_COMMIT:
	    commit_string(inpinfo, iccf, iccf->lcch_max_len);
	case BC_VAL_ABSORB:
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    editing_status(cf, inpinfo, iccf);
	    return (rval == BC_VAL_COMMIT) ? IMKEY_COMMIT : IMKEY_ABSORB;
        default:
	    return check_qphr_fallback(cf, inpinfo, keyinfo);
        }
        break;

    case BC_STATE_SELECTION_ZHI:
    case BC_STATE_SELECTION_TSI:
	inpinfo->guimode |= GUIMOD_SELKEYSPOT;
	if ((keyinfo->keystate & ControlMask) == ControlMask ||
	    (keyinfo->keystate & Mod1Mask) == Mod1Mask)
	    return IMKEY_ABSORB;

        rval = determine_selection(cf, inpinfo, iccf, state, keysym, NULL);
        if (rval != -1) {
	    if (rval > 0)
		bimsPindownByNumber(inpinfo->imid, rval-1);
	    inpinfo->n_mcch = 0;
	    inpinfo->mcch[0].wch = (wchar_t)0;
	    bimsToggleEditing(inpinfo->imid);
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    editing_status(cf, inpinfo, iccf);
	    inpinfo->guimode |= GUIMOD_LISTCHAR;
	    inpinfo->guimode &= ~GUIMOD_SELKEYSPOT;
        }
	return IMKEY_ABSORB;

    default:
        break;
    }
    return IMKEY_IGNORE;
}

static unsigned int
simple_keystroke(phone_conf_t *cf, phone_iccf_t *iccf, 
		 inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    int state, rval;
    wch_t ch_sel;

    if (! inpinfo->keystroke_len) {
	if (keyinfo->keysym == XK_Return || keyinfo->keysym == XK_space)
	    return IMKEY_IGNORE;
    }
    state = bimsQueryState(inpinfo->imid);
    switch(state) {
    case BC_STATE_EDITING:
	inpinfo->guimode &= ~GUIMOD_SELKEYSPOT;
	if (keyinfo->keystate) {
	    int ret, gotit;
	    ret = modifier_escape(cf, inpinfo, keyinfo, &gotit);
	    if (gotit)
		return ret;
	}

	inpinfo->n_mcch = 0;
	inpinfo->mcch[0].wch = (wchar_t)0;
	rval = bimsFeedKey(inpinfo->imid, keyinfo->keysym);
	if (bimsToggleZhiSelection(inpinfo->imid) == BC_VAL_IGNORE) {
	    switch (rval) {
	    case BC_VAL_COMMIT:
	    case BC_VAL_ABSORB:
		inpinfo->cch_publish.wch = (wchar_t)0;
		editing_status(cf, inpinfo, iccf);
		return IMKEY_ABSORB;
	    default:
		return check_qphr_fallback(cf, inpinfo, keyinfo);
	    }
	}
	else {
	    char *str = (char *)bimsQueryLastZuYinString(inpinfo->imid);
	    inpinfo->keystroke_len =
	        big5_mbs_wcs(inpinfo->s_keystroke, str, N_MAX_KEYCODE+1);
	    determine_selection(cf, inpinfo, iccf, BC_STATE_SELECTION_ZHI, 
				(KeySym)0, NULL);
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    if (inpinfo->n_mcch == 1) {
		ch_sel.wch = inpinfo->mcch[0].wch;
		inpinfo->n_mcch = 0;
		inpinfo->mcch[0].wch = (wchar_t)0;
		editing_status(cf, inpinfo, iccf);

		s_commit_string(inpinfo, (char *)ch_sel.s);
		publish_composed_cch(cf, inpinfo, &ch_sel);
		bimsFreeBC(inpinfo->imid);
		bimsSetKeyMap(inpinfo->imid, keymaplist[(int)(cf->keymap)]);
		return IMKEY_COMMIT;
	    }
	    else {
		inpinfo->guimode |= GUIMOD_SELKEYSPOT;
		return IMKEY_ABSORB;
	    }
	}
	break;

    case BC_STATE_SELECTION_ZHI:
	inpinfo->guimode |= GUIMOD_SELKEYSPOT;
	if ((keyinfo->keystate & ControlMask) == ControlMask ||
	    (keyinfo->keystate & Mod1Mask) == Mod1Mask)
	    return IMKEY_ABSORB;

	rval = determine_selection(cf, inpinfo, iccf, state, 
				keyinfo->keysym, &ch_sel);
	if (rval == -1)
	    return IMKEY_ABSORB;
	else {
	    inpinfo->n_mcch = 0;
	    inpinfo->mcch[0].wch = (wchar_t)0;
	    inpinfo->cch_publish.wch = (wchar_t)0;
	    editing_status(cf, inpinfo, iccf);
	    inpinfo->guimode &= ~GUIMOD_SELKEYSPOT;
	    if (rval > 0) {
		s_commit_string(inpinfo, (char *)ch_sel.s);
		publish_composed_cch(cf, inpinfo, &ch_sel);
	    }
	    bimsFreeBC(inpinfo->imid);
	    bimsSetKeyMap(inpinfo->imid, keymaplist[(int)(cf->keymap)]);

	    if (rval > 0) {
		int sel_num = (rval-1) % cf->n_selkey;
		if (keyinfo->keysym != sel[cf->selmap]->keysym[sel_num]) {
		    bimsFeedKey(inpinfo->imid, keyinfo->keysym);
		    editing_status(cf, inpinfo, iccf);
		}
		return IMKEY_COMMIT;
	    }
	    else
		return IMKEY_ABSORB;
	}

    default:
	break;
    }
    return IMKEY_IGNORE;
}

static unsigned int
phone_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    phone_conf_t *cf   = (phone_conf_t *)conf;
    phone_iccf_t *iccf = (phone_iccf_t *)inpinfo->iccf;

    if ((cf->mode & BIMSPH_MODE_AUTOSEL))
	return bims_keystroke(cf, iccf, inpinfo, keyinfo);
    else
	return simple_keystroke(cf, iccf, inpinfo, keyinfo);
}

static int 
phone_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    static wch_t keystroke_list[5];
    phone_conf_t *cf = (phone_conf_t *)conf;
    char *str, *str1;
    struct TsiDB **tdb;
    struct TsiYinDB **ydb;
    struct TsiInfo zhi;
    int n_db, i;

    if (simdinfo->cch_publish.wch && (n_db=bimsReturnDBPool(&tdb, &ydb)) > 0) {
	keystroke_list[0].wch = (wchar_t)0;
	zhi.tsi = simdinfo->cch_publish.s;
	zhi.refcount = 0;
	zhi.yinnum = 0;
	zhi.yindata = NULL;
	for (i=0; i<n_db; i++) {
	    if (tabeTsiInfoLookupZhiYin(tdb[i], &zhi) == 0) {
    		simdinfo->s_keystroke = keystroke_list;
		str = (char *)tabeYinToZuYinSymbolSequence(zhi.yindata[0]);

		if ((cf->mode & BIMSPH_MODE_PINYIN))
		    str1 = pho2pinyinw(cf->pinyin, str);
		else
		    str1 = str;
		if (str1)
		    big5_mbs_wcs(keystroke_list, str1, 5);
		free(str);
		break;
	    }
	}
	free(tdb);
	free(ydb);
	if (keystroke_list[0].wch != (wchar_t)0)
	    return True;
    }
    simdinfo->s_keystroke = NULL;
    return False;
}


/*----------------------------------------------------------------------------

        Definition of phonetic input method module (template).

----------------------------------------------------------------------------*/

static char *phone_valid_objname[] = { "bimsphone*", "bimspinyin*", NULL };

static char phone_comments[] = N_(
	"This is the \"official\" phonetic input method module.\n"
	"It uses the libtabe & bims libraries as the search engine\n"
	"to perform the automatic characher selection from multipole\n"
	"charachers with the same keystroke. This engine also provides\n"
	"several keymaps for each specific phonetic input rules.\n"
	"We hope that this module will serve as the most \"natural way\"\n"
	"for Chinese input.\n\n"
	"This module is free software, as part of xcin system.\n");

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "bimsphone",				/* name */
      MODULE_VERSION,				/* version */
      phone_comments },				/* comments */
    phone_valid_objname,			/* valid_objname */
    sizeof(phone_conf_t),			/* conf_size */
    phone_init,					/* init */
    phone_xim_init,				/* xim_init */
    phone_xim_end,				/* xim_end */
    phone_keystroke,				/* keystroke */
    phone_show_keystroke,			/* show_keystroke */
    NULL					/* terminate */
};


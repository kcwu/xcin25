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

#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "xcintool.h"
#include "module.h"
#include "gen_inp.h"

#ifdef DEBUG
extern int verbose;
#endif

/*----------------------------------------------------------------------------

        gen_inp_init()

----------------------------------------------------------------------------*/

static int
loadtab(gen_inp_conf_t *cf, FILE *fp, char *encoding)
{
    int n, nn, ret=1;
    char modID[MODULE_ID_SIZE];

    if (fread(modID, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(modID, "gencin")) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: invalid tab file.\n"),cf->tabfn);
	return False;
    }
    if (fread(&(cf->header), sizeof(cintab_head_t), 1, fp) != 1) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: reading error.\n"), cf->tabfn);
	return False;
    }
    if (! check_version(GENCIN_VERSION, cf->header.version, 5)) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: invalid version.\n"), cf->tabfn);
	return False;
    }
    if (strcmp(encoding, cf->header.encoding) != 0) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: invalid encoding: %s\n"),
				cf->tabfn, cf->header.encoding);
	return False;
    }
    if (! cf->inp_cname)
	cf->inp_cname = cf->header.cname;

    n  = cf->header.n_icode;
    nn = cf->ccinfo.total_char;
    cf->icidx = malloc(sizeof(icode_idx_t) * n);
    cf->ichar = malloc(sizeof(ichar_t) * nn);
    cf->ic1 = malloc(sizeof(icode_t) * n);
    if (!n || !nn ||
	fread(cf->icidx, sizeof(icode_idx_t), n, fp) != n ||
	fread(cf->ichar, sizeof(ichar_t), nn, fp) != nn ||
	fread(cf->ic1, sizeof(icode_t), n, fp) != n) {
	if (n) {
	    free(cf->icidx);
	    free(cf->ic1);
	}
	if (nn)
	    free(cf->ichar);
	ret = 0;
    }
    if (ret && cf->header.icode_mode == ICODE_MODE2) {
        cf->ic2 = malloc(sizeof(icode_t) * n);
	if (fread(cf->ic2, sizeof(icode_t), n, fp) != n) {
	    if (n)
		free(cf->ic2);
	    ret = 0;
	}
    }

    if (! ret) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: reading error.\n"), cf->tabfn);
	return False;
    }
    else
	return True;
}

static void
setup_kremap(gen_inp_conf_t *cf, char *value)
{
    int i=0;
    unsigned int num;
    char *s = value, *sp;

    while (*s) {
	if (*s == ';')
	    i++;
	s++;
    }
    cf->n_kremap = i;
    cf->kremap = malloc(sizeof(kremap_t) * i);

    s = sp = value;
    for (i=0; i<cf->n_kremap; i++) {
	while (*s != ':')
	    s++;
	*s = '\0';
	strncpy(cf->kremap[i].keystroke, sp, INP_CODE_LENGTH+1);

	s++;
	sp = s;
	while (*s != ';')
	    s++;
	*s = '\0';
	cf->kremap[i].wch.wch = (wchar_t)0;
	if (*sp == '0' && *(sp+1) == 'x') {
	    num = strtol(sp+2, NULL, 16);
	    cf->kremap[i].wch.s[0] = (num & 0xff00) >> 8;
	    cf->kremap[i].wch.s[1] = (num & 0x00ff);
	}
	else
	    strncpy((char *)cf->kremap[i].wch.s, sp, WCH_SIZE);

	s++;
	sp = s;
    }
}

static void
gen_inp_resource(gen_inp_conf_t *cf, char *objname)
{
    char *cmd[2], value[256];

    cmd[0] = objname;

    cmd[1] = "INP_CNAME";				/* inp_names */
    if (get_resource(cmd, value, 256, 2)) {
	if (cf->inp_cname)
	    free(cf->inp_cname);
        cf->inp_cname = (char *)strdup(value);
    }
    cmd[1] = "AUTO_COMPOSE";				/* auto compose */
    if (get_resource(cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOCOMPOSE, 0);

    cmd[1] = "AUTO_UPCHAR";				/* auto_up char */
    if (get_resource(cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOUPCHAR, 0);
    cmd[1] = "SPACE_AUTOUP";				/* space key auto_up */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACEAUTOUP, 0);
    cmd[1] = "SELKEY_SHIFT";				/* selkey shift */
    if (get_resource(cmd, value, 256, 2))	
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SELKEYSHIFT, 0);

    cmd[1] = "AUTO_FULLUP";				/* auto full_up */
    if (get_resource(cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOFULLUP, 0);
    cmd[1] = "SPACE_IGNORE";				/* space ignore */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACEIGNOR, 0);

    cmd[1] = "AUTO_RESET";				/* auto reset error */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTORESET, 0);
    cmd[1] = "SPACE_RESET";				/* space reset error */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACERESET, 0);

    cmd[1] = "SINMD_IN_LINE1";				/* sinmd in line1 mode*/
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SINMDLINE1, 0);
    cmd[1] = "WILD_ENABLE";				/* wild key enable */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_WILDON, 0);
    cmd[1] = "BEEP_WRONG";				/* beep mode: wrong */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_BEEPWRONG, 0);
    cmd[1] = "BEEP_DUPCHAR";				/* beep mode: dupchar */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_BEEPDUP, 0);
    cmd[1] = "QPHRASE_MODE";				/* qphrase mode */
    if (get_resource(cmd, value, 256, 2))
	cf->modesc = (ubyte_t)(atoi(value) % 256);

    cmd[1] = "DISABLE_SEL_LIST";			/* disable selkey */
    if (get_resource(cmd, value, 256, 2)) {
	if (cf->disable_sel_list)
	    free(cf->disable_sel_list);
	if (strcmp(value, "NONE"))
	    cf->disable_sel_list = (char *)strdup(value);
	else
	    cf->disable_sel_list = NULL;
    }
    cmd[1] = "KEYSTROKE_REMAP";
    if (get_resource(cmd, value, 256, 2)) {
	if (cf->kremap)
	    free(cf->kremap);
	if (strcmp(value, "NONE") == 0) {
	    cf->kremap = NULL;
	    cf->n_kremap = 0;
	}
	else
	    setup_kremap(cf, value);
    }

    cmd[1] = "END_KEY";					/* end key enable */
    if (get_resource(cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_ENDKEY, 0);
}

static int
gen_inp_init(void *conf, char *objname, xcin_rc_t *xc)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf, cfd;
    char *s, value[128], truefn[256];
    objenc_t objenc;
    FILE *fp;
    int ret;

    memset(&cfd, 0, sizeof(gen_inp_conf_t));
    if (get_objenc(objname, &objenc) == -1)
	return False;
    gen_inp_resource(&cfd, "gen_inp_default");
    gen_inp_resource(&cfd, objenc.objloadname);
/*
 *  Resource setup.
 */
    cf->inp_ename = (char *)strdup(objenc.objname);
    cf->inp_cname = cfd.inp_cname;
    if ((cfd.mode & INP_MODE_AUTOCOMPOSE))
	cf->mode |= INP_MODE_AUTOCOMPOSE;
    if ((cfd.mode & INP_MODE_AUTOUPCHAR)) {
	cf->mode |= INP_MODE_AUTOUPCHAR;
	if ((cfd.mode & INP_MODE_SPACEAUTOUP))
	    cf->mode |= INP_MODE_SPACEAUTOUP;
	if ((cfd.mode & INP_MODE_SELKEYSHIFT))
	    cf->mode |= INP_MODE_SELKEYSHIFT;
    }
    if ((cfd.mode & INP_MODE_AUTOFULLUP)) {
	cf->mode |= INP_MODE_AUTOFULLUP;
	if ((cfd.mode & INP_MODE_SPACEIGNOR))
	    cf->mode |= INP_MODE_SPACEIGNOR;
    }
    if ((cfd.mode & INP_MODE_AUTORESET))
	cf->mode |= INP_MODE_AUTORESET;
    else if ((cfd.mode & INP_MODE_SPACERESET))
	cf->mode |= INP_MODE_SPACERESET;
    if ((cfd.mode & INP_MODE_SINMDLINE1))
	cf->mode |= INP_MODE_SINMDLINE1;
    if ((cfd.mode & INP_MODE_WILDON))
	cf->mode |= INP_MODE_WILDON;
    if ((cfd.mode & INP_MODE_BEEPWRONG))
	cf->mode |= INP_MODE_BEEPWRONG;
    if ((cfd.mode & INP_MODE_BEEPDUP))
	cf->mode |= INP_MODE_BEEPDUP;
    cf->modesc = cfd.modesc;
    cf->disable_sel_list = cfd.disable_sel_list;
    cf->kremap = cfd.kremap;
    cf->n_kremap = cfd.n_kremap;
/*
 *  Read the IM tab file.
 */
    ccode_info(&(cf->ccinfo));
    if (! (s = strrchr(cf->inp_ename, '.')) || strcmp(s+1, "tab"))
        snprintf(value, 50, "%s.tab", cf->inp_ename);
    if ((fp = open_data(value, "rb", NULL, truefn, 256, XCINMSG_WARNING))) {
	cf->tabfn = (char *)strdup(truefn);
        ret = loadtab(cf, fp, objenc.encoding);
	fclose(fp);
    }
    else
	return False;

    if (cf->header.n_endkey) {
	if ((cfd.mode & INP_MODE_ENDKEY))
	    cf->mode |= INP_MODE_ENDKEY;
    }
    return  ret;
}


/*----------------------------------------------------------------------------

        gen_inp_xim_init()

----------------------------------------------------------------------------*/

static int
gen_inp_xim_init(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf;
    int i;

    inpinfo->iccf = calloc(1, sizeof(gen_inp_iccf_t));
    iccf = (gen_inp_iccf_t *)inpinfo->iccf;

    inpinfo->inp_cname = cf->inp_cname;
    inpinfo->inp_ename = cf->inp_ename;
    inpinfo->area3_len = cf->header.n_max_keystroke * 2 + 1;
    inpinfo->guimode = ((cf->mode & INP_MODE_SINMDLINE1)) ? 
			GUIMOD_SINMDLINE1 : 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = calloc(INP_CODE_LENGTH+1, sizeof(wch_t));
    inpinfo->suggest_skeystroke = calloc(INP_CODE_LENGTH+1, sizeof(wch_t));

    if (! (cf->mode & INP_MODE_SELKEYSHIFT)) {
	inpinfo->n_selkey = cf->header.n_selkey;
	inpinfo->s_selkey = calloc(inpinfo->n_selkey, sizeof(wch_t));
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i].s[0] = cf->header.selkey[i];
    }
    else {
	inpinfo->n_selkey = cf->header.n_selkey+1;
	inpinfo->s_selkey = calloc(inpinfo->n_selkey, sizeof(wch_t));
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i+1].s[0] = cf->header.selkey[i];
    }
    inpinfo->n_mcch = 0;
    inpinfo->mcch = calloc(inpinfo->n_selkey, sizeof(wch_t));
    inpinfo->mcch_grouping = NULL;
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    inpinfo->n_lcch = 0;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;
    inpinfo->cch_publish.wch = (wchar_t)0;

    return  True;
}

static unsigned int
gen_inp_xim_end(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;

    if (iccf->n_mcch_list)
	free(iccf->mcch_list);
    free(inpinfo->iccf);
    free(inpinfo->s_keystroke);
    free(inpinfo->suggest_skeystroke);
    free(inpinfo->s_selkey);
    free(inpinfo->mcch);
    inpinfo->iccf = NULL;
    inpinfo->s_keystroke = NULL;
    inpinfo->suggest_skeystroke = NULL;
    inpinfo->s_selkey = NULL;
    inpinfo->mcch = NULL;
    return IMKEY_ABSORB;
}


/*----------------------------------------------------------------------------

        gen_inp_keystroke(), gen_inp_show_keystroke

----------------------------------------------------------------------------*/

static unsigned int
return_wrong(gen_inp_conf_t *cf)
{
    return ((cf->mode & INP_MODE_BEEPWRONG)) ? IMKEY_BELL : IMKEY_ABSORB;
}

static unsigned int
return_correct(gen_inp_conf_t *cf)
{
    return ((cf->mode & INP_MODE_BEEPDUP)) ? IMKEY_BELL2 : IMKEY_ABSORB;
}

static void
reset_keystroke(inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    inpinfo->s_keystroke[0].wch = (wchar_t)0;
    inpinfo->keystroke_len = 0;
    inpinfo->n_mcch = 0;
    iccf->keystroke[0] = '\0';
    iccf->mode = 0;
    inpinfo->mcch_pgstate = MCCH_ONEPG;
    if (iccf->n_mcch_list) {
	free(iccf->mcch_list);
	iccf->n_mcch_list = 0;
    }
}


/*------------------------------------------------------------------------*/

static int
cmp_icvalue(icode_t *ic1, icode_t *ic2, unsigned int idx,
	    icode_t icode1, icode_t icode2, int mode)
{
    if (ic1[idx] > icode1)
	return 1;
    else if (ic1[idx] < icode1)
	return -1;
    else {
	if (! mode)
	    return 0;
	else if (ic2[idx] > icode2)
	    return 1;
	else if (ic2[idx] < icode2)
	    return -1;
	else
	    return 0;
    }
}

static int 
bsearch_char(icode_t *ic1, icode_t *ic2, 
	     icode_t icode1, icode_t icode2, int size, int mode, int wild)
{
    int head, middle, end, ret;

    head   = 0;
    middle = size / 2;
    end    = size;
    while ((ret=cmp_icvalue(ic1, ic2, middle, icode1, icode2, mode))) {
        if (ret > 0)
            end = middle;
        else
            head = middle + 1;
        middle = (end + head) / 2;
        if (middle == head && middle == end)
            break;
    }
    if (ret == 0) {
	while(middle > 0 &&
	      ! cmp_icvalue(ic1, ic2, middle-1, icode1, icode2, mode)) 
	    middle --;
	return middle;
    }
    else
	return (wild) ? middle : -1;
}

static int
pick_cch_wild(gen_inp_conf_t *cf, int *head, byte_t dir, char *keystroke, 
		wch_t *mcch, unsigned int mcch_size, unsigned int *n_mcch)
{
    unsigned int i, size, klist[2];
    int n_klist, ks_size, r=0, idx;
    char *ks;

    size = cf->header.n_icode;
    ks_size = cf->header.n_max_keystroke + 1;
    ks = malloc(ks_size);
    n_klist = (cf->header.icode_mode == ICODE_MODE1) ? 1 : 2;

    if (dir == 1) {
        for (i=0, idx = *head; idx<size && i<=mcch_size; idx++) {
	    klist[0] = cf->ic1[idx];
	    if (cf->header.icode_mode == ICODE_MODE2)
		klist[1] = cf->ic2[idx];
	    codes2keys(klist, n_klist, ks, ks_size);

	    if (strcmp_wild(keystroke, ks) == 0) {
		if (i < mcch_size) {
		    ccode_to_char(cf->icidx[idx], mcch[i].s, WCH_SIZE);
		    *head = idx;
		    i ++;
		}
		else
		    r = 1;
	    }
	}
    }
    else {
        for (i=0, idx = *head; idx>=0 && i<=mcch_size; idx--) {
	    klist[0] = cf->ic1[idx];
	    if (cf->header.icode_mode == ICODE_MODE2)
		klist[1] = cf->ic2[idx];
	    codes2keys(klist, n_klist, ks, ks_size);

	    if (strcmp_wild(keystroke, ks) == 0) {
		if (i < mcch_size) {
		    ccode_to_char(cf->icidx[idx], mcch[i].s, WCH_SIZE);
		    *head = idx;
		    i ++;
		}
		else
		    r = 1;
	    }
	}
    }
    free(ks);
    *n_mcch = i;
    return r;
}

static int
match_keystroke_wild(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    icode_t icode[2];
    unsigned int md, n_mcch;
    int idx;
    char *s1, *s2, *s, tmpch;

    md = (cf->header.icode_mode == ICODE_MODE2) ? 1 : 0;
    icode[0] = icode[1] = 0;

    /*
     *  Search for the first char.
     */
    s1 = strchr(iccf->keystroke, '*');
    s2 = strchr(iccf->keystroke, '?');
    if (s1 && s2)
	s = (s1 < s2) ? s1 : s2;
    else if (s1)
	s = s1;
    else
	s = s2;
    tmpch = *s;
    *s = '\0';

    keys2codes(icode, 2, iccf->keystroke);
    idx = bsearch_char(cf->ic1, cf->ic2, icode[0], icode[1], 
			cf->header.n_icode, md, 1);
    *s = tmpch;
    iccf->mcch_hidx = idx;		/* refer to head index of cf->icidx */

    /*
     *  Pick up the remaining chars;
     */
    if (pick_cch_wild(cf, &idx, 1, iccf->keystroke,
	    inpinfo->mcch, inpinfo->n_selkey, &n_mcch) == 0)
        inpinfo->mcch_pgstate = MCCH_ONEPG;
    else 
	inpinfo->mcch_pgstate = MCCH_BEGIN;
    inpinfo->n_mcch = (unsigned short)n_mcch;
    iccf->mcch_eidx = idx;		/* refer to end index of cf->icidx */
    return (n_mcch) ? 1 : 0;
}

static int
match_keystroke_normal(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    icode_t icode[2];
    unsigned int size, md, n_ich=0, mcch_size;
    int idx;
    wch_t *mcch;

    size = cf->header.n_icode;
    md = (cf->header.icode_mode == ICODE_MODE2) ? 1 : 0;
    icode[0] = icode[1] = 0;

    /*
     *  Search for the first char.
     */
    keys2codes(icode, 2, iccf->keystroke);
    if ((idx = bsearch_char(cf->ic1, cf->ic2, 
		icode[0], icode[1], size, md, 0)) == -1)
	return 0;

    /*
     *  Search for all the chars with the same keystroke.
     */
    mcch_size = inpinfo->n_selkey;
    mcch = malloc(mcch_size * sizeof(wch_t));
    do {
	if (mcch_size <= n_ich) {
	    mcch_size *= 2;
	    mcch = realloc(mcch, mcch_size * sizeof(wch_t));
	}
	if (! ccode_to_char(cf->icidx[idx], mcch[n_ich].s, WCH_SIZE))
	    return 0;
	n_ich ++;
        idx ++;
    } while (idx < size &&
             ! cmp_icvalue(cf->ic1, cf->ic2, idx, icode[0], icode[1], md));

    /*
     *  Prepare mcch for display.
     */
    for (idx=0; idx<inpinfo->n_selkey && idx<n_ich; idx++)
	inpinfo->mcch[idx].wch = mcch[idx].wch;
    inpinfo->n_mcch = idx;

    if (idx >= n_ich) {
        inpinfo->mcch_pgstate = MCCH_ONEPG;
	free(mcch);
    }
    else {
	inpinfo->mcch_pgstate = MCCH_BEGIN;
	if (iccf->n_mcch_list)
	    free(iccf->mcch_list);
        iccf->mcch_list = mcch;
        iccf->n_mcch_list = n_ich;
	iccf->mcch_hidx = 0;		/* refer to index of iccf->mcch_list */
    }
    return 1;
}

static int 
match_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
/* return: 1: success, 0: false */
{
    int ret;

    inpinfo->n_mcch = 0;
    if (! (iccf->mode & INPINFO_MODE_INWILD))
	ret = match_keystroke_normal(cf, inpinfo, iccf);
    else
	ret = match_keystroke_wild(cf, inpinfo, iccf);

    if (inpinfo->n_mcch > 1 && (iccf->mode & INPINFO_MODE_SPACE))
	iccf->mode &= ~(INPINFO_MODE_SPACE);
    return ret;
}


/*------------------------------------------------------------------------*/

static void
commit_char(inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, wch_t *cch)
{
    static char cch_s[WCH_SIZE+1];
    int i;

    inpinfo->cch = cch_s;
    strncpy(cch_s, (char *)cch->s, WCH_SIZE);
    cch_s[WCH_SIZE] = '\0';

    for (i=0; i<=inpinfo->keystroke_len; i++)
	inpinfo->suggest_skeystroke[i].wch = inpinfo->s_keystroke[i].wch; 
    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke[0].wch = (wchar_t)0;
    inpinfo->n_mcch = 0;
    inpinfo->cch_publish.wch = cch->wch;
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    iccf->mode &= ~(INPINFO_MODE_MCCH);
    iccf->mode &= ~(INPINFO_MODE_INWILD);
    inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
}

static unsigned int
commit_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
/* return: the IMKEY state */
{
    if (cf->n_kremap > 0) {
	int i;
	for (i=0; i<cf->n_kremap; i++) {
	    if (strcmp(iccf->keystroke, cf->kremap[i].keystroke) == 0) {
		commit_char(inpinfo, iccf, &(cf->kremap[i].wch));
		return IMKEY_COMMIT;
	    }
	}
    }

    if (match_keystroke(cf, inpinfo, iccf)) {
        if (inpinfo->n_mcch == 1) {
            commit_char(inpinfo, iccf, inpinfo->mcch);
            return IMKEY_COMMIT;
        }
        else {
            iccf->mode |= INPINFO_MODE_MCCH;
	    inpinfo->guimode |= GUIMOD_SELKEYSPOT;
            return return_correct(cf);
        }
    }
    else {
	if ((cf->mode & INP_MODE_AUTORESET))
	    reset_keystroke(inpinfo, iccf);
	else
	    iccf->mode |= INPINFO_MODE_WRONG;
        return return_wrong(cf);
    }
}


/*------------------------------------------------------------------------*/

static int
mcch_choosech(gen_inp_conf_t *cf, 
		inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx)
{
    int min;
    wch_t wch;

    if (inpinfo->n_mcch == 0 && ! match_keystroke(cf, inpinfo, iccf))
        return 0;

    if (idx < 0)		/* Always select the first. */
	idx = 0;
    else {
        if ((cf->mode & INP_MODE_SELKEYSHIFT))
	    idx ++;
        min = (inpinfo->n_selkey > inpinfo->n_mcch) ? 
                inpinfo->n_mcch : inpinfo->n_selkey;
        if (idx >= min)
            return 0;
    }
    wch.wch = inpinfo->mcch[idx].wch;
    commit_char(inpinfo, iccf, &wch);
    reset_keystroke(inpinfo, iccf);

    return 1;
}

static int
fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo, 
	 gen_inp_iccf_t *iccf, byte_t dir)
{
    int i, j, n_pg=inpinfo->n_selkey;

    if (! (iccf->mode & INPINFO_MODE_INWILD)) {
        int total = iccf->n_mcch_list;

        switch (dir) {
        case 0:
	    iccf->mcch_hidx = 0;
	    break;
        case 1:
	    if (iccf->mcch_hidx + n_pg < total)
	        iccf->mcch_hidx += n_pg;
	    else
	        return 0;
	    break;
        case -1:
	    if (iccf->mcch_hidx - n_pg >= 0)
	        iccf->mcch_hidx -= n_pg;
	    else
	        return 0;
	    break;
        }
	for (i=0, j=iccf->mcch_hidx; i<n_pg && j<total; i++, j++)
	    inpinfo->mcch[i].wch = iccf->mcch_list[j].wch;
        if (iccf->mcch_hidx == 0)
            inpinfo->mcch_pgstate = (i < total) ? MCCH_BEGIN : MCCH_ONEPG;
        else if (total - iccf->mcch_hidx > n_pg)
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
        else
	    inpinfo->mcch_pgstate = MCCH_END;
        inpinfo->n_mcch = i;
    }
    else {
	int r=0, n_mcch, hidx, eidx; 
	wch_t mcch[SELECT_KEY_LENGTH];

        switch (dir) {
        case 0:
	    return 0;
        case 1:
	    if (inpinfo->mcch_pgstate == MCCH_BEGIN ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE) {
		hidx = eidx = iccf->mcch_eidx + 1;
        	r = pick_cch_wild(cf, &eidx, 1, iccf->keystroke, inpinfo->mcch, 
			inpinfo->n_selkey, (unsigned int *)&n_mcch);
	    }
	    else
		return 0;
	    break;
        case -1:
	    if (inpinfo->mcch_pgstate == MCCH_END ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE) {
		hidx = eidx = iccf->mcch_hidx - 1;
        	r = pick_cch_wild(cf, &hidx, -1, iccf->keystroke,
	        	mcch, inpinfo->n_selkey, (unsigned int *)&n_mcch);
	        for (i=0, j=n_mcch-1; j>=0; i++, j--)
		    inpinfo->mcch[i].wch = mcch[j].wch;
	    }
	    else
		return 0;
	    break;
        }

        if (r == 0) {
	    if (dir == 1)
                inpinfo->mcch_pgstate = MCCH_END;
	    else
                inpinfo->mcch_pgstate = MCCH_BEGIN;
	}
        else
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
        inpinfo->n_mcch = n_mcch;
	iccf->mcch_hidx = hidx;
	iccf->mcch_eidx = eidx;
    }

    return 1;
}

static int
mcch_nextpage(gen_inp_conf_t *cf, 
	      inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, char key)
{
    int ret=0;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_ONEPG:
	switch (key) {
	case ' ':
	    if ((cf->mode & INP_MODE_AUTOUPCHAR))
                ret = (mcch_choosech(cf, inpinfo, iccf, -1)) ?
                	IMKEY_COMMIT : return_wrong(cf);
            else
                ret = return_correct(cf);
	    break;
	case '<':
	case '>':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_END:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 0)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_BEGIN:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    default:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;
    }
    return ret;
}

/*------------------------------------------------------------------------*/

static unsigned int
modifier_escape(gen_inp_conf_t *cf, int class)
{
    unsigned int ret=0;

    switch (class) {
    case QPHR_SHIFT:
	if ((cf->modesc & QPHR_SHIFT))
	    ret |= IMKEY_SHIFTPHR;
	ret |= IMKEY_SHIFTESC;
	break;
    case QPHR_CTRL:
	if ((cf->modesc & QPHR_CTRL))
	    ret |= IMKEY_CTRLPHR;
	break;
    case QPHR_ALT:
	if ((cf->modesc & QPHR_ALT))
	    ret |= IMKEY_ALTPHR;
	break;
    case QPHR_FALLBACK:
	if ((cf->modesc & QPHR_FALLBACK))
	    ret |= IMKEY_FALLBACKPHR;
	break;
    }
    return ret;
}

static unsigned int
gen_inp_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    char *keystr = keyinfo->keystr;
    int len, max_len;
    char sp_ignore=0, inp_wrong=0;

    len = inpinfo->keystroke_len;
    max_len = cf->header.n_max_keystroke;
    if ((iccf->mode & INPINFO_MODE_SPACE)) {
        sp_ignore = 1;
        iccf->mode &= ~(INPINFO_MODE_SPACE);
    }
    if ((iccf->mode & INPINFO_MODE_WRONG)) {
	inp_wrong = 1;
	iccf->mode &= ~(INPINFO_MODE_WRONG);
    }

    if ((keysym == XK_BackSpace || keysym == XK_Delete) && len) {
        iccf->keystroke[len-1] = '\0';
        inpinfo->s_keystroke[len-1].wch = (wchar_t)0;
        inpinfo->keystroke_len --;
	inpinfo->n_mcch = 0;
	inpinfo->cch_publish.wch = (wchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
	iccf->mode = 0;
	if (strchr(iccf->keystroke, '*') || strchr(iccf->keystroke, '?'))
	    iccf->mode |= INPINFO_MODE_INWILD;
	if (len-1 > 0 && (cf->mode & INP_MODE_AUTOCOMPOSE))
	    match_keystroke(cf, inpinfo, iccf);
        return IMKEY_ABSORB;
    }

    else if (keysym == XK_Escape && len) {
	reset_keystroke(inpinfo, iccf);
        inpinfo->cch_publish.wch = (wchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
        return IMKEY_ABSORB;
    }

    else if (keysym == XK_space) {
        inpinfo->cch_publish.wch = (wchar_t)0;

	if ((cf->mode & INP_MODE_SPACEAUTOUP) && 
	    (inpinfo->n_mcch > 1 || inpinfo->mcch_pgstate != MCCH_ONEPG)) {
            if ((mcch_choosech(cf, inpinfo, iccf, -1)))
                return IMKEY_COMMIT;
            else {
                if ((cf->mode & INP_MODE_AUTORESET))
                    reset_keystroke(inpinfo, iccf);
                else
                    iccf->mode |= INPINFO_MODE_WRONG;
                return return_wrong(cf);
            }
	}
	else if ((iccf->mode & INPINFO_MODE_MCCH))
	    return mcch_nextpage(cf, inpinfo, iccf, ' ');
        else if ((cf->mode & INP_MODE_SPACERESET) && inp_wrong) {
            reset_keystroke(inpinfo, iccf);
            return IMKEY_ABSORB;
        }
        else if (sp_ignore)
            return IMKEY_ABSORB;
	else if (inpinfo->keystroke_len)
	    return commit_keystroke(cf, inpinfo, iccf);
    }

    else if ((keysym >= XK_KP_0 && keysym <= XK_KP_9) || 
	     (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide))
	return IMKEY_IGNORE;

    else if (keyinfo->keystr_len == 1) {
        unsigned int ret=IMKEY_ABSORB, ret1=0;
	int selkey_idx, endkey_pressed=0;
        char keycode, *s;
        wch_t wch;

        inpinfo->cch_publish.wch = (wchar_t)0;
	keycode = key2code(keystr[0]);
	wch.wch = cf->header.keyname[(int)keycode].wch;
	selkey_idx = ((s = strchr(cf->header.selkey, keystr[0]))) ? 
		(int)(s - cf->header.selkey) : -1;
	if (cf->header.n_endkey && 
	    strchr(cf->header.endkey, iccf->keystroke[len-1]))
	    endkey_pressed = 1;

	if (len && selkey_idx != -1 && (endkey_pressed || ! wch.wch)) {
	    /* Don't enter the multi-cch selection, but selkey pressed. */
	    if (len==1 && cf->disable_sel_list && 
		strchr(cf->disable_sel_list, iccf->keystroke[len-1]))
		wch.s[0] = keystr[0];
	    else
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	}
	else if ((keystr[0]=='<' || keystr[0]=='>') &&
		 (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	    return mcch_nextpage(cf, inpinfo, iccf, keystr[0]);
	else if ((iccf->mode & INPINFO_MODE_MCCH)) {
	    /* Enter the multi-cch selection. */
	    if (selkey_idx != -1)
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	    else if ((cf->mode & INP_MODE_AUTOUPCHAR)) {
		if (! mcch_choosech(cf, inpinfo, iccf, -1))
		    return return_wrong(cf);
		ret |= IMKEY_COMMIT;
	    }
	    else
		return return_wrong(cf);
	}

	/* The previous cch might be committed, so len might be 0 */
	len = inpinfo->keystroke_len;

	if ((keyinfo->keystate & ShiftMask)) {
	    if (! (cf->mode & INP_MODE_WILDON) || 
		(keystr[0] != '*' && keystr[0] != '?'))
		return (ret | modifier_escape(cf, QPHR_SHIFT));
	    else
	        iccf->mode |= INPINFO_MODE_INWILD;
	}
	else if ((keyinfo->keystate & ControlMask) &&
		 (ret1 = modifier_escape(cf, QPHR_CTRL)))
	    return (ret | ret1);
	else if ((keyinfo->keystate & Mod1Mask) &&
		 (ret1 = modifier_escape(cf, QPHR_ALT)))
	    return (ret | ret1);
	else if (! wch.wch)
	    return (ret | IMKEY_IGNORE);
        else if (len >= max_len)
            return return_wrong(cf);

        iccf->keystroke[len] = keystr[0];
        iccf->keystroke[len+1] = '\0';
	if (keystr[0] == '*' || keystr[0] == '?') {
	    inpinfo->s_keystroke[len].s[0] = keystr[0];
	    inpinfo->s_keystroke[len].s[1] = ' ';
	}
	else
            inpinfo->s_keystroke[len].wch = wch.wch;
        inpinfo->s_keystroke[len+1].wch = (wchar_t)0;
        inpinfo->keystroke_len ++;
        len ++;

	if ((cf->mode & INP_MODE_SPACEIGNOR) && len == max_len)
	    iccf->mode |= INPINFO_MODE_SPACE;
	if ((cf->mode & INP_MODE_ENDKEY) && len>1 &&
		 (s=strchr(cf->header.endkey, keystr[0])))
	    return commit_keystroke(cf, inpinfo, iccf);
	else if ((cf->mode & INP_MODE_AUTOFULLUP) && len == max_len)
	    return commit_keystroke(cf, inpinfo, iccf);
	else if ((cf->mode & INP_MODE_AUTOCOMPOSE))
	    match_keystroke(cf, inpinfo, iccf);

	return ret;
    }

    return IMKEY_IGNORE;
}

static int 
gen_inp_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    int i, idx;
    wchar_t tmp;
    char *k, keystroke[INP_CODE_LENGTH+1];
    static wch_t keystroke_list[INP_CODE_LENGTH+1];

    if ((i = ccode_to_idx(&(simdinfo->cch_publish))) == -1)
        return False;

    if ((idx = cf->ichar[i]) == ICODE_IDX_NOTEXIST)
	return False;
    if (cf->header.icode_mode == ICODE_MODE1)
	codes2keys(&(cf->ic1[idx]), 1, keystroke, SELECT_KEY_LENGTH+1);
    else if (cf->header.icode_mode == ICODE_MODE2) {
        unsigned int klist[2];

	klist[0] = cf->ic1[idx];
	klist[1] = cf->ic2[idx];
	codes2keys(klist, 2, keystroke, SELECT_KEY_LENGTH+1);
    }
    for (i=0, k=keystroke; i<INP_CODE_LENGTH && *k; i++, k++) {
	idx = key2code(*k);
	if ((tmp = cf->header.keyname[idx].wch))
	    keystroke_list[i].wch = tmp;
	else {
	    keystroke_list[i].wch = (wchar_t)0;
	    keystroke_list[i].s[0] = '?';
	}
    }
    keystroke_list[i].wch = (wchar_t)0;
    simdinfo->s_keystroke = keystroke_list;

    return (i) ? True : False;
}

/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *gen_inp_valid_objname[] = { "*", NULL };

static char gen_inp_comments[] = N_(
	"This is the general input method module. It is the most generic\n"
	"module which can read the IM table (\".tab\" file) and perform\n"
	"the correct operations of the specific IM. It also can accept a\n"
	"lot of options from the \"xcinrc\" file to detailly control the\n"
	"behavior of the IM. We hope that this module could match most of\n"
	"your Chinese input requirements.\n\n"
	"This module is free software, as part of xcin system.\n");

#ifdef USE_DYNAMIC
module_t module_ptr = {
#else
module_t gen_inp_module_ptr = {
#endif
    "gen_inp",					/* name */
    MODULE_VERSION,				/* version */
    gen_inp_comments,				/* comments */
    gen_inp_valid_objname,			/* valid_objname */
    MOD_CINPUT,					/* module_type */
    sizeof(gen_inp_conf_t),			/* conf_size */
    gen_inp_init,				/* init */
    gen_inp_xim_init,				/* xim_init */
    gen_inp_xim_end,				/* xim_end */
    gen_inp_keystroke,				/* keystroke */
    gen_inp_show_keystroke,			/* show_keystroke */
    NULL					/* terminate */
};
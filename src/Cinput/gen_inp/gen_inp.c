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

#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#ifdef HAVE_LIBTABE
#include <tabe.h>
#include <iconv.h>
#include <errno.h>
#endif
#include "xcintool.h"
#include "module.h"
#include "gen_inp.h"
#define XCIN_BYTE_NATIVE   2
#define XCIN_BYTE_UTF8     3

/*----------------------------------------------------------------------------

        gen_inp_init()

----------------------------------------------------------------------------*/

static int
loadtab(gen_inp_conf_t *cf, FILE *fp, char *encoding)
{
    int i, j;
    int n, nn, ret=1;
    char modID[MODULE_ID_SIZE];
    /* KhoGuan add */
    int n_tsi = 0;

    if (fread(modID, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(modID, "gencin")) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: invalid tab file.\n"),cf->tabfn);
	return False;
    }
    if (fread(&(cf->header), sizeof(cintab_head_t), 1, fp) != 1) {
	perr(XCINMSG_WARNING, N_("gen_inp: %s: reading header error.\n"), cf->tabfn);
	return False;
    }
    if (strcmp(GENCIN_VERSION, cf->header.version) > 0) {
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
    cf->icidx = xcin_malloc(sizeof(icode_idx_t) * n, 0);
    cf->ichar = xcin_malloc(sizeof(ichar_t) * nn, 0);
    if(ret && n)
	ret = (fread(cf->icidx, sizeof(icode_idx_t), n, fp)==n);
    if(ret && nn)
	ret = (fread(cf->ichar, sizeof(ichar_t), nn, fp)==nn);
    for(i=0; i<cf->header.icode_mode; i++) {
	cf->ic[i] = xcin_malloc(sizeof(icode_t) * n, 0);
	if(fread(cf->ic[i], sizeof(icode_t), n, fp) != n) {
	    if(n) {
		free(cf->icidx);
		for(j=0; j<=i; j++)
		    free(cf->ic[j]);
	    }
	    if(nn)
		free(cf->ichar);
	    ret = 0;
	    break;
	}
    }

    /* KhoGuan: add support of phrase input.
       read further for phrase data appended to the cin table */
    n_tsi = cf->header.n_tsi;
    if (ret && n_tsi > 0) {
        cf->tsi_idx = xcin_malloc(sizeof(unsigned int) * n_tsi, 0);
        if (fread(cf->tsi_idx, sizeof(unsigned int), n_tsi, fp) != n_tsi) {
            free(cf->tsi_idx);
            ret = 0;
        }
        n = cf->tsi_idx[n_tsi-1];
        cf->tsi_list = xcin_malloc(sizeof(icode_idx_t) * n, 0);
        if ((n=fread(cf->tsi_list, sizeof(icode_idx_t), n, fp)) != cf->tsi_idx[n_tsi-1]) {
            free(cf->tsi_list);
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
    cf->kremap = xcin_malloc(sizeof(kremap_t) * i, 0);

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
gen_inp_resource(gen_inp_conf_t *cf, xcin_rc_t *xrc, char *objname, char *ftsi)
{
    char *cmd[2], value[256];

    cmd[0] = objname;

    cmd[1] = "INP_CNAME";				/* inp_names */
    if (get_resource(xrc, cmd, value, 256, 2)) {
	if (cf->inp_cname)
	    free(cf->inp_cname);
        cf->inp_cname = (char *)strdup(value);
    }
    cmd[1] = "AUTO_COMPOSE";				/* auto compose */
    if (get_resource(xrc, cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOCOMPOSE, 0);

    cmd[1] = "AUTO_UPCHAR";				/* auto_up char */
    if (get_resource(xrc, cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOUPCHAR, 0);
    cmd[1] = "SPACE_AUTOUP";				/* space key auto_up */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACEAUTOUP, 0);
    cmd[1] = "SELKEY_SHIFT";				/* selkey shift */
    if (get_resource(xrc, cmd, value, 256, 2))	
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SELKEYSHIFT, 0);

    cmd[1] = "AUTO_FULLUP";				/* auto full_up */
    if (get_resource(xrc, cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTOFULLUP, 0);
    cmd[1] = "SPACE_IGNORE";				/* space ignore */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACEIGNOR, 0);

    cmd[1] = "AUTO_RESET";				/* auto reset error */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_AUTORESET, 0);
    cmd[1] = "SPACE_RESET";				/* space reset error */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SPACERESET, 0);

    cmd[1] = "SINMD_IN_LINE1";				/* sinmd in line1 mode*/
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_SINMDLINE1, 0);
    cmd[1] = "WILD_ENABLE";				/* wild key enable */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_WILDON, 0);
    cmd[1] = "BEEP_WRONG";				/* beep mode: wrong */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_BEEPWRONG, 0);
    cmd[1] = "BEEP_DUPCHAR";				/* beep mode: dupchar */
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_BEEPDUP, 0);
    cmd[1] = "QPHRASE_MODE";				/* qphrase mode */
    if (get_resource(xrc, cmd, value, 256, 2))
	cf->modesc = (ubyte_t)(atoi(value) % 256);

    cmd[1] = "DISABLE_SEL_LIST";			/* disable selkey */
    if (get_resource(xrc, cmd, value, 256, 2)) {
	if (cf->disable_sel_list)
	    free(cf->disable_sel_list);
	if (strcmp(value, "NONE"))
	    cf->disable_sel_list = (char *)strdup(value);
	else
	    cf->disable_sel_list = NULL;
    }
    cmd[1] = "KEYSTROKE_REMAP";
    if (get_resource(xrc, cmd, value, 256, 2)) {
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
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_ENDKEY, 0);

    cmd[1] = "HINT_SELECT";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_HINTSEL, 0);
    cmd[1] = "HINT_TSI";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_HINTTSI, 0);
    cmd[1] = "TSI_FNAME";
    if (get_resource(xrc, cmd, value, 256, 2))
	strcpy(ftsi, value);

/* KhoGuan add */
    cmd[1] = "TAB_NEXTPAGE";			/* tab to next candidate page */
    if (get_resource(xrc, cmd, value, 256, 2))
        set_data(&(cf->mode), RC_IFLAG, value, INP_MODE_TABNEXTPAGE, 0);
}

static int
gen_inp_init(void *conf, char *objname, xcin_rc_t *xrc)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf, cfd;
    char *s, value[128], truefn[256], sub_path[256], ftsi[256];
    objenc_t objenc;
    FILE *fp;
    int ret;

    memset(&cfd, 0, sizeof(gen_inp_conf_t));
    if (get_objenc(objname, &objenc) == -1)
	return False;
    ftsi[0] = '\0';
    gen_inp_resource(&cfd, xrc, "gen_inp_default", ftsi);
    gen_inp_resource(&cfd, xrc, objenc.objloadname, ftsi);
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
    if ((cfd.mode & INP_MODE_TABNEXTPAGE))
	cf->mode |= INP_MODE_TABNEXTPAGE;

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
/* KhoGuan add 2 lines */
    else
        snprintf(value, 50, "%s", cf->inp_ename);

    snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
    if (check_datafile(value, sub_path, xrc, truefn, 256) == True) {
	cf->tabfn = (char *)strdup(truefn);
	if ((fp = open_file(truefn, "rb", XCINMSG_WARNING)) == NULL)
	    return False;
        ret = loadtab(cf, fp, objenc.encoding);
	fclose(fp);
    }
    else
	return False;

    if (cf->header.n_endkey) {
	if ((cfd.mode & INP_MODE_ENDKEY))
	    cf->mode |= INP_MODE_ENDKEY;
    }

#ifdef HAVE_LIBTABE
    if ( strcasecmp(xrc->locale.encoding,"utf-8")==0 )
	cf->codeset=XCIN_BYTE_UTF8;
    else
	cf->codeset=XCIN_BYTE_NATIVE;
    if (cfd.mode & (INP_MODE_HINTSEL | INP_MODE_HINTTSI)) {
	snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
	if (check_datafile(ftsi, sub_path, xrc, truefn, 256) == True) {
	    cf->tsidb = tabeTsiDBOpen(DB_TYPE_DB, truefn,
			DB_FLAG_READONLY|DB_FLAG_SHARED|DB_FLAG_NOUNPACK_YIN);
	    if (cf->tsidb == NULL)
		perr(XCINMSG_WARNING,
		    N_("gen_inp: cannot open tsi db file: %s\n"), ftsi);
	    else {
		if (cfd.mode & INP_MODE_HINTSEL)
		    cf->mode |= INP_MODE_HINTSEL;
		if (cfd.mode & INP_MODE_HINTTSI)
		    cf->mode |= INP_MODE_HINTTSI;
	    }
	}
    }
    else
	cf->tsidb = NULL;
#else
    if (cfd.mode & (INP_MODE_HINTSEL | INP_MODE_HINTTSI)) {
	perr(XCINMSG_WARNING,
	    N_("gen_inp: xcin complied without libtabe, HINTTSI & HINTSEL disabled.\n"));
    }
#endif

    return ret;
}


/*----------------------------------------------------------------------------

        gen_inp_xim_init()

----------------------------------------------------------------------------*/

static int num_candi_bufsz = NUM_CANDI_BUFSZ;

static int
gen_inp_xim_init(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf;
    int i;

    inpinfo->iccf = xcin_malloc(sizeof(gen_inp_iccf_t), 1);
    iccf = (gen_inp_iccf_t *)inpinfo->iccf;
    iccf->all_index = xcin_malloc(sizeof(int)*num_candi_bufsz, 1);
    iccf->all_grouping = xcin_malloc(sizeof(ubyte_t)*num_candi_bufsz + 1, 1);

    inpinfo->inp_cname = cf->inp_cname;
    inpinfo->inp_ename = cf->inp_ename;
    inpinfo->area3_len = cf->header.n_max_keystroke * 2 + 1;
    inpinfo->guimode = ((cf->mode & INP_MODE_SINMDLINE1)) ? 
			GUIMOD_SINMDLINE1 : 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = xcin_malloc((INP_CODE_LENGTH+1)*sizeof(wch_t), 1);
    inpinfo->suggest_skeystroke =
		xcin_malloc((INP_CODE_LENGTH+1)*sizeof(wch_t), 1);

    if (! (cf->mode & INP_MODE_SELKEYSHIFT)) {
	inpinfo->n_selkey = cf->header.n_selkey;
	inpinfo->s_selkey = xcin_malloc(inpinfo->n_selkey*sizeof(wch_t), 1);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i].s[0] = cf->header.selkey[i];
    }
    else {
	inpinfo->n_selkey = cf->header.n_selkey+1;
	inpinfo->s_selkey = xcin_malloc(inpinfo->n_selkey*sizeof(wch_t), 1);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i+1].s[0] = cf->header.selkey[i];
    }
    inpinfo->n_mcch = 0;
#ifdef WITH_LIBTABE
    i = (cf->tsidb) ? HINTSZ : inpinfo->n_selkey;
#else
    i = inpinfo->n_selkey;
#endif
    if (cf->header.n_tsi > 0 && i<MAX_TSI_LEN*inpinfo->n_selkey) {
	i = MAX_TSI_LEN * inpinfo->n_selkey;
    }
    inpinfo->mcch = xcin_malloc(i * sizeof(wch_t), 1);
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

/* KhoGuan trace */
    if (iccf->n_mcch_list) {
	free(iccf->mcch_list);
    }
    if (iccf->n_mkey_list) {
	free(iccf->mkey_list);
    }
    free(iccf->all_index);
    free(iccf->all_grouping);
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

	gen_inp_keystroke()

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
    if (iccf->n_mkey_list) {
	free(iccf->mkey_list);
	iccf->n_mkey_list = 0;
    }
}


/*------------------------------------------------------------------------*/

static int
cmp_icvalue(icode_t *ic[MAX_ICODE_MODE], unsigned int idx,
	    icode_t icode[MAX_ICODE_MODE], int mode)
{
    int i;
    for(i=0; i<mode; i++)
	if(ic[i][idx] > icode[i])
	    return 1;
	else if(ic[i][idx] < icode[i])
	    return -1;
    return 0;
}

static int 
bsearch_char(icode_t *ic[MAX_ICODE_MODE], icode_t icode[MAX_ICODE_MODE], 
	int size, int mode, int wild)
{
    int head, middle, end, ret;

    head   = 0;
    middle = size / 2;
    end    = size;
    while ((ret=cmp_icvalue(ic, middle, icode, mode))) {
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
	      ! cmp_icvalue(ic, middle-1, icode, mode)) 
	    middle --;
	return middle;
    }
    else
	return (wild) ? middle : -1;
}

static int
ccode_to_char_or_tsi(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf,
                     icode_idx_t ic, wch_t *mcch, int n_ich, int *n_zi_p);

static int
pick_cch_wild(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf, int *head, byte_t dir,
		wch_t *mcch, unsigned int mcch_size, unsigned int *n_ich)
{
    unsigned int i, size;
    int j;
    unsigned int klist[MAX_ICODE_MODE];

    int n_klist, ks_size, r=0, idx;
    char *ks;
/* KhoGuan add */
    int n_zi = 0;

    size = cf->header.n_icode;
    ks_size = cf->header.n_max_keystroke + 1;
    ks = xcin_malloc(ks_size, 0);
    n_klist = cf->header.icode_mode;
    dir = (dir > 0) ? (byte_t)1 : (byte_t)-1;

    if (iccf->n_mkey_list)
	free(iccf->mkey_list);
    iccf->mkey_list = xcin_malloc(mcch_size*sizeof(int), 0);

    memset(iccf->all_index, 0, sizeof(int)*num_candi_bufsz);
    memset(iccf->all_grouping, 0, sizeof(ubyte_t)*(1+num_candi_bufsz));

    for (i=0, idx = *head; idx>=0 && idx<size && i<=mcch_size; idx+=dir) {
	for(j=0; j<n_klist; j++)
	    klist[j] = cf->ic[j][idx];
	codes2keys(klist, n_klist, ks, ks_size);

	if (strcmp_wild(iccf->keystroke, ks) == 0) {
	    if (i < mcch_size) {
		/* ccode_to_char(cf->icidx[idx], mcch[i].s, WCH_SIZE); */
                ccode_to_char_or_tsi(cf, iccf, cf->icidx[idx], mcch, i, &n_zi);
		iccf->mkey_list[i] = idx;
		*head = idx;
		i ++;
	    }
	    else
		r = 1;
	}
    }
    free(ks);
    *n_ich = iccf->all_grouping[0] = i;
    iccf->n_mkey_list = i;
    return r;
}

static int
match_keystroke_wild(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    int i;
    icode_t icode[MAX_ICODE_MODE];
    unsigned int md, n_ich;
    int idx;
    char *s1, *s2, *s, tmpch;

    md = cf->header.icode_mode;
    for(i=0; i<md; i++)
	icode[i] = 0;

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
    iccf->has_tsi = 0;
    memset(iccf->all_index, 0, sizeof(int)*num_candi_bufsz);
    memset(iccf->all_grouping, 0, sizeof(ubyte_t)*(1+num_candi_bufsz));
    inpinfo->mcch_grouping = NULL;

    if (pick_cch_wild(cf, iccf, &idx, 1,
	    inpinfo->mcch, inpinfo->n_selkey, &n_ich) == 0)
        inpinfo->mcch_pgstate = MCCH_ONEPG;
    else 
	inpinfo->mcch_pgstate = MCCH_BEGIN;

    if (n_ich) {
        inpinfo->n_mcch = 
          (unsigned short)(iccf->all_index[n_ich-1]+iccf->all_grouping[n_ich]);
        if (iccf->has_tsi)
            inpinfo->mcch_grouping = iccf->all_grouping;
    }
    else
        inpinfo->n_mcch = 0;

    iccf->mcch_eidx = idx;		/* refer to end index of cf->icidx */
    return (n_ich) ? 1 : 0;
}

static int
ccode_to_char_or_tsi(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf, icode_idx_t ic, 
                     wch_t *mcch, int n_ich, int *n_zi_p)
{
    int i, tsi_num, tsi_beg, tsi_len;

    if (ic >= 0 && ic < cf->ccinfo.total_char) {
        if (! ccode_to_char(ic, mcch[*n_zi_p].s, WCH_SIZE))
            return 0;
        iccf->all_grouping[n_ich+1] = 1;
        iccf->all_index[n_ich] = (n_ich == 0)?  0 : 
                    (iccf->all_index[n_ich-1] + iccf->all_grouping[n_ich]);
        (*n_zi_p)++;
    }
    else if (ic >= FIRST_TSI_NUM) {
        tsi_num = ic - FIRST_TSI_NUM;
        if (tsi_num == 0) {
            tsi_beg = 0;
            tsi_len = cf->tsi_idx[0];
        }
        else {
            tsi_beg = cf->tsi_idx[tsi_num-1];
            tsi_len = cf->tsi_idx[tsi_num] - cf->tsi_idx[tsi_num-1];
        }
        for (i = 0; i < tsi_len; i++) {

            ic = cf->tsi_list[tsi_beg+i];
            if (ic < 0 && ic > INVALID_ICODE_IDX) {
                mcch[*n_zi_p].s[0] = (unsigned char)(ic * -1);
            }
            else if (ic >= 0 && ic < cf->ccinfo.total_char) {
                if (! ccode_to_char(ic, mcch[*n_zi_p].s, WCH_SIZE))
                    return 0;
            }
            else {
                perr(XCINMSG_WARNING, "Input table error.\n");
                return 0;
            }
                (*n_zi_p)++;
        }
        iccf->all_grouping[n_ich+1] = tsi_len;
        iccf->all_index[n_ich] = (n_ich == 0)?  0 : 
                      (iccf->all_index[n_ich-1] + iccf->all_grouping[n_ich]);
        iccf->has_tsi = 1;
    }
    else {
        perr(XCINMSG_WARNING, "Input table error.\n");
        return 0;
    }
   return 1;
}


static int
match_keystroke_normal(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    icode_t icode[MAX_ICODE_MODE];
    unsigned int size, md, n_ich=0, n_zi = 0, mcch_size;
    int idx, i, j;
    wch_t *mcch; 

    static ubyte_t mcch_grouping_buf[1+SELECT_KEY_LENGTH]; /* add 1 for SELKEY_SHIFT */
    memset(mcch_grouping_buf, 0, sizeof(mcch_grouping_buf));

    size = cf->header.n_icode;
    md = cf->header.icode_mode;
    for(i=0; i<md; i++)
	icode[i] = 0;

    /*
     *  Search for the first char.
     */
    keys2codes(icode, MAX_ICODE_MODE, iccf->keystroke);
    if ((idx = bsearch_char(cf->ic, icode, size, md, 0)) == -1)
	return 0;

    /*
     *  Search for all the chars with the same keystroke.
     */
/* KhoGuan rev
    mcch_size = inpinfo->n_selkey; */
    mcch_size = inpinfo->n_selkey * MAX_TSI_LEN;
    mcch = xcin_malloc(mcch_size * sizeof(wch_t), 1);

    iccf->has_tsi = 0;
/* KhoGuan note: iccf->all_index/_grouping are initially allocated in
   gen_inp_xim_init(), and will be realloc in this function if need. */
    memset(iccf->all_index, 0, sizeof(int)*num_candi_bufsz);
    memset(iccf->all_grouping, 0, sizeof(ubyte_t)*(1+num_candi_bufsz));

    do {
	if ((mcch_size - MAX_TSI_LEN) <= n_zi) {
	    mcch_size *= 2;
	    mcch = xcin_realloc(mcch, mcch_size * sizeof(wch_t));
	}
        if (n_ich >= num_candi_bufsz) {
            int old_num_candi_bufsz = num_candi_bufsz;
            num_candi_bufsz += NUM_CANDI_BUFSZ;
            perr(XCINMSG_NORMAL, 
                 "Too many candidates(%d), realloc all_index and all_grouping array size from %d to %d\n",
                  n_ich+1, old_num_candi_bufsz, num_candi_bufsz);

            iccf->all_index = xcin_realloc(iccf->all_index, 
                                           sizeof(int)*num_candi_bufsz);
            memset((iccf->all_index + old_num_candi_bufsz), 0, 
                   sizeof(int) * NUM_CANDI_BUFSZ);

            iccf->all_grouping = xcin_realloc(iccf->all_grouping, 
                                            1+sizeof(ubyte_t)*num_candi_bufsz);
            memset((iccf->all_grouping + 1 + old_num_candi_bufsz), 0,
                   sizeof(ubyte_t) * NUM_CANDI_BUFSZ);
        }
        if (! ccode_to_char_or_tsi(cf, iccf, cf->icidx[idx], 
                                   mcch, n_ich, &n_zi))
            return 0;
/*
	if (! ccode_to_char(cf->icidx[idx], mcch[n_ich].s, WCH_SIZE))
	    return 0;
*/
	n_ich ++;
        idx ++;
    } while (idx < size &&
             ! cmp_icvalue(cf->ic, idx, icode, md));

    /*
     *  Prepare mcch for display.
     */
/* KhoGuan rev
    for (idx=0; idx<inpinfo->n_selkey && idx<n_ich; idx++)
	inpinfo->mcch[idx].wch = mcch[idx].wch;
    inpinfo->n_mcch = idx;
*/
    iccf->all_grouping[0] = n_ich;

    for (idx=0, i=0; idx<inpinfo->n_selkey && idx<n_ich; idx++) {
        if (iccf->has_tsi) {
            for (j=0; j < iccf->all_grouping[idx+1]; i++, j++) {
	        inpinfo->mcch[i].wch = mcch[i].wch;
            }
            mcch_grouping_buf[idx+1] = iccf->all_grouping[idx+1];
        }
        else {
	    inpinfo->mcch[i].wch = mcch[i].wch;
            ++i;
        }
    }
    inpinfo->n_mcch = i;
    mcch_grouping_buf[0] = idx;

    if (iccf->has_tsi) {
        inpinfo->mcch_grouping = mcch_grouping_buf;
    }
    else
        inpinfo->mcch_grouping = NULL;

    if (idx >= n_ich) {
        inpinfo->mcch_pgstate = MCCH_ONEPG;
	free(mcch);
    }
    else {
	inpinfo->mcch_pgstate = MCCH_BEGIN;
	if (iccf->n_mcch_list)
	    free(iccf->mcch_list);
        iccf->mcch_list = mcch;
/* KhoGuan rev
        iccf->n_mcch_list = n_ich; */
        iccf->n_mcch_list = n_zi;

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
get_correct_skeystroke(gen_inp_conf_t *cf,
		inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx, wch_t *cch)
{
/* KhoGuan rev
    unsigned int i=0, j, klist[2]; */
    unsigned int i=0, j;
    unsigned int klist[MAX_ICODE_MODE];

    int n_klist, ks_size, keycode;
    char *ks;

    if (idx < iccf->n_mkey_list)
	i = iccf->mkey_list[idx];
    else {
	inpinfo->suggest_skeystroke[0].wch = (wchar_t)0;
	return;
    }
    ks_size = cf->header.n_max_keystroke + 1;
    ks = xcin_malloc(ks_size, 0);
    n_klist = cf->header.icode_mode;
    for(j=0; j<n_klist; j++)
	klist[j] = cf->ic[j][i];
    codes2keys(klist, n_klist, ks, ks_size);
    if (strcmp_wild(iccf->keystroke, ks) == 0) {
	for (j=0; ks[j] != '\0'; j++) {
	    keycode = key2code(ks[j]);
	    inpinfo->suggest_skeystroke[j].wch =
			cf->header.keyname[keycode].wch;
	}
	inpinfo->suggest_skeystroke[j].wch = (wchar_t)0;
    }
    else
	inpinfo->suggest_skeystroke[0].wch = (wchar_t)0;
    free(ks);
}

static void
commit_char(gen_inp_conf_t *cf,
	    inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx, wch_t *cch)
{
/* KhoGuan rev
    static char cch_s[WCH_SIZE+1]; */
    static char cch_s[WCH_SIZE*MAX_TSI_LEN+1];
    char *s=NULL;
    int i;

    memset(cch_s, 0, WCH_SIZE*MAX_TSI_LEN+1);
    inpinfo->cch = cch_s;

    if (! iccf->has_tsi) {
        strncpy(cch_s, (char *)cch->s, WCH_SIZE);
        cch_s[WCH_SIZE] = '\0';
    }
    else {
        for (i = 0; i < inpinfo->mcch_grouping[idx+1]; i++) {
            strncat(cch_s, cch[i].s, WCH_SIZE);
        }
    }
    if (! (s = strchr(iccf->keystroke,'*')) &&
	! (s = strchr(iccf->keystroke,'?'))) {
	for (i=0; i<=inpinfo->keystroke_len; i++)
	    inpinfo->suggest_skeystroke[i].wch = inpinfo->s_keystroke[i].wch;
    }
    else
	get_correct_skeystroke(cf, inpinfo, iccf, idx, cch);
    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke[0].wch = (wchar_t)0;
    inpinfo->n_mcch = 0;
    inpinfo->cch_publish.wch = cch->wch;
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    iccf->mode &= ~(INPINFO_MODE_MCCH);
    iccf->mode &= ~(INPINFO_MODE_INWILD);
/* KhoGuan add */
    iccf->has_tsi = 0;
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
		commit_char(cf, inpinfo, iccf, i, &(cf->kremap[i].wch));
		return IMKEY_COMMIT;
	    }
	}
    }

    if (match_keystroke(cf, inpinfo, iccf)) {
/* KhoGuan rev
        if (inpinfo->n_mcch == 1) { */
        if (iccf->all_grouping[0] == 1) {
/* KhoGuan debug
            commit_char(cf, inpinfo, iccf, 1, inpinfo->mcch); */
            commit_char(cf, inpinfo, iccf, 0, inpinfo->mcch);
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
    int min, cch_beg = 0;
    wch_t wch;
    wch_t *wch_p;

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
/* KhoGuan add */
    if (iccf->has_tsi) {
        if (! (iccf->mode & INPINFO_MODE_INWILD))
            cch_beg = iccf->all_index[iccf->mcch_hidx+idx] - 
                      iccf->all_index[iccf->mcch_hidx];
        else {
            cch_beg = iccf->all_index[idx];
        }
        wch_p = &(inpinfo->mcch[cch_beg]);
        commit_char(cf, inpinfo, iccf, idx, wch_p);
    }
    else {
        wch.wch = inpinfo->mcch[idx].wch;
        commit_char(cf, inpinfo, iccf, idx, &wch);
    }
    reset_keystroke(inpinfo, iccf);

    return 1;
}

static int
fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo, 
	 gen_inp_iccf_t *iccf, byte_t dir)
{
    int i, j, k, m;
    int n_pg=inpinfo->n_selkey;
    int tsi_beg, tsi_len;

    if (! (iccf->mode & INPINFO_MODE_INWILD)) {
        int total_zi = iccf->n_mcch_list;
        int total_candi = iccf->all_grouping[0];

/* KhoGuan note
   For non-wild mode, iccf->mcch_hidx is used as the index, among all the candidates,
   of the first candidate at the current page. For wild mode, the use is different,
   see the following else section for wild mode and match_keystroke_wild().
*/
        switch (dir) {
        case 0:
	    iccf->mcch_hidx = 0;
	    break;
        case 1:
	    if (iccf->mcch_hidx + n_pg < total_candi)
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
        if (iccf->has_tsi) {
            inpinfo->mcch_grouping[0] = 0;	/* reinitialize */
            tsi_beg = iccf->all_index[iccf->mcch_hidx];
            tsi_len = iccf->all_grouping[iccf->mcch_hidx+1];
            i = 0;
            for (j = 0; j < n_pg && (iccf->mcch_hidx + j) < total_candi; j++) {
                for (k = 0; k< tsi_len; k++) {
                    inpinfo->mcch[i].wch = iccf->mcch_list[tsi_beg+k].wch;
                    i++;
                }
                inpinfo->mcch_grouping[0]++;
                inpinfo->mcch_grouping[j+1] = tsi_len;

                tsi_beg = iccf->all_index[iccf->mcch_hidx+j+1];
                tsi_len = iccf->all_grouping[1+iccf->mcch_hidx+j+1];
            }
        }
        else {
	    for (i=0, j=iccf->mcch_hidx; i<n_pg && j<total_zi; i++, j++)
	        inpinfo->mcch[i].wch = iccf->mcch_list[j].wch;
        }

        if (iccf->mcch_hidx == 0)
            inpinfo->mcch_pgstate = (i < total_zi) ? MCCH_BEGIN : MCCH_ONEPG;
        else if (total_candi - iccf->mcch_hidx > n_pg)
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
        else
	    inpinfo->mcch_pgstate = MCCH_END;
        inpinfo->n_mcch = i;
    }
    else { /* wild mode */
/* KhoGuan rev
	int r=0, n_mcch, hidx, eidx; */
	int r=0, n_ich, hidx, eidx;
	wch_t mcch[SELECT_KEY_LENGTH * MAX_TSI_LEN];
/* KhoGuan add */
        static ubyte_t mcch_grouping[SELECT_KEY_LENGTH + 1];
        static int mcch_index[SELECT_KEY_LENGTH];

        memset(mcch_grouping, 0, sizeof(mcch_grouping));
        memset(mcch_index, 0, sizeof(mcch_index));

        switch (dir) {
        case 0:
	    return 0;
        case 1:
	    if (inpinfo->mcch_pgstate == MCCH_BEGIN ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE) {
                iccf->has_tsi = 0;
                inpinfo->mcch_grouping = NULL;
                inpinfo->n_mcch = 0;
		hidx = eidx = iccf->mcch_eidx + 1;
        	r = pick_cch_wild(cf, iccf, &eidx, 1, inpinfo->mcch, 
			inpinfo->n_selkey, (unsigned int *)&n_ich);
                if (iccf->has_tsi)
                    inpinfo->mcch_grouping = iccf->all_grouping;
                inpinfo->n_mcch = iccf->all_index[n_ich-1] + 
                                  iccf->all_grouping[n_ich];
	    }
	    else
		return 0;
	    break;
        case -1:
	    if (inpinfo->mcch_pgstate == MCCH_END ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE) {
                iccf->has_tsi = 0;
                inpinfo->mcch_grouping = NULL;
                inpinfo->n_mcch = 0;
		hidx = eidx = iccf->mcch_hidx - 1;
        	r = pick_cch_wild(cf, iccf, &hidx, -1,
	        	mcch, inpinfo->n_selkey, (unsigned int *)&n_ich);
/* KhoGuan note:
   For wild mode, iccf->all_index/grouping has only one page of candidates every time.
   And no matther iccf->has_tsi is 1 or 0, their data are used here.
*/
                m = 0;
	        for (i=0, j=n_ich-1; j>=0; i++, j--) {
		    /* inpinfo->mcch[i].wch = mcch[j].wch; */
                    tsi_beg = iccf->all_index[j];
                    tsi_len = iccf->all_grouping[j+1];
                    for (k=0; k < tsi_len; k++) {
                        inpinfo->mcch[m].wch = mcch[tsi_beg+k].wch;
                        ++m;
                    }
                    mcch_grouping[i+1] = tsi_len;
/* KhoGuan note
   all_index[] need be recalculated, use mcch_index as tmp buffer */
                    mcch_index[i] = (i == 0)?  0 : (mcch_index[i-1] + mcch_grouping[i]);
                }
                mcch_grouping[0] = iccf->all_grouping[0]; 
                if (iccf->has_tsi) {
                    inpinfo->mcch_grouping = mcch_grouping;
                }
                inpinfo->n_mcch = mcch_index[n_ich-1] + mcch_grouping[n_ich];
                for (i=0; i < mcch_grouping[0]; i++)
                    iccf->all_index[i] = mcch_index[i]; 
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
        /* inpinfo->n_mcch = n_mcch; */
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
	if ((cf->mode & INP_MODE_WILDON) &&
	    (strchr(iccf->keystroke, '*') || strchr(iccf->keystroke, '?')))
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
/* avoid wild mode auto up */
	    (! (iccf->mode & INPINFO_MODE_INWILD) ||
	       (iccf->mode & INPINFO_MODE_MCCH)) &&
/* end of wild mod */
	    (inpinfo->n_mcch > 1 || inpinfo->mcch_pgstate != MCCH_ONEPG)) {
            if (mcch_choosech(cf, inpinfo, iccf, -1))
                return IMKEY_COMMIT;
            else {
                if ((cf->mode & INP_MODE_AUTORESET))
                    reset_keystroke(inpinfo, iccf);
                else
                    iccf->mode |= INPINFO_MODE_WRONG;
                return return_wrong(cf);
            }
	}
	else if ((iccf->mode & INPINFO_MODE_MCCH)) {
            if ((cf->mode & INP_MODE_TABNEXTPAGE) &&
                (! (iccf->mode & INPINFO_MODE_INWILD) ||
                   (iccf->mode & INPINFO_MODE_MCCH)) &&
                (inpinfo->n_mcch > 1 || inpinfo->mcch_pgstate != MCCH_ONEPG)) {
                mcch_choosech(cf, inpinfo, iccf, -1);
                return IMKEY_COMMIT;
            }
            else
	        return mcch_nextpage(cf, inpinfo, iccf, ' ');
        }
        else if ((cf->mode & INP_MODE_SPACERESET) && inp_wrong) {
            reset_keystroke(inpinfo, iccf);
            return IMKEY_ABSORB;
        }
        else if (sp_ignore)
            return IMKEY_ABSORB;
	else if (inpinfo->keystroke_len)
	    return commit_keystroke(cf, inpinfo, iccf);
    }

/* KhoGuan add */
    else if ((keysym == XK_Tab) && (cf->mode & INP_MODE_TABNEXTPAGE)) {
        if ((iccf->mode & INPINFO_MODE_MCCH)) {
            return mcch_nextpage(cf, inpinfo, iccf, ' ');
        }
    }

    else if ((keysym >= XK_KP_0 && keysym <= XK_KP_9) || 
	     (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide))
	return IMKEY_IGNORE;
/* KhoGuan general case */
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


/*----------------------------------------------------------------------------

	Selection/Tsi hint, by Kuang-che Wu <kcwu@ck.tp.edu.tw>.

----------------------------------------------------------------------------*/

#ifdef HAVE_LIBTABE
static int my_mbstowcs(gen_inp_conf_t *cf, wch_t *wcs, const char *mbs, size_t nwcs)
{
    int i;
    size_t nb;
    if(cf->codeset == XCIN_BYTE_UTF8) {
	nb=3;
    } else {
	nb=2;
    }
    for(i=0; i<nwcs && *mbs; i++, mbs+=nb) {
	wcs[i].wch='\0';
	memcpy(wcs[i].s, mbs, nb);
    }
    return i;
}

static void
record_commit(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf, char *cmtstr)
{
    wch_t *history=iccf->commithistory;
    wch_t cmtwcs[HINTSZ];
    int len=my_mbstowcs(cf, cmtwcs, cmtstr, HINTSZ);

    if(len > HINTSZ)
	memcpy(history, cmtwcs+len-HINTSZ, HINTSZ*sizeof(wch_t));
    else {
	memmove(history, history+len, HINTSZ-len);
	memcpy(history+HINTSZ-len, cmtwcs, len*sizeof(wch_t));
    }
}

#define TSISZ	10
typedef struct candidate {
    int matchlen;
    long int ref;
    int len;
    wch_t str[TSISZ+1];
} candidate_t;

static int
insert_candidate(int ncandi, candidate_t candi[], candidate_t *one, int max)
{
    int i;

    for(i=0; i<ncandi; i++) {
	if(! (one->matchlen < candi[i].matchlen ||
	      one->ref <= candi[i].ref))
	    break;
	else if(candi[i].len == one->len &&
		memcmp(candi[i].str, one->str, one->len*sizeof(wch_t)) == 0)
	    return ncandi;
    }
    if(i < max) {
	memmove(candi+i+1, candi+i, sizeof(candi[0])*(max-i-1));
	memcpy(candi+i, one, sizeof(candidate_t));
	if(ncandi < max)
	    ncandi++;
    }
    return ncandi;
}

static int iconv_to_big5(gen_inp_conf_t *cf, const char *from, char *to, size_t to_len)
{
    if(cf->codeset == XCIN_BYTE_UTF8) {
	size_t from_len=strlen(from);
	int rtv = 0;
	iconv_t cd=iconv_open("BIG-5","UTF-8");
	if(iconv(cd, &from, &from_len, &to, &to_len)==(size_t)-1)
	    rtv = errno;
	if(to_len)
	    *to='\0';
	iconv_close(cd);
	return rtv;
    } else {
	strcpy(to, from);
    }
    return 0;
}

static int iconv_from_big5(gen_inp_conf_t *cf, const char *from, char *to, size_t to_len)
{
    if(cf->codeset == XCIN_BYTE_UTF8) {
	size_t from_len=strlen(from);
	int rtv = 0;
	iconv_t cd=iconv_open("UTF-8","BIG-5");
	if(iconv(cd, &from, &from_len, &to, &to_len)==(size_t)-1)
	    rtv = errno;
	if(to_len)
	    *to='\0';
	iconv_close(cd);
	return rtv;
    } else {
	strcpy(to, from);
    }
    return 0;
}

static int
may_next(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf, wch_t word)
{
    struct TsiInfo tsi;
    wch_t tmp[TSISZ+1];
    char tsi_str[1024];
    wch_t *histend;
    char tmp_mbs[TSISZ*4],tmp_mbs_big5[TSISZ*4];
    int match;
    int len;

    histend = iccf->commithistory+HINTSZ;
    tsi.tsi = (ZhiStr)tsi_str;
    for(match=TSISZ-1; match>=1; match--) {	/* Max Tsi len = TSISZ-1 */
	memcpy(tmp, histend-match, match*sizeof(wch_t));
	tmp[match]=word;
	tmp[match+1].wch = '\0';
	if(tmp[0].wch=='\0') continue;

	len=wchs_to_mbs(tmp_mbs, tmp, sizeof(tmp_mbs));
	if(iconv_to_big5(cf, tmp_mbs, tmp_mbs_big5, sizeof(tmp_mbs_big5))!=0)
	    continue;
	strcpy(tsi_str, tmp_mbs_big5);
	len=strlen(tsi_str);

	if (cf->tsidb->CursorSet(cf->tsidb, &tsi, 1) == 0) {
	    if (strncmp(tsi_str, tmp_mbs_big5, len) == 0)
		return True;
	}
    }
    return False;
}

static int
guess_next(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf,
	   candidate_t candi[], int maxcandi)
{
    struct TsiInfo tsi;
    wch_t *histend;
    char tsi_str[1024];
    wch_t matched_wc[128];
    char matched_mb[1024], matched_mb_big5[1024];
    int ncandi=0, match, guess, slen, errno;
    candidate_t one;
    int match_strlen;

    histend = iccf->commithistory+HINTSZ;
    tsi.tsi = (ZhiStr)tsi_str;
    for(match=TSISZ-1; match>=1; match--) {	/* Max Tsi len = TSISZ-1 */
	if (match > HINTSZ)
	    continue;
	if ((histend-match)->wch=='\0')
	    continue;

	for (guess=TSISZ-1-match; guess>=1; guess--) {
	    if (guess-match > 2)
		continue;

	    nwchs_to_mbs(matched_mb, histend-match, match, sizeof(matched_mb));
	    if(iconv_to_big5(cf, matched_mb, matched_mb_big5, sizeof(matched_mb_big5))!=0)
		continue;
	    strcpy(tsi_str, matched_mb_big5);
	    match_strlen=strlen(matched_mb_big5);
	    errno = cf->tsidb->CursorSet(cf->tsidb, &tsi, 1);
	    while (errno == 0) {
		if (memcmp(tsi_str, matched_mb_big5, match_strlen))
		    break;
		if(iconv_from_big5(cf, tsi_str, matched_mb, sizeof(matched_mb))!=0)
		    continue;
		slen = my_mbstowcs(cf, matched_wc, matched_mb, sizeof(matched_wc)/sizeof(wch_t));
		if (slen == (match+guess)) {
		    one.matchlen = match;
		    one.ref = tsi.refcount;
		    one.len = slen-match;
		    memcpy(one.str, matched_wc+match, (slen-match)*sizeof(wch_t));
		    ncandi = insert_candidate(ncandi, candi, &one, maxcandi);
		}
		errno = cf->tsidb->CursorNext(cf->tsidb, &tsi);
	    }
	}
    }
    return ncandi;
}

static unsigned int
gen_inp_keystroke_wrap(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    static char cch_s[HINTSZ], mcch_hint[HINTSZ];

    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    char *keystr = keyinfo->keystr;
    unsigned int i, ret=IMKEY_ABSORB, hint_processing=0;

    if (! cf->tsidb) 
	return gen_inp_keystroke(conf, inpinfo, keyinfo);

    if (cf->mode & INP_MODE_HINTTSI) {
	if (iccf->showtsiflag) {
	    inpinfo->n_mcch = 0;
	    inpinfo->mcch_grouping = NULL;

	    if ((keyinfo->keystate & Mod1Mask) &&	/* alt-Num/space */
		(('0'<=keystr[0] && keystr[0]<='9') ||
		 ((cf->mode & INP_MODE_SPACEAUTOUP) && keysym==XK_space))) {
		int choice=-1;

		if (keysym == XK_space)
		    choice = 1;
		else {
		    if (keystr[0] >= '1' && keystr[0] <= '9')
			choice = (int)(keystr[0] - '0');
		    else if (keystr[0] == '0')
			choice = 10;
		    if (cf->mode & INP_MODE_SELKEYSHIFT)
			choice ++;
		}
		if (choice >= 1 && choice <= iccf->nreltsi) {
		    nwchs_to_mbs(cch_s, iccf->reltsi+iccf->tsiindex[choice-1], 
			    iccf->tsigroup[choice], sizeof(cch_s));
		    inpinfo->cch = cch_s;
		    ret |= IMKEY_COMMIT;
		}
		hint_processing = 1;
	    }
	    else if (keysym==XK_Escape || 
		     keysym==XK_Shift_L || keysym==XK_Shift_R ||
		     keysym==XK_Control_L || keysym==XK_Control_R) {
		hint_processing = 1;
	    }
	    else if (keysym==XK_BackSpace || keysym==XK_Delete) {
		hint_processing = 1;
		iccf->showtsiflag = 0;
		ret |= IMKEY_IGNORE;
	    }
	}
    }
    if (hint_processing == 0)
	ret = gen_inp_keystroke(conf, inpinfo, keyinfo);

    if (ret & IMKEY_COMMIT)
	record_commit(cf, iccf, inpinfo->cch);

    if (cf->mode & INP_MODE_HINTTSI) {
	if (ret != IMKEY_IGNORE)
	    iccf->showtsiflag = 0;
	if (ret & IMKEY_COMMIT) {
	    int ncandi;
	    candidate_t candi[SELECT_KEY_LENGTH];

	    ncandi = guess_next(cf, iccf, candi, inpinfo->n_selkey);
	    iccf->nreltsi = 0;
	    iccf->tsiindex[0] = 0;
	    for (i=0; i<ncandi; i++) {
		int n = iccf->nreltsi;
		if(iccf->tsiindex[n] + candi[i].len > 15) /* TODO crash, who overflow? */
		    break;
		memcpy(iccf->reltsi+iccf->tsiindex[n], candi[i].str,
		       candi[i].len*sizeof(wch_t));
		iccf->tsigroup[n+1] = candi[i].len;
		iccf->tsiindex[n+1] = iccf->tsiindex[n] + candi[i].len;
		iccf->nreltsi ++;
	    }
	    iccf->showtsiflag = 1;
	}
	if (iccf->showtsiflag) {
	    inpinfo->n_mcch = iccf->tsiindex[iccf->nreltsi];
	    inpinfo->mcch_grouping    = iccf->tsigroup;
	    inpinfo->mcch_grouping[0] = iccf->nreltsi;

	    for (i=0; i<inpinfo->n_mcch; i++)
		inpinfo->mcch[i] = iccf->reltsi[i];
	}
    }

    if (cf->mode & INP_MODE_HINTSEL) {
/*	if (ret==IMKEY_ABSORB && (inpinfo->guimode & GUIMOD_SELKEYSPOT)) { */
	if ((inpinfo->guimode & GUIMOD_SELKEYSPOT)) {
	    inpinfo->mcch_hint = mcch_hint;
	    for (i=0; i<inpinfo->n_mcch; i++) {
		if (may_next(cf, iccf, inpinfo->mcch[i]))
		    inpinfo->mcch_hint[i] = 1;
		else
		    inpinfo->mcch_hint[i] = 0;
	    }
	}
	else
	    inpinfo->mcch_hint = NULL;
    }
    return ret;
}
#endif



/*----------------------------------------------------------------------------

	gen_inp_show_keystroke()

----------------------------------------------------------------------------*/

static int 
gen_inp_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    int i, idx;
    wchar_t tmp;
    char *k, keystroke[INP_CODE_LENGTH+1];
    static wch_t keystroke_list[INP_CODE_LENGTH+1];
    unsigned int klist[MAX_ICODE_MODE];

    if ((i = ccode_to_idx(&(simdinfo->cch_publish))) == -1)
        return False;

    if ((idx = cf->ichar[i]) == ICODE_IDX_NOTEXIST)
	return False;
    for(i=0; i<cf->header.icode_mode; i++)
	klist[i] = cf->ic[i][idx];
    codes2keys(klist, cf->header.icode_mode, keystroke, SELECT_KEY_LENGTH+1);
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

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "gen_inp",				/* name */
      MODULE_VERSION,				/* version */
      gen_inp_comments },			/* comments */
    gen_inp_valid_objname,			/* valid_objname */
    sizeof(gen_inp_conf_t),			/* conf_size */
    gen_inp_init,				/* init */
    gen_inp_xim_init,				/* xim_init */
    gen_inp_xim_end,				/* xim_end */
#ifdef HAVE_LIBTABE
    gen_inp_keystroke_wrap,			/* keystroke */
#else
    gen_inp_keystroke,				/* keystroke */
#endif
    gen_inp_show_keystroke,			/* show_keystroke */
    NULL					/* terminate */
};

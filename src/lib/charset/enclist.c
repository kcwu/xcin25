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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <iconv.h>
#include "xcintool.h"
#include "charset.h"

typedef struct {
    char *encname;
    csdata_t *csdata;
    int (*wcs2mbs)(char *mbs, int mblen, wchar_t *wcs, int wclen);
    int (*mbs2wcs)(wchar_t *wcs, int wclen, char *mbs, int mblen);
    iconv_t cdwm, cdmw;
    int refwm, refmw;
} enclist_t;

static int n_enc, n_iconv;
static enclist_t enclist[N_ENCODING_ENTRY];


/*----------------------------------------------------------------------------

	List of encoding functions.

----------------------------------------------------------------------------*/

static char *encname[] = {
    "utf8", "big5", "big5hkscs", "gb2312", "gbk", NULL
};

static void
encfunc_init(int i, xcin_rc_t *xrc)
{
    switch (i) {
    case 0:	enclist[n_enc].csdata  = utf8_init(n_enc, xrc);
		enclist[n_enc].wcs2mbs = NULL;
		enclist[n_enc].mbs2wcs = NULL;
		break;
    case 1:	enclist[n_enc].csdata  = big5_init(n_enc, xrc);
		enclist[n_enc].wcs2mbs = big5_lnr2mbs;
		enclist[n_enc].mbs2wcs = big5_mbs2lnr;
		break;
    case 2:	enclist[n_enc].csdata  = big5hkscs_init(n_enc, xrc);
		enclist[n_enc].wcs2mbs = big5hkscs_lnr2mbs;
		enclist[n_enc].mbs2wcs = big5hkscs_mbs2lnr;
		break;
    case 3:	enclist[n_enc].csdata  = gb2312_init(n_enc, xrc);
		enclist[n_enc].wcs2mbs = gb2312_lnr2mbs;
		enclist[n_enc].mbs2wcs = gb2312_mbs2lnr;
		break;
    case 4:	enclist[n_enc].csdata  = gbk_init(n_enc, xrc);
		enclist[n_enc].wcs2mbs = gbk_lnr2mbs;
		enclist[n_enc].mbs2wcs = gbk_mbs2lnr;
		break;
    default:    enclist[n_enc].csdata  = NULL;
		enclist[n_enc].wcs2mbs = NULL;
		enclist[n_enc].mbs2wcs = NULL;
		break;
    }
}

/*----------------------------------------------------------------------------

	Encoding list public functions.

----------------------------------------------------------------------------*/

#define MAX_ICONV_DES	6
#define MAX_REF_CNT	1073741824			/* 2^30 */

/*
 *  Here we may need a sorting algorithm (from encname to encid).
 *
 *  Because it is expected that in most cases there are at most 5 - 10
 *  encodings loaded in run-time, it will not cause too much trouble
 *  for doing linear search in such a few elements. So here we don't
 *  need a complex algorithm and extra memory storage for doing search.
 *  If one day we will have more than 16 encodings loaded in run-time,
 *  then the good algorithms should be considered.
 */

int
xcin_newenc(char *encoding, xcin_rc_t *xrc)
{
    int i;

    for (i=0; i<n_enc; i++) {
	if (strcmp(enclist[i].encname, encoding) == 0)
	    return i;
    }
    if (n_enc < N_ENCODING_ENTRY) {
	enclist[n_enc].encname = (char *)strdup(encoding);
	for (i=0; encname[i] != NULL; i++) {
	    if (strcmp(encname[i], encoding) == 0) {
		encfunc_init(i, xrc);
		break;
	    }
	}
	n_enc ++;
	return (n_enc-1);
    }
    else
	return -1;
}

int
xcin_enc2id(char *encoding)
{
    int i;

    for (i=0; i<n_enc; i++) {
	if (strcmp(encoding, enclist[i].encname) == 0)
	    return i;
    }
    return -1;
}

char *
xcin_id2enc(int encid)
{
    return (encid>=0 && encid<n_enc) ? enclist[encid].encname : NULL;
}

csdata_t *
xcin_csdata(int encid)
{
    return (encid>=0 && encid<n_enc) ? enclist[encid].csdata : NULL;
}

void
xcin_enclist_clean(void)
{
    int i;

    for (i=0; i<n_enc; i++) {
	free(enclist[i].encname);
	enclist[i].encname = NULL;
	enclist[i].csdata  = NULL;
	enclist[i].wcs2mbs = NULL;
	enclist[i].mbs2wcs = NULL;
	if (enclist[i].refwm > 0) {
	    iconv_close(enclist[i].cdwm);
	    enclist[i].cdwm = (iconv_t)0;
	}
	if (enclist[i].refmw > 0) {
	    iconv_close(enclist[i].cdmw);
	    enclist[i].cdmw = (iconv_t)0;
	}
	enclist[i].refwm = 0;
	enclist[i].refmw = 0;
    }
    n_enc = 0;
}

/*----------------------------------------------------------------------------

	Charater set converting functions.

----------------------------------------------------------------------------*/
/*
 *  The "XXwm" means conversion from wcs to mbs; while "XXmw" means
 *  conversion from mbs to wcs. They are only used when iconv() is
 *  involved for the character set conversion. The maxmum number of
 *  opened iconv descriptors is limited to MAX_ICONV_DES, so the "refwm"
 *  and "refmw" are used to record the referenced count number of "cdwm"
 *  and "cdmw". The meaning of the values are:
 *
 *	0:  means not opened yet.
 *	>0: means opened and used in the program.
 *	<0: means opened false and should not be used any more.
 */

static void
normalize_enclist_ref(void)
{
    int i;

    for (i=0; i<n_enc; i++) {
	if (enclist[i].refwm > 0)
	    enclist[i].refwm = enclist[i].refwm / 2 + 1;
	if (enclist[i].refmw > 0)
	    enclist[i].refmw = enclist[i].refmw / 2 + 1;
    }
}

static void
release_minref_iconv(void)
{
    int i, minref=MAX_REF_CNT, minidx=0;

    for (i=0; i<n_enc; i++) {
	if (enclist[i].refwm < minref) {
	    minref = enclist[i].refwm;
	    minidx = i;
	}
	if (enclist[i].refmw < minref) {
	    minref = enclist[i].refmw;
	    minidx = i;
	}
    }

    n_iconv --;
    if (enclist[i].refwm < enclist[i].refmw) {
	iconv_close(enclist[i].cdwm);
	enclist[i].refwm = 0;
    }
    else {
	iconv_close(enclist[i].cdmw);
	enclist[i].refmw = 0;
    }
}

int
connect_iconv(int encid, int to_wch)
{
    iconv_t cd;

    if (to_wch == 0) {
	if (enclist[encid].refwm > 0) {
	    enclist[encid].refwm ++;
	    if (enclist[encid].refwm >= MAX_REF_CNT)
		normalize_enclist_ref();
	    return True;
	}
	else if (enclist[encid].refwm < 0)
	    return False;

	if ((cd = iconv_open(enclist[encid].encname, "ucs4")) == (iconv_t)-1) {
	    enclist[encid].refwm = -1;
	    return False;
	}
	else {
	    n_iconv ++;
	    if (n_iconv > MAX_ICONV_DES)
		release_minref_iconv();
	    enclist[encid].cdwm = cd;
	    enclist[encid].refwm ++;
	    return True;
	}
    }
    else {
	if (enclist[encid].refmw > 0) {
	    enclist[encid].refmw ++;
	    if (enclist[encid].refmw >= MAX_REF_CNT)
		normalize_enclist_ref();
	    return True;
	}
	else if (enclist[encid].refmw < 0)
	    return False;

	if ((cd = iconv_open("ucs4", enclist[encid].encname)) == (iconv_t)-1) {
	    enclist[encid].refmw = -1;
	    return False;
	}
	else {
	    n_iconv ++;
	    if (n_iconv > MAX_ICONV_DES)
		release_minref_iconv();
	    enclist[encid].cdmw = cd;
	    enclist[encid].refmw ++;
	    return True;
	}
    }
}

int
xcin_wcs2mbs(xch_t *mbs, xch_t *wcs)
{
    static char *mbstring;
    static int mblen;
    int r=0;

    if (wcs->type == XCH_TYPE_MBYTES) {
	perr(XCINMSG_IWARNING, N_("xcin_wcs2mbs: invalid wcs->type.\n"));
	return False;
    }
    if (wcs->encid >= n_enc) {
	perr(XCINMSG_IWARNING, N_("xcin_wcs2mbs: invalid wcs->encid.\n"));
	return False;
    }
    if (wcs->len == 0 || wcs->c.w == NULL) {
	perr(XCINMSG_IWARNING, N_("xcin_wcs2mbs: empty wcs string.\n"));
	return False;
    }

    if (mblen < wcs->len*MB_LEN_MAX) {
	if (mbstring != NULL)
	    free(mbstring);
	mblen = wcs->len*MB_LEN_MAX;
	mbstring = xcin_malloc(mblen, 0);
    }

    if (wcs->type == XCH_TYPE_WLINEAR) {
	if (enclist[wcs->encid].wcs2mbs == NULL) {
	    perr(XCINMSG_IWARNING,
		N_("xcin_wcs2mbs: no valid lnr2mbs() for encoding: %s.\n"),
		enclist[wcs->encid].encname);
	    return False;
	}
	r = enclist[wcs->encid].wcs2mbs(mbstring, mblen, wcs->c.w, wcs->len);
	if (r <= 0) {
	    perr(XCINMSG_IWARNING,
		N_("xcin_wcs2mbs: wcs2lnr() false for encoding: %s, r=%d.\n"),
		enclist[wcs->encid].encname, r);
	    return False;
	}
    }
    else if (wcs->type == XCH_TYPE_WUCS4) {
	char *inbuf, *outbuf;
	size_t inlen, outlen;

	if (connect_iconv(wcs->encid, 0) != True) {
	    perr(XCINMSG_IWARNING,
		N_("xcin_wcs2mbs: iconv_open() false for encoding: %s.\n"),
		enclist[wcs->encid].encname);
	    return False;
	}
	inbuf  = (char *)wcs->c.w;
	inlen  = wcs->len*sizeof(wchar_t);
	outbuf = mbstring;
	outlen = mblen;
	r = iconv(enclist[wcs->encid].cdwm, &inbuf, &inlen, &outbuf, &outlen);
	if (r <= 0) {
	    perr(XCINMSG_IWARNING,
		N_("xcin_wcs2mbs: iconv() false for encoding: %s, r=%d.\n"),
		enclist[wcs->encid].encname, r);
	    return False;
	}
	r = mblen - outlen;
    }
    mbs->type = (x_uint8)XCH_TYPE_MBYTES;
    mbs->encid = (x_uint8)wcs->encid;
    mbs->len = (x_uint16)((r <= N_XCH_SLEN) ? r : N_XCH_SLEN);
    mbs->c.s = mbstring;
    return True;
}

static int
to_wlinear(wchar_t *wcstring, int wclen, xch_t *mbs)
{
    if (enclist[mbs->encid].mbs2wcs == NULL)
	return 0;
    return enclist[mbs->encid].mbs2wcs(wcstring, wclen, mbs->c.s, mbs->len);
}

static int
to_wucs4(wchar_t *wcstring, int wclen, xch_t *mbs)
{
    char *inbuf, *outbuf;
    size_t inlen, outlen;
    int r;

    if (connect_iconv(mbs->encid, 0) != True)
	return 0;
    inbuf  = mbs->c.s;
    inlen  = mbs->len;
    outbuf = (char *)wcstring;
    outlen = wclen;
    r = iconv(enclist[mbs->encid].cdmw, &inbuf, &inlen, &outbuf, &outlen);
    return (r <= 0) ? r : wclen - outlen;
}

int
xcin_mbs2wcs(xch_t *wcs, xch_t *mbs, int wcstype)
{
    static wchar_t *wcstring;
    static int wclen;
    int r=0;

    if (mbs->type != XCH_TYPE_MBYTES) {
	perr(XCINMSG_IWARNING, N_("xcin_mbs2wcs: invalid mbs->type.\n"));
	return False;
    }
    if (mbs->encid >= n_enc) {
	perr(XCINMSG_IWARNING, N_("xcin_mbs2wcs: invalid mbs->encid.\n"));
	return False;
    }
    if (mbs->len == 0 || mbs->c.s == NULL) {
	perr(XCINMSG_IWARNING, N_("xcin_mbs2wcs: empty mbs string.\n"));
	return False;
    }

    if (wclen < mbs->len) {
	if (wcstring != NULL)
	    free(wcstring);
	wclen = mbs->len;
	wcstring = xcin_malloc(wclen*sizeof(wchar_t), 0);
    }

    if (wcstype == XCH_TYPE_WLINEAR) {
	if ((r = to_wlinear(wcstring, wclen, mbs)) <= 0) {
	    if ((r = to_wucs4(wcstring, wclen, mbs)) > 0)
		wcstype = XCH_TYPE_WUCS4;
	}
    }
    else if (wcstype == XCH_TYPE_WUCS4) {
	if ((r = to_wucs4(wcstring, wclen, mbs)) <= 0) {
	    if ((r = to_wlinear(wcstring, wclen, mbs)) > 0)
		wcstype = XCH_TYPE_WLINEAR;
	}
    }

    if (r <= 0) {
	perr(XCINMSG_IWARNING,
	    N_("xcin_mbs2wcs: cannot convert to wcs for encoding: %s\n"),
	    enclist[wcs->encid].encname);
	return False;
    }
    else {
	wcs->type = (x_uint8)wcstype;
	wcs->encid = (x_uint8)mbs->encid;
	wcs->len = (x_uint16)((r <= N_XCH_SLEN) ? r : N_XCH_SLEN);
	wcs->c.w = wcstring;
        return True;
    }
}

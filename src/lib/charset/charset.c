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

#include <stdlib.h>
#include "xcintool.h"
#include "charset.h"


/*----------------------------------------------------------------------------

	CHSET_NOMODAL 2-byte encoding + 1-byte ASCII common transformation.

----------------------------------------------------------------------------*/

static int
cs_cmplnr1(const void *key, const void *arr)
{
    int *ckey=(int *)key;
    cs_seg_t *carr=(cs_seg_t *)arr;

    if (*ckey < carr->c_start)
	return -1;
    else if (*ckey >= carr->c_start+carr->c_cnt)
	return  1;
    else
	return  0;
}

static int
cs_cmplnr2(const void *key, const void *arr)
{
    int *ckey=(int *)key;
    cs_seg2_t *carr=(cs_seg2_t *)arr;

    if (*ckey < carr->c_accu2)
	return -1;
    else if (*ckey >= carr->c_accu2+carr->c_cnt2)
	return  1;
    else
	return  0;
}

static int
cs_cmpmbs1(const void *key, const void *arr)
{
    int *ckey=(int *)key;
    cs_seg_t *carr=(cs_seg_t *)arr;

    if (*ckey < carr->enc1a)
	return -1;
    else if (*ckey > carr->enc1b)
	return  1;
    else
	return  0;
}

static int
cs_cmpmbs2(const void *key, const void *arr)
{
    int *ckey=(int *)key;
    cs_seg2_t *carr=(cs_seg2_t *)arr;

    if (*ckey < carr->enc2a)
	return -1;
    else if (*ckey > carr->enc2b)
	return  1;
    else
	return  0;
}

int
cs2_lnr2mbs(csdata_t *cs, cs_seg_t *seg, int n_seg,
	    char *mbs, int mblen, wchar_t *wcs, int wclen)
{
    int code, mlen=0, wlen=0, c0, c1;
    cs_seg_t  *cseg;
    cs_seg2_t *cseg2;

    if (mblen < cs->max_mblen)
	return -1;

    while (mlen < mblen && wlen < wclen) {
	if (*wcs == (wchar_t)0) {
	    *mbs = '\0';
	    mbs  ++;
	    mlen ++;
	}
	else if ((*wcs & UINT32BIT)) {
	    *mbs = (char)(*wcs & (~UINT32BIT));
	    mbs  ++;
	    mlen ++;
	}
	else if (*wcs > (wchar_t)(cs->n_chcoded))
	    return -mlen;
	else {
	    if (! (cseg = bsearch(wcs, seg,
			n_seg, sizeof(cs_seg_t), cs_cmplnr1)))
		return -mlen;
	    code = ((int)(*wcs) - cseg->c_start) % cseg->c_cnt2;
	    if (seg->n_seg2 == 1) {
		if (cs_cmplnr2(&code, seg->seg2) != 0)
		    return -mlen;
		else
		    cseg2 = seg->seg2;
	    }
	    else {
		if (! (cseg2 = bsearch(&code, seg->seg2,
			seg->n_seg2, sizeof(cs_seg2_t), cs_cmplnr2)))
		    return -mlen;
	    }

	    code = (int)(*wcs) - cseg->c_start;
	    c0 = code / cseg->c_cnt2 + cseg->enc1a;
	    c1 = code % cseg->c_cnt2 + cseg2->enc2a - cseg2->c_accu2;
	    *mbs     = (char)c0;
	    *(mbs+1) = (char)c1;
	    mbs  += 2;
	    mlen += 2;
	}
	wcs  ++;
	wlen ++;
	if (*(wcs-1) == (wchar_t)0)
	    break;
    }
    return (*(wcs-1) == (wchar_t)0 || wlen == wclen) ? mlen : -mlen;
}

int
cs2_mbs2lnr(csdata_t *cs, cs_seg_t *seg, int n_seg,
	    wchar_t *wcs, int wclen, char *mbs, int mblen)
{
    unsigned char *ms = (unsigned char *)mbs;
    int mlen=0, wlen=0, enc1, enc2;
    cs_seg_t  *cseg;
    cs_seg2_t *cseg2;

    while (mlen < mblen && wlen < wclen) {
	if (*ms == '\0') {
	    *wcs = (wchar_t)0;
	    ms   ++;
	    mlen ++;
	}
	else if (*ms < 0x80) {
	    *wcs = (wchar_t)(*ms) | UINT32BIT;
	    ms   ++;
	    mlen ++;
	}
	else {
	    enc1 = (int)(*ms);
	    enc2 = (int)(*(ms+1));
	    if (! (cseg = bsearch(&enc1, seg,
			n_seg, sizeof(cs_seg_t), cs_cmpmbs1)))
		return -wlen;
	    if (cseg->n_seg2 == 1) {
		if (cs_cmpmbs2(&enc2, seg->seg2) != 0)
		    return -wlen;
		else
		    cseg2 = seg->seg2;
	    }
	    else {
		if (! (cseg2 = bsearch(&enc2, seg->seg2,
			seg->n_seg2, sizeof(cs_seg2_t), cs_cmpmbs2)))
		    return -wlen;
	    }

	    *wcs = (wchar_t)((enc1 - cseg->enc1a) * cseg->c_cnt2 +
			     (enc2 - cseg2->enc2a + cseg2->c_accu2) +
			     cseg->c_start);
	    ms   += 2;
	    mlen += 2;
	}
	wcs  ++;
	wlen ++;
	if (*(ms-1) == '\0')
	    break;
    }
    return (*(ms-1) == '\0' || mlen == mblen) ? wlen : -wlen;
}

int
cs2_init(csdata_t *cs, cs_seg_t *seg)
{
    int i, j, c_cnt1, c_accu=0, c_accu2;
    cs_seg2_t *sg2;

    for (i=0; seg[i].enc1a!=0; i++) {
	for (j=0, c_accu2=0, sg2=seg[i].seg2; sg2->enc2a!=0; j++, sg2++) {
	    if (sg2->c_cnt2 == 0) {
		sg2->c_cnt2  = sg2->enc2b - sg2->enc2a + 1;
		sg2->c_accu2 = c_accu2;
	    }
	    c_accu2 += sg2->c_cnt2;
	}
	c_cnt1 = seg[i].enc1b - seg[i].enc1a + 1;
	seg[i].n_seg2  = j;
	seg[i].c_start = c_accu+1;
	seg[i].c_cnt2  = c_accu2;
	seg[i].c_cnt   = c_cnt1 * c_accu2;
	c_accu += (seg[i].c_cnt);
    }
    cs->n_chcoded = c_accu;
    cs->n_char    = c_accu + 127;
    return i;
}

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

#include "xcintool.h"
#include "charset.h"

/* 
 * BIG5 character set:  iso8859-1, big5
 *	encoding type:  CHSET_NOMODAL
 */

static sub_csdata_t data_subset[2] = {
    { "iso8859-1",		/* subset_name */
      127,			/* n_chars */
      1,			/* mblen */
      0 },			/* code_start */

    { "big5",			/* subset_name */
      0,			/* n_chars */
      2,			/* mblen */
      1	}			/* code_start */
};

static csdata_t csdata = {
    "BIG5",			/* charset_name */
    CHSET_NOMODAL,		/* charset_type */
    2,				/* n_subset */
    data_subset,		/* sub-charset list */
    0,				/* n_chars */
    0,				/* n_chcoded */
    2,				/* max_mblen */
};

static int n_seg;

static cs_seg2_t seg2_1[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_2[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xbf, 0, 0 },
	{ 0xe1, 0xe1, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg_t seg[] = {
	{ 0xa1, 0xa2, 0, 0, 0, 0, seg2_1 },
	{ 0xa3, 0xa3, 0, 0, 0, 0, seg2_2 },
	{ 0xa4, 0xf9, 0, 0, 0, 0, seg2_1 },
	{ 0,    0,    0, 0, 0, 0, NULL   }	};

static char *wc_ascii =
    "　！”＃＄％＆’（）＊＋，－．╱０１２３４５６７８９：；＜＝＞？＠"
    "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ〔╲〕︿ˍ‵"
    "ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～";

static char *imname_en = "英數";
static char *imname_sb = "半形";
static char *imname_wc = "全形";

/*----------------------------------------------------------------------------

	BIG5 Charset Module Functions.

----------------------------------------------------------------------------*/

int
big5_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen)
{
    return cs2_lnr2mbs(&csdata, seg, n_seg, mbs, mblen, wcs, wclen);
}

int
big5_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen)
{
    return cs2_mbs2lnr(&csdata, seg, n_seg, wcs, wclen, mbs, mblen);
}

csdata_t *
big5_init(int encid, xcin_rc_t *xc)
{
    n_seg = cs2_init(&csdata, seg);
    csdata.sch[1].n_char   = csdata.n_chcoded;
    csdata.wc_ascii.type   = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.wc_ascii.encid  = (x_uint8)encid;
    csdata.wc_ascii.c.w    = xcin_malloc(N_ASCII*sizeof(wchar_t), 0);
    csdata.wc_ascii.len    = (x_uint16)big5_mbs2lnr(
		csdata.wc_ascii.c.w, N_ASCII, wc_ascii, N_ASCII*2);
    csdata.imname_en.type  = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.imname_en.encid = (x_uint8)encid;
    csdata.imname_en.c.w   = xcin_malloc(2*sizeof(wchar_t), 0);
    csdata.imname_en.len   = (x_uint16)big5_mbs2lnr(
		csdata.imname_en.c.w, 2, imname_en, 4);
    csdata.imname_sb.type  = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.imname_sb.encid = (x_uint8)encid;
    csdata.imname_sb.c.w   = xcin_malloc(2*sizeof(wchar_t), 0);
    csdata.imname_sb.len   = (x_uint16)big5_mbs2lnr(
		csdata.imname_sb.c.w, 2, imname_sb, 4);
    csdata.imname_wc.type  = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.imname_wc.encid = (x_uint8)encid;
    csdata.imname_wc.c.w   = xcin_malloc(2*sizeof(wchar_t), 0);
    csdata.imname_wc.len   = (x_uint16)big5_mbs2lnr(
		csdata.imname_wc.c.w, 2, imname_wc, 4);
    return &csdata;
}

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
 * BIG5HKSCS character set:  iso8859-1, big5hkscs
 *	     encoding type:  CHSET_NOMODAL
 */

static sub_csdata_t data_subset[2] = {
    { "iso8859-1",		/* subset_name */
      127,			/* n_chars */
      1,			/* mblen */
      0 },			/* code_start */

    { "big5hkscs",		/* subset_name */
      0,			/* n_chars */
      2,			/* mblen */
      1	}			/* code_start */
};

static csdata_t csdata = {
    "BIG5HKSCS",		/* charset_name */
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
	{ 0xa1, 0xaa, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_2[] = {
	{ 0x40, 0x41, 0, 0 },
	{ 0x43, 0x43, 0, 0 },
	{ 0x46, 0x49, 0, 0 },
	{ 0x4c, 0x7e, 0, 0 },
	{ 0xa1, 0xa6, 0, 0 },
	{ 0xab, 0xae, 0, 0 },
	{ 0xb0, 0xb2, 0, 0 },
	{ 0xb5, 0xbf, 0, 0 },
	{ 0xc1, 0xc3, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_3[] = {
	{ 0x40, 0x41, 0, 0 },
	{ 0x43, 0x62, 0, 0 },
	{ 0x64, 0x74, 0, 0 },
	{ 0x76, 0x7e, 0, 0 },
	{ 0xa1, 0xaa, 0, 0 },
	{ 0xac, 0xb0, 0, 0 },
	{ 0xb2, 0xb9, 0, 0 },
	{ 0xbb, 0xc7, 0, 0 },
	{ 0xc9, 0xcc, 0, 0 },
	{ 0xce, 0xdc, 0, 0 },
	{ 0xdf, 0xf4, 0, 0 },
	{ 0xf6, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_4[] = {
	{ 0x40, 0x53, 0, 0 },
	{ 0x55, 0x7e, 0, 0 },
	{ 0xa1, 0xdc, 0, 0 },
	{ 0xde, 0xfd, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_5[] = {
	{ 0x60, 0x7e, 0, 0 },
	{ 0xa1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_6[] = {
	{ 0x40, 0x68, 0, 0 },
	{ 0x6a, 0x6e, 0, 0 },
	{ 0x70, 0x7d, 0, 0 },
	{ 0xa1, 0xaa, 0, 0 },
	{ 0xac, 0xb3, 0, 0 },
	{ 0xb5, 0xcc, 0, 0 },
	{ 0xce, 0xcf, 0, 0 },
	{ 0xd1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_7[] = {
	{ 0x40, 0x56, 0, 0 },
	{ 0x58, 0x68, 0, 0 },
	{ 0x6a, 0x6d, 0, 0 },
	{ 0x6f, 0x7e, 0, 0 },
	{ 0xa1, 0xca, 0, 0 },
	{ 0xcd, 0xfd, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_8[] = {
	{ 0x40, 0x6c, 0, 0 },
	{ 0x6e, 0x79, 0, 0 },
	{ 0x7b, 0x7e, 0, 0 },
	{ 0xa1, 0xdb, 0, 0 },
	{ 0xdd, 0xf0, 0, 0 },
	{ 0xf2, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_9[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xbe, 0, 0 },
	{ 0xc0, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_10[] = {
	{ 0x40, 0x43, 0, 0 },
	{ 0x45, 0x7e, 0, 0 },
	{ 0xa1, 0xae, 0, 0 },
	{ 0xb3, 0xc7, 0, 0 },
	{ 0xc9, 0xd0, 0, 0 },
	{ 0xd2, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_11[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_12[] = {
	{ 0x40, 0x46, 0, 0 },
	{ 0x48, 0x7e, 0, 0 },
	{ 0xa1, 0xc9, 0, 0 },
	{ 0xcb, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_13[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xd8, 0, 0 },
	{ 0xda, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_14[] = {
	{ 0x40, 0x43, 0, 0 },
	{ 0x45, 0x7e, 0, 0 },
	{ 0xa1, 0xec, 0, 0 },
	{ 0xee, 0xfb, 0, 0 },
	{ 0xfd, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_15[] = {
	{ 0x40, 0x60, 0, 0 },
	{ 0x62, 0x75, 0, 0 },
	{ 0x77, 0x77, 0, 0 },
	{ 0x79, 0x7a, 0, 0 },
	{ 0x7c, 0x7e, 0, 0 },
	{ 0xa1, 0xc5, 0, 0 },
	{ 0xc7, 0xdd, 0, 0 },
	{ 0xdf, 0xeb, 0, 0 },
	{ 0xed, 0xf5, 0, 0 },
	{ 0xf7, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_16[] = {
	{ 0x40, 0x41, 0, 0 },
	{ 0x43, 0x52, 0, 0 },
	{ 0x54, 0x61, 0, 0 },
	{ 0x63, 0x67, 0, 0 },
	{ 0x69, 0x6a, 0, 0 },
	{ 0x6c, 0x76, 0, 0 },
	{ 0x78, 0x7e, 0, 0 },
	{ 0xa1, 0xbb, 0, 0 },
	{ 0xbe, 0xcf, 0, 0 },
	{ 0xd1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_17[] = {
	{ 0x40, 0x56, 0, 0 },
	{ 0x58, 0x59, 0, 0 },
	{ 0x5b, 0x7e, 0, 0 },
	{ 0xa1, 0xc3, 0, 0 },
	{ 0xc5, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_18[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xa8, 0, 0 },
	{ 0xaa, 0xab, 0, 0 },
	{ 0xad, 0xc3, 0, 0 },
	{ 0xc5, 0xee, 0, 0 },
	{ 0xf0, 0xf3, 0, 0 },
	{ 0xf5, 0xfc, 0, 0 },
	{ 0xfe, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_19[] = {
	{ 0x40, 0x4d, 0, 0 },
	{ 0x4f, 0x5f, 0, 0 },
	{ 0x61, 0x65, 0, 0 },
	{ 0x67, 0x7e, 0, 0 },
	{ 0xa1, 0xac, 0, 0 },
	{ 0xae, 0xb0, 0, 0 },
	{ 0xb2, 0xbf, 0, 0 },
	{ 0xc1, 0xc7, 0, 0 },
	{ 0xc9, 0xca, 0, 0 },
	{ 0xcc, 0xd7, 0, 0 },
	{ 0xd9, 0xd9, 0, 0 },
	{ 0xdb, 0xe5, 0, 0 },
	{ 0xe7, 0xe9, 0, 0 },
	{ 0xeb, 0xee, 0, 0 },
	{ 0xf0, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_20[] = {
	{ 0x40, 0x53, 0, 0 },
	{ 0x55, 0x56, 0, 0 },
	{ 0x58, 0x59, 0, 0 },
	{ 0x5b, 0x61, 0, 0 },
	{ 0x64, 0x71, 0, 0 },
	{ 0x73, 0x76, 0, 0 },
	{ 0x78, 0x7e, 0, 0 },
	{ 0xa1, 0xa4, 0, 0 },
	{ 0xa6, 0xac, 0, 0 },
	{ 0xae, 0xae, 0, 0 },
	{ 0xb0, 0xd2, 0, 0 },
	{ 0xd4, 0xd4, 0, 0 },
	{ 0xd6, 0xde, 0, 0 },
	{ 0xe0, 0xe0, 0, 0 },
	{ 0xe2, 0xe3, 0, 0 },
	{ 0xe5, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_21[] = {
	{ 0x40, 0x59, 0, 0 },
	{ 0x5b, 0x7e, 0, 0 },
	{ 0xa1, 0xc2, 0, 0 },
	{ 0xc4, 0xc4, 0, 0 },
	{ 0xc6, 0xfd, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_22[] = {
	{ 0x41, 0x7e, 0, 0 },
	{ 0xa1, 0xcb, 0, 0 },
	{ 0xcd, 0xcd, 0, 0 },
	{ 0xcf, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_23[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xbf, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_24[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xce, 0, 0 },
	{ 0xd0, 0xd2, 0, 0 },
	{ 0xd4, 0xd4, 0, 0 },
	{ 0xd6, 0xd6, 0, 0 },
	{ 0xd8, 0xdd, 0, 0 },
	{ 0xe0, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_25[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xa4, 0, 0 },
	{ 0xcd, 0xf1, 0, 0 },
	{ 0xf5, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_26[] = {
	{ 0x40, 0x5e, 0, 0 },
	{ 0x60, 0x65, 0, 0 },
	{ 0x67, 0x7e, 0, 0 },
	{ 0xa1, 0xbc, 0, 0 },
	{ 0xbe, 0xc4, 0, 0 },
	{ 0xc6, 0xd4, 0, 0 },
	{ 0xd6, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_27[] = {
	{ 0x40, 0x47, 0, 0 },
	{ 0x49, 0x7e, 0, 0 },
	{ 0xa1, 0xb7, 0, 0 },
	{ 0xb9, 0xf2, 0, 0 },
	{ 0xf4, 0xf8, 0, 0 },
	{ 0xfa, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_28[] = {
	{ 0x40, 0x4e, 0, 0 },
	{ 0x50, 0x6b, 0, 0 },
	{ 0x6d, 0x7e, 0, 0 },
	{ 0xa1, 0xb8, 0, 0 },
	{ 0xba, 0xe1, 0, 0 },
	{ 0xe3, 0xf0, 0, 0 },
	{ 0xf2, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_29[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0xa1, 0xb6, 0, 0 },
	{ 0xb9, 0xba, 0, 0 },
	{ 0xbc, 0xf0, 0, 0 },
	{ 0xf2, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_30[] = {
	{ 0x40, 0x51, 0, 0 },
	{ 0x53, 0x6e, 0, 0 },
	{ 0x70, 0x7e, 0, 0 },
	{ 0xa1, 0xa9, 0, 0 },
	{ 0xab, 0xdc, 0, 0 },
	{ 0xde, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg_t seg[] = {
	{ 0x88, 0x88, 0, 0, 0, 0, seg2_1 },
	{ 0x89, 0x89, 0, 0, 0, 0, seg2_2 },
	{ 0x8a, 0x8a, 0, 0, 0, 0, seg2_3 },
	{ 0x8b, 0x8b, 0, 0, 0, 0, seg2_4 },
	{ 0x8d, 0x8d, 0, 0, 0, 0, seg2_5 },
	{ 0x8e, 0x8e, 0, 0, 0, 0, seg2_6 },
	{ 0x8f, 0x8f, 0, 0, 0, 0, seg2_7 },
	{ 0x90, 0x90, 0, 0, 0, 0, seg2_8 },
	{ 0x91, 0x91, 0, 0, 0, 0, seg2_9 },
	{ 0x92, 0x92, 0, 0, 0, 0, seg2_10 },
	{ 0x93, 0x93, 0, 0, 0, 0, seg2_11 },
	{ 0x94, 0x94, 0, 0, 0, 0, seg2_12 },
	{ 0x95, 0x95, 0, 0, 0, 0, seg2_13 },
	{ 0x96, 0x96, 0, 0, 0, 0, seg2_14 },
	{ 0x97, 0x9a, 0, 0, 0, 0, seg2_11 },
	{ 0x9b, 0x9b, 0, 0, 0, 0, seg2_15 },
	{ 0x9c, 0x9c, 0, 0, 0, 0, seg2_16 },
	{ 0x9d, 0x9d, 0, 0, 0, 0, seg2_17 },
	{ 0x9e, 0x9e, 0, 0, 0, 0, seg2_18 },
	{ 0x9f, 0x9f, 0, 0, 0, 0, seg2_19 },
	{ 0xa0, 0xa0, 0, 0, 0, 0, seg2_20 },
	{ 0xa1, 0xa1, 0, 0, 0, 0, seg2_21 },
	{ 0xa2, 0xa2, 0, 0, 0, 0, seg2_22 },
	{ 0xa3, 0xa3, 0, 0, 0, 0, seg2_23 },
	{ 0xa4, 0xc5, 0, 0, 0, 0, seg2_11 },
	{ 0xc6, 0xc6, 0, 0, 0, 0, seg2_24 },
	{ 0xc7, 0xc7, 0, 0, 0, 0, seg2_11 },
	{ 0xc8, 0xc8, 0, 0, 0, 0, seg2_25 },
	{ 0xc9, 0xf9, 0, 0, 0, 0, seg2_11 },
	{ 0xfa, 0xfa, 0, 0, 0, 0, seg2_26 },
	{ 0xfb, 0xfb, 0, 0, 0, 0, seg2_27 },
	{ 0xfc, 0xfc, 0, 0, 0, 0, seg2_28 },
	{ 0xfd, 0xfd, 0, 0, 0, 0, seg2_29 },
	{ 0xfe, 0xfe, 0, 0, 0, 0, seg2_30 },
	{ 0,    0,    0, 0, 0, 0, NULL    }	};

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
big5hkscs_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen)
{
    return cs2_lnr2mbs(&csdata, seg, n_seg, mbs, mblen, wcs, wclen);
}

int
big5hkscs_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen)
{
    return cs2_mbs2lnr(&csdata, seg, n_seg, wcs, wclen, mbs, mblen);
}

csdata_t *
big5hkscs_init(int encid, xcin_rc_t *xc)
{
    n_seg = cs2_init(&csdata, seg);
    csdata.sch[1].n_char  = csdata.n_chcoded;
    csdata.wc_ascii.type  = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.wc_ascii.encid = (x_uint8)encid;
    csdata.wc_ascii.c.w   = xcin_malloc(N_ASCII*sizeof(wchar_t), 0);
    csdata.wc_ascii.len   = (x_uint16)big5hkscs_mbs2lnr(
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

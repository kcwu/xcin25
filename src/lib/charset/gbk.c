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

#include "xcintool.h"
#include "charset.h"

/* 
 * GBK character set:  iso8859-1, gbk
 *     encoding type:  CHSET_NOMODAL
 */

static sub_csdata_t data_subset[2] = {
    { "iso8859-1",		/* subset_name */
      127,			/* n_chars */
      1,			/* mblen */
      0 },			/* code_start */

    { "gbk",			/* subset_name */
      0,			/* n_chars */
      2,			/* mblen */
      1	}			/* code_start */
};

static csdata_t csdata = {
    "GBK",			/* charset_name */
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
	{ 0x80, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_2[] = {
	{ 0xa1, 0xfe, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_3[] = {
	{ 0xa1, 0xaa, 0, 0 },
	{ 0xb1, 0xe2, 0, 0 },
	{ 0xe5, 0xee, 0, 0 },
	{ 0xf1, 0xfc, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_4[] = {
	{ 0xa1, 0xf3, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_5[] = {
	{ 0xa1, 0xf6, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_6[] = {
	{ 0xa1, 0xb8, 0, 0 },
	{ 0xc1, 0xd8, 0, 0 },
	{ 0xe0, 0xeb, 0, 0 },
	{ 0xee, 0xf2, 0, 0 },
	{ 0xf4, 0xf5, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_7[] = {
	{ 0xa1, 0xc1, 0, 0 },
	{ 0xd1, 0xf1, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_8[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0x80, 0x95, 0, 0 },
	{ 0xa1, 0xc0, 0, 0 },
	{ 0xc5, 0xe9, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_9[] = {
	{ 0x40, 0x57, 0, 0 },
	{ 0x59, 0x5a, 0, 0 },
	{ 0x5c, 0x5c, 0, 0 },
	{ 0x60, 0x7e, 0, 0 },
	{ 0x80, 0x96, 0, 0 },
	{ 0xa4, 0xef, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_10[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0x80, 0xa0, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg2_t seg2_11[] = {
	{ 0x40, 0x7e, 0, 0 },
	{ 0x80, 0xf9, 0, 0 },
	{ 0,    0,    0, 0 }	};

static cs_seg_t seg[] = {
	{ 0x81, 0xa0, 0, 0, 0, 0, seg2_1 },
	{ 0xa1, 0xa1, 0, 0, 0, 0, seg2_2 },
	{ 0xa2, 0xa2, 0, 0, 0, 0, seg2_3 },
	{ 0xa3, 0xa3, 0, 0, 0, 0, seg2_2 },
	{ 0xa4, 0xa4, 0, 0, 0, 0, seg2_4 },
	{ 0xa5, 0xa5, 0, 0, 0, 0, seg2_5 },
	{ 0xa6, 0xa6, 0, 0, 0, 0, seg2_6 },
	{ 0xa7, 0xa7, 0, 0, 0, 0, seg2_7 },
	{ 0xa8, 0xa8, 0, 0, 0, 0, seg2_8 },
	{ 0xa9, 0xa9, 0, 0, 0, 0, seg2_9 },
	{ 0xaa, 0xaf, 0, 0, 0, 0, seg2_10 },
	{ 0xb0, 0xd6, 0, 0, 0, 0, seg2_1 },
	{ 0xd7, 0xd7, 0, 0, 0, 0, seg2_11 },
	{ 0xd8, 0xf7, 0, 0, 0, 0, seg2_1 },
	{ 0xf8, 0xfe, 0, 0, 0, 0, seg2_10 },
	{ 0,    0,    0, 0, 0, 0, NULL    }	};

static char *wc_ascii =
    "¡¡£¡¡±££¡ç£¥£¦¡¯£¨£©£ª£«£¬£­£®£¯£°£±£²£³£´£µ£¶£·£¸£¹£º£»£¼£½£¾£¿£À"
    "£Á£Â£Ã£Ä£Å£Æ£Ç£È£É£Ê£Ë£Ì£Í£Î£Ï£Ð£Ñ£Ò£Ó£Ô£Õ£Ö£×£Ø£Ù£Ú£Û£Ü£Ý¡Ä¡®"
    "£á£â£ã£ä£å£æ£ç£è£é£ê£ë£ì£í£î£ï£ð£ñ£ò£ó£ô£õ£ö£÷£ø£ù£ú£ûØ­£ý¡«";

static char *imname_en = "Ó¢Êý";
static char *imname_sb = "°ë½Ç";
static char *imname_wc = "È«½Ç";

/*----------------------------------------------------------------------------

	GBK Charset Module Functions.

----------------------------------------------------------------------------*/

int
gbk_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen)
{
    return cs2_lnr2mbs(&csdata, seg, n_seg, mbs, mblen, wcs, wclen);
}

int
gbk_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen)
{
    return cs2_mbs2lnr(&csdata, seg, n_seg, wcs, wclen, mbs, mblen);
}

csdata_t *
gbk_init(int encid, xcin_rc_t *xc)
{
    n_seg = cs2_init(&csdata, seg);
    csdata.sch[1].n_char  = csdata.n_chcoded;
    csdata.wc_ascii.type  = (x_uint8)XCH_TYPE_WLINEAR;
    csdata.wc_ascii.encid = (x_uint8)encid;
    csdata.wc_ascii.c.w   = xcin_malloc(N_ASCII*sizeof(wchar_t), 0);
    csdata.wc_ascii.len   = (x_uint16)gbk_mbs2lnr(
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

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
#include <iconv.h>
#include "xcintool.h"
#include "charset.h"

/* 
 * UTF8 character set:  utf8
 *	encoding type:  CHSET_NOMODAL
 */

static sub_csdata_t data_subset[1] = {
    { "utf8",			/* subset_name */
      65535,			/* n_chars */
      4,			/* mblen */
      1 }			/* code_start */
};

static csdata_t csdata = {
    "UTF8",			/* charset_name */
    CHSET_NOMODAL,		/* charset_type */
    1,				/* n_subset */
    data_subset,		/* sub-charset list */
    65535,			/* n_chars */
    65535,			/* n_chcoded */
    4,				/* max_mblen */
};

static char *traditional_wc_ascii =
"　！”＃＄％＆’（）＊＋，－．╱０１２３４５６７８９：；＜＝＞？＠"
"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ〔╲〕︿ˍ‵"
"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～";

static char *traditional_imname_en = "英數";
static char *traditional_imname_sb = "半形";
static char *traditional_imname_wc = "全形";

static char *simplified_wc_ascii =
"　！”＃＄％＆’（）＊＋，－．／０１２３４５６７８９：；＜＝＞？＠"
"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ［＼］∧‘"
"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ｛丨｝～";

static char *simplified_imname_en  = "英数";
static char *simplified_imname_sb  = "半角";
static char *simplified_imname_wc  = "全角";


/*----------------------------------------------------------------------------

	UTF8 Charset Module Functions.

----------------------------------------------------------------------------*/

static void
to_ucs4(iconv_t cd, int encid, xch_t *wcs, char *mbs)
{
    char *inbuf, *outbuf;
    size_t wclen, inlen, outlen;
    wchar_t *wbuf;
    int i;

    wclen  = strlen(mbs) + 1;
    inbuf  = mbs;
    inlen  = wclen;
    wbuf = xcin_malloc(inlen*sizeof(wchar_t), 0);
    outbuf = (char *)wbuf;
    outlen = inlen;
    if (iconv(cd, &inbuf, &inlen, &outbuf, &outlen) > 0) {
	wcs->type  = (x_uint8)XCH_TYPE_WUCS4;
	wcs->encid = (x_uint8)encid;
	wcs->len   = wclen - outlen;
	wcs->c.w   = xcin_malloc(wcs->len*sizeof(wchar_t), 0);
	for (i=0; i<wcs->len; i++)
	    wcs->c.w[i] = wbuf[i];
    }
    free(wbuf);
}

csdata_t *
utf8_init(int encid, xcin_rc_t *xc)
{
    iconv_t cd;

    if (strncmp(xc->locale.lc_ctype, "zh_TW", 5) == 0 &&
	(cd = iconv_open("UCS4", "utf8")) != (iconv_t)(-1)) {
	to_ucs4(cd, encid, &(csdata.wc_ascii),  traditional_wc_ascii);
	to_ucs4(cd, encid, &(csdata.imname_en), traditional_imname_en);
	to_ucs4(cd, encid, &(csdata.imname_sb), traditional_imname_sb);
	to_ucs4(cd, encid, &(csdata.imname_wc), traditional_imname_wc);
	iconv_close(cd);
    }
    else if (strncmp(xc->locale.lc_ctype, "zh_CN", 5) == 0 &&
	     (cd = iconv_open("UCS4", "utf8")) == (iconv_t)(-1)) {
	to_ucs4(cd, encid, &(csdata.wc_ascii),  simplified_wc_ascii);
	to_ucs4(cd, encid, &(csdata.imname_en), simplified_imname_en);
	to_ucs4(cd, encid, &(csdata.imname_sb), simplified_imname_sb);
	to_ucs4(cd, encid, &(csdata.imname_wc), simplified_imname_wc);
	iconv_close(cd);
    }
    return &csdata;
}

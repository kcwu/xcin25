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

#ifndef _CHARSET_H
#define _CHARSET_H

typedef struct {
    int enc2a, enc2b;		/* 2nd byte encoding start/stop */
    int c_accu2;		/* accumulation in segment 2 */
    int c_cnt2;                 /* # of characters (2) in the segment */
} cs_seg2_t;

typedef struct {
    int enc1a, enc1b;		/* 1st byte encoding start/stop */
    int c_start;                /* linear coding start number */
    int c_cnt;			/* # of characters in the segment */
    int c_cnt2;			/* # of characters in each segment 2 */
    int n_seg2;			/* number of segment 2 */
    cs_seg2_t *seg2;
} cs_seg_t;

extern int cs2_init(csdata_t *cs, cs_seg_t *seg);
extern int cs2_lnr2mbs(csdata_t *cs, cs_seg_t *seg, int n_seg,
            char *mbs, int mblen, wchar_t *wcs, int wclen);
extern int cs2_mbs2lnr(csdata_t *cs, cs_seg_t *seg, int n_seg,
            wchar_t *wcs, int wclen, char *mbs, int mblen);

extern csdata_t *utf8_init(int encid, xcin_rc_t *xc);

extern csdata_t *big5_init(int encid, xcin_rc_t *xc);
extern int big5_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen);
extern int big5_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen);

extern csdata_t *big5hkscs_init(int encid, xcin_rc_t *xc);
extern int big5hkscs_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen);
extern int big5hkscs_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen);

extern csdata_t *gb2312_init(int encid, xcin_rc_t *xc);
extern int gb2312_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen);
extern int gb2312_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen);

extern csdata_t *gbk_init(int encid, xcin_rc_t *xc);
extern int gbk_lnr2mbs(char *mbs, int mblen, wchar_t *wcs, int wclen);
extern int gbk_mbs2lnr(wchar_t *wcs, int wclen, char *mbs, int mblen);

#endif

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

#ifndef _BIMSCIN_H
#define _BIMSCIN_H

#include <stdlib.h>
#include "constant.h"

/*
	Binary data file format of pinyin.tab for bimspinyin:

	char		verion[10];
	int		pinno;		number of pinpho_t.
	unsigned char	tone[6];	the tone keys.
	unsigned char	zhu[37*2+1];	the zhu-yin sequence.
	pinpho_t	pinpho[pinno];	sorting in pin_idx order.
	pinpho_t	phopin[pinno];	sorting in phone_idx order.
*/

#define BIMSCIN_VERSION  "20000404"

typedef struct {
    unsigned int pin_idx;	/* base: 26 */
    unsigned int phone_idx;	/* base: 256 */
} pinpho_t;

typedef struct {
    char version[10];
    int pinno;
    unsigned char tone[6];
    unsigned char zhu[83];
} pinyin_t;

#endif

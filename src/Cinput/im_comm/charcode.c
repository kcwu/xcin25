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
#include "module.h"

typedef struct {
    short n;
    ubyte_t begin[N_CCODE_RULE], end[N_CCODE_RULE];
    short num[N_CCODE_RULE], ac_num[N_CCODE_RULE];
    short total_num;
    unsigned long base;
} ccode_t;

static ccode_t charcode[WCH_SIZE];

static unsigned int total_char;
static byte_t highest_idx;

/* KhoGuan: After reading ccp[4] from sys.tab, we pass ccp to ccode_init(),
   calculate various numbers, fill them into charcode[4] 
   total_char is the number of total chars for an encoding.
*/
void
ccode_init(charcode_t *ccp, int n)
{
    int i, j, idx;

    for (i=0; i<WCH_SIZE && i<n && ccp[i].n; i++) {
	idx = charcode[i].n = ccp[i].n;
	for (j=0; j<idx; j++) {
	    charcode[i].begin[j] = ccp[i].begin[j];
	    charcode[i].end[j] = ccp[i].end[j];
	    charcode[i].num[j] = charcode[i].end[j] - charcode[i].begin[j] + 1;
	    charcode[i].total_num += charcode[i].num[j];
	    if (j > 0)
		charcode[i].ac_num[j] = 
			charcode[i].ac_num[j-1] + charcode[i].num[j-1];
	}
	if (i == 0)
	    charcode[i].base = 1;
	else
	    charcode[i].base = charcode[i-1].total_num * charcode[i-1].base;
    }
    total_char = charcode[i-1].total_num * charcode[i-1].base;
    highest_idx = i - 1;
}

void
ccode_info(ccode_info_t *info)
{
    int i, j;

    info->total_char = total_char;
    info->n_ch_encoding = highest_idx + 1;

    for (i=0; i<=highest_idx; i++) {
	memset(info->ccode+i, 0, sizeof(charcode_t));
	info->ccode[i].n = charcode[i].n;
	for (j=0; j<charcode[i].n; j++) {
	    info->ccode[i].begin[j] = charcode[i].begin[j];
	    info->ccode[i].end[j] = charcode[i].end[j];
	}
    }
}

int
match_encoding(wch_t *wch)
{
    int i, j, n;
    unsigned char *s = wch->s;

    for (i=0; i<WCH_SIZE && i<=highest_idx; i++, s++) {
	n = charcode[i].n;
	for (j=0; j<n; j++) {
	    if (*s >= charcode[i].begin[j] && *s <= charcode[i].end[j])
		break;
	}
	if (j >= charcode[i].n)
	    return 0;
    }
    return 1;
}
/* ccode_to_idx(): non-linear big5 char code to linear index number */
int
ccode_to_idx(wch_t *wch)
{
    int i, j, n, idx=0;
    ccode_t *ccp = charcode;
    ubyte_t *s = (ubyte_t *)(wch->s);
/* KhoGuan: for big5, only ccp[0] and ccp[1] are used, so ccp[2]->n == 0.
   If we has arg1 in our cin table with 2 chinese chars (4bytes), *wch
   will has its all 4 bytes filled. When i goes to 2,
     n = ccp->n; // ( n = 0 )
     for (j=0; j<n; j++) // no run
     if (j >= n)
         return -1;
   so cin_chardef() in gencin.c will get -1 and n_ignore++, and this entry 
   in our cin table will be ignored. Note that the main purpose of checking
   (j >= n) is to make sure the code point is between the valid range.
*/
    for (i=0; i<WCH_SIZE && *s; i++, ccp++, s++) {
	n = ccp->n;
	for (j=0; j<n; j++) {
            if (*s >= ccp->begin[j] && *s <= ccp->end[j])
                break;
	}
	if (j >= n)
	    return -1;
	idx += (*s - ccp->begin[j] + ccp->ac_num[j]) * ccp->base;
    }
    return (idx < total_char) ? idx : -1;
}
/* linear char code to non-linear big5 code */
int
ccode_to_char(int idx, unsigned char *mbs, int mbs_size)
{
    ccode_t *ccp = charcode + highest_idx;
    int i, j, n;
    int idx_tmp = idx;
    ubyte_t ch;

    if (idx < 0 || idx >= total_char || highest_idx >= mbs_size)
	return 0;

    memset(mbs, 0, mbs_size);
    for (i=highest_idx; i >= 0; i--, ccp--) {
	ch = (ubyte_t)(idx_tmp / ccp->base);
	idx_tmp -= ch * ccp->base;

	n = ccp->n;
	for (j=1; j<n && ch>=ccp->ac_num[j]; j++);
	mbs[i] = ch - ccp->ac_num[j-1] + ccp->begin[j-1];
    }
    return 1;
}



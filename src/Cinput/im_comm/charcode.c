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

static ccode_t charcode[N_ENC_SCHEMA * WCH_SIZE];
  
static unsigned int total_char[N_ENC_SCHEMA];
static byte_t highest_idx[N_ENC_SCHEMA];

/* KhoGuan: After reading ccp[4] from sys.tab, we pass ccp to ccode_init(),
   calculate various numbers, fill them into charcode[4] 
   total_char is the number of total chars for an encoding.
*/
void
ccode_init(charcode_t *ccp, int n)
{
    int i, j, idx, enc_schema;

    for (enc_schema = 0; enc_schema < N_ENC_SCHEMA; ++enc_schema) {
        charcode_t *tmp_ccp = ccp + enc_schema * WCH_SIZE;
        ccode_t    *tmp_charcode = charcode + enc_schema * WCH_SIZE;

	if (tmp_ccp[0].n == 0) return;

        for (i=0; i<WCH_SIZE && i<n && tmp_ccp[i].n; i++) {
	    idx = tmp_charcode[i].n = tmp_ccp[i].n;
	    for (j=0; j<idx; j++) {
	       tmp_charcode[i].begin[j] = tmp_ccp[i].begin[j];
	       tmp_charcode[i].end[j] = tmp_ccp[i].end[j];
	       tmp_charcode[i].num[j] = tmp_charcode[i].end[j] - tmp_charcode[i].begin[j] + 1;
	       tmp_charcode[i].total_num += tmp_charcode[i].num[j];
	       if (j > 0)
	   	  tmp_charcode[i].ac_num[j] = 
			tmp_charcode[i].ac_num[j-1] + tmp_charcode[i].num[j-1];
	    }
	    if (i == 0)
	        tmp_charcode[i].base = 1;
	    else
	        tmp_charcode[i].base = tmp_charcode[i-1].total_num * tmp_charcode[i-1].base;
        }
        total_char[enc_schema] = tmp_charcode[i-1].total_num * tmp_charcode[i-1].base;
        highest_idx[enc_schema] = i - 1;
    }
}

void
ccode_info(ccode_info_t *info)
{
    int i, j, enc_schema;

    info->total_char = 0;
    for (enc_schema = 0; enc_schema < N_ENC_SCHEMA; ++enc_schema) {
	ccode_t     *tmp_charcode = charcode + enc_schema * WCH_SIZE;
	charcode_t  *info_ccode = info->ccode + enc_schema * WCH_SIZE;

        info->total_char += total_char[enc_schema];
        info->n_ch_encoding[enc_schema] = highest_idx[enc_schema] + 1;

        for (i=0; i<=highest_idx[enc_schema]; i++) {
            memset(info_ccode+i, 0, sizeof(charcode_t));
	    info_ccode[i].n = tmp_charcode[i].n;
	    for (j=0; j<tmp_charcode[i].n; j++) {
	        info_ccode[i].begin[j] = tmp_charcode[i].begin[j];
	        info_ccode[i].end[j] = tmp_charcode[i].end[j];
	    }
        }
    }
}

static int
mbtowch_enc(wch_t *wch, const unsigned char *wch_str, size_t nbytes, int *schema)
{
    int i, j, n, enc_schema;

    for(enc_schema=0; enc_schema<N_ENC_SCHEMA; ++enc_schema) {
	ccode_t *tmp_charcode = charcode + enc_schema * WCH_SIZE;

	for(i=0; i<WCH_SIZE && i<=highest_idx[enc_schema] && i<nbytes; i++) {
	    n = tmp_charcode[i].n;
	    for(j=0; j<n; j++) {
	        if (wch_str[i] >= tmp_charcode[i].begin[j] && wch_str[i] <= tmp_charcode[i].end[j])
		    break;
	    }
	    if (j >= tmp_charcode[i].n)
	        break;
	}
	if ( i > highest_idx[enc_schema]) {
	    if(wch) {
		wch->wch='\0';
		memcpy(wch->s, wch_str, highest_idx[enc_schema]+1);
	    }
	    if(schema) {
		*schema=enc_schema;
	    }
	    return highest_idx[enc_schema]+1;
	}
    }
    return 0;
}

int
mbtowch(wch_t *wch, const char *wch_str, int nbytes)
{
    return mbtowch_enc(wch, wch_str, nbytes, NULL);
}

int
match_encoding(wch_t *wch)
{
    int enc_schema;
    if(mbtowch_enc(NULL, wch->s, WCH_SIZE, &enc_schema)==0)
	return 0;
    return enc_schema+1;
}

/* ccode_to_idx(): non-linear big5 char code to linear index number */
int
ccode_to_idx(wch_t *wch)
{
    int i, j, n, idx=0, enc_schema;
    ccode_t *ccp = charcode;
    ubyte_t *s = (ubyte_t *)(wch->s);
    if ((enc_schema = match_encoding(wch)) == 0) return -1;
    --enc_schema;
    ccp = charcode + enc_schema * WCH_SIZE; 

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

   if (idx > total_char[enc_schema]) return -1;

   while (enc_schema) idx += total_char[--enc_schema];

   return idx;
}
/* linear char code to non-linear big5 code */
int
ccode_to_char(int idx, unsigned char *mbs, int mbs_size)
{
    ccode_t *ccp;
    int i, j, n, enc_schema;
    int idx_tmp;
    ubyte_t ch;

    for (enc_schema = 0; enc_schema < N_ENC_SCHEMA; ++enc_schema)
	if (idx < total_char[enc_schema]) break;
	else idx -= total_char[enc_schema];
    if ( enc_schema == N_ENC_SCHEMA) return 0;

    idx_tmp = idx;
    ccp = charcode + enc_schema * WCH_SIZE + highest_idx[enc_schema];

    if (idx < 0 || idx >= total_char[enc_schema] || highest_idx[enc_schema] >= mbs_size)
	return 0;

    memset(mbs, 0, mbs_size);
    for (i=highest_idx[enc_schema]; i >= 0; i--, ccp--) {
	ch = (ubyte_t)(idx_tmp / ccp->base);
	idx_tmp -= ch * ccp->base;

	n = ccp->n;
	for (j=1; j<n && ch>=ccp->ac_num[j]; j++);
	mbs[i] = ch - ccp->ac_num[j-1] + ccp->begin[j-1];
    }
    return 1;
}



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

int
wchs_to_mbs(char *mbs, wch_t *wchs, int size)
{
    int i=0, j;
    wch_t *wch = wchs;
    char  *ch = mbs;

    if (! wch)
	return 0;

    while (wch->wch && i < size-1) {
	for (j=0; j<WCH_SIZE && wch->s[j]; j++, ch++) {
	    *ch = wch->s[j];
	    i ++;
	}
	wch ++;
    }
    *ch = '\0';

    return i;
}

int
nwchs_to_mbs(char *mbs, wch_t *wchs, int n_wchs, int size)
{
    int i=0, j, n_wch=0;
    wch_t *wch = wchs;
    char  *ch = mbs;

    if (! wch)
	return 0;

    while (wch->wch && n_wch < n_wchs && i < size-1) {
	for (j=0; j<WCH_SIZE && wch->s[j]; j++, ch++) {
	    *ch = wch->s[j];
	    i ++;
	}
	wch ++;
	n_wch ++;
    }
    *ch = '\0';

    return i;
}

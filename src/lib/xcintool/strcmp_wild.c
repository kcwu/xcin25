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
#include <string.h>
#include "xcintool.h"


static int
next_token(char **s, char *tok, int tok_size)
{
    char *s1;
    int token_len;

    if (! *s || ! **s || tok_size < 2)
	return 0;

    if (**s == '*') {
	do {
	    (*s) ++;
	} while (**s == '*' || **s == '?');
	tok[0] = '*';
	tok[1] = '\0';
    }
    else if (**s == '?') {
	(*s) ++;
	tok[0] = '?';
	tok[1] = '\0';
    }
    else {
	s1 = *s;
	while (*s1 && *s1 != '*' && *s1 != '?')
	    s1 ++;
	token_len = (int)(s1 - *s);
	if (token_len >= tok_size)
	    token_len = tok_size-1;
	strncpy(tok, *s, token_len);
	tok[token_len] = '\0';
	*s = s1;
    }
    return 1;
}

int
strcmp_wild(char *s1, char *s2)
{
    char *cp1=s1, *cp2=s2, tok[1024];
    int slen, ret=0;

    while (ret == 0 && *cp2 && next_token(&cp1, tok, 1024)) {
	if (*tok == '?') {
	    if (! *cp2)
		ret = 1;
	    else
		cp2 ++;
	}
	else if (*tok == '*') {
	    if (! next_token(&cp1, tok, 1024)) {
		while (*cp2)
		    cp2 ++;
	    }
	    else {
                slen = strlen(tok);
	        while (*cp2 && strncmp(tok, cp2, slen))
		    cp2 ++;
		if (*cp2)
		    cp2 += slen;
		else
		    ret = 1;
	    }
	}
	else {
	    if ((ret = strncmp(tok, cp2, strlen(tok))) != 0)
	        ret = (ret > 0) ? 1 : -1;
	    cp2 += strlen(tok);
	}
    }

    if (! ret) {
        if (*cp2)
	    ret = -1;
        else if (next_token(&cp1, tok, 1024)) {
	    if (*tok == '*' && ! next_token(&cp1, tok, 1024))
		ret = 0;
	    else
		ret = 1;
	}
    }
    return ret;
}


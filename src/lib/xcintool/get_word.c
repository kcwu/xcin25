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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "xcintool.h"

static char quote='"', backslash='\\';

int
get_word(char **line, char *word, int word_size, char *token)
{
    char *str = *line, *ret, *wd=word;
    char *strend;

    if (word_size < 2)
	return False;

    while (*str && (*str==' ' || *str=='\t' || *str=='\n'))
        ++str;
    if (! (*str)) {
        *line = str;
        return False;
    }
    else if (token && (ret = strchr(token, *str))) {
	*line = str + 1;
	word[0] = *ret;
	word[1] = '\0';
	return True;
    }

    if (quote == *str) {
        strend = str + 1;
        while (*strend && *strend != quote) {
            if (*strend == backslash && strend[1] == quote)
                ++strend;
	    *wd = *strend;
            ++strend;
	    ++wd;
        }
        *wd = '\0';
        if (*strend == quote)
            ++strend;
    }
    else {
        strend = str;
        while (*strend && (*strend!=' ' && *strend!='\t' && *strend!='\n')) {
	    if (token && strchr(token, *strend))
		break;
            if (*strend == backslash && strend[1] == quote)
                ++strend;
	    *wd = *strend;
            ++strend;
	    ++wd;
	}
        *wd = '\0';
    }

    while (*strend && (*strend==' ' || *strend=='\t' || *strend=='\n')) 
        ++strend;
    *line = strend;
    return True;
}


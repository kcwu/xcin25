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
#include "xcintool.h"

int
check_version(char *valid_version, char *version, int const_len)
{
    char *vv, *v;

    if (const_len) {
	if (strncmp(valid_version, version, const_len))
	    return 0;
	vv = valid_version + const_len;
	v  = version + const_len;
    }
    else {
	vv = valid_version;
	v  = version;
    }
    if (strlen(vv) != strlen(v))
	return 0;
    return (strcmp(vv, v) == 0) ? 1 : 0;
}


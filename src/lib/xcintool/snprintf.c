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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "xcintool.h"

int 
xcin_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    char *buf;
    int cnt;

    buf = malloc(size*5);
    va_start(ap, format);
    cnt = vsprintf(buf, format, ap);
    va_end(ap);
    strncpy(str, buf, size-1);
    free(buf);

    if (cnt < size-1) {
	str[cnt] = '\0';
	return cnt;
    }
    else {
	str[size-1] = '\0';
	return (size-1);
    }
}


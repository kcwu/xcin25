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

void *
xcin_malloc(size_t n_bytes, int reset)
{
    char *s;

    if ((s = malloc(n_bytes)) == NULL)
	perr(XCINMSG_IERROR, N_("xcin_malloc: memory exhaust.\n"));
    if (reset)
	memset(s, 0, n_bytes);
    return s;
}

void *
xcin_realloc(void *pt, size_t n_bytes)
{
    char *s;

    if ((s = realloc(pt, n_bytes)) == NULL)
	perr(XCINMSG_IERROR, N_("xcin_realloc: memory exhaust.\n"));
    return s;
}

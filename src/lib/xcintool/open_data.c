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
#include <string.h>
#include "xcintool.h"

static char *default_p, *user_p, *default_subp, *locale_subp;

void
set_open_data(char *default_path, char *user_path, 
	      char *default_sub_path, char *locale_sub_path)
{
    if (default_path)
	default_p = (char *)strdup(default_path);
    else
	perr(XCINMSG_IERROR, N_("NULL xcin default_path\n"));

    if (user_path)
	user_p = (char *)strdup(user_path);

    if (default_sub_path)
	default_subp = (char *)strdup(default_sub_path);
    else
	perr(XCINMSG_IERROR, N_("NULL xcin default_sub_path\n"));

    if (locale_sub_path)
	locale_subp  = (char *)strdup(locale_sub_path);
    else
	perr(XCINMSG_IERROR, N_("NULL xcin locale_sub_path\n"));
}

FILE *
open_data(char *fn, char *md, char *sub_path,
	  char *true_fn, int true_size, int exitcode)
{
    FILE *f;
    char *path, path_buf[256], *sp;
    int buf_size;

    if (true_fn) {
	path = true_fn;
	buf_size = true_size;
    }
    else {
	path = path_buf;
	buf_size = 256;
    }
/*
 *  For the obsulated path, try to open it directly.
 */
    if (fn[0] == '/') {
	strncpy(path, fn, buf_size);
	if ((f = fopen(path, md)))
	    return f;
	else
	    return NULL;
    }
/*
 *  Otherwise, search it in user data trees.
 */
    sp = (sub_path) ? sub_path : default_subp;
    if (user_p) {
	if (locale_subp) {
	    snprintf(path, buf_size, "%s/%s/%s/%s", user_p,sp,locale_subp,fn);
	    if ((f = fopen(path, md)))
		return f;
	}
	snprintf(path, buf_size, "%s/%s/%s", user_p, sp, fn);
	if ((f = fopen(path, md)))
	    return f;
	snprintf(path, buf_size, "%s/%s", user_p, fn);
	if ((f = fopen(path, md)))
	    return f;
    }
/*
 *  Otherwise, search it in xcin data trees.
 */
    if (locale_subp) {
	snprintf(path, buf_size, "%s/%s/%s/%s", default_p, sp, locale_subp, fn);
	if ((f = fopen(path, md)))
	    return f;
    }
    snprintf(path, buf_size, "%s/%s/%s", default_p, sp, fn);
    if ((f = fopen(path, md)))
	return f;
    snprintf(path, buf_size, "%s/%s", default_p, fn);
    if ((f = fopen(path, md)))
	return f;

    if (exitcode != XCINMSG_EMPTY && exitcode != XCINMSG_NORMAL)
	perr(exitcode, N_("cannot open data file: %s\n"), fn);

    return NULL;
}

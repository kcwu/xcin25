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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "xcintool.h"

static char *errhead;

void
set_perr(char *error_head)
{
    errhead = (char *)strdup(error_head);
}

void
perr(int msgcode, const char *fmt,...)
{
    va_list ap;
    int exitcode=0;
    FILE *fout;

    if (! errhead)
	errhead = "perr()";
    fout = (msgcode==XCINMSG_NORMAL || msgcode==XCINMSG_EMPTY) ? 
		stdout : stderr;
    switch (msgcode) {
    case XCINMSG_NORMAL:
	fprintf(fout, "%s: ", errhead);
	break;
    case XCINMSG_WARNING:
	fprintf(fout, _("%s: warning: "), errhead);
	break;
    case XCINMSG_IWARNING:
	fprintf(fout, _("%s internal: warning: "), errhead);
	break;
    case XCINMSG_ERROR:
	fprintf(fout, _("%s: error: "), errhead);
	exitcode = msgcode;
	break;
    case XCINMSG_IERROR:
	fprintf(fout, _("%s internal: error: "), errhead);
	exitcode = msgcode;
	break;
    }
    va_start(ap, fmt);
    vfprintf(fout, fmt, ap);
    va_end(ap);
    if (exitcode)
        exit(exitcode);
}

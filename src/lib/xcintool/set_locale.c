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

#ifdef HPUX
#  define _INCLUDE_XOPEN_SOURCE
#endif

#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include "constant.h"
#include "xcintool.h"

#ifdef HAVE_GETTEXT
#  include <libintl.h>
#endif
#ifdef HAVE_NL_LANGINFO
#  include <langinfo.h>
#endif

void 
locale_setting(char **lc_ctype, char **lc_messages, char **encoding, 
	       int exitcode)
{
    char *locale = NULL, *s;

#ifdef HAVE_GETTEXT
    if (lc_messages)
        *lc_messages = setlocale(LC_MESSAGES, "");
    textdomain("xcin");
    bindtextdomain("xcin", XCIN_MSGLOCAT);
#endif

    locale = setlocale(LC_CTYPE, "");
    if (! locale) {
	locale = getenv("LC_ALL");
	if (! locale)
	    locale = getenv("LC_CTYPE");
	if (! locale)
	    locale = getenv("LANG");
	if (! locale)
	    locale = "C";
	perr(exitcode, 
	     N_("C locale \"%s\" is not supported by your system.\n"), locale);
	locale = NULL;
    }
    if (lc_ctype)
        *lc_ctype = locale;
    if (locale && encoding) {
	*encoding = NULL;

#ifdef HAVE_NL_LANGINFO
	if ((s = nl_langinfo(CODESET)))
	    *encoding = (char *)strdup(s);
#else
	if ((s = strrchr(locale, '.')))
	    *encoding = (char *)strdup(s+1);
#endif
	if (*encoding) {
	    s = *encoding;
	    while (*s) {
		*s = (char)tolower(*s);
		s ++;
	    }
	}
    }
}

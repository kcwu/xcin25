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
#include "xcintool.h"
#include "module.h"

typedef struct {
    short n;
    char *s;
} qphr_t;

static qphr_t qphr[50], qphr_shift[50], qphr_ctrl[50],
	      qphr_alt[50], qphr_fallback[50];
static char true_fn[256];

void
qphrase_init(xcin_rc_t *xrc, char *phrase_fn)
{
    char *s, buf[256], cmd[15], var[80];
    int lineno=0, key;
    qphr_t *qp = qphr;
    FILE *fp=NULL;
   
    snprintf(buf, 256, "tab/%s", xrc->locale.encoding);
    if (check_datafile(phrase_fn, buf, xrc, true_fn, 256) == True)
	fp = open_file(true_fn, "rt", XCINMSG_WARNING);
    if (! fp)
	return;

    while (get_line(buf, 256, fp, &lineno, "#\n")) {
	s = buf;
	get_word(&s, cmd, 15, NULL);
	if (cmd[0] == '%') {
	    if (! strcmp(cmd, "%trigger"))
		qp = qphr;
	    else if (! strcmp(cmd, "%shift"))
		qp = qphr_shift;
	    else if (! strcmp(cmd, "%ctrl"))
		qp = qphr_ctrl;
	    else if (! strcmp(cmd, "%alt"))
		qp = qphr_alt;
	    else if (! strcmp(cmd, "%fallback"))
		qp = qphr_fallback;
	    else
		perr(XCINMSG_WARNING, 
		   N_("qphrase: %s: unknown tag: %s, ignore.\n"), true_fn, cmd);
	    continue;
	}
	if (! (key = key2code(cmd[0])))
	    continue;
	if (! get_word(&s, var, 80, NULL))
	    continue;

	qp[key].n = strlen(var);
	qp[key].s = (char *)strdup(var);
    }
}

char *
qphrase_str(int ch, int class)
{
    int key;
    qphr_t *qp;

    switch (class) {
	case QPHR_TRIGGER:	qp=qphr;		break;
	case QPHR_SHIFT:	qp=qphr_shift;		break;
	case QPHR_CTRL:		qp=qphr_ctrl;		break;
	case QPHR_ALT:		qp=qphr_alt;		break;
	case QPHR_FALLBACK:	qp=qphr_fallback;	break;
	default:		qp=qphr;		break;
    }
    if ((key = key2code(ch)) && qp[key].n > 0)
	return qp[key].s;
    else
	return NULL;
}

char *
get_qphrase_list(void)
{
    static char list[51];
    char *s=list;
    int i;

    if (list[0] != '\0')
	return list;

    for (i=0; i<50; i++) {
	if (qphr[i].n > 0) {
	    *s = code2key(i);
	    s ++;
	}
    }
    *s = '\0';
    return list;
}

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "constant.h"
#include "xcintool.h"
#include "xcin.h"

#define BUFSIZE		1024
#define DRAW_EMPTY	1
#define	DRAW_MCCH	2
#define	DRAW_PRE_MCCH	3
#define DRAW_LCCH	4

extern XIMS ims;

static unsigned long preedit_mode;

static char inpn_english[101];
static char inpn_sbyte[101];
static char inpn_2bytes[101];
static char status_buf[BUFSIZE] = {0};

static void
onspot_draw_multich(inpinfo_t *inpinfo)
{
    int i, j, n_groups, n, toggle_flag;
    wch_t *selkey, *cch;

    if (inpinfo->n_mcch == 0)
	return;

    cch = inpinfo->mcch;
    selkey = inpinfo->s_selkey;
    if (! inpinfo->mcch_grouping || inpinfo->mcch_grouping[0]==0) {
	toggle_flag = 0;
	n_groups = inpinfo->n_mcch;
    }
    else {
	toggle_flag = 1;
	n_groups = inpinfo->mcch_grouping[0];
    }
    for (i=0; i<n_groups && toggle_flag!=-1; i++, selkey++) {
	n = (toggle_flag > 0) ? inpinfo->mcch_grouping[i+1] : 1;
	if (selkey->wch != (wchar_t)0) {
            strcat(status_buf, " ");
            strcat(status_buf, selkey->s);
        }
        for (j=0; j<n; j++, cch++) {
	    if (cch->wch == (wchar_t)0) {
		toggle_flag = -1;
		break;
	    }
            strcat(status_buf, cch->s);
        }
    }
    if (! (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	return;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_BEGIN:
	strcat(status_buf, " >");
	break;
    case MCCH_MIDDLE:
	strcat(status_buf, " <>");
	break;
    case MCCH_END:
	strcat(status_buf, " <");
	break;
    }
}

static void
draw_lcch(inpinfo_t *inpinfo)
{
    char buf[BUFSIZE];

    if (inpinfo->n_lcch == 0)
	return;

    if (inpinfo->edit_pos < inpinfo->n_lcch) {
	wch_t *tmp = inpinfo->lcch + inpinfo->edit_pos;

	if (inpinfo->edit_pos > 0) {
	    nwchs_to_mbs(buf, inpinfo->lcch, inpinfo->edit_pos, BUFSIZE);
	    strcat(status_buf, buf);
	}
	strcat(status_buf, "(");
	strcat(status_buf, (char *)tmp->s);
	strcat(status_buf, ")");

        if (inpinfo->edit_pos < inpinfo->n_lcch - 1) {
            wchs_to_mbs(buf, inpinfo->lcch+inpinfo->edit_pos+1, BUFSIZE);
	    strcat(status_buf, buf);
        }
    }
    else {
        if (wchs_to_mbs(buf, inpinfo->lcch, BUFSIZE))
	    strcat(status_buf, buf);
    }
}

static char *mbstocts(gui_t *gui, char *s)
{
    XTextProperty tp;
    XmbTextListToTextProperty(gui->display, &s, 1, XCompoundTextStyle, &tp);
    return tp.value;
}

static void
onspot_status_start(IC *ic)
{
    IMStatusCBStruct data;

    data.major_code = XIM_STATUS_START;
    data.connect_id = ic->connect_id;
    data.icid       = ic->id;
    IMCallCallback (ims, (XPointer)&data);
    ic->status_is_start = True;
}

static void
onspot_status_draw(gui_t *gui, IC *ic)
{
    int len = strlen(status_buf);
    XIMText text;
    IMStatusCBStruct data;
    XIMFeedback feedback[1] = {0};
    char *tmp;

    data.todo.draw.type = XIMTextType;
    data.connect_id = ic->connect_id;
    data.icid = ic->id;
    data.major_code = XIM_STATUS_DRAW;
    data.todo.draw.data.text = &text;

    text.feedback = feedback;
    if (len == 0)
    {
	text.length = 0;
	text.string.multi_byte = "";
	IMCallCallback(ims, (XPointer)&data);
    }
    else
    {
	tmp = mbstocts(gui, status_buf);
	text.string.multi_byte = tmp;
	text.length = strlen(tmp);
	IMCallCallback(ims, (XPointer)&data);
	XFree(tmp);
    }
}

static void
onspot_preedit_start(IC *ic)
{
    IMPreeditCBStruct data;

    if (ic->preedit_is_start)
	return;

    data.major_code = XIM_PREEDIT_START;
    data.connect_id = ic->connect_id;
    data.icid       = ic->id;
    IMCallCallback (ims, (XPointer)&data);
    ic->preedit_is_start = True;
    ic->length = 0;
}

static void
onspot_preedit_done(IC *ic)
{
    IMPreeditCBStruct data;

    data.major_code = XIM_PREEDIT_DONE;
    data.connect_id = ic->connect_id;
    data.icid       = ic->id;
    IMCallCallback (ims, (XPointer)&data);
    ic->preedit_is_start = False;
}

static void
onspot_preedit_draw(gui_t *gui, IC *ic)
{
    char buf[256];
    int i;
    XIMText text;
    IMPreeditCBStruct data;
    XIMFeedback feedback[256] = {0};
    char *tmp;
    wch_t *keystroke = ic->imc->inpinfo.s_keystroke;
    int len = wchs_to_mbs(buf, keystroke, 254);

    if (!preedit_mode)
    {
        strcat(status_buf, buf);
	return;
    }

    if (!len && !ic->length)
	return;

    for(i=0; i<256; i++)
	feedback[i] = preedit_mode;

    data.major_code = XIM_PREEDIT_DRAW;
    data.connect_id = ic->connect_id;
    data.icid = ic->id;
    data.todo.draw.caret = XIMIsInvisible;
    data.todo.draw.chg_first = 0;
    data.todo.draw.chg_length = ic->length;
    data.todo.draw.text = &text;
    text.encoding_is_wchar = False;
    text.feedback = feedback;
    if (!len)
    {
	text.length = 0;
	text.string.multi_byte = "";
	feedback[0] = 0;
	IMCallCallback(ims, (XPointer)&data);
	ic->length = 0;
    }
    else
    {
	tmp = mbstocts(gui, buf);
	text.string.multi_byte = tmp;
	text.length = strlen(tmp);
	feedback[text.length] = 0;
	IMCallCallback(ims, (XPointer)&data);
	XFree(tmp);
	ic->length = ic->imc->inpinfo.keystroke_len;
	for (i = 0 ; i < strlen(buf) ; i++)
	{
	    if (buf[i] == '*' || buf[i] == '?')
		ic->length ++;
	}
    }
}

static void
draw_inpname(IM_Context_t *imc)
{
    char inpname[101], *inpn=NULL, *inpb=NULL;
    char *s, buf[101];

    inpb = ((imc->inp_state & IM_2BYTES)) ? inpn_2bytes : inpn_sbyte;

    if ((imc->inp_state & IM_CINPUT)) {
	s = imc->inpinfo.inp_cname;
	while (*s) {
	    if (*s == '%' && *(s+1) == '%' && *(s+2)) {
		inpn = s+2;
		break;
	    }
	    s++;
	}
	if (! inpn) {
	    strcpy(buf, imc->inpinfo.inp_cname);
	    inpn = buf;
	}
    }
    else
	inpn = inpn_english;

    snprintf(inpname, 100, "[%s|%s]", inpn, inpb);
    strcpy(status_buf, inpname);
}

static void
draw_cch_publish(IM_Context_t *imc)
{
    char *str, buf[256];
    int slen;

    if (imc->inpinfo.cch_publish.wch) {
	slen = snprintf(buf, 256, "%s:", imc->inpinfo.cch_publish.s);
	str = buf + slen;
	if (imc->sinmd_keystroke[0].wch &&
	    wchs_to_mbs(str, imc->sinmd_keystroke, 256-slen)) {
	    strcat(status_buf, buf);
	}
    }
}

static void
onspot_win_draw(gui_t *gui, winlist_t *win, IM_Context_t *imc, int flag)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    draw_inpname(imc);

    switch (flag) {
    case DRAW_MCCH:
	onspot_draw_multich(&(imc->inpinfo));
	break;
    case DRAW_LCCH:
	draw_lcch(&(imc->inpinfo));
	onspot_preedit_draw(gui, ic);
	break;
    case DRAW_PRE_MCCH:
	onspot_preedit_draw(gui, ic);
	onspot_draw_multich(&(imc->inpinfo));
	break;
    case DRAW_EMPTY:
	if ((imc->inp_state & IM_CINPUT)) {
	    onspot_preedit_draw(gui, ic);
	    draw_cch_publish(imc);
	}
	break;
    default:
	break;
    }
    onspot_status_draw(gui, ic);
}

static void
gui_onspot_draw(gui_t *gui, winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    IM_Context_t *imc;
    int idx, flag=0;

    if ((win->winmode & WMODE_EXIT) || ic==NULL ||
	ic->ic_rec.input_style != XIMSTY_OnSpot)
	return;

    imc = ic->imc;

    if ((imc->inp_state & IM_XIMFOCUS) || (imc->inp_state & IM_2BFOCUS)) {
	flag = DRAW_EMPTY;
	if ((imc->inp_state & IM_XIMFOCUS)) {
	    if (imc->inpinfo.n_lcch && (imc->inpinfo.guimode & GUIMOD_LISTCHAR))
		flag = DRAW_LCCH;
	    else if (imc->inpinfo.n_mcch) {
		if ((imc->inpinfo.guimode & GUIMOD_SELKEYSPOT))
		    flag = DRAW_MCCH;
		else
		    flag = DRAW_PRE_MCCH;
	    }
	    else if (imc->inpinfo.keystroke_len)
		flag = DRAW_PRE_MCCH;
	}
    }
    if (flag)
	onspot_win_draw(gui, win, imc, flag);
}

winlist_t *
gui_onspot_init(gui_t *gui, xccore_t *xccore)
{
    winlist_t *win=NULL;
    XSetWindowAttributes win_attr;

    strcpy(inpn_english, gui->inpn_english);
    strcpy(inpn_sbyte, gui->inpn_sbyte);
    strcpy(inpn_2bytes, gui->inpn_2bytes);

    win = gui_new_win();
    win->wtype = WTYPE_ONSPOT;
    win->imid  = 0;

    win->c_width  = 1;
    win->c_height = 1;
    win->width    = 10;
    win->height   = 10;
    win->pos_x    = 1;
    win->pos_y    = 1;
    win->data     = (void *)xccore;
    win->win_draw_func    = gui_onspot_draw;
    win->win_attrib_func  = NULL;
    win->win_destroy_func = NULL;
    win->window = XCreateSimpleWindow(gui->display, gui->root,
		win->pos_x, win->pos_y, win->width, win->height, 1,
		gui->fg_color, gui->bg_color);
    XSelectInput(gui->display, win->window, (ExposureMask|StructureNotifyMask));
    if (strcasecmp(xccore->irc->onspot_preedit_mode, "NONE") == 0)
	preedit_mode = 0;
    else if (strcasecmp(xccore->irc->onspot_preedit_mode, "UNDERLINE") == 0)
	preedit_mode = XIMUnderline;
    else
	preedit_mode = XIMReverse;

    return win;
}

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "constant.h"
#include "xcintool.h"
#include "xcin.h"

#define FIELD_STEP	5

#define OVERSPOT_USE_USRCOLOR	0x0001
#define OVERSPOT_USE_USRFONTSET	0x0002
#define OVERSPOT_DRAW_EMPTY	0x0004

#define DRAW_EMPTY	1
#define	DRAW_MCCH	2
#define	DRAW_PRE_MCCH	3
#define DRAW_LCCH	4

static xmode_t display_mode;
static char inpn_english[11];
static char inpn_sbyte[11];
static char inpn_2bytes[11];

#define GC_idx		0	/* Window fg_color, bg_color */
#define GCM_idx		1	/* For spot mark: mfg_color, mbg_color */
#define GCS_idx		2	/* For multi-cch selection key */
#define GCRM_idx	3	/* For lcch cursor: mfg_color */
#define GCLINE_idx	4	/* For underline mark: uline_color */

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window adjustment.

----------------------------------------------------------------------------*/

static void
overspot_location(gui_t *gui, winlist_t *win, 
		  ic_rec_t *ic_rec, int *pos_x, int *pos_y)
{
    int new_x, new_y;
    Window junkwin;

    new_x = ic_rec->pre_attr.spot_location.x + ic_rec->pre_attr.area.x;
    new_y = ic_rec->pre_attr.spot_location.y + ic_rec->pre_attr.area.y + 15;
    if (ic_rec->focus_win)
	XTranslateCoordinates(gui->display, ic_rec->focus_win, gui->root,
		new_x, new_y, pos_x, pos_y, &junkwin);
    else
	XTranslateCoordinates(gui->display, ic_rec->client_win, gui->root,
		new_x, new_y, pos_x, pos_y, &junkwin);

    if (*pos_x + win->width > gui->display_width)
	*pos_x = gui->display_width - win->width;
    if (*pos_y + win->height > gui->display_height)
	*pos_y = *pos_y - 40 - win->height;
}

static void
overspot_win_adjust(gui_t *gui, winlist_t *win, ic_rec_t *ic_rec, int winlen)
{
    int new_x, new_y;

    overspot_location(gui, win, ic_rec, &new_x, &new_y);
    if (new_x != win->pos_x || new_y != win->pos_y || winlen != win->width) {
	win->pos_x = new_x;
	win->pos_y = new_y;
	win->width = winlen;
	XMoveResizeWindow(gui->display, win->window, 
			  win->pos_x, win->pos_y, win->width, win->height);
	XRaiseWindow(gui->display, win->window);
    }
}

font_t *
update_IC_fontset(gui_t *gui, char *fontname)
{
    font_t *font;

    font = gui_create_fontset(fontname, 0);
    if (font == NULL)
	font = gui_create_fontset(gui->overspot_font, 1);
    if (font == NULL)
	font = gui_create_fontset(gui->font, 1);
    return font;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(XCIN colorful drawing)

----------------------------------------------------------------------------*/

static int
overspot_draw_multich(gui_t *gui, winlist_t *win, 
		      int x, int y, inpinfo_t *inpinfo)
{
    int i, j, n_groups, n, len=0, toggle_flag, spot_GC_idx;
    wch_t *selkey, *cch;
    char *pgstate;

    if (inpinfo->n_mcch == 0)
	return x;

    cch = inpinfo->mcch;
    selkey = inpinfo->s_selkey;
    spot_GC_idx = ((inpinfo->guimode & GUIMOD_SELKEYSPOT)) ? GCM_idx : GCS_idx;
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
	if ((len = wch_mblen(selkey))) {
	    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
			win->wingc[spot_GC_idx], x, y, (char *)selkey->s, len);
	    x += (XmbTextEscapement(win->font->fontset, 
			(char *)selkey->s, len) + 2);
        }
        for (j=0; j<n; j++, cch++) {
	    if (! (len = wch_mblen(cch))) {
		toggle_flag = -1;
		break;
	    }
	    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
			win->wingc[GC_idx], x, y, (char *)cch->s, len);
	    x += XmbTextEscapement(win->font->fontset, (char *)cch->s, len);
        }
	x += FIELD_STEP;
    }
    if (! (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	return x;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_BEGIN:
	pgstate = ">";
	len = 1;
	break;
    case MCCH_MIDDLE:
	pgstate = "<>";
	len = 2;
	break;
    case MCCH_END:
	pgstate = "<";
	len = 1;
	break;
    default:
	pgstate = NULL;
	break;
    }
    if (pgstate) {
	XmbDrawImageString(gui->display, win->window,
		win->font->fontset, win->wingc[GC_idx], x, y, pgstate, len);
	x += XmbTextEscapement(win->font->fontset, pgstate, len);
    }
    return x;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(XIM client specified color drawing)

----------------------------------------------------------------------------*/

#define BUFSIZE 1024

static int
overspot_draw_multichBW(gui_t *gui, winlist_t *win, 
			int x, int y, inpinfo_t *inpinfo)
{
    int i, j, n_groups, n, len=0, toggle_flag, bufidx=0;
    wch_t *selkey, *cch;
    char *pgstate, buf[BUFSIZE];

    if (inpinfo->n_mcch == 0) 
	return x;

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

    memset(buf, 0, BUFSIZE);
    buf[bufidx] = ' ';
    bufidx ++;
    for (i=0; i<n_groups && toggle_flag!=-1; i++, selkey++) {
	n = (toggle_flag > 0) ? inpinfo->mcch_grouping[i+1] : 1;
	if ((len = wch_mblen(selkey))) {
	    nwchs_to_mbs(buf+bufidx, selkey, 1, BUFSIZE-bufidx);
	    strncat(buf, " ", BUFSIZE-bufidx-len);
	    bufidx += (len + 1);
	}
	for (j=0; j<n; j++, cch++) {
	    if (! (len = wch_mblen(cch))) {
		toggle_flag = -1;
		break;
	    }
	    nwchs_to_mbs(buf+bufidx, cch, 1, BUFSIZE-bufidx);
	    bufidx += len;
	}
	strncat(buf, " ", BUFSIZE-bufidx);
	bufidx += 1;
    }
    if (! (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	return x;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_BEGIN:
	pgstate = ">";
	len = 1;
	break;
    case MCCH_MIDDLE:
	pgstate = "<>";
	len = 2;
	break;
    case MCCH_END:
	pgstate = "<";
	len = 1;
	break;
    default:
	pgstate = NULL;
	break;
    }
    if (pgstate) {
	strncat(buf, " ", BUFSIZE-bufidx);
	strncat(buf, pgstate, BUFSIZE-bufidx-2);
	bufidx += (len+1);
    }
    XmbDrawImageString(gui->display, win->window,
		win->font->fontset, win->wingc[0], x, y, buf, bufidx);
    x += XmbTextEscapement(win->font->fontset, buf, bufidx);

    return x;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(Main drawing function)

----------------------------------------------------------------------------*/

static void
draw_lcch_grouping(gui_t *gui, winlist_t *win, int x, int y,
                   wch_t *lcch, int n, ubyte_t *glist)
{
    int i, x2, n_cch=0, n_seg, len;
    char str[65];
    
    y = y + win->font->ef_height - win->font->ef_ascent + 1;
    for (i=0; i<n; i++) {
        n_seg = glist[i];
        len = nwchs_to_mbs(str, lcch+n_cch, n_seg, 65);
        x2 = x + XmbTextEscapement(win->font->fontset, str, len);
        if (n_seg > 1)
            XDrawLine(gui->display, win->window, 
                        win->wingc[GCLINE_idx], x+2, y, x2-5, y);
        x = x2;
        n_cch += n_seg;
    }
}

static int
draw_lcch(gui_t *gui, winlist_t *win, int x, int y, inpinfo_t *inpinfo)
{
    int len, gcrm_idx;
    char buf[BUFSIZE];

    if (inpinfo->n_lcch == 0)
	return x;

    if (inpinfo->lcch_grouping && (display_mode & OVERSPOT_USE_USRCOLOR))
	draw_lcch_grouping(gui, win, x, y, inpinfo->lcch,
		inpinfo->lcch_grouping[0], inpinfo->lcch_grouping+1);
    if (inpinfo->edit_pos < inpinfo->n_lcch) {
	wch_t *tmp = inpinfo->lcch + inpinfo->edit_pos;

	if (inpinfo->edit_pos > 0) {
	    len = nwchs_to_mbs(buf, inpinfo->lcch, inpinfo->edit_pos, BUFSIZE);
	    XmbDrawImageString(gui->display, win->window,
		win->font->fontset, win->wingc[GC_idx], x, y, buf, len);
	    x += XmbTextEscapement(win->font->fontset, buf, len);
	}

	len = wch_mblen(tmp);
	XmbDrawImageString(gui->display, win->window, win->font->fontset, 
		win->wingc[GCM_idx], x, y, (char *)tmp->s, len);
	x += XmbTextEscapement(win->font->fontset, (char *)tmp->s, len);

        if (inpinfo->edit_pos < inpinfo->n_lcch - 1) {
            len = wchs_to_mbs(buf, inpinfo->lcch+inpinfo->edit_pos+1, BUFSIZE);
            XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, buf, len);
	    x += XmbTextEscapement(win->font->fontset, buf, len);
        }
    }
    else {
	gcrm_idx = (win->n_gc > 2) ? GCRM_idx : GC_idx;
        len = wchs_to_mbs(buf, inpinfo->lcch, BUFSIZE);
        if (len) {
            XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, buf, len);
            x += XmbTextEscapement(win->font->fontset, buf, len);
        }
        else
            x = FIELD_STEP;
        XFillRectangle(gui->display, win->window, win->wingc[gcrm_idx], 
            x, 2, win->font->ef_width, win->font->ef_height);
	x += win->font->ef_width;
    }
    return x;
}

static int
draw_preedit(gui_t *gui, winlist_t *win, int x, int y, wch_t *keystroke)
{
    char buf[256];
    int len;

    if (! keystroke || keystroke[0].wch==(wchar_t)0)
	return x;

    buf[0] = '[';
    len = wchs_to_mbs(buf+1, keystroke, 254);
    buf[len+1] = ']';
    buf[len+2] = '\0';
    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
		win->wingc[GC_idx], x, y, buf, len+2);
    x += XmbTextEscapement(win->font->fontset, buf, len+2);
    return x;
}

static int
draw_inpname(gui_t *gui, winlist_t *win, int x, int y, IM_Context_t *imc)
{
    char inpname[15], *inpn=NULL, *inpb=NULL;
    char *s, buf[9];
    int len, gc_index;

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
	    extract_char(imc->inpinfo.inp_cname, buf, sizeof(buf));
	    inpn = buf;
	}
    }
    else
	inpn = inpn_english;

    if ((display_mode & OVERSPOT_USE_USRCOLOR)) {
	gc_index = GCLINE_idx;
	len = snprintf(inpname, 15, "%s|%s", inpn, inpb);
    }
    else {
	gc_index = GC_idx;
	len = snprintf(inpname, 15, "[%s|%s]", inpn, inpb);
    }
    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
		win->wingc[gc_index], x, y, inpname, len);
    x += (XmbTextEscapement(win->font->fontset, inpname, len) + 2*FIELD_STEP);
    return x;
}

static int
draw_cch_publish(gui_t *gui, winlist_t *win, int x, int y, IM_Context_t *imc)
{
    char *str, buf[256];
    int slen;

    if (imc->inpinfo.cch_publish.wch) {
	slen = snprintf(buf, 256, "%s:", imc->inpinfo.cch_publish.s);
	str = buf + slen;
	if (imc->sinmd_keystroke[0].wch &&
	    wchs_to_mbs(str, imc->sinmd_keystroke, 256-slen)) {
	    slen = strlen(buf);
	    XmbDrawImageString(gui->display, win->window, win->font->fontset,
			win->wingc[GC_idx], x, y, buf, slen);
	    x += XmbTextEscapement(win->font->fontset, buf, slen);
	}
    }
    return x;
}

static int
overspot_win_draw(gui_t *gui, winlist_t *win, IM_Context_t *imc, int flag)
{
    int x, y;
    int (*draw_multich)(gui_t *, winlist_t *, int, int, inpinfo_t *);

    if ((gui->winchange & WIN_CHANGE_IM))
	XClearWindow(gui->display, win->window);

    if (win->n_gc > 2)
	draw_multich = overspot_draw_multich;
    else
	draw_multich = overspot_draw_multichBW;
    x = FIELD_STEP;
    y = win->font->ef_ascent + 1;

    if ((display_mode & OVERSPOT_DRAW_EMPTY))
	x = draw_inpname(gui, win, x, y, imc);

    switch (flag) {
    case DRAW_MCCH:
	x = draw_multich(gui, win, x, y, &(imc->inpinfo));
	break;
    case DRAW_LCCH:
	x = draw_lcch(gui, win, x, y, &(imc->inpinfo)) + 2*FIELD_STEP;
	x = draw_preedit(gui, win, x, y, imc->inpinfo.s_keystroke);
	break;
    case DRAW_PRE_MCCH:
	x = draw_preedit(gui, win, x, y, imc->inpinfo.s_keystroke)+2*FIELD_STEP;
	x = draw_multich(gui, win, x, y, &(imc->inpinfo));
	break;
    case DRAW_EMPTY:
	if ((imc->inp_state & IM_CINPUT)) {
	    x = draw_preedit(gui, win, x, y, imc->inpinfo.s_keystroke);
	    x = draw_cch_publish(gui, win, x, y, imc);
	}
	break;
    default:
	break;
    }
    return x;
}

static void
gui_overspot_draw(gui_t *gui, winlist_t *win)
{
    IM_Context_t *imc = (IM_Context_t *)win->data;
    int x, flag=0;

    if ((win->winmode & WMODE_EXIT))
	return;

    if ((imc->inp_state & IM_XIMFOCUS) || (imc->inp_state & IM_2BFOCUS)) {
/*
 *  Check and change to the new fontset updated from the XIM client.
 */
	if (! (display_mode & OVERSPOT_USE_USRFONTSET) &&
	    (imc->ic_rec->ic_value_update & CLIENT_SETIC_PRE_FONTSET)) {
	    imc->ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_FONTSET;
	    gui_free_fontset(win->font);
	    win->font = update_IC_fontset(gui, imc->ic_rec->pre_attr.base_font);
	    win->width  = win->c_width * win->font->ef_width;
	    win->height = win->c_height * win->font->ef_height + 3;
	}
/*
 *  Updade the OverTheSpot window contents.
 */
	if ((display_mode & OVERSPOT_DRAW_EMPTY))
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
    if (flag) {
	XRaiseWindow(gui->display, win->window);
	gui_winmap_change(win, 1);
	x = overspot_win_draw(gui, win, imc, flag);
	overspot_win_adjust(gui, win, imc->ic_rec, x+FIELD_STEP*2);
    }
    else
	gui_winmap_change(win, 0);
}


/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window initialization.

----------------------------------------------------------------------------*/

winlist_t *
gui_overspot_init(gui_t *gui, IM_Context_t *imc, xmode_t xcin_mode)
{
    winlist_t *win=NULL;
    XSetWindowAttributes win_attr;

    if ((xcin_mode & XCIN_OVERSPOT_USRCOLOR))
	display_mode |= OVERSPOT_USE_USRCOLOR;
    if ((xcin_mode & XCIN_OVERSPOT_FONTSET))
	display_mode |= OVERSPOT_USE_USRFONTSET;
    if ((xcin_mode & XCIN_OVERSPOT_WINONLY))
	display_mode |= OVERSPOT_DRAW_EMPTY;
    extract_char(gui->inpn_english, inpn_english, 11);
    extract_char(gui->inpn_sbyte, inpn_sbyte, 11);
    extract_char(gui->inpn_2bytes, inpn_2bytes, 11);

    win = gui_new_win();
    win->wtype = WTYPE_OVERSPOT;
    win->imid  = imc->id;

    if ((display_mode & OVERSPOT_USE_USRFONTSET))
	win->font = update_IC_fontset(gui, NULL);
    else 
	win->font = update_IC_fontset(gui, imc->ic_rec->pre_attr.base_font);
    win->c_width  = 1;
    win->c_height = 1;
    win->width    = win->c_width * win->font->ef_width;
    win->height   = win->c_height * win->font->ef_height + 5;
    overspot_location(gui, win, imc->ic_rec, &(win->pos_x), &(win->pos_y));

    win->data = (void *)imc;
    win->win_draw_func    = gui_overspot_draw;
    win->win_attrib_func  = NULL;
    win->win_destroy_func = NULL;

    win->window = XCreateSimpleWindow(gui->display, gui->root, 
		win->pos_x, win->pos_y, win->width, win->height, 1,
		gui->fg_color, gui->bg_color);
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window, 
			CWOverrideRedirect, &win_attr);
    XSelectInput(gui->display, win->window, (ExposureMask|StructureNotifyMask));

/*  Setup GC  */
    if (!(display_mode & OVERSPOT_USE_USRCOLOR) &&
	(imc->ic_rec->ic_value_set & CLIENT_SETIC_PRE_FGCOLOR) &&
	(imc->ic_rec->ic_value_set & CLIENT_SETIC_PRE_BGCOLOR)) {
	XSetWindowBackground(gui->display, 
		       win->window, imc->ic_rec->pre_attr.background);
	win->n_gc = 2;
	win->wingc = malloc(sizeof(GC) * win->n_gc);

	win->wingc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GC_idx], 
		       imc->ic_rec->pre_attr.foreground);
	XSetBackground(gui->display, win->wingc[GC_idx], 
		       imc->ic_rec->pre_attr.background);

	win->wingc[GCM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetBackground(gui->display, win->wingc[GCM_idx], 
		       imc->ic_rec->pre_attr.foreground);
	XSetForeground(gui->display, win->wingc[GCM_idx], 
		       imc->ic_rec->pre_attr.background);
    }
    else {
	win->n_gc = 5;
	win->wingc = malloc(sizeof(GC) * win->n_gc);

	win->wingc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GC_idx], gui->fg_color);
	XSetBackground(gui->display, win->wingc[GC_idx], gui->bg_color);

	win->wingc[GCM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GCM_idx], gui->mfg_color);
	XSetBackground(gui->display, win->wingc[GCM_idx], gui->mbg_color);

	win->wingc[GCS_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GCS_idx], gui->fg_color);
	XSetBackground(gui->display, win->wingc[GCS_idx], gui->mbg_color);

	win->wingc[GCRM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GCRM_idx], gui->mbg_color);
	XSetBackground(gui->display, win->wingc[GCRM_idx], gui->bg_color);

	win->wingc[GCLINE_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, win->wingc[GCLINE_idx], gui->uline_color);
	XSetBackground(gui->display, win->wingc[GCLINE_idx], gui->black_color);
	XSetLineAttributes(gui->display, win->wingc[GCLINE_idx], 2,
			LineSolid, CapRound, JoinRound);
    }
    gui_overspot_draw(gui, win);
    return win;
}

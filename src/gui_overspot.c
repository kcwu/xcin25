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

struct ov_cli_s {		/* Indices by icid */
/*
    x_uint16 icid;
    x_int16  pos_update;
*/
    int icid;
    int pos_x, pos_y;
    GC gc[2];
    font_t *font;
};

#define OC_LENTH	64
static struct ov_cli_s **oc;
static int oc_edx, oc_len;

/*----------------------------------------------------------------------------

	OverTheSpot IC Checking.

----------------------------------------------------------------------------*/

static int
search_struct_oc(int icid)
{
    static int i;

    if (oc && oc[i] && oc[i]->icid == (x_uint16)icid)
	return i;
    for (i=0; i<oc_edx; i++) {
	if (oc[i] && oc[i]->icid == (x_uint16)icid)
	    return i;
    }
    i = 0;
    return -1;
}

static font_t *
update_IC_fontset(gui_t *gui, char *fontname)
{
    font_t *font=NULL;

    if (fontname)
	font = gui_create_fontset(fontname, 0);
    if (font == NULL)
	font = gui_create_fontset(gui->overspot_font, 1);
    if (font == NULL)
	font = gui_create_fontset(gui->font, 1);
    return font;
}

static void
overspot_check_ic(gui_t *gui, int idx, ic_rec_t *ic_rec)
{
/*
 *  Check and change to the new fontset updated from the XIM client.
 */
    if (! (display_mode & OVERSPOT_USE_USRFONTSET) &&
	(ic_rec->ic_value_update & CLIENT_SETIC_PRE_FONTSET)) {
	ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_FONTSET;
	gui_free_fontset(oc[idx]->font);
	oc[idx]->font = update_IC_fontset(gui, ic_rec->pre_attr.base_font);
    }
/*
 *  Check and change to the new colors updated from the XIM client.
 */
    if (! (display_mode & OVERSPOT_USE_USRCOLOR)) {
	if (ic_rec->ic_value_update & CLIENT_SETIC_PRE_FGCOLOR) {
	    ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_FGCOLOR;
	    XSetForeground(gui->display, oc[idx]->gc[GC_idx],
			ic_rec->pre_attr.foreground);
	    XSetBackground(gui->display, oc[idx]->gc[GCM_idx],
			ic_rec->pre_attr.foreground);
	}
	if (ic_rec->ic_value_update & CLIENT_SETIC_PRE_BGCOLOR) {
	    ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_BGCOLOR;
	    XSetBackground(gui->display, oc[idx]->gc[GC_idx],
			ic_rec->pre_attr.background);
	    XSetForeground(gui->display, oc[idx]->gc[GCM_idx],
			ic_rec->pre_attr.background);
	}
    }   
}

/*
void
gui_overspot_check_client(gui_t *gui, int icid)
{
    int idx;

    if ((idx = search_struct_oc(icid)) >= 0)
	oc[idx]->pos_update = 1;
}
*/

void
gui_overspot_delete_client(gui_t *gui, int icid)
{
    int idx;

    if ((idx = search_struct_oc(icid)) < 0)
	return;
    if (! (display_mode & OVERSPOT_USE_USRCOLOR)) {
	XFreeGC(gui->display, oc[idx]->gc[GC_idx]);
	XFreeGC(gui->display, oc[idx]->gc[GCM_idx]);
    }
    if (! (display_mode & OVERSPOT_USE_USRFONTSET))
	gui_free_fontset(oc[idx]->font);
    if (idx == oc_edx-1 && oc_edx > 1)
	oc_edx --;
    oc[idx]->icid = (x_uint16)0;
}

static int
overspot_register_ic(gui_t *gui, winlist_t *win, int icid, ic_rec_t *ic_rec)
{
    Window w, junkwin;
    int i, idx;

    for (idx=0; idx<oc_edx; idx++) {
	if (oc[idx] == NULL || oc[idx]->icid == 0)
	    break;
    }
    if (idx >= oc_edx) {
	if (oc_edx >= oc_len) {
	    oc_len += OC_LENTH;
	    oc = xcin_realloc(oc, sizeof(struct ov_cli_s)*oc_len);
	    for (i=oc_edx; i<oc_len; i++)
		oc[i] = NULL;
	}
	oc_edx ++;
    }
    if (oc[idx] == NULL)
	oc[idx] = xcin_malloc(sizeof(struct ov_cli_s), 0);
    else
	gui_overspot_delete_client(gui, idx+1);

    oc[idx]->icid = (x_uint16)icid;
/*
    oc[idx]->pos_update = 0;
*/
    w = (ic_rec->focus_win) ? ic_rec->focus_win : ic_rec->client_win;
    XTranslateCoordinates(gui->display, w, gui->root,
	0, 0, &(oc[idx]->pos_x), &(oc[idx]->pos_y), &junkwin);
    if (! (display_mode & OVERSPOT_USE_USRCOLOR)) {
	ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_FGCOLOR;
	ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_BGCOLOR;

	oc[idx]->gc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetForeground(gui->display, oc[idx]->gc[GC_idx],
			ic_rec->pre_attr.foreground);
	XSetBackground(gui->display, oc[idx]->gc[GC_idx],
			ic_rec->pre_attr.background);

	oc[idx]->gc[GCM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
	XSetBackground(gui->display, oc[idx]->gc[GCM_idx],
			ic_rec->pre_attr.foreground);
	XSetForeground(gui->display, oc[idx]->gc[GCM_idx],
			ic_rec->pre_attr.background);
    }
    if (! (display_mode & OVERSPOT_USE_USRFONTSET))
	oc[idx]->font = update_IC_fontset(gui, ic_rec->pre_attr.base_font);
    else
	oc[idx]->font = win->font;
    return idx;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window adjustment.

----------------------------------------------------------------------------*/

static void
overspot_win_adjust(gui_t *gui, winlist_t *win,
		    int idx, ic_rec_t *ic_rec, int winlen)
{
    int new_x, new_y;
    Window w, junkwin;

    w = (ic_rec->focus_win) ? ic_rec->focus_win : ic_rec->client_win;
/*
    if (oc[idx]->pos_update == 1) {
	XTranslateCoordinates(gui->display, w, gui->root,
		0, 0, &(oc[idx]->pos_x), &(oc[idx]->pos_y), &junkwin);
	oc[idx]->pos_update = 0;
    }
*/
    XTranslateCoordinates(gui->display, w, gui->root,
		0, 0, &(oc[idx]->pos_x), &(oc[idx]->pos_y), &junkwin);
    ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_SPOTLOC;
    ic_rec->ic_value_update &= ~CLIENT_SETIC_PRE_AREA;
    new_x = oc[idx]->pos_x +
	    ic_rec->pre_attr.spot_location.x + ic_rec->pre_attr.area.x;
    new_y = oc[idx]->pos_y +
	    ic_rec->pre_attr.spot_location.y + ic_rec->pre_attr.area.y + 15;
    if (new_x + winlen > gui->display_width)
	new_x = gui->display_width - winlen - 5;
    if (new_y + win->height > gui->display_height)
	new_y = new_y - 40 - win->height;

    if (new_x != win->pos_x || new_y != win->pos_y || winlen != win->width) {
	win->pos_x = new_x;
	win->pos_y = new_y;
	win->width = winlen;
	XMoveResizeWindow(gui->display, win->window, 
			  win->pos_x, win->pos_y, win->width, win->height);
	XRaiseWindow(gui->display, win->window);
    }
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(XCIN colorful drawing)

----------------------------------------------------------------------------*/

static int
overspot_draw_multich(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
		      int x, int y, inpinfo_t *inpinfo)
{
    int i, j, n_groups, n, x2, len=0, toggle_flag, spot_GC_idx;
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
/*
	if ((len = wch_mblen(selkey))) {
*/
	if (selkey->wch != (wchar_t)0) {
	    len = (selkey->s[1] != '\0') ? 2 : 1;
	    XmbDrawImageString(gui->display, win->window, font->fontset,
			gc[spot_GC_idx], x, y, (char *)selkey->s, len);
	    x += (XmbTextEscapement(font->fontset, (char *)selkey->s, len) + 2);
        }
        for (j=0; j<n; j++, cch++) {
/*
	    if (! (len = wch_mblen(cch))) {
*/
	    len = (cch->s[1] != '\0') ? 2 : 1;
	    if (cch->wch == (wchar_t)0) {
		toggle_flag = -1;
		break;
	    }
	    x2 = x + XmbTextEscapement(font->fontset, (char *)cch->s, len);
	    if (inpinfo->mcch_hint &&
		inpinfo->mcch_hint[(int)(cch-inpinfo->mcch)])
        	XDrawLine(gui->display, win->window, gc[GCLINE_idx],
        			x, y+2, x2, y+2);
	    XmbDrawImageString(gui->display, win->window, font->fontset, 
			gc[GC_idx], x, y, (char *)cch->s, len);
	    x = x2;
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
		font->fontset, gc[GC_idx], x, y, pgstate, len);
	x += XmbTextEscapement(font->fontset, pgstate, len);
    }
    return x;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(XIM client specified color drawing)

----------------------------------------------------------------------------*/

#define BUFSIZE 1024

static int
overspot_draw_multichBW(gui_t *gui, winlist_t *win, font_t *font , GC *gc,
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
/*
	if ((len = wch_mblen(selkey))) {
*/
	if (selkey->wch != (wchar_t)0) {
	    len = (selkey->s[1] != '\0') ? 2 : 1;
	    nwchs_to_mbs(buf+bufidx, selkey, 1, BUFSIZE-bufidx);
	    strncat(buf, " ", BUFSIZE-bufidx-len);
	    bufidx += (len + 1);
	}
	for (j=0; j<n; j++, cch++) {
/*
	    if (! (len = wch_mblen(cch))) {
*/
	    if (cch->wch == (wchar_t)0) {
		toggle_flag = -1;
		break;
	    }
	    len = (cch->s[1] != '\0') ? 2 : 1;
	    nwchs_to_mbs(buf+bufidx, cch, 1, BUFSIZE-bufidx);
	    bufidx += len;
	}
	strncat(buf, " ", BUFSIZE-bufidx);
	bufidx += 1;
    }
    if (inpinfo->guimode & GUIMOD_SELKEYSPOT) {
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
    }
    XmbDrawImageString(gui->display, win->window,
		font->fontset, gc[GC_idx], x, y, buf, bufidx);
    x += XmbTextEscapement(font->fontset, buf, bufidx);

    return x;
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window drawing function.
	(Main drawing function)

----------------------------------------------------------------------------*/

static void
draw_lcch_grouping(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
		   int x, int y, wch_t *lcch, int n, ubyte_t *glist)
{
    int i, x2, n_cch=0, n_seg, len;
    char str[65];
    
    y = y + font->ef_height - font->ef_ascent + 1;
    for (i=0; i<n; i++) {
        n_seg = glist[i];
        len = nwchs_to_mbs(str, lcch+n_cch, n_seg, 65);
        x2 = x + XmbTextEscapement(font->fontset, str, len);
        if (n_seg > 1)
            XDrawLine(gui->display, win->window, 
                        gc[GCLINE_idx], x+2, y, x2-5, y);
        x = x2;
        n_cch += n_seg;
    }
}

static int
draw_lcch(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
	  int x, int y, inpinfo_t *inpinfo)
{
    int len, gcrm_idx;
    char buf[BUFSIZE];

    if (inpinfo->n_lcch == 0)
	return x;

    if (inpinfo->lcch_grouping && (display_mode & OVERSPOT_USE_USRCOLOR))
	draw_lcch_grouping(gui, win, font, gc, x, y, inpinfo->lcch,
		inpinfo->lcch_grouping[0], inpinfo->lcch_grouping+1);
    if (inpinfo->edit_pos < inpinfo->n_lcch) {
	wch_t *tmp = inpinfo->lcch + inpinfo->edit_pos;

	if (inpinfo->edit_pos > 0) {
	    len = nwchs_to_mbs(buf, inpinfo->lcch, inpinfo->edit_pos, BUFSIZE);
	    XmbDrawImageString(gui->display, win->window,
		font->fontset, gc[GC_idx], x, y, buf, len);
	    x += XmbTextEscapement(font->fontset, buf, len);
	}
/*
	len = wch_mblen(tmp);
*/
	len = (tmp->s[1] == '\0') ? 1 : 2;
	XmbDrawImageString(gui->display, win->window, font->fontset, 
		gc[GCM_idx], x, y, (char *)tmp->s, len);
	x += XmbTextEscapement(font->fontset, (char *)tmp->s, len);

        if (inpinfo->edit_pos < inpinfo->n_lcch - 1) {
            len = wchs_to_mbs(buf, inpinfo->lcch+inpinfo->edit_pos+1, BUFSIZE);
            XmbDrawImageString(gui->display, win->window,
                font->fontset, gc[GC_idx], x, y, buf, len);
	    x += XmbTextEscapement(font->fontset, buf, len);
        }
    }
    else {
	gcrm_idx = (display_mode & OVERSPOT_USE_USRCOLOR) ? GCRM_idx : GC_idx;
        len = wchs_to_mbs(buf, inpinfo->lcch, BUFSIZE);
        if (len) {
            XmbDrawImageString(gui->display, win->window,
                font->fontset, gc[GC_idx], x, y, buf, len);
            x += XmbTextEscapement(font->fontset, buf, len);
        }
        else
            x = FIELD_STEP;
        XFillRectangle(gui->display, win->window, gc[gcrm_idx], 
            x, 2, font->ef_width, font->ef_height);
	x += font->ef_width;
    }
    return (x+2*FIELD_STEP);
}

static int
draw_preedit(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
	     int x, int y, wch_t *keystroke)
{
    char buf[256];
    int len;

    if (! keystroke || keystroke[0].wch==(wchar_t)0)
	return x;

    buf[0] = '[';
    len = wchs_to_mbs(buf+1, keystroke, 254);
    buf[len+1] = ']';
    buf[len+2] = '\0';
    XmbDrawImageString(gui->display, win->window, font->fontset, 
		gc[GC_idx], x, y, buf, len+2);
    x += (XmbTextEscapement(font->fontset, buf, len+2) + +2*FIELD_STEP);
    return x;
}

static int
draw_inpname(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
	     int x, int y, IM_Context_t *imc)
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
/*
	    extract_char(imc->inpinfo.inp_cname, buf, sizeof(buf));
*/
	    strncpy(buf, imc->inpinfo.inp_cname, 2);
	    buf[2] = '\0';
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
    XmbDrawImageString(gui->display, win->window, font->fontset, 
		gc[gc_index], x, y, inpname, len);
    x += (XmbTextEscapement(font->fontset, inpname, len)+2*FIELD_STEP);
    return x;
}

static int
draw_cch_publish(gui_t *gui, winlist_t *win, font_t *font, GC *gc,
		 int x, int y, IM_Context_t *imc)
{
    char *str, buf[256];
    int slen;

    if (imc->inpinfo.cch_publish.wch) {
	slen = snprintf(buf, 256, "%s:", imc->inpinfo.cch_publish.s);
	str = buf + slen;
	if (imc->sinmd_keystroke[0].wch &&
	    wchs_to_mbs(str, imc->sinmd_keystroke, 256-slen)) {
	    slen = strlen(buf);
	    XmbDrawImageString(gui->display, win->window, font->fontset,
			gc[GC_idx], x, y, buf, slen);
	    x += XmbTextEscapement(font->fontset, buf, slen);
	}
    }
    return x;
}

static int
overspot_win_draw(gui_t *gui, winlist_t *win, int idx,
		  IM_Context_t *imc, int flag)
{
    int x, y;
    GC *gc;
    font_t *font;
    int (*draw_multich)(gui_t*, winlist_t*, font_t*, GC*, int, int, inpinfo_t*);

    if ((gui->winchange & WIN_CHANGE_IM))
	XClearWindow(gui->display, win->window);

    if (display_mode & OVERSPOT_USE_USRCOLOR) {
	draw_multich = overspot_draw_multich;
	gc = win->wingc;
    }
    else {
	XSetWindowBorder(gui->display, win->window,
			 imc->ic_rec->pre_attr.foreground);
	XSetWindowBackground(gui->display, win->window,
			     imc->ic_rec->pre_attr.background);
	draw_multich = overspot_draw_multichBW;
	gc = oc[idx]->gc;
    }
    font = oc[idx]->font;
    x = FIELD_STEP;
    y = font->ef_ascent + 1;

    if ((display_mode & OVERSPOT_DRAW_EMPTY))
	x = draw_inpname(gui, win, font, gc, x, y, imc);

    switch (flag) {
    case DRAW_MCCH:
	x = draw_multich(gui, win, font, gc, x, y, &(imc->inpinfo));
	break;
    case DRAW_LCCH:
	x = draw_lcch(gui, win, font, gc, x, y, &(imc->inpinfo));
	x = draw_preedit(gui, win, font, gc, x, y, imc->inpinfo.s_keystroke);
	break;
    case DRAW_PRE_MCCH:
	x = draw_preedit(gui, win, font, gc, x, y, imc->inpinfo.s_keystroke);
	x = draw_multich(gui, win, font, gc, x, y, &(imc->inpinfo));
	break;
    case DRAW_EMPTY:
	if ((imc->inp_state & IM_CINPUT)) {
	    x = draw_preedit(gui, win, font, gc, x, y,imc->inpinfo.s_keystroke);
	    x = draw_cch_publish(gui, win, font, gc, x, y, imc);
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
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    IM_Context_t *imc;
    int idx, x, flag=0;

    if ((win->winmode & WMODE_EXIT) || ic==NULL ||
	ic->ic_rec.input_style != XIMSTY_OverSpot)
	return;

    imc = ic->imc;
    if ((idx = search_struct_oc(ic->id)) < 0)
	idx = overspot_register_ic(gui, win, ic->id, &(ic->ic_rec));

    if ((imc->inp_state & IM_XIMFOCUS) || (imc->inp_state & IM_2BFOCUS)) {
	overspot_check_ic(gui, idx, &(ic->ic_rec));
	win->height = win->c_height * oc[idx]->font->ef_height + 5;

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
	x = overspot_win_draw(gui, win, idx, imc, flag);
	overspot_win_adjust(gui, win, idx, &(ic->ic_rec), x+FIELD_STEP*2);
    }
    else
	gui_winmap_change(win, 0);
}


/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window initialization.

----------------------------------------------------------------------------*/

winlist_t *
gui_overspot_init(gui_t *gui, xccore_t *xccore)
{
    winlist_t *win=NULL;
    XSetWindowAttributes win_attr;

    if ((xccore->xcin_mode & XCIN_OVERSPOT_USRCOLOR))
	display_mode |= OVERSPOT_USE_USRCOLOR;
    if ((xccore->xcin_mode & XCIN_OVERSPOT_FONTSET))
	display_mode |= OVERSPOT_USE_USRFONTSET;
    if ((xccore->xcin_mode & XCIN_OVERSPOT_WINONLY))
	display_mode |= OVERSPOT_DRAW_EMPTY;
/*
    extract_char(gui->inpn_english, inpn_english, 11);
    extract_char(gui->inpn_sbyte, inpn_sbyte, 11);
    extract_char(gui->inpn_2bytes, inpn_2bytes, 11);
*/
    strncpy(inpn_english, gui->inpn_english, 2);
    strncpy(inpn_sbyte, gui->inpn_sbyte, 2);
    strncpy(inpn_2bytes, gui->inpn_2bytes, 2);
    inpn_english[2] = '\0';
    inpn_sbyte[2] = '\0';
    inpn_2bytes[2] = '\0';

    win = gui_new_win();
    win->wtype = WTYPE_OVERSPOT;
    win->imid  = 0;

    if ((display_mode & OVERSPOT_USE_USRFONTSET))
	win->font = update_IC_fontset(gui, NULL);
    win->c_width  = 1;
    win->c_height = 1;
    win->width    = 10;
    win->height   = 10;
    if (gui->mainwin2) {
	win->pos_x = gui->mainwin2->pos_x;
	win->pos_y = gui->mainwin2->pos_y - win->height - 5;
    }
    else {
	win->pos_x = gui->mainwin->pos_x;
	win->pos_y = gui->mainwin->pos_y - win->height - 5;
    }
    win->data = (void *)xccore;
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
    if (display_mode & OVERSPOT_USE_USRCOLOR) {
	win->n_gc = 5;
	win->wingc = xcin_malloc(sizeof(GC)*win->n_gc, 0);

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
    else {
	win->n_gc = 0;
	win->wingc = NULL;
    }
    return win;
}

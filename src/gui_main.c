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
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "constant.h"
#include "xcintool.h"
#include "gui.h"
#include "xcin.h"

#define FIELD_STEP  10

typedef struct {
    int x, width;
} subw_conf_t;

#define GC_idx		0	/* For xcin main window: fg_color, bg_color */
#define GCE_idx		1	/* For indexfont: GC_idx + indexfont */
#define GCM_idx		2	/* For spot mark: mfg_color, mbg_color */
#define GCRM_idx	3	/* For keystroke area background: mbg_color */
#define GCLINE_idx	4	/* For underline mark: uline_color */

#define CNAME_LENGTH	30
#define ENAME_LENGTH	30

typedef struct {
    char s_inpname[CNAME_LENGTH];
    char e_inpname[ENAME_LENGTH];
    char *inpn_english, *inpn_sbyte, *inpn_2bytes;
    subw_conf_t w_inpname, w_coding, w_show_coding, w_e_inpname;
    int star_pix;
} xcin_main_win_t;

static xcin_main_win_t xmw;


/*----------------------------------------------------------------------------

	XCIN Main Window drawing functions.

----------------------------------------------------------------------------*/

static void
inpstate_content(gui_t *gui, winlist_t *win, 
		 IC *ic, xmode_t xcin_mode, inp_state_t inp_state)
{
    char *inpn, *inpb, buf[CNAME_LENGTH];

    if ((inp_state & IM_XIMFOCUS)) {
	char *s = buf;
	strncpy(buf, ic->imc->inpinfo.inp_cname, CNAME_LENGTH);
	while (*s) {
	    if (*s == '%' && *(s+1) == '%') {
		*s = '\0';
		break;
	    }
	    s++;
	}
	inpn = buf;
    }
    else
	inpn = xmw.inpn_english;
    inpb = (! (inp_state & IM_2BFOCUS)) ? xmw.inpn_sbyte : xmw.inpn_2bytes;
    snprintf(xmw.s_inpname, CNAME_LENGTH, "[%s][%s]", inpn, inpb);
    xmw.w_inpname.x = FIELD_STEP;
    xmw.w_inpname.width = XmbTextEscapement(
		win->font->fontset, xmw.s_inpname, strlen(xmw.s_inpname));
    if (! ic)
	return;

    xmw.w_coding.width = ic->imc->inpinfo.area3_len * win->font->ef_width;
    xmw.w_coding.x = 2*FIELD_STEP + xmw.w_inpname.x + xmw.w_inpname.width;
    xmw.w_show_coding.x = 2*FIELD_STEP + xmw.w_coding.x + xmw.w_coding.width;

    if ((ic->imc->inp_state & IM_CINPUT)) {
	snprintf(xmw.e_inpname, ENAME_LENGTH, 
			"*%s", ic->imc->inpinfo.inp_ename);
        xmw.w_e_inpname.width = XTextWidth(gui->indexfont, 
			xmw.e_inpname, strlen(xmw.e_inpname)-1);
    }
}

static void
win_draw_multich(gui_t *gui, winlist_t *win, inpinfo_t *inpinfo)
{
    int i, j, n_groups, n, x, y, len=0, spot_GC_idx, toggle_flag;
    wch_t *selkey, *cch;
    char *pgstate;

    if ((cch = inpinfo->mcch) == NULL)
	return;
    x = FIELD_STEP;
    y = win->font->ef_ascent;
    selkey = inpinfo->s_selkey;
    spot_GC_idx = ((inpinfo->guimode & GUIMOD_SELKEYSPOT)) ? GCM_idx : GC_idx;
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
	    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
			win->wingc[spot_GC_idx], x, y, (char *)selkey->s, len);
	    x += (XmbTextEscapement(win->font->fontset, 
			(char *)selkey->s, len) + 5);
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
	    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
			win->wingc[GC_idx], x, y, (char *)cch->s, len);
	    x += XmbTextEscapement(win->font->fontset, (char *)cch->s, len);
	}
	x += FIELD_STEP;
    }

    switch (inpinfo->mcch_pgstate) {
    case MCCH_BEGIN:
	pgstate = ">";
	len = 1;
	break;
    case MCCH_MIDDLE:
	pgstate = "</>";
	len = 3;
	break;
    case MCCH_END:
	pgstate = "<";
	len = 1;
	break;
    default:
	pgstate = NULL;
	break;
    }
    if (pgstate)
	XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, pgstate, len);
}

static void
draw_lcch_grouping(gui_t *gui, winlist_t *win, 
		   wch_t *lcch, int n, ubyte_t *glist)
{
    int i, x1, x2, y, n_cch=0, n_seg, len;
    char str[65];
    
    x1 = FIELD_STEP;
    y  = win->font->ef_height+1;
    for (i=0; i<n; i++) {
	n_seg = glist[i];
	len = nwchs_to_mbs(str, lcch+n_cch, n_seg, 65);
	x2 = x1 + XmbTextEscapement(win->font->fontset, str, len);
	if (n_seg > 1)
	    XDrawLine(gui->display, win->window, 
			win->wingc[GCLINE_idx], x1+2, y, x2-5, y);
	x1 = x2;
	n_cch += n_seg;
    }
}

static void
win_draw_listcch(gui_t *gui, winlist_t *win, inpinfo_t *inpinfo)
{
    static int str_size;
    static char *str=NULL;
    int x, y, edit_pos, len;

    if (! inpinfo->lcch)
	return;
    if (inpinfo->lcch_grouping)
	draw_lcch_grouping(gui, win, inpinfo->lcch,
		inpinfo->lcch_grouping[0], inpinfo->lcch_grouping+1);
    x = FIELD_STEP;
    y = win->font->ef_ascent;
    len = WCH_SIZE * inpinfo->n_lcch;

    if (len >= str_size) {
	str_size = len+1;
	if (str)
	    free(str);
	str = calloc(str_size, sizeof(char));
    }

    edit_pos = inpinfo->edit_pos;
    if (edit_pos < inpinfo->n_lcch) {
        wch_t *tmp = inpinfo->lcch + edit_pos;

	if (edit_pos > 0) {
            len = nwchs_to_mbs(str, inpinfo->lcch, edit_pos, str_size);
            XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, str, len);
	    x += XmbTextEscapement(win->font->fontset, str, len);
	}
/*
	len = wch_mblen(tmp);
*/
	len = 4;
        XmbDrawImageString(gui->display, win->window,
            win->font->fontset, win->wingc[GCM_idx], x, y, (char *)tmp->s, len);
	x += XmbTextEscapement(win->font->fontset, (char *)tmp->s, len);

	if (edit_pos < inpinfo->n_lcch - 1) {
            len = wchs_to_mbs(str, inpinfo->lcch+edit_pos+1, str_size);
            XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, str, len);
	}
    }
    else {
        len = wchs_to_mbs(str, inpinfo->lcch, str_size);
	if (len) {
            XmbDrawImageString(gui->display, win->window,
                win->font->fontset, win->wingc[GC_idx], x, y, str, len);
	    x += XmbTextEscapement(win->font->fontset, str, len);
	}
	else
	    x = FIELD_STEP;
        XFillRectangle(gui->display, win->window, win->wingc[GCRM_idx], 
	    x, 0, win->font->ef_width, win->font->ef_height);
    }
}

static void 
win_draw(gui_t *gui, winlist_t *win, IC *ic, xmode_t xcin_mode)
{
    static inp_state_t pre_inp_state=0xff, pre_inp_num;
    inp_state_t inp_state, inp_num;
    int x, y, slen;
    char *str, buf[256];

    if (ic) {
	inp_state = ic->imc->inp_state;
	inp_num   = ic->imc->inp_num;
    }
    else {
	inp_state = (inp_state_t)0;
	inp_num   = (inp_state_t)0;
    }
    if (pre_inp_state != inp_state || pre_inp_num != inp_num)
	inpstate_content(gui, win, ic, xcin_mode, inp_state);
    pre_inp_state = inp_state;
    pre_inp_num   = inp_num;

    /* Draw area 2. */
    XClearWindow(gui->display, win->window);
    x = xmw.w_inpname.x;
    y = (win->c_height == 1) ? win->font->ef_ascent + 5: 
			       win->font->ef_height + win->font->ef_ascent + 5;
    str = xmw.s_inpname;
    XmbDrawImageString(gui->display, win->window,
            win->font->fontset, win->wingc[GC_idx], x, y, str, strlen(str));

    if (ic && (ic->imc->inp_state & IM_XIMFOCUS)) {
	IM_Context_t *imc = ic->imc;
	/* Draw area 1. */
	if ((imc->inpinfo.guimode & GUIMOD_LISTCHAR)) {
	    if (ic->ic_rec.input_style == XIMSTY_Root)
		win_draw_listcch(gui, win, &(imc->inpinfo));
	}
	else if (imc->inpinfo.n_mcch > 0) {
	    if (ic->ic_rec.input_style == XIMSTY_Root)
		win_draw_multich(gui, win, &(imc->inpinfo));
	}

	/* Draw area 3. */
	if (ic->ic_rec.input_style == XIMSTY_Root) {
	    x = xmw.w_coding.x;
	    XFillRectangle(gui->display, win->window, win->wingc[GCRM_idx], x, 
			y-win->font->ef_ascent, xmw.w_coding.width, 
			win->font->ef_height);
	    if ((slen = wchs_to_mbs(buf, imc->inpinfo.s_keystroke, 256)))
		XmbDrawImageString(gui->display,win->window,win->font->fontset, 
			win->wingc[GCM_idx], x, y, buf, slen);
	}

	/* Draw area 4. */
	if (imc->inpinfo.cch_publish.wch) {
            slen = snprintf(buf, 256, "%s:", imc->inpinfo.cch_publish.s);
	    str = buf + slen;
            if (imc->sinmd_keystroke[0].wch &&
	        wchs_to_mbs(str, imc->sinmd_keystroke, 256-slen)) {
	        if ((imc->inpinfo.guimode & GUIMOD_SINMDLINE1)) {
		    x = FIELD_STEP;
		    y = win->font->ef_ascent;
	        }
	        else
                    x = xmw.w_show_coding.x;
                XmbDrawImageString(gui->display,win->window,win->font->fontset, 
			win->wingc[GC_idx], x, y, buf, strlen(buf));
            }
	}

	/* Draw area 5. */
        str = xmw.e_inpname;
        if (str[0]) {
            x = win->width - FIELD_STEP - xmw.w_e_inpname.width;
            y = win->height - gui->indexfont->descent;
	    if (! (xcin_mode & XCIN_IM_FOCUS))
        	XDrawString(gui->display, win->window, 
	            win->wingc[GCE_idx], x, y, str+1, strlen(str+1));
	    else {
		x -= xmw.star_pix;
		XDrawString(gui->display, win->window,
		    win->wingc[GCE_idx], x, y, str, strlen(str));
	    }
        }
    }
}

static void
xcin_mainwin_draw(gui_t *gui, winlist_t *win)
{
    inp_state_t inp_state;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if ((win->winmode & WMODE_EXIT))
	return;

    inp_state = (ic) ? ic->imc->inp_state : 0;
    if (! (inp_state & IM_CINPUT) && ! (inp_state & IM_2BYTES)) {
	if ((xccore->xcin_mode & XCIN_MODE_HIDE) ||
	    (xccore->xcin_mode & XCIN_MAINWIN2)  ||
	    (xccore->xcin_mode & XCIN_OVERSPOT_WINONLY)) {
	    gui_winmap_change(win, 0);
	    return;
	}
    }
    else {
	if (((xccore->xcin_mode & XCIN_MAINWIN2) ||
	     (xccore->xcin_mode & XCIN_OVERSPOT_WINONLY)) && 
	    ic->ic_rec.input_style != XIMSTY_Root) {
	    gui_winmap_change(win, 0);
	    return;
	}
	if ((xccore->xcin_mode & XCIN_MODE_HIDE) ||
	    (xccore->xcin_mode & XCIN_MAINWIN2)  ||
	    (xccore->xcin_mode & XCIN_OVERSPOT_WINONLY))
	    gui_winmap_change(win, 1);
	if ((inp_state & IM_XIMFOCUS))
	    XRaiseWindow(gui->display, win->window);
    }
    win_draw(gui, win, ic, xccore->xcin_mode);
}

/*----------------------------------------------------------------------------

	XCIN Main window tool functions.

----------------------------------------------------------------------------*/

static void
xcin_mainwin_attrib(gui_t *gui, winlist_t *win, 
		    XConfigureEvent *event, int keep_flag)
{
    if (keep_flag) {
	if (event->x >= 0 && event->x <= gui->display_width &&
	    event->y >= 0 && event->y <= gui->display_height) {
	    win->pos_x = event->x;
	    win->pos_y = event->y;
	}
	else
	    XMoveWindow(gui->display, win->window, win->pos_x, win->pos_y);
    }
    else {
	win->pos_x = event->x;
	win->pos_y = event->y;
    }
    win->width = event->width;
    win->c_width = win->width / win->font->ef_width;
}

static void
xcin_mainwin_destroy(gui_t *gui, winlist_t *win)
{
    int i;
    xccore_t *xccore = (xccore_t *)win->data;
    IM_Context_t *imc;

    if (xccore->ic) {
	imc = xccore->ic->imc;
	for (i=0; i<imc->n_gwin; i++)
	    gui_freewin(imc->gwin[i].window);
	if (imc->overspot_win)
	    gui_freewin(imc->overspot_win);
    }
    xim_close(xccore->ic);
}

/*----------------------------------------------------------------------------

	XCIN Main window initialization.

----------------------------------------------------------------------------*/

static void
x_set_geometry(gui_t *gui, winlist_t *win, char *value, Bool *negx, Bool *negy)
{
    int r=0;
    int pos_x=0, pos_y=0;
    unsigned int width=0, height=0;
    
    if (value)
	r = XParseGeometry(value, &pos_x, &pos_y, &width, &height);
    win->pos_x = ((r & XValue)) ? pos_x : 100;
    win->pos_y = ((r & YValue)) ? pos_y : 100;
    win->c_width = ((r & WidthValue)) ? width : MIN_WIN_WIDTH;
    win->c_height = 2;
/*
 *  The width and height are measured in English characters.
 */
    if (win->c_width < MIN_WIN_WIDTH)
	win->c_width = MIN_WIN_WIDTH;
    win->width  = win->c_width  * win->font->ef_width;
    win->height = win->c_height * win->font->ef_height + 5;

    if (win->pos_x < 0) {
	win->pos_x += (gui->display_width - win->width);
	*negx = 1;
    }
    if (win->pos_y < 0) {
	win->pos_y += (gui->display_height - win->height);
	*negy = 1;
    }
}

static void 
set_wm_property(gui_t *gui, winlist_t *win, xcin_rc_t *xrc,
		Bool negative_x, Bool negative_y)
{
    char *win_name, *icon_name = "xcin";
    XTextProperty windowName, iconName;
    XSizeHints size_hints;
    XWMHints wm_hints;
    XClassHint class_hints;

    win_name = malloc(128);
    snprintf(win_name, 128, "xcin %s", XCIN_VERSION);
    if (! XStringListToTextProperty(&win_name, 1, &windowName) ||
    	! XStringListToTextProperty(&icon_name, 1, &iconName))
	perr(XCINMSG_IERROR, N_("string text property error.\n"));

    size_hints.flags = 
	USPosition | USSize | PMinSize | PResizeInc | PMaxSize | PWinGravity;
    size_hints.x = win->pos_x;
    size_hints.y = win->pos_y;
    size_hints.width = win->width;
    size_hints.height = win->height;
    size_hints.min_width = MIN_WIN_WIDTH * win->font->ef_width;
    size_hints.max_width = MAX_WIN_WIDTH * win->font->ef_width;
    size_hints.min_height = win->height;
    size_hints.max_height = size_hints.min_height;
    size_hints.width_inc = win->font->ef_width * 2;
    size_hints.height_inc = 1;
    if (negative_x)
        size_hints.win_gravity = 
	    (negative_y) ? SouthEastGravity: NorthEastGravity;
    else
        size_hints.win_gravity = 
	    (negative_y) ? SouthWestGravity: NorthWestGravity;

    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = False;
    wm_hints.initial_state = NormalState;

    class_hints.res_name = "xcin";
    class_hints.res_class = "xcin";

    XSetWMProperties(gui->display, win->window, &windowName, 
		&iconName, xrc->argv, xrc->argc, &size_hints, 
		&wm_hints, &class_hints);
    XFree(windowName.value);
    XFree(iconName.value);
    free(win_name);
}

static void 
set_GC(gui_t *gui, winlist_t *win)
{
    win->n_gc = 5;
    win->wingc = malloc(sizeof(GC) * win->n_gc);

    win->wingc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GC_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GC_idx], gui->bg_color);

    win->wingc[GCE_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCE_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GCE_idx], gui->bg_color);
    XSetFont(gui->display, win->wingc[GCE_idx], gui->indexfont->fid);

    win->wingc[GCM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCM_idx], gui->mfg_color);
    XSetBackground(gui->display, win->wingc[GCM_idx], gui->mbg_color);

    win->wingc[GCRM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCRM_idx], gui->mbg_color);
    XSetBackground(gui->display, win->wingc[GCRM_idx], gui->mbg_color);

    win->wingc[GCLINE_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCLINE_idx], gui->uline_color);
    XSetLineAttributes(gui->display, win->wingc[GCLINE_idx], 3, 
			LineSolid, CapRound, JoinRound);
}

winlist_t *
xcin_mainwin_init(gui_t *gui, xccore_t *xccore)
{
    winlist_t *win;
    int border;
    Bool negative_x=0, negative_y=0;
    char *cmd[1], geometry[256];

/*  Initially Setup  */
    xmw.inpn_english = gui->inpn_english;
    xmw.inpn_sbyte   = gui->inpn_sbyte;
    xmw.inpn_2bytes  = gui->inpn_2bytes;
    xmw.star_pix = XTextWidth(gui->indexfont, "*", 1);

    win = gui_new_win();
    win->wtype = (xtype_t)WTYPE_MAIN;
    win->imid  = 0;
    win->font  = gui_create_fontset(xccore->gui.font, 1);
    if (! win->font)
	perr(XCINMSG_ERROR, N_("fontset setting error.\n"));

/*  Winlist Setup  */
    cmd[0] = "X_GEOMETRY";
    if (! get_resource(&(xccore->xcin_rc), cmd, geometry, 256, 1))
	geometry[0] = '\0';
    x_set_geometry(gui, win, geometry, &negative_x, &negative_y);

    border = ((xccore->xcin_mode & XCIN_NO_WM_CTRL)) ? 3 : 1;
    win->pos_y = win->pos_y - 2*border;
    win->window = XCreateSimpleWindow(gui->display, gui->root, 
		win->pos_x, win->pos_y, win->width, win->height, 
		border, gui->fg_color, gui->bg_color);
    win->data = (void *)xccore;
    win->win_draw_func = xcin_mainwin_draw;
    win->win_attrib_func = xcin_mainwin_attrib;
    win->win_destroy_func = xcin_mainwin_destroy;

/*  Window Manager Property Setup  */
    if (! (xccore->xcin_mode & XCIN_NO_WM_CTRL)) {
	set_wm_property(gui, win, &(xccore->xcin_rc), negative_x, negative_y);
	if (! (xccore->xcin_mode & XCIN_XKILL_OFF))
            XSetWMProtocols(gui->display, win->window, &(gui->wm_del_win), 1);
    }
    else {
	XSetWindowAttributes win_attr;
	win_attr.override_redirect = True;
	XChangeWindowAttributes(gui->display, win->window,
				CWOverrideRedirect, &win_attr);
    }
    set_GC(gui, win);
    XSelectInput(gui->display, win->window, (ExposureMask|StructureNotifyMask));

    if (! (xccore->xcin_mode & XCIN_MODE_HIDE) && 
	! (xccore->xcin_mode & XCIN_OVERSPOT_WINONLY) &&
	! (xccore->xcin_mode & XCIN_MAINWIN2))
	gui_winmap_change(win, 1);
    return win;
}


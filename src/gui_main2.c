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
#include <iconv.h>
#include "constant.h"
#include "xcintool.h"
#include "gui.h"
#include "xcin.h"

#define FIELD_STEP  5

#define GC_idx		0	/* For xcin main window: fg_color, bg_color */
#define GCE_idx		1	/* For indexfont: GC_idx + indexfont */

#define CNAME_LENGTH	30
#define ENAME_LENGTH	30

typedef struct {
    char s_inpname[CNAME_LENGTH];
    char e_inpname[ENAME_LENGTH];
    int s_inpname_len, e_inpname_len;
    int s_inpname_pix, e_inpname_pix, star_pix;
    char inpn_english[11], inpn_sbyte[12], inpn_2bytes[12];
} xcin_main_win_t;

static xcin_main_win_t xmw2;


/*----------------------------------------------------------------------------

	XCIN Main Window 2 drawing functions.

----------------------------------------------------------------------------*/

static void
inpstate_content2(gui_t *gui, winlist_t *win, IC *ic, 
		  xmode_t xcin_mode, inp_state_t inp_state)
{
    char *inpn=NULL, *inpb, *s, buf[32];
    xccore_t *xccore = (xccore_t *)win->data;

    iconv_t cd;
    int inlen, outlen, len;
    char *inbuf, *outbuf;

    if ((inp_state & IM_XIMFOCUS)) {
	s = ic->imc->inpinfo.inp_cname;
	while (*s) {
	    if (*s == '%' && *(s+1) == '%' && *(s+2)) {
		inpn = s+2;
		break;
	    }
	    s++;
	}
	if (! inpn) {
	    cd = iconv_open("UCS4", xccore->xcin_rc.locale.encoding);
	    if (cd == (iconv_t)-1) {
		strncpy(buf, ic->imc->inpinfo.inp_cname, 2);
		buf[2] = '\0';
	    }
	    else {
		outbuf = buf;
		len    = strlen(ic->imc->inpinfo.inp_cname);
		inlen  = len;
		outlen = 4;
		inbuf  = ic->imc->inpinfo.inp_cname;
		iconv(cd, &inbuf, &inlen, &outbuf, &outlen);
		strncpy(buf, ic->imc->inpinfo.inp_cname, len-inlen);
		buf[len-inlen] = '\0';
		iconv_close(cd);
	    }
	    inpn = buf;
	}
    }
    else
        inpn = xmw2.inpn_english;
    inpb = (! (inp_state & IM_2BFOCUS)) ? xmw2.inpn_sbyte : xmw2.inpn_2bytes;
    xmw2.s_inpname_len = snprintf(xmw2.s_inpname, CNAME_LENGTH,
				"[%s][%s]", inpn, inpb);
    xmw2.s_inpname_pix = XmbTextEscapement(
		win->font->fontset, xmw2.s_inpname, xmw2.s_inpname_len);

    if (ic && (ic->imc->inp_state & IM_CINPUT)) {
        snprintf(xmw2.e_inpname, ENAME_LENGTH, 
                        "*%s", ic->imc->inpinfo.inp_ename);
	xmw2.e_inpname_len = strlen(xmw2.e_inpname) - 1;
        xmw2.e_inpname_pix = XTextWidth(gui->indexfont, 
                        xmw2.e_inpname + 1, xmw2.e_inpname_len);
    }
}

static void 
win_draw2(gui_t *gui, winlist_t *win, IC *ic, xmode_t xcin_mode)
{
    static inp_state_t pre_inp_state=0xff, pre_inp_num;
    inp_state_t inp_state, inp_num;
    int x, y;

    if (ic) {
	inp_state = ic->imc->inp_state;
	inp_num   = ic->imc->inp_num;
    }
    else {
	inp_state = (inp_state_t)0;
	inp_num   = (inp_state_t)0;
    }
    if (pre_inp_state != inp_state || pre_inp_num != inp_num)
	inpstate_content2(gui, win, ic, xcin_mode, inp_state);
    pre_inp_state = inp_state;
    pre_inp_num   = inp_num;

    XClearWindow(gui->display, win->window);
    x = FIELD_STEP;
    y = win->font->ef_ascent + 3;
    XmbDrawImageString(gui->display, win->window, win->font->fontset, 
		win->wingc[GC_idx], x, y, xmw2.s_inpname, xmw2.s_inpname_len);

    if (ic && (ic->imc->inp_state & IM_XIMFOCUS)) {
	char *str, buf[256];
	int slen;

        if (ic->imc->inpinfo.cch_publish.wch) {
            slen = snprintf(buf, 256, "%s:", ic->imc->inpinfo.cch_publish.s);
            str = buf + slen;
            if (ic->imc->sinmd_keystroke[0].wch &&
                wchs_to_mbs(str, ic->imc->sinmd_keystroke, 256-slen)) {
                x = xmw2.s_inpname_pix + 3*FIELD_STEP;
                XmbDrawImageString(gui->display,win->window,win->font->fontset, 
                        win->wingc[GC_idx], x, y, buf, strlen(buf));
            }
        }

	y = win->height - gui->indexfont->descent;
	x = win->width - xmw2.e_inpname_pix - FIELD_STEP;
        if (! (xcin_mode & XCIN_IM_FOCUS))
	   XDrawString(gui->display, win->window,
		win->wingc[GCE_idx], x,y, xmw2.e_inpname+1,xmw2.e_inpname_len);
	else {
	   x -= xmw2.star_pix;
	   XDrawString(gui->display, win->window,
		win->wingc[GCE_idx], x,y, xmw2.e_inpname,xmw2.e_inpname_len+1);
	}
    }
}

static void
xcin_mainwin2_draw(gui_t *gui, winlist_t *win)
{
    inp_state_t inp_state;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if ((win->winmode & WMODE_EXIT) ||
	(xccore->xcin_mode & XCIN_OVERSPOT_WINONLY))
	return;

    inp_state = (ic) ? ic->imc->inp_state : 0;
    if (! (inp_state & IM_CINPUT) && ! (inp_state & IM_2BYTES)) {
	if ((xccore->xcin_mode & XCIN_MODE_HIDE)) {
	    gui_winmap_change(win, 0);
	    return;
	}
    }
    if (gui->mainwin && (gui->mainwin->winmode & WMODE_MAP)) {
	gui_winmap_change(win, 0);
	return;
    }
    else {
	gui_winmap_change(win, 1);
	if ((inp_state & IM_XIMFOCUS))
	    XRaiseWindow(gui->display, win->window);
    }
    win_draw2(gui, win, ic, xccore->xcin_mode);
}


/*----------------------------------------------------------------------------

	XCIN Main window 2 tool functions.

----------------------------------------------------------------------------*/

static void
xcin_mainwin2_attrib(gui_t *gui, winlist_t *win, 
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
xcin_mainwin2_destroy(gui_t *gui, winlist_t *win)
{
    int i;
    xccore_t *xccore = (xccore_t *)win->data;
    IM_Context_t *imc;

    if (xccore->ic) {
	imc = xccore->ic->imc;
	for (i=0; i<imc->n_gwin; i++)
	    gui_freewin(imc->gwin[i].window);
    }
    xim_close(xccore->ic);
}

/*----------------------------------------------------------------------------

	XCIN Main window 2 initialization.

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
    win->c_height = 1;
/*
 *  The width and height are measured in English characters.
 */
    if (win->c_width < MIN_WIN_WIDTH2)
	win->c_width = MIN_WIN_WIDTH2;
    win->width  = win->c_width * win->font->ef_width;
    win->height = win->font->ef_height + 5;

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

    win_name = xcin_malloc(128, 0);
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
    size_hints.min_width = MIN_WIN_WIDTH2 * win->font->ef_width;
    size_hints.max_width = MAX_WIN_WIDTH  * win->font->ef_width;
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
    win->n_gc = 2;
    win->wingc = xcin_malloc(sizeof(GC)*win->n_gc, 0);

    win->wingc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GC_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GC_idx], gui->bg_color);

    win->wingc[GCE_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCE_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GCE_idx], gui->bg_color);
    XSetFont(gui->display, win->wingc[GCE_idx], gui->indexfont->fid);
}

winlist_t *
xcin_mainwin2_init(gui_t *gui, xccore_t *xccore)
{
    winlist_t *win;
    int border;
    Bool negative_x=0, negative_y=0;
    char *cmd[1], geometry[256];

    iconv_t cd;
    int inlen, outlen, len;
    char *inbuf, *outbuf, buf[1024];

/*  Initially Setup  */
    cd = iconv_open("UCS4", xccore->xcin_rc.locale.encoding);
    if (cd == (iconv_t)-1) {
	strncpy(xmw2.inpn_english, gui->inpn_english, 2);
	strncpy(xmw2.inpn_sbyte, gui->inpn_sbyte, 2);
	strncpy(xmw2.inpn_2bytes, gui->inpn_2bytes, 2);
	xmw2.inpn_english[2] = '\0';
	xmw2.inpn_sbyte[2] = '\0';
	xmw2.inpn_2bytes[2] = '\0';
    }
    else {
	outbuf = buf;
	len    = strlen(gui->inpn_english);
	inlen  = len;
	outlen = 4;
	inbuf  = gui->inpn_english;
	iconv(cd, &inbuf, &inlen, &outbuf, &outlen);
	strncpy(xmw2.inpn_english, gui->inpn_english, len-inlen);
	xmw2.inpn_english[len-inlen] = '\0';

	len    = strlen(gui->inpn_sbyte);
	inlen  = len;
	outlen = 4;
	inbuf  = gui->inpn_sbyte;
	iconv(cd, &inbuf, &inlen, &outbuf, &outlen);
	strncpy(xmw2.inpn_sbyte, gui->inpn_sbyte, len-inlen);
	xmw2.inpn_sbyte[len-inlen] = '\0';

	len    = strlen(gui->inpn_2bytes);
	inlen  = len;
	outlen = 4;
	inbuf  = gui->inpn_2bytes;
	iconv(cd, &inbuf, &inlen, &outbuf, &outlen);
	strncpy(xmw2.inpn_2bytes, gui->inpn_2bytes, len-inlen);
	xmw2.inpn_2bytes[len-inlen] = '\0';
	iconv_close(cd);
    }
    xmw2.star_pix = XTextWidth(gui->indexfont, "*", 1);

    win = gui_new_win();
    win->wtype = (xtype_t)WTYPE_MAIN;
    win->imid  = 0;
    win->font  = gui_create_fontset(xccore->gui.font, 1);
    if (! win->font)
	perr(XCINMSG_ERROR, N_("fontset setting error.\n"));

/*  Winlist Setup  */
    cmd[0] = "MAINWIN2_GEOMETRY";
    if (! get_resource(&(xccore->xcin_rc), cmd, geometry, 256, 1))
	geometry[0] = '\0';
    x_set_geometry(gui, win, geometry, &negative_x, &negative_y);
    border = ((xccore->xcin_mode & XCIN_NO_WM_CTRL)) ? 3 : 1;
    win->pos_y = win->pos_y - 2*border;
    win->window = XCreateSimpleWindow(gui->display, gui->root, 
		win->pos_x, win->pos_y, win->width, win->height, 
		border, gui->fg_color, gui->bg_color);
    win->data = (void *)xccore;
    win->win_draw_func = xcin_mainwin2_draw;
    win->win_attrib_func = xcin_mainwin2_attrib;
    win->win_destroy_func = xcin_mainwin2_destroy;

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
	(xccore->xcin_mode & XCIN_MAINWIN2))
	gui_winmap_change(win, 1);
    return win;
}


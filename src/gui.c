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

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "constant.h"
#include "gui.h"
#include "xcin.h"

winlist_t *xcin_mainwin_init(gui_t *gui, xccore_t *xccore);
winlist_t *xcin_mainwin2_init(gui_t *gui, xccore_t *xccore);
winlist_t *gui_overspot_init(gui_t *gui, xccore_t *xccore);
winlist_t *gui_menusel_init(gui_t *gui, int imid, greq_win_t *gw);
/* Add By Firefly(firefly@firefly.idv.tw) */
winlist_t *gui_onspot_init(gui_t *gui, xccore_t *xccore);
void gui_overspot_check_client(gui_t *gui, int icid);
void gui_overspot_delete_client(gui_t *gui, int icid);
void xim_terminate(void);
int ic_destroy(int icid, xccore_t *xccore);

static gui_t *gui;


/*----------------------------------------------------------------------------

	GUI fontset management.

----------------------------------------------------------------------------*/

typedef struct ifont_s {
    font_t fontset;
    int ref_cnt;
    struct ifont_s *next, *prev;
} ifont_t;

static ifont_t *fontlist, *freef_head, *freef_end;

font_t *
gui_create_fontset(char *base_font, int verb)
{
    int charset_count=0, i, fsize;
    int ef_width, ef_height, ef_ascent;
    ifont_t *ff;
    char **charset_list=NULL, *def_string=NULL;
    XFontSet fontset=(XFontSet)NULL;
    XFontStruct **font_structs;

    if (! base_font)
	return NULL;
/*
 *  Search the existing fontset in the fontlist and free_font list.
 */
    ff = fontlist;
    while (ff) {
	if (strcmp(base_font, ff->fontset.basename) == 0) {
	    ff->ref_cnt ++;
	    return &(ff->fontset);
	}
	ff = ff->next;
    }
    ff = freef_head;
    while (ff) {
	if (strcmp(base_font, ff->fontset.basename) == 0) {
	    if (ff->prev)
		ff->prev->next = ff->next;
	    else
		freef_head = ff->next;
	    if (ff->next)
		ff->next->prev = ff->prev;
	    else
		freef_end = ff->prev;

	    ff->next = fontlist;
	    ff->prev = NULL;
	    fontlist = ff;
	    ff->ref_cnt ++;
	    return &(ff->fontset);
	}
	ff = ff->next;
    }
    if (freef_end) {
	ff = freef_end;
	freef_end = ff->prev;
	if (freef_end)
	    freef_end->next = NULL;
	else
	    freef_head = NULL;
	free(ff->fontset.basename);
	XFreeFontSet(gui->display, ff->fontset.fontset);
    }
/*
 *  Create a new fontset.
 */
    fontset = XCreateFontSet(gui->display, base_font,
			&charset_list, &charset_count, &def_string);
    if (charset_count || ! fontset) {
	for (i=0; i<charset_count; i++) {
	    if (verb)
		perr(XCINMSG_WARNING, N_("invalid font %s.\n"),charset_list[i]);
	}
	if (charset_list)
	    XFreeStringList(charset_list);
	if (fontset)
	    XFreeFontSet(gui->display, fontset);
	return NULL;
    }

    charset_count = XFontsOfFontSet(fontset, &font_structs, &charset_list);
    ef_width  = 0;
    ef_height = 0;
    ef_ascent = 0;
    for (i=0; i<charset_count; i++) {
	fsize = font_structs[i]->max_bounds.width / 2;
	if (fsize > ef_width)
	    ef_width = fsize;
	fsize = font_structs[i]->ascent + font_structs[i]->descent;
	if (fsize > ef_height) {
	    ef_height = fsize;
	    ef_ascent = font_structs[i]->ascent;
        }
    }
    if (ff == NULL)
	ff = xcin_malloc(sizeof(ifont_t), 0);
    ff->next = fontlist;
    ff->prev = NULL;
    fontlist = ff;
    ff->fontset.basename  = (char *)strdup(base_font);
    ff->fontset.fontset   = fontset;
    ff->fontset.ef_width  = ef_width;
    ff->fontset.ef_height = ef_height;
    ff->fontset.ef_ascent = ef_ascent;
    ff->ref_cnt = 1;
    return &(ff->fontset);
}

void
gui_free_fontset(font_t *base_font)
{
    ifont_t *ff=fontlist;

    while (ff) {
	if (base_font == &(ff->fontset)) {
	    ff->ref_cnt --;
	    if (ff->ref_cnt <= 0) {
		if (ff->prev)
		    ff->prev->next = ff->next;
		else
		    fontlist = ff->next;
		if (ff->next)
		    ff->next->prev = ff->prev;

		ff->prev = NULL;
		ff->next = freef_head;
		freef_head = ff;
		if (freef_end == NULL)
		    freef_end = ff;
	    }
	    return;
	}
	ff = ff->next;
    }
}

/*----------------------------------------------------------------------------

	Basic WinList handling functions.

----------------------------------------------------------------------------*/

static winlist_t *winlist, *free_win;

static void 
free_win_content(winlist_t *win)
{
    int i;

    if ((win->winmode & WMODE_EXIT))
	return;

    if (win->win_destroy_func)
	win->win_destroy_func(gui, win);
    else {
	XUnmapWindow(gui->display, win->window);
	win->winmode &= ~WMODE_MAP;

	gui_free_fontset(win->font);
	for (i=0; i<win->n_gc; i++)
	    XFreeGC(gui->display, win->wingc[i]);
	XFree(win->wingc);

	XDestroyWindow(gui->display, win->window);
    }
    win->winmode |= WMODE_EXIT;
}

winlist_t *
gui_search_win(Window window)
{
    winlist_t *w = winlist;

    while (w) {
	if (w->window == window)
	    return w;
	w = w->next;
    }
    return NULL;
}

winlist_t *
gui_new_win(void)
{
    winlist_t *win;

    if (free_win) {
	win = free_win;
	free_win = free_win->next;
	memset(win, 0, sizeof(winlist_t));
    }
    else
	win = xcin_malloc(sizeof(winlist_t), 1);
    win->next = winlist;
    winlist = win;

    return win;
}

void
gui_freewin(Window window)
{
    winlist_t *w = winlist, *wp = NULL;

    while (w) {
	if (w->window == window) {
	    free_win_content(w);
	    if (wp == NULL)
		winlist = w->next;
	    else
		wp->next = w->next;
	    w->next = free_win;
	    free_win = w;
	    return;
	}
	wp = w;
	w  = w->next;
    }
}

void
gui_winmap_change(winlist_t *win, int state)
{
    if ((win->winmode & WMODE_EXIT))
	return;

    if (state) {
	XMapWindow(gui->display, win->window);
	win->winmode |= WMODE_MAP;
    }
    else {
	XUnmapWindow(gui->display, win->window);
	win->winmode &= ~WMODE_MAP;
    }
}

int
gui_check_window(Window window)
{
    Window root;
    int x, y;
    unsigned width, height, bw, depth;

    XGetGeometry(gui->display, window,
		&root, &x, &y, &width, &height, &bw, &depth);
    XSync(gui->display, False);
    if (errstatus == 0)
	return True;
    else {
	errstatus = 0;
	return False;
    }
}

int
gui_check_input_focus(xccore_t *xccore, Window win)
{
    Window w;
    int revert_to_return;

    XGetInputFocus(xccore->gui.display, &w, &revert_to_return);
    return (w == win) ? True : False;
}

/*----------------------------------------------------------------------------

	GUI Initialization & Main loop

----------------------------------------------------------------------------*/

static unsigned long 
x_set_color(char *color_name)
{
    XColor color;

    if (! XParseColor(gui->display, gui->colormap, color_name, &color))
	return  0;
    if (! XAllocColor(gui->display, gui->colormap, &color))
	return  0;

    return  color.pixel;
}

static int errhandle(Display *disp, XErrorEvent *xevent)
{
    errstatus = xevent->error_code;
    return 1;
}

void
gui_init(xccore_t *xccore)
{
    gui = &(xccore->gui);

    if (! (gui->display = XOpenDisplay(xccore->irc->display_name)))
	perr(XCINMSG_ERROR, N_("cannot open display: %s\n"), 
			    xccore->irc->display_name);
    (void) XSetErrorHandler(errhandle);
    gui->screen = DefaultScreen(gui->display);
    gui->colormap = DefaultColormap(gui->display, gui->screen);
    gui->display_width = DisplayWidth(gui->display, gui->screen);
    gui->display_height = DisplayHeight(gui->display, gui->screen);
    gui->root = RootWindow(gui->display, gui->screen);

    gui->fg_color    = x_set_color(xccore->irc->fg_color);
    gui->bg_color    = x_set_color(xccore->irc->bg_color);
    gui->mfg_color   = x_set_color(xccore->irc->mfg_color);
    gui->mbg_color   = x_set_color(xccore->irc->mbg_color);
    gui->uline_color = x_set_color(xccore->irc->uline_color);
    gui->grid_color  = x_set_color(xccore->irc->grid_color);
    gui->black_color = BlackPixel(gui->display, gui->screen);
    gui->white_color = WhitePixel(gui->display, gui->screen);
    gui->wm_del_win  = XInternAtom(gui->display, "WM_DELETE_WINDOW", False);

    if (! (gui->indexfont=XLoadQueryFont(gui->display,xccore->irc->indexfont)))
        perr(XCINMSG_ERROR, N_("invalid INDEX_FONT: %s\n"),
			xccore->irc->indexfont);
    if ((xccore->xcin_mode & XCIN_MAINWIN2))
	gui->mainwin2 = xcin_mainwin2_init(gui, xccore);
    else
	gui->mainwin  = xcin_mainwin_init(gui, xccore);
    gui->overspot_win = gui_overspot_init(gui, xccore);
    /* Add by Firefly(firefly@firefly.idv.tw) */
    gui->onspot_win = gui_onspot_init(gui, xccore);
    gui->winchange |= WIN_CHANGE_IM;
}

void 
gui_loop(xccore_t *xccore)
{
    XEvent event;
    winlist_t *win;

    while (1) {
	if ((xccore->xcin_mode & XCIN_RUN_EXITALL)) {
	    xim_terminate();
	    IM_free_all();
	    exit(0);
	}
        XNextEvent(gui->display, &event);
	if (XFilterEvent(&event, None) == True)
            continue;

	switch (event.type) {
	case Expose:
	    if (event.xexpose.count == 0) {
		win = gui_search_win(event.xexpose.window);
		if (win && win->win_draw_func)
		    win->win_draw_func(gui, win);
	    }
	    break;
	case GraphicsExpose:
	    win = winlist;
	    while (win) {
		if (win->win_draw_func)
		    win->win_draw_func(gui, win);
		win = win->next;
	    }
	    break;
	case ConfigureNotify:
	    win = gui_search_win(event.xconfigure.window);
	    if (win->win_attrib_func)
		win->win_attrib_func(gui, win, &(event.xconfigure),
				(xccore->xcin_mode & XCIN_KEEP_POSITION));
	    break;
	case ClientMessage:
	    if (event.xclient.format == 32 && 
		event.xclient.data.l[0] == gui->wm_del_win)
		gui_freewin(event.xclient.window);
	    break;
        }
    }
}

/*----------------------------------------------------------------------------

	GUI update window list functions.

----------------------------------------------------------------------------*/

Window
gui_request_init(IM_Context_t *imc, greq_win_t *gw)
{
    winlist_t *win;
    Window window;

    switch (gw->greq_data->type) {
    case GREQ_MENUSEL:
	win = gui_menusel_init(gui, imc->id, gw);
	window = win->window;
	break;
    default:
	window = (Window)0;
	break;
    }
    return window;
}

static void 
gui_greq_windraw(int n_gwin, greq_win_t *greqw, int state)
{
    winlist_t *w;
    int i;

    for (i=0; i<n_gwin; i++) {
	if ((w = gui_search_win(greqw[i].window))) {
	    gui_winmap_change(w, state);
	    if (state)
		w->win_draw_func(gui, w);
	}
    }
}

static void
update_gui_request(IC *ic, IC *icp)
{
    if ((gui->winchange & WIN_CHANGE_FOCUS) && icp)
	gui_greq_windraw(icp->imc->n_gwin, icp->imc->gwin, 0);

    if (ic->imc->n_gwin == 0)
	return;
    if (! ic || !(ic->imc->inp_state & IM_XIMFOCUS))
	gui_greq_windraw(ic->imc->n_gwin, ic->imc->gwin, 0);
    else
	gui_greq_windraw(ic->imc->n_gwin, ic->imc->gwin, 1);
}

static void
gui_bell(xmode_t xcin_mode)
{
    static int bell_pitch;

    if (! (xcin_mode & XCIN_DIFF_BEEP))
	XBell(gui->display, -20);
    else {
	XKeyboardControl kc;
	XKeyboardState ks;
	int new_pitch;

	if (bell_pitch == 0) {
	    XGetKeyboardControl(gui->display, &ks);
	    bell_pitch = ks.bell_pitch;
	}
	if ((gui->winchange & WIN_CHANGE_BELL))
	    new_pitch = 700;
	else if ((gui->winchange & WIN_CHANGE_BELL2))
	    new_pitch = 1400;
	else
	    new_pitch = bell_pitch;

	kc.bell_pitch = new_pitch;
	XChangeKeyboardControl(gui->display, KBBellPitch, &kc);
	XBell(gui->display, -20);
	kc.bell_pitch = bell_pitch;
	XChangeKeyboardControl(gui->display, KBBellPitch, &kc);
    }
}

void
gui_update_winlist(xccore_t *xccore)
{
    if (xccore->ic) {
	if (xccore->ic->ic_rec.input_style == XIMSTY_Root && ! gui->mainwin)
	    gui->mainwin = xcin_mainwin_init(gui, xccore);
	update_gui_request(xccore->ic, xccore->icp);
    }
    if (gui->overspot_win)
	gui->overspot_win->win_draw_func(gui, gui->overspot_win);
    /* Add by Firefly(firefly@firefly.idv.tw) */
    if (gui->onspot_win)
	gui->onspot_win->win_draw_func(gui, gui->onspot_win);
    if ((gui->winchange & WIN_CHANGE_IM)) {
	if (gui->mainwin)
	    gui->mainwin->win_draw_func(gui, gui->mainwin);
	if (gui->mainwin2)
	    gui->mainwin2->win_draw_func(gui, gui->mainwin2);
    }

    if ((gui->winchange & WIN_CHANGE_BELLALL))
	gui_bell(xccore->xcin_mode);
    gui->winchange = 0;
}


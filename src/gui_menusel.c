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
#include "gui.h"
#include "IC.h"

#define MENUSEL_NROW	5
#define FIELD_STEP	5
#define LINE_STEP	10

#define GC_idx		0	/* window fg_color, bg_color */
#define GCM_idx		1	/* for selection key spot mark & focused item */
#define GCGRID_idx	2	/* table grid line color */

void greq_unregister_internal(IM_Context_t *imc, int idx);

/*----------------------------------------------------------------------------

	IM GUI Request: Menu Selection Window drawing function.

----------------------------------------------------------------------------*/

static char *default_selkeys =  "1234567890";
#define DEFAULT_N_SEL		10

static void
get_elem(char *buf, int buf_size,
	 int n, wch_t *elements, ubyte_t *elem_group, char *selkeys)
{
    int i, idx=0;

    if (elem_group) {
	for (i=1; i<n; i++)
	    idx += ((int)elem_group[i]);
	nwchs_to_mbs(buf, elements+idx, elem_group[n], buf_size);
    }
    else
	strcpy(buf, (char *)elements[n-1].s);
}

static void
gui_menusel_draw(gui_t *gui, winlist_t *win)
{
    greq_win_t *gw = (greq_win_t *)(win->data);
    greq_menusel_t *info = (greq_menusel_t *)(gw->greq_data);
    int gcd_idx, gc_sel_idx;
    int i, j, h1, h2, len, max_len=0, max_len2, x, y, n_elem, n_sel;
    char buf[256], *selkeys;

    if ((win->winmode & WMODE_EXIT))
	return;
    XRaiseWindow(gui->display, win->window);
/*
 *  Draw headers of the item lists.
 */
    for (i=0, h1=info->head_item; 
	 i<win->c_height && h1<=info->n_item; i++, h1++) {
	len = wchs_to_mbs(buf, info->item[h1-1].title, 256);
	len = XmbTextEscapement(win->font->fontset, buf, len);
	if (max_len < len)
	    max_len = len;

	y = i*(win->font->ef_height+LINE_STEP) + 
	    win->font->ef_ascent + LINE_STEP/2;
	gcd_idx = (h1 == info->focus_item) ? GCM_idx : GC_idx;
	XmbDrawImageString(gui->display, win->window, win->font->fontset,
	 	win->wingc[gcd_idx], FIELD_STEP, y, buf, strlen(buf));
    }

/*
 *  Draw the grid lines.
 */
    for (i=1; i<win->c_height; i++) {
	y = i * (win->font->ef_height+LINE_STEP);
	XDrawLine(gui->display, win->window, 
		win->wingc[GCGRID_idx], 1, y, win->width, y);
    }
    x = max_len + FIELD_STEP*2;
    XDrawLine(gui->display, win->window, 
		win->wingc[GCGRID_idx], x, 1, x, win->height);

/*
 *  Draw the item list.
 */
    x += FIELD_STEP;
    if (info->selkeys) {
	selkeys = info->selkeys;
	n_sel = info->n_sel;
    }
    else {
	selkeys = default_selkeys;
	n_sel = (info->n_sel > DEFAULT_N_SEL) ? DEFAULT_N_SEL : info->n_sel;
    }

    for (i=0, h1=info->head_item; 
	 i<win->c_height && h1<=info->n_item; i++, h1++) {
	int xx = x;

	if (! info->item[h1-1].elements)
	    continue;
	y = i*(win->font->ef_height+LINE_STEP) + 
	    win->font->ef_ascent + LINE_STEP/2;
	n_elem = (info->item[h1-1].elem_group) ? 
		    info->item[h1-1].elem_group[0] : info->item[h1-1].n_elem;

	for (j=0, h2=info->item[h1-1].head_idx; 
	     j<n_sel && h2<=n_elem; j++, h2++) {
	    gc_sel_idx = GCM_idx;
	    gcd_idx    = GC_idx;
	    if (info->enable_focus_elem && 
		h1 == info->focus_item && h2 == info->focus_elem)
		gcd_idx = GCM_idx;

	    get_elem(buf, 256, h2, info->item[h1-1].elements, 
				info->item[h1-1].elem_group, selkeys);
	    len = strlen(buf);
	    max_len  = XmbTextEscapement(win->font->fontset, selkeys+j, 1);
	    max_len2 = XmbTextEscapement(win->font->fontset, buf, len);
	    if (xx+max_len+max_len2+FIELD_STEP > win->width)
		break;

	    XmbDrawImageString(gui->display, win->window, win->font->fontset,
				win->wingc[gc_sel_idx], xx, y, selkeys+j, 1);
	    xx += (max_len + FIELD_STEP);
	    XmbDrawImageString(gui->display, win->window, win->font->fontset,
				win->wingc[gcd_idx], xx, y, buf, len);
	    xx += (max_len2 + FIELD_STEP*2);
	}
	info->item[h1-1].n_sel_return = j;
    }
}

/*----------------------------------------------------------------------------

	IM GUI Request: Menu Selection Window initialization.

----------------------------------------------------------------------------*/

static void
gui_menusel_destroy(gui_t *gui, winlist_t *win)
{
    int i;
    IM_Context_t *imc;
    greq_win_t *gw;

    if (! (imc = imc_find(win->imid)))
	return;

    gw = (greq_win_t *)(win->data);
/*
 *  If greq_callback exist, then it means the user close the menusel 
 *  window by mouse, so we should do the unregister menually. Otherwise 
 *  the unregister has been performed by greq_unregister, and we don't
 *  need to unregister again.
 */
    if (gw->greq_callback) {
	gw->greq_callback(GREQCB_WIN_DESTORY, gw->reqid, &(imc->inpinfo), NULL);

	for (i=0; i<imc->n_gwin; i++) {
	    if (imc->gwin[i].reqid == gw->reqid)
		break;
	}
	greq_unregister_internal(imc, i);
    }
    for (i=0; i<win->n_gc; i++)
	XFreeGC(gui->display, win->wingc[i]);
    XFree(win->wingc);
    XDestroyWindow(gui->display, win->window);
}

static void
gui_menusel_attrib(gui_t *gui, winlist_t *win,
		   XConfigureEvent *event, int keep_flag)
{
    win->width = event->width;
    win->c_width = win->width / win->font->ef_width;
}

winlist_t *
gui_menusel_init(gui_t *gui, int imid, greq_win_t *gw)
{
    winlist_t *win;
    greq_menusel_t *info;
    XSetWindowAttributes win_attr;
    const int border=4;

    if (gw->greq_data->type != GREQ_MENUSEL)
	return NULL;
    info = &(gw->greq_data->menusel);

    win = gui_new_win();
    win->wtype = (xtype_t)WTYPE_GUIREQ;
    win->imid  = imid;

    win->font = gui_create_fontset(gui->font, 1);
    win->c_height = (info->n_item<=MENUSEL_NROW) ? info->n_item : MENUSEL_NROW;
    win->height   = win->c_height * (win->font->ef_height + LINE_STEP);
    if (gui->mainwin) {
	win->c_width = gui->mainwin->c_width;
	win->width   = gui->mainwin->width;
	win->pos_x   = gui->mainwin->pos_x - border;
	win->pos_y   = gui->mainwin->pos_y - win->height - 10*FIELD_STEP;
    }
    else {
	win->c_width = gui->mainwin2->c_width * 2;
	win->width   = gui->mainwin2->width * 2;
	win->pos_x   = gui->mainwin2->pos_x - border - gui->mainwin2->width/2;
	win->pos_y   = gui->mainwin2->pos_y - win->height - 10*FIELD_STEP;
    }
    if (win->pos_y <= 0)
	win->pos_y = 1;
    if (win->pos_x <=0)
	win->pos_x = 1;
    else if (win->pos_x+win->width > gui->display_width)
	win->pos_x = gui->display_width - win->width - border;

    win->data = (void *)gw;
    win->win_draw_func = gui_menusel_draw;
    win->win_attrib_func = gui_menusel_attrib;
    win->win_destroy_func = gui_menusel_destroy;
    win->window = XCreateSimpleWindow(gui->display, gui->root,
		win->pos_x, win->pos_y, win->width, win->height, border,
		gui->fg_color, gui->bg_color);
    XSelectInput(gui->display, win->window, (ExposureMask|StructureNotifyMask));
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);
/*  Setup GC  */
    win->n_gc = 3;
    win->wingc = xcin_malloc(sizeof(GC)*win->n_gc, 0);

    win->wingc[GC_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GC_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GC_idx], gui->bg_color);

    win->wingc[GCM_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCM_idx], gui->fg_color);
    XSetBackground(gui->display, win->wingc[GCM_idx], gui->mbg_color);

    win->wingc[GCGRID_idx] = XCreateGC(gui->display, win->window, 0, NULL);
    XSetForeground(gui->display, win->wingc[GCGRID_idx], gui->grid_color);
    XSetBackground(gui->display, win->wingc[GCGRID_idx], gui->bg_color);

    gui_winmap_change(win, 1);
    gui_menusel_draw(gui, win);
    return win;
}

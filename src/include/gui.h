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

#ifndef  _WINLIST_H
#define  _WINLIST_H

#include <X11/Xlib.h>
#include "xcintool.h"
typedef struct gui_s gui_t;

/*
 *  GUI fontset structer.
 */
typedef struct {
    char *basename;
    XFontSet fontset;
    int ef_width, ef_height, ef_ascent;
} font_t;

/*
 *  GUI Window configuration.
 */
#define WTYPE_MAIN		0
#define WTYPE_GUIREQ		1
#define WTYPE_OVERSPOT		2
#define WTYPE_ONSPOT		3

#define WMODE_MAP		1
#define	WMODE_EXIT		2

typedef struct winlist_s winlist_t;
struct winlist_s {
    Window window;
    xtype_t wtype;
    int imid;
    xmode_t winmode;

    int pos_x, pos_y;
    unsigned int width, height, c_width, c_height;
    font_t *font;
    unsigned short n_gc;
    GC *wingc;

    void *data;
    void (*win_draw_func)(gui_t *, winlist_t *);
    void (*win_attrib_func)(gui_t *, winlist_t *, XConfigureEvent *, int);
    void (*win_destroy_func)(gui_t *, winlist_t *);
    winlist_t *next;
};

/*
 *  GUI global configuration.
 */
#define WIN_CHANGE_IM           0x0001
#define WIN_CHANGE_FOCUS        0x0002
#define WIN_CHANGE_REDRAW       0x000f
#define WIN_CHANGE_BELL		0x0010
#define WIN_CHANGE_BELL2	0x0020
#define WIN_CHANGE_BELLALL	0x00f0

struct gui_s {
    Display *display;
    int display_width, display_height;
    int screen;

    Window root;
    Atom wm_del_win;
    Colormap colormap;
    unsigned long fg_color, bg_color;
    unsigned long mfg_color, mbg_color;
    unsigned long uline_color, grid_color;
    unsigned long white_color, black_color;

    char *font;
    char *overspot_font;
    XFontStruct *indexfont;
    char *inpn_english, *inpn_sbyte, *inpn_2bytes;
    winlist_t *mainwin, *mainwin2, *overspot_win;
    // Add by Firefly(firefly@firefly.idv.tw)
    winlist_t *onspot_win;
    xmode_t winchange;
};

extern winlist_t *gui_get_mainwin(void);
extern winlist_t *gui_search_win(Window window);
extern winlist_t *gui_new_win(void);
extern void gui_freewin(Window window);
extern void gui_winmap_change(winlist_t *win, int state);
extern font_t *gui_create_fontset(char *base_font, int verbose);
extern void gui_free_fontset(font_t *font);
extern int gui_check_window(Window window);
extern void gui_set_monitor(Window w, xmode_t flag, int icid);

#endif

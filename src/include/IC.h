#ifndef _IC_H
#define _IC_H
/******************************************************************
 
         Copyright 1994, 1995 by Sun Microsystems, Inc.
         Copyright 1993, 1994 by Hewlett-Packard Company
 
Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sun Microsystems, Inc.
and Hewlett-Packard not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.
Sun Microsystems, Inc. and Hewlett-Packard make no representations about
the suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
 
SUN MICROSYSTEMS INC. AND HEWLETT-PACKARD COMPANY DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
SUN MICROSYSTEMS, INC. AND HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
  Author: Hidetoshi Tajima(tajima@Eng.Sun.COM) Sun Microsystems, Inc.
 
******************************************************************/

#include <X11/Xlib.h>           /* for IMdkit.h/Xpointer */
#include "IMdkit.h"
#include "Xi18n.h"
#include "imodule.h"

typedef struct {
    XRectangle	area;		/* area */
    XPoint	spot_location;	/* spot location */
    char       *base_font;	/* base font of fontset */
    CARD32	foreground;	/* foreground */
    CARD32	background;	/* background */
    CARD32	line_space;	/* line spacing */
#ifdef XIM_COMPLETE
    XRectangle	area_needed;	/* area needed */
    Colormap	cmap;		/* colormap */
    Pixmap	bg_pixmap;	/* background pixmap */
    Cursor	cursor;		/* cursor */
#endif
} PreeditAttributes;

#ifdef XIM_COMPLETE
typedef struct {
    XRectangle	area;		/* area */
    XRectangle	area_needed;	/* area needed */
    Colormap	cmap;		/* colormap */
    CARD32	foreground;	/* foreground */
    CARD32	background;	/* background */
    Pixmap	bg_pixmap;	/* background pixmap */
    char	*base_font;	/* base font of fontset */
    CARD32	line_space;	/* line spacing */
    Cursor	cursor;		/* cursor */
} StatusAttributes;
#endif

#define CLIENT_SETIC_PRE_AREA		0x00000001
#define CLIENT_SETIC_PRE_SPOTLOC	0x00000002
#define CLIENT_SETIC_PRE_FONTSET	0x00000004
#define CLIENT_SETIC_PRE_FGCOLOR	0x00000008
#define CLIENT_SETIC_PRE_BGCOLOR	0x00000010

typedef struct {
    xmode_t		ic_value_set;	/* IC value set by the client */
    xmode_t		ic_value_update;/* IC value updated by the client */
    INT32		input_style;	/* input style */
    Window		client_win;	/* client window */
    Window		focus_win;	/* focus window */
    PreeditAttributes   pre_attr;	/* preedit attributes */
#ifdef XIM_COMPLETE
    StatusAttributes	sts_attr;	/* status attributes */
    char	       *resource_name;	/* resource name */
    char	       *resource_class; /* resource class */
#endif
} ic_rec_t;

/*
 *  Flags for inp_state of each IC.
 */
#define IM_CINPUT  	0x01
#define IM_2BYTES  	0x02
#define IM_XIMFOCUS	0x04
#define IM_2BFOCUS	0x08

typedef unsigned char  inp_state_t;

typedef struct greq_win_s {
    int reqid;
    Window window;
    greq_t *greq_data;
    int (*greq_callback)(int cmd, int reqid, 
			 inpinfo_t *inpinfo, greq_cb_t *info);
} greq_win_t;

typedef struct _IMC IM_Context_t;
struct _IMC {
    unsigned short	id;		/* id of this IMC */
    unsigned short	icid;		/* id of the current attached IC */
    ic_rec_t	       *ic_rec;		/* point to the current IC resource */

    inp_state_t		inp_state;      /* ic cinput state */
    inp_state_t		inp_num;        /* ic cinput num */
    inp_state_t		sinp_num;       /* ic cinput num (sinmd) */
    imodule_t	       *imodp;		/* current binding cinput module */
    imodule_t	       *s_imodp;	/* show keystroke cinput module */
    inpinfo_t		inpinfo;	/* inp info referenced by gui */
    unsigned int	skey_size;	/* sinmd_keystroke buf size. */
    wch_t 	       *sinmd_keystroke;/* for keystroke of a published cch. */
    unsigned int	cch_size;	/* cch buf size. */
    char	       *cch;		/* composed char for commit. */

    int			n_gwin;		/* IM GUI request window recorder. */
    greq_win_t		gwin[MAX_GREQ_CNT];
    Window		overspot_win;	/* OverTheSpot candidate window. */

    IM_Context_t       *next;
    IM_Context_t       *prev;
};

#define IC_NEWIC	0x01
#define IC_CONNECT	0x02
#define IC_FOCUS	0x04

#define XIMSTY_Root     (XIMPreeditNothing|XIMStatusNothing)
#define XIMSTY_OverSpot (XIMPreeditPosition|XIMStatusNothing)
#define XIMSTY_OffSpot  (XIMPreeditArea|XIMStatusArea)
#define XIMSTY_OnSpot   (XIMPreeditCallbacks|XIMStatusCallbacks)

typedef struct _IC IC;
struct _IC {
    CARD16		id;		/* ic id */
    CARD16		connect_id;	/* id of connected client */
    time_t		exec_time;	/* recent excution time */
    xmode_t		ic_state;	/* status of the IC */
    ic_rec_t		ic_rec;		/* the IC resource setting by client */
    IM_Context_t       *imc;		/* the IM Context */
    IC		       *next;
};


extern IM_Context_t *imc_find(int imid);
extern IC *ic_find(CARD16 icid);
extern void xim_close(IC *ic);
extern void call_xim_init(IC *ic, int reset_inpinfo);
extern void call_xim_end(IC *ic, int ic_delete, int reset_inpinfo);
extern void call_switch_in(IC *ic);
extern void call_switch_out(IC *ic);

#endif /* _IC_H */

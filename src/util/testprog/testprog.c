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

#ifdef HPUX
#  define _INCLUDE_XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xlocale.h>

#ifdef HAVE_WCHAR_H
#  include <wchar.h>
#endif
#ifdef HAVE_NL_LANGINFO
#  include <langinfo.h>
#endif

#ifdef HAVE_GETTEXT
#  include <libintl.h>
#  define _(STRING) gettext(STRING)
#  ifndef MSGLOCAT
#    define MSGLOCAT  "/tmp"
#  endif
#else
#  define _(STRING) STRING
#endif
#define N_(msg) (msg)

#define PROGNAME  "testprog"

typedef struct {
    XIMStyle style;
    char *description;
} im_style_t;

Display   *display;
int        screen;
Window     window;
GC	   gc;
char      *fontset_name;
XFontSet   fontset;
int        ef_height, ef_width, ef_ascent;
XRectangle win_rect;
Atom	   wm_del_win;

XIM        im;
XIC        ic;
char      *im_name;
char	  *im_style;
char	  *lc_ctype;
char	  *encoding;
XIMStyle   style;
wchar_t    input_buf[1024];
int        idx;
char       ic_focus;
im_style_t im_styles[] = {
    { XIMPreeditNothing  | XIMStatusNothing,	"Root" },
    { XIMPreeditPosition | XIMStatusNothing,	"OverTheSpot" },
    { XIMPreeditArea     | XIMStatusArea,	"OffTheSpot" },
    { XIMPreeditCallbacks| XIMStatusCallbacks,	"OnTheSpot" },
    { (XIMStyle)0,				NULL }};

void win_draw(void);

/*---------------------------------------------------------------------------

	If the wcs* functions is not available.

---------------------------------------------------------------------------*/

#ifndef HAVE_WCHAR_H

int
wcslen(wchar_t *s)
{
    wchar_t *cp = s;
    int len=0;

    while (*s) {
	len ++;
	s ++;
    }
    return len;
}
#endif

/*---------------------------------------------------------------------------

	XIM Functions.

---------------------------------------------------------------------------*/

static void
destroy_callback_func(XIM current_ic, XPointer client_data, XPointer call_data)
{
    ic = NULL;
    im = NULL;
    ic_focus = 0;

    printf(_("destroy_callback_func\n"));
}

void
im_callback(Display *display, XPointer client_data, XPointer call_data)
{
    XIMStyles *xim_styles = NULL;
    /* XIMValuesList im_values_list; */
    XIMCallback destroy;
    int i, j;

    XVaNestedList preedit_attr = NULL;
    XPoint spot;
    XRectangle local_win_rect;

/*
 *  Open connection to IM server.
 */
    if (! (im = XOpenIM(display, NULL, NULL, NULL))) {
        printf(_("Cannot open the connection to XIM server.\n"));
	exit(1);
    }

/*
 *  Get IM values.
 */
/*
    if (XGetIMValues(im, XNQueryIMValuesList, &im_values_list, NULL) || 
	! im_values_list.count_values) {
	printf("no IM values list available.\n");
        XCloseIM(im);
        exit(1);
    }
    else {
	for (i=0; i<im_values_list.count_values; i++)
	    printf("supported: %s\n", im_values_list.supported_values[i]);
    }
*/
    destroy.callback = (XIMProc)destroy_callback_func;
    destroy.client_data = NULL;
    XSetIMValues(im, XNDestroyCallback, &destroy, NULL);

/*
 *  Detect the input style supported by XIM server.
 */
    if (XGetIMValues(im, XNQueryInputStyle, &xim_styles, NULL) || 
	! xim_styles) {
        printf(_("input method doesn't support any style\n"));
        XCloseIM(im);
        exit(1);
    }
    else {
	for (i=0; i<xim_styles->count_styles; i++) {
	    for (j=0; im_styles[j].description!=NULL; j++) {
		if (im_styles[j].style == xim_styles->supported_styles[i]) {
		    printf(_("XIM server support input_style = %s\n"),
				im_styles[j].description);
		    break;
		}
	    }
	    if (im_styles[j].description==NULL)
		printf(_("XIM server support unknown input_style = %x\n"), 
			(unsigned)(xim_styles->supported_styles[i]));
	}
    }
/*
 *  Setting the XIM style used by me.
 */
    if (im_style) {
	for (j=0; im_styles[j].description!=NULL; j++) {
	    if (! strcmp(im_styles[j].description, im_style)) {
		style = im_styles[j].style;
		printf(_("Use input style: %s\n"), im_style);
		break;
	    }
	}
	if (im_styles[j].description==NULL) {
	    printf(_("The input style %s is not supported.\n"), im_style);
	    exit(1);
	}
    }
    else {		/* Root input_style as the default */
	for (j=0; im_styles[j].description!=NULL; j++) {
	    if (! strcmp(im_styles[j].description, "Root")) {
		style = im_styles[j].style;
		printf(_("Use input style: %s\n"), "Root");
		break;
	    }
	}
    }
    if (style == (XIMPreeditPosition | XIMStatusNothing)) {
	spot.x = 5;
	spot.y = 2*ef_height + 3*(ef_ascent+5);
	local_win_rect.x = 1;
	local_win_rect.y = 1;
	local_win_rect.width  = win_rect.width;
	local_win_rect.height = win_rect.height;
	preedit_attr = XVaCreateNestedList(0,
				XNArea, &local_win_rect,
				XNSpotLocation, &spot,
				XNFontSet, fontset,
				NULL);
    }

/*
 *  Create IC.
 */
    ic = XCreateIC(im, 
		   XNInputStyle, style,
		   XNClientWindow, window,
		   XNFocusWindow, window,
		   (preedit_attr) ? XNPreeditAttributes : NULL, preedit_attr,
		   NULL);
    if(ic == NULL) {
        printf(_("Cannot create XIC.\n"));
        exit(1);
    }
}

void
xim_init(void)
{
    char buf[1024];

    if (im_name)
	sprintf(buf, "@im=%s", im_name);
    else
	memset(buf, 0, 1024);
    if (XSetLocaleModifiers(buf) == NULL) {
	printf(_("XSetLocaleModifiers false.\n"));
	exit(0);
    }
    if (XRegisterIMInstantiateCallback(
		display, NULL, NULL, NULL, im_callback, NULL) != True) {
	printf(_("XRegisterIMInstantiateCallback false.\n"));
	exit(0);
    }
}

/*---------------------------------------------------------------------------

	Key press looking up functions.

---------------------------------------------------------------------------*/

void
send_spot_loc(void)
{
    XPoint spot;
    XVaNestedList preedit_attr;

    if (! ic)
	return;

    spot.x = XwcTextEscapement(fontset, input_buf, idx) + 5;
    spot.y = 2*ef_height + 3*(ef_ascent+5);
    preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
    XSetICValues(ic, XNPreeditAttributes, preedit_attr, NULL);
    XFree(preedit_attr);
}

int
check_chars(char *buf, int buf_len)
{
    wchar_t *cch;
    int i;

    cch = malloc(buf_len * (sizeof(wchar_t) + 1));
    if (mbstowcs(cch, buf, buf_len+1) <= 0) {
	printf(_("Unprinted char.\n"));
	return 0;
    }
    else {
	printf(_("Get char: %s\n"), buf);
	for (i=0; cch[i] && idx<1024-1; idx++, i++)
	    input_buf[idx] = cch[i];
	return 1;
    }
    free(cch);
}

int
check_keysym(KeySym keysym)
{
    if (keysym == XK_Escape) {
	if (ic)
	    XDestroyIC(ic);
	if (im)
            XCloseIM(im);
	exit(0);
    }
    else if (keysym == XK_BackSpace) {
	if (idx)
	    idx --;
	input_buf[idx] = (wchar_t)0;
    }
    else if (keysym == XK_Return)
	XBell(display, 30);
    else
	return 0;

    return 1;
}

#define BASE_BUFSIZE 50
void
xim_lookup_key(XKeyPressedEvent *event)
{
    static char *buf;
    static int buf_len, rlen;
    KeySym keysym;
    Status status;

    if (! buf) {
	buf_len = BASE_BUFSIZE;
	buf = malloc(buf_len);
    }

    memset(buf, 0, buf_len);
    rlen = XmbLookupString(ic, event, buf, buf_len, &keysym, &status);
    if ((status == XBufferOverflow)) {
	buf_len += BASE_BUFSIZE;
	buf = realloc(buf, buf_len);
        memset(buf, 0, buf_len);
	rlen = XmbLookupString(ic, event, buf, buf_len, &keysym, &status);
    }

    switch (status) {
    case XLookupNone:
	break;
    case XLookupKeySym:
	check_keysym(keysym);
	break;
    case XLookupChars:
	check_chars(buf, rlen);
	break;
    case XLookupBoth:
	if (! check_keysym(keysym))
	    check_chars(buf, rlen);
	break;
    }
    if (style == (XIMPreeditPosition|XIMStatusNothing))
	send_spot_loc();
}

void
x_lookup_key(XKeyEvent *kev)
{
    KeySym keysym;
    unsigned int keystate;
    char keystr[65];
    int keystr_len;

    keystate = kev->state;
    keystr_len = XLookupString(kev, keystr, 64, &keysym, NULL);
    keystr[(keystr_len >= 0) ? keystr_len : 0] = 0;

    if (! check_keysym(keysym) && keystr_len == 1)
	check_chars(keystr, keystr_len);
}


/*--------------------------------------------------------------------------

	GUI functions

--------------------------------------------------------------------------*/

void
set_wm_property(int x, int y, int width, int height, int argc, char **argv)
{
    char *win_name = PROGNAME, *icon_name = PROGNAME;
    XTextProperty windowName, iconName;
    XSizeHints size_hints;
    XWMHints wm_hints;
    XClassHint class_hints;

    if (! XStringListToTextProperty(&win_name, 1, &windowName) ||
        ! XStringListToTextProperty(&icon_name, 1, &iconName)) {
        printf(_("string text property error.\n"));
	exit(0);
    }
    win_rect.x = x;
    win_rect.y = y;
    win_rect.width = width;
    win_rect.height = height;

    size_hints.flags = USPosition | USSize;
    size_hints.x = x;
    size_hints.y = y;
    size_hints.width  = width;
    size_hints.height = height;

    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;

    class_hints.res_name  = PROGNAME;
    class_hints.res_class = PROGNAME;

    XSetWMProperties(display, window, &windowName,
                &iconName, argv, argc, &size_hints, &wm_hints, &class_hints);

    XFree(windowName.value);
    XFree(iconName.value);
}

void
create_fontset(void)
{
    int  i, fsize, charset_count, fontset_count=1;
    char lc_ctype_buf[1024];
    char **charset_list, *def_string;
    XFontStruct **font_structs;

    if (! fontset_name) {
	strncpy(lc_ctype_buf, lc_ctype, 1024);
	if (encoding[0] != '\0') {
	    strncat(lc_ctype_buf, ".", 1024);
	    strncat(lc_ctype_buf, encoding, 1024);
	}
	if (! strcmp(lc_ctype_buf, "zh_TW.big5"))
	    fontset_name = "-*-big5-0,-*-iso8859-1";
	else if (! strcmp(lc_ctype_buf, "zh_CN.gb2312"))
	    fontset_name = "-*-gb2312.1980-0,-*-iso8859-1";
	else {
	    printf(_("Error: you should specify the fontset.\n"));
	    exit(0);
	}
    }

/*
 *  Create fontset and extract font information.
 */
    fontset = XCreateFontSet(display, fontset_name,
                &charset_list, &charset_count, &def_string);
    if (charset_count || ! fontset) {
	printf(_("Error: cannot create fontset.\n"));
	exit(0);
    }
    fontset_count = XFontsOfFontSet(fontset, &font_structs, &charset_list);
    for (i=0; i<fontset_count; i++) {
        fsize = font_structs[i]->max_bounds.width / 2;
        if (fsize > ef_width)
            ef_width = fsize;
        fsize = font_structs[i]->ascent + font_structs[i]->descent;
        if (fsize > ef_height) {
            ef_height = fsize;
            ef_ascent = font_structs[i]->ascent;
        }
    }
}

void
gui_init(int argc, char **argv)
{
    if (! (display = XOpenDisplay(NULL))) {
	printf(_("XOpenDisplay false.\n"));
	exit(0);
    }
    screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 
		50, 50, 400, 200, 1, BlackPixel(display, screen), 
		WhitePixel(display, screen));
    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, screen));
    XSetBackground(display, gc, WhitePixel(display, screen));
    set_wm_property(50, 50, 400, 200, argc, argv);
    XSelectInput(display, window, 
	(ExposureMask | StructureNotifyMask | KeyPressMask | FocusChangeMask));
    wm_del_win = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_del_win, 1);

    create_fontset();
}

void
win_draw(void)
{
    char *s = _("This is the Xi18n test program.");
    char str[256];
    int x, y, len;

    XClearWindow(display, window);
    x = 5;
    y = 5 + ef_ascent;
    XmbDrawImageString(display, window, fontset, gc, x, y, s, strlen(s));

    sprintf(str, "(window=0x%x)", (unsigned)window); 
    y += (ef_height + ef_ascent + 5);
    XmbDrawImageString(display, window, fontset, gc, x, y, str, strlen(str));

    y += (ef_height + ef_ascent + 5);
    len = wcslen(input_buf);
    XwcDrawImageString(display, window, fontset, gc, x, y, input_buf, len);

    x += XwcTextEscapement(fontset, input_buf, len);
    y -= ef_ascent;
    XFillRectangle(display, window, gc, x, y, ef_width, ef_height);
}

void
win_attrib(XConfigureEvent *event)
{
    win_rect.x = event->x;
    win_rect.y = event->y;
    win_rect.width = event->width;
    win_rect.height = event->height;
}

/*--------------------------------------------------------------------------

	Main Program

--------------------------------------------------------------------------*/

void
locale_init(void)
{
    char *s;

    if ((s = setlocale(LC_CTYPE, "")) == NULL) {
	printf(_("setlocale LC_CTYPE false.\n"));
	exit(0);
    }
    lc_ctype = (char *)strdup(s);
    if ((s = strchr(lc_ctype, '.')) != NULL) {
	*s = '\0';
	s++;
    }
    else
	s = "";

#ifdef HAVE_NL_LANGINFO
    encoding = (char *)strdup(nl_langinfo(CODESET));
#else
    encoding = s;
#endif
    s = encoding;
    while (*s) {
	*s = (char)tolower(*s);
	s++;
    }

#ifdef HAVE_GETTEXT
    if (setlocale(LC_MESSAGES, "") == NULL) {
	printf(_("setlocale LC_MESSAGES false.\n"));
	exit(0);
    }
    else {
	textdomain(PROGNAME);
	bindtextdomain(PROGNAME, MSGLOCAT);
    }
#endif
    if (XSupportsLocale() != True) {
	printf(_("XSupportsLocale false.\n"));
	exit(0);
    }
}

void
cmd_options(int argc, char **argv)
{
    int i;

    printf(_("Use \"-h\" option to see help.\n"));

    for (i=1; i<argc; i++) {
	if (! strcmp(argv[i], "-h")) {
	    printf(_("Usage: testprog -fontset <fontset> -im <im_name> -pt <im_style>\n"));
	    printf(_("Example:\n"));
	    printf(_("  export LC_CTYPE=<locale name>.\n"));
	    printf(_("  export LC_MESSAGES=<locale name>.\n"));
	    printf(_("  export XMODIFIERS=\"@im=<im_name>\".\n"));
	    printf(_("  zh_TW.Big5 fontset: \"-*-big5-0,-*-iso8859-1\".\n"));
	    printf(_("  zh_CN.GB2312 fontset: \"-*-gb2312.1980-0,-*-iso8859-1\".\n"));
	    exit(0);
	}
	else if (! strcmp(argv[i], "-fontset"))
	    fontset_name = (char *)strdup(argv[++i]);
	else if (! strcmp(argv[i], "-im"))
	    im_name = (char *)strdup(argv[++i]);
	else if (! strcmp(argv[i], "-pt"))
	    im_style = (char *)strdup(argv[++i]);
    }
}

int
main(int argc, char **argv)
{
    XEvent event;

    locale_init();
    cmd_options(argc, argv);
    gui_init(argc, argv);
    xim_init();
    XMapWindow(display, window);

    while (1) {
	XNextEvent(display, &event);
	if (XFilterEvent(&event, None))
	    continue;

        switch (event.type) {
        case Expose:
	    printf(_("Event: Expose.\n"));
            win_draw();
            break;

        case GraphicsExpose:
	    printf(_("Event: GraphicsExpose.\n"));
            win_draw();
            break;

	case ConfigureNotify:
	    printf(_("Event: ConfigureNotify.\n"));
	    win_attrib(&(event.xconfigure));
	    break;

        case FocusIn:
            if (event.xany.window == window && ic) {
	        printf(_("Event: FocusIN.\n"));
                XSetICFocus(ic);
	        ic_focus = 1;
	    }
            break;

        case FocusOut:
            if (event.xany.window == window && ic) {
	        printf(_("Event: FocusOut.\n"));
                XUnsetICFocus(ic);
	        ic_focus = 0;
	    }
            break;

	case KeyPress:
	    printf(_("Event: KeyPress.\n"));
	    if (ic_focus)
	        xim_lookup_key((XKeyPressedEvent *)&event);
	    else
		x_lookup_key((XKeyEvent *)&event);
	    win_draw();
	    break;

	case ClientMessage:
	    printf(_("Event: ClientMessage.\n"));
	    if (event.xclient.format == 32 && 
		event.xclient.data.l[0] == wm_del_win) {
		if (ic)
		    XDestroyIC(ic);
		if (im)
	            XCloseIM(im);
		exit(0);
	    }
            break;
	}
    }
}

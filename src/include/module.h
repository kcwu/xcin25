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


                        **** ADDITIONAL NOTE ****

    This file, module.h, defines the interface to the xcin module system,
    which provides the facilities and definitions to write an Input Method
    module for xcin. Although this file, as a whole, is distributed under
    the terms of GPL (GNU General Public License) with xcin, everyone is 
    freely to adopt the contents there if he/she wants to write a new 
    XCIN MODULE for xcin. The adoption of any part of the contents below 
    will not affect the license terms of your program.

    Please note that the freedom of adoption of this file DOES NOT mean
    that you can statically link your module with ANY functions declared
    there if your module is a proprietary (not under GPL, LGPL, BSD, or other 
    free software license terms) program. In fact, this is not necessary to 
    do any links, since when xcin loads your module and execute it, the 
    functions your module calls will automatically linked by the system. So 
    please don't worry about any statically or dynamically linking problem. 
    Just compile your module to be a shared library (with suffix .so) and 
    everything will go.
*/      

#ifndef	_MODULE_H
#define	_MODULE_H

#include <X11/Xlib.h>
#include "xcintool.h"

#ifdef __cplusplus 
extern "C" { 
#endif 

#define  MODULE_VERSION         "20010918"


typedef struct {
    char objname[50];
    char encoding[50];
    char objloadname[100];
} objenc_t;

/*---------------------------------------------------------------------------

	Special GUI request structures.

---------------------------------------------------------------------------*/

/*---------------------------- GUI Menu Selection -------------------------*/
typedef struct {
    wch_t *title;			/* title of the item list. */
    wch_t *elements;			/* elements of the item list. */
    ubyte_t *elem_group;		/* grouping info. of the elements. */
    unsigned short n_elem;		/* size of "elements" array. */
    unsigned short head_idx;		/* head index of the item list. */
    unsigned short n_sel_return;	/* num of selection returned by xcin. */
} menu_item_t;

typedef struct {
    int type;				/* GUI Request type. */
    unsigned short n_item;		/* number of item lists. */
    unsigned short head_item;		/* head index of the item lists. */
    unsigned short n_sel;		/* number of selection keys. */
    char *selkeys;			/* the selection keys. */
    unsigned short focus_item;		/* index of the focused item. */
    unsigned short focus_elem;		/* index of the focused element. */
    ubyte_t enable_focus_elem;		/* use focus element facility or not. */
    menu_item_t *item;			/* the item lists. */
} greq_menusel_t;


/*------------------- Common Structure of GUI Request ----------------------*/
#define MAX_GREQ_CNT	5

enum greq_type {
    GREQ_MENUSEL
};

enum greq_callback_cmd {
    GREQCB_WIN_DESTORY
};

typedef void greq_cb_t;		/* reserve for future usage. */

typedef union {
    int			type;
    greq_menusel_t	menusel;
} greq_t;

/*--------------------------------------------------------------------------

	Input Method Information.

--------------------------------------------------------------------------*/

#ifndef True
#  define True  1
#  define False 0
#endif

/* Size of module ID in .tab file. */
#define MODULE_ID_SIZE		20

/* Max allowed # of selection keys */
#define SELECT_KEY_LENGTH       15

/* Returned value of keystroke() function. */
#define IMKEY_ABSORB    	0x00000000
#define IMKEY_COMMIT    	0x00000001
#define IMKEY_IGNORE    	0x00000002
#define IMKEY_BELL      	0x00000004
#define IMKEY_BELL2      	0x00000008
#define IMKEY_SHIFTESC  	0x00000010
#define IMKEY_SHIFTPHR		0x00000020
#define IMKEY_CTRLPHR		0x00000040
#define IMKEY_ALTPHR		0x00000080
#define IMKEY_FALLBACKPHR	0x00000100

/* Represent Page State of multi-char selection. */
#define MCCH_ONEPG		0
#define MCCH_BEGIN		1
#define MCCH_MIDDLE		2
#define MCCH_END		3

/* Options to control the GUI mode. */
#define GUIMOD_SELKEYSPOT  	0x0001
#define GUIMOD_SINMDLINE1  	0x0002
#define GUIMOD_LISTCHAR    	0x0004


/*  Structure for IM in IC.  */
typedef struct {
    int imid;				/* ID of current IM Context */
    void *iccf;				/* Internal data of IM for each IC */

    char *inp_cname;			/* IM Chinese name */
    char *inp_ename;			/* IM English name */
    ubyte_t area3_len;			/* Length of area 3 of window (n_char)*/
    ubyte_t zh_ascii;			/* The zh_ascii mode */
    unsigned short xcin_wlen;		/* xcin window length */
    xmode_t guimode;			/* GUI mode flag */

    ubyte_t keystroke_len;		/* # chars of keystroke */
    wch_t *s_keystroke;			/* keystroke printed in area 3 */
    wch_t *suggest_skeystroke;		/* keystroke printed in area 3 */

    ubyte_t n_selkey;			/* # of selection keys */
    wch_t *s_selkey;			/* the displayed select keys */
    unsigned short n_mcch;		/* # of chars with the same keystroke */
    wch_t *mcch;			/* multi-char list */
    ubyte_t *mcch_grouping;		/* grouping of mcch list */
    byte_t mcch_pgstate;		/* page state of multi-char */

    unsigned short n_lcch;		/* # of composed cch list. */
    wch_t *lcch;			/* composed cch list. */
    unsigned short edit_pos;		/* editing position in lcch list. */
    ubyte_t *lcch_grouping;		/* grouping of lcch list */

    wch_t cch_publish;			/* A published cch. */
    char *cch;				/* the string for commit. */
} inpinfo_t;

typedef struct {
    int imid;				/* ID of current Input Context */
    unsigned short xcin_wlen;		/* xcin window length */
    xmode_t guimode;			/* GUI mode flag */
    wch_t cch_publish;			/* A published cch. */
    wch_t *s_keystroke;			/* keystroke of cch_publish returned */
} simdinfo_t;

typedef struct {
    KeySym keysym;			/* X11 key code. */
    unsigned int keystate;		/* X11 key state/modifiers */
    char keystr[16];			/* X11 key name */
    int keystr_len;			/* key name length */
} keyinfo_t;


/*---------------------------------------------------------------------------

	General module definition

---------------------------------------------------------------------------*/

typedef struct module_s  module_t;
struct module_s {
    mod_header_t module_header;
    char **valid_objname;

    int conf_size;
    int (*init) (void *conf, char *objname, xcin_rc_t *xc);
	/* called when IM first loaded & initialized. */
    int (*xim_init) (void *conf, inpinfo_t *inpinfo);
	/* called when trigger key occures to switch IM. */
    unsigned int (*xim_end) (void *conf, inpinfo_t *inpinfo);
	/* called just before xim_init() to leave IM, not necessary */
    unsigned int (*keystroke) (void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo);
	/* called to input key code, and output chinese char. */
    int (*show_keystroke) (void *conf, simdinfo_t *simdinfo);
    	/* called to show the key stroke. */
    int (*terminate) (void *conf);
	/* called when xcin is going to exit. */
};


/*---------------------------------------------------------------------------

	IM Common Module.

---------------------------------------------------------------------------*/

#define  N_CCODE_RULE           5       /* # of rules of encoding */
#define  N_KEYCODE              50      /* # of valid keys 0-9, a-z, .... */
#define  N_ASCII_KEY            95      /* Num of printable ASCII char */

/* For encoding check. */
typedef struct {
    short n;
    ubyte_t begin[N_CCODE_RULE], end[N_CCODE_RULE];
} charcode_t;

typedef struct {
    unsigned int total_char;
    ubyte_t n_ch_encoding;
    charcode_t ccode[WCH_SIZE];
} ccode_info_t;

/* Define the qphrase classes. */
#define QPHR_TRIGGER		0
#define QPHR_SHIFT		1
#define QPHR_CTRL		2
#define QPHR_ALT		4
#define QPHR_FALLBACK		8

/* Module object & encoding names */
extern int get_objenc(char *objname, objenc_t *objenc);

/* Key <-> Code convertion */
extern int key2code(int key);
extern int code2key(int code);
extern int keys2codes(unsigned int *klist, int klist_size, char *keystroke);
extern void codes2keys(unsigned int *klist, int n_klist, char *keystroke, int keystroke_len);

/* CharCode system */
extern void ccode_init(charcode_t *ccp, int n);
extern void ccode_info(ccode_info_t *info);
extern int match_encoding(wch_t *wch);
extern int ccode_to_idx(wch_t *wch);
extern int ccode_to_char(int idx, unsigned char *mbs, int mbs_size);

/* Wide char for ASCII */
extern void fullascii_init(wch_t *list);
extern char *fullchar_keystring(int ch);
extern char *fullchar_keystroke(inpinfo_t *inpinfo, KeySym keysym);
extern char *fullchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo);
extern char *halfchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo);
extern KeySym keysym_ascii(int ch);

/* Quick key phrase: %trigger, %shift, %ctrl, %alt  */
extern void qphrase_init(xcin_rc_t *xrc, char *phrase_fn);
extern char *qphrase_str(int ch, int class);
extern char *get_qphrase_list(void);

/* GUI Request registration */
extern int greq_register(int imid, greq_t *greq, 
		int (*greq_callback)(int, int, inpinfo_t *, greq_cb_t *));
extern void greq_unregister(int imid, int reqid);
extern void greq_query(int imid, int *n_greq, int **reqid_list_return);

#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif

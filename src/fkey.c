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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "Ximd/IMdkit.h"
#include "xcintool.h"
#include "fkey.h"

/*----------------------------------------------------------------------------

	Key Data.

----------------------------------------------------------------------------*/

typedef struct {
    char *keyname;
    unsigned int modifier;
} fkey_mod_t;

static fkey_mod_t fkey_mod[] = {
	{"shift", 	ShiftMask},
	{"L-shift", 	ShiftMask},
	{"R-shift", 	ShiftMask},
	{"ctrl", 	ControlMask},
	{"L-ctrl", 	ControlMask},
	{"R-ctrl", 	ControlMask},
	{"alt", 	Mod1Mask},
	{"L-alt", 	Mod1Mask},
	{"R-alt", 	Mod1Mask},
	{NULL,		0}
};


typedef struct {
    char *keyname, shortname;
    KeySym keysym, skeysym;
} fkey_map_t;

static fkey_map_t fkey_map[] = {
        {"shift",       '\0',	XK_Shift_L,		XK_Shift_L},
        {"L-shift",     '\0',	XK_Shift_L,		XK_Shift_L},
        {"R-shift",     '\0',	XK_Shift_R,		XK_Shift_R},
        {"ctrl",        '\0',	XK_Control_L,		XK_Control_L},
        {"L-ctrl",      '\0',	XK_Control_L,		XK_Control_L},
        {"R-ctrl",      '\0',	XK_Control_R,		XK_Control_R},
        {"alt",         '\0',	XK_Alt_L,		XK_Alt_L},
        {"L-alt",       '\0',	XK_Alt_L,		XK_Alt_L},
        {"R-alt",       '\0',	XK_Alt_R,		XK_Alt_R},
	{"esc", 	'\0',	XK_Escape,		XK_Escape},
	{"f1", 		'\0',	XK_F1,			XK_F1},
	{"f2", 		'\0',	XK_F2,			XK_F2},
	{"f3", 		'\0',	XK_F3,			XK_F3},
	{"f4", 		'\0',	XK_F4,			XK_F4},
	{"f5", 		'\0',	XK_F5,			XK_F5},
	{"f6", 		'\0',	XK_F6,			XK_F6},
	{"f7", 		'\0',	XK_F7,			XK_F7},
	{"f8", 		'\0',	XK_F8,			XK_F8},
	{"f9", 		'\0',	XK_F9,			XK_F9},
	{"f10", 	'\0',	XK_F10,			XK_F10},
	{"f11", 	'\0',	XK_F11,			XK_F11},
	{"f12", 	'\0',	XK_F12,			XK_F12},
	{"grave", 	'`',	XK_grave,		XK_asciitilde},
	{"1", 		'\0',	XK_1,			XK_exclam},
	{"2", 		'\0',	XK_2,			XK_at},
	{"3", 		'\0',	XK_3,			XK_numbersign},
	{"4", 		'\0',	XK_4,			XK_dollar},
	{"5", 		'\0',	XK_5,			XK_percent},
	{"6", 		'\0',	XK_6,			XK_asciicircum},
	{"7", 		'\0',	XK_7,			XK_ampersand},
	{"8", 		'\0',	XK_8,			XK_asterisk},
	{"9", 		'\0',	XK_9,			XK_parenleft},
	{"0", 		'\0',	XK_0,			XK_parenright},
	{"-", 		'\0',	XK_minus,		XK_underscore},
	{"=", 		'\0',	XK_equal,		XK_plus},
	{"tab", 	'\0',	XK_Tab,			XK_Tab},
	{"space", 	' ',	XK_space,		XK_space},
	{"backspace", 	'\0',	XK_BackSpace,		XK_BackSpace},
	{"a", 		'\0',	XK_a,			XK_A},
	{"b", 		'\0',	XK_b,			XK_B},
	{"c", 		'\0',	XK_c,			XK_C},
	{"d", 		'\0',	XK_d,			XK_D},
	{"e", 		'\0',	XK_e,			XK_E},
	{"f", 		'\0',	XK_f,			XK_F},
	{"g", 		'\0',	XK_g,			XK_G},
	{"h", 		'\0',	XK_h,			XK_H},
	{"i", 		'\0',	XK_i,			XK_I},
	{"j", 		'\0',	XK_j,			XK_J},
	{"k", 		'\0',	XK_k,			XK_K},
	{"l", 		'\0',	XK_l,			XK_L},
	{"m", 		'\0',	XK_m,			XK_M},
	{"n", 		'\0',	XK_n,			XK_N},
	{"o", 		'\0',	XK_o,			XK_O},
	{"p", 		'\0',	XK_p,			XK_P},
	{"q", 		'\0',	XK_q,			XK_Q},
	{"r", 		'\0',	XK_r,			XK_R},
	{"s", 		'\0',	XK_s,			XK_S},
	{"t", 		'\0',	XK_t,			XK_T},
	{"u", 		'\0',	XK_u,			XK_U},
	{"v", 		'\0',	XK_v,			XK_V},
	{"w", 		'\0',	XK_w,			XK_W},
	{"x", 		'\0',	XK_x,			XK_X},
	{"y", 		'\0',	XK_y,			XK_Y},
	{"z", 		'\0',	XK_z,			XK_Z},
	{"[", 		'\0',	XK_bracketleft,		XK_braceleft},
	{"]", 		'\0',	XK_bracketright,	XK_braceright},
	{"semicolon", 	';',	XK_semicolon,		XK_colon},
	{"apostrophe", 	'\'',	XK_apostrophe,		XK_quotedbl},
	{"comma", 	',',	XK_comma,		XK_less},
	{"period", 	'.',	XK_period,		XK_greater},
	{"slash", 	'/',	XK_slash,		XK_question},
	{"backslash", 	'\\',	XK_backslash,		XK_bar},
	{NULL,		'\0',	0,			0}
};


/*----------------------------------------------------------------------------

	Functional Keys Setting and Analyzing.

----------------------------------------------------------------------------*/

typedef struct {
    char *name;
    KeySym keysym;
    unsigned int modifier;
    byte_t trigger;
} fkey_t;

static fkey_t funckey[] = {
	{"FKEY_ZHEN", XK_space, ControlMask, 0},
	{"FKEY_2BSB", XK_space, ShiftMask, 0},
	{"FKEY_CIRIM", XK_Shift_L, ControlMask, 0},
	{"FKEY_CIRRIM", XK_Control_L, ShiftMask, 0},
	{"FKEY_CHREP", XK_r, ControlMask|Mod1Mask, 0},
	{"FKEY_SIMD", XK_i, ControlMask|Mod1Mask, 0},
	{"FKEY_IMFOCUS", XK_f, ControlMask|Mod1Mask, 0},
	{"FKEY_IMN", 0, ControlMask|Mod1Mask, 0},
	{"FKEY_QPHRASE", 0, ShiftMask|Mod1Mask, 0},
	{NULL, 0, 0}
};

static int n_types;
static byte_t changed;

static unsigned int
search_modifier(char *value, int *keypos)
{
    int i;

    for (i=0; fkey_mod[i].keyname && 
	      strcasecmp(fkey_mod[i].keyname, value); i++);
    if (keypos)
        *keypos = i;
    return (! fkey_mod[i].keyname) ? 0 : fkey_mod[i].modifier;
}

static KeySym
search_keysym(char *value, int *keypos, int shift)
{
    int i;

    for (i=0; fkey_map[i].keyname && 
	      strcasecmp(fkey_map[i].keyname, value); i++);
    if (! fkey_map[i].keyname && value[1] == '\0') {
	for (i=0; fkey_map[i].keyname &&
		value[0] != fkey_map[i].shortname; i++);
    }
    if (keypos)
        *keypos = i;

    if (! shift)
        return (! fkey_map[i].keyname) ? 0 : fkey_map[i].keysym;
    else
        return (! fkey_map[i].keyname) ? 0 : fkey_map[i].skeysym;
}

void
set_funckey(int key_type, char *value)
{
    char keys1[80], keys2[80], keys3[80], *keys[3];
    char *s = value, tmp[80], has_keysym=0; 
    int i, n_keys, keypos[3]={-1, -1, -1};
    unsigned int mod, modifier=0;
    byte_t shift=0;
    KeySym keysym=0;

    keys[0] = keys1;
    keys[1] = keys2;
    keys[2] = keys3;

    if (n_types == 0)
	n_types = sizeof(funckey)/sizeof(fkey_t) - 1;
    if (key_type >= n_types || key_type < 0)
	perr(XCINMSG_IERROR, 
	     N_("set_funckey: %d: unknown key_type\n"), key_type);

    for (n_keys=0; n_keys<3 && get_word(&s, keys[n_keys], 80, NULL); n_keys++);
    if (get_word(&s, tmp, 80, NULL))
	perr(XCINMSG_WARNING, N_("%s: %s: too many keys, ignore.\n"),
		funckey[key_type].name, tmp);

    if (key_type != FKEY_IMN && key_type != FKEY_QPHRASE) {
	/* This case we need both modifiers and keysym,
	   otherwise we only need modifiers. */
	n_keys --;
	has_keysym = 1;
    }
    for (i=0; i<n_keys; i++) {
	if ((mod = search_modifier(keys[i], keypos+i)) == 0)
	    perr(XCINMSG_ERROR, N_("%s: %s: not valid modifier key.\n"),
		funckey[key_type].name, keys[i]);
	if (keypos[i]>=0 && keypos[i]<3)
	    shift = 1;
	modifier |= mod;
    }
    if (has_keysym && 
	(keysym = search_keysym(keys[n_keys], keypos+n_keys, shift)) == 0)
	perr(XCINMSG_ERROR, N_("%s: %s: not valid key name.\n"),
		funckey[key_type].name, keys[n_keys]);

    if (keypos[1] != -1) {		/* At least 2 keys defined */
        if (keypos[0] == keypos[1])
	    perr(XCINMSG_ERROR, N_("%s: %s: duplicate keys.\n"),
		funckey[key_type].name, keys[0]);
	if (keypos[2] != -1) {		/* 3 keys defined */
	    if (keypos[0] == keypos[2] || keypos[1] == keypos[2])
		perr(XCINMSG_ERROR, N_("%s: %s: duplicate keys.\n"),
			funckey[key_type].name, keys[2]);
	}
    }

    funckey[key_type].keysym = keysym;
    funckey[key_type].modifier = modifier;
    changed = 1;
}

void
check_funckey(void)
{
    int i, j;

    if (! changed)
	return;

    if (n_types == 0)
	n_types = sizeof(funckey)/sizeof(fkey_t) - 1;
    for (i=0; i<n_types; i++) {
	for (j=i+1; j<n_types; j++) {
	    if (funckey[i].keysym == funckey[j].keysym &&
		funckey[i].modifier == funckey[j].modifier)
		perr(XCINMSG_ERROR, 
			N_("%s and %s have the same key-definition.\n"),
			funckey[i].name, funckey[j].name);
	}
    }
}


/*----------------------------------------------------------------------------

	Trigger Keys Setting and Analyzing.

----------------------------------------------------------------------------*/

typedef struct {
    int major_code, minor_code;
} tkeylist_t;

static XIMTriggerKey *tkey;
static tkeylist_t *tkeylist;
static int n_keys;

static void
impose_trigger_key(int func_idx, int t_idx)
{
    tkey[t_idx].keysym = funckey[func_idx].keysym;
    tkey[t_idx].modifier = tkey[t_idx].modifier_mask = 
		funckey[func_idx].modifier;
    tkeylist[t_idx].major_code = func_idx;
    tkeylist[t_idx].minor_code = 0;
    funckey[func_idx].trigger = 1;
}

void
make_trigger_keys(XIMTriggerKeys *trigger_keys)
{
    int trigger_keys_size=20;
    char *qp, str[2];
    byte_t shift;
    numlist_t *np;

    if (n_keys) {
	trigger_keys->count_keys = n_keys;
	trigger_keys->keylist = tkey;
	return;
    }

    if (n_types == 0)
	n_types = sizeof(funckey)/sizeof(fkey_t) - 1;
    tkey = malloc(sizeof(XIMTriggerKey) * trigger_keys_size);
    tkeylist = malloc(sizeof(tkeylist_t) * trigger_keys_size);

    impose_trigger_key(FKEY_ZHEN, 0);
    impose_trigger_key(FKEY_2BSB, 1);
    impose_trigger_key(FKEY_CIRIM, 2);
    impose_trigger_key(FKEY_CIRRIM, 3);
    n_keys += 4;

    np = get_cinput_numlist();
    str[1] = '\0';
    while (*np != (numlist_t)-1) {
	if (n_keys >= trigger_keys_size-1) {
	    trigger_keys_size += 20;
	    tkey = realloc(tkey, sizeof(XIMTriggerKey) * trigger_keys_size);
	    tkeylist = realloc(tkeylist, sizeof(tkeylist_t) * trigger_keys_size);
	}

	switch (*np) {
	    case 10: str[0] = '-'; break;
	    case 11: str[0] = '='; break;
	    default: str[0] = (char)(*np + '0'); break;
	}
	shift = ((funckey[FKEY_IMN].modifier & ShiftMask)) ? 1 : 0;
	tkey[n_keys].keysym = search_keysym(str, NULL, shift); 
	tkey[n_keys].modifier = tkey[n_keys].modifier_mask = 
		funckey[FKEY_IMN].modifier;
	tkeylist[n_keys].major_code = FKEY_IMN;
	tkeylist[n_keys].minor_code = (int)(*np);
	np ++;
	n_keys ++;
    }
    tkey[n_keys].keysym = tkey[n_keys].modifier = 
	tkey[n_keys].modifier_mask = 0L;
    funckey[FKEY_IMN].trigger = 1;

    qp = get_qphrase_list();
    while (*qp) {
	if (n_keys >= trigger_keys_size-1) {
	    trigger_keys_size += 20;
	    tkey = realloc(tkey, sizeof(XIMTriggerKey) * trigger_keys_size);
	    tkeylist = realloc(tkeylist, sizeof(tkeylist_t) * trigger_keys_size);
	}
	str[0] = *qp;
	shift = ((funckey[FKEY_QPHRASE].modifier & ShiftMask)) ? 1 : 0;
	tkey[n_keys].keysym = search_keysym(str, NULL, shift); 
	tkey[n_keys].modifier = tkey[n_keys].modifier_mask = 
		funckey[FKEY_QPHRASE].modifier;
	tkeylist[n_keys].major_code = FKEY_QPHRASE;
	tkeylist[n_keys].minor_code = (int)(*qp);
	qp ++;
	n_keys ++;
    }
    funckey[FKEY_QPHRASE].trigger = 1;

    trigger_keys->count_keys = n_keys;
    trigger_keys->keylist = tkey;
}

void
get_trigger_key(int imcode, int *major_code, int *minor_code)
{
    *major_code = tkeylist[imcode].major_code;
    *minor_code = tkeylist[imcode].minor_code;
}

int
search_funckey(KeySym keysym, unsigned int modifier,
		   int *major_code, int *minor_code)
{
    int i;

    for (i=0; i<n_keys; i++) {
	if ((tkey[i].modifier & modifier) == tkey[i].modifier &&
	    tkey[i].keysym == keysym) {
	    *major_code = tkeylist[i].major_code;
	    *minor_code = tkeylist[i].minor_code;
	    return 1;
	}
    }
    for (i=0; i<n_types; i++) {
	if (! funckey[i].trigger && 
	    (funckey[i].modifier & modifier) == funckey[i].modifier) {
	    if (funckey[i].keysym == 0 || keysym == funckey[i].keysym) {
		*major_code = i;
		*minor_code = 0;
		return 1;
	    }
	}
    }
    return 0;
}

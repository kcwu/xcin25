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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "module.h"


typedef struct xkeymap_s {
    KeySym xkey;
    wch_t wch;
} xkeymap_t; 

static xkeymap_t fullchar[] = {		/* sorted by ascii code */
	{XK_space,		{{'\0'}}},	/*     */
	{XK_exclam,		{{'\0'}}},	/*  !  */
	{XK_quotedbl,		{{'\0'}}},	/*  "  */
	{XK_numbersign,		{{'\0'}}},	/*  #  */
	{XK_dollar,		{{'\0'}}},	/*  $  */
	{XK_percent,		{{'\0'}}},	/*  %  */
	{XK_ampersand,		{{'\0'}}},	/*  &  */
	{XK_apostrophe,		{{'\0'}}},	/*  '  */
	{XK_parenleft,		{{'\0'}}},	/*  (  */
	{XK_parenright,		{{'\0'}}},	/*  )  */
	{XK_asterisk,		{{'\0'}}},	/*  *  */
	{XK_plus,		{{'\0'}}},	/*  +  */
	{XK_comma,		{{'\0'}}},	/*  ,  */
	{XK_minus,		{{'\0'}}},	/*  -  */
	{XK_period,		{{'\0'}}},	/*  .  */
	{XK_slash,		{{'\0'}}},	/*  /  (undefined) */
	{XK_0,			{{'\0'}}},	/*  0  */
	{XK_1,			{{'\0'}}},	/*  1  */
	{XK_2,			{{'\0'}}},	/*  2  */
	{XK_3,			{{'\0'}}},	/*  3  */
	{XK_4,			{{'\0'}}},	/*  4  */
	{XK_5,			{{'\0'}}},	/*  5  */
	{XK_6,			{{'\0'}}},	/*  6  */
	{XK_7,			{{'\0'}}},	/*  7  */
	{XK_8,			{{'\0'}}},	/*  8  */
	{XK_9,			{{'\0'}}},	/*  9  */
	{XK_colon,		{{'\0'}}},	/*  :  */
	{XK_semicolon,		{{'\0'}}},	/*  ;  */
	{XK_less,		{{'\0'}}},	/*  <  */
	{XK_equal,		{{'\0'}}},	/*  =  */
	{XK_greater,		{{'\0'}}},	/*  >  */
	{XK_question,		{{'\0'}}},	/*  ?  */
	{XK_at,			{{'\0'}}},	/*  @  */
	{XK_A,			{{'\0'}}},	/*  A  */
	{XK_B,			{{'\0'}}},	/*  B  */
	{XK_C,			{{'\0'}}},	/*  C  */
	{XK_D,			{{'\0'}}},	/*  D  */
	{XK_E,			{{'\0'}}},	/*  E  */
	{XK_F,			{{'\0'}}},	/*  F  */
	{XK_G,			{{'\0'}}},	/*  G  */
	{XK_H,			{{'\0'}}},	/*  H  */
	{XK_I,			{{'\0'}}},	/*  I  */
	{XK_J,			{{'\0'}}},	/*  J  */
	{XK_K,			{{'\0'}}},	/*  K  */
	{XK_L,			{{'\0'}}},	/*  L  */
	{XK_M,			{{'\0'}}},	/*  M  */
	{XK_N,			{{'\0'}}},	/*  N  */
	{XK_O,			{{'\0'}}},	/*  O  */
	{XK_P,			{{'\0'}}},	/*  P  */
	{XK_Q,			{{'\0'}}},	/*  Q  */
	{XK_R,			{{'\0'}}},	/*  R  */
	{XK_S,			{{'\0'}}},	/*  S  */
	{XK_T,			{{'\0'}}},	/*  T  */
	{XK_U,			{{'\0'}}},	/*  U  */
	{XK_V,			{{'\0'}}},	/*  V  */
	{XK_W,			{{'\0'}}},	/*  W  */
	{XK_X,			{{'\0'}}},	/*  X  */
	{XK_Y,			{{'\0'}}},	/*  Y  */
	{XK_Z,			{{'\0'}}},	/*  Z  */
	{XK_bracketleft,	{{'\0'}}},	/*  [  */
	{XK_backslash,		{{'\0'}}},	/*  \  (undefined) */
	{XK_bracketright,	{{'\0'}}},	/*  ]  */
	{XK_asciicircum,	{{'\0'}}},	/*  ^  */
	{XK_underscore,		{{'\0'}}},	/*  _  (undefined) */
	{XK_grave,		{{'\0'}}},	/*  `  */
	{XK_a,			{{'\0'}}},	/*  a  */
	{XK_b,			{{'\0'}}},	/*  b  */
	{XK_c,			{{'\0'}}},	/*  c  */
	{XK_d,			{{'\0'}}},	/*  d  */
	{XK_e,			{{'\0'}}},	/*  e  */
	{XK_f,			{{'\0'}}},	/*  f  */
	{XK_g,			{{'\0'}}},	/*  g  */
	{XK_h,			{{'\0'}}},	/*  h  */
	{XK_i,			{{'\0'}}},	/*  i  */
	{XK_j,			{{'\0'}}},	/*  j  */
	{XK_k,			{{'\0'}}},	/*  k  */
	{XK_l,			{{'\0'}}},	/*  l  */
	{XK_m,			{{'\0'}}},	/*  m  */
	{XK_n,			{{'\0'}}},	/*  n  */
	{XK_o,			{{'\0'}}},	/*  o  */
	{XK_p,			{{'\0'}}},	/*  p  */
	{XK_q,			{{'\0'}}},	/*  q  */
	{XK_r,			{{'\0'}}},	/*  r  */
	{XK_s,			{{'\0'}}},	/*  s  */
	{XK_t,			{{'\0'}}},	/*  t  */
	{XK_u,			{{'\0'}}},	/*  u  */
	{XK_v,			{{'\0'}}},	/*  v  */
	{XK_w,			{{'\0'}}},	/*  w  */
	{XK_x,			{{'\0'}}},	/*  x  */
	{XK_y,			{{'\0'}}},	/*  y  */
	{XK_z,			{{'\0'}}},	/*  z  */
	{XK_braceleft,		{{'\0'}}},	/*  {  */
	{XK_bar,		{{'\0'}}},	/*  |  */
	{XK_braceright,		{{'\0'}}},	/*  }  */
	{XK_asciitilde,		{{'\0'}}},	/*  ~  */
	{0L,			{{'\0'}}}
};

static char cch[WCH_SIZE+1];

void
fullascii_init(wch_t *list)
{
    xkeymap_t *fc=fullchar;
    int i=0;

    for (; fc->xkey; fc++)
	fc->wch.wch = list[i++].wch;
}

char *
fullchar_keystroke(inpinfo_t *inpinfo, KeySym keysym)
{
    xkeymap_t *fc=fullchar;

    while (fc->xkey != 0) {
	if (keysym == fc->xkey) {
	    strncpy(cch, (char *)fc->wch.s, WCH_SIZE);
	    cch[WCH_SIZE] = '\0';
	    return cch;
	}
	fc ++;
    }
    return NULL;
}

char *
fullchar_keystring(int ch)
{
    int i;

    if ((i = ch - (int)' ') >= 95 || i < 0)
	return NULL;
    strncpy(cch, (char *)fullchar[i].wch.s, WCH_SIZE);
    cch[WCH_SIZE] = '\0';
    return cch;
}

char *
fullchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo)
{
    int i;

    if (keyinfo->keystr_len != 1)
	return fullchar_keystroke(inpinfo, keyinfo->keysym);

    if ((i = (int)(keyinfo->keystr[0]-' ')) >= 95 || i < 0)
	return NULL;

    if (mode) {
	if ((keyinfo->keystate & LockMask) && (keyinfo->keystate & ShiftMask))
	    i = toupper(keyinfo->keystr[0]) - ' ';
	else
	    i = tolower(keyinfo->keystr[0]) - ' ';
    }
    strncpy(cch, (char *)fullchar[i].wch.s, WCH_SIZE);
    cch[WCH_SIZE] = '\0';
    return cch;
}

char *
halfchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo)
{
    int i;

    if (keyinfo->keystr_len != 1)
	return NULL;

    if ((i = (int)(keyinfo->keystr[0]-' ')) >= 95 || i < 0)
	return NULL;

    if (mode) {
	if ((keyinfo->keystate & LockMask) && (keyinfo->keystate & ShiftMask))
	    i = toupper(keyinfo->keystr[0]);
	else
	    i = tolower(keyinfo->keystr[0]);

        cch[0] = (char)i;
        cch[1] = '\0';
        return cch;
    }
    else
	return NULL;
}

KeySym
keysym_ascii(int ch)
{
    int i;

    if ((i = (int)(ch - ' ')) >= 95 || i < 0)
	return (KeySym)0;
    else
	return fullchar[i].xkey;
}

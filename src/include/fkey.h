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


#ifndef  _FKEY_H
#define  _FKEY_H

#include <X11/Xlib.h>
#include "imodule.h"

/*
 *  Functional key symbols.
 */
enum fkey {
    FKEY_ZHEN=0,		/* Chinese/English switch */
    FKEY_2BSB,			/* 2-bytes/single-byte switch */
    FKEY_CIRIM,			/* Circular change IM */
    FKEY_CIRRIM,		/* Reverse circular change IM */
    FKEY_CHREP,			/* Repeat the previous char */
    FKEY_SIMD,			/* Change simd display */
    FKEY_IMFOCUS,		/* IM-focus switch */
    FKEY_IMN,			/* IM switch 0-9,-,= mod */
    FKEY_QPHRASE		/* Quick phrase mod */
};

extern void set_funckey(int key_type, char *value);
extern void check_funckey(void);
extern void make_trigger_keys(XIMTriggerKeys *trigger_keys);
extern void get_trigger_key(int imcode, int *major_code, int *minor_code);
extern int search_funckey(KeySym keysym, unsigned int modifier,
			int *major_code, int *minor_code);

#endif

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

#ifndef _CIN2TAB_H
#define _CIN2TAB_H

#include "xcintool.h"

typedef struct {
    xcin_rc_t xrc;
    char *fname;
    char *fname_cin;
    char *fname_tab;
    char *sysfn;
    FILE *fr, *fw;
    int lineno;
} cintab_t;

extern int cmd_arg(char *cmd, int cmdlen, ...);
extern int read_hexwch(unsigned char *wch_str, char *arg);
extern void load_systab(char *sysfn, xcin_rc_t *xrc);

#endif

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

#ifndef	_IMODULE_H
#define	_IMODULE_H

#include "module.h"

#define MAX_INP_ENTRY	12	/* Number of input method capability */

/*---------------------------------------------------------------------------

	General module definition (Internal).

---------------------------------------------------------------------------*/

typedef struct imodule_s  imodule_t;
struct imodule_s {
    char *name;
    char *version;
    char *comments;
    char *objname;
    enum mtype module_type;

    void *conf;
    int (*init) (void *conf, char *objname, xcin_rc_t *xc);
    int (*xim_init) (void *conf, inpinfo_t *inpinfo);
    unsigned int (*xim_end) (void *conf, inpinfo_t *inpinfo);
    int (*switch_in) (void *conf, inpinfo_t *inpinfo);
    int (*switch_out) (void *conf, inpinfo_t *inpinfo);
    unsigned int (*keystroke) (void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo);
    int (*show_keystroke) (void *conf, simdinfo_t *simdinfo);
    int (*terminate) (void *conf);
};

typedef struct {
    char *modname;
    char *objname;
    imodule_t *inpmod;
    int with_enc;
} cinput_t;

typedef char numlist_t;

extern imodule_t *load_IM(char *modname, char *objenc, xcin_rc_t *xrc);
extern cinput_t *get_cinput(int idx);
extern cinput_t *get_cinput_next(int idx, int *idx_ret);
extern cinput_t *get_cinput_prev(int idx, int *idx_ret);
extern cinput_t *set_cinput(int idx,char *modname,char *objenc,char *encoding);
extern cinput_t *search_cinput(char *objname, char *encoding, int *idx_ret);
extern void free_cinput(cinput_t *cp);
extern numlist_t *get_cinput_numlist(void);
extern void cinput_terminate(void);

#endif

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

#ifndef	_IMODULE_H
#define	_IMODULE_H

#include "module.h"

#define MAX_IM_ENTRY	12	/* Number of input method capability */

/*---------------------------------------------------------------------------

	General module definition (Internal).

---------------------------------------------------------------------------*/

typedef struct imodule_s  imodule_t;
struct imodule_s {
    void *modp;
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

typedef char numlist_t;

extern int IM_register(int idx, char *modname, char *objname, char *encoding);
extern int IM_check_registered(int idx);
extern imodule_t *IM_get(int idx, xcin_rc_t *xrc);
extern imodule_t *IM_get_next(int idx, int *idx_ret, xcin_rc_t *xrc);
extern imodule_t *IM_get_prev(int idx, int *idx_ret, xcin_rc_t *xrc);
extern numlist_t *IM_get_numlist(void);
extern imodule_t *IM_search(char *objname, char *encoding, int *idx_ret, xcin_rc_t *xrc);
extern void IM_free(int idx);
extern void IM_free_all(void);
extern int get_objenc(char *objname, objenc_t *objenc);

#endif

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

#include "xcintool.h"
#include "imodule.h"

typedef struct {
    char *modname;
    char *objname;
    imodule_t *imodp;
    int with_enc;
} im_t;

static im_t IM[MAX_IM_ENTRY];

/*---------------------------------------------------------------------------

        Load Module & Module Init.

---------------------------------------------------------------------------*/

static int
check_module(module_t *modp, char *objenc)
{
    char *str=NULL, **objn, objname[64];
    int check_ok=0;

    if (modp == NULL)
	return False;
/*
 *  Check for necessary symbols.
 */
    if (! modp->conf_size)
	str = "conf_size";
    else if (! modp->init)
	str = "init";
    else if (! modp->xim_init)
	str = "xim_init";
    else if (! modp->xim_end)
	str = "xim_end";
    else if (! modp->keystroke)
	str = "keystroke";
    else if (! modp->show_keystroke)
	str = "show_keystroke";
    if (str) {
	perr(XCINMSG_IWARNING,
	    N_("undefined symbol \"%s\" in module \"%s\", ignore.\n"),
	    str, modp->module_header.name);
	return  False;
    }
/*
 *  Check for the valid objname.
 */
    strncpy(objname, objenc, 64);
    if ((str = strrchr(objname, '@')))
	*str = '\0';
    objn = modp->valid_objname;
    if (! objn) {
	if (strcmp_wild(modp->module_header.name, objname) == 0)
	    check_ok = 1;
    }
    else {
	while (*objn) {
	    if(strcmp_wild(*objn, objname) == 0) {
		check_ok = 1;
		break;
	    }
	    objn ++;
	}
    }
    if (check_ok == 0) {
	perr(XCINMSG_WARNING,
	    N_("invalid objname \"%s\" for module \"%s\", ignore.\n"),
	    objname, modp->module_header.name);
	return False;
    }
    return True;
}

static imodule_t *
creat_module(module_t *templet, char *objenc)
{
    imodule_t *imodp;

    imodp = xcin_malloc(sizeof(imodule_t), 1);
    imodp->modp = (void *)templet;
    imodp->name = templet->module_header.name;
    imodp->version = templet->module_header.version;
    imodp->comments = templet->module_header.comments;
    imodp->module_type = templet->module_header.module_type;
    imodp->conf = xcin_malloc(templet->conf_size, 1);
    imodp->init = templet->init;
    imodp->xim_init = templet->xim_init;
    imodp->xim_end = templet->xim_end;
    imodp->keystroke = templet->keystroke;
    imodp->show_keystroke = templet->show_keystroke;
    imodp->terminate = templet->terminate;

    imodp->objname = (objenc) ? (char *)strdup(objenc) : imodp->name; 

    return imodp;
}

static imodule_t *
IM_load(char *modname, char *objenc, xcin_rc_t *xrc)
{
    module_t *modp;
    imodule_t *imodp;
    int load_ok=1;

    modp = (module_t *)load_module(modname,MTYPE_IM,MODULE_VERSION,xrc,NULL);
    if (check_module(modp, objenc) == False)
	load_ok = 0;
    else {
	imodp = creat_module(modp, objenc);
	if (imodp->init(imodp->conf, objenc, xrc) != True) {
	    free(imodp->conf);
	    free(imodp);
	    load_ok = 0;
	}
    }
    if (load_ok == 0) {
	perr(XCINMSG_WARNING,
	    N_("cannot load IM: %s, ignore.\n"), objenc);
	unload_module((mod_header_t *)modp);
	return NULL;
    }
    return imodp;
}

/*----------------------------------------------------------------------------

	Cinput structer manager.

----------------------------------------------------------------------------*/

int
IM_register(int idx, char *modname, char *objname, char *encoding)
{
    im_t *imp;
    int len;

    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return False;

    imp = IM + idx;
    if (imp->modname)
	free(imp->modname);
    if (imp->objname)
	free(imp->objname);
    imp->modname = (char *)strdup(modname);
    if (strrchr(objname, '@')) {
	imp->objname = (char *)strdup(objname);
	imp->with_enc = (ubyte_t)1;
    }
    else {
	len = strlen(objname) + strlen(encoding) + 2;
	imp->objname = xcin_malloc(len, 0);
	sprintf(imp->objname, "%s@%s", objname, encoding);
	imp->with_enc = (ubyte_t)0;
    }
    return True;
}

int
IM_check_registered(int idx)
{
    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return False;
    if (IM[idx].modname && IM[idx].objname)
	return True;
    else
	return False;
}

imodule_t *
IM_get(int idx, xcin_rc_t *xrc)
{
    im_t *imp = IM + idx;

    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return NULL;
    if (imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname, xrc);
    if (imp->imodp == NULL)
	IM_free(idx);
    return imp->imodp;
}

imodule_t *
IM_get_next(int idx, int *idx_ret, xcin_rc_t *xrc)
{
    int i, j;
    im_t *imp;

    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return NULL;

    *idx_ret = -1;
    for (j=0, i=idx, imp=IM+idx; j<MAX_IM_ENTRY; j++, i++, imp++) {
	if (i >= MAX_IM_ENTRY) {
	    imp = IM;
	    i = 0;
	}
	if (imp->modname && imp->objname) {
	    *idx_ret = i;
            break;
	}
    }
    if (*idx_ret != -1 && imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname, xrc);
    if (imp->imodp == NULL)
	IM_free(*idx_ret);
    return imp->imodp;
}

imodule_t *
IM_get_prev(int idx, int *idx_ret, xcin_rc_t *xrc)
{
    int i, j;
    im_t *imp;

    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return NULL;

    *idx_ret = -1;
    for (j=0, i=idx, imp=IM+idx; j<MAX_IM_ENTRY; j++, i--, imp--) {
	if (i < 0) {
	    imp = IM + MAX_IM_ENTRY - 1;
	    i = MAX_IM_ENTRY - 1;
	}
	if (imp->modname && imp->objname) {
	    *idx_ret = i;
	    break;
	}
    }
    if (*idx_ret != -1 && imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname, xrc);
    if (imp->imodp == NULL)
	IM_free(*idx_ret);
    return imp->imodp;
}

numlist_t *
IM_get_numlist(void)
{
    static numlist_t numlist[MAX_IM_ENTRY+1] = {(numlist_t)-1};
    numlist_t *s=numlist;
    int i;

    if (numlist[0] != (numlist_t)-1)
	return numlist;

    for (i=0; i<MAX_IM_ENTRY; i++) {
	if (IM[i].modname && IM[i].objname) {
	    *s = (numlist_t)i;
	    s++;
	}
    }
    *s = (numlist_t)-1;

    return numlist;
}

imodule_t *
IM_search(char *objname, char *encoding, int *idx_ret, xcin_rc_t *xrc)
{
    int i;
    im_t *imp;
    char objenc[100];

    if (strrchr(objname, '@'))
	strncpy(objenc, objname, 100);
    else
	snprintf(objenc, 100, "%s@%s", objname, encoding);

    *idx_ret = -1;
    for (i=0, imp=IM; i<MAX_IM_ENTRY; i++, imp++) {
	if (imp->objname && ! strcmp(imp->objname, objenc)) {
	    *idx_ret = i;
	    break;
	}
    }
    if (*idx_ret != -1 && imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname, xrc);
    if (imp->imodp == NULL)
	IM_free(*idx_ret);
    return imp->imodp;
}

void 
IM_free(int idx)
{
    im_t *imp;

    if (idx < 0 || idx >= MAX_IM_ENTRY)
	return;

    imp = IM+idx;
    if (imp->modname) {
	free(imp->modname);
	imp->modname = NULL;
    }
    if (imp->objname) {
	free(imp->objname);
	imp->objname = NULL;
    }
    if (imp->imodp) {
	if (imp->imodp->terminate)
	    imp->imodp->terminate(imp->imodp->conf);
	if (imp->imodp->modp)
	    unload_module((mod_header_t *)imp->imodp->modp);
	if (imp->imodp->conf)
	    free(imp->imodp->conf);
	free(imp->imodp);
	imp->imodp = NULL;
    }
}

void
IM_free_all(void)
{
    int i;
    for (i=0; i<MAX_IM_ENTRY; i++)
	IM_free(i);
}

int
get_objenc(char *objname, objenc_t *objenc)
{
    int i;
    im_t *imp;
    char *s;

    for (i=0, imp=IM; i<MAX_IM_ENTRY; i++, imp++) {
	if (imp->objname && ! strcmp(imp->objname, objname))
	    break;
    }
    if (i >= MAX_IM_ENTRY)
	return -1;

    strncpy(objenc->objname, objname, 50);
    s = strrchr(objenc->objname, '@');
    *s = '\0';
    strncpy(objenc->encoding, s+1, 50);

    if (imp->with_enc)
	strncpy(objenc->objloadname, objname, 100);
    else
	strncpy(objenc->objloadname, objenc->objname, 100);
    return 0;
}

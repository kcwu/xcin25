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

struct _immod_list_s {
    module_t *modp;
    int ref;
    struct _immod_list_s *next;
};

static struct _immod_list_s *immod;
static cinput_t cinput[MAX_INP_ENTRY];

/*---------------------------------------------------------------------------

        Load Module & Module Init.

---------------------------------------------------------------------------*/

static int
check_module(module_t *modp, char *ldso_name)
{
    char *str=NULL;

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
	    str, ldso_name);
	return  False;
    }
    return  True;
}


static struct _immod_list_s *
load_IM_module(char *modname, xcin_rc_t *xrc)
{
    module_t *modp;
    struct _immod_list_s *mlist;

    modp = (module_t *)load_module(modname,MTYPE_IM,MODULE_VERSION,xrc,NULL);
    if (modp == NULL || check_module(modp, modname) == False) {
	unload_module((mod_header_t *)modp);
	return NULL;
    }
    mlist = xcin_malloc(sizeof(struct _immod_list_s), 0);
    mlist->modp = modp;
    mlist->ref  = 0;
    mlist->next = immod;
    immod = mlist;

    return mlist;
}

static imodule_t * 
creat_module(module_t *templet, char *objname)
{
    imodule_t *imodp;

    imodp = xcin_malloc(sizeof(imodule_t), 1);
    imodp->name = templet->module_header.name;
    imodp->version = templet->module_header.version;
    imodp->comments = templet->module_header.comments;
    imodp->module_type = templet->module_header.module_type;
    imodp->conf = calloc(1, templet->conf_size);
    imodp->init = templet->init;
    imodp->xim_init = templet->xim_init;
    imodp->xim_end = templet->xim_end;
    imodp->keystroke = templet->keystroke;
    imodp->show_keystroke = templet->show_keystroke;
    imodp->terminate = templet->terminate;

    imodp->objname = (objname) ? (char *)strdup(objname) : imodp->name; 

    return  imodp;
}

imodule_t *
load_IM(char *modname, char *objenc, xcin_rc_t *xrc)
{
    struct _immod_list_s *mlist=immod;
    imodule_t *imodp;
    char **objn, objname[64], *s;
    int i;
/*
 *  See that if the desired module with obj_name has been created.
 */
    for (i=0; i<MAX_INP_ENTRY; i++) {
	if (cinput[i].inpmod &&
	    !strcmp(cinput[i].inpmod->name, modname) &&
	    !strcmp(cinput[i].inpmod->objname, objenc))
	    return  cinput[i].inpmod;
    }
/*
 *  Search the templet.
 */
    strncpy(objname, objenc, 64);
    if ((s = strrchr(objname, '@')))
	*s = '\0';
    while (mlist) {
	if (strcmp(modname, mlist->modp->module_header.name) != 0) {
	    mlist = mlist->next;
	    continue;
	}
	objn = mlist->modp->valid_objname;
	if (! objn) {
	    if (! strcmp_wild(mlist->modp->module_header.name, objname))
		break;
	}
	else {
	    while (*objn) {
	        if(! strcmp_wild(*objn, objname))
		    break;
	        objn ++;
	    }
	    if (*objn)
		break;
	}
	mlist = mlist->next;
    }
/*
 *  Load modules from dynamic libs and run module init.
 */
    if (! mlist && (mlist = load_IM_module(modname, xrc)) == NULL)
	return NULL;
    imodp = creat_module(mlist->modp, objenc);
    if (imodp->init(imodp->conf, objenc, xrc) != True) {
	free(imodp->conf);
	free(imodp);
	return NULL;
    }
    mlist->ref ++;
    return imodp;
}

/*----------------------------------------------------------------------------

	Cinput structer manager.

----------------------------------------------------------------------------*/

cinput_t *
get_cinput(int idx)
{
    cinput_t *cp = cinput + idx;

    if (cp->modname && cp->objname)
	return cp;
    else
	return NULL;
}

cinput_t *
get_cinput_next(int idx, int *idx_ret)
{
    int i, j;
    cinput_t *cp;

    for (j=0, i=idx, cp=cinput+idx; j<MAX_INP_ENTRY; j++, i++, cp++) {
	if (i >= MAX_INP_ENTRY) {
	    cp = cinput;
	    i = 0;
	}
	if (cp->modname && cp->objname) {
	    *idx_ret = i;
            return cp;
	}
    }
    *idx_ret = -1;
    return NULL;
}

cinput_t *
get_cinput_prev(int idx, int *idx_ret)
{
    int i, j;
    cinput_t *cp;

    for (j=0, i=idx, cp=cinput+idx; j<MAX_INP_ENTRY; j++, i--, cp--) {
	if (i < 0) {
	    cp = cinput + MAX_INP_ENTRY - 1;
	    i = MAX_INP_ENTRY - 1;
	}
	if (cp->modname && cp->objname) {
	    *idx_ret = i;
            return cp;
	}
    }
    *idx_ret = -1;
    return NULL;
}

numlist_t *
get_cinput_numlist(void)
{
    static numlist_t numlist[13] = {(numlist_t)-1};
    numlist_t *s=numlist;
    cinput_t *cp;
    int i;

    if (numlist[0] != (numlist_t)-1)
	return numlist;

    for (i=0, cp=cinput; i<MAX_INP_ENTRY; i++, cp++) {
	if (cp->modname && cp->objname) {
	    *s = (numlist_t)i;
	    s++;
	}
    }
    *s = (numlist_t)-1;

    return numlist;
}

cinput_t *
set_cinput(int idx, char *modname, char *objname, char *encoding)
{
    cinput_t *cp = cinput + idx;
    int len;

    if (cp->modname)
	free(cp->modname);
    if (cp->objname)
	free(cp->objname);
    cp->modname = (char *)strdup(modname);
    if (strrchr(objname, '@')) {
	cp->objname = (char *)strdup(objname);
	cp->with_enc = (ubyte_t)1;
    }
    else {
	len = strlen(objname) + strlen(encoding) + 2;
	cp->objname = malloc(len);
	sprintf(cp->objname, "%s@%s", objname, encoding);
	cp->with_enc = (ubyte_t)0;
    }
    return cp;
}

cinput_t *
search_cinput(char *objname, char *encoding, int *idx_ret)
{
    int i;
    cinput_t *cp;
    char objenc[100];

    if (strrchr(objname, '@'))
	strncpy(objenc, objname, 100);
    else
	snprintf(objenc, 100, "%s@%s", objname, encoding);

    for (i=0, cp=cinput; i<MAX_INP_ENTRY; i++, cp++) {
	if (cp->objname && ! strcmp(cp->objname, objenc)) {
	    *idx_ret = i;
	    return cp;
	}
    }
    *idx_ret = -1;
    return NULL;
}

void 
free_cinput(cinput_t *cp)
{
    if (cp->modname) {
	free(cp->modname);
	cp->modname = NULL;
    }
    if (cp->objname) {
	free(cp->objname);
	cp->objname = NULL;
    }
    if (cp->inpmod) {
	free(cp->inpmod->conf);
	free(cp->inpmod);
	cp->inpmod = NULL;
    }
}

int
get_objenc(char *objname, objenc_t *objenc)
{
    int i;
    cinput_t *cp;
    char *s;

    for (i=0, cp=cinput; i<MAX_INP_ENTRY; i++, cp++) {
	if (cp->objname && ! strcmp(cp->objname, objname))
	    break;
    }
    if (i >= MAX_INP_ENTRY)
	return -1;

    strncpy(objenc->objname, objname, 50);
    s = strrchr(objenc->objname, '@');
    *s = '\0';
    strncpy(objenc->encoding, s+1, 50);

    if (cp->with_enc)
	strncpy(objenc->objloadname, objname, 100);
    else
	strncpy(objenc->objloadname, objenc->objname, 100);
    return 0;
}

void
cinput_terminate(void)
{
    int i;
    cinput_t *cp;

    for (i=0, cp=cinput; i<MAX_INP_ENTRY; i++, cp++) {
	if (cp->inpmod && cp->inpmod->terminate)
	    cp->inpmod->terminate(cp->inpmod->conf);
    }
}

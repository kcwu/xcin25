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

#ifdef HAVE_DLOPEN
#  include <dlfcn.h>
#  define load_mod_dynamic load_mod_ldso
#endif

#include "xcintool.h"
#include "imodule.h"

static tmodule_t *mod_templet;
static imodule_t *imodules;     /* imodule: head of list;                  *
                                 * imodule->next: next to the head;        *
                                 * imodule->prev: end element of the list; */
static cinput_t cinput[MAX_INP_ENTRY];

/*---------------------------------------------------------------------------

        Load Module & Module Init.

---------------------------------------------------------------------------*/

static int
check_module(module_t *modp, char *ldso_name)
{
    char *str=NULL;

    if (! modp->name)
	str = "name";
    else if (! modp->version)
	str = "version";
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
	return  0;
    }
    if (! check_version(MODULE_VERSION, modp->version, 0)) {
        perr(XCINMSG_IWARNING, 
	    N_("not valid module \"%s\" with version \"%s\", ignore.\n"),
            modp->name, (modp->version) ? modp->version : "(null)");
	return  False;
    }
    return  True;
}


#ifdef HAVE_DLOPEN
static tmodule_t *
load_mod_ldso(char *modname, int mod_type, xcin_rc_t *xc)
{
    char modn[128], ldso_fn[256];
    char *s, err=0;
    module_t *modp;
    tmodule_t *tmodp;
    void *ldso;
    
    if (! (s = strrchr(modname, '.')) || strcmp(s, ".so"))
        sprintf(modn, "%s.so", modname);
    else
	strncpy(modn, modname, 256);

    if (strchr(modname, '/')) {
	if (! (ldso = dlopen(modn, RTLD_LAZY)))
	    err = 1;
    }
    else {
        sprintf(ldso_fn, "%s/%s", xc->user_dir, modn);
        if (! (ldso = dlopen(ldso_fn, RTLD_LAZY))) {
            sprintf(ldso_fn, "%s/%s", xc->default_dir, modn);
            if (! (ldso = dlopen(ldso_fn, RTLD_LAZY)))
		err = 1;
	}
    }
    if (err) {
	perr(XCINMSG_IWARNING, N_("dlerror: %s\n"), dlerror());
	perr(XCINMSG_WARNING, N_("module \"%s\" not found, ignore.\n"), modn);
	return  NULL;
    }
    modp = (module_t *)dlsym(ldso, "module_ptr");
    if (! modp || ! check_module(modp, ldso_fn)) {
	dlclose(ldso);
	return  NULL;
    }

    tmodp = calloc(1, sizeof(tmodule_t));
    tmodp->ldso = ldso;
    tmodp->name = modp->name;
    tmodp->version = modp->version;
    tmodp->comments = modp->comments;
    tmodp->valid_objname = modp->valid_objname;
    tmodp->module_type = modp->module_type;
    tmodp->conf_size = modp->conf_size;
    tmodp->init = modp->init;
    tmodp->xim_init = modp->xim_init;
    tmodp->xim_end = modp->xim_end;
    tmodp->keystroke = modp->keystroke;
    tmodp->show_keystroke = modp->show_keystroke;
    tmodp->terminate = modp->terminate;

    if (! mod_templet)
        mod_templet = tmodp;
    else {
        mod_templet->prev->next = tmodp;
        tmodp->prev = mod_templet->prev;
    }
    mod_templet->prev = tmodp;

    return  tmodp;
}
#endif


static imodule_t * 
creat_module(tmodule_t *templet, char *objname)
{
    imodule_t *imodp;

    if (! templet)
	return  NULL;

    imodp = calloc(1, sizeof(imodule_t));
    imodp->name = templet->name;
    imodp->version = templet->version;
    imodp->comments = templet->comments;
    imodp->module_type = templet->module_type;
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
load_module(char *modname, char *objenc, int mod_type, xcin_rc_t *xc)
{
    imodule_t *imodp=imodules;
    tmodule_t *tmodp=mod_templet;
    char **objn, objname[64], *s;

/*
 *  See that if the desired module with obj_name has been created.
 */
    while (imodp) {
	if (imodp->module_type == mod_type && !strcmp(imodp->name, modname) && 
	    !strcmp(imodp->objname, objenc))
	    return  imodp;
	imodp = imodp->next;
    }

/*
 *  Search the templet.
 */
    strncpy(objname, objenc, 64);
    if ((s = strrchr(objname, '@')))
	*s = '\0';
    while (tmodp) {
	if (tmodp->module_type != mod_type || strcmp(modname, tmodp->name)) {
	    tmodp = tmodp->next;
	    continue;
	}
	objn = tmodp->valid_objname;
	if (! objn) {
	    if (! strcmp_wild(tmodp->name, objname))
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
	tmodp = tmodp->next;
    }

/*
 *  Load modules from dynamic libs and run module init.
 */
    if (! tmodp)
	tmodp = load_mod_dynamic(modname, mod_type, xc);
    imodp = creat_module(tmodp, objenc);

    if (! imodp)
	return NULL;
    if (imodp->init(imodp->conf, objenc, xc) != True) {
	free(imodp->conf);
	free(imodp);
	return NULL;
    }

/*
 *  Load module OK. Now put the module into link list.
 */
    if (! imodules)
        imodules = imodp;
    else {
        imodules->prev->next = imodp;
        imodp->prev = imodules->prev;
    }
    imodules->prev = imodp;
    return imodp;
}

void
module_comment(char *modname, xcin_rc_t *xc)
{
    tmodule_t *tmodp;

    if ((tmodp = load_mod_dynamic(modname, MOD_CINPUT, xc))) {
	perr(XCINMSG_NORMAL, N_("module \"%s\":"), modname);
	if (tmodp->comments)
	    perr(XCINMSG_EMPTY, "\n\n%s\n", tmodp->comments);
	else
	    perr(XCINMSG_EMPTY, N_("no comments available\n"));
    }
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

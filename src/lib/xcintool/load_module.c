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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xcintool.h"

/*----------------------------------------------------------------------------

	Dynamic module loading:

	For ELF systems (Solaris, GNU/Linux, FreeBSD, HP-UX.11)
	For HP-UX PA32, PA64 systems.

----------------------------------------------------------------------------*/

#ifdef	HAVE_DLOPEN			/* ELF */
#include <dlfcn.h>
#else
#ifdef	HAVE_SHL_LOAD			/* HP-UX */
#include <dl.h>
#endif
#endif

#define BUFLEN	1024

struct _mod_stack_s {
   void *ld;
   mod_header_t *modp;
   int ref;
   struct _mod_stack_s *next;
};

struct _mod_stack_s *mod_stack = NULL;

/*----------------------------------------------------------------------------

	Load and Unload modules.

----------------------------------------------------------------------------*/

static int
find_module(char *path, int path_size, xcin_rc_t *xrc, char *sub_path)
{
    char fn[BUFLEN], str[BUFLEN], *s, *modname=NULL;
    FILE *fp;

    if (check_datafile(path, sub_path, xrc, fn, BUFLEN) == False) {
	s = strstr(path, ".la");
	if(s) {
	    strcpy(s,".so");
	    return check_datafile(path, sub_path, xrc, path, BUFLEN);
	}
	return False;
    }

    fp = open_file(fn, "rt", XCINMSG_IERROR);
    while (get_line(str, BUFLEN, fp, NULL, "#\n") == True) {
	if (strncmp(str, "dlname", 6) == 0) {
	    modname = str+6;
	    break;
	}
    }
    fclose(fp);

    if (! modname)
	return False;

    while (*modname && (*modname == ' ' || *modname == '\t'))
	modname ++;
    if (*modname == '=')
	modname ++;
    while (*modname && (*modname == ' ' || *modname == '\t'))
	modname ++;
    if (*modname == '\'')
	modname ++;
    if ((s = strrchr(modname, '\'')) != NULL)
	*s = '\0';

    if ((s = strrchr(fn, '/')) != NULL)
	*s = '\0';
    snprintf(path, path_size, "%s/%s", fn, modname);
    return check_file_exist(path, FTYPE_FILE);
}

mod_header_t *
load_module(char *modname, enum mtype mod_type, char *version,
	    xcin_rc_t *xrc, char *sub_path)
{
    struct _mod_stack_s *ms=mod_stack;
    char ldfn[BUFLEN];
    mod_header_t *module;
    void *ld=NULL;
    int err=1;

    while (ms != NULL) {
	if (strcmp(modname, ms->modp->name) == 0) {
	    ms->ref ++;
	    return ms->modp;
	}
	ms = ms->next;
    }

    snprintf(ldfn, BUFLEN, "%s.la", modname);
    if (find_module(ldfn, BUFLEN, xrc, sub_path) == True &&
#ifdef	HAVE_DLOPEN
        (ld = dlopen(ldfn, RTLD_LAZY)) != NULL)
#else
#ifdef	HAVE_SHL_LOAD
        (ld = (void *)shl_load(ldfn, BIND_DEFERRED, 0L)) != NULL)
#endif
#endif
	err = 0;

    if (err) {
#ifdef	HAVE_DLOPEN
	char *errstr = dlerror();
        perr(XCINMSG_IWARNING, N_("dlerror: %s\n"), errstr);
#endif
	ld = NULL;
    }

#ifdef	HAVE_DLOPEN
    if (!err && !(module = (mod_header_t *)dlsym(ld, "module_ptr")))
#else
#ifdef	HAVE_SHL_LOAD
    if (!err && shl_findsym((shl_t *)&ld, "module_ptr", TYPE_DATA, &module)!=0)
#endif
#endif
    {
	perr(XCINMSG_IWARNING, N_("module symbol \"module_ptr\" not found.\n"));
	err = 1;
    }
    if (!err && module->module_type != mod_type) {
	perr(XCINMSG_IWARNING,
	     N_("invalid module type, type %d required.\n"), mod_type);
	err = 1;
    }
    if (!err && strcmp(module->version, version) != 0) {
	perr(XCINMSG_IWARNING,
	     N_("invalid module version: %s, version %s required.\n"),
	     module->version, version);
    }

    if (err) {
        perr(XCINMSG_WARNING,
	     N_("cannot load module \"%s\", ignore.\n"), modname);
	if (ld)
#ifdef HAVE_DLOPEN
	    dlclose(ld);
#else
#ifdef HAVE_SHL_OPEN
	    shl_unload((shl_t)ld);
#endif
#endif
	return NULL;
    }
    else {
	ms = xcin_malloc(sizeof(struct _mod_stack_s), 0);
	ms->ld = ld;
	ms->modp = module;
	ms->ref = 1;
	ms->next = mod_stack;
	mod_stack = ms;
	return module;
    }
}

void
unload_module(mod_header_t *modp)
{
    struct _mod_stack_s *ms = mod_stack;

    while (ms != NULL) {
	if (modp == ms->modp) {
	    ms->ref --;
	    break;
	}
	ms = ms->next;
    }
    if (ms && ms->ref <= 0) {
#ifdef	HAVE_DLOPEN
	dlclose(ms->ld);
#else
#ifdef	HAVE_SHL_LOAD
	shl_unload((shl_t)(ms->ld));
#endif
#endif
	mod_stack = ms->next;
	free(ms);
    }
}

/*----------------------------------------------------------------------------

	Print the module comment.

----------------------------------------------------------------------------*/

void
module_comment(mod_header_t *modp, char *mod_name)
{
    if (modp) {
	perr(XCINMSG_NORMAL, N_("module \"%s\":"), mod_name);
	if (modp->comments)
	    perr(XCINMSG_EMPTY, "\n\n%s\n", N_(modp->comments));
	else
	    perr(XCINMSG_EMPTY, N_("no comments available.\n"));
    }
}


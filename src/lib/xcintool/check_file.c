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
#include <unistd.h>
#ifdef HPUX
#  define _INCLUDE_POSIX_SOURCE
#endif
#include <sys/stat.h>
#include "constant.h"
#include "xcintool.h"

int
check_file_exist(char *path, int type)
{
    struct stat buf;

    if (stat(path, &buf) != 0)
	return  False;

    if (type == FTYPE_FILE)
	return  (S_ISREG(buf.st_mode)) ? True : False;
    else if (type == FTYPE_DIR)
	return  (S_ISDIR(buf.st_mode)) ? True : False;
    else
	return  False;
}

/*----------------------------------------------------------------------------

	XCIN data file checking. 
 
----------------------------------------------------------------------------*/

#define BUFLEN	1024

#define return_truefn(true_fn, fn, true_fnsize)				\
{									\
    if (true_fn && true_fnsize > 0)					\
	strncpy(true_fn, fn, true_fnsize);				\
}

int
check_datafile(char *fn, char *sub_path, xcin_rc_t *xrc,
	       char *true_fn, int true_fnsize)
{
    char path[BUFLEN], subp[BUFLEN], *s;
/*
 *  For the obsulated path, try to open it directly.
 */
    if (fn[0] == '/') {
	int ret = check_file_exist(fn, FTYPE_FILE);
	if (ret == True)
	    return_truefn(true_fn, fn, true_fnsize);
	return ret;
    }
/*
 *  Otherwise, search it in user data trees.
 */
    if (sub_path)
	strncpy(subp, sub_path, BUFLEN);
    else
	subp[0] = '\0';

    if (xrc->user_dir) {
	while (subp[0] != '\0') {
	    snprintf(path, BUFLEN, "%s/%s/%s", xrc->user_dir, subp, fn);
	    if (check_file_exist(path, FTYPE_FILE) == True) {
		return_truefn(true_fn, path, true_fnsize);
		return True;
	    }
	    if ((s = strrchr(subp, '/')) != NULL)
		*s = '\0';
	    else
		subp[0] = '\0';
	}
	snprintf(path, BUFLEN, "%s/%s", xrc->user_dir, fn);
	if (check_file_exist(path, FTYPE_FILE) == True) {
	    return_truefn(true_fn, path, true_fnsize);
	    return True;
	}
    }
/*
 *  Otherwise, search it in xcin data trees.
 */
    if (sub_path)
	strncpy(subp, sub_path, BUFLEN);
    else
	subp[0] = '\0';

    while (subp[0] != '\0') {
	snprintf(path, BUFLEN, "%s/%s/%s", xrc->default_dir, subp, fn);
	if (check_file_exist(path, FTYPE_FILE) == True) {
	    return_truefn(true_fn, path, true_fnsize);
	    return True;
	}
	if ((s = strrchr(subp, '/')) != NULL)
	    *s = '\0';
	else
	    subp[0] = '\0';
    }
    snprintf(path, BUFLEN, "%s/%s", xrc->default_dir, fn);
    if (check_file_exist(path, FTYPE_FILE) == True) {
	return_truefn(true_fn, path, true_fnsize);
	return True;
    }

    return False;
}

void
check_xcin_path(xcin_rc_t *xrc, int exitcode)
{
    char path[1024];

    if (xrc->default_dir==NULL)
	xrc->default_dir = XCIN_DEFAULT_DIR;
    if (check_file_exist(xrc->default_dir, FTYPE_DIR) == False) {
	if (exitcode != XCINMSG_NORMAL && exitcode != XCINMSG_EMPTY) {
	    perr(exitcode,
		 N_("the default xcin dir \"%s\" does not exist.\n"),
		 xrc->default_dir);
	    xrc->default_dir = NULL;
	}
    }

    if (! (xrc->usrhome = getenv("HOME")))
	xrc->usrhome = getenv("home");

    if (xrc->user_dir == NULL)
	xrc->user_dir = XCIN_USER_DIR;
    if (xrc->user_dir[0] != '/')
	snprintf(path, 1024, "%s/%s", xrc->usrhome, xrc->user_dir);
    else
	strncpy(path, xrc->user_dir, 1024);
    if (check_file_exist(path, FTYPE_DIR) == False)
	xrc->user_dir = NULL;
    else
	xrc->user_dir = (char *)strdup(path);
}

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

#ifndef  _CONSTANT_H
#define  _CONSTANT_H

#define  XCIN_VERSION  		"2.5.3-pre1"

#ifndef  XCIN_DEFAULT_RCDIR
#define  XCIN_DEFAULT_RCDIR	"/usr/local/etc"
#endif
#ifndef  XCIN_DEFAULT_DIR
#define  XCIN_DEFAULT_DIR	"/usr/local/lib/xcin"
#endif
#ifndef  XCIN_MSGLOCAT
#define  XCIN_MSGLOCAT		"/usr/local/share/locale"
#endif 
#define  XCIN_DEFAULT_RC  	"xcinrc"
#define  XCIN_USER_DIR		".xcin"
#define  DEFAULT_XIMNAME 	"xcin"

#define  MIN_WIN_WIDTH		40	/* Min window width. */
#define  MIN_WIN_WIDTH2		10	/* Min window2 width. */
#define  MAX_WIN_WIDTH		100	/* Max window width. */

/*
 * Internal interface numbers (quotted from `info libtool')
 *
 *   1. Start with version information of `0:0:0' for each libtool library.
 *
 *   2. Update the version information only immediately before a public
 *      release of your software.  More frequent updates are unnecessary,
 *      and only guarantee that the current interface number gets larger
 *      faster.
 *
 *   3. If the library source code has changed at all since the last
 *      update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
 *
 *   4. If any interfaces have been added, removed, or changed since the
 *      last update, increment CURRENT, and set REVISION to 0.
 *
 *   5. If any interfaces have been added since the last public release,
 *      then increment AGE.
 *
 *   6. If any interfaces have been removed since the last public release,
 *      then set AGE to 0.
 */

#define CURRENT_VER	0
#define REVISION_VER	0
#define AGE_VER		0

#endif

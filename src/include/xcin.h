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


#ifndef  _XCIN_H
#define  _XCIN_H

#include <X11/Xlib.h>
#include "gui.h"
#include "IC.h"

/*
 *  Flags for xcin_mode (for the global setting).
 */
#define XCIN_MODE_HIDE          0x00000001      /* hide xcin */
#define XCIN_SHOW_CINPUT        0x00000002      /* sinmd is set to be default */
#define XCIN_XKILL_OFF          0x00000004      /* disable xkill */
#define XCIN_IM_FOCUS           0x00000008      /* IM focus on */
#define XCIN_SINGLE_IMC		0x00000010	/* single inpinfo for all IC */
#define XCIN_KEEP_POSITION	0x00000020	/* keep position enable */
#define XCIN_NO_WM_CTRL		0x00000040	/* disable WM contral */
#define XCIN_OVERSPOT_USRCOLOR	0x00000080	/* use client color setting */
#define XCIN_OVERSPOT_FONTSET	0x00000100	/* use user specified fontset */
#define XCIN_MAINWIN2		0x00000200	/* start mainwin 2 */
#define XCIN_DIFF_BEEP		0x00000400	/* enable different beep */
#define XCIN_OVERSPOT_WINONLY	0x00000800	/* only start OverSpot window */
#define XCIN_KEYBOARD_TRANS	0x00001000	/* translation keyboard layout*/
#define XCIN_RUN_IM_FOCUS       0x00100000      /* run time IM focus on */
#define XCIN_RUN_2B_FOCUS	0x00200000	/* run time 2B focus on */
#define XCIN_RUN_EXIT		0x00400000	/* xcin is now exiting */
#define XCIN_RUN_KILL		0x00800000	/* xcin gets killed & exiting */
#define XCIN_RUN_EXITALL	0x01000000	/* XIM terminated, exiting */

/*
 *  XCIN initialization data structer.
 */
typedef struct {
    char default_im_name[64];
    char default_im_mod_name[64];
    char default_im_sinmd_name[64];
    char im_objname[256];
    char phrase_fn[256];

    char display_name[256];
    char indexfont[1024];
    char fg_color[64], bg_color[64], mfg_color[64], mbg_color[64], 
	 uline_color[64], grid_color[64], label_color[64];
    char xim_name[64];
    char input_styles[1024];
} inner_rc_t;


/*
 *  XCIN core configuration.
 */
typedef struct {
    /* GUI configuration. */
    gui_t gui;

    /* XIM & Input Method configuration. */
    IC *ic, *icp;
    xmode_t xcin_mode;
    inp_state_t default_im;
    inp_state_t default_im_sinmd;
    inp_state_t im_focus;
    XIMStyles input_styles;

    /* Initialization structer. */
    xcin_rc_t xcin_rc;
    inner_rc_t *irc;
} xccore_t;


#endif

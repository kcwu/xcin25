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

#include <string.h>
#include <X11/Xlocale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "IMdkit.h"
#include "Xi18n.h"
#include "constant.h"
#include "xcintool.h"
#include "fkey.h"
#include "xcin.h"

static xccore_t *xccore;
static XIMS ims;

void gui_update_winlist(xccore_t *xccore);
int ic_create(XIMS ims, IMChangeICStruct *call_data, xccore_t *xccore);
int ic_destroy(int icid, xccore_t *xccore);
int ic_clean_all(CARD16 connect_id, xccore_t *xccore);
void ic_get_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore);
void ic_set_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore);

/*----------------------------------------------------------------------------

	XIM Tool Functions.

----------------------------------------------------------------------------*/

struct commit_buf_s {
    unsigned short icid, connect_id;
    int slen, size;
    char *str;
} commit_buf;

static void
xim_commit(void)
{
    char *cch_str_list[1];
    XTextProperty tp;
    IMCommitStruct call_data;

    if (commit_buf.slen == 0)
	return;
    DebugLog(1, ("commit str (%u,%u): %s\n",
		commit_buf.icid, commit_buf.connect_id, commit_buf.str));

    cch_str_list[0] = commit_buf.str;
    XmbTextListToTextProperty(xccore->gui.display, cch_str_list, 1,
                                  XCompoundTextStyle, &tp);
    memset(&call_data, 0, sizeof(call_data));
    call_data.major_code = XIM_COMMIT;
    call_data.icid = commit_buf.icid;
    call_data.connect_id = commit_buf.connect_id;
    call_data.flag = XimLookupChars;
    call_data.commit_string = (char *)tp.value;
    IMCommitString(ims, (XPointer)(&call_data));
    XFree(tp.value);
    commit_buf.slen = 0;
}

static void
commit_string(IC *ic, char *str)
{
    int cch_len;

    if (! str) {
	str = ic->imc->cch;
	cch_len = strlen(str);
    }
    else {
        cch_len = strlen(str);
        if (ic->imc->cch_size < cch_len+1) {
	    ic->imc->cch_size = cch_len+1;
	    ic->imc->cch = xcin_realloc(ic->imc->cch, ic->imc->cch_size);
        }
        strcpy(ic->imc->cch, str);
    }
    commit_buf.icid = ic->id;
    commit_buf.connect_id = ic->connect_id;
    if (commit_buf.size-commit_buf.slen < cch_len+1) {
	commit_buf.size = commit_buf.slen+cch_len+1;
	commit_buf.str = xcin_realloc(commit_buf.str, commit_buf.size);
    }
    strcpy(commit_buf.str+commit_buf.slen, str);
    commit_buf.slen += cch_len;
}

int
xim_connect(IC *ic)
{
    IMPreeditStateStruct call_data;

    if ((ic->ic_state & IC_CONNECT))
	return True;
    call_data.major_code = XIM_PREEDIT_START;
    call_data.minor_code = 0;
    call_data.connect_id = (CARD16)(ic->connect_id);
    call_data.icid = (CARD16)(ic->id);
    ic->ic_state |= IC_CONNECT;
    return IMPreeditStart(ims, (XPointer)&call_data);
}

int
xim_disconnect(IC *ic)
{
    IMTriggerNotifyStruct call_data;

    if (! (ic->ic_state & IC_CONNECT))
	return True;
    call_data.connect_id = (CARD16)(ic->connect_id);
    call_data.icid = (CARD16)(ic->id);
    ic->ic_state &= ~IC_CONNECT;
    return IMPreeditEnd(ims, (XPointer)(&call_data));
}

static int
process_IMKEY(IC *ic, keyinfo_t *keyinfo, unsigned int ret)
{
    char *cch = NULL;
    IM_Context_t *imc = ic->imc;

    if (ret == 0) {
	/* This is IMKEY_ABSORB */
	xccore->gui.winchange |= WIN_CHANGE_IM;
	return True;
    }
    if ((ret & IMKEY_COMMIT) && imc->inpinfo.cch) {
	/* Send the result to IM lib. */
	xccore->gui.winchange |= WIN_CHANGE_IM;
	commit_string(ic, imc->inpinfo.cch);
    }
    if ((ret & IMKEY_BELL)) {
	/* The IM request a bell ring. */
	xccore->gui.winchange |= (WIN_CHANGE_IM | WIN_CHANGE_BELL);
    }
    if ((ret & IMKEY_BELL2)) {
	/* The IM request a bell ring. */
	xccore->gui.winchange |= (WIN_CHANGE_IM | WIN_CHANGE_BELL2);
    }

    if (keyinfo) {
	/* The IM request a modifier escape. */
	if ((ret & IMKEY_SHIFTPHR))
	    cch = qphrase_str(keyinfo->keystr[0], QPHR_SHIFT);
	else if ((ret & IMKEY_CTRLPHR))
	    cch = qphrase_str(keyinfo->keystr[0], QPHR_CTRL);
	else if ((ret & IMKEY_ALTPHR))
	    cch = qphrase_str(keyinfo->keystr[0], QPHR_ALT);
	else if ((ret & IMKEY_FALLBACKPHR)) 
	    cch = qphrase_str(keyinfo->keystr[0], QPHR_FALLBACK);
	if (! cch) {
	    if ((ret & IMKEY_SHIFTESC)) {
		if ((imc->inp_state & IM_2BYTES))
		    cch = fullchar_ascii(&(imc->inpinfo), 1, keyinfo);
		else
		    cch = halfchar_ascii(&(imc->inpinfo), 1, keyinfo);
		if (! cch)
		    ret |= IMKEY_IGNORE;
	    }
	    if ((ret & IMKEY_FALLBACKPHR))
		ret |= IMKEY_IGNORE;
	}
	if (cch) {
	    xccore->gui.winchange |= WIN_CHANGE_IM;
	    commit_string(ic, cch);
	    ret &= ~IMKEY_IGNORE;
	}
    }

    if (! (ret & IMKEY_IGNORE)) {
	/* The key will not be processed further more, and will not be
	   forewared back to XIM client. */
	return True;
    }
    return False;
}

/*----------------------------------------------------------------------------

	IM Context Handlers.

----------------------------------------------------------------------------*/

void
call_xim_init(IC *ic, int reset)
{
    IM_Context_t *imc = ic->imc;

    if (imc->imodp && ! (imc->inp_state & IM_CINPUT)) {
	imc->inpinfo.xcin_wlen = (xccore->gui.mainwin != NULL) ? 
			xccore->gui.mainwin->c_width : 0;
	if (reset)
	    imc->imodp->xim_init(imc->imodp->conf, &(imc->inpinfo));
    }
    imc->inp_state |= (IM_CINPUT | IM_XIMFOCUS);
}

void
call_xim_end(IC *ic, int ic_delete, int clear)
{
    unsigned int ret, i;
    IM_Context_t *imc = ic->imc;

    if (imc->imodp && (imc->inp_state & IM_CINPUT)) {
	if (clear) {
	    ret = imc->imodp->xim_end(imc->imodp->conf, &(imc->inpinfo));
	    if (! ic_delete && ret)
		process_IMKEY(ic, NULL, ret);
	    for (i=0; i<imc->n_gwin; i++)
		gui_freewin(imc->gwin[i].window);
	    imc->n_gwin = 0;
	}
    }
    if (! ic_delete)
	imc->inp_state &= ~(IM_CINPUT | IM_XIMFOCUS);
}

int
change_IM(IC *ic, int inp_num)
{
    static int first_call;
    imodule_t *imodp=NULL;
    int reset_inpinfo, nextim_english=0;
    IM_Context_t *imc = ic->imc;
/*
 *  Check if the module of the desired IM has been loaded or not.
 */
    if (inp_num < 0 || inp_num >= MAX_IM_ENTRY)
	nextim_english = 1;
    else if (! (imodp = IM_get(inp_num, &(xccore->xcin_rc))))
	return False;

    if (first_call == 0) {
	/* in order to let xim_init() take effect */
	reset_inpinfo = 1;
	first_call = 1;
    }
    else if ((xccore->xcin_mode & XCIN_SINGLE_IMC) &&
	     ((int)(imc->inp_num) == inp_num || nextim_english)) {
	/* switch to / from English IM && do not change IM */
	reset_inpinfo = 0;
    }
    else
	reset_inpinfo = 1;

/*
 *  The switch_out() xim_end() will only be called when really change IM.
 */
    xccore->gui.winchange |= WIN_CHANGE_IM;
    if (imc->imodp && imodp != imc->imodp)
	call_xim_end(ic, 0, reset_inpinfo);
 
    if (nextim_english) {
    /* This will switch to English mode. */
	imc->imodp = NULL;
        if ((xccore->xcin_mode & XCIN_IM_FOCUS))
	    xccore->xcin_mode &= ~(XCIN_RUN_IM_FOCUS);
	return True;
    }

/*
 *  Initialize the new IM for this IC.
 */
    imc->imodp = imodp;
    imc->inp_num = inp_num;
    if ((xccore->xcin_mode & XCIN_IM_FOCUS)) {
	xccore->xcin_mode |= XCIN_RUN_IM_FOCUS;
	xccore->im_focus = imc->inp_num;
    }
    if ((xccore->xcin_mode & XCIN_SINGLE_IMC))
	xccore->default_im = imc->inp_num;
    call_xim_init(ic, reset_inpinfo);

    if ((xccore->xcin_mode & XCIN_SHOW_CINPUT)) {
	imc->s_imodp = imc->imodp;
	imc->sinp_num = imc->inp_num;
    }
    else {
	imc->s_imodp = IM_get(xccore->default_im_sinmd, &(xccore->xcin_rc));
	imc->sinp_num = xccore->default_im_sinmd;
    }
    return True;
}

static void
circular_change_IM(IC *ic, int direction)
{
    int i, j;
    inp_state_t inpnum;

    inpnum = ((ic->imc->inp_state & IM_CINPUT)) ? 
			ic->imc->inp_num : MAX_IM_ENTRY;
    for (i=0, j=inpnum+direction; i<MAX_IM_ENTRY+1; i++, j+=direction) {
	if (j > MAX_IM_ENTRY || j < 0)
	    j += ( (j < 0) ? (MAX_IM_ENTRY+1) : -(MAX_IM_ENTRY+1) );
	if (change_IM(ic, j))
	    return;
    }
}

static void
call_simd(IC *ic)
{
    static wchar_t prev_wch;
    simdinfo_t simdinfo;
    wch_t *skey = NULL;
    int skey_len;
    IM_Context_t *imc = ic->imc;

    if (imc->inpinfo.cch_publish.wch) {
	if (imc->inpinfo.cch_publish.wch != prev_wch) {
	    xccore->gui.winchange |= WIN_CHANGE_IM;
	    prev_wch = imc->inpinfo.cch_publish.wch;
	}
	if (imc->inp_num == imc->sinp_num && 
	    imc->inpinfo.suggest_skeystroke &&
	    imc->inpinfo.suggest_skeystroke[0].wch)
	    skey = imc->inpinfo.suggest_skeystroke;
	else {
	    simdinfo.imid = ic->imc->id;
	    simdinfo.xcin_wlen = imc->inpinfo.xcin_wlen;
	    simdinfo.guimode = imc->inpinfo.guimode;
	    simdinfo.cch_publish.wch = imc->inpinfo.cch_publish.wch;
	
	    if (imc->s_imodp->show_keystroke(imc->s_imodp->conf, &simdinfo))
		skey = simdinfo.s_keystroke;
	}
    }
    if (! skey) {
	imc->sinmd_keystroke[0].wch = (wchar_t)0;
	return;
    }

/*
    skey_len = wchs_len(skey);
*/
    for (skey_len=0; skey[skey_len].wch != (wchar_t)0; skey_len++);

    if (imc->skey_size < skey_len+1) {
	imc->skey_size = skey_len+1;
	imc->sinmd_keystroke = xcin_realloc(
		imc->sinmd_keystroke, imc->skey_size * sizeof(wch_t));
    }
    memcpy(imc->sinmd_keystroke, skey, (skey_len+1)*sizeof(wch_t));
}

static void
change_simd(IC *ic)
{
    int idx;
    imodule_t *imodp;
    IM_Context_t *imc = ic->imc;

    do {
	if ((imodp = IM_get_next(imc->sinp_num+1, &idx, &(xccore->xcin_rc))))
	    break;
    } while (idx != imc->sinp_num);

    if (idx != imc->sinp_num) {
        imc->sinp_num = idx;
        imc->s_imodp = imodp;
        xccore->default_im_sinmd = (inp_state_t)idx;
	xccore->gui.winchange |= WIN_CHANGE_IM;
        xccore->xcin_mode &= ~XCIN_SHOW_CINPUT;
	call_simd(ic);
    }
}

/*----------------------------------------------------------------------------

	IM Protocol Handler.

----------------------------------------------------------------------------*/

static int 
xim_open_handler(XIMS ims, IMOpenStruct *call_data)
{
    DebugLog(2, ("XIM_OPEN\n"));
    return True;
}

static int 
xim_close_handler(XIMS ims, IMCloseStruct *call_data)
{
    DebugLog(2, ("XIM_CLOSE\n"));

    if (! (xccore->xcin_mode & XCIN_RUN_EXIT)) {
	ic_clean_all(call_data->connect_id, xccore);
	xccore->gui.winchange |= WIN_CHANGE_IM;
    }
    return True;
}

static int
xim_create_ic_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    int ret;

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;
    ret = ic_create(ims, call_data, xccore);
    *icid = (ret == True) ? call_data->icid : -1;
    DebugLog(2, ("XIM_CREATE_IC: icid=%d\n", *icid));
    return ret;
}

static int
xim_destroy_ic_handler(XIMS ims, IMDestroyICStruct *call_data, int *icid)
{
    *icid = call_data->icid;
    DebugLog(2, ("XIM_DESTORY_IC: icid=%d\n", *icid));

    if (! (xccore->xcin_mode & XCIN_RUN_EXIT))
	ic_destroy(*icid, xccore);
    return True;
}

static int 
xim_set_focus_handler(XIMS ims, IMChangeFocusStruct *call_data, int *icid) 
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_SET_IC_FOCUS: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;
    if ((ic->ic_state & IC_FOCUS))
	return True;
/*
 *  This is special handler for XCIN_SINGLE_IMC mode. In this mode, all ICs
 *  share a single IMC. When the ICs change their focus status, it will have
 *  racing condition: for example: IC A focus out, and IC B focus in. It may
 *  by that IC B receive the focus-in event first, and then IC A receive the
 *  focus-out. This will lead to the un-correct ic->ic_state value. So here
 *  we only allow the focus-in IC to set its id to IMC, and IMC's focus state
 *  will change only when its icid is the same as the id of the current IC.
 *
 *  But for XCIN_SINGLE_IMC mode off, there will not have this problem. But
 *  this technique is still valid, since each IMC in each IC will have the
 *  same icid value as their corresponding IC's id.
 */
    ic->imc->icid = ic->id;
    if ((ic->imc->inp_state & IM_CINPUT)) {
	ic->imc->inp_state |= IM_XIMFOCUS;
	xccore->gui.winchange |= WIN_CHANGE_IM;
    }
    if ((ic->imc->inp_state & IM_2BYTES)) {
	ic->imc->inp_state |= IM_2BFOCUS;
	xccore->gui.winchange |= WIN_CHANGE_IM;
    }
    ic->ic_state |= IC_FOCUS;
    xccore->ic = ic;

    if ((ic->ic_state & IC_NEWIC)) {
	if ((xccore->xcin_mode & XCIN_RUN_IM_FOCUS) ||
	    (xccore->xcin_mode & XCIN_RUN_2B_FOCUS)) {
	    if (xim_connect(ic) == True) {
		if ((xccore->xcin_mode & XCIN_RUN_IM_FOCUS))
		    change_IM(ic, xccore->im_focus);
		if ((xccore->xcin_mode & XCIN_RUN_2B_FOCUS))
		    ic->imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	    }
	}
        ic->ic_state &= ~(IC_NEWIC);
    }
    if ((xccore->xcin_mode & XCIN_SINGLE_IMC)) {
	if ((ic->imc->inp_state & IM_CINPUT) ||
	    (ic->imc->inp_state & IM_2BYTES))
	    xim_connect(ic);
	else
	    xim_disconnect(ic);
	ic->imc->ic_rec = &(ic->ic_rec);
    }
    else
	xccore->gui.winchange |= WIN_CHANGE_REDRAW;
    return True;
}

static int 
xim_unset_focus_handler(XIMS ims, IMChangeFocusStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_UNSET_IC_FOCUS: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;
    if (! (ic->ic_state & IC_FOCUS))
	return True;

    if (! (xccore->xcin_mode & XCIN_SINGLE_IMC)) {
	if ((ic->imc->inp_state & IM_CINPUT)) {
	    ic->imc->inp_state &= ~(IM_XIMFOCUS);
	    xccore->gui.winchange |= WIN_CHANGE_IM;
	}
	if ((ic->imc->inp_state & IM_2BYTES)) {
	    ic->imc->inp_state &= ~(IM_2BFOCUS);
	    xccore->gui.winchange |= WIN_CHANGE_IM;
	}
    }
    xccore->icp = ic;
    ic->ic_state &= ~(IC_FOCUS);
    if (! (xccore->xcin_mode & XCIN_SINGLE_IMC))
	xccore->gui.winchange |= WIN_CHANGE_REDRAW;
    return True;
}

static int 
xim_trigger_handler(XIMS ims, IMTriggerNotifyStruct *call_data, int *icid)
{
    int major_code, minor_code;
    char *str;
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_TRIGGER_NOTIFY: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;

    xccore->icp = xccore->ic;
    xccore->ic = ic;
    xccore->ic->ic_state |= IC_FOCUS;
    ic->imc->icid = ic->id;
    if (call_data->flag == 0) { 		/* on key */
        /* 
	 *  Here, the start of preediting is notified from
         *  IMlibrary, which is the only way to start preediting
         *  in case of Dynamic Event Flow, because ON key is
         *  mandatary for Dynamic Event Flow. 
	 */
        /* xim_preedit_callback_start(ims, ic); */
	ic->ic_state |= IC_CONNECT;

	get_trigger_key(call_data->key_index/3, &major_code, &minor_code);
	switch (major_code) {
	case FKEY_ZHEN:				/* ctrl+space: default IM */
	    if (change_IM(ic, ic->imc->inp_num) == False)
	        xim_disconnect(ic);
	    break;
	case FKEY_2BSB:				/* shift+space: sb/2b */
	    ic->imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	    xccore->gui.winchange = WIN_CHANGE_IM;
	    if ((xccore->xcin_mode & XCIN_IM_FOCUS))
	        xccore->xcin_mode |= XCIN_RUN_2B_FOCUS;
	    break;
	case FKEY_CIRIM:			/* ctrl+shift: circuler + */
	    circular_change_IM(ic, 1);
	    if (! (ic->imc->inp_state & IM_CINPUT))
		xim_disconnect(ic);
	    break;
	case FKEY_CIRRIM:			/* shift+ctrl: circuler - */
	    circular_change_IM(ic, -1);
	    if (! (ic->imc->inp_state & IM_CINPUT))
		xim_disconnect(ic);
	    break;
	case FKEY_IMN:				/* ctrl+alt+?: select IM */
	    if (change_IM(ic, minor_code) == False)
	        xim_disconnect(ic);
	    break;
	case FKEY_QPHRASE:
	    if ((str = qphrase_str(minor_code, QPHR_TRIGGER))) {
		commit_string(ic, str);
		xim_commit();
	    }
	    xim_disconnect(ic);
	}
        return True;
    } 
    else 					/* never happens */
        return False;
}

#ifndef AltGrMask
#define AltGrMask (1L<<13)
#endif

static int
forward_keys_handler(IC *ic, keyinfo_t *keyinfo)
{
    unsigned int ret;
    int major_code, minor_code;
    IM_Context_t *imc;

    imc = ic->imc;
/*
 *  Keycode translation for different layout of keyboards.
 *
 *  In most places of the world, we use the "qwerty" layout of the keyboard.
 *  But in other places, e.g., France, they use the "azerty" layout of the
 *  keyboard. There are many differences between them. For example, the
 *  number keys KP_1 .... KP_9 for "qwerty" keyboard do not have the Shift
 *  ON, but for "azerty" they do. So we have to do the keyboard translation
 *  here, if necessary. The translation is just from other layout to the
 *  "standard" "qwerty" layout.
 *
 *  This translation code is provided by dupre <dupre@lifo.univ-orleans.fr>.
 *  He commented that to do the correct translation, we have to turn off
 *  the non-necessary Shift mask for some keys, and turn on Shift mask for
 *  the others. Sometimes the keycode of the "azerty" keyboard will come
 *  with the "AltGr" mask on. It should always be turned off. Note that the
 *  translation is performed only when Shift and AltGr are NOT ON at the
 *  same moment.
 */
    if ((xccore->xcin_mode & XCIN_KEYBOARD_TRANS)) {
	if (! (keyinfo->keystate & ShiftMask &&
	       keyinfo->keystate & AltGrMask) &&
	    keyinfo->keysym > 32 && keyinfo->keysym < 127 &&
	    keyinfo->keystr_len) {
	    if (strstr("',-./0123456789;=[\\]`abcdefghijklmnopqrstuvwxyz",
			keyinfo->keystr) != NULL) {
		keyinfo->keystate &= ~ShiftMask;
		keyinfo->keystate &= ~AltGrMask;
	    }
	    else {
		keyinfo->keystate |=  ShiftMask;
		keyinfo->keystate &= ~AltGrMask;
	    }
	}
    }
/*
 *  Process the special key binding.
 */
    if (search_funckey(keyinfo->keysym, keyinfo->keystate,
		&major_code, &minor_code)) {
	char *str;
	switch (major_code) {
	case FKEY_ZHEN:
            if ((imc->inp_state & IM_CINPUT))
                change_IM(ic, -1);
            else
                change_IM(ic, imc->inp_num);
	    break;
	case FKEY_2BSB:
            if ((imc->inp_state & IM_2BYTES)) {
                imc->inp_state &= ~(IM_2BYTES | IM_2BFOCUS);
	        if ((xccore->xcin_mode & XCIN_IM_FOCUS))
	            xccore->xcin_mode &= ~XCIN_RUN_2B_FOCUS;
	    }
            else {
                imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	        if ((xccore->xcin_mode & XCIN_IM_FOCUS))
	            xccore->xcin_mode |= XCIN_RUN_2B_FOCUS;
	    }
            xccore->gui.winchange |= WIN_CHANGE_IM;
	    break;
	case FKEY_CIRIM:
            circular_change_IM(ic, 1);
	    break;
	case FKEY_CIRRIM:
            circular_change_IM(ic, -1);
	    break;
	case FKEY_IMN:
            change_IM(ic, minor_code);
	    break;
	case FKEY_CHREP:
	    commit_string(ic, NULL);
	    break;
	case FKEY_SIMD:
            change_simd(ic);
	    break;
	case FKEY_IMFOCUS:
	    if ((xccore->xcin_mode & XCIN_IM_FOCUS)) {
		xccore->xcin_mode &= ~XCIN_IM_FOCUS;
		xccore->xcin_mode &= ~XCIN_RUN_IM_FOCUS;
		xccore->xcin_mode &= ~XCIN_RUN_2B_FOCUS;
	    }
	    else {
		xccore->xcin_mode |= XCIN_IM_FOCUS;
		if ((imc->inp_state & IM_CINPUT))
		    xccore->xcin_mode |= XCIN_RUN_IM_FOCUS;
		else
		    xccore->xcin_mode |= XCIN_RUN_2B_FOCUS;
		xccore->im_focus = imc->inp_num;
	    }
	    xccore->gui.winchange |= WIN_CHANGE_IM;
	    break;
	case FKEY_QPHRASE:
	    if ((str = qphrase_str(minor_code, QPHR_TRIGGER)))
                commit_string(ic, str);
	    break;
	}
	if (! (imc->inp_state & IM_CINPUT) && ! (imc->inp_state & IM_2BYTES))
	    return 0;
	return 1;
    }

/*
 *  Forward key to the IM module.
 */
    if ((imc->inp_state & IM_CINPUT)) {
        imc->inpinfo.xcin_wlen = (xccore->gui.mainwin != NULL) ? 
		xccore->gui.mainwin->c_width : 0;
	imc->inpinfo.zh_ascii = ((imc->inp_state & IM_2BYTES)) ?
		(ubyte_t)1 : (ubyte_t)0;
        ret = imc->imodp->keystroke(imc->imodp->conf,&(imc->inpinfo), keyinfo);
	call_simd(ic);
	if (process_IMKEY(ic, keyinfo, ret) == True)
	    return 1;
    }
    if ((imc->inp_state & IM_2BYTES)) {
	char *cch;
	if ((keyinfo->keystate & Mod1Mask) != Mod1Mask &&
	    (keyinfo->keystate & ControlMask) != ControlMask &&
            (cch = fullchar_keystroke(&(imc->inpinfo), keyinfo->keysym))) {
	    commit_string(ic, cch);
	    return 1;
	}
    }
    return -1;
}

static int
xim_forward_handler(XIMS ims, IMForwardEventStruct *call_data, int *icid)
{
    IC *ic=NULL;
    int ret;
    XKeyEvent *kev;
    keyinfo_t keyinfo;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_FORWARD_EVENT: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;
    if (call_data->event.type != KeyPress)
        return True;    		/* doesn't matter */

    kev = &call_data->event.xkey;
    keyinfo.keystate = kev->state;
    keyinfo.keystr_len = XLookupString(kev, keyinfo.keystr, 16, 
					&(keyinfo.keysym), NULL);
    keyinfo.keystr[(keyinfo.keystr_len >= 0) ? keyinfo.keystr_len : 0] = 0;
    ret = forward_keys_handler(ic, &keyinfo);
    xim_commit();
    if (ret == 0)
	xim_disconnect(ic);
    else if (ret == -1)
	IMForwardEvent(ims, (XPointer)call_data);
    return True;
}

static int
xim_set_ic_values_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_SET_IC_VALUES: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;

    if (! (ic = ic_find(call_data->icid)))
	return False;
    ic_set_values(ic, call_data, xccore);
    return True;
}

static int
xim_get_ic_values_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_GET_IC_VALUES: icid=%d\n", *icid));

    if ((xccore->xcin_mode & XCIN_RUN_EXIT))
	return True;

    if (! (ic = ic_find(call_data->icid)))
	return False;
    ic_get_values(ic, call_data, xccore);
    return True;
}

#define MAX_XIM_SYNC_CHECK	5

static int
xim_sync_reply_handler(XIMS ims, IMSyncXlibStruct *call_data, int *icid)
{
    static int n_check=0;

    *icid = call_data->icid;
/*
 *  This is the main controlling process to terminate xcin, which is a really
 *  involved and complicated process.
 *
 *  There are 2 ways to terminate xcin. 1. Use "kill" command in the shell.
 *  2. Click the "kill" button of the xcin window. In the OverTheSpot style,
 *  every action of the XIM client it will use XSetICValues() to send its
 *  spot location to xcin, and xcin *must* reply it. So when the termination
 *  occures, xcin have to ensure that it has replied all the XSetICValues()
 *  request to the current IC on focus, and then actually exits.
 *
 *  So, when terminating, xcin will send XIM_SYNC to the on-focus IC and wait
 *  for its response. This is the dummy event which is just giving the time
 *  for the XIM client to send its XSetICValues() request to xcin in time
 *  such that xcin can reply it. However, xcin cannot assume that the client
 *  will always send the XSetICValues() request everytime when it is going to
 *  exit or how many requests the client will send. So, we use the method
 *  that xcin tries to send several XIM_SYNC one by one, and expect a centain
 *  number of response from the client. In the amount of the response, the
 *  client may send its XSetICValues() request *in time* (if it needs) such
 *  that xcin can response to it.
 *
 *  For case 1 of termination, note that the "kill" signal is received by the
 *  sighandler() of the main program, but not in the X event loop. In this
 *  case we cannot send the XIM_SYNC in the sighandler(), because outside the
 *  X event loop the XIM_SYNC request will be queued somewhere and cannot be
 *  processed immediately. Hence in sighandler() XIM_SYNC we only set the
 *  special flag XCIN_RUN_KILL to inform xcin this circumstance. When backing
 *  the X event loop, xcin will check for this flag and send the XIM_SYNC
 *  request to the client.
 *
 *  For case 2 since it is already in the X event loop, so it can send the
 *  XIM_SYNC request directly (in gui_main*() when the xcin main window is
 *  being killed).
 *
 *  All the XIM_SYNC request is sent by xim_close() function. When it is
 *  called, it will check if the client window is really exist, and set the
 *  special flag XCIN_RUN_EXIT to denote the situation that xcin is exiting.
 *  Then in the xim_sync_reply_handler() it will decide when the xcin can
 *  actually exit, determined by the number of the XIM_SYNC reply. If xcin
 *  can actually exit, the flag XCIN_RUN_EXITALL is set.
 */
    if ((xccore->xcin_mode & XCIN_RUN_EXIT)) {
	n_check ++;
	DebugLog(2, ("XIM_SYNC_REPLY: icid=%d, n_check=%d\n", *icid, n_check));
	if (n_check < MAX_XIM_SYNC_CHECK) {
	    IC *ic = ic_find(call_data->icid);
	    xim_close(ic);
	}
	else
	    xccore->xcin_mode |= XCIN_RUN_EXITALL;
    }
    return True;
}


static int 
im_protocol_handler(XIMS ims, IMProtocol *call_data)
{
    int ret, icid=-1;

    switch (call_data->major_code) {
    case XIM_OPEN:      
        ret = xim_open_handler(ims, &(call_data->imopen));
	break;
    case XIM_CLOSE:     
        ret = xim_close_handler(ims, &(call_data->imclose));
	break;
    case XIM_CREATE_IC: 
        ret = xim_create_ic_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_DESTROY_IC:
        ret = xim_destroy_ic_handler(ims, &(call_data->destroyic), &icid);
	break;
    case XIM_SET_IC_FOCUS: 
        ret = xim_set_focus_handler(ims, &(call_data->changefocus), &icid);
	break;
    case XIM_UNSET_IC_FOCUS: 
        ret = xim_unset_focus_handler(ims, &(call_data->changefocus), &icid);
	break;
    case XIM_TRIGGER_NOTIFY:
        ret = xim_trigger_handler(ims, &(call_data->triggernotify), &icid);
	break;
    case XIM_FORWARD_EVENT: 
        ret = xim_forward_handler(ims, &(call_data->forwardevent), &icid);
	break;
    case XIM_SET_IC_VALUES: 
        ret = xim_set_ic_values_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_GET_IC_VALUES: 
        ret = xim_get_ic_values_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_SYNC_REPLY:
	ret = xim_sync_reply_handler(ims, &(call_data->sync_xlib), &icid);
	break;
    case XIM_RESET_IC:
        DebugLog(2, ("XIM_RESET_IC\n"));
	ret = True;
	break;
/*
 *  Not implement yet.
 */
    case XIM_PREEDIT_START_REPLY:
        DebugLog(2, ("XIM_PREEDIT_START_REPLY\n"));
	ret = True;
	break;
    case XIM_PREEDIT_CARET_REPLY:
        DebugLog(2, ("XIM_PREEDIT_CARET_REPLY\n"));
	ret = True;
	break;
    default:
	DebugLog(2, ("XIM Unknown\n"));
	ret = False;
	break;
    }
    if (xccore->xcin_mode & XCIN_RUN_KILL) {
	xccore->xcin_mode &= ~XCIN_RUN_KILL;
	xim_close(xccore->ic);
    }
    gui_update_winlist(xccore);
    return ret;
}


/*----------------------------------------------------------------------------

	XIM Initialization.

----------------------------------------------------------------------------*/

#define XIM_TCP_PORT 9010

enum trans_type {
    TRANSPORT_X, 
    TRANSPORT_LOCAL, 
    TRANSPORT_TCP
};

/* Supported Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    NULL
};

/* Supported input styles */
typedef struct {
    char *sty_name;
    XIMStyle value;
    ubyte_t enable;
} im_style_t;

im_style_t im_styles[] = {
    {"Root", 		XIMSTY_Root,		(ubyte_t)0},
    {"OverTheSpot",	XIMSTY_OverSpot,	(ubyte_t)0},
/*    {"OffTheSpot",	XIMSTY_OffSpot,		(ubyte_t)0},	*/
/*    {"OnTheSpot",	XIMSTY_OnSpot,		(ubyte_t)0},	*/
    {NULL,		(XIMStyle)0,		(ubyte_t)0}
};

static XIMStyle
toggle_im_styles(char *sty_name)
{
    int i;

    for (i=0; im_styles[i].sty_name != NULL; i++) {
	if (strcasecmp(sty_name, im_styles[i].sty_name) == 0) {
	    im_styles[i].enable = (ubyte_t)1;
	    return im_styles[i].value;
	}
    }
    perr(XCINMSG_WARNING, 
	N_("unsupported input style: %s, ignore.\n"), sty_name);
    return (XIMStyle)0;
}

static void
setup_input_styles(char *style_list, XIMStyles *input_styles)
{
    static XIMStyle defaultStyles[10];
    int n = sizeof(defaultStyles);

    if (style_list[0] == '\0') {
	defaultStyles[0] = toggle_im_styles("Root");
	input_styles->count_styles = 1;
	input_styles->supported_styles = defaultStyles;
    }
    else {
	char *s=style_list, word[256];
	int n_set=0;

	while (get_word(&s, word, 256, NULL)) {
	    n_set ++;
	    if (n_set >= n) {
		perr(XCINMSG_WARNING, 
		    N_("too many input styles setting: max=%d.\n"), n);
		break;
	    }
	    defaultStyles[n_set-1] = toggle_im_styles(word);
	}
	input_styles->count_styles = n_set;
	input_styles->supported_styles = defaultStyles;
    }
}

void 
xim_init(xccore_t *core)
{
    char transport[128], xim_name[128];
    unsigned int transport_type;
    XIMTriggerKeys on_keys;
    XIMEncodings encodings;
    Window mainwin;
    locale_t *locale;
    int i;

    transport_type = TRANSPORT_X;
    xccore = core;
    locale = &(xccore->xcin_rc.locale);
/*
    if (transport_type == TRANSPORT_LOCAL) {
        char hostname[64];
        char *address = "/tmp/.ximsock";
        gethostname(hostname, 64);
        sprintf(transport, "local/%s:%s", hostname, address);
    } 
    else if (transport_type == TRANSPORT_TCP) {
        char hostname[64];
        int port_number = XIM_TCP_PORT;
        gethostname(hostname, 64);
        sprintf(transport, "tcp/%s:%d", hostname, port_number);
    } 
    else 
*/
    strcpy(transport, "X/");

    setup_input_styles(xccore->irc->input_styles, &(xccore->input_styles));
    encodings.count_encodings = sizeof(zhEncodings)/sizeof(XIMEncoding) - 1;
    encodings.supported_encodings = zhEncodings;
    make_trigger_keys(&on_keys);

    if (xccore->irc->xim_name[0] == '\0') {
        if (strcasecmp("zh_TW.Big5", locale->lc_ctype))
	    snprintf(xim_name, 128, "%s-%s", DEFAULT_XIMNAME, locale->lc_ctype);
        else
	    strncpy(xim_name, DEFAULT_XIMNAME, sizeof(xim_name));
    }
    else
	strncpy(xim_name, xccore->irc->xim_name, sizeof(xim_name));

    mainwin = (xccore->gui.mainwin2) ? 
		xccore->gui.mainwin2->window : xccore->gui.mainwin->window;
    ims = IMOpenIM(xccore->gui.display,
                   IMServerWindow,    mainwin,
                   IMModifiers,       "Xi18n",
                   IMServerName,      xim_name,
                   IMLocale,          locale->lc_ctype,
                   IMServerTransport, transport,
                   IMInputStyles,     &(xccore->input_styles),
                   IMEncodingList,    &encodings,
                   IMProtocolHandler, im_protocol_handler,
                   IMFilterEventMask, KeyPressMask,
                   IMOnKeysList,      &on_keys,
                   NULL);
    if (ims == 0)
	perr(XCINMSG_ERROR,
		N_("IMOpenIM() with name \"%s\" transport \"%s\" failed.\n"),
		xim_name, transport);
    else {
	perr(XCINMSG_NORMAL,
		N_("XIM server \"%s\" transport \"%s\"\n"),
		xim_name, transport);
	perr(XCINMSG_NORMAL, N_("inp_styles: "));
	for (i=0; im_styles[i].sty_name != NULL; i++) {
	    if (im_styles[i].enable)
		perr(XCINMSG_EMPTY, "%s ", im_styles[i].sty_name);
	}
	perr(XCINMSG_EMPTY, "\n");
    }
}

void
xim_close(IC *ic)
{
    IMSyncXlibStruct pass_data;

    DebugLog(2, ("xim_close\n"));
    xccore->xcin_mode |= XCIN_RUN_EXIT;
    if (ic && gui_check_window(ic->ic_rec.client_win) == True) {
	DebugLog(2, ("xim_close sent\n"));
	pass_data.major_code = XIM_SYNC;
	pass_data.minor_code = 0;
	pass_data.connect_id = ic->connect_id;
	pass_data.icid = ic->id;
	IMSyncXlib(ims, (XPointer)&pass_data);
	XSync(xccore->gui.display, False);
    }
    else
	xccore->xcin_mode |= XCIN_RUN_EXITALL;
}

void
xim_terminate(void)
{
    IMCloseIM(ims);
    XSync(xccore->gui.display, False);
}

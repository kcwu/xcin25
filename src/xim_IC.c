/******************************************************************
 
  Copyright 1994, 1995 by Sun Microsystems, Inc.
  Copyright 1993, 1994 by Hewlett-Packard Company
 
Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sun Microsystems, Inc.
and Hewlett-Packard not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.
Sun Microsystems, Inc. and Hewlett-Packard make no representations about
the suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
 
SUN MICROSYSTEMS INC. AND HEWLETT-PACKARD COMPANY DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
SUN MICROSYSTEMS, INC. AND HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
  Author: Hidetoshi Tajima(tajima@Eng.Sun.COM) Sun Microsystems, Inc.
 
******************************************************************/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include "IMdkit.h"
#include "Xi18n.h"
#include "xcintool.h"
#include "xcin.h"

/*----------------------------------------------------------------------------

	Basic IMC handling functions.

----------------------------------------------------------------------------*/

static IM_Context_t *imc_list = (IM_Context_t *)NULL;
static IM_Context_t *imc_free = (IM_Context_t *)NULL;

static IM_Context_t *
new_IMC(int icid, int single_imc)
{
    IM_Context_t *imc;
    int create=0;

    if (! single_imc) {
	if (imc_free != NULL) {
            imc = imc_free;
            imc_free = imc_free->next;
	    memset(imc, 0, sizeof(IM_Context_t));
	}
	else
	    imc = (IM_Context_t *)xcin_malloc(sizeof(IM_Context_t), 1);
	if (imc_list)
	    imc_list->prev = imc;
	imc->next = imc_list;
	imc_list  = imc;
	create = 1;
    }
    else {
	if (! imc_list) {
	    imc = (IM_Context_t *)xcin_malloc(sizeof(IM_Context_t), 1);
	    imc_list = imc;
	    create = 1;
	}
	else
	    imc = imc_list;
    }

    if (create) {
	imc->id = icid;
	imc->sinmd_keystroke = xcin_malloc(10*sizeof(wch_t), 1);
	imc->skey_size = 10;
	imc->cch = xcin_malloc((WCH_SIZE+1)*sizeof(char), 1);
	imc->cch_size = WCH_SIZE + 1;
    }
    imc->icid = icid;
    return imc;
}

static void
delete_IMC(IM_Context_t *imc)
{
    IM_Context_t *prev, *next;

    prev = imc->prev;
    next = imc->next;

    if (prev != NULL)
	prev->next = next;
    else
	imc_list = next;
    if (next != NULL)
	next->prev = prev;

    imc->next = imc_free;
    imc_free  = imc;
    if (imc->cch)
	free(imc->cch);
    if (imc->sinmd_keystroke)
	free(imc->sinmd_keystroke);
    if (imc->overspot_win)
	gui_freewin(imc->overspot_win);
}

IM_Context_t *
imc_find(int imid)
{
    IM_Context_t *imc = imc_list;

    while (imc) {
	if (imc->id == imid)
	    return imc;
	imc = imc->next;
    }
    return NULL;
}

/*----------------------------------------------------------------------------

	New IC, Delete IC, and Find IC functions.

----------------------------------------------------------------------------*/

static IC *ic_list = (IC *)NULL;
static IC *ic_free = (IC *)NULL;

static IC *
new_IC(int single_imc)
{
    static CARD16 icid = 0;
    CARD16 new_icid;
    IC *rec;

    if (ic_free != NULL) {
        rec = ic_free;
        ic_free = ic_free->next;
	new_icid = rec->id;
	memset(rec, 0, sizeof(IC));
    } 
    else {
        rec = (IC *)xcin_malloc(sizeof(IC), 1);
	icid ++;
	new_icid = icid;
    }
    rec->id  = new_icid;
    rec->imc = new_IMC(new_icid, single_imc);

    rec->next = ic_list;
    ic_list   = rec;
    return rec;
}

static void 
delete_IC(IC *ic, IC *last, xccore_t *xccore)
{
    int clear = ((xccore->xcin_mode & XCIN_SINGLE_IMC)) ? 0 : 1;
    ic_rec_t *ic_rec = &(ic->ic_rec);

/* 
 *  The IC is eventually being deleted, so don't process any IMKEY send back.
 */
    call_xim_end(ic, 1, clear);
    xccore->gui.winchange |= WIN_CHANGE_IM;

    if (last != NULL)
	last->next = ic->next;
    else
	ic_list = ic->next;
    ic->next = ic_free;
    ic_free = ic;
    if (xccore->ic == ic)
	xccore->ic = NULL;
    if (xccore->icp == ic)
	xccore->icp = NULL;

    if (ic_rec->pre_attr.base_font)
	free(ic_rec->pre_attr.base_font);
#ifdef XIM_COMPLETE
    if (ic_rec->sts_attr.base_font)
	free(ic_rec->sts_attr.base_font);
    if (ic_rec->resource_name)
	free(ic_rec->resource_name);
    if (ic_rec->resource_class)
	free(ic_rec->resource_class);
#endif

    if (clear)
	delete_IMC(ic->imc);
}

IC *
ic_find(CARD16 icid)
{
    IC *ic = ic_list;

    while (ic != NULL) {
        if (ic->id == icid)
            return ic;
        ic = ic->next;
    }
    return NULL;
}


/*----------------------------------------------------------------------------

	ic_get_values(): Get IC values from xcin.

----------------------------------------------------------------------------*/

/* 
 * prototype:  match(char *attr, XICAttribute *attr_list);
 */
#define match(attr, attr_list) \
	( ( strcmp((attr), (attr_list)->name) == 0 ) ? 1 : 0 )

void
ic_get_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore)
{
    XICAttribute *ic_attr  = call_data->ic_attr;
    XICAttribute *pre_attr = call_data->preedit_attr;
#ifdef XIM_COMPLETE
    XICAttribute *sts_attr = call_data->status_attr;
#endif
    ic_rec_t	 *ic_rec   = &(ic->ic_rec);
    register int  i;

    for (i=0; i < (int)call_data->ic_attr_num; i++, ic_attr++) {
	if (! ic_attr->name)
	    continue;
        if (match (XNFilterEvents, ic_attr)) {
            ic_attr->value = (void *)xcin_malloc(sizeof(CARD32), 0);
            ic_attr->value_length = sizeof(CARD32);
	    *(CARD32*)ic_attr->value = KeyPressMask;
        }
	else if (match (XNInputStyle, ic_attr)) {
	    ic_attr->value = (void *)xcin_malloc(sizeof(INT32), 0);
	    ic_attr->value_length = sizeof(INT32);
	    *(INT32*)ic_attr->value = ic_rec->input_style;
	}
	else if (match (XNSeparatorofNestedList, ic_attr)) {
	    ic_attr->value = (void *)xcin_malloc(sizeof(CARD16), 0);
	    ic_attr->value_length = sizeof(CARD16);
	    *(CARD16*)ic_attr->value = 0;
	}
	else {
            perr(XCINMSG_WARNING, 
		N_("ic_get: unknown IC attr: %s\n"), ic_attr->name);
	}
    }

    for (i=0; i < (int)call_data->preedit_attr_num; i++, pre_attr++) {
	if (! pre_attr->name)
	    continue;
	if (match (XNArea, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(XRectangle), 0);
	    *(XRectangle*)pre_attr->value = ic_rec->pre_attr.area;
	    pre_attr->value_length = sizeof(XRectangle);
	}
	else if (match (XNSpotLocation, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(XPoint), 0);
	    *(XPoint*)pre_attr->value = ic_rec->pre_attr.spot_location;
	    pre_attr->value_length = sizeof(XPoint);
        } 
	else if (match (XNFontSet, pre_attr)) {
	    CARD16 base_len=0;
	    int total_len;
	    char *p;
	    if (ic_rec->pre_attr.base_font)
		base_len = (CARD16)strlen(ic_rec->pre_attr.base_font);
	    total_len = sizeof(CARD16) + (CARD16)base_len + 1;
	    pre_attr->value = (void *)xcin_malloc(total_len, 0);
	    p = (char *)pre_attr->value;
	    memmove(p, &base_len, sizeof(CARD16));
	    p += sizeof(CARD16);
	    strncpy(p, ic_rec->pre_attr.base_font, base_len);
	    pre_attr->value_length = total_len;
        } 
	else if (match (XNForeground, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(long), 0);
	    *(long*)pre_attr->value = xccore->gui.fg_color;
	    pre_attr->value_length = sizeof(long);
        } 
	else if (match (XNBackground, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(long), 0);
	    *(long*)pre_attr->value = xccore->gui.bg_color;
	    pre_attr->value_length = sizeof(long);
	} 
	else if (match (XNPreeditState, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(long), 0);
	    if ((ic->imc->inp_state & IM_CINPUT) ||
		(ic->imc->inp_state & IM_2BYTES))
		*(long*)pre_attr->value = XIMPreeditEnable;
	    else
		*(long*)pre_attr->value = XIMPreeditDisable;
	}
	else if (match (XNLineSpace, pre_attr)) {
            pre_attr->value = (void *)xcin_malloc(sizeof(long), 0);
            *(long*)pre_attr->value = ic_rec->pre_attr.line_space;
            pre_attr->value_length = sizeof(long);
        }

#ifdef XIM_COMPLETE
	else if (match (XNAreaNeeded, pre_attr)) {
	    pre_attr->value = (void *)xcin_malloc(sizeof(XRectangle), 0);
	    *(XRectangle*)pre_attr->value = ic_rec->pre_attr.area_needed;
	    pre_attr->value_length = sizeof(XRectangle);
        } 
#endif
	else {
            perr(XCINMSG_WARNING, 
		N_("ic_get: unknown IC pre_attr: %s\n"), pre_attr->name);
	}
    }

#ifdef XIM_COMPLETE
    for (i = 0; i < (int)call_data->status_attr_num; i++, sts_attr++) {
	if (! sts_attr->name)
	    continue;
        if (match (XNArea, sts_attr)) {
            sts_attr->value = (void *)xcin_malloc(sizeof(XRectangle), 0);
            *(XRectangle*)sts_attr->value = ic->sts_attr.area;
            sts_attr->value_length = sizeof(XRectangle);
        } 
	else if (match (XNAreaNeeded, sts_attr)) {
            sts_attr->value = (void *)xcin_malloc(sizeof(XRectangle), 0);
            *(XRectangle*)sts_attr->value = ic->sts_attr.area_needed;
            sts_attr->value_length = sizeof(XRectangle);
        } 
	else if (match (XNFontSet, sts_attr)) {
            CARD16 base_len = (CARD16)strlen(ic->sts_attr.base_font);
            int total_len = sizeof(CARD16) + (CARD16)base_len;
            char *p;

            sts_attr->value = (void *)xcin_malloc(total_len, 0);
            p = (char *)sts_attr->value;
            memmove(p, &base_len, sizeof(CARD16));
            p += sizeof(CARD16);
            strncpy(p, ic->sts_attr.base_font, base_len);
            sts_attr->value_length = total_len;
        } 
	else if (match (XNForeground, sts_attr)) {
            sts_attr->value = (void *)xcin_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic->sts_attr.foreground;
            sts_attr->value_length = sizeof(long);
        } 
	else if (match (XNBackground, sts_attr)) {
            sts_attr->value = (void *)xcin_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic->sts_attr.background;
            sts_attr->value_length = sizeof(long);
        } 
	else if (match (XNLineSpace, sts_attr)) {
            sts_attr->value = (void *)xcin_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic->sts_attr.line_space;
            sts_attr->value_length = sizeof(long);
        }
	else {
            perr(XCINMSG_WARNING, 
		N_("ic_get: unknown IC sts_attr: %s\n"), sts_attr->name);
	}
    }
#endif
    return;
}


/*----------------------------------------------------------------------------

	ic_set_values(): Set IC values into xcin.

	Here we should carefully check if the value is really changed or
	not, and update it in xcin only when it is really changed.

----------------------------------------------------------------------------*/

#define checkset_ic_val(flag, type, val) 				\
    if (ic_rec->ic_value_set & flag) {					\
	if (memcmp(&(val), pre_attr->value, sizeof(type)) != 0) {	\
	    val = *(type *)pre_attr->value;				\
	    ic_rec->ic_value_update |= flag;				\
	}								\
    }									\
    else {								\
	val = *(type *)pre_attr->value;					\
	ic_rec->ic_value_update |= flag;				\
	ic_rec->ic_value_set |= flag;					\
    }

#define checkset_ic_str(flag, str)					\
    if ((ic_rec->ic_value_set & flag) && str) {				\
	if (strcmp(str, pre_attr->value) != 0) {			\
	    free(str);							\
	    str = (char *)strdup(pre_attr->value);			\
	    ic_rec->ic_value_update |= flag;				\
	}								\
    }									\
    else {								\
	str = (char *)strdup(pre_attr->value);				\
	ic_rec->ic_value_update |= flag;				\
	ic_rec->ic_value_set |= flag;					\
    }

void
ic_set_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore)
/*  For details, see Xlib Ref, Chap 11.6  */
{
    XICAttribute *ic_attr  = call_data->ic_attr;
    XICAttribute *pre_attr = call_data->preedit_attr;
#ifdef XIM_COMPLETE
    XICAttribute *sts_attr = call_data->status_attr;
#endif
    ic_rec_t	 *ic_rec   = &(ic->ic_rec);
    register int  i;

    for (i=0; i < (int)(call_data->ic_attr_num); i++, ic_attr++) {
	if (! ic_attr->name && ! ic_attr->value)
	    continue;
        if (match (XNInputStyle, ic_attr)) {
	    int j;
            ic_rec->input_style = *((INT32 *)ic_attr->value);

	    for (j=0; j < xccore->input_styles.count_styles &&
		      ic_rec->input_style != 
			xccore->input_styles.supported_styles[j]; j++);
	    if (j >= xccore->input_styles.count_styles) {
                perr(XCINMSG_WARNING, 
		     N_("client input style not enabled, set to \"Root\".\n"));
		ic_rec->input_style = XIMSTY_Root;
	    }
        }
	else if (match (XNClientWindow, ic_attr))
            ic_rec->client_win = *(Window *)ic_attr->value;
        else if (match (XNFocusWindow, ic_attr))
            ic_rec->focus_win = *(Window *)ic_attr->value;
#ifdef XIM_COMPLETE
	else if (match (XNResourceName, ic_attr))
	    ic->resource_name = (char *)strdup((char *)ic_attr->value);
	else if (match (XNResourceClass, ic_attr))
	    ic->resource_class = (char *)strdup((char *)ic_attr->value);
#endif
        else
            perr(XCINMSG_WARNING, 
		N_("ic_set: unknown IC attr: %s\n"), ic_attr->name);
    }
        
    for (i=0; i < (int)(call_data->preedit_attr_num); i++, pre_attr++) {
	if (! pre_attr->name && ! pre_attr->value)
	    continue;
        if (match (XNArea, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_AREA, XRectangle,
			    ic_rec->pre_attr.area);
	}
        else if (match (XNSpotLocation, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_SPOTLOC, XPoint,
			    ic_rec->pre_attr.spot_location);
	}
        else if (match (XNFontSet, pre_attr)) {
	    checkset_ic_str(CLIENT_SETIC_PRE_FONTSET,
			    ic_rec->pre_attr.base_font);
        } 
        else if (match (XNForeground, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_FGCOLOR, CARD32,
			    ic_rec->pre_attr.foreground);
	}
        else if (match (XNBackground, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_BGCOLOR, CARD32,
			    ic_rec->pre_attr.background);
	}
        else if (match (XNLineSpace, pre_attr))
            ic_rec->pre_attr.line_space = *(CARD32 *)pre_attr->value;

#ifdef XIM_COMPLETE
        else if (match (XNAreaNeeded, pre_attr))
            ic_rec->pre_attr.area_needed = *(XRectangle *)pre_attr->value;
        else if (match (XNColormap, pre_attr))
            ic_rec->pre_attr.cmap = *(Colormap *)pre_attr->value;
        else if (match (XNStdColormap, pre_attr))
            ic_rec->pre_attr.cmap = *(Colormap *)pre_attr->value;
        else if (match (XNBackgroundPixmap, pre_attr))
            ic_rec->pre_attr.bg_pixmap = *(Pixmap *)pre_attr->value;
        else if (match (XNCursor, pre_attr))
            ic_rec->pre_attr.cursor = *(Cursor *)pre_attr->value;
#endif
        else
            perr(XCINMSG_WARNING, 
		N_("ic_set: unknown IC pre_attr: %s\n"), pre_attr->name);
    }

#ifdef XIM_COMPLETE	
    for (i=0; i < (int)(call_data->status_attr_num); i++, sts_attr++) {
	if (! sts_attr->name && ! sts_attr->value)
	    continue;
        if (match (XNArea, sts_attr))
            ic_rec->sts_attr.area = *(XRectangle *)sts_attr->value;
        else if (match (XNAreaNeeded, sts_attr))
            ic_rec->sts_attr.area_needed = *(XRectangle *)sts_attr->value;
        else if (match (XNColormap, sts_attr))
            ic_rec->sts_attr.cmap = *(Colormap *)sts_attr->value;
        else if (match (XNStdColormap, sts_attr))
            ic_rec->sts_attr.cmap = *(Colormap *)sts_attr->value;
        else if (match (XNForeground, sts_attr))
            ic_rec->sts_attr.foreground = *(CARD32 *)sts_attr->value;
        else if (match (XNBackground, sts_attr))
            ic_rec->sts_attr.background = *(CARD32 *)sts_attr->value;
        else if (match (XNBackgroundPixmap, sts_attr))
            ic_rec->sts_attr.bg_pixmap = *(Pixmap *)sts_attr->value;
        else if (match (XNFontSet, sts_attr)) {
            if (ic->sts_attr.base_font != NULL) {
                if (match (ic->sts_attr.base_font, sts_attr))
                    continue;
                XFree(ic->sts_attr.base_font);
            }
            ic->sts_attr.base_font = (char *)strdup(sts_attr->value);

        } 
	else if (match (XNLineSpace, sts_attr))
            ic_rec->sts_attr.line_space= *(CARD32 *)sts_attr->value;
        else if (match (XNCursor, sts_attr))
            ic_rec->sts_attr.cursor = *(Cursor *)sts_attr->value;
        else
            perr(XCINMSG_WARNING, 
		N_("ic_set: unknown IC sts_attr: %s\n"), sts_attr->name);
    }
#endif
}

int 
ic_create(XIMS ims, IMChangeICStruct *call_data, xccore_t *xccore)
{
    IC *ic;
 
    if (! (ic = new_IC((xccore->xcin_mode & XCIN_SINGLE_IMC))))
        return False;

    ic->connect_id = call_data->connect_id;
    call_data->icid = ic->id;

    if ((xccore->xcin_mode & XCIN_IM_FOCUS))
	ic->imc->inp_num = xccore->im_focus;
    else
	ic->imc->inp_num = xccore->default_im;
    ic->ic_state |= IC_NEWIC;
    ic->imc->inpinfo.imid = (int)(ic->imc->id);

    ic_set_values(ic, call_data, xccore);
    return True;
}

int 
ic_destroy(XIMS ims, IMDestroyICStruct *call_data, xccore_t *xccore)
{
    IC *ic, *last=NULL;

    for (ic=ic_list; ic!=NULL; last=ic, ic=ic->next) {
	if (ic->id == call_data->icid) {
	    delete_IC(ic, last, xccore);
	    return  True;
	}
    }
    return False;
}

int
ic_clean_all(CARD16 connect_id, xccore_t *xccore)
{
    IC *ic=ic_list, *last=NULL;
    int clean_count=0;

    while (ic != NULL) {
        if (ic->connect_id == connect_id) {
	    delete_IC(ic, last, xccore);
	    ic = (last) ? last->next : ic_list;
	    clean_count ++;
	}
	else {
	    last = ic;
	    ic = ic->next;
        }
    }
    return (clean_count) ? True : False;
}

/*---------------------------------------------------------------------------

	Garbage Collection

---------------------------------------------------------------------------*/

#ifdef DEBUG
#define TIMECHECK_STEP    10
#define IC_IDLE_TIME      20
#else
#define TIMECHECK_STEP    300
#define IC_IDLE_TIME      600
#endif

void
check_ic_exist(int icid, xccore_t *xccore)
{
    static time_t last_check;
    IC *ic = ic_list, *last = NULL, *ref_ic;
    time_t current_time;
    int delete;

    if (icid == -1 || (ref_ic = ic_find(icid)) == NULL)
	return;
    current_time = time(NULL);
    if (current_time - last_check <= TIMECHECK_STEP)
	return;

    DebugLog(3, verbose, "Begin check: current time = %d, last check = %d\n", 
		(int)current_time, (int)last_check);

    while (ic != NULL) {
	DebugLog(3, verbose, "IC: id=%d, focus_w=0x%x, client_w=0x%x%s\n",
		ic->id, (unsigned int)ic->ic_rec.focus_win, 
		(unsigned int)ic->ic_rec.client_win, 
		(ic==ref_ic) ? ", (ref)." : ".");
	delete = 0;

	if (ic == ref_ic)
	    ic->exec_time = current_time;
	else if (ic->ic_rec.focus_win && 
		 ic->ic_rec.focus_win == ref_ic->ic_rec.focus_win)
	/* each IC should has its distinct window */
	    delete = 1;
	else if (current_time - ic->exec_time > IC_IDLE_TIME &&
		 ic->ic_rec.client_win != 0) {
	    DebugLog(3, verbose, 
		    "Check IC: id=%d, window=0x%x, exec_time=%d.\n", 
		    ic->id, (unsigned int)ic->ic_rec.client_win, 
		    (int)ic->exec_time);
	    ic->exec_time = current_time;
	    if (gui_check_window(ic->ic_rec.client_win) != True)
		delete = 1;
	}

	if (delete) {
	    DebugLog(3, verbose, 
		    "Delete IC: id=%d, window=0x%x, exec_time=%d.\n", 
		    ic->id, (unsigned int)ic->ic_rec.client_win, 
		    (int)ic->exec_time);
	    delete_IC(ic, last, xccore);
	    ic = (last) ? last->next : ic_list;
	}
	else {
	    last = ic;
	    ic = ic->next;
	}
    }
    last_check = current_time;
}

/*---------------------------------------------------------------------------

	GUI Request registration functions.

---------------------------------------------------------------------------*/

Window gui_request_init(IM_Context_t *imc, greq_win_t *gw);

int
greq_register(int imid, greq_t *greq,
	      int (*greq_callback)(int, int, inpinfo_t *, greq_cb_t *))
{
    int idx, i, reqid=0;
    IM_Context_t *imc;
    greq_win_t *gw;

    if (greq == NULL)
	return -1;
    if (! (imc = imc_find(imid)))
	return -1;

    gw = imc->gwin;
    idx = imc->n_gwin;
    if (idx >= MAX_GREQ_CNT)
	return -1;

    for (i=0; i<idx; i++) {
	if (gw[i].reqid > reqid)
	    reqid = gw[i].reqid;
    }
    gw[idx].reqid = (++reqid);
    gw[idx].greq_callback = greq_callback;
    gw[idx].greq_data = greq;
    if (! (gw[idx].window = gui_request_init(imc, gw+idx))) {
	gw[idx].reqid = 0;
	gw[idx].greq_callback = NULL;
	gw[idx].greq_data = NULL;
	return -1;
    }
    imc->n_gwin ++;
    return reqid;
}

void
greq_unregister_internal(IM_Context_t *imc, int idx)
{
    int j;
    greq_win_t *gw;

    gw = imc->gwin;
    for (j=idx+1; j<imc->n_gwin; idx++, j++) {
	gw[idx].reqid  = gw[j].reqid;
	gw[idx].window = gw[j].window;
	gw[idx].greq_callback = gw[j].greq_callback;
	gw[idx].greq_data = gw[j].greq_data;
    }
    gw[idx].reqid = 0;
    gw[idx].window = (Window)NULL;
    gw[idx].greq_callback = NULL;
    gw[idx].greq_data = NULL;
    imc->n_gwin --;
}

void
greq_unregister(int imid, int reqid)
{
    int i;
    IM_Context_t *imc;
    greq_win_t *gw;

    if (! (imc = imc_find(imid)))
	return;
    if (imc->n_gwin == 0)
	return;
    gw = imc->gwin;

    for (i=0; i<imc->n_gwin; i++) {
	if (gw[i].reqid == reqid)
	    break;
    }
    if (i >= imc->n_gwin)
	return;

    gw[i].greq_callback = NULL;
    gui_freewin(gw[i].window);
    greq_unregister_internal(imc, i);
}

void 
greq_query(int imid, int *n_greq, int **reqid_list_return)
{
    static int reqid_list[MAX_GREQ_CNT];
    int i;
    IM_Context_t *imc;
    greq_win_t *gw;

    if (! (imc = imc_find(imid))) {
	*n_greq = 0;
	reqid_list_return = NULL;
	return;
    }

    *n_greq = imc->n_gwin;
    gw = imc->gwin;
    for (i=0; i < *n_greq; i++)
	reqid_list[i] = gw->reqid;
    for (; i<MAX_GREQ_CNT; i++)
	reqid_list[i] = 0;
    *reqid_list_return = reqid_list;
}


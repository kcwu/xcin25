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

#ifdef HPUX
#  define _INCLUDE_POSIX_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlocale.h>
#include <signal.h>
#include "constant.h"
#include "xcintool.h"
#include "xcin.h"
#include "fkey.h"
#include "util/cin2tab/syscin.h"

int verbose, errstatus;

static xccore_t xcin_core;		/* XCIN kernel data. */

void gui_init(xccore_t *xccore);
void gui_loop(xccore_t *xccore);
void xim_init(xccore_t *xccore);

/*----------------------------------------------------------------------------

        Initial Input.

----------------------------------------------------------------------------*/

static void
xcin_setlocale(void)
{
    locale_t *locale = &(xcin_core.xcin_rc.locale);
    char loc_return[128], enc_return[128];

    set_perr("xcin");
    set_lc_ctype("", loc_return, 128, enc_return, 128, XCINMSG_ERROR);
    locale->lc_ctype = (char *)strdup(loc_return);
    locale->encoding = (char *)strdup(enc_return);

    set_lc_messages("", loc_return, 128);
    locale->lc_messages = (char *)strdup(loc_return);

    if (XSupportsLocale() != True)
        perr(XCINMSG_ERROR, 
	     N_("X locale \"%s\" is not supported by your system.\n"),
	     locale->lc_ctype);
}

static void
print_usage(void)
{
    perr(XCINMSG_EMPTY,
     N_("\n"
	"Usage:  xcin [-h] [-m module] [-d DISPLAY] [-x XIM_name] [-r rcfile]\n"
	"             [-U user_dir] [-D default_dir] [-v n]\n"
	"Options:  -h: print this message.\n"
	"          -m: print the comment of module \"mod_name\"\n"
	"          -d: set X Display.\n"
	"          -x: register XIM server name to Xlib.\n"
	"          -r: set user specified rcfile.\n"
	"          -U: set user xcin data directory.\n"
	"          -D: set default xcin data directory.\n"
	"          -v: set debug level to n.\n\n"));
    perr(XCINMSG_EMPTY,
     N_("Useful environment variables:  $XCIN_RCFILE.\n\n"));
}

static void
command_switch(int argc, char **argv)
/* Command line arguements. */
{
    int  rev;
    char *rcfile=NULL, *mod_comment=NULL;
    xcin_rc_t *xrc = &(xcin_core.xcin_rc);
#ifdef HPUX
    extern char *optarg;
    extern int opterr, optopt;
#endif
/*
 *  Command line arguement & preparing for xcin_rc_t.
 */
    xcin_setlocale();
    xrc->argc = argc;
    xrc->argv = argv;

    opterr = 0;
    while ((rev = getopt(argc, argv, "hm:d:x:r:U:D:v:")) != EOF) {
        switch (rev) {
        case 'h':
            print_usage();
            exit(0);
	case 'm':
	    mod_comment = optarg;
	    break;
	case 'd':
	    strncpy(xcin_core.irc->display_name, optarg,
			sizeof(xcin_core.irc->display_name));
	    break;
	case 'x':
	    strncpy(xcin_core.irc->xim_name, optarg, 
			sizeof(xcin_core.irc->xim_name));
	    break;
        case 'r':
            rcfile = optarg;
            break;
	case 'U':
	    xrc->user_dir = optarg;
	    break;
	case 'D':
	    xrc->default_dir = optarg;
	    break;
	case 'v':
	    verbose = atoi(optarg);
	    break;
        case '?':
            perr(XCINMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }
    check_xcin_path(xrc, XCINMSG_ERROR);
    read_xcinrc(xrc, rcfile);
/*
 *  XCIN perface.
 */
    perr(XCINMSG_EMPTY,
	N_("XCIN (Chinese XIM server) version %s.\n" 
	   "(module ver: %s, syscin ver: %s).\n"
           "(use \"-h\" option for help)\n\n"),
	XCIN_VERSION, MODULE_VERSION, SYSCIN_VERSION);
    perr(XCINMSG_NORMAL, N_("locale \"%s\" encoding \"%s\"\n"),
	xrc->locale.lc_ctype, xrc->locale.encoding);
/*
 *  Print the comment of the module.
 */
    if (mod_comment) {
	mod_header_t *modp;
	if ((modp = load_module(mod_comment, MTYPE_IM, MODULE_VERSION,
		&(xcin_core.xcin_rc), NULL)) != NULL)
	    module_comment(modp, mod_comment);
	else
	    perr(XCINMSG_EMPTY, N_("cannot open module: %s\n"), mod_comment);
	exit(0);
    }
}


/*----------------------------------------------------------------------------

        Resource Reading & Checking.

----------------------------------------------------------------------------*/

static void
read_core_config(void)
{
    xcin_rc_t *xrc = &(xcin_core.xcin_rc);
    char *cmd[1], value[256];
/*
 *  GUI config reading.
 */
    cmd[0] = "INDEX_FONT";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->indexfont, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->indexfont));
    else
	perr(XCINMSG_ERROR, N_("%s: %s: value not specified.\n"),
			xcin_core.xcin_rc.rcfile, cmd[0]);
    cmd[0] = "FG_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->fg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->fg_color));
    cmd[0] = "BG_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->bg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->bg_color));
    cmd[0] = "M_FG_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->mfg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->mfg_color));
    cmd[0] = "M_BG_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->mbg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->mbg_color));
    cmd[0] = "ULINE_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->uline_color, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->uline_color));
    cmd[0] = "GRID_COLOR";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(xcin_core.irc->grid_color, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->grid_color));
    cmd[0] = "XCIN_HIDE";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_MODE_HIDE, 0);
    cmd[0] = "XKILL_DISABLE";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_XKILL_OFF, 0);
    cmd[0] = "ICCHECK_DISABLE";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_ICCHECK_OFF, 0);
    cmd[0] = "SINGLE_IM_CONTEXT";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_SINGLE_IMC, 0);
    cmd[0] = "IM_FOCUS_ON";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_IM_FOCUS, 0);
    cmd[0] = "KEEP_POSITION_ON";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_KEEP_POSITION,0);
    cmd[0] = "DISABLE_WM_CTRL";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_NO_WM_CTRL, 0);
    cmd[0] = "START_MAINWIN2";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_MAINWIN2, 0);
    cmd[0] = "DIFF_BEEP";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_DIFF_BEEP,0);

    cmd[0] = "FKEY_ZHEN";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_ZHEN, value);
    cmd[0] = "FKEY_2BSB";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_2BSB, value);
    cmd[0] = "FKEY_CIRIM";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_CIRIM, value);
    cmd[0] = "FKEY_CIRRIM";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_CIRRIM, value);
    cmd[0] = "FKEY_CHREP";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_CHREP, value);
    cmd[0] = "FKEY_SIMD";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_SIMD, value);
    cmd[0] = "FKEY_IMFOCUS";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_IMFOCUS, value);
    cmd[0] = "FKEY_IMN";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_IMN, value);
    cmd[0] = "FKEY_QPHRASE";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_funckey(FKEY_QPHRASE, value);
    check_funckey();

    cmd[0] = "INPUT_STYLE";
    if (get_resource(xrc, cmd, value, 256, 1))
        set_data(xcin_core.irc->input_styles, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->input_styles));
    cmd[0] = "OVERSPOT_USE_USRCOLOR";
    xcin_core.xcin_mode |= XCIN_OVERSPOT_USRCOLOR;
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_USRCOLOR, 0);
    cmd[0] = "OVERSPOT_USE_USRFONTSET";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_FONTSET, 0);
    cmd[0] = "OVERSPOT_WINDOW_ONLY";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_WINONLY, 0);

    cmd[0] = "KEYBOARD_TRANSLATE";
    if (get_resource(xrc, cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_KEYBOARD_TRANS, 0);
}

static void
split_lc_ctype(char *loc_buf, char *loc, char *enc)
{
    char *s, *s1;

    if ((s = strchr(loc_buf, '.'))) {
	strncpy(loc, loc_buf, (int)(s-loc_buf));
	loc[(int)(s-loc_buf)] = '\0';

	s++;
	s1 = enc;
	while (*s) {
	    *s1 = (char)tolower(*s);
	    s ++;
	    s1++;
	}
	*s1 = '\0';
    }
    else {
	strcpy(loc, loc_buf);
	enc[0] = '\0';
    }
}

static void
read_core_config_locale(void)
{
    char *cmd[2], value[256], *s, loc_buf[64], *loc_name=NULL;
    char *fmt = N_("%s:\n\tlocale section \"%s\": %s: value not specified.\n");
    xcin_rc_t *xrc = &(xcin_core.xcin_rc);
    locale_t *locale = &(xcin_core.xcin_rc.locale);
/*
 *  Determine the true locale setting name.
 */
    cmd[0] = "LOCALE";
    if (get_resource(xrc, cmd, value, 256, 1)) {
	char loc[64], enc[64], sys_loc[64], sys_enc[64];
	s = value;
	split_lc_ctype(locale->lc_ctype, sys_loc, sys_enc);
	while (get_word(&s, loc_buf, 64, NULL)) {
	    split_lc_ctype(loc_buf, loc, enc);
	    if (strcmp(loc, sys_loc) == 0 &&
		strcmp(enc, locale->encoding) == 0) {
		loc_name = loc_buf;
		break;
	    }
	}
    }
    if (loc_name == NULL)
	loc_name = locale->lc_ctype;
/*
 *  Read locale specific global setting.
 */
    cmd[0] = loc_name;
    cmd[1] = "DEFAULT_IM";
    if (! get_resource(xrc, cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, xcin_core.xcin_rc.rcfile, cmd[0], cmd[1]);
    else
        set_data(xcin_core.irc->default_im_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_name));

    cmd[1] = "DEFAULT_IM_MODULE";
    if (! get_resource(xrc, cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else 
	set_data(xcin_core.irc->default_im_mod_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_mod_name));
    if ((s = strrchr(xcin_core.irc->default_im_mod_name, '.')) && 
	!strcmp(s, ".so"))
	*s = '\0';

    cmd[1] = "DEFAULT_IM_SINMD";
    if (! get_resource(xrc, cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else
	set_data(xcin_core.irc->default_im_sinmd_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_sinmd_name));

    cmd[1] = "FONTSET";
    if (get_resource_long(xrc, cmd, value, 256, 2, ','))
	set_data(&(xcin_core.gui.font), RC_STRING, value, 0, 0);
    else
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);

    cmd[1] = "OVERSPOT_FONTSET";
    if (get_resource_long(xrc, cmd, value, 256, 2, ',') && strcmp(value, "NONE"))
	set_data(&(xcin_core.gui.overspot_font), RC_STRING, value, 0, 0);

    cmd[1] = "PHRASE";
    if (get_resource(xrc, cmd, value, 256, 2))
	set_data(xcin_core.irc->phrase_fn, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->phrase_fn));

    cmd[1] = "CINPUT";
    if (! get_resource(xrc, cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else
	set_data(xcin_core.irc->im_objname, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->im_objname));
}

static void
read_core_config_IM(void)
{
    char *cmd[2], value[256], *s, *s1, objname[100], objenc[100];
    char *fmt = N_("%s:\n\tIM section \"%s\": %s: value not specified.\n");
    xcin_rc_t *xrc = &(xcin_core.xcin_rc);
    locale_t *locale = &(xcin_core.xcin_rc.locale);
    int setkey;
/*
 *  Go to each CINPUT sub-node and read important keywords.
 */
    s1 = xcin_core.irc->im_objname;
    while (get_word(&s1, objname, 100, NULL)) {
	snprintf(objenc, 100, "%s@%s", objname, locale->encoding);
	cmd[0] = objenc;
	cmd[1] = "SETKEY";
        if (! get_resource(xrc, cmd, value, 256, 2)) {
	    cmd[0] = objname;
            if (! get_resource(xrc, cmd, value, 256, 2))
		perr(XCINMSG_ERROR, fmt, 
		     xcin_core.xcin_rc.rcfile, objname, cmd[1]);
	}

	/* setkey should be uniquely defined */
	set_data(&setkey, RC_INT, value, 0, 0);
	if (setkey < 0 || setkey > MAX_IM_ENTRY ||
	    IM_check_registered(setkey) == True)
	    perr(XCINMSG_ERROR,_(fmt),xcin_core.xcin_rc.rcfile,objname,cmd[1]);

	cmd[1] = "MODULE";
        if (get_resource(xrc, cmd, value, 256, 2)) {
            if ((s = strrchr(value, '.')) && !strcmp(s, ".so"))
		*s = '\0';
	    IM_register(setkey, value, cmd[0], locale->encoding);
	}
	else
	    IM_register(setkey, xcin_core.irc->default_im_mod_name, 
		       cmd[0], locale->encoding);
    }
}


/*----------------------------------------------------------------------------

        Initialization

----------------------------------------------------------------------------*/

void load_syscin(void)
{
    FILE *fp=NULL;
    int len;
    xcin_rc_t *xrc = &(xcin_core.xcin_rc);
    char buf[40], truefn[256], sub_path[256];
    char inpn_english[CIN_CNAME_LENGTH];
    char inpn_sbyte[CIN_CNAME_LENGTH];
    char inpn_2bytes[CIN_CNAME_LENGTH];
    wch_t ascii[N_ASCII_KEY];
    charcode_t ccp[WCH_SIZE];

    snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
    if (check_datafile("sys.tab", sub_path, xrc, truefn, 256) == True)
	fp = open_file(truefn, "rb", XCINMSG_ERROR);
    else
	perr(XCINMSG_ERROR, N_("sys.tab does not exist.\n"));
    if (fread(buf, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(buf, "syscin"))
	perr(XCINMSG_ERROR, N_("invalid tab file: %s.\n"), truefn);
    len = sizeof(SYSCIN_VERSION);
    if (fread(buf, len, 1, fp) != 1 || strcmp(SYSCIN_VERSION, buf)!=0)
	perr(XCINMSG_ERROR, N_("invalid sys.tab version: %s.\n"), buf);

    if (fread(inpn_english, sizeof(char), CIN_CNAME_LENGTH, fp)
		!= CIN_CNAME_LENGTH ||
	fread(inpn_sbyte, sizeof(char), CIN_CNAME_LENGTH, fp)
		!= CIN_CNAME_LENGTH ||
	fread(inpn_2bytes, sizeof(char), CIN_CNAME_LENGTH, fp)
		!= CIN_CNAME_LENGTH ||
	fread(ascii, sizeof(wch_t), N_ASCII_KEY, fp) != N_ASCII_KEY ||
	fread(ccp, sizeof(charcode_t), WCH_SIZE, fp) != WCH_SIZE)
	perr(XCINMSG_ERROR, N_("sys.tab reading error.\n"));
    fclose(fp);

    xcin_core.gui.inpn_english = (char *)strdup(inpn_english);
    xcin_core.gui.inpn_sbyte   = (char *)strdup(inpn_sbyte);
    xcin_core.gui.inpn_2bytes  = (char *)strdup(inpn_2bytes);
    fullascii_init(ascii);
    ccode_init(ccp, WCH_SIZE);
}

static void
load_default_IM(void)
{
    int idx;
    imodule_t *imodp=NULL;
/*
 *  Try to load the default input method.
 */
    imodp = IM_search(xcin_core.irc->default_im_name, 
	    xcin_core.xcin_rc.locale.encoding, &idx, &(xcin_core.xcin_rc));
    if (imodp) {
	xcin_core.im_focus = xcin_core.default_im = (inp_state_t)idx;
	return;
    }
/*
 *  If false, try to load any specified input method.
 */
    imodp = IM_get_next(0, &idx, &(xcin_core.xcin_rc));
    if (imodp) {
	strncpy(xcin_core.irc->default_im_name, imodp->objname,
		sizeof(xcin_core.irc->default_im_name));
	xcin_core.im_focus = xcin_core.default_im = (inp_state_t)idx;
    }
    else
	perr(XCINMSG_ERROR, N_("no valid input method loaded.\n"));
}
    
static void
load_default_sinmd(void)
{
    int idx;
    imodule_t *imodp=NULL;
/*
 *  Check the default im_sinmd.
 */
    if (! strcmp(xcin_core.irc->default_im_sinmd_name, "DEFAULT"))
	xcin_core.xcin_mode |= XCIN_SHOW_CINPUT;
    else {
	imodp = IM_search(xcin_core.irc->default_im_sinmd_name, 
	    xcin_core.xcin_rc.locale.encoding, &idx, &(xcin_core.xcin_rc));
	if (imodp)
	    xcin_core.default_im_sinmd = (inp_state_t)idx;
	else
	    xcin_core.xcin_mode |= XCIN_SHOW_CINPUT;
    }
}

/*----------------------------------------------------------------------------

        Main Program.

----------------------------------------------------------------------------*/

void xim_terminate(void);

void sighandler(int sig)
{
    if (sig == SIGQUIT)
	DebugLog(1, verbose, "catch signal: SIGQUIT\n");
    else if (sig == SIGTERM)
	DebugLog(1, verbose, "catch signal: SIGTERM\n");
    else
	DebugLog(1, verbose, "catch signal: SIGINT\n");

    if (sig == SIGQUIT)
	return;

    xim_close();
    if ((xcin_core.xcin_mode & XCIN_RUN_EXITALL)) {
	xim_terminate();
	IM_free_all();
	exit(0);
    }
}

int
main(int argc, char **argv)
{
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);

    xcin_core.irc = calloc(sizeof(inner_rc_t), 1);
/*
 *  Option piroity:  command line > environment variable > user rcfile.
 */
    command_switch(argc, argv);
    read_core_config();
    read_core_config_locale();
    read_core_config_IM();

    load_syscin();
    load_default_IM();
    load_default_sinmd();
    qphrase_init(&(xcin_core.xcin_rc), xcin_core.irc->phrase_fn);
    gui_init(&xcin_core);
    xim_init(&xcin_core);
    free(xcin_core.irc);

    gui_loop(&xcin_core);
    return 0;
}

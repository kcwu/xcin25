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
#include "syscin.h"

static xccore_t xcin_core;		/* XCIN kernel data. */
int verbose;

void gui_init(xccore_t *xccore);
void gui_loop(xccore_t *xccore);
void xim_init(xccore_t *xccore);

/*----------------------------------------------------------------------------

        Initial Input.

----------------------------------------------------------------------------*/

static char *
check_default_dir(char *path)
{
    if (path && path[0] && check_file_exist(path, FTYPE_DIR))
	return  (char *)strdup(path);
    else if (check_file_exist(XCIN_DEFAULT_DIR, FTYPE_DIR))
	return  XCIN_DEFAULT_DIR;
    else {
	perr(XCINMSG_ERROR, N_("the default xcin dir does not exist.\n"));
	return  NULL;
    }
}

static char *
check_user_dir(char *path, char *home)
{
    char str[256];

    if (! path) {
	snprintf(str, 256, "%s/%s", home, XCIN_USER_DIR);
        if (check_file_exist(str, FTYPE_DIR))
	    return  (char *)strdup(str);
    }
    else if (path[0] == '/' && check_file_exist(path, FTYPE_DIR))
	return  (char *)strdup(path);
    else {
	snprintf(str, 256, "%s/%s", home, path);
        if (check_file_exist(str, FTYPE_DIR))
	    return  (char *)strdup(str);
    }
    return  NULL;
}


static void
print_usage(void)
{
    perr(XCINMSG_EMPTY,
     N_("\n"
	"Usage:  xcin [-h] [-m module] [-d DISPLAY] [-x XIM_name] [-r rcfile]\n"
	"Options:  -h: print this message.\n"
	"          -m: print the comment of module \"mod_name\"\n"
	"          -d: set X Display.\n"
	"          -x: register XIM server name to Xlib.\n"
	"          -r: set user specified rcfile.\n\n"));
#ifdef DEBUG
    perr(XCINMSG_EMPTY,
     N_("        -v n: set debug level to n.\n\n"));
#endif
    perr(XCINMSG_EMPTY,
     N_("Useful environment variables:  $XCIN_RCFILE.\n\n"));
}

static void
command_switch(int argc, char **argv)
/* Command line arguements. */
{
    int  rev;
    char value[256]={'\0'}, *mod_comment=NULL, *home, *cmd[1];
#ifdef HPUX
    extern char *optarg;
    extern int opterr, optopt;
#endif

    perr(XCINMSG_EMPTY,
	N_("XCIN (Chinese XIM server) version %s.\n" 
	   "(module ver: %s, syscin ver: %s).\n"
           "(use \"-h\" option for help)\n\n"),
	XCIN_VERSION, MODULE_VERSION, SYSCIN_VERSION);
    perr(XCINMSG_NORMAL, N_("locale \"%s\" encoding \"%s\"\n"),
	xcin_core.xcin_rc.locale.lc_ctype, xcin_core.xcin_rc.locale.encoding);

    xcin_core.gui.argc = argc;
    xcin_core.gui.argv = argv;
    opterr = 0;
    while ((rev = getopt(argc, argv, "hm:d:x:r:v:")) != EOF) {
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
            strncpy(value, optarg, 256);
            break;
	case 'v':
	    verbose = atoi(optarg);
	    break;
        case '?':
            perr(XCINMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }
    if (! (home = getenv("HOME")))
	home = getenv("home");

/*
 *  Read rcfile & check default_dir and user_dir.
 */
    xcin_core.xcin_rc.rcfile = read_xcinrc(value, home);

    cmd[0] = "XCIN_DEFAULT_DIR";
    if (get_resource(cmd, value, 256, 1))
	xcin_core.xcin_rc.default_dir = check_default_dir(value);
    else
	xcin_core.xcin_rc.default_dir = check_default_dir(NULL);

    cmd[0] = "XCIN_USER_DIR";
    if (get_resource(cmd, value, 256, 1))
        xcin_core.xcin_rc.user_dir = check_user_dir(value, home);
    else
	xcin_core.xcin_rc.user_dir = check_user_dir(NULL, home);

    set_open_data(xcin_core.xcin_rc.default_dir, xcin_core.xcin_rc.user_dir, 
		  "tab", xcin_core.xcin_rc.locale.encoding);
    if (mod_comment) {
	module_comment(mod_comment, &(xcin_core.xcin_rc));
	exit(0);
    }
}


/*----------------------------------------------------------------------------

        Resource Reading & Checking.

----------------------------------------------------------------------------*/

static void
xcin_setlocale(void)
{
    locale_t *locale = &(xcin_core.xcin_rc.locale);

    set_perr("xcin");
    locale_setting(&(locale->lc_ctype), &(locale->lc_messages), 
		   &(locale->encoding), XCINMSG_ERROR);
    if (XSupportsLocale() != True)
        perr(XCINMSG_ERROR, 
		N_("X locale \"%s\" is not supported by your system.\n"),
		locale->lc_ctype);
}

static void
read_core_config(void)
{
    char *cmd[1], value[256];
/*
 *  GUI config reading.
 */
    cmd[0] = "INDEX_FONT";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->indexfont, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->indexfont));
    else
	perr(XCINMSG_ERROR, N_("%s: %s: value not specified.\n"),
			xcin_core.xcin_rc.rcfile, cmd[0]);
    cmd[0] = "FG_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->fg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->fg_color));
    cmd[0] = "BG_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->bg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->bg_color));
    cmd[0] = "M_FG_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->mfg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->mfg_color));
    cmd[0] = "M_BG_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->mbg_color, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->mbg_color));
    cmd[0] = "ULINE_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->uline_color, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->uline_color));
    cmd[0] = "GRID_COLOR";
    if (get_resource(cmd, value, 256, 1))
	set_data(xcin_core.irc->grid_color, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->grid_color));
    cmd[0] = "XCIN_HIDE";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_MODE_HIDE, 0);
    cmd[0] = "XKILL_DISABLE";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_XKILL_OFF, 0);
    cmd[0] = "ICCHECK_DISABLE";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_ICCHECK_OFF, 0);
    cmd[0] = "SINGLE_IM_CONTEXT";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_SINGLE_IMC, 0);
    cmd[0] = "IM_FOCUS_ON";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_IM_FOCUS, 0);
    cmd[0] = "KEEP_POSITION_ON";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_KEEP_POSITION,0);
    cmd[0] = "DISABLE_WM_CTRL";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_NO_WM_CTRL, 0);
    cmd[0] = "START_MAINWIN2";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_MAINWIN2, 0);
    cmd[0] = "DIFF_BEEP";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value, XCIN_DIFF_BEEP,0);

    cmd[0] = "FKEY_ZHEN";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_ZHEN, value);
    cmd[0] = "FKEY_2BSB";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_2BSB, value);
    cmd[0] = "FKEY_CIRIM";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_CIRIM, value);
    cmd[0] = "FKEY_CIRRIM";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_CIRRIM, value);
    cmd[0] = "FKEY_CHREP";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_CHREP, value);
    cmd[0] = "FKEY_SIMD";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_SIMD, value);
    cmd[0] = "FKEY_IMFOCUS";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_IMFOCUS, value);
    cmd[0] = "FKEY_IMN";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_IMN, value);
    cmd[0] = "FKEY_QPHRASE";
    if (get_resource(cmd, value, 256, 1))
	set_funckey(FKEY_QPHRASE, value);
    check_funckey();

    cmd[0] = "INPUT_STYLE";
    if (get_resource(cmd, value, 256, 1))
        set_data(xcin_core.irc->input_styles, RC_STRARR, value, 0, 
			sizeof(xcin_core.irc->input_styles));
    cmd[0] = "OVERSPOT_USE_USRCOLOR";
    xcin_core.xcin_mode |= XCIN_OVERSPOT_USRCOLOR;
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_USRCOLOR, 0);
    cmd[0] = "OVERSPOT_USE_USRFONTSET";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_FONTSET, 0);
    cmd[0] = "OVERSPOT_WINDOW_ONLY";
    if (get_resource(cmd, value, 256, 1))
	set_data(&(xcin_core.xcin_mode), RC_IFLAG, value,
			XCIN_OVERSPOT_WINONLY, 0);

    cmd[0] = "KEYBOARD_TRANSLATE";
    if (get_resource(cmd, value, 256, 1))
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
    locale_t *locale = &(xcin_core.xcin_rc.locale);
/*
 *  Determine the true locale setting name.
 */
    cmd[0] = "LOCALE";
    if (get_resource(cmd, value, 256, 1)) {
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
    if (! get_resource(cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, xcin_core.xcin_rc.rcfile, cmd[0], cmd[1]);
    else
        set_data(xcin_core.irc->default_im_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_name));

    cmd[1] = "DEFAULT_IM_MODULE";
    if (! get_resource(cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else 
	set_data(xcin_core.irc->default_im_mod_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_mod_name));
    if ((s = strrchr(xcin_core.irc->default_im_mod_name, '.')) && 
	!strcmp(s, ".so"))
	*s = '\0';

    cmd[1] = "DEFAULT_IM_SINMD";
    if (! get_resource(cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else
	set_data(xcin_core.irc->default_im_sinmd_name, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->default_im_sinmd_name));

    cmd[1] = "FONTSET";
    if (get_resource_long(cmd, value, 256, 2, ','))
	set_data(&(xcin_core.gui.font), RC_STRING, value, 0, 0);
    else
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);

    cmd[1] = "OVERSPOT_FONTSET";
    if (get_resource_long(cmd, value, 256, 2, ',') && strcmp(value, "NONE"))
	set_data(&(xcin_core.gui.overspot_font), RC_STRING, value, 0, 0);

    cmd[1] = "PHRASE";
    if (get_resource(cmd, value, 256, 2))
	set_data(xcin_core.irc->phrase_fn, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->phrase_fn));

    cmd[1] = "CINPUT";
    if (! get_resource(cmd, value, 256, 2))
	perr(XCINMSG_ERROR, fmt, cmd[0], cmd[1]);
    else
	set_data(xcin_core.irc->im_objname, RC_STRARR, value, 0,
			sizeof(xcin_core.irc->im_objname));
}

static void
read_core_config_cinput(void)
{
    char *cmd[2], value[256], *s, *s1, objname[100], objenc[100];
    char *fmt = N_("%s:\n\tIM section \"%s\": %s: value not specified.\n");
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
        if (! get_resource(cmd, value, 256, 2)) {
	    cmd[0] = objname;
            if (! get_resource(cmd, value, 256, 2))
		perr(XCINMSG_ERROR, fmt, 
		     xcin_core.xcin_rc.rcfile, objname, cmd[1]);
	}

	/* setkey should be uniquely defined */
	set_data(&setkey, RC_INT, value, 0, 0);
	if (setkey < 0 || setkey > MAX_INP_ENTRY || get_cinput(setkey))
	    perr(XCINMSG_ERROR, fmt, xcin_core.xcin_rc.rcfile, objname, cmd[1]);

	cmd[1] = "MODULE";
        if (get_resource(cmd, value, 256, 2)) {
            if ((s = strrchr(value, '.')) && !strcmp(s, ".so"))
		*s = '\0';
	    set_cinput(setkey, value, cmd[0], locale->encoding);
	}
	else
	    set_cinput(setkey, xcin_core.irc->default_im_mod_name, 
		       cmd[0], locale->encoding);
    }
}


/*----------------------------------------------------------------------------

        Initialization

----------------------------------------------------------------------------*/

void load_syscin()
{
    FILE *fp;
    int len;
    char buf[40], truefn[256];
    char inpn_english[CIN_CNAME_LENGTH];
    char inpn_sbyte[CIN_CNAME_LENGTH];
    char inpn_2bytes[CIN_CNAME_LENGTH];
    wch_t ascii[N_ASCII_KEY];
    charcode_t ccp[WCH_SIZE];

    fp = open_data("sys.tab", "rb", NULL, truefn, 256, XCINMSG_ERROR);
    if (fread(buf, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(buf, "syscin"))
	perr(XCINMSG_ERROR, N_("invalid tab file: %s.\n"), truefn);
    len = sizeof(SYSCIN_VERSION);
    if (fread(buf, len, 1, fp) != 1 || ! check_version(SYSCIN_VERSION, buf, 5))
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
load_default_cinput(void)
{
    int idx;
    cinput_t *cp;
/*
 *  Try to load the default input method.
 */
    if (! (cp = search_cinput(xcin_core.irc->default_im_name, 
		xcin_core.xcin_rc.locale.encoding, &idx)))
	perr(XCINMSG_WARNING, N_("not valid %s: %s, ignore\n"),
		"DEFAULT_IM", xcin_core.irc->default_im_name);
    else {
        if (! (cp->inpmod = load_module(cp->modname, cp->objname, 
		MOD_CINPUT, &(xcin_core.xcin_rc)))) {
	    perr(XCINMSG_WARNING, 
		N_("error loading IM: %s, ignore.\n"), cp->objname);
	    free_cinput(cp);
	}
	else {
	    xcin_core.im_focus = xcin_core.default_im = (inp_state_t)idx;
	    return;
	}
    }

/*
 *  If false, try to load any specified input method.
 */
    while (1) {
	if (! (cp = get_cinput_next(0, &idx)))
	    perr(XCINMSG_ERROR, N_("no valid input method loaded.\n"));

        if (! (cp->inpmod = load_module(cp->modname, cp->objname, 
		MOD_CINPUT, &(xcin_core.xcin_rc)))) {
	    perr(XCINMSG_WARNING, 
		N_("error loading IM: %s, ignore.\n"), cp->objname);
	    free_cinput(cp);
	}
	else {
	    strncpy(xcin_core.irc->default_im_name, cp->objname,
		sizeof(xcin_core.irc->default_im_name));
	    xcin_core.im_focus = xcin_core.default_im = (inp_state_t)idx;
	    break;
	}
    }
}
    
static void
load_default_sinmd(void)
{
    int idx;
    cinput_t *cp;
/*
 *  Check the default show_cinput.
 */
    if (! strcmp(xcin_core.irc->default_im_sinmd_name, "DEFAULT"))
	xcin_core.xcin_mode |= XCIN_SHOW_CINPUT;
    else {
	if ((cp = search_cinput(xcin_core.irc->default_im_sinmd_name, 
			xcin_core.xcin_rc.locale.encoding, &idx))) {
	    if (cp->inpmod || 
		(cp->inpmod = load_module(cp->modname, cp->objname, 
			MOD_CINPUT, &(xcin_core.xcin_rc)))) {
		xcin_core.default_im_sinmd = (inp_state_t)idx;
	        return;
	    }
	    else
	        free_cinput(cp);
	}
	perr(XCINMSG_WARNING, N_("error loading IM: %s, ignore.\n"),
		xcin_core.irc->default_im_sinmd_name);
	xcin_core.xcin_mode |= XCIN_SHOW_CINPUT;
    }
}

/*----------------------------------------------------------------------------

        Main Program.

----------------------------------------------------------------------------*/

void xim_terminate(void);

void sighandler(int sig)
{
#ifdef DEBUG
    if (sig == SIGQUIT)
	DebugLog(1, verbose, "catch signal: SIGQUIT\n");
    else if (sig == SIGTERM)
	DebugLog(1, verbose, "catch signal: SIGTERM\n");
    else
	DebugLog(1, verbose, "catch signal: SIGINT\n");
#endif
    if (sig == SIGQUIT)
	return;

    xim_close();
    if ((xcin_core.xcin_mode & XCIN_RUN_EXITALL)) {
	xim_terminate();
	cinput_terminate();
	exit(0);
    }
}

int
main(int argc, char **argv)
{
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);

    xcin_setlocale();
    xcin_core.irc = calloc(sizeof(inner_rc_t), 1);
/*
 *  Option piroity:  command line > environment variable > user rcfile.
 */
    command_switch(argc, argv);
    read_core_config();
    read_core_config_locale();
    read_core_config_cinput();

    load_syscin();
    load_default_cinput();
    load_default_sinmd();
    qphrase_init(xcin_core.irc->phrase_fn);
    gui_init(&xcin_core);
    xim_init(&xcin_core);
    free(xcin_core.irc);

    gui_loop(&xcin_core);
    return 0;
}

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
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include "xcintool.h"
#include "module.h"
#include "cin2tab.h"

cintab_t cintab;

struct cin {
    char *name, *modname, *cin_version;
    void (*func) (cintab_t *);
};

/*----------------------------------------------------------------------------

	All the converting procedure functions should registered here.

----------------------------------------------------------------------------*/

#include "syscin.h"
#include "gencin.h"
#include "bimscin.h"

int verbose;

extern void syscin(cintab_t *cintab);
extern void gencin(cintab_t *cintab);
extern void bimscin(cintab_t *cintab);

static struct cin cinfunc[] = {
/*  <cin tag>,	 <module ID>,	<cin version>	 <converting func>  */
    {"%sys",  	 "syscin", 	SYSCIN_VERSION,	 syscin},
    {"%gen_inp", "gencin", 	GENCIN_VERSION,  gencin},
    {"%bimspin", "bimscin",	BIMSCIN_VERSION, bimscin},
    {NULL, 	 NULL, 		NULL,		 NULL}
};


/*----------------------------------------------------------------------------

	Cin Reading Functions.

----------------------------------------------------------------------------*/

int
cmd_arg(char *cmd, int cmdlen, ...)
{
    char line[256], *s=line, *arg;
    int arglen, n_read=1;
    va_list list;

    va_start(list, cmdlen);
    if (! get_line(line, 256, cintab.fr, &(cintab.lineno), "#\n"))
	return 0;

    cmd[0] = '\0';
    get_word(&s, cmd, cmdlen, NULL);

    while ((arg = va_arg(list, char *))) {
	arglen = va_arg(list, int);
	if (! get_word(&s, arg, arglen, NULL))
	    break;
	n_read ++;
    }
    return n_read;
}

int
read_hexch_enc(wch_t *wch, char *arg)
{
    int i;
    char *s;
    unsigned char wch_str[WCH_SIZE+1];
    char tmp[3];
    int len;

    if (arg[0] != '0' || !(arg[1] == 'x' || arg[1] == 'X'))
	return 0;

    tmp[2] = '\0';
    s=arg+2;
    for(i=0; i<WCH_SIZE && isxdigit(s[0]) && isxdigit(s[1]); i++, s+=2) {
	tmp[0]=s[0];
	tmp[1]=s[1];
	wch_str[i] = (unsigned char)strtoul(tmp, NULL, 16);
    }
    wch_str[i] = '\0';

    len=mbtowch(wch, wch_str, WCH_SIZE);
    if(len==0) return 0;
    return 2+len*2;
}

int
read_hexwch(unsigned char *wch_str, char *arg)
{
    if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
	char *s = arg+2, tmp[3];
	int i;

	while (*s && isxdigit(*s))
	    s ++;
	if (*s)
	    return 0;

	tmp[2] = '\0';
	for (i=0, s=arg+2; i<WCH_SIZE; i++, s+=2) {
	    if (*s) {
	        tmp[0] = *s;
	        tmp[1] = *(s+1);
	        wch_str[i] = (unsigned char)strtoul(tmp, NULL, 16);
	    }
	    else
		wch_str[i] = (unsigned char)0;
	}
	return 1;
    }
    return 0;
}

char *
turncat_fn(char *fn, char *ext_rm, char *ext_add)
{
    char fn_tmp[256], *s=NULL;

    strncpy(fn_tmp, fn, 254-strlen(ext_add));
    if ((s = strrchr(fn_tmp, '.')) && ! strcmp(s+1, ext_rm)) {
	*s = '\0';
        sprintf(s, ".%s", ext_add);
    }
    else if (! s || strcmp(s+1, ext_add)) {
	strcat(fn_tmp, ".");
        strcat(fn_tmp, ext_add);
    }

    return (char *)strdup(fn_tmp);
}

void
load_systab(char *sysfn, xcin_rc_t *xrc)
{
    FILE *fp=NULL;
    char  version[40], true_fn[256];
    charcode_t ccp[N_ENC_SCHEMA * WCH_SIZE];

    if (sysfn) {
	if (! (fp = open_file(sysfn, "rb", XCINMSG_EMPTY))) {
	    char *fn = turncat_fn(sysfn, "tab", "tab");
	    fp = open_file(fn, "rb", XCINMSG_ERROR);
	}
	strncpy(true_fn, sysfn, 256);
    }
    else {
	char sub_path[256];
	snprintf(sub_path, 256, "tab/%s", xrc->locale.encoding);
	if (check_datafile("sys.tab", sub_path, xrc, true_fn, 256)==True)
	    fp = open_file(true_fn, "rb", XCINMSG_ERROR);
	else
	    perr(XCINMSG_ERROR, "data file \"sys.tab\" does not exist.\n");
    }

    if (fread(version, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(version, "syscin"))
	perr(XCINMSG_ERROR, N_("%s: invalid tab file.\n"), true_fn);
    if (fread(version, sizeof(SYSCIN_VERSION), 1, fp) != 1 ||
	strcmp(SYSCIN_VERSION, version) != 0)
	perr(XCINMSG_ERROR, N_("%s: invalid version.\n"), true_fn);

    if (fseek(fp, CIN_CNAME_LENGTH*3+sizeof(wch_t)*N_ASCII_KEY, SEEK_CUR)==-1 ||
	fread(ccp, sizeof(charcode_t), WCH_SIZE * N_ENC_SCHEMA, fp) != WCH_SIZE * N_ENC_SCHEMA)
	perr(XCINMSG_ERROR, N_("%s: reading error.\n"), true_fn);

    fclose(fp);
    ccode_init(ccp, WCH_SIZE);
}


/*----------------------------------------------------------------------------

	Main Functions.

----------------------------------------------------------------------------*/

void
cin2tab(void)
{
    int i;
    char cmd[64], arg[64], modID[MODULE_ID_SIZE];

    if (! (cintab.fr = open_file(cintab.fname_cin, "rt", XCINMSG_EMPTY))) {
	cintab.fr = open_file(cintab.fname, "rt", XCINMSG_ERROR);
	free(cintab.fname_cin);
	cintab.fname_cin = cintab.fname;
    }
    cintab.fw = open_file(cintab.fname_tab, "wb", XCINMSG_ERROR);

    if (cmd_arg(cmd, 64, arg, 64, NULL)) {
	for (i=0; cinfunc[i].name && strcmp(cinfunc[i].name, cmd) != 0; i++);
	if (cinfunc[i].name) {
	    perr(XCINMSG_NORMAL, 
		N_("cin file: %s, use module: %s version %s.\n"),
		cintab.fname_cin, cinfunc[i].modname, cinfunc[i].cin_version);
	    memset(modID, 0, MODULE_ID_SIZE);
	    strncpy(modID, cinfunc[i].modname, MODULE_ID_SIZE);
	    fwrite(modID, sizeof(char), MODULE_ID_SIZE, cintab.fw);

	    cinfunc[i].func(&cintab);
	}
	else
	    perr(XCINMSG_ERROR, N_("no module header name specified.\n"));
    }
    fclose(cintab.fr);
    fclose(cintab.fw);
}

static void
print_usage(void)
{
    int i;

    perr(XCINMSG_EMPTY, 
	 N_("CIN2TAB version (xcin %s)\n"
	    "Usage: cin2tab [-v] [-r <rcfile>] [-s <sysfn>] [-l <encoding>]\n"
	    "               [-o output] <cin_fn>\n\n"
	    "Supported module header names:\n\t"),
	    XCIN_VERSION);
    for (i=0; cinfunc[i].name; i++)
	perr(XCINMSG_EMPTY, "%s, ", cinfunc[i].name);
    perr(XCINMSG_EMPTY, "\n\n");
}

static void
cin2tab_setlocale(locale_t *locale)
{
    char loc_return[128], enc_return[128];
    int ret;

    if (locale->encoding != NULL)
	return;

    ret = set_lc_ctype("", loc_return, 128, enc_return, 128, XCINMSG_WARNING);
    if (ret == True) {
	locale->lc_ctype = (char *)strdup(loc_return);
	locale->encoding = (char *)strdup(enc_return);
    }
    else {
	set_lc_ctype_env("", loc_return, 128, enc_return, 128, XCINMSG_WARNING);
	locale->lc_ctype = (char *)strdup(loc_return);
	locale->encoding = (char *)strdup(loc_return);
    }
}

int
main(int argc, char **argv)
{
    char *s;
    int rev;
    char loc_return[128], enc_return[128];
#ifdef HPUX
    extern char *optarg;
    extern int opterr, optopt, optind;
#endif

    set_perr("cin2tab");
    set_lc_ctype("", loc_return, 128, enc_return, 128, XCINMSG_ERROR);
    set_lc_messages("", NULL, 0);
    cintab.xrc.argc = argc;
    cintab.xrc.argv = argv;
    if (argc < 2) {
	print_usage();
        exit(0);
    }
    opterr = 0;
    while ((rev = getopt(argc, argv, "v:hr:s:l:o:")) != EOF) {
        switch (rev) {
	case 'v':
	    verbose = atoi(optarg);
	    break;
	case 'h':
	    print_usage();
	    exit(0);
	case 's':
	    cintab.sysfn = (char *)strdup(optarg);
	    break;
	case 'l':
	    if ((s = strchr(optarg, '.')) != NULL) {
		cintab.xrc.locale.lc_ctype = (char *)strdup(optarg);
		cintab.xrc.locale.encoding = (char *)strdup(s+1);
	    }
	    else
		cintab.xrc.locale.encoding = (char *)strdup(optarg);
	    s = cintab.xrc.locale.encoding;
	    while (*s) {
		*s = (char)tolower(*s);
		s ++;
	    }
	    break;
	case 'o':
	    cintab.fname_tab = (char *)strdup(optarg);
	    break;
        case '?':
            perr(XCINMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }
    cin2tab_setlocale(&(cintab.xrc.locale));

    perr(XCINMSG_EMPTY, N_("CIN2TAB version (%s) encoding=%s\n"),
	XCIN_VERSION, cintab.xrc.locale.encoding);

    if (! argv[optind])
	perr(XCINMSG_ERROR, N_("no cin file specified.\n"));
    if (! cintab.xrc.locale.encoding)
	perr(XCINMSG_ERROR, N_("no valid encoding specified.\n"));
    cintab.fname = (char *)strdup(argv[optind]);
    cintab.fname_cin = turncat_fn(argv[optind], "cin", "cin");
    if (! cintab.fname_tab)
	cintab.fname_tab = turncat_fn(argv[optind], "cin", "tab");

    cin2tab();
    return 0;
}

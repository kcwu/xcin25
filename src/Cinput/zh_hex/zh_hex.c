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
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "xcintool.h"
#include "module.h"
#include "zh_hex.h"


/*----------------------------------------------------------------------------

        zh_hex_init()

----------------------------------------------------------------------------*/

static void 
cname_analy(char *inp_cname, char *value, int slen)
{
    char *s1=value, *s2=value, *s3=NULL, tmp[3];
    int i=0;

    if (value[0] == '0' && value[1] == 'x') {
	tmp[2] = '\0';
	do {
	    s2 = strchr(s1, '+');
	    if (s2 != NULL)
		*s2 = '\0';
	    for(s1+=2; i<slen-1 && *s1!='\0'; i++, s1+=2) {
		tmp[0] = *s1;
		tmp[1] = *(s1+1);
		inp_cname[i] = (char)strtol(tmp, &s3, 16);
		if (*s3 != '\0')
		    break;
	    }
	    if (*s3 != '\0')
		break;
	    s1 = s2 + 1;
	} while (s2 != NULL);

	inp_cname[i] = '\0';
	if (*s3 != '\0')
	    strncat(inp_cname, s1, slen-i-1);
    }
    else
	strncpy(inp_cname, value, slen);
}

static int
zh_hex_init(void *conf, char *objname, xcin_rc_t *xrc)
{
    char *cmd[2], value[50], buf[100];
    objenc_t objenc;
    zh_hex_conf_t *cf = (zh_hex_conf_t *)conf;

    if (get_objenc(objname, &objenc) == -1)
	return False;

    cmd[0] = objenc.objloadname;

    cmd[1] = buf;
    snprintf(buf, 100, "INP_CNAME_%s", objenc.encoding);
    if (get_resource(xrc, cmd, value, 50, 2))		/* inp_names */
	cname_analy((char *)cf->inp_cname, value, sizeof(cf->inp_cname));
    else {
	cmd[1] = "INP_CNAME";
	if (get_resource(xrc, cmd, value, 50, 2))	/* inp_names */
	    cname_analy((char *)cf->inp_cname, value, sizeof(cf->inp_cname));
    }
    if (cf->inp_cname[0] == '\0')
	strncpy((char *)cf->inp_cname, "ZhHex", sizeof(cf->inp_cname));
    cf->inp_ename = (char *)strdup(objenc.objname);

    cmd[1] = "SETKEY";						/* setkey */
    if (get_resource(xrc, cmd, value, 50, 2))
        cf->setkey = (byte_t)atoi(value);
    else {
        perr(XCINMSG_WARNING, N_("%s: %s: value not found.\n"), 
			      objenc.objname, cmd[1]);
	return False;
    }

    cmd[1] = "BEEP_WRONG";					/* beep_mode */
    if (get_resource(xrc, cmd, value, 50, 2))
	set_data(&(cf->beep_mode), RC_BFLAG, value, INP_MODE_BEEPWRONG, 0);

    ccode_info(&(cf->ccode_info));
    return  True;
}


/*----------------------------------------------------------------------------

        zh_hex_xim_init(), zh_hex_xim_end()

----------------------------------------------------------------------------*/

static int
zh_hex_xim_init(void *conf, inpinfo_t *inpinfo)
{
    zh_hex_conf_t *cf = (zh_hex_conf_t *)conf;

    inpinfo->iccf = xcin_malloc(sizeof(zh_hex_iccf_t), 1);
    inpinfo->inp_cname = (char *)cf->inp_cname;
    inpinfo->inp_ename = cf->inp_ename;
    inpinfo->area3_len = 8;
    inpinfo->guimode = 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = xcin_malloc((KEY_CODE_LEN+1)*sizeof(wch_t), 1);
    inpinfo->suggest_skeystroke = NULL;

    inpinfo->n_selkey = 0;
    inpinfo->s_selkey = NULL;
    inpinfo->n_mcch = 0;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;

    inpinfo->n_lcch = 0;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;
    inpinfo->cch_publish.wch = (wchar_t)0;
    return True;
}

static unsigned int
zh_hex_xim_end(void *conf, inpinfo_t *inpinfo)
{
    free(inpinfo->s_keystroke);
    free(inpinfo->iccf);
    inpinfo->s_keystroke = NULL;
    inpinfo->iccf = NULL;

    return IMKEY_ABSORB;
}


/*----------------------------------------------------------------------------

        zh_hex_switch_in(), zh_hex_switch_out()

----------------------------------------------------------------------------*/

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
static int
IsValidByteSeq(charcode_t *ccp, unsigned char *s, int num, int incomplete)
{
   int i, j, idx, enc_schema, num_char;

   num_char = MIN(num, WCH_SIZE);
   for (enc_schema = 0; enc_schema < N_ENC_SCHEMA; ++enc_schema) {
       charcode_t *tmp_ccp = ccp + enc_schema * WCH_SIZE;
       if (tmp_ccp[0].n == 0) return -1;

       for (i=0; i<num_char; i++) {
           idx = tmp_ccp[i].n;

           for (j=0; j<idx; j++) {
	       unsigned char s1, begin, end;

	       if (incomplete && i == num_char -1) {
		   s1 = *(s+i) & 0xf0;
		   begin = tmp_ccp[i].begin[j] & 0xf0;
		   end = tmp_ccp[i].end[j] & 0xf0;
	       } else {
		   s1 = *(s+i);
		   begin = tmp_ccp[i].begin[j];
		   end = tmp_ccp[i].end[j];
	       }

	       if ( s1 >= begin && s1 <= end ) break;
           }

          if (idx == 0  || j == idx) break;
       }

       if (i == num_char) return enc_schema;
   }

  return -1;
}
#undef MIN

static int
zh_hex_compose_char(char *s, char *keystroke, int num_keys)
{
    unsigned int cc, i;

    for (i = 0; i < num_keys; ++i) {
    	cc = keystroke[i];
    	cc -= ('0' <= cc && cc <= '9') ? '0' : ('A'-10);
	if (i % 2) s[i/2] |= cc;
	else s[i/2] |= (cc << 4);
    }
   
    return (num_keys + 1)/2;
}

static wchar_t
zh_hex_check_char(zh_hex_conf_t *cf, char *keystroke, int num_keys)
{
    wch_t cch;

    cch.wch = 0;

    zh_hex_compose_char(cch.s, keystroke, num_keys);

    return (match_encoding(&cch)) ? cch.wch : 0;
}

static unsigned int
zh_hex_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    zh_hex_conf_t *cf = (zh_hex_conf_t *)conf;
    zh_hex_iccf_t *iccf = (zh_hex_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    wch_t cch_w;
    static char cch_s[WCH_SIZE+1];
    int len;
    char keychar;

    len = inpinfo->keystroke_len;
    inpinfo->cch = NULL;

    if ((keysym == XK_BackSpace || keysym == XK_Delete) && len) {
        inpinfo->cch_publish.wch = (wchar_t)0;
	iccf->keystroke[len-1] = '\0';
	inpinfo->s_keystroke[len-1].wch = (wchar_t)0;
	inpinfo->keystroke_len --;
        return IMKEY_ABSORB;
    }
    else if (keysym == XK_Escape && len) {
        inpinfo->cch_publish.wch = (wchar_t)0;
	iccf->keystroke[0] = '\0';
	inpinfo->s_keystroke[0].wch = (wchar_t)0;
	inpinfo->keystroke_len = 0;
        return IMKEY_ABSORB;
    }
    else if ((XK_0 <= keysym && keysym <= XK_9) ||
             (XK_A <= keysym && keysym <= XK_F) ||
             (XK_a <= keysym && keysym <= XK_f)) {

	int enc_schema, num_char;
	unsigned char keystroke[KEY_CODE_LEN];
	char code[4];

	if ((keyinfo->keystate & ShiftMask))
	    return IMKEY_SHIFTESC;
	else if ((keyinfo->keystate & ControlMask) || 
		 (keyinfo->keystate & Mod1Mask))
	    return IMKEY_IGNORE;
	else if (len >= KEY_CODE_LEN)
	    return ((cf->beep_mode & INP_MODE_BEEPWRONG)) ?
                        IMKEY_BELL : IMKEY_ABSORB;

        inpinfo->cch_publish.wch = (wchar_t)0;
	keychar = (char)toupper(keyinfo->keystr[0]);

	memset(code, 0, 4);
	if (len) strncpy(keystroke, iccf->keystroke, len);
	keystroke[len] = keychar;
	num_char = zh_hex_compose_char(code, keystroke, len+1);
	if ((enc_schema = IsValidByteSeq(cf->ccode_info.ccode, code, num_char, 
				(len+1) % 2 ? 1 : 0)) == -1)
	    return ((cf->beep_mode & INP_MODE_BEEPWRONG)) ?
                        IMKEY_BELL : IMKEY_ABSORB;

	iccf->keystroke[len] = keychar;
	iccf->keystroke[len+1] = '\0';
	inpinfo->s_keystroke[len].wch = (wchar_t)0;
	inpinfo->s_keystroke[len].s[0] = (char)keychar;
	inpinfo->s_keystroke[len+1].wch = (wchar_t)0;
	len ++;

	if (len < cf->ccode_info.n_ch_encoding[enc_schema] * 2) {
	    inpinfo->keystroke_len ++;
	    return IMKEY_ABSORB;
	}
	else {
	    if ((cch_w.wch = zh_hex_check_char(cf, iccf->keystroke, len))) {
		strncpy(cch_s, (char *)cch_w.s, WCH_SIZE);
		cch_s[WCH_SIZE] = '\0';

	        inpinfo->keystroke_len = 0;
	        inpinfo->s_keystroke[0].wch = (wchar_t)0;
		inpinfo->cch_publish.wch = cch_w.wch;
		inpinfo->cch = cch_s;
	        return IMKEY_COMMIT;
	    }
	    else {
		inpinfo->keystroke_len ++;
		return ((cf->beep_mode & INP_MODE_BEEPWRONG)) ? 
			IMKEY_BELL : IMKEY_ABSORB;
	    }
	}
    }
    else
	return IMKEY_IGNORE;
}

static int 
zh_hex_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    char *s = (char *)simdinfo->cch_publish.s;
    unsigned int i, c, flag1=0x0f, flag2=0xf0;
    static wch_t keystroke_list[KEY_CODE_LEN+1];
 
    if (! match_encoding(&(simdinfo->cch_publish)))
	return False;
    
    for (i=0; i<WCH_SIZE*2 && *s && i<KEY_CODE_LEN; i++) {
	if (i%2 == 0)
	    c = ((unsigned)(*s) & flag2) >> 4;
	else {
	    c = (unsigned)(*s) & flag1;
	    s ++;
	}
	keystroke_list[i].wch = (wchar_t)0;
	keystroke_list[i].s[0] = (char)((c < 10) ? c + '0' : c - 10 + 'A');
    }
    keystroke_list[i].wch = (wchar_t)0;

    if (i) {
	simdinfo->s_keystroke = keystroke_list;
	return True;
    }
    else {
	simdinfo->s_keystroke = NULL;
	return False;
    }
}

/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *zh_hex_valid_objname[] = { "zh_hex", "zh_hex_*", NULL };

static char zh_hex_comments[] = N_(
	"This is the internal zh_encoding input method module.\n"
	"It is the simplist module which demonstrate the basic\n"
	"xcin IM module programming. It also has the ability to\n"
	"get Chinese characters input from different encodings,\n"
	"say, Big5 or GB codes, depending on the locale setting.\n"
	"Currently it can only accept 2-bytes encodings per character.\n\n"
	"This module is free software, as part of xcin system.\n");


module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "zh_hex",					/* name */
      MODULE_VERSION,				/* version */
      zh_hex_comments },			/* comments */
    zh_hex_valid_objname,			/* valid_objname */
    sizeof(zh_hex_conf_t),			/* conf_size */
    zh_hex_init,				/* init */
    zh_hex_xim_init,				/* xim_init */
    zh_hex_xim_end,				/* xim_end */
    zh_hex_keystroke,				/* keystroke */
    zh_hex_show_keystroke,			/* show_keystroke */
    NULL					/* terminate */
};

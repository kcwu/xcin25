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

#include <stdio.h>
#include <string.h>
#include <X11/keysym.h>
#include "module.h"
#include "bimsphone.h"
#include "bims.h"


int
load_pinyin_data(FILE *fp, char *truefn, phone_conf_t *cf)
{
    char modID[MODULE_ID_SIZE];
    pinyin_t pinyin;
    pinpho_t *pinpho, *phopin;
    int n;

    if (fread(modID, sizeof(char), MODULE_ID_SIZE, fp) != MODULE_ID_SIZE ||
	strcmp(modID, "bimscin")) {
	perr(XCINMSG_WARNING, N_("bimsphone: %s: invalid tab file.\n"), truefn);
	return False;
    }
    if (fread(&pinyin, sizeof(pinyin_t), 1, fp)!=1 || ! pinyin.pinno) {
	perr(XCINMSG_WARNING, N_("bimsphone: %s: reading error.\n"), truefn);
	return False;
    }
    n = pinyin.pinno;

    pinpho = malloc(sizeof(pinpho_t) * n);
    phopin = malloc(sizeof(pinpho_t) * n);
    if (fread(pinpho, sizeof(pinpho_t), n, fp) != n ||
	fread(phopin, sizeof(pinpho_t), n, fp) != n) {
	perr(XCINMSG_WARNING, N_("bimsphone: %s: reading error.\n"), truefn);
	free(pinpho);
	free(phopin);
	return False;
    }

    cf->pinyin = calloc(1, sizeof(ipinyin_t));
    cf->pinyin->pinno = n;
    strcpy((char *)cf->pinyin->tone, (char *)pinyin.tone);
    strcpy((char *)cf->pinyin->zhu, (char *)pinyin.zhu);
    strcpy((char *)cf->pinyin->ntone[0].s, 
	   fullchar_keystring(cf->pinyin->tone[0]));
    strcpy((char *)cf->pinyin->ntone[1].s, 
	   fullchar_keystring(cf->pinyin->tone[1]));
    strcpy((char *)cf->pinyin->ntone[2].s, 
	   fullchar_keystring(cf->pinyin->tone[2]));
    strcpy((char *)cf->pinyin->ntone[3].s, 
	   fullchar_keystring(cf->pinyin->tone[3]));
    strcpy((char *)cf->pinyin->ntone[4].s, 
	   fullchar_keystring(cf->pinyin->tone[4]));
    strncpy((char *)cf->pinyin->stone[1].s, (char *)cf->pinyin->zhu+74, 2);
    strncpy((char *)cf->pinyin->stone[2].s, (char *)cf->pinyin->zhu+76, 2);
    strncpy((char *)cf->pinyin->stone[3].s, (char *)cf->pinyin->zhu+78, 2);
    strncpy((char *)cf->pinyin->stone[4].s, (char *)cf->pinyin->zhu+80, 2);
    cf->pinyin->pinpho = pinpho;
    cf->pinyin->phopin = phopin;
    return True; 
}

/*--------------------------------------------------------------------------*/

#define KEYSTROKE_LEN	6
static char zozy_ekey[]="1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-6347";

static unsigned int
encode_pinyin(char *string)
{
    int i, len;
    unsigned int code=0, pin_idx=0;

    len = strlen(string);
    if (len > KEYSTROKE_LEN)
	len = KEYSTROKE_LEN;
    for (i=0; i<len; i++) {
	code = (unsigned int)(string[i] - 'a') + 1;
	if (code < 1 || code > 26)
	    return 0;
	if (i == 0)
	    pin_idx = code;
	else
	    pin_idx = pin_idx*27 + code;
    }
    return pin_idx;
}

static void
decode_pinyin(unsigned int code, char *string, int size)
{
    char keystroke[KEYSTROKE_LEN+1];
    char tmp[KEYSTROKE_LEN+1];
    int i, j;

    for (i=0; i<KEYSTROKE_LEN && code>0; i++) {
	tmp[i] = (char)(code % 27 + 'a' - 1);
	code /= 27;
    }
    for (j=0, i--; i>=0; j++, i--)
	keystroke[j] = tmp[i];
    keystroke[j] = '\0';
    strncpy(string, keystroke, size);
}

static unsigned int
encode_zhuyin(char *string)
{
    int i, j, len;
    unsigned int phone_idx=0;

    len = strlen(string);
    if (len > 3)
        len = 3;
    for (i=0; i<len; i++) {
        for (j=0; j<37; j++) {
	    if (zozy_ekey[j] == string[i])
		break;
        }
        if (j >= 37)
	    continue;
        phone_idx += ((unsigned int)(j+1) << i*8);
    }
    return phone_idx;
}

static void 
decode_zhuyin(unsigned int code, char *string, int size)
{
    char zhuyin[4];
    int i, idx;

    for (i=0; i<3 && code>0; i++) {
	idx = (code & 0xff) - 1;
	zhuyin[i]   = zozy_ekey[idx];
	code = code >> 8;
    }
    zhuyin[i] = '\0';
    strncpy(string, zhuyin, size);
}


static int
pin_cmp(const void *a, const void *b)
{
    pinpho_t *aa = (pinpho_t *)a;
    pinpho_t *bb = (pinpho_t *)b;

    if (aa->pin_idx > bb->pin_idx)
	return  1;
    else if (aa->pin_idx < bb->pin_idx)
	return -1;
    else
	return 0;
}

static int
pho_cmp(const void *a, const void *b)
{
    pinpho_t *aa = (pinpho_t *)a;
    pinpho_t *bb = (pinpho_t *)b;

    if (aa->phone_idx > bb->phone_idx)
	return  1;
    else if (aa->phone_idx < bb->phone_idx)
	return -1;
    else
	return 0;
}


/* 
 *  Return the wchar position that is defined for PINYIN.
 */
static int 
findpinyinmapw(char *keystring, char *key)
{
    int rc=-1;
    char *pos=strstr(keystring, key);
    if (pos) 
	rc = (int)(pos-keystring)/2;
    return rc;
}

char *
pho2pinyinw(ipinyin_t *pf, char *phostring)
{
    static char engwchr[15];
    char str[9], phomap[4];
    char tonechr[2];
    wch_t tmpwchr;
    int i, j, len;
    pinpho_t iphomap, *pinloc;

    strcpy(str, phostring);
    len = strlen(str);

    tmpwchr.wch = (wchar_t)0;
    tmpwchr.s[0] = str[len-2];
    tmpwchr.s[1] = str[len-1];
    tonechr[0] = tonechr[1] = '\0';
    for (i=1; i<5; i++) {
	if (tmpwchr.wch == pf->stone[i].wch)
	    tonechr[0] = pf->tone[i];
    }
    if (tonechr[0] || !strcmp((char *)tmpwchr.s, fullchar_keystring(' '))) {
	len -= 2;
	str[len] = '\0';
    }

    for (i=0;  i<len/2 && i<4; i++) {
	tmpwchr.wch = (wchar_t)0;
	tmpwchr.s[0] = str[i*2];
	tmpwchr.s[1] = str[i*2+1];
	if ((j = findpinyinmapw((char *)pf->zhu, (char *)tmpwchr.s)) != -1)
	    phomap[i] = zozy_ekey[j];
    }
    phomap[i] = '\0';

    iphomap.phone_idx = encode_zhuyin(phomap);
    iphomap.pin_idx = 0;
    if ((pinloc = bsearch(&iphomap, pf->phopin, pf->pinno, 
		sizeof(pinpho_t), pho_cmp)) != NULL) {
	decode_pinyin(pinloc->pin_idx, engwchr, 15);
	if (tonechr[0])
	    strncat(engwchr, tonechr, 15);
	return engwchr;
    }
    else
	return NULL;
}


int
pinyin_keystroke(phone_conf_t *cf, phone_iccf_t *iccf,
		 inpinfo_t *inpinfo, keyinfo_t *keyinfo, int *rval2)
{
    char ch;
    int i, len, tone_idx;
    pinpho_t iphomap, *pinloc;

    *rval2 = IMKEY_IGNORE;
    if (keyinfo->keysym == XK_BackSpace) {
	if (inpinfo->keystroke_len) {
	    inpinfo->keystroke_len --;
	    iccf->pin_keystroke[inpinfo->keystroke_len] = '\0';
	    inpinfo->s_keystroke[inpinfo->keystroke_len].wch = (wchar_t)0;
	    *rval2 = IMKEY_ABSORB;
	    return BC_VAL_IGNORE;
	}
	else
	    return bimsFeedKey(inpinfo->imid, keyinfo->keysym);
    }
    else if (keyinfo->keysym == XK_Escape) {
	inpinfo->keystroke_len = 0;
	iccf->pin_keystroke[0] = '\0';
	inpinfo->s_keystroke[0].wch = (wchar_t)0;
	*rval2 = IMKEY_ABSORB;
	return BC_VAL_IGNORE;
    }
    else if (keyinfo->keystr_len != 1)
	return bimsFeedKey(inpinfo->imid, keyinfo->keysym);;

    ch = keyinfo->keystr[0];
    if (ch == ' ')
	tone_idx = 0;
    else {
	for (tone_idx=0; tone_idx<5; tone_idx++) {
	    if (cf->pinyin->tone[tone_idx] == ch)
		break;
	}
    }
    if (tone_idx < 5 && inpinfo->keystroke_len) {
	iccf->pin_keystroke[inpinfo->keystroke_len] = '\0';

	iphomap.pin_idx = encode_pinyin(iccf->pin_keystroke);
	iphomap.phone_idx = 0;
	if ((pinloc = bsearch(&iphomap, cf->pinyin->pinpho, cf->pinyin->pinno, 
			sizeof(pinpho_t), pin_cmp)) != NULL) {
	    KeySym keysym;
	    char phonemap[4], *zhuyin_str;
	    int rval, zhuyin_exist;

	    decode_zhuyin(pinloc->phone_idx, phonemap, 4);
	    len = strlen(phonemap);
	    for (i=0; i<len && i<3; i++) {
		keysym = keysym_ascii(phonemap[i]);
		bimsFeedKey(inpinfo->imid, keysym);
	    }
	    if (tone_idx == 0)
		rval = bimsFeedKey(inpinfo->imid, XK_space);
	    else {
		keysym = keysym_ascii(zozy_ekey[36+tone_idx]);
		rval = bimsFeedKey(inpinfo->imid, keysym);
	    }

	    zhuyin_str = (char *)bimsQueryZuYinString(inpinfo->imid);
	    zhuyin_exist = (zhuyin_str[0] != '\0') ? 1 : 0;
	    free(zhuyin_str);
	    if (zhuyin_exist) {
		*rval2 = IMKEY_BELL;
		return BC_VAL_ABSORB;
	    }
	    else {
		inpinfo->keystroke_len = 0;
		inpinfo->s_keystroke[0].wch = (wchar_t)0;
		iccf->mode |= ICCF_MODE_COMPOSEDOK;
		return rval;
	    }
	}
	else {
	    *rval2 = IMKEY_BELL;
	    return BC_VAL_IGNORE;
	}
    } 

    else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
	iccf->mode &= ~ICCF_MODE_COMPOSEDOK;
	if (inpinfo->keystroke_len >= N_MAX_KEYCODE_PINYIN) {
	    *rval2 = IMKEY_BELL;
	    return BC_VAL_IGNORE;
	}
	len = inpinfo->keystroke_len;
	iccf->pin_keystroke[len] = ch;
	iccf->pin_keystroke[len+1] = '\0';
	strncpy((char *)inpinfo->s_keystroke[len].s, 
		fullchar_keystring(ch), WCH_SIZE);
	inpinfo->s_keystroke[len+1].wch = (wchar_t)0;
	inpinfo->keystroke_len ++;
	*rval2 = IMKEY_ABSORB;
	return BC_VAL_IGNORE;
    }

    return BC_VAL_IGNORE;
}

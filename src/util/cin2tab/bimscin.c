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
#include "xcintool.h"
#include "bimscin.h"
#include "cin2tab.h"

#define PINPHO_SIZE	1024
#define KEYSTROKE_LEN	6

static pinyin_t pinyin;
static char ph_pho[] = 
  "£t£u£v£w£x£y£z£{£|£}£~£¡£¢£££¤£¥£¦£§£¨£©£ª£¸£¹£º£«£¬£­£®£¯£°£±£²£³£´£µ£¶£·"
  "£½£¾£¿£»";
static pinpho_t *pinpho;
static pinpho_t *phopin;
static int pinpho_size;

/*--------------------------------------------------------------------------*/

static void
encode_keystroke(cintab_t *cintab, pinpho_t *pt, char *string)
{
    int i, len, ch;
    unsigned int code;

    len = strlen(string);
    if (len > KEYSTROKE_LEN) {
	perr(XCINMSG_WARNING, N_("%s(%d): keystroke too long, ignore.\n"),
			cintab->fname_cin, cintab->lineno);
	len = KEYSTROKE_LEN;
    }
    for (i=0; i<len; i++) {
	ch = tolower(string[i]);
	if (ch < (int)'a' || ch > (int)'z')
	    perr(XCINMSG_ERROR, N_("%s(%d): keystroke out of range [a-z].\n"),
			cintab->fname_cin, cintab->lineno);
	code = (unsigned int)(ch - 'a' + 1);
	if (i == 0)
	    pt->pin_idx = code;
	else
	    pt->pin_idx = pt->pin_idx*27 + code;
    }
}

static void
decode_keystroke(unsigned int code, char *string, int size)
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

static void
encode_zhuyin(cintab_t *cintab, pinpho_t *pt, char *string)
{
    int i, j, len;

    len = strlen(string) / 2;
    if (len > 3) {
	perr(XCINMSG_WARNING, N_("%s(%d): ZhuYin string too long, ignore.\n"),
			cintab->fname_cin, cintab->lineno);
	len = 3;
    }
    pt->phone_idx = 0;
    for (i=0; i<len*2; i+=2) {
	for (j=0; j<74; j+=2) {
	    if (ph_pho[j] == string[i] && ph_pho[j+1] == string[i+1])
		break;
	}
	if (j >= 74)
	    perr(XCINMSG_ERROR, N_("%s(%d): unknown ZhuYin character.\n"),
			cintab->fname_cin, cintab->lineno);
	pt->phone_idx += ((unsigned int)(j/2+1) << i*4);
    }
}

static void 
decode_zhuyin(unsigned int code, char *string, int size)
{
    char zhuyin[7];
    int i, idx;

    for (i=0; i<3 && code>0; i++) {
	idx = (code & 0xff) - 1;
	zhuyin[i*2]   = ph_pho[idx*2];
	zhuyin[i*2+1] = ph_pho[idx*2+1];
	code = code >> 8;
    }
    zhuyin[i*2] = '\0';
    strncpy(string, zhuyin, size);
}

static int
pinpho_cmp(const void *a, const void *b)
{
    pinpho_t *aa = (pinpho_t *)a;
    pinpho_t *bb = (pinpho_t *)b;
    char keys[10];

    if (aa->pin_idx > bb->pin_idx)
	return  1;
    else if (aa->pin_idx < bb->pin_idx)
	return -1;
    else {
	decode_keystroke(aa->pin_idx, keys, 10);
	perr(XCINMSG_ERROR, N_("duplicated keystroke: %s\n"), keys);
	return 0;
    }
}

static int
phopin_cmp(const void *a, const void *b)
{
    pinpho_t *aa = (pinpho_t *)a;
    pinpho_t *bb = (pinpho_t *)b;
    char zhuyin[10], keys1[10], keys2[10];

    if (aa->phone_idx > bb->phone_idx)
	return  1;
    else if (aa->phone_idx < bb->phone_idx)
	return -1;
    else {
	decode_zhuyin(aa->phone_idx, zhuyin, 10);
	decode_keystroke(aa->pin_idx, keys1, 10);
	decode_keystroke(bb->pin_idx, keys2, 10);
	perr(XCINMSG_WARNING, N_("duplicated ZhuYin code: %s: %s, %s\n"),
			zhuyin, keys1, keys2);
	return 0;
    }
}

static void
read_yinmap(cintab_t *cintab)
{
    char cmd[64], arg[64];

    pinpho_size = PINPHO_SIZE;
    pinpho = xcin_malloc(pinpho_size * sizeof(pinpho_t), 0);

    while (cmd_arg(cmd, 64, arg, 64, NULL)) {
	if (pinyin.pinno >= pinpho_size) {
	    pinpho_size += PINPHO_SIZE;
	    pinpho = xcin_realloc(pinpho, pinpho_size * sizeof(pinpho_t));
	}
	if (! strcmp(cmd, "%yinmap")) {
	    if (! strcmp(arg, "end"))
		break;
	    else
		perr(XCINMSG_ERROR, N_("%s(%d): token \"end\" expected.\n"),
			cintab->fname_cin, cintab->lineno);
	}
	encode_keystroke(cintab, pinpho+pinyin.pinno, cmd);
	encode_zhuyin(cintab, pinpho+pinyin.pinno, arg);
	pinyin.pinno ++;
    }
    phopin = xcin_malloc(pinyin.pinno * sizeof(pinpho_t), 0);
    memcpy(phopin, pinpho, pinyin.pinno * sizeof(pinpho_t));

    stable_sort(pinpho, pinyin.pinno, sizeof(pinpho_t), pinpho_cmp);
    stable_sort(phopin, pinyin.pinno, sizeof(pinpho_t), phopin_cmp);
}

void
bimscin(cintab_t *cintab)
{
    char cmd[64], arg[64];

    strncpy(pinyin.version, BIMSCIN_VERSION, 10);
    strncpy((char *)pinyin.zhu, ph_pho, 83);

    memset(pinyin.tone, ' ', 5);
    pinyin.tone[5] = '\0';
    while (cmd_arg(cmd, 64, arg, 64, NULL)) {
	if (! strncmp("%tone", cmd, 5))
	    pinyin.tone[(cmd[5]-'1')] = arg[0];

	else if (! strcmp("%yinmap", cmd)) {
	    if (strcmp("begin", arg))
		perr(XCINMSG_ERROR, N_("%s(%d): token \"begin\" expected.\n"),
			cintab->fname_cin, cintab->lineno);
	    read_yinmap(cintab);
	}
	else
	    perr(XCINMSG_ERROR, N_("%s(%d): unknown command: %s\n"),
			cintab->fname_cin, cintab->lineno, cmd);
    }
    perr(XCINMSG_NORMAL, N_("number of pinpho added: %d\n"), pinyin.pinno);

    fwrite(&pinyin, sizeof(pinyin_t), 1, cintab->fw);
    fwrite(pinpho, sizeof(pinpho_t), pinyin.pinno, cintab->fw);
    fwrite(phopin, sizeof(pinpho_t), pinyin.pinno, cintab->fw);
}

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
#include "module.h"

#define N_KEYS  47

static char keymap[128];
static char *ichmap = " 1234567890abcdefghijklmnopqrstuvwxyz`-=[];',./\\";

static void
keymap_init(void)
{
    keymap['1'] = 1;
    keymap['2'] = 2;
    keymap['3'] = 3;
    keymap['4'] = 4;
    keymap['5'] = 5;
    keymap['6'] = 6;
    keymap['7'] = 7;
    keymap['8'] = 8;
    keymap['9'] = 9;
    keymap['0'] = 10;

    keymap['!'] = 1;
    keymap['@'] = 2;
    keymap['#'] = 3;
    keymap['$'] = 4;
    keymap['%'] = 5;
    keymap['^'] = 6;
    keymap['&'] = 7;
    keymap['*'] = 8;
    keymap['('] = 9;
    keymap[')'] = 10;

    keymap['a'] = 11;
    keymap['b'] = 12;
    keymap['c'] = 13;
    keymap['d'] = 14;
    keymap['e'] = 15;
    keymap['f'] = 16;
    keymap['g'] = 17;
    keymap['h'] = 18;
    keymap['i'] = 19;
    keymap['j'] = 20;
    keymap['k'] = 21;
    keymap['l'] = 22;
    keymap['m'] = 23;
    keymap['n'] = 24;
    keymap['o'] = 25;
    keymap['p'] = 26;
    keymap['q'] = 27;
    keymap['r'] = 28;
    keymap['s'] = 29;
    keymap['t'] = 30;
    keymap['u'] = 31;
    keymap['v'] = 32;
    keymap['w'] = 33;
    keymap['x'] = 34;
    keymap['y'] = 35;
    keymap['z'] = 36;

    keymap['A'] = 11;
    keymap['B'] = 12;
    keymap['C'] = 13;
    keymap['D'] = 14;
    keymap['E'] = 15;
    keymap['F'] = 16;
    keymap['G'] = 17;
    keymap['H'] = 18;
    keymap['I'] = 19;
    keymap['J'] = 20;
    keymap['K'] = 21;
    keymap['L'] = 22;
    keymap['M'] = 23;
    keymap['N'] = 24;
    keymap['O'] = 25;
    keymap['P'] = 26;
    keymap['Q'] = 27;
    keymap['R'] = 28;
    keymap['S'] = 29;
    keymap['T'] = 30;
    keymap['U'] = 31;
    keymap['V'] = 32;
    keymap['W'] = 33;
    keymap['X'] = 34;
    keymap['Y'] = 35;
    keymap['Z'] = 36;

    keymap['`'] = 37;
    keymap['-'] = 38;
    keymap['='] = 39;
    keymap['['] = 40;
    keymap[']'] = 41;
    keymap[';'] = 42;
    keymap['\''] = 43;
    keymap[','] = 44;
    keymap['.'] = 45;
    keymap['/'] = 46;
    keymap['\\'] = 47;

    keymap['~'] = 37;
    keymap['_'] = 38;
    keymap['+'] = 39;
    keymap['{'] = 40;
    keymap['}'] = 41;
    keymap[':'] = 42;
    keymap['\"'] = 43;
    keymap['<'] = 44;
    keymap['>'] = 45;
    keymap['?'] = 46;
    keymap['|'] = 47;
}


/*------------------------------------------------------------------------*/

int
key2code(int key)
{
    if (! keymap['1'])
	keymap_init();
    return (key < 128) ? keymap[key] : 0;
}

int
code2key(int code)
{
    return (code <= N_KEYS && code > 0) ? ichmap[code] : 0;
}

#define N_KEY_IN_CODE 5		/* Number of keys in code. */

int
keys2codes(unsigned int *klist, int klist_size, char *keystroke)
{
    int i, j;
    unsigned int k, *kk=klist;
    char *kc = keystroke;

    if (! keymap['1'])
	keymap_init();

    *kk = 0;
    for (i=0, j=0; *kc; i++, kc++) {
        k = keymap[(int) *kc];
        *kk |= (k << (24 - (i - j*N_KEY_IN_CODE) * 6));
	
        if (i % N_KEY_IN_CODE == N_KEY_IN_CODE-1) {
	    if (++j >= klist_size)
		break;
	    kk++;
	    *kk = 0;
        }
    }
    return j;
}

void
codes2keys(unsigned int *klist, int n_klist, char *keystroke, int keystroke_len)
{
    int i, j, n_ch=0, shift;
    unsigned int mask = 0x3f, idx;    
    char *s;

    for (j=0; j<n_klist; j++) {
	for (i=0; i<N_KEY_IN_CODE; i++) {
	    shift = 24 - i * 6;
	    if (n_ch < keystroke_len-1) {
	        idx = (klist[j] & (mask << shift)) >> shift;
		keystroke[n_ch] = ichmap[idx];
		n_ch ++;
	    }
	    else
		break;
	}
    }
    keystroke[n_ch] = '\0';

    if ((s = strchr(keystroke, ' ')))
	*s = '\0';
}


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

#include <stdlib.h>
#include <string.h>
#include "xcintool.h"

static int
on_or_off(char *s)
{
    if (! strcasecmp(s, "on") || ! strcasecmp(s, "yes") ||
	! strcasecmp(s, "true"))
        return 1;
    else if (! strcasecmp(s, "off") || ! strcasecmp(s, "no") ||
	     ! strcasecmp(s, "false"))
        return 0;
    else
	return -1;
}

void
set_data(void *ref, int type, char *value, unsigned long flag_mask, int bufsize)
{
    switch (type) {
    case (RC_BFLAG):
        if (on_or_off(value))
            *(unsigned char *)ref |= (unsigned char)flag_mask;
        else
            *(unsigned char *)ref &= (~(unsigned char)flag_mask);
	break;
	
    case (RC_SFLAG):
        if (on_or_off(value))
            *(unsigned short *)ref |= (unsigned short)flag_mask;
        else
            *(unsigned short *)ref &= (~(unsigned short)flag_mask);
	break;
	
    case (RC_IFLAG):
        if (on_or_off(value))
            *(unsigned int *)ref |= (unsigned int)flag_mask;
        else
            *(unsigned int *)ref &= (~(unsigned int)flag_mask);
	break;
	
    case (RC_LFLAG):
        if (on_or_off(value))
            *(unsigned long *)ref |= flag_mask;
        else
            *(unsigned long *)ref &= (~flag_mask);
	break;
	
    case (RC_BYTE):
	*(char *)ref = (char)atol(value);
	break;
	
    case (RC_UBYTE):
	*(unsigned char *)ref = (unsigned char)strtoul(value, NULL, 10);
	break;
	
    case (RC_SHORT):
	*(short *)ref = (short)atol(value);
	break;
	
    case (RC_USHORT):
	*(unsigned short *)ref = (unsigned short)strtoul(value, NULL, 10);
	break;
	
    case (RC_INT):
	*(int *)ref = (int)atol(value);
	break;
	
    case (RC_UINT):
	*(unsigned int *)ref = (unsigned int)strtoul(value, NULL, 10);
	break;
	
    case (RC_LONG):
	*(long *)ref = (long)atol(value);
	break;
	
    case (RC_ULONG):
	*(unsigned long *)ref = (unsigned long)strtoul(value, NULL, 10);
	break;
	
    case (RC_FLOAT):
	*(float *)ref = (float)atof(value);
	break;
	
    case (RC_DOUBLE):
	*(double *)ref = (double)atof(value);
	break;
	
    case (RC_STRING):
	*(char **)ref = (char *)strdup(value);
	break;
	
    case (RC_STRARR):
	strncpy((char *)ref, value, bufsize);
	break;

    case (RC_NONE):
	break;
	
    default:
	perr(XCINMSG_IERROR, _("set_rc(): unknown rctype: %d.\n"), type);
    }
}




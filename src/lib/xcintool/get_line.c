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
#include "xcintool.h"

int
get_line(char *str, int str_size, FILE *f, int *lineno, char *ignore_ch)
/*   str=buffer,          str_size=buffer_size, 
 *  *lineno=line_#_of_f,  ignore_ch=ignore ch's 
 */
{
    char *s, *s_ignor;

    str[0] = '\0';
    while (!feof(f)) {
        fgets(str, str_size, f);
        (*lineno)++;

        for (s_ignor = ignore_ch; *s_ignor != '\0'; s_ignor++) {
            if ((s = strchr(str, *s_ignor)) != NULL)
                *s = '\0';
        }
        if (str[0] != '\0')
            return 1;
    }
    return 0;
}

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
#include "xcintool.h"

void
copy_file(char *fn1, char *fn2, int exitcode)
{
    FILE *f1, *f2;
    char str[4096];
    int n_read;

    f1 = open_file(fn1, "rb", exitcode);
    f2 = open_file(fn2, "wb", exitcode);

    while ((n_read = fread(str, sizeof(char), 4096, f1)) == 4096)
        fwrite(str, sizeof(char), n_read, f2);
    fwrite(str, sizeof(char), n_read, f2);

    fclose(f1);
    fclose(f2);
}


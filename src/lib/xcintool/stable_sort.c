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

/*--------------------------------------------------------------------------

    This sorting function is not trivial :-)), since xcin-2.5/cin2tab 
    needs a "stable_sort", which requires that the sorting result will
    preserve the order of the same valued objects. Even that every UNIX
    system provides their standard sorting function, but not all of them
    are "stable". For example, the qsort() in Linux glibc-2.X is stable,
    but in FreeBSD it is not. Instead, FreeBSD provides the "mergesort"
    to be the stable sorting function.

    Therefore, the choice of the sorting function becomes system dependent,
    and should be very careful. So we provide a stable_sort() function
    defined in xcintool.h. If the autoconf cannot find a "stable" sorting
    function in the system library, then the "my_merge_sort()" here will
    be linked as the stable_sort(). This is a simple sorting function using
    the merge sort algorithm. It is proved that both the merge sort and
    insertion sort algorithms are "stable_sort", but the performace are very
    different between them: merge sort == O(n log n); while insertion sort
    == O(n^2).

    By Tung-Han Hsieh
--------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef HAVE_MERGESORT
#include <stdlib.h>
#include <string.h>
#include "xcintool.h"

static char *buf;

static void
swap(char *elm1, char *elm2, size_t size)
{
    memcpy(buf, elm1, size);
    memcpy(elm1, elm2, size);
    memcpy(elm2, buf, size);
}

static void
merge(char *base1, size_t nmemb1, char *base2, size_t nmemb2, size_t size, 
        int (*compar)(const void *, const void *))
{
    char *b1=base1, *b2=base2, *b3=buf;
    size_t i1=0, i2=0, i3=0;

    while (i1<nmemb1 && i2<nmemb2) {
        if (compar(b1, b2) > 0) {
            memcpy(b3, b2, size);
            b2 += size;
            i2 ++;
        }
        else {
            memcpy(b3, b1, size);
            b1 += size;
            i1 ++;
        }
        b3 += size;
        i3 ++;
    }
    if (i1 < nmemb1)
        memcpy(b3, b1, size*(nmemb1-i1));
    else if (i2 < nmemb2)
        memcpy(b3, b2, size*(nmemb2-i2));
    memcpy(base1, buf, size*(nmemb1+nmemb2));
}

static void
separate(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    if (nmemb == 1)
        return;

    else if (nmemb == 2) {
        char *b = (char *)base;
        if (compar(b, b+size) > 0)
            swap(b, b+size, size);
    }
    else {
        size_t nmemb1, nmemb2;
        char  *b1, *b2;

        nmemb1 = nmemb / 2;
        nmemb2 = nmemb - nmemb1;
        b1 = (char *)base;
        b2 = b1 + nmemb1 * size;
        separate((void *)b1, nmemb1, size, compar);
        separate((void *)b2, nmemb2, size, compar);
        merge(b1, nmemb1, b2, nmemb2, size, compar);
    }
}

void
xcin_mergesort(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    buf = xcin_malloc(nmemb*size, 0);    
    separate(base, nmemb, size, compar);
    free(buf);
}
#endif

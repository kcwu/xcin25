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

/*
    This is a simple implimentation of snprintf() function. It will try
    to estimate the needed buffer size and allocate the needed buffer
    before actually print strings, such that the overflow will never
    occure. This function may be buggy, without the fully implimentation
    of the true snprintf() function, and it only serves the purpose that
    the snprintf() is not available in the operating system. If snprintf()
    is already available in the operating system, we should use it anyway.

    T.H.Hsieh
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef HAVE_SNPRINTF
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "xcintool.h"

#ifdef HAVE_LONG_DOUBLE
typedef long double	ldouble_t;
#else
typedef double		ldouble_t;
#endif

enum int_kind {
    KIND_UINT,			/* normal unsigned int */
    KIND_UOINT,			/* %o: unsigned octal int */
    KIND_UXINT,			/* %x, %p: unsigned hex "abcdef" int */
    KIND_UXXINT			/* %X: unsigned hex "ABCDEF" int */
};

enum double_kind {
    KIND_EDB,			/* %e: exponent "e" double */
    KIND_EEDB,			/* %E: exponent "E" double */
    KIND_FDB,			/* %f, %F: double */
    KIND_GDB,			/* %g: double */
    KIND_GGDB,			/* %G: double */
};

enum format_type {
    TYPE_INT,			/* signed int */
    TYPE_UINT,			/* unsigned int */
    TYPE_DOUBLE,		/* double */
    TYPE_CHAR,			/* char */
    TYPE_STRING,		/* string */
    TYPE_PERCENT,		/* '%' character */
    TYPE_DEFAULT		/* default characters */
};

enum length_mode {
    LEN_DEFAULT=0,
    LEN_CHAR,			/* hh int */
    LEN_SHORT,			/* h  int */
    LEN_LONG,			/* l  int */
    LEN_LLONG,			/* ll int */
    LEN_LDB			/* L  double */
};

typedef struct {
    int type, kind;
    int alternative, zero_padded, align_boundary, sign;
    int precision[2], query[3];			/* query start from 1 */
    int length_modifier;
} format_t;

/*----------------------------------------------------------------------------

	Format fields analysis functions.

----------------------------------------------------------------------------*/

static int
determine_field(int fc, int format_field)
{
    int current_field;

    switch (fc) {
    case '#':				/* header field */
    case '-':
    case '+':
	current_field = 0;
	break;
    case '.':				/* precision field */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '*':
    case '$':
	current_field = 1;
	break;
    case 'h':				/* type modifier field */
    case 'l':
    case 'L':
	current_field = 2;
	break;
    default:				/* error */
	current_field = format_field;
	break;
    }
    if (format_field > current_field)
	return -1;
    else
	return current_field;
}

static void
field0_analy(char **format, format_t *fmt)
{
    while (**format != '\0') {
	switch (**format) {
	case '#':
	    fmt->alternative = 1;
	    break;
	case '-':
	    fmt->align_boundary = 1;
	    break;
	case '+':
	    fmt->sign = 1;
	    break;
	default:
	    return;
	}
	(*format) ++;
    }
}

#define QFIELD_ARG	1
#define QFIELD_PRE1	2
#define QFIELD_PRE2	3

static void
field1_analy(char **format, format_t *fmt)
{
    int i, number=0, stop=0, status=QFIELD_ARG, p_field=0, seq=1, zero_idx;
    int query_sequence[3]={0}, query_class[3]={0,0,0};
    char *endptr;

    if (**format == 0) {
	fmt->zero_padded = 1;
	(*format) ++;
    }
    while (**format != '\0' && stop == 0) {
	if (isdigit(**format)) {	/* get integer number */
	    number = (int)strtol(*format, &endptr, 10);
	    *format = endptr;
	}
	else if (**format == '$') {	/* determine order exactly */
	    if (number > 0 && number <= 3 && query_sequence[number-1] == 0) {
		query_sequence[number-1] = status;
		query_class[status-1] = number;
		number = 0;
	    }
	    (*format) ++;
	    if (**format == '$')
		return;
	}
	else if (**format == '*') {	/* determine order via sequence */
	    status = (p_field == 0) ? QFIELD_PRE1 : QFIELD_PRE2;
	    if (seq <= 3) {
		query_class[status-1] = -seq;
		seq ++;
	    }
	    (*format) ++;
	    if (**format == '$' || **format == '*')
		return;
	}
	else if (**format == '.') {	/* shift precision field */
	    if (p_field == 0) {
		if (number != 0) {
		    fmt->precision[0] = number;
		    number = 0;
		}
		p_field ++;
		(*format) ++;
	    }
	    else
		return;
	}
	else {				/* stop the loop */
	    if (number != 0) {
		fmt->precision[p_field] = number;
		number = 0;
	    }
	    stop = 1;
	    break;
	}
    }
    for (i=0; i<3; i++) {
	if (query_class[i] < 0) {	/* expected to be queried but not set */
	    zero_idx = -query_class[i]-1;
	    if (query_sequence[zero_idx] == 0)
		query_sequence[zero_idx] = i+1;
	}
    }
    fmt->query[0] = query_sequence[0];
    fmt->query[1] = query_sequence[1];
    fmt->query[2] = query_sequence[2];

    zero_idx = -1;
    for (i=0; i<3 && fmt->query[i]!=QFIELD_ARG; i++) {
	if (fmt->query[i] == 0 && zero_idx == -1)
	    zero_idx = i;
    }
    if (i >= 3 && zero_idx != -1)
	fmt->query[zero_idx] = QFIELD_ARG;
}

static void
field2_analy(char **format, format_t *fmt)
{
    switch (**format) {
    case 'h':
	(*format) ++;
	if (**format == 'h') {
	    fmt->length_modifier = LEN_CHAR;
	    (*format) ++;
	}
	else
	    fmt->length_modifier = LEN_SHORT;
	break;
    case 'l':
	(*format) ++;
	if (**format == 'l') {
	    fmt->length_modifier = LEN_LLONG;
	    (*format) ++;
	}
	else
	    fmt->length_modifier = LEN_LONG;
	break;
    case 'L':
	(*format) ++;
	fmt->length_modifier = LEN_LDB;
	break;
    default:
	break;
    }
}

/*----------------------------------------------------------------------------

	Format analysis functions.

----------------------------------------------------------------------------*/

static int
analy_format(char **format, format_t *fmt)
{
    char *s;
    int format_field=0;

    if (**format == '\0')
	return 1;

    memset(fmt, 0, sizeof(format_t));
    if (**format != '%') {
	fmt->type = TYPE_DEFAULT;
	s = *format;
	while (*s != '\0' && *s != '%') {
	    s ++;
	    fmt->precision[0] ++;
	}
    }
    else {
	(*format) ++;
	if (**format != '\0' && determine_field(**format, format_field) != -1)
	    field0_analy(format, fmt);
	format_field ++;
	if (**format != '\0' && determine_field(**format, format_field) != -1)
	    field1_analy(format, fmt);
	format_field ++;
	if (**format != '\0' && determine_field(**format, format_field) != -1)
	    field2_analy(format, fmt);
	if (*format == '\0')
	    return -1;

	switch (**format) {
	case 'd':
	case 'i':
	    fmt->type = TYPE_INT;
	    break;
	case 'o':
	    fmt->type = TYPE_UINT;
	    fmt->kind = KIND_UOINT;
	    break;
	case 'u':
	    fmt->type = TYPE_UINT;
	    fmt->kind = KIND_UINT;
	    break;
	case 'x':
	    fmt->type = TYPE_UINT;
	    fmt->kind = KIND_UXINT;
	    break;
	case 'X':
	    fmt->type = TYPE_UINT;
	    fmt->kind = KIND_UXXINT;
	    break;
	case 'p':
	    fmt->type = TYPE_UINT;
	    fmt->kind = KIND_UXINT;
	    fmt->alternative = 1;
	    fmt->length_modifier = LEN_LONG;
	    break;
	case 'e':
	    fmt->type = TYPE_DOUBLE;
	    fmt->kind = KIND_EDB;
	    break;
	case 'E':
	    fmt->type = TYPE_DOUBLE;
	    fmt->kind = KIND_EEDB;
	    break;
	case 'f':
	case 'F':
	    fmt->type = TYPE_DOUBLE;
	    fmt->kind = KIND_FDB;
	    break;
	case 'g':
	    fmt->type = TYPE_DOUBLE;
	    fmt->kind = KIND_GDB;
	    break;
	case 'G':
	    fmt->type = TYPE_DOUBLE;
	    fmt->kind = KIND_GGDB;
	    break;
	case 'c':
	    fmt->type = TYPE_CHAR;
	    break;
	case 's':
	    fmt->type = TYPE_STRING;
	    break;
	case '%':
	    fmt->type = TYPE_PERCENT;
	    break;
	default:
	    return -1;
	}
	(*format) ++;
    }
    return 0;
}

/*----------------------------------------------------------------------------

	Extract va_list functions.

----------------------------------------------------------------------------*/

static void *
get_va_list(va_list *ap, int type, int length)
{
    void *var_pt;

    char		*var_string;
    char		var_char;
    unsigned char	var_uchar;
    short		var_short;
    unsigned short	var_ushort;
    int			var_int;
    unsigned int	var_uint;
    long		var_long;
    unsigned long	var_ulong;
    long long		var_llong;
    unsigned long long	var_ullong;
    double		var_db;
    ldouble_t		var_ldb;

    switch (type) {
    case TYPE_INT:
	switch (length) {
	case LEN_CHAR:
	    var_char = va_arg((*ap), char);
	    var_pt = (void *)&var_char;
	    break;
	case LEN_SHORT:
	    var_short = va_arg((*ap), short);
	    var_pt = (void *)&var_short;
	    break;
	case LEN_LONG:
	    var_long = va_arg((*ap), long);
	    var_pt = (void *)&var_long;
	    break;
	case LEN_LLONG:
	    var_llong = va_arg((*ap), long long);
	    var_pt = (void *)&var_llong;
	    break;
	default:
	    var_int = va_arg((*ap), int);
	    var_pt = (void *)&var_int;
	    break;
	}
	break;

    case TYPE_UINT:
	switch (length) {
	case LEN_CHAR:
	    var_uchar = va_arg((*ap), unsigned char);
	    var_pt = (void *)&var_uchar;
	    break;
	case LEN_SHORT:
	    var_ushort = va_arg((*ap), unsigned short);
	    var_pt = (void *)&var_ushort;
	    break;
	case LEN_LONG:
	    var_ulong = va_arg((*ap), unsigned long);
	    var_pt = (void *)&var_ulong;
	    break;
	case LEN_LLONG:
	    var_ullong = va_arg((*ap), unsigned long long);
	    var_pt = (void *)&var_ullong;
	    break;
	default:
	    var_uint = va_arg((*ap), uint);
	    var_pt = (void *)&var_uint;
	    break;
	}
	break;

    case TYPE_DOUBLE:
	switch (length) {
#ifdef HAVE_LONG_DOUBLE
	case LEN_LDB:
	    var_ldb = va_arg((*ap), ldouble_t);
	    var_pt = (void *)&var_ldb;
	    break;
#endif
	default:
	    var_db = va_arg((*ap), double);
	    var_pt = (void *)&var_db;
	    break;
	}
	break;

    case TYPE_CHAR:
	var_char = va_arg((*ap), char);
	var_pt = (void *)&var_char;
	break;

    case TYPE_STRING:
	var_string = va_arg((*ap), char *);
	var_pt = (void *)var_string;
	break;

    default:
	return NULL;
    }
    return var_pt;
}

static void
get_query(format_t *fmt, va_list *ap, void **arg)
{
    int i, imax, arg_idx, *int_pt;

    *arg = NULL;
    for (imax=2; imax>=0 && fmt->query[imax]==0; imax--);

    for (i=0; i<=imax; i++) {
	switch (fmt->query[i]) {
	case QFIELD_ARG:
	    *arg = get_va_list(ap, fmt->type, fmt->length_modifier);
	    break;
	case QFIELD_PRE1:
	    int_pt = (int *)get_va_list(ap, TYPE_INT, LEN_DEFAULT);
	    fmt->precision[0] = *int_pt;
	    break;
	case QFIELD_PRE2:
	    int_pt = (int *)get_va_list(ap, TYPE_INT, LEN_DEFAULT);
	    fmt->precision[1] = *int_pt;
	    break;
	default:				/* dummy case, just skip */
	    int_pt = (int *)get_va_list(ap, TYPE_INT, LEN_DEFAULT);
	    break;
	}
    }
}

/*----------------------------------------------------------------------------

	Format handling functions.

----------------------------------------------------------------------------*/

static int
get_exp(ldouble_t ff)
{
    int expff=0;
#ifdef HAVE_LONG_DOUBLE
    const ldouble_t dzer=0.00L;
    const ldouble_t done=1.00L;
    const ldouble_t dten=10.0L;
    const ldouble_t ddon=0.10L;
#else
    const double dzer=0.00;
    const double done=1.00;
    const double dten=10.0;
    const double ddon=0.10;
#endif

    if (ff < dzer)
	ff = -ff;
    while (ff > done) {
        ff = ff * ddon;
        expff ++;
    }
    while (ff < ddon) {
        ff = ff * dten;
        expff --;
    }
    return expff;
}

static int
handle_integer(format_t *fmt, va_list *ap)
{
    void *var;

    get_query(fmt, ap, var);
    return 50;
}

static int
handle_double(format_t *fmt, va_list *ap)
{
    double *var_db;
    ldouble_t *var_ldb, var;
    int default_len, power;

    if (fmt->length_modifier == LEN_DEFAULT) {
	get_query(fmt, ap, (void **)&var_db);
	var = (ldouble_t)(*var_db);
    }
    else {
	get_query(fmt, ap, (void **)&var_ldb);
	var = *var_ldb;
    }

    switch (fmt->kind) {
    case KIND_EDB:		/* default: [-]X.YYYYYYe+ZZZ */
    case KIND_EEDB:
    case KIND_GDB:
    case KIND_GGDB:
	if (fmt->precision[1] == 0)
	    fmt->precision[1] = 6;    /*  d   .   e+XXX */
	default_len = fmt->precision[1] + 1 + 1 + 5;
	if (fmt->precision[0] == 0 || fmt->precision[0] < default_len)
	    fmt->precision[0] = default_len;
	break;
    case KIND_FDB:
	power = get_exp(var);
	if (power < 0)
	    power = 0;
	if (fmt->precision[1] == 0)
	    fmt->precision[1] = 6;
	if (fmt->precision[0] == 0)
	    fmt->precision[0] = fmt->precision[1] + 2 + power;
	break;
    default:
	break;
    }
    fmt->precision[0] += 1;
    return fmt->precision[0];
}

static int
handle_string(format_t *fmt, va_list *ap, int mode)
{
    char *s;
    int slen, print_len;

    get_query(fmt, ap, (void **)&s);
    if (s == NULL)
	return 0;
    slen = (mode == 0) ? strlen(s) : 1;
    return (fmt->precision[0] == 0) ? slen : fmt->precision[0];
}

/*----------------------------------------------------------------------------

	XCIN snprintf() main program.

----------------------------------------------------------------------------*/

int
xcin_snprintf(char *str, size_t size, char *format, ...)
{
    va_list ap;
    int slen=0, slen2;
    char *format_str = format, *buf;
    format_t fmt;

    va_start(ap, format);
    while (*format != '\0') {
	if (analy_format(&format_str, &fmt) != 0)
	    break;

	switch (fmt.type) {
	case TYPE_INT:
	    slen += handle_integer(&fmt, &ap);
	    break;
	case TYPE_UINT:
	    slen += handle_integer(&fmt, &ap);
	    break;
	case TYPE_DOUBLE:
	    slen += handle_double(&fmt, &ap);
	    break;
	case TYPE_CHAR:
	    slen += handle_string(&fmt, &ap, 1);
	    break;
	case TYPE_STRING:
	    slen += handle_string(&fmt, &ap, 0);
	    break;
	case TYPE_PERCENT:
	    slen ++;
	    break;
	default:
	    slen += fmt.precision[0];
	    format_str += fmt.precision[0];
	    break;
	}
    }
    va_end(ap);
    slen += 1;

    buf = xcin_malloc(slen);
    va_start(ap, format);
    slen2 = vsprintf(buf, format, ap);
    va_end(ap);
    if (slen2 > slen)
	perr(XCINMSG_IERROR, "xcin_printf(): slen=%d, slen2=%d\n", slen, slen2);
    strncpy(str, buf, size);
    free(buf);

    return slen;
}
#endif

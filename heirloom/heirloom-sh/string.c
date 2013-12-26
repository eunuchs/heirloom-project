/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)string.c	1.5 (gritter) 6/16/05
 */

/* from OpenSolaris "string.c	1.11	05/06/08 SMI"	 SVr4.0 1.8.2.1 */
/*
 * UNIX shell
 */

#include	"defs.h"


/* ========	general purpose string handling ======== */


unsigned char *
movstr(register const unsigned char *a, register unsigned char *b)
{
	while (*b++ = *a++);
	return(--b);
}

int
any(wchar_t c, const unsigned char *s)
{
	register unsigned int d;

	while (d = *s++)
	{
		if (d == c)
			return(TRUE);
	}
	return(FALSE);
}

int 
anys(const unsigned char *c, const unsigned char *s)
{
	wchar_t f, e;
	register wchar_t d;
	register int n;
	if((n = nextc(&f, (const char *)c)) <= 0)
		return(FALSE);
	d = f;
	while(1) {
		if((n = nextc(&e, (const char *)s)) <= 0)
			return(FALSE);
		if(d == e)
			return(TRUE);
		s += n;
	}
}

int 
cf(register const unsigned char *s1, register const unsigned char *s2)
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return(0);
	return(*--s1 - *s2);
}

int 
length(const unsigned char *as)
{
	register const unsigned char	*s;

	if (s = as)
		while (*s++);
	return(s - as);
}

unsigned char *
movstrn(register const unsigned char *a,
		register unsigned char *b, register int n)
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}

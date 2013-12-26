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
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	from OpenSolaris "allprint.c	6.11	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)allprint.c	1.4 (gritter) 11/27/05
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef	__sun
#include <sys/euc.h>
#include <widec.h>
#endif
#include <wctype.h>
#include <wchar.h>

extern FILE *yyout;

#ifndef JLSLEX
#define	CHR    char
#endif

#ifdef WOPTION
#define	CHR	wchar_t
#define	sprint	sprint_w
#define	allprint	allprint_w
#endif

#ifdef EOPTION
#define	CHR	wchar_t
#endif

void
allprint(CHR c)
{
	switch (c) {
	case '\n':
		fprintf(yyout, "\\n");
		break;
	case '\t':
		fprintf(yyout, "\\t");
		break;
	case '\b':
		fprintf(yyout, "\\b");
		break;
	case ' ':
		fprintf(yyout, "\\_");
		break;
	default:
		if (!iswprint(c))
		    fprintf(yyout, "\\x%-2x", (int)c);
		else
		    putwc(c, yyout);
		break;
	}
}

void
sprint(CHR *s)
{
	while (*s)
		allprint(*s++);
}

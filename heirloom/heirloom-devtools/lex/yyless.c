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
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "yyless.c	6.14	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)yyless.c	1.6 (gritter) 11/27/05
 */

#include <stdlib.h>
#ifdef	__sun
#include <sys/euc.h>
#include <widec.h>
#endif
#include <limits.h>
#include <inttypes.h>
#include <unistd.h>

extern int yyprevious;

#ifndef JLSLEX
#define	CHR    char

extern CHR yytext[];

#define	YYTEXT yytext
#define	YYLENG yyleng
#define	YYINPUT yyinput
#define	YYUNPUT yyunput
#define	YYOUTPUT yyoutput
#endif

#ifdef WOPTION
#define	CHR    wchar_t

extern CHR yytext[];

#define	YYTEXT yytext
#define	YYLENG yyleng
#define	YYINPUT yyinput
#define	YYUNPUT yyunput
#define	YYOUTPUT yyoutput
#define yyless yyless_w
#endif

#ifdef EOPTION
#define	CHR    wchar_t

extern int yyleng;
extern CHR yytext[];
extern CHR yywtext[];

#define	YYTEXT yywtext
#define	YYLENG yywleng
#define	YYINPUT yywinput
#define	YYUNPUT yywunput
#define	YYOUTPUT yywoutput
#define yyless yyless_e
#endif

extern int YYLENG;
#if defined(__STDC__)
    extern void YYUNPUT(int);
#endif

#if defined(__cplusplus) || defined(__STDC__)
/* XCU4: type of yyless() changes to int */
int
yyless(int x)
#else
yyless(x)
int x;
#endif
{
	register CHR *lastch, *ptr;

	lastch = YYTEXT+YYLENG;
	if (x >= 0 && x <= YYLENG)
		ptr = x + YYTEXT;
	else {
		if (sizeof (int) != sizeof (intptr_t)) {
			static int seen = 0;

			if (!seen) {
				write(2,
				"warning: yyless pointer arg truncated\n", 39);
				seen = 1;
			}
		}
	/*
	 * The cast on the next line papers over an unconscionable nonportable
	 * glitch to allow the caller to hand the function a pointer instead of
	 * an integer and hope that it gets figured out properly.  But it's
	 * that way on all systems.
	 */
		ptr = (CHR *)(intptr_t)x;
	}
	while (lastch > ptr)
		YYUNPUT(*--lastch);
	*lastch = 0;
	if (ptr > YYTEXT)
		yyprevious = *--lastch;
	YYLENG = ptr-YYTEXT;
#ifdef EOPTION
	yyleng = wcstombs((char *)yytext, YYTEXT, YYLENG*MB_LEN_MAX);
#endif
	return (0);
}

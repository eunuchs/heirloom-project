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
/*	Copyright (c) 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "reject.c	6.10	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)reject.c	1.4 (gritter) 11/27/05
 */

#include <stdio.h>

#ifdef EUC
#ifdef	__sun
#include <euc.h>
#include <widec.h>
#else	/* !sun */
#include <wchar.h>
#endif	/* !sun */
#include <limits.h>
#endif


#ifndef JLSLEX

#define	CHR    char
#define	YYTEXT yytext
#define	YYLENG yyleng
#define	YYINPUT yyinput
#define	YYUNPUT yyunput
#define	YYOUTPUT yyoutput
#define	YYREJECT yyreject
#endif

#ifdef WOPTION

#define	CHR    wchar_t
#define	YYTEXT yytext
#define	YYLENG yyleng
#define	YYINPUT yyinput
#define	YYUNPUT yyunput
#define	YYOUTPUT yyoutput
#define	YYREJECT yyreject_w
#endif

#ifdef EOPTION

#define	CHR    wchar_t
#define	YYTEXT yywtext
#define	YYLENG yywleng
#define	YYINPUT yywinput
#define	YYUNPUT yywunput
#define	YYOUTPUT yywoutput
#define	YYREJECT yyreject_e
extern unsigned char yytext[];
extern int yyleng;
#endif

#if defined(__cplusplus) || defined(__STDC__)
extern int	yyback(int *, int);
extern int	YYINPUT(void);
extern void	YYUNPUT(int);
#ifdef EUC
	static int	yyracc(int);
#else
	extern int	yyracc(int);
#endif
#ifdef EOPTION
	extern size_t	wcstombs(char *, const wchar_t *, size_t);
#endif
#endif

extern FILE *yyout, *yyin;

extern int yyprevious, *yyfnd;

extern char yyextra[];

extern int YYLENG;
extern CHR YYTEXT[];

extern struct {int *yyaa, *yybb; int *yystops; } *yylstate[], **yylsp, **yyolsp;
#if defined(__cplusplus) || defined(__STDC__)
int
YYREJECT(void)
#else
YYREJECT()
#endif
{
	for (; yylsp < yyolsp; yylsp++)
		YYTEXT[YYLENG++] = YYINPUT();
	if (*yyfnd > 0)
		return (yyracc(*yyfnd++));
	while (yylsp-- > yylstate) {
		YYUNPUT(YYTEXT[YYLENG-1]);
		YYTEXT[--YYLENG] = 0;
		if (*yylsp != 0 && (yyfnd = (*yylsp)->yystops) && *yyfnd > 0)
			return (yyracc(*yyfnd++));
	}
#ifdef EOPTION
	yyleng = wcstombs((char *)yytext, YYTEXT, YYLENG*MB_LEN_MAX);
#endif
	if (YYTEXT[0] == 0)
		return (0);
	YYLENG = 0;
#ifdef EOPTION
	yyleng = 0;
#endif
	return (-1);
}

#ifdef	EUC
static
#endif
#if defined(__cplusplus) || defined(__STDC__)
int
yyracc(int m)
#else
yyracc(m)
#endif
{
	yyolsp = yylsp;
	if (yyextra[m]) {
		while (yyback((*yylsp)->yystops, -m) != 1 && yylsp > yylstate) {
			yylsp--;
			YYUNPUT(YYTEXT[--YYLENG]);
		}
	}
	yyprevious = YYTEXT[YYLENG-1];
	YYTEXT[YYLENG] = 0;
#ifdef EOPTION
	yyleng = wcstombs((char *)yytext, YYTEXT, YYLENG*MB_LEN_MAX);
#endif
	return (m);
}

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
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)wcio.c	1.1 (gritter) 6/25/05
 */
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

extern int	error(char *, ...);

/*
 * This is like getwc() but issues an error message when an illegal
 * byte sequence is encountered.
 */
wint_t
lex_getwc(FILE *fp)
{
	wint_t	wc;

	if ((wc = getwc(fp)) != WEOF)
		return wc;
	if (ferror(fp) && errno == EILSEQ)
		error("illegal byte sequence");
	return wc;
}

/*
 * A substitute for putwc(), to ensure that stdio output FILE objects
 * are always byte-oriented.
 */
wint_t
lex_putwc(wchar_t wc, FILE *fp)
{
	char	mb[MB_LEN_MAX];
	int	i, n;

	if ((n = wctomb(mb, wc)) < 0) {
		wctomb(mb, 0);
		errno = EILSEQ;
		return WEOF;
	}
	for (i = 0; i < n; i++)
		if (putc(mb[i]&0377, fp) == EOF)
			return WEOF;
	return wc;
}

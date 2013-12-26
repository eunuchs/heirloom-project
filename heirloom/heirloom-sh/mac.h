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
 * Sccsid @(#)mac.h	1.6 (gritter) 6/19/05
 */

/* from OpenSolaris "mac.h	1.8	05/06/08 SMI"	 SVr4.0 1.8 */
/*
 *	UNIX shell
 */

#undef	TRUE
#undef	FALSE
#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define QUOTE	0200

/*
 * NetBSD includes stdio.h indirectly through a variety of headers; maybe
 * others do too. To make sure that we get rid of EOF, include it here
 * forcibly.
 */
#include <stdio.h>
#undef	EOF
#define EOF	0
#define NL	'\n'
#define SPACE	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'


#define blank()		prc(SPACE)
#define	tab()		prc(TAB)
#define newline()	prc(NL)


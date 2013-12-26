/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from cat.c 1.4 06/12/12
 */

/*	from OpenSolaris "cat.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)cat.c	1.4 (gritter) 01/21/07
 */
/*	from OpenSolaris "sccs:lib/mpwlib/cat.c"	*/
#include	<stdlib.h>
#include	<stdarg.h>

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
*/

/*VARARGS*/
char *
cat(char *dest, ...)
{
	register char *d, *s;
	va_list ap;

	va_start(ap, dest);
	d = dest;

	while ((s = va_arg(ap, char *)) != NULL) {
		while (*d++ = *s++) ;
		d--;
	}
	va_end(ap);
	return (dest);
}

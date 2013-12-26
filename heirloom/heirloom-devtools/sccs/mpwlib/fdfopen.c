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
 * from fdfopen.c 1.5 06/12/12
 */

/*	from OpenSolaris "fdfopen.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fdfopen.c	1.3 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/fdfopen.c"	*/
/*
	Interfaces with /lib/libS.a
	First arg is file descriptor, second is read/write mode (0/1).
	Returns file pointer on success,
	NULL on failure (no file structures available).
*/

# include	<stdio.h>
# include	<sys/types.h>
# include	<macros.h>

FILE *
fdfopen(register int fd, register int mode)
{
	if (fstat(fd, &Statbuf) < 0)
		return(NULL);
	if (mode)
		return(fdopen(fd,"w"));
	else
		return(fdopen(fd,"r"));
}

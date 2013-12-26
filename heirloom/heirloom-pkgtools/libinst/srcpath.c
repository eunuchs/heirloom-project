/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
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

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "srcpath.c	1.7	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)srcpath.c	1.3 (gritter) 2/25/07
 */

/*LINTLIBRARY*/

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

char *
srcpath(char *d, char *p, int part, int nparts)
{
	static char tmppath[PATH_MAX];
	char	*copy;
	size_t	copyLen;

	copy = tmppath;

	if (d) {
		size_t theLen;
		(void) strcpy(copy, d);
		theLen = strlen(copy);
		copy += theLen;
		copyLen = (sizeof (tmppath) - theLen);
	} else {
		copy[0] = '\0';
		copyLen = sizeof (tmppath);
	}

	if (nparts > 1) {
		(void) snprintf(copy, copyLen,
			((p[0] == '/') ? "/root.%d%s" : "/reloc.%d/%s"),
			part, p);
	} else {
		(void) snprintf(copy, copyLen,
			((p[0] == '/') ? "/root%s" : "/reloc/%s"), p);
	}

	return (tmppath);
}

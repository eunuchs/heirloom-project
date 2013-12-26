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
 * from xopen.c 1.8 06/12/12
 */

/*	from OpenSolaris "xopen.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)xopen.c	1.3 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/xopen.c"	*/
/*
	Interface to open(II) which differentiates among the various
	open errors.
	Returns file descriptor on success,
	fatal() on failure.
*/

# include <stdio.h>
# include <errno.h>
# include <fcntl.h>
# include <i18n.h>
# include <ccstypes.h>
# include <locale.h>
# include <defines.h>

int
xopen(char name[], mode_t mode)
{
	register int fd;
	extern char SccsError[];

	if ((fd = open(name,mode)) < 0) {
		if(errno == EACCES) {
			if(mode == 0)
				sprintf(SccsError,"`%s' unreadable (ut5)",
					name);
			else if(mode == 1)
				sprintf(SccsError,"`%s' unwritable (ut6)",
					name);
			else
				sprintf(SccsError,"`%s' unreadable or unwritable (ut7)",
					name);
			fd = fatal(SccsError);
		}
		else
			fd = xmsg(name,NOGETTEXT("xopen"));
	}
	return(fd);
}

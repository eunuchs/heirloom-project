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
 * from xmsg.c 1.6 06/12/12
 */

/*	from OpenSolaris "xmsg.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)xmsg.c	1.3 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/xmsg.c"	*/
# include	<defines.h>

/*
	Call fatal with an appropriate error message
	based on errno.  If no good message can be made up, it makes
	up a simple message.
	The second argument is a pointer to the calling functions
	name (a string); it's used in the manufactured message.
*/
int 
xmsg(const char *file, const char *func)
{
	register char *str;
	char d[FILESIZE];
	extern char SccsError[];

	switch (errno) {
	case ENFILE:
		str = "no file (ut3)";
		break;
	case ENOENT:
		sprintf(str = SccsError,"`%s' nonexistent (ut4)",file);
		break;
	case EACCES:
		copy(file,d);
		sprintf(str = SccsError,"directory `%s' unwritable (ut2)",
			dname(d));
		break;
	case ENOSPC:
		str = "no space! (ut10)";
		break;
	case EFBIG:
		str = "write error (ut8)";
		break;
	default:
		sprintf(str = SccsError,"errno = %d, function = `%s' (ut11)",
			errno,
			func);
		break;
	}
	return(fatal(str));
}





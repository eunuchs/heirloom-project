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
 * from logname.c 1.7 06/12/12
 */

/*	from OpenSolaris "logname.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)logname.c	1.5 (gritter) 01/05/07
 */
/*	from OpenSolaris "sccs:lib/comobj/logname.c"	*/
# include	<defines.h>
# include	<pwd.h>

/* initialize this variable to make the Mac OS X linker happy */
char saveid[50] = { 0 };
char *
logname(void)
{
	struct passwd *log_name;
	uid_t uid;

	uid = getuid();
	log_name = getpwuid(uid);
	if (!log_name)
	   return(0);
	else
	   strcpy(saveid,log_name->pw_name);
	return(saveid);
}

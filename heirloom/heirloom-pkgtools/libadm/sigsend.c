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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "sigsend.c	1.11	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sigsend.c	1.3 (gritter) 2/24/07
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/


#include <sys/types.h>
#ifdef __sun
#include <sys/procset.h>
#else
#include <sys/wait.h>
#endif
#include <signal.h>
#include <errno.h>

int
sigsend(idtype_t idtype, id_t id, int sig)
{
#ifdef	__sun
	procset_t set;
	setprocset(&set, POP_AND, idtype, id, P_ALL, P_MYID);
	return (sigsendset(&set, sig));
#else	/* !__sun */
	switch (idtype) {
	case P_PID:
		return kill(id, sig);
	case P_PGID:
		return kill(-id, sig);
	default:
		errno = EINVAL;
		return -1;
	}
#endif	/* !__sun */
}

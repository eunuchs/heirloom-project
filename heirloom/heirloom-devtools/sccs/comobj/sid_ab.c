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
 * Copyright 1996 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from sid_ab.c 1.4 06/12/12
 */

/*	from OpenSolaris "sid_ab.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sid_ab.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/sid_ab.c"	*/
# include	<defines.h>


char *
sid_ab(register char *p, register struct sid *sp)
{
	if (*(p = satoi(p,&sp->s_rel)) == '.')
		p++;
	if (*(p = satoi(p,&sp->s_lev)) == '.')
		p++;
	if (*(p = satoi(p,&sp->s_br)) == '.')
		p++;
	p = satoi(p,&sp->s_seq);
	return(p);
}

char *
omit_sid(char *p)
{
	struct sid sid;
	
	return(sid_ab(p,&sid));
}


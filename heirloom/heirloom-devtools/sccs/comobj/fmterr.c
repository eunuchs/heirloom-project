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
 * Copyright 1998 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from fmterr.c 1.5 06/12/12
 */

/*	from OpenSolaris "fmterr.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fmterr.c	1.5 (gritter) 01/03/07
 */
/*	from OpenSolaris "sccs:lib/comobj/fmterr.c"	*/
# include	<defines.h>
# include       <locale.h>

void
fmterr(register struct packet *pkt)
{
	extern char SccsError[];

	fclose(pkt->p_iop);
	pkt->p_iop = NULL;
	sprintf(SccsError, "format error at line %d (co4)", pkt->p_slnno);
	fatal(SccsError);
}

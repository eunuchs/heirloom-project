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
 * Copyright 1997 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from doie.c 1.7 06/12/12
 */

/*	from OpenSolaris "doie.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)doie.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/doie.c"	*/
# include	<defines.h>
# include       <locale.h>

void
doie(struct packet *pkt,char *ilist,char *elist,char *glist)
{
	if (ilist) {
		if (pkt->p_verbose) {
			fprintf(pkt->p_stdout, "Included:\n");
		}
		dolist(pkt,ilist,INCLUDE);
	}
	if (elist) {
		if (pkt->p_verbose) {
			fprintf(pkt->p_stdout, "Excluded:\n");
		}
		dolist(pkt,elist,EXCLUDE);
	}
	if (glist)
		dolist(pkt,glist,IGNORE);
}

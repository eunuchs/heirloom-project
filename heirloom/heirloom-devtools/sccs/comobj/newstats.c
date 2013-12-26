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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from newstats.c 1.3 06/12/12
 */

/*	from OpenSolaris "newstats.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)newstats.c	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/comobj/newstats.c"	*/
# include	<defines.h>

void 
newstats(register struct packet *pkt, register char *strp, register char *ch)
{
	char fivech[6];
	register char *r;
	int i;

	r = fivech;
	for (i=0; i < 5; i++)
		*r++ = *ch;
	*r = '\0';
	sprintf(strp,"%c%c %s/%s/%s\n",CTLCHAR,STATS,fivech,fivech,fivech);
	putline(pkt,strp);
}

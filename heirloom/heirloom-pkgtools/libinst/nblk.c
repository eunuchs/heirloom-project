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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "nblk.c	1.7	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)nblk.c	1.2 (gritter) 2/24/07
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>

/*
 * This should not be a constant, but for ufs it is 12, not 10 like for s5.
 */
#define	DIRECT 12	/* Number of logical blocks before indirection */

long
nblk(fsblkcnt_t size, fsblkcnt_t bsize, fsblkcnt_t frsize)
{
	fsblkcnt_t tot, count, count1, d_indirect, t_indirect, ind;
	fsblkcnt_t frags = 0;

	if (size == 0 || bsize == 0)
		return (1);

	/*
	 * Need to keep track of indirect blocks.
	 */

	ind = howmany(bsize, sizeof (daddr_t));
	d_indirect = ind + DIRECT; 			/* double indirection */
	t_indirect = ind * (ind + 1) + DIRECT; 		/* triple indirection */

	tot = howmany(size, bsize);

	if (tot > t_indirect) {
		count1 = (tot - ind * ind - (DIRECT + 1)) / ind;
		count = count1 + count1 / ind + ind + 3;
	} else if (tot > d_indirect) {
		count = (tot - (DIRECT + 1)) / ind + 2;
	} else if (tot > DIRECT) {
		count = 1;
	} else {
		count = 0;
		frags = ((long)frsize > 0) ?
		    roundup(size, frsize) :
		    roundup(size, bsize);
	}

	/* Accounting for the indirect blocks, the total becomes */
	tot += count;

	/*
	 * calculate number of 512 byte blocks, for frag or full block cases.
	 */
	if (!frags)
		tot *= howmany(bsize, DEV_BSIZE);
	else
		tot = howmany(frags, DEV_BSIZE);
	return (tot);
}

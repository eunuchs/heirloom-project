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
 * from cmpdate.c 1.2 06/12/12
 */

/*	from OpenSolaris "cmpdate.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)cmpdate.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/cmpdate.c"	*/
#include <time.h>

/*
 * Compare two delta's based on time
 *
 * Returns 0 for same,
 * > 0 if d1 > d2
 * < 0 if d1 < d2
 * N.B. - only looks at fields that SCCS sets
 */
int
cmpdate(struct tm * d1, struct tm * d2)
{
	if (d1->tm_year != d2->tm_year) {
		return (d1->tm_year - d2->tm_year);
	}
	if (d1->tm_mon != d2->tm_mon) {
		return (d1->tm_mon - d2->tm_mon);
	}
	if (d1->tm_mday != d2->tm_mday) {
		return (d1->tm_mday - d2->tm_mday);
	}
	if (d1->tm_hour != d2->tm_hour) {
		return (d1->tm_hour - d2->tm_hour);
	}
	if (d1->tm_min != d2->tm_min) {
		return (d1->tm_min - d2->tm_min);
	}
	if (d1->tm_sec != d2->tm_sec) {
		return (d1->tm_sec - d2->tm_sec);
	}
	return (0);	/* they're equal */
}

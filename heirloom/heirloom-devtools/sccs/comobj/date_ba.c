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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from date_ba.c 1.5 06/12/12
 */

/*	from OpenSolaris "date_ba.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)date_ba.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/date_ba.c"	*/
# include	<defines.h>

# define DO2(p,n,c)	*p++ = ((char) ((n)/10) + '0'); *p++ = ( (char) ((n)%10) + '0'); *p++ = c;


char *
date_ba(time_t *bdt, char *adt)
{
	register struct tm *lcltm;
	register char *p;

#if defined(BUG_1205145) || defined(GMT_TIME)
	lcltm = gmtime(bdt);
#else
	lcltm = localtime(bdt);
#endif
	p = adt;
	if (lcltm->tm_year >= 100) {
	   lcltm->tm_year -= 100;
	}
	DO2(p,lcltm->tm_year,'/');
	DO2(p,lcltm->tm_mon + 1,'/');
	DO2(p,lcltm->tm_mday,' ');
	DO2(p,lcltm->tm_hour,':');
	DO2(p,lcltm->tm_min,':');
	DO2(p,lcltm->tm_sec,0);
	return(adt);
}

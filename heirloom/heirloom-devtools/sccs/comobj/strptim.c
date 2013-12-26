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
 * from strptim.c 1.7 06/12/12
 */

/*	from OpenSolaris "strptime.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)strptim.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/strptime.c"	*/
#include <defines.h>

/*
 * We assume that p looks like: "91/04/13 23:23:46" if val = 1
 */

int
mystrptime(char *p, struct tm *t, int val)
{
	int dn, cn, warn = 0;
#if defined(BUG_1205145) || defined(GMT_TIME)
	time_t gtime;
#endif

	/*
	 * If 'year' field is less than 70 then we actually have 
	 * 21-st century. Add 100 to the t->tm_year because 
	 * tm_year should be a year since 1900.
	 * For example, the string 02/09/05 should give tm_year
	 * equal to 102, not 2.
	 */
	memset(t, 0, sizeof(*t));
	if (val) {
		NONBLANK(p);

		t->tm_year=gN(p, &p, 4, &dn, &cn);
		if(t->tm_year<0) return(-1);
		if((dn!=2 && dn!=4) || cn!=dn || *p!='/') warn=1;
		if(dn<=2) {
			if(t->tm_year<69) {
				t->tm_year += 100;
			}
		} else {
			if (t->tm_year<1969) {
				return(-1);
			}
			t->tm_year -= 1900;
		}

		t->tm_mon=gN(p, &p, 2, &dn, &cn);
		if(t->tm_mon<1 || t->tm_mon>12) return(-1);
		if(dn!=2 || cn!=dn+1 || *p!='/') warn=1;

		t->tm_mday=gN(p, &p, 2, &dn, &cn);
		if(t->tm_mday<1 || t->tm_mday>mosize(t->tm_year,t->tm_mon)) return(-1);
		if(dn!=2 || cn!=dn+1) warn=1;

		NONBLANK(p);

		t->tm_hour=gN(p, &p, 2, &dn, &cn);
		if(t->tm_hour<0 || t->tm_hour>23) return(-1);
		if(dn!=2 || cn!=dn || *p!=':') warn=1;

		t->tm_min=gN(p, &p, 2, &dn, &cn);
		if(t->tm_min<0 || t->tm_min>59) return(-1);
		if(dn!=2 || cn!=dn+1 || *p!=':') warn=1;

		t->tm_sec=gN(p, &p, 2, &dn, &cn);
		if(t->tm_sec<0 || t->tm_sec>59) return(-1);
		if(dn!=2 || cn!=dn+1) warn=1;
#if defined(BUG_1205145) || defined(GMT_TIME)
		gtime = mktime(t);
		if (gtime == -1) return(-1);
		gtime -= timezone;
		*t = *(localtime(&gtime));
#endif
	} else {
		if((t->tm_year=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_year = 99;
		if (t->tm_year<69) t->tm_year += 100;

		if((t->tm_mon=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_mon = 12;
		if(t->tm_mon<1 || t->tm_mon>12) return(-1);

		if((t->tm_mday=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_mday = mosize(t->tm_year,t->tm_mon);
		if(t->tm_mday<1 || t->tm_mday>mosize(t->tm_year,t->tm_mon)) return(-1);

		if((t->tm_hour=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_hour = 23;
		if(t->tm_hour<0 || t->tm_hour>23) return(-1);

		if((t->tm_min=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_min = 59;
		if(t->tm_min<0 || t->tm_min>59) return(-1);

		if((t->tm_sec=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_sec = 59;
		if(t->tm_sec<0 || t->tm_sec>59) return(-1);
	}
	return(warn);
}

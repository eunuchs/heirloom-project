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
 * from date_ab.c 1.8 06/12/12
 */

/*	from OpenSolaris "date_ab.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)date_ab.c	1.5 (gritter) 01/12/07
 */
/*	from OpenSolaris "sccs:lib/comobj/date_ab.c"	*/
# include	<sys/types.h>
# include	<defines.h>
# include	<macros.h>
#if !(defined(BUG_1205145) || defined(GMT_TIME))
# include	<time.h>
#endif

/*
	Function to convert date in the form "[yy|yyyy]/mm/dd hh:mm:ss" to
	standard UNIX time (seconds since Jan. 1, 1970 GMT).

	The function corrects properly for leap year,
	daylight savings time, offset from Greenwich time, etc.

	Function returns -1 if bad time is given.
*/
#define	dysize(A) (((A)%4)? 365: 366)

char *Datep;
static int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int
date_ab(char *adt,time_t *bdt)
{
	int y, t, d, h, m, s, i, dn, cn, warn = 0;
	time_t	tim;
#if defined (__FreeBSD__) || defined (__DragonFly__)
	long	timezone;
	struct tm	*tp;
	tim = 999;
	tp = localtime(&tim);
	timezone = -tp->tm_gmtoff;
#endif	/* __FreeBSD__, __DragonFly__ */

#if !(defined(BUG_1205145) || defined(GMT_TIME))
	tzset();
#endif
	Datep = adt;

	NONBLANK(Datep);

	y=gN(Datep, &Datep, 4, &dn, &cn);
	if(y<0) return(-1);
	if((dn!=2 && dn!=4) || cn!=dn || *Datep!='/') warn=1;
	if(dn<=2) {
		if(y<69) {
			y += 2000;
		} else {
			y += 1900;
		}
	} else {
		if(y<1969) {
			return(-1);
		}
	}

	t=gN(Datep, &Datep, 2, &dn, &cn);
	if(t<1 || t>12) return(-1);
	if(dn!=2 || cn!=dn+1 || *Datep!='/') warn=1;

	d=gN(Datep, &Datep, 2, &dn, &cn);
	if(d<1 || d>mosize(y,t)) return(-1);
	if(dn!=2 || cn!=dn+1) warn=1;

	NONBLANK(Datep);

	h=gN(Datep, &Datep, 2, &dn, &cn);
	if(h<0 || h>23) return(-1);
	if(dn!=2 || cn!=dn || *Datep!=':') warn=1;

	m=gN(Datep, &Datep, 2, &dn, &cn);
	if(m<0 || m>59) return(-1);
	if(dn!=2 || cn!=dn+1 || *Datep!=':') warn=1;

	s=gN(Datep, &Datep, 2, &dn, &cn);
	if(s<0 || s>59) return(-1);
	if(dn!=2 || cn!=dn+1) warn=1;

	tim = (time_t)0L;
	for(i=1970; i<y; i++)
		tim += dysize(i);
	while(--t)
		tim += mosize(y,t);
	tim += d - 1;
	tim *= 24;
	tim += h;
	tim *= 60;
	tim += m;
	tim *= 60;
	tim += s;

#if !(defined(BUG_1205145) || defined(GMT_TIME))
	tim += timezone;			/* GMT correction */
	if((localtime(&tim))->tm_isdst)
		tim += -1*60*60;		/* daylight savings */
#endif
	*bdt = tim;
	return(warn);
}

/*
	Function to convert date in the form "yymmddhhmmss" to
	standard UNIX time (seconds since Jan. 1, 1970 GMT).
	Units left off of the right are replaced by their
	maximum possible values.

	The function corrects properly for leap year,
	daylight savings time, offset from Greenwich time, etc.

	Function returns -1 if bad time is given (i.e., "730229").
*/
int
parse_date(char *adt,time_t *bdt)
{
	int y, t, d, h, m, s, i;
	time_t	tim;
#if defined (__FreeBSD__) || defined (__DragonFly__)
	long	timezone;
	struct tm	*tp;
	tim = 999;
	tp = localtime(&tim);
	timezone = -tp->tm_gmtoff;
#endif	/* __FreeBSD__, __DragonFly__ */

	tzset();

	if((y=gN(adt, &adt, 2, NULL, NULL)) == -2) y = 99;
	if (y<69) y += 100;

	if((t=gN(adt, &adt, 2, NULL, NULL)) == -2) t = 12;
	if(t<1 || t>12) return(-1);

	if((d=gN(adt, &adt, 2, NULL, NULL)) == -2) d = mosize(y,t);
	if(d<1 || d>mosize(y,t)) return(-1);

	if((h=gN(adt, &adt, 2, NULL, NULL)) == -2) h = 23;
	if(h<0 || h>23) return(-1);

	if((m=gN(adt, &adt, 2, NULL, NULL)) == -2) m = 59;
	if(m<0 || m>59) return(-1);

	if((s=gN(adt, &adt, 2, NULL, NULL)) == -2) s = 59;
	if(s<0 || s>59) return(-1);

	tim = (time_t)0L;
	y += 1900;
	for(i=1970; i<y; i++)
		tim += dysize(i);
	while(--t)
		tim += mosize(y,t);
	tim += d - 1;
	tim *= 24;
	tim += h;
	tim *= 60;
	tim += m;
	tim *= 60;
	tim += s;

	tim += timezone;			/* GMT correction */
	if((localtime(&tim))->tm_isdst)
		tim += -1*60*60;		/* daylight savings */
	*bdt = tim;
	return(0);
}

int 
mosize(int y, int t)
{

	if(t==2 && dysize(y)==366) return(29);
	return(dmsize[t-1]);
}

int
gN(char *str, char **next, int num, int *digits, int *chars)
{
	register int c = 0;
	register int n = 0;
	register int m = 0;

	while (*str && !numeric(*str)) {
		str++;
		m++;
	}
	if (*str) {
		while ((num-- > 0) && numeric(*str)) {
			c = (c * 10) + (*str++ - '0');
			n++;
			m++;
		}
	} else {
		c = -2;
	}
	if (next) {
		*next = str;
	}
	if (digits) {
		*digits = n;
	}
	if (chars) {
		*chars = m;
	}

	return c;
}

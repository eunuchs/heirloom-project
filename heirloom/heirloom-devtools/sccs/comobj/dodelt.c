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
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from dodelt.c 1.8 06/12/12
 */

/*	from OpenSolaris "dodelt.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dodelt.c	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/comobj/dodelt.c"	*/
#include	<defines.h>

# define ONEYEAR 31536000L

static char	getadel(struct packet *,struct deltab *);
static void	doixg(char *,struct ixg **);

time_t	Timenow;

char	Pgmr[LOGSIZE];	/* for rmdel & chghist (rmchg) */
int	First_esc;
int	First_cmt;
int	CDid_mrs;		/* for chghist to check MRs */

struct idel *
dodelt(register struct packet *pkt, struct stats *statp, struct sid *sidp, int type)
{
	extern  char	had[26];
	char *c;
	struct deltab dt;
	register struct idel *rdp = NULL;
	int n, founddel;
	void	fredck(struct packet *), escdodelt(struct packet *);
	register char *p;

	pkt->p_idel = 0;
	founddel = 0;

	time(&Timenow);
	stats_ab(pkt,statp);
	while (getadel(pkt,&dt) == BDELTAB) {
		if (pkt->p_idel == 0) {
			if (Timenow < dt.d_datetime)
				fprintf(stderr,"Time stamp later than current clock time (co10)\n");
			pkt->p_idel = (struct idel *)
					fmalloc((unsigned) (n=((dt.d_serial+1)*
					sizeof(*pkt->p_idel))));
			zero((char *) pkt->p_idel,n);
			pkt->p_apply = (struct apply *)
					fmalloc((unsigned) (n=((dt.d_serial+1)*
					sizeof(*pkt->p_apply))));
			zero((char *) pkt->p_apply,n);
			pkt->p_idel->i_pred = dt.d_serial;
		}
		if (dt.d_type == 'D') {
			if (sidp && eqsid(&dt.d_sid,sidp)) {
				copy(dt.d_pgmr,Pgmr);	/* for rmchg */
				zero((char *) sidp,sizeof(*sidp));
				founddel = 1;
				First_esc = 1;
				First_cmt = 1;
				CDid_mrs = 0;
				for (p = pkt->p_line; *p && *p != 'D'; p++)
					;
				if (*p)
					*p = type;
			}
			else
				First_esc = founddel = First_cmt = 0;
			pkt->p_maxr = max(pkt->p_maxr,dt.d_sid.s_rel);
			rdp = &pkt->p_idel[dt.d_serial];
			rdp->i_sid.s_rel = dt.d_sid.s_rel;
			rdp->i_sid.s_lev = dt.d_sid.s_lev;
			rdp->i_sid.s_br = dt.d_sid.s_br;
			rdp->i_sid.s_seq = dt.d_sid.s_seq;
			rdp->i_pred = dt.d_pred;
			rdp->i_datetime = dt.d_datetime;
		}
		while ((c = getline(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch (pkt->p_line[1]) {
				case EDELTAB:
					break;
				case COMMENTS:
				case MRNUM:
					if (founddel)
					{
						escdodelt(pkt);
						if(type == 'R' && (had[('z' - 'a')]) && pkt->p_line[1] == MRNUM )
						{
							fredck(pkt);
						}
					}
				continue;
				default:
					fmterr(pkt);
				/*FALLTHRU*/
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
					if (dt.d_type == 'D') {
						doixg(pkt->p_line,&rdp->i_ixg);
					}
					continue;
				}
				break;
			}
		if (c == NULL || pkt->p_line[0] != CTLCHAR || getline(pkt) == NULL)
			fmterr(pkt);
		if (pkt->p_line[0] != CTLCHAR || pkt->p_line[1] != STATS)
			break;
	}
	return(pkt->p_idel);
}

static char 
getadel(register struct packet *pkt, register struct deltab *dt)
{
	if (getline(pkt) == NULL)
		fmterr(pkt);
	return(del_ab(pkt->p_line,dt,pkt));
}

static void 
doixg(char *p, struct ixg **ixgp)
{
	int *v, *ip;
	int type, cnt, i;
	struct ixg *curp, *prevp;
	int xtmp[MAXLINE];

	v = ip = xtmp;
	++p;
	type = *p++;
	NONBLANK(p);
	while (numeric(*p)) {
		p = satoi(p,ip++);
		NONBLANK(p);
	}
	cnt = ip - v;
	for (prevp = curp = (*ixgp); curp; curp = (prevp = curp)->i_next ) 
		;
	curp = (struct ixg *) fmalloc((unsigned) 
		(sizeof(struct ixg) + (cnt-1)*sizeof(curp->i_ser[0])));
	if (*ixgp == 0)
		*ixgp = curp;
	else
		prevp->i_next = curp;
	curp->i_next = 0;
	curp->i_type = (char) type;
	curp->i_cnt = (char) cnt;
	for (i=0; cnt>0; --cnt)
		curp->i_ser[i++] = *v++;
}

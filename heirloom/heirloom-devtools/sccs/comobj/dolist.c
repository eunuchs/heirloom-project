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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from dolist.c 1.5 06/12/12
 */

/*	from OpenSolaris "dolist.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dolist.c	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/comobj/dolist.c"	*/
# include	<defines.h>
# include       <locale.h>

static char *getasid(register char *, register struct sid *);

void 
dolist(struct packet *pkt, register char *list, int ch)
{
	void	enter(struct packet *, int, int, struct sid *);
	struct sid lowsid, highsid, sid;
	int n;

	while (*list) {
		list = getasid(list,&lowsid);
		if (*list == '-') {
			++list;
			list = getasid(list,&highsid);
			if (lowsid.s_br == 0) {
				if ((highsid.s_br || highsid.s_seq ||
					highsid.s_rel < lowsid.s_rel ||
					(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev < lowsid.s_lev)))
				        fatal("bad range (co12)");
				sid.s_br = sid.s_seq = 0;
				for (sid.s_rel = lowsid.s_rel;
				     sid.s_rel <= highsid.s_rel;
				     sid.s_rel++) {
				   sid.s_lev = (sid.s_rel == lowsid.s_rel?
					        lowsid.s_lev : 1);
				   if (sid.s_rel < highsid.s_rel) {
				      for (; n = sidtoser(&sid,pkt);
					   sid.s_lev++)
					 enter(pkt,ch,n,&sid);
				   } else { /* == */
				      for (;
				           (sid.s_lev <= highsid.s_lev) && 
				           (n = sidtoser(&sid,pkt));
					   sid.s_lev++ )
					 enter(pkt,ch,n,&sid);
				   }	 
				}
			}
			else {
				if (!(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev == lowsid.s_lev &&
					highsid.s_br == lowsid.s_br &&
					highsid.s_seq >= lowsid.s_seq))
				       fatal("bad range (co12)");
				for (;
				     (lowsid.s_seq <= highsid.s_seq) &&
				     (n = sidtoser(&lowsid,pkt));
				     lowsid.s_seq++ )
				   enter(pkt,ch,n,&lowsid);
			}
		}
		else {
			if (n = sidtoser(&lowsid,pkt))
				enter(pkt,ch,n,&lowsid);
		}
		if (*list == ',')
			++list;
	}
}


static char *
getasid(register char *p, register struct sid *sp)
{
	register char *old;

	p = sid_ab(old = p,sp);
	if (old == p || sp->s_rel == 0)
		fatal("delta list syntax (co13)");
	if (sp->s_lev == 0) {
		sp->s_lev = MAX;
		if (sp->s_br || sp->s_seq)
			fatal("delta list syntax (co13)");
	}
	else if (sp->s_br) {
		if (sp->s_seq == 0)
			sp->s_seq = MAX;
	}
	else if (sp->s_seq)
		fatal("delta list syntax (co13)");
	return(p);
}

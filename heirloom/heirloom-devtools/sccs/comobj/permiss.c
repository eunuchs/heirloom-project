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
 * from permiss.c 1.9 06/12/12
 */

/*	from OpenSolaris "permiss.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)permiss.c	1.5 (gritter) 01/05/07
 */
/*	from OpenSolaris "sccs:lib/comobj/permiss.c"	*/
# include	<defines.h>
# include       <i18n.h>

static void ck_lock(register char *, register struct packet *);

void 
finduser(register struct packet *pkt)
{
	register char *p;
	char	*user;
	char groupid[6];
	int none;
	int ok_user;
	extern char saveid[];

	none = 1;
#if 0
	user = logname();
#else
	user = saveid;
#endif
	sprintf(groupid,"%lu",(unsigned long)getgid());
	while ((p = getline(pkt)) != NULL && *p != CTLCHAR) {
		none = 0;
		ok_user = 1;
		repl(p,'\n','\0');	/* this is done for equal test below */
		if(*p == '!') {
			++p;
			ok_user = 0;
			}
		if (!pkt->p_user)
			if (equal(user,p) || equal(groupid,p))
				pkt->p_user = ok_user;
		*(strend(p)) = '\n';	/* repl \0 end of line w/ \n again */
	}
	if (none)
		pkt->p_user = 1;
	if (p == NULL || p[1] != EUSERNAM)
		fmterr(pkt);
}

/* initialize this variable to make the Mac OS X linker happy */
char	*Sflags[NFLAGS] = { 0 };

void 
doflags(struct packet *pkt)
{
	register char *p;
	register int k;

	for (k = 0; k < NFLAGS; k++)
		Sflags[k] = 0;
	while ((p = getline(pkt)) != NULL && *p++ == CTLCHAR && *p++ == FLAG) {
		NONBLANK(p);
		k = *p++ - 'a';
		NONBLANK(p);
		Sflags[k] = fmalloc(size(p));
		copy(p,Sflags[k]);
		for (p = Sflags[k]; *p++ != '\n'; )
			;
		*--p = 0;
	}
}

void 
permiss(register struct packet *pkt)
{
	extern char *Sflags[];
	register char *p;
	register int n;
	extern char SccsError[];

	if (!pkt->p_user)
		fatal("not authorized to make deltas (co14)");
	if ((p = Sflags[FLORFLAG - 'a']) != NULL) {
		if (((unsigned)pkt->p_reqsid.s_rel) < (n = patoi(p))) {
			sprintf(SccsError, "release %d < %d floor (co15)",
				pkt->p_reqsid.s_rel,
				n);
			fatal(SccsError);
		}
	}
	if ((p = Sflags[CEILFLAG - 'a']) != NULL) {
		if (((unsigned)pkt->p_reqsid.s_rel) > (n = patoi(p))) {
			sprintf(SccsError,"release %d > %d ceiling (co16)",
				pkt->p_reqsid.s_rel,
				n);
			fatal(SccsError);
		}
	}
	/*
	check to see if the file or any particular release is
	locked against editing. (Only if the `l' flag is set)
	*/
	if ((p = Sflags[LOCKFLAG - 'a']) != NULL)
		ck_lock(p,pkt);
}



/* 
 * Multiply  space needed for C locale by 3.  This should be adequate
 * for the longest localized strings.  The length is
 * strlen("SCCS file locked against editing (co23)") * 3 + 1)
 */
static char l_str[121];	

static void 
ck_lock(register char *p, register struct packet *pkt)
{
	int l_rel;
	int locked;

	strcpy(l_str, (const char *) NOGETTEXT("SCCS file locked against editing (co23)"));
	locked = 0;
	if (*p == 'a')
		locked++;
	else while(*p) {
		p = satoi(p,&l_rel);
		++p;
		if (l_rel == pkt->p_gotsid.s_rel || l_rel == pkt->p_reqsid.s_rel) {
			locked++;
			sprintf(l_str, "release `%d' locked against editing (co23)",
				l_rel);
			break;
		}
	}
	if (locked)
		fatal(l_str);
}

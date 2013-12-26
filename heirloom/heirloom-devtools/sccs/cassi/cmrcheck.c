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
 * from cmrcheck.c 1.5 06/12/12
 */

/*	from OpenSolaris "cmrcheck.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)cmrcheck.c	1.5 (gritter) 2/26/07
 */
/*	from OpenSolaris "sccs:lib/cassi/cmrcheck.c"	*/

/* EMACS_MODES: c tabstop=4 !fill */

/*
 *	cmrcheck -- Check list in p file to see if this cmr is valid.
 *
 *
 */

#include <defines.h>
#include <filehand.h>
#include <i18n.h>

/* Debugging options. */
#define MAXLENCMR 12
#ifdef TRACE
#define TR(W,X,Y,Z) fprintf (stdout, W, X, Y, Z)
#else
#define TR(W,X,Y,Z) /* W X Y Z */
#endif

#define CMRLIMIT 128		/* Length of cmr string. */

int 
cmrcheck(char *cmr, char *appl)
{
	char		lcmr[CMRLIMIT],	/* Local copy of CMR list. */
				*p[2], /* Field to match in .FRED file. */
				*format;

       /*
	* A safe estimate is that the longest localized string should 
	* not be more than 3 times as long as the "C" locale string.
	*/
	format = (char *) malloc(3 * sizeof("%s is not a valid CMR.\n") + 1);

	format = "%s is not a valid CMR.\n";

	TR("Cmrcheck: cmr=(%s) appl=(%s)\n", cmr, appl, NULL);
	p[1] = EMPTY;
	strcpy (lcmr, cmr);
	while ((p[0] = strrchr (lcmr, ',')) != EMPTY) {
		p[0]++;		/* Skip the ','. */
		if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY,
		  '\n', WHITE, 40, p, EMPTY, (char**) NULL, (int (*)(char *, int, char **)) NULL,
		  (int (*)(char **, char **, int)) NULL) != FOUND) {
			fprintf (stdout, format, p[0]);
			TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
			return (1);
			}
		p[0]--;							/* Go back to comma. */
		*p[0] = 0;					/* Clobber comma to end string. */
		}
	TR("Cmrcheck: last entry\n", NULL, NULL, NULL);
	p[0] = lcmr;						/* Last entry on the list. */
	if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY, '\n',
	  WHITE, 40, p, EMPTY, (char**) NULL, (int (*)(char *, int, char **)) NULL, (int (*)(char **, char **, int)) NULL)
	  != FOUND) {
		fprintf (stdout, format, p[0]);
		TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
		return (1);
		}
	TR("Cmrcheck: return=0\n", NULL, NULL, NULL);
	return (0);
}

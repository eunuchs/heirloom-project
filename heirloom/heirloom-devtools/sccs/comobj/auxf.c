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
 * Copyright 2000 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from auxf.c 1.5 06/12/12
 */

/*	from OpenSolaris "auxf.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)auxf.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/auxf.c"	*/
# include	<defines.h>


/*
	Figures out names for g-file, l-file, x-file, etc.

	File	Module	g-file	l-file	x-file & rest

	a/s.m	m	m	l.m	a/x.m

	Second argument is letter; 0 means module name is wanted.
*/

char *
auxf(register char *sfile, int ch)
{
	static char auxfile[FILESIZE];
	register char *snp;

	auxfile[0] = '\0';
	if(sfile[0] == '\0')
		return(auxfile);
	
	snp = sname(sfile);

	switch(ch) {

	case 0:
	case 'g':
			copy(&snp[2], auxfile);
			break;
	case 'A':
			copy("g.", auxfile);
			strcat(auxfile, &snp[2]);
			break;
	case 'l':
			copy(snp, auxfile);
			auxfile[0] = 'l';
			break;
	case 'G':
			copy(sfile, auxfile);
			auxfile[snp-sfile] = '\0';
			strcat(auxfile, "g.");
			strcat(auxfile, snp);
			break;
	default:
			copy(sfile, auxfile);
			auxfile[snp-sfile] = ch;
	}
	return(auxfile);
}

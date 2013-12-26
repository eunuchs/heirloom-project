/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
     
/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */
  
/*	from OpenSolaris "sqrt.c	1.4	05/06/02 SMI"	 SVr4.0 1.1		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)sqrt.c	1.6 (gritter) 1/13/08
 */

# include "e.h"

void
sqrt(int p2) {
#ifndef NEQN
	float nps;

	nps = (int)(EFFPS(((eht[p2]*9)/10+(resolution/POINT-1))/(resolution/POINT)));
#endif /* NEQN */
	yyval = p2;
#ifndef NEQN
	eht[yyval] = VERT(EM(1.2, nps));
	if(dbg)printf(".\tsqrt: S%d <- S%d;b=%g, h=%g\n", 
		yyval, p2, ebase[yyval], eht[yyval]);
	if (ital(rfont[yyval]))
		printf(".as %d \\|\n", yyval);
#endif /* NEQN */
	nrwid(p2, ps, p2);
#ifndef NEQN
	printf(".ds %d \\v'%gp'\\s%s\\v'-.2m'\\(sr\\l'\\n(%du\\(rn'\\v'.2m'\\s%s", 
		yyval, ebase[p2], tsize(nps), p2, tsize(ps));
	printf("\\v'%gp'\\h'-\\n(%du'\\*(%d\n", -ebase[p2], p2, p2);
	lfont[yyval] = ROM;
#else /* NEQN */
	printf(".ds %d \\v'%du'\\e\\L'%du'\\l'\\n(%du'",
		p2, ebase[p2], -eht[p2], p2);
	printf("\\v'%du'\\h'-\\n(%du'\\*(%d\n", eht[p2]-ebase[p2], p2, p2);
	eht[p2] += VERT(1);
	if(dbg)printf(".\tsqrt: S%d <- S%d;b=%d, h=%d\n", 
		p2, p2, ebase[p2], eht[p2]);
#endif /* NEQN */
}

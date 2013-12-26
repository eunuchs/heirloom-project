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
  
/*	from OpenSolaris "fromto.c	1.3	05/06/02 SMI"	 SVr4.0 1.1		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fromto.c	1.5 (gritter) 10/19/06
 */

# include "e.h"

void
fromto(int p1, int p2, int p3) {
	float pss;
#ifndef	NEQN
	float b, h1, b1;
#else	/* NEQN */
	int b, h1, b1;
#endif	/* NEQN */
	yyval = oalloc();
	lfont[yyval] = rfont[yyval] = 0;
	h1 = eht[yyval] = eht[p1];
	b1 = ebase[p1];
	b = 0;
	pss = EFFPS(ps);
	ps += 3;
	nrwid(p1, ps, p1);
	printf(".nr %d \\n(%d\n", yyval, p1);
	if( p2>0 ) {
		nrwid(p2, pss, p2);
		printf(".if \\n(%d>\\n(%d .nr %d \\n(%d\n", p2, yyval, yyval, p2);
		eht[yyval] += eht[p2];
		b = eht[p2];
	}
	if( p3>0 ) {
		nrwid(p3, pss, p3);
		printf(".if \\n(%d>\\n(%d .nr %d \\n(%d\n", p3, yyval, yyval, p3);
		eht[yyval] += eht[p3];
	}
	printf(".ds %d ", yyval);	/* bottom of middle box */
	if( p2>0 ) {
#ifndef	NEQN
		printf("\\v'%gp'\\h'\\n(%du-\\n(%du/2u'\\s%s\\*(%d\\s%s", 
			eht[p2]-ebase[p2]+b1, yyval, p2, tsize(pss), p2, tsize(EFFPS(ps)));
		printf("\\h'-\\n(%du-\\n(%du/2u'\\v'%gp'\\\n", 
			yyval, p2, -(eht[p2]-ebase[p2]+b1));
#else	/* NEQN */
		printf("\\v'%du'\\h'\\n(%du-\\n(%du/2u'\\s%s\\*(%d\\s%s", 
			eht[p2]-ebase[p2]+b1, yyval, p2, tsize(pss), p2, tsize(EFFPS(ps)));
		printf("\\h'-\\n(%du-\\n(%du/2u'\\v'%du'\\\n", 
			yyval, p2, -(eht[p2]-ebase[p2]+b1));
#endif	/* NEQN */
	}
#ifndef NEQN
	printf("\\h'\\n(%du-\\n(%du/2u'\\*(%d\\h'\\n(%du-\\n(%du/2u'\\\n", 
		yyval, p1, p1, yyval, p1);
#else /* NEQN */
	printf("\\h'\\n(%du-\\n(%du/2u'\\*(%d\\h'\\n(%du-\\n(%du+2u/2u'\\\n", 
		yyval, p1, p1, yyval, p1);
#endif /* NEQN */
	if( p3>0 ) {
#ifndef	NEQN
		printf("\\v'%gp'\\h'-\\n(%du-\\n(%du/2u'\\s%s\\*(%d\\s%s\\h'\\n(%du-\\n(%du/2u'\\v'%gp'\\\n", 
			-(h1-b1+ebase[p3]), yyval, p3, tsize(pss), p3, tsize(EFFPS(ps)), yyval, p3, (h1-b1+ebase[p3]));
#else	/* NEQN */
		printf("\\v'%du'\\h'-\\n(%du-\\n(%du/2u'\\s%s\\*(%d\\s%s\\h'\\n(%du-\\n(%du/2u'\\v'%du'\\\n", 
			-(h1-b1+ebase[p3]), yyval, p3, tsize(pss), p3, tsize(EFFPS(ps)), yyval, p3, (h1-b1+ebase[p3]));
#endif	/* NEQN */
	}
	printf("\n");
	ebase[yyval] = b + b1;
#ifndef	NEQN
	if(dbg)printf(".\tfrom to: S%d <- %d f %d t %d; h=%g b=%g\n", 
		yyval, p1, p2, p3, eht[yyval], ebase[yyval]);
#else	/* NEQN */
	if(dbg)printf(".\tfrom to: S%d <- %d f %d t %d; h=%d b=%d\n", 
		yyval, p1, p2, p3, eht[yyval], ebase[yyval]);
#endif	/* NEQN */
	ofree(p1);
	if( p2>0 ) ofree(p2);
	if( p3>0 ) ofree(p3);
}

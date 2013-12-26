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
  
/*	from OpenSolaris "move.c	1.4	05/06/02 SMI"	 SVr4.0 1.1		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)move.c	1.4 (gritter) 10/29/05
 */

# include "e.h"
# include "e.def"

void
move(int dir, int amt, int p) {
#ifndef	NEQN
	float a;
#else	/* NEQN */
	int a;
#endif	/* NEQN */

	yyval = p;
#ifndef NEQN
	a = VERT(EM(amt/100.0, EFFPS(ps)));
#else /* NEQN */
	a = VERT( (amt+49)/50 );	/* nearest number of half-lines */
#endif /* NEQN */
	printf(".ds %d ", yyval);
	if( dir == FWD || dir == BACK )	/* fwd, back */
#ifndef	NEQN
		printf("\\h'%s%gp'\\*(%d\n", (dir==BACK) ? "-" : "", a, p);
#else	/* NEQN */
		printf("\\h'%s%du'\\*(%d\n", (dir==BACK) ? "-" : "", a, p);
#endif	/* NEQN */
	else if (dir == UP)
#ifndef	NEQN
		printf("\\v'-%gp'\\*(%d\\v'%gp'\n", a, p, a);
#else	/* NEQN */
		printf("\\v'-%du'\\*(%d\\v'%du'\n", a, p, a);
#endif	/* NEQN */
	else if (dir == DOWN)
#ifndef	NEQN
		printf("\\v'%gp'\\*(%d\\v'-%gp'\n", a, p, a);
	if(dbg)printf(".\tmove %d dir %d amt %g; h=%g b=%g\n", 
		p, dir, a, eht[yyval], ebase[yyval]);
#else	/* NEQN */
		printf("\\v'%du'\\*(%d\\v'-%du'\n", a, p, a);
	if(dbg)printf(".\tmove %d dir %d amt %d; h=%d b=%d\n", 
		p, dir, a, eht[yyval], ebase[yyval]);
#endif	/* NEQN */
}

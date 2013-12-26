/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*
 * Copyright (c) 2001 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)blok.c	1.7 (gritter) 6/16/05
 */

/* from OpenSolaris "blok.c	1.19	05/06/08 SMI" */
/*
 *	UNIX shell
 */

#include	"defs.h"


/*
 *	storage allocator
 *	(circular first fit strategy)
 */

#define	BUSY 01
#define	busy(x)	(Rcheat((x)->word) & BUSY)

unsigned	brkincr = BRKINCR;
struct blk *blokp;			/* current search pointer */
struct blk *bloktop;		/* top of arena (last blok) */

unsigned char		*brkbegin;

void *
alloc(nbytes)
	size_t nbytes;
{
	register unsigned rbytes = round(nbytes+BYTESPERWORD, BYTESPERWORD);

	if (stakbot == 0) {
		addblok((unsigned)0);
	}

	for (;;)
	{
		int	c = 0;
		register struct blk *p = blokp;
		register struct blk *q;

		do
		{
			if (!busy(p))
			{
				while (!busy(q = p->word))
					p->word = q->word;
				if ((char *)q - (char *)p >= rbytes)
				{
					blokp = (struct blk *)
							((char *)p + rbytes);
					if (q > blokp)
						blokp->word = p->word;
					p->word = (struct blk *)
							(Rcheat(blokp) | BUSY);
					return ((char *)(p + 1));
				}
			}
			q = p;
			p = (struct blk *)(Rcheat(p->word) & ~BUSY);
		} while (p > q || (c++) == 0);
		addblok(rbytes);
	}
}

void
addblok(unsigned reqd)
{
	if (stakbot == 0)
	{
		brkbegin = setbrk(3 * BRKINCR);
		bloktop = (struct blk *)brkbegin;
	}

	if (stakbas != staktop)
	{
		register unsigned char *rndstak;
		register struct blk *blokstak;

		if (staktop >= brkend)
			growstak(staktop);
		pushstak(0);
		rndstak = (unsigned char *)round(staktop, BYTESPERWORD);
		blokstak = (struct blk *)(stakbas) - 1;
		blokstak->word = stakbsy;
		stakbsy = blokstak;
		bloktop->word = (struct blk *)(Rcheat(rndstak) | BUSY);
		bloktop = (struct blk *)(rndstak);
	}
	reqd += brkincr;
	reqd &= ~(brkincr - 1);
	blokp = bloktop;
	/*
	 * brkend points to the first invalid address.
	 * make sure bloktop is valid.
	 */
	if ((unsigned char *)&bloktop->word >= brkend)
	{
		if (setbrk((unsigned)((unsigned char *)
		    (&bloktop->word) - brkend + sizeof (struct blk))) ==
		    (unsigned char *)-1)
			error(nospace);
	}
	bloktop = bloktop->word = (struct blk *)(Rcheat(bloktop) + reqd);
	if ((unsigned char *)&bloktop->word >= brkend)
	{
		if (setbrk((unsigned)((unsigned char *)
		    (&bloktop->word) - brkend + sizeof (struct blk))) ==
		    (unsigned char *)-1)
			error(nospace);
	}
	bloktop->word = (struct blk *)(brkbegin + 1);
	{
		register unsigned char *stakadr = (unsigned char *)
							(bloktop + 2);
		register unsigned char *sp = stakadr;
		if (reqd = (staktop-stakbot))
		{
			if (stakadr + reqd >= brkend)
				growstak(stakadr + reqd);
			while (reqd-- > 0)
				*sp++ = *stakbot++;
			sp--;
		}
		staktop = sp;
		if (staktop >= brkend)
			growstak(staktop);
		stakbas = stakbot = stakadr;
	}
}

void
free(ap)
	void *ap;
{
	register struct blk *p;

	if ((p = (struct blk *)ap) && p < bloktop && p > (struct blk *)brkbegin)
	{
#ifdef DEBUG
		chkbptr(p);
#endif
		--p;
		p->word = (struct blk *)(Rcheat(p->word) & ~BUSY);
	}


}


#ifdef DEBUG

int 
chkbptr(struct blk *ptr)
{
	int	exf = 0;
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;)
	{
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (p+1 == ptr)
			exf++;

		if (q < (struct blk *)brkbegin || q > bloktop)
			abort(3);

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q)
			abort(4);

		p = q;
	}
	if (exf == 0)
		abort(1);
}


int 
chkmem(void)
{
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;) {
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (q < (struct blk *)brkbegin || q > bloktop)
			abort(3);

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q)
			abort(4);

		p = q;
	}

	prs("un/used/avail ");
	prn(un);
	blank();
	prn(us);
	blank();
	prn((char *)bloktop - brkbegin - (un + us));
	newline();

}

#endif

size_t
blklen(char *q)
{
	register struct blk *pp = (struct blk *)q;
	register struct blk *p;

	--pp;
	p = (struct blk *)(Rcheat(pp->word) & ~BUSY);

	return ((size_t)((long)p - (long)q));
}

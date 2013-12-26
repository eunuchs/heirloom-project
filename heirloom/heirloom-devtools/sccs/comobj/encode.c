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
 * from encode.c 1.4 06/12/12
 */

/*	from OpenSolaris "encode.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)encode.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/encode.c"	*/
# include       <defines.h>

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) (((c) & 077) + ' ')

/* single character decode */
#define DEC(c)	(((c) - ' ') & 077)

static int	fr(FILE *, char *, int);
static void	e_outdec(char *, FILE *);
static void	d_outdec(char *, FILE *, int);

void
encode(FILE *infile,FILE *outfile)
{
	char buf[80];
	int i,n;

	for (;;) 
	{
		/* 1 (up to) 45 character line */
		n = fr(infile, buf, 45);
		putc(ENC(n), outfile);
		for (i=0; i<n; i += 3)
			e_outdec(&buf[i], outfile);

		putc('\n', outfile);
		if (n <= 0)
			break;
	}
}

/*
 * output one group of 3 bytes, pointed at by p, on file f.
 */
static void
e_outdec(char *p, FILE *f)
{
        int c1, c2, c3, c4;
 
  
	c1 = *p >> 2;
	c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
	c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	c4 = p[2] & 077;
	putc(ENC(c1), f);
	putc(ENC(c2), f);
	putc(ENC(c3), f);
	putc(ENC(c4), f);
}

void
decode(char *istr,FILE *outfile)
{
	char *bp;
	int n;

	n = DEC(istr[0]);
	if (n <= 0)
		return;

	bp = &istr[1];
	while (n > 0) {
		d_outdec(bp, outfile, n);
		bp += 4;
		n -= 3;
	}
}

/*
 * output a group of 3 bytes (4 input characters).
 * the input chars are pointed to by p, they are to
 * be output to file f.  n is used to tell us not to
 * output all of them at the end of the file.
 */
static void
d_outdec(char *p, FILE *f, int n)
{
	int c1, c2, c3;

	c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
	c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
	c3 = DEC(p[2]) << 6 | DEC(p[3]);
	if (n >= 1)
		putc(c1, f);
	if (n >= 2)
		putc(c2, f);
	if (n >= 3)
		putc(c3, f);
}

/* fr: like read but stdio */
static int
fr(FILE *fd, char *buf, int cnt)
{
        int c, i;
 
        for (i=0; i<cnt; i++) {
                c = getc(fd);
                if (c == EOF)
                        return(i);
                buf[i] = c;
        }
        return (cnt);
}

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, June 2005.
 *
 * Sccsid @(#)umask.c	1.1 (gritter) 6/16/05
 */
/*	from Unix 7th Edition /usr/src/cmd/chmod.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include	"defs.h"
#include	<sys/stat.h>

extern const char	badumask[];
static mode_t		um;

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

#ifndef	S_ENFMT
#define	S_ENFMT	02000	/* mandatory locking bit */
#endif

static mode_t	absol(const char **);
static mode_t	who(const char **, mode_t *);
static int	what(const char **);
static mode_t	where(const char **, mode_t, int *, int *, const mode_t);

static mode_t
newmode(const char *ms, const mode_t pm)
{
	register mode_t	o, m, b;
	int	lock, setsgid = 0, cleared = 0, copy = 0;
	mode_t	nm, om, mm;

	nm = om = pm;
	m = absol(&ms);
	if (!*ms) {
		nm = m;
		goto out;
	}
	if ((lock = (nm&S_IFMT) != S_IFDIR && (nm&(S_ENFMT|S_IXGRP)) == S_ENFMT)
			== 01)
		nm &= ~(mode_t)S_ENFMT;
	do {
		m = who(&ms, &mm);
		while (o = what(&ms)) {
			b = where(&ms, nm, &lock, &copy, pm);
			switch (o) {
			case '+':
				nm |= b & m & ~mm;
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock |= 02;
				break;
			case '-':
				nm &= ~(b & m & ~mm);
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock = 0;
				break;
			case '=':
				nm &= ~m;
				nm |= b & m & ~mm;
				lock &= ~01;
				if (lock & 04)
					lock |= 02;
				om = 0;
				if (copy == 0)
					cleared = 1;
				break;
			}
			lock &= ~04;
		}
	} while (*ms++ == ',');
	if (*--ms)
		failed(&badumask[4], badumask);
out:	if (pm & S_IFDIR) {
		if ((pm & S_ISGID) && setsgid == 0)
			nm |= S_ISGID;
		else if ((nm & S_ISGID) && setsgid == 0)
			nm &= ~(mode_t)S_ISGID;
	}
	return(nm);
}

static mode_t
absol(const char **ms)
{
	register int c, i;

	i = 0;
	while ((c = *(*ms)++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	(*ms)--;
	return(i);
}

static mode_t
who(const char **ms, mode_t *mp)
{
	register int m;

	m = 0;
	*mp = 0;
	for (;;) switch (*(*ms)++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		(*ms)--;
		if (m == 0) {
			m = ALL;
			*mp = um;
		}
		return m;
	}
}

static int
what(const char **ms)
{
	switch (**ms) {
	case '+':
	case '-':
	case '=':
		return *(*ms)++;
	}
	return(0);
}

static mode_t
where(const char **ms, mode_t om, int *lock, int *copy, const mode_t pm)
{
	register mode_t m;

	m = 0;
	*copy = 0;
	switch (**ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		*copy = 1;
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++(*ms);
		return m;
	}
	for (;;) switch (*(*ms)++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((pm&S_IFMT) == S_IFDIR || (pm & EXEC))
			m |= EXEC;
		continue;
	case 'l':
		if ((pm&S_IFMT) != S_IFDIR)
			*lock |= 04;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		(*ms)--;
		return m;
	}
}

void
sysumask(int argc, char **argv)
{
	BOOL	symbolic = 0;
	mode_t	mode;
	register int	i;
	register char	*cp;

	if (argc > 1 && eq(argv[1], "-S")) {
		umask(um = mode = umask(0));
		prs_buff("u=");
		if ((mode & 0400) == 0)
			prc_buff('r');
		if ((mode & 0200) == 0)
			prc_buff('w');
		if ((mode & 0100) == 0)
			prc_buff('x');
		prs_buff(",g=");
		if ((mode & 040) == 0)
			prc_buff('r');
		if ((mode & 020) == 0)
			prc_buff('w');
		if ((mode & 010) == 0)
			prc_buff('x');
		prs_buff(",o=");
		if ((mode & 04) == 0)
			prc_buff('r');
		if ((mode & 02) == 0)
			prc_buff('w');
		if ((mode & 01) == 0)
			prc_buff('x');
		prc_buff(NL);
	} else if (argc == 1) {
		umask(um = mode = umask(0));
		prc_buff('0');
		for (i = 6; i >= 0; i -= 3)
			prc_buff(((mode >> i) & 07) +'0');
		prc_buff(NL);
	} else {
		for (cp = argv[1]; *cp; cp++) {
			if (*cp < '0' || *cp > '7') {
				symbolic = 1;
				break;
			}
		}
		if (symbolic) {
			umask(um = mode = umask(0));
			mode = newmode(argv[1], ~mode);
			mode = (~mode & 0777);
			umask(mode);
		} else {
			mode = 0;
			while ((i = *argv[1]++) >= '0' && i <= '7')
				mode = (mode << 3) + i - '0';
			umask(mode);
		}
	}
}

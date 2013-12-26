/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2004
 *	Gunnar Ritter.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Gunnar Ritter
 *	and his contributors.
 * 4. Neither the name of Gunnar Ritter nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GUNNAR RITTER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL GUNNAR RITTER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)imap_search.c	1.29 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <time.h>

/*
 * Mail -- a mail program
 *
 * Client-side implementation of the IMAP SEARCH command. This is used
 * for folders not located on IMAP servers, or for IMAP servers that do
 * not implement the SEARCH command.
 */

static enum itoken {
	ITBAD,
	ITEOD,
	ITBOL,
	ITEOL,
	ITAND,
	ITSET,
	ITALL,
	ITANSWERED,
	ITBCC,
	ITBEFORE,
	ITBODY,
	ITCC,
	ITDELETED,
	ITDRAFT,
	ITFLAGGED,
	ITFROM,
	ITHEADER,
	ITKEYWORD,
	ITLARGER,
	ITNEW,
	ITNOT,
	ITOLD,
	ITON,
	ITOR,
	ITRECENT,
	ITSEEN,
	ITSENTBEFORE,
	ITSENTON,
	ITSENTSINCE,
	ITSINCE,
	ITSMALLER,
	ITSUBJECT,
	ITTEXT,
	ITTO,
	ITUID,
	ITUNANSWERED,
	ITUNDELETED,
	ITUNDRAFT,
	ITUNFLAGGED,
	ITUNKEYWORD,
	ITUNSEEN
} itoken;

static unsigned long	inumber;
static void	*iargs[2];
static int	needheaders;

static struct itlex {
	const char	*s_string;
	enum itoken	s_token;
} strings[] = {
	{ "ALL",	ITALL },
	{ "ANSWERED",	ITANSWERED },
	{ "BCC",	ITBCC },
	{ "BEFORE",	ITBEFORE },
	{ "BODY",	ITBODY },
	{ "CC",		ITCC },
	{ "DELETED",	ITDELETED },
	{ "DRAFT",	ITDRAFT },
	{ "FLAGGED",	ITFLAGGED },
	{ "FROM",	ITFROM },
	{ "HEADER",	ITHEADER },
	{ "KEYWORD",	ITKEYWORD },
	{ "LARGER",	ITLARGER },
	{ "NEW",	ITNEW },
	{ "NOT",	ITNOT },
	{ "OLD",	ITOLD },
	{ "ON",		ITON },
	{ "OR",		ITOR },
	{ "RECENT",	ITRECENT },
	{ "SEEN",	ITSEEN },
	{ "SENTBEFORE",	ITSENTBEFORE },
	{ "SENTON",	ITSENTON },
	{ "SENTSINCE",	ITSENTSINCE },
	{ "SINCE",	ITSINCE },
	{ "SMALLER",	ITSMALLER },
	{ "SUBJECT",	ITSUBJECT },
	{ "TEXT",	ITTEXT },
	{ "TO",		ITTO },
	{ "UID",	ITUID },
	{ "UNANSWERED",	ITUNANSWERED },
	{ "UNDELETED",	ITUNDELETED },
	{ "UNDRAFT",	ITUNDRAFT },
	{ "UNFLAGGED",	ITUNFLAGGED },
	{ "UNKEYWORD",	ITUNKEYWORD },
	{ "UNSEEN",	ITUNSEEN },
	{ NULL,		ITBAD }
};

static struct itnode {
	enum itoken	n_token;
	unsigned long	n_n;
	void	*n_v;
	void	*n_w;
	struct itnode	*n_x;
	struct itnode	*n_y;
} *ittree;

static const char	*begin;

static enum okay itparse(const char *spec, char **xp, int sub);
static enum okay itscan(const char *spec, char **xp);
static enum okay itsplit(const char *spec, char **xp);
static enum okay itstring(void **tp, const char *spec, char **xp);
static int itexecute(struct mailbox *mp, struct message *m,
		int c, struct itnode *n);
static int matchfield(struct message *m, const char *field, const char *what);
static int matchenvelope(struct message *m, const char *field,
		const char *what);
static char *mkenvelope(struct name *np);
static int matchmsg(struct message *m, const char *what, int withheader);
static const char *around(const char *cp);

enum okay 
imap_search(const char *spec, int f)
{
	static char	*lastspec;
	char	*xp;
	int	i;

	if (strcmp(spec, "()")) {
		free(lastspec);
		lastspec = sstrdup(spec);
	} else if (lastspec == NULL) {
		fprintf(stderr, "No last SEARCH criteria available.\n");
		return STOP;
	} else
		spec = lastspec;
	begin = spec;
	if (imap_search1(spec, f) == OKAY)
		return OKAY;
	needheaders = 0;
	if (itparse(spec, &xp, 0) == STOP)
		return STOP;
	if (ittree == NULL)
		return OKAY;
	if (mb.mb_type == MB_IMAP && needheaders)
		imap_getheaders(1, msgCount);
	for (i = 0; i < msgCount; i++) {
		if (message[i].m_flag&MHIDDEN)
			continue;
		if (f == MDELETED || (message[i].m_flag&MDELETED) == 0)
			if (itexecute(&mb, &message[i], i+1, ittree))
				mark(i+1, f);
	}
	return OKAY;
}

static enum okay 
itparse(const char *spec, char **xp, int sub)
{
	int	level = 0;
	struct itnode	n, *z, *_ittree;
	enum okay	ok;

	ittree = NULL;
	while ((ok = itscan(spec, xp)) == OKAY && itoken != ITBAD &&
			itoken != ITEOD) {
		_ittree = ittree;
		memset(&n, 0, sizeof n);
		spec = *xp;
		switch (itoken) {
		case ITBOL:
			level++;
			continue;
		case ITEOL:
			if (--level == 0) {
				return OKAY;
			}
			if (level < 0) {
				if (sub > 0) {
					(*xp)--;
					return OKAY;
				}
				fprintf(stderr, "Excess in \")\".\n");
				return STOP;
			}
			continue;
		case ITNOT:
			/* <search-key> */
			n.n_token = ITNOT;
			if (itparse(spec, xp, sub+1) == STOP)
				return STOP;
			spec = *xp;
			if ((n.n_x = ittree) == NULL) {
				fprintf(stderr,
				"Criterion for NOT missing: >>> %s <<<\n",
					around(*xp));
				return STOP;
			}
			itoken = ITNOT;
			break;
		case ITOR:
			/* <search-key1> <search-key2> */
			n.n_token = ITOR;
			if (itparse(spec, xp, sub+1) == STOP)
				return STOP;
			if ((n.n_x = ittree) == NULL) {
				fprintf(stderr, "First criterion for OR "
						"missing: >>> %s <<<\n",
						around(*xp));
				return STOP;
			}
			spec = *xp;
			if (itparse(spec, xp, sub+1) == STOP)
				return STOP;
			spec = *xp;
			if ((n.n_y = ittree) == NULL) {
				fprintf(stderr, "Second criterion for OR "
						"missing: >>> %s <<<\n",
						around(*xp));
				return STOP;
			}
			break;
		default:
			n.n_token = itoken;
			n.n_n = inumber;
			n.n_v = iargs[0];
			n.n_w = iargs[1];
		}
		ittree = _ittree;
		if (ittree == NULL) {
			ittree = salloc(sizeof *ittree);
			*ittree = n;
		} else {
			z = ittree;
			ittree = salloc(sizeof *ittree);
			ittree->n_token = ITAND;
			ittree->n_x = z;
			ittree->n_y = salloc(sizeof*ittree->n_y);
			*ittree->n_y = n;
		}
		if (sub && level == 0)
			break;
	}
	return ok;
}

static enum okay 
itscan(const char *spec, char **xp)
{
	int	i, n;

	while (spacechar(*spec&0377))
		spec++;
	if (*spec == '(') {
		*xp = (char *)&spec[1];
		itoken = ITBOL;
		return OKAY;
	}
	if (*spec == ')') {
		*xp = (char *)&spec[1];
		itoken = ITEOL;
		return OKAY;
	}
	while (spacechar(*spec&0377))
		spec++;
	if (*spec == '\0') {
		itoken = ITEOD;
		return OKAY;
	}
	for (i = 0; strings[i].s_string; i++) {
		n = strlen(strings[i].s_string);
		if (ascncasecmp(spec, strings[i].s_string, n) == 0 &&
				(spacechar(spec[n]&0377) || spec[n] == '\0'
				 || spec[n] == '(' || spec[n] == ')')) {
			itoken = strings[i].s_token;
			spec += n;
			while (spacechar(*spec&0377))
				spec++;
			return itsplit(spec, xp);
		}
	}
	if (digitchar(*spec&0377)) {
		inumber = strtoul(spec, xp, 10);
		if (spacechar(**xp&0377) || **xp == '\0' ||
				**xp == '(' || **xp == ')') {
			itoken = ITSET;
			return OKAY;
		}
	}
	fprintf(stderr, "Bad SEARCH criterion \"");
	while (*spec && !spacechar(*spec&0377) &&
			*spec != '(' && *spec != ')') {
		putc(*spec&0377, stderr);
		spec++;
	}
	fprintf(stderr, "\": >>> %s <<<\n", around(*xp));
	itoken = ITBAD;
	return STOP;
}

static enum okay 
itsplit(const char *spec, char **xp)
{
	char	*cp;
	time_t	t;

	switch (itoken) {
	case ITBCC:
	case ITBODY:
	case ITCC:
	case ITFROM:
	case ITSUBJECT:
	case ITTEXT:
	case ITTO:
		/* <string> */
		needheaders++;
		return itstring(&iargs[0], spec, xp);
	case ITSENTBEFORE:
	case ITSENTON:
	case ITSENTSINCE:
		needheaders++;
		/*FALLTHRU*/
	case ITBEFORE:
	case ITON:
	case ITSINCE:
		/* <date> */
		if (itstring(&iargs[0], spec, xp) != OKAY)
			return STOP;
		if ((t = imap_read_date(iargs[0])) == (time_t)-1) {
			fprintf(stderr, "Invalid date \"%s\": >>> %s <<<\n",
					(char *)iargs[0], around(*xp));
			return STOP;
		}
		inumber = t;
		return OKAY;
	case ITHEADER:
		/* <field-name> <string> */
		needheaders++;
		if (itstring(&iargs[0], spec, xp) != OKAY)
			return STOP;
		spec = *xp;
		return itstring(&iargs[1], spec, xp);
	case ITKEYWORD:
	case ITUNKEYWORD:
		/* <flag> */
		if (itstring(&iargs[0], spec, xp) != OKAY)
			return STOP;
		if (asccasecmp(iargs[0], "\\Seen") == 0)
			inumber = MREAD;
		else if (asccasecmp(iargs[0], "\\Deleted") == 0)
			inumber = MDELETED;
		else if (asccasecmp(iargs[0], "\\Recent") == 0)
			inumber = MNEW;
		else if (asccasecmp(iargs[0], "\\Flagged") == 0)
			inumber = MFLAGGED;
		else if (asccasecmp(iargs[0], "\\Answered") == 0)
			inumber = MANSWERED;
		else if (asccasecmp(iargs[0], "\\Draft") == 0)
			inumber = MDRAFT;
		else
			inumber = 0;
		return OKAY;
	case ITLARGER:
	case ITSMALLER:
		/* <n> */
		if (itstring(&iargs[0], spec, xp) != OKAY)
			return STOP;
		inumber = strtoul(iargs[0], &cp, 10);
		if (spacechar(*cp&0377) || *cp == '\0')
			return OKAY;
		fprintf(stderr, "Invalid size: >>> %s <<<\n",
				around(*xp));
		return STOP;
	case ITUID:
		/* <message set> */
		fprintf(stderr,
			"Searching for UIDs is not supported: >>> %s <<<\n",
			around(*xp));
		return STOP;
	default:
		*xp = (char *)spec;
		return OKAY;
	}
}

static enum okay 
itstring(void **tp, const char *spec, char **xp)
{
	int	inquote = 0;
	char	*ap;

	while (spacechar(*spec&0377))
		spec++;
	if (*spec == '\0' || *spec == '(' || *spec == ')') {
		fprintf(stderr, "Missing string argument: >>> %s <<<\n",
				around(&(*xp)[spec - *xp]));
		return STOP;
	}
	ap = *tp = salloc(strlen(spec) + 1);
	*xp = (char *)spec;
	 do {
		if (inquote && **xp == '\\')
			*ap++ = *(*xp)++;
		else if (**xp == '"')
			inquote = !inquote;
		else if (!inquote && (spacechar(**xp&0377) ||
				**xp == '(' || **xp == ')')) {
			*ap++ = '\0';
			break;
		}
		*ap++ = **xp;
	} while (*(*xp)++);
	return OKAY;
}

static int 
itexecute(struct mailbox *mp, struct message *m, int c, struct itnode *n)
{
	char	*cp, *line = NULL;
	size_t	linesize = 0;
	FILE	*ibuf;

	if (n == NULL) {
		fprintf(stderr, "Internal error: Empty node in SEARCH tree.\n");
		return 0;
	}
	switch (n->n_token) {
	case ITBEFORE:
	case ITON:
	case ITSINCE:
		if (m->m_time == 0 && (m->m_flag&MNOFROM) == 0 &&
				(ibuf = setinput(mp, m, NEED_HEADER)) != NULL) {
			if (readline(ibuf, &line, &linesize) > 0)
				m->m_time = unixtime(line);
			free(line);
		}
		break;
	case ITSENTBEFORE:
	case ITSENTON:
	case ITSENTSINCE:
		if (m->m_date == 0)
			if ((cp = hfield("date", m)) != NULL)
				m->m_date = rfctime(cp);
		break;
	default:
		break;
	}
	switch (n->n_token) {
	default:
		fprintf(stderr, "Internal SEARCH error: Lost token %d\n",
				n->n_token);
		return 0;
	case ITAND:
		return itexecute(mp, m, c, n->n_x) &
			itexecute(mp, m, c, n->n_y);
	case ITSET:
		return c == n->n_n;
	case ITALL:
		return 1;
	case ITANSWERED:
		return (m->m_flag&MANSWERED) != 0;
	case ITBCC:
		return matchenvelope(m, "bcc", n->n_v);
	case ITBEFORE:
		return m->m_time < n->n_n;
	case ITBODY:
		return matchmsg(m, n->n_v, 0);
	case ITCC:
		return matchenvelope(m, "cc", n->n_v);
	case ITDELETED:
		return (m->m_flag&MDELETED) != 0;
	case ITDRAFT:
		return (m->m_flag&MDRAFTED) != 0;
	case ITFLAGGED:
		return (m->m_flag&MFLAGGED) != 0;
	case ITFROM:
		return matchenvelope(m, "from", n->n_v);
	case ITHEADER:
		return matchfield(m, n->n_v, n->n_w);
	case ITKEYWORD:
		return (m->m_flag & n->n_n) != 0;
	case ITLARGER:
		return m->m_xsize > n->n_n;
	case ITNEW:
		return (m->m_flag&(MNEW|MREAD)) == MNEW;
	case ITNOT:
		return !itexecute(mp, m, c, n->n_x);
	case ITOLD:
		return (m->m_flag&MNEW) == 0;
	case ITON:
		return m->m_time >= n->n_n && m->m_time < n->n_n + 86400;
	case ITOR:
		return itexecute(mp, m, c, n->n_x) |
			itexecute(mp, m, c, n->n_y);
	case ITRECENT:
		return (m->m_flag&MNEW) != 0;
	case ITSEEN:
		return (m->m_flag&MREAD) != 0;
	case ITSENTBEFORE:
		return m->m_date < n->n_n;
	case ITSENTON:
		return m->m_date >= n->n_n && m->m_date < n->n_n + 86400;
	case ITSENTSINCE:
		return m->m_date >= n->n_n;
	case ITSINCE:
		return m->m_time >= n->n_n;
	case ITSMALLER:
		return m->m_xsize < n->n_n;
	case ITSUBJECT:
		return matchfield(m, "subject", n->n_v);
	case ITTEXT:
		return matchmsg(m, n->n_v, 1);
	case ITTO:
		return matchenvelope(m, "to", n->n_v);
	case ITUNANSWERED:
		return (m->m_flag&MANSWERED) == 0;
	case ITUNDELETED:
		return (m->m_flag&MDELETED) == 0;
	case ITUNDRAFT:
		return (m->m_flag&MDRAFTED) == 0;
	case ITUNFLAGGED:
		return (m->m_flag&MFLAGGED) == 0;
	case ITUNKEYWORD:
		return (m->m_flag & n->n_n) == 0;
	case ITUNSEEN:
		return (m->m_flag&MREAD) == 0;
	}
}

static int 
matchfield(struct message *m, const char *field, const char *what)
{
	struct str	in, out;
	int	i;

	if ((in.s = hfield(imap_unquotestr(field), m)) == NULL)
		return 0;
	in.l = strlen(in.s);
	mime_fromhdr(&in, &out, TD_ICONV);
	what = imap_unquotestr(what);
	i = substr(out.s, what);
	free(out.s);
	return i;
}

static int 
matchenvelope(struct message *m, const char *field, const char *what)
{
	struct name	*np;
	char	*cp;

	if ((cp = hfield(imap_unquotestr(field), m)) == NULL)
		return 0;
	what = imap_unquotestr(what);
	np = sextract(cp, GFULL);
	while (np) {
		if (substr(np->n_name, what))
			return 1;
		if (substr(mkenvelope(np), what))
			return 1;
		np = np->n_flink;
	}
	return 0;
}

static char *
mkenvelope(struct name *np)
{
	size_t	epsize;
	char	*ep;
	char	*realname = NULL, *sourceaddr = NULL,
		*localpart = NULL, *domainpart = NULL,
		*cp, *rp, *xp, *ip;
	struct str	in, out;
	int	level = 0, hadphrase = 0;

	in.s = np->n_fullname;
	in.l = strlen(in.s);
	mime_fromhdr(&in, &out, TD_ICONV);
	rp = ip = ac_alloc(strlen(out.s) + 1);
	for (cp = out.s; *cp; cp++) {
		switch (*cp) {
		case '"':
			while (*cp) {
				if (*++cp == '"')
					break;
				if (*cp == '\\' && cp[1])
					cp++;
				*rp++ = *cp;
			}
			break;
		case '<':
			while (cp > out.s && blankchar(cp[-1]&0377))
				cp--;
			rp = ip;
			xp = out.s;
			if (xp < &cp[-1] && *xp == '"' && cp[-1] == '"') {
				xp++;
				cp--;
			}
			while (xp < cp)
				*rp++ = *xp++;
			hadphrase = 1;
			goto done;
		case '(':
			if (level++)
				goto dfl;
			if (hadphrase++ == 0)
				rp = ip;
			break;
		case ')':
			if (--level)
				goto dfl;
			break;
		case '\\':
			if (level && cp[1])
				cp++;
			goto dfl;
		default:
		dfl:
			*rp++ = *cp;
		}
	}
done:	*rp = '\0';
	if (hadphrase)
		realname = ip;
	free(out.s);
	localpart = savestr(np->n_name);
	if ((cp = strrchr(localpart, '@')) != NULL) {
		*cp = '\0';
		domainpart = &cp[1];
	}
	ep = salloc(epsize = strlen(np->n_fullname) * 2 + 40);
	snprintf(ep, epsize, "(%s %s %s %s)",
			realname ? imap_quotestr(realname) : "NIL",
			sourceaddr ? imap_quotestr(sourceaddr) : "NIL",
			localpart ? imap_quotestr(localpart) : "NIL",
			domainpart ? imap_quotestr(domainpart) : "NIL");
	ac_free(ip);
	return ep;
}

static int 
matchmsg(struct message *m, const char *what, int withheader)
{
	char	*tempFile, *line = NULL;
	size_t	linesize, linelen, count;
	FILE	*fp;
	int	yes = 0;

	if ((fp = Ftemp(&tempFile, "Ra", "w+", 0600, 1)) == NULL)
		return 0;
	rm(tempFile);
	Ftfree(&tempFile);
	if (send(m, fp, NULL, NULL, SEND_TOSRCH, NULL) < 0)
		goto out;
	fflush(fp);
	rewind(fp);
	count = fsize(fp);
	line = smalloc(linesize = LINESIZE);
	linelen = 0;
	if (!withheader)
		while (fgetline(&line, &linesize, &count, &linelen, fp, 0))
			if (*line == '\n')
				break;
	what = imap_unquotestr(what);
	while (fgetline(&line, &linesize, &count, &linelen, fp, 0))
		if (substr(line, what)) {
			yes = 1;
			break;
		}
out:
	free(line);
	Fclose(fp);
	return yes;
}

#define	SURROUNDING	16
static const char *
around(const char *cp)
{
	int	i;
	static char	ab[2*SURROUNDING+1];

	for (i = 0; i < SURROUNDING && cp > begin; i++)
		cp--;
	for (i = 0; i < sizeof ab - 1; i++)
		ab[i] = *cp++;
	ab[i] = '\0';
	return ab;
}

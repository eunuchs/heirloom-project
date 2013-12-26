/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
static char sccsid[] = "@(#)head.c	2.17 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <time.h>

/*
 * Mail -- a mail program
 *
 * Routines for processing and detecting headlines.
 */

static char *copyin(char *src, char **space);
static char *nextword(char *wp, char *wbuf);
static int gethfield(FILE *f, char **linebuf, size_t *linesize, int rem,
		char **colon);
static int msgidnextc(const char **cp, int *status);
static int charcount(char *str, int c);

/*
 * See if the passed line buffer is a mail header.
 * Return true if yes.  POSIX.2 leaves the content
 * following 'From ' unspecified, so don't care about
 * it.
 */
/*ARGSUSED 2*/
int
is_head(char *linebuf, size_t linelen)
{
	char *cp;

	cp = linebuf;
	if (*cp++ != 'F' || *cp++ != 'r' || *cp++ != 'o' || *cp++ != 'm' ||
	    *cp++ != ' ')
		return (0);
	return(1);
}

/*
 * Split a headline into its useful components.
 * Copy the line into dynamic string space, then set
 * pointers into the copied line in the passed headline
 * structure.  Actually, it scans.
 */
void
parse(char *line, size_t linelen, struct headline *hl, char *pbuf)
{
	char *cp;
	char *sp;
	char *word;

	hl->l_from = NULL;
	hl->l_tty = NULL;
	hl->l_date = NULL;
	cp = line;
	sp = pbuf;
	word = ac_alloc(linelen + 1);
	/*
	 * Skip over "From" first.
	 */
	cp = nextword(cp, word);
	cp = nextword(cp, word);
	if (*word)
		hl->l_from = copyin(word, &sp);
	if (cp != NULL && cp[0] == 't' && cp[1] == 't' && cp[2] == 'y') {
		cp = nextword(cp, word);
		hl->l_tty = copyin(word, &sp);
	}
	if (cp != NULL)
		hl->l_date = copyin(cp, &sp);
	else
		hl->l_date = catgets(catd, CATSET, 213, "<Unknown date>");
	ac_free(word);
}

/*
 * Copy the string on the left into the string on the right
 * and bump the right (reference) string pointer by the length.
 * Thus, dynamically allocate space in the right string, copying
 * the left string into it.
 */
static char *
copyin(char *src, char **space)
{
	char *cp;
	char *top;

	top = cp = *space;
	while ((*cp++ = *src++) != '\0')
		;
	*space = cp;
	return (top);
}

#ifdef	notdef
static int	cmatch(char *, char *);
/*
 * Test to see if the passed string is a ctime(3) generated
 * date string as documented in the manual.  The template
 * below is used as the criterion of correctness.
 * Also, we check for a possible trailing time zone using
 * the tmztype template.
 */

/*
 * 'A'	An upper case char
 * 'a'	A lower case char
 * ' '	A space
 * '0'	A digit
 * 'O'	An optional digit or space
 * ':'	A colon
 * '+'	A sign
 * 'N'	A new line
 */
static char  *tmztype[] = {
	"Aaa Aaa O0 00:00:00 0000",
	"Aaa Aaa O0 00:00 0000",
	"Aaa Aaa O0 00:00:00 AAA 0000",
	"Aaa Aaa O0 00:00 AAA 0000",
	/*
	 * Sommer time, e.g. MET DST
	 */
	"Aaa Aaa O0 00:00:00 AAA AAA 0000",
	"Aaa Aaa O0 00:00 AAA AAA 0000",
	/*
	 * time zone offset, e.g.
	 * +0200 or +0200 MET or +0200 MET DST
	 */
	"Aaa Aaa O0 00:00:00 +0000 0000",
	"Aaa Aaa O0 00:00 +0000 0000",
	"Aaa Aaa O0 00:00:00 +0000 AAA 0000",
	"Aaa Aaa O0 00:00 +0000 AAA 0000",
	"Aaa Aaa O0 00:00:00 +0000 AAA AAA 0000",
	"Aaa Aaa O0 00:00 +0000 AAA AAA 0000",
	/*
	 * time zone offset without time zone specification (pine)
	 */
	"Aaa Aaa O0 00:00:00 0000 +0000",
	NULL,
};

static int 
is_date(char *date)
{
	int ret = 0, form = 0;

	while (tmztype[form]) {
		if ( (ret = cmatch(date, tmztype[form])) == 1 )
			break;
		form++;
	}

	return ret;
}

/*
 * Match the given string (cp) against the given template (tp).
 * Return 1 if they match, 0 if they don't
 */
static int 
cmatch(char *cp, char *tp)
{
	int c;

	while (*cp && *tp)
		switch (*tp++) {
		case 'a':
			if (c = *cp++, !lowerchar(c))
				return 0;
			break;
		case 'A':
			if (c = *cp++, !upperchar(c))
				return 0;
			break;
		case ' ':
			if (*cp++ != ' ')
				return 0;
			break;
		case '0':
			if (c = *cp++, !digitchar(c))
				return 0;
			break;
		case 'O':
			if (c = *cp, c != ' ' && !digitchar(c))
				return 0;
			cp++;
			break;
		case ':':
			if (*cp++ != ':')
				return 0;
			break;
		case '+':
			if (*cp != '+' && *cp != '-')
				return 0;
			cp++;
			break;
		case 'N':
			if (*cp++ != '\n')
				return 0;
			break;
		}
	if (*cp || *tp)
		return 0;
	return (1);
}
#endif	/* notdef */

/*
 * Collect a liberal (space, tab delimited) word into the word buffer
 * passed.  Also, return a pointer to the next word following that,
 * or NULL if none follow.
 */
static char *
nextword(char *wp, char *wbuf)
{
	int c;

	if (wp == NULL) {
		*wbuf = 0;
		return (NULL);
	}
	while ((c = *wp++) != '\0' && !blankchar(c)) {
		*wbuf++ = c;
		if (c == '"') {
 			while ((c = *wp++) != '\0' && c != '"')
 				*wbuf++ = c;
 			if (c == '"')
 				*wbuf++ = c;
			else
				wp--;
 		}
	}
	*wbuf = '\0';
	for (; blankchar(c); c = *wp++)
		;
	if (c == 0)
		return (NULL);
	return (wp - 1);
}

void
extract_header(FILE *fp, struct header *hp)
{
	char *linebuf = NULL;
	size_t linesize = 0;
	int seenfields = 0;
	char *colon, *cp, *value;
	struct header nh;
	struct header *hq = &nh;
	int lc, c;

	memset(hq, 0, sizeof *hq);
	for (lc = 0; readline(fp, &linebuf, &linesize) > 0; lc++);
	rewind(fp);
	while ((lc = gethfield(fp, &linebuf, &linesize, lc, &colon)) >= 0) {
		if ((value = thisfield(linebuf, "to")) != NULL) {
			seenfields++;
			hq->h_to = checkaddrs(cat(hq->h_to,
					sextract(value, GTO|GFULL)));
		} else if ((value = thisfield(linebuf, "cc")) != NULL) {
			seenfields++;
			hq->h_cc = checkaddrs(cat(hq->h_cc,
					sextract(value, GCC|GFULL)));
		} else if ((value = thisfield(linebuf, "bcc")) != NULL) {
			seenfields++;
			hq->h_bcc = checkaddrs(cat(hq->h_bcc,
					sextract(value, GBCC|GFULL)));
		} else if ((value = thisfield(linebuf, "from")) != NULL) {
			seenfields++;
			hq->h_from = checkaddrs(cat(hq->h_from,
					sextract(value, GEXTRA|GFULL)));
		} else if ((value = thisfield(linebuf, "reply-to")) != NULL) {
			seenfields++;
			hq->h_replyto = checkaddrs(cat(hq->h_replyto,
					sextract(value, GEXTRA|GFULL)));
		} else if ((value = thisfield(linebuf, "sender")) != NULL) {
			seenfields++;
			hq->h_sender = checkaddrs(cat(hq->h_sender,
					sextract(value, GEXTRA|GFULL)));
		} else if ((value = thisfield(linebuf,
						"organization")) != NULL) {
			seenfields++;
			for (cp = value; blankchar(*cp & 0377); cp++);
			hq->h_organization = hq->h_organization ?
				save2str(hq->h_organization, cp) :
				savestr(cp);
		} else if ((value = thisfield(linebuf, "subject")) != NULL ||
				(value = thisfield(linebuf, "subj")) != NULL) {
			seenfields++;
			for (cp = value; blankchar(*cp & 0377); cp++);
			hq->h_subject = hq->h_subject ?
				save2str(hq->h_subject, cp) :
				savestr(cp);
		} else
			fprintf(stderr, catgets(catd, CATSET, 266,
					"Ignoring header field \"%s\"\n"),
					linebuf);
	}
	/*
	 * In case the blank line after the header has been edited out.
	 * Otherwise, fetch the header separator.
	 */
	if (linebuf) {
		if (linebuf[0] != '\0') {
			for (cp = linebuf; *(++cp) != '\0'; );
			fseek(fp, (long)-(1 + cp - linebuf), SEEK_CUR);
		} else {
			if ((c = getc(fp)) != '\n' && c != EOF)
				ungetc(c, fp);
		}
	}
	if (seenfields) {
		hp->h_to = hq->h_to;
		hp->h_cc = hq->h_cc;
		hp->h_bcc = hq->h_bcc;
		hp->h_from = hq->h_from;
		hp->h_replyto = hq->h_replyto;
		hp->h_sender = hq->h_sender;
		hp->h_organization = hq->h_organization;
		hp->h_subject = hq->h_subject;
	} else
		fprintf(stderr, catgets(catd, CATSET, 267,
				"Restoring deleted header lines\n"));
	if (linebuf)
		free(linebuf);
}

/*
 * Return the desired header line from the passed message
 * pointer (or NULL if the desired header field is not available).
 * If mult is zero, return the content of the first matching header
 * field only, the content of all matching header fields else.
 */
char *
hfield_mult(char *field, struct message *mp, int mult)
{
	FILE *ibuf;
	char *linebuf = NULL;
	size_t linesize = 0;
	int lc;
	char *hfield;
	char *colon, *oldhfield = NULL;

	if ((ibuf = setinput(&mb, mp, NEED_HEADER)) == NULL)
		return NULL;
	if ((lc = mp->m_lines - 1) < 0)
		return NULL;
	if ((mp->m_flag & MNOFROM) == 0) {
		if (readline(ibuf, &linebuf, &linesize) < 0) {
			if (linebuf)
				free(linebuf);
			return NULL;
		}
	}
	while (lc > 0) {
		if ((lc = gethfield(ibuf, &linebuf, &linesize, lc, &colon))
				< 0) {
			if (linebuf)
				free(linebuf);
			return oldhfield;
		}
		if ((hfield = thisfield(linebuf, field)) != NULL) {
			oldhfield = save2str(hfield, oldhfield);
			if (mult == 0)
				break;
		}
	}
	if (linebuf)
		free(linebuf);
	return oldhfield;
}

/*
 * Return the next header field found in the given message.
 * Return >= 0 if something found, < 0 elsewise.
 * "colon" is set to point to the colon in the header.
 * Must deal with \ continuations & other such fraud.
 */
static int
gethfield(FILE *f, char **linebuf, size_t *linesize, int rem, char **colon)
{
	char *line2 = NULL;
	size_t line2size = 0;
	char *cp, *cp2;
	int c, isenc;

	if (*linebuf == NULL)
		*linebuf = srealloc(*linebuf, *linesize = 1);
	**linebuf = '\0';
	for (;;) {
		if (--rem < 0)
			return -1;
		if ((c = readline(f, linebuf, linesize)) <= 0)
			return -1;
		for (cp = *linebuf; fieldnamechar(*cp & 0377); cp++);
		if (cp > *linebuf)
			while (blankchar(*cp & 0377))
				cp++;
		if (*cp != ':' || cp == *linebuf)
			continue;
		/*
		 * I guess we got a headline.
		 * Handle wraparounding
		 */
		*colon = cp;
		cp = *linebuf + c;
		for (;;) {
			isenc = 0;
			while (--cp >= *linebuf && blankchar(*cp & 0377));
			cp++;
			if (rem <= 0)
				break;
			if (cp-8 >= *linebuf && cp[-1] == '=' && cp[-2] == '?')
				isenc |= 1;
			ungetc(c = getc(f), f);
			if (!blankchar(c))
				break;
			if ((c = readline(f, &line2, &line2size)) < 0)
				break;
			rem--;
			for (cp2 = line2; blankchar(*cp2 & 0377); cp2++);
			c -= cp2 - line2;
			if (cp2[0] == '=' && cp2[1] == '?' && c > 8)
				isenc |= 2;
			if (cp + c >= *linebuf + *linesize - 2) {
				size_t diff = cp - *linebuf;
				size_t colondiff = *colon - *linebuf;
				*linebuf = srealloc(*linebuf,
						*linesize += c + 2);
				cp = &(*linebuf)[diff];
				*colon = &(*linebuf)[colondiff];
			}
			if (isenc != 3)
				*cp++ = ' ';
			memcpy(cp, cp2, c);
			cp += c;
		}
		*cp = 0;
		if (line2)
			free(line2);
		return rem;
	}
	/* NOTREACHED */
}

/*
 * Check whether the passed line is a header line of
 * the desired breed.  Return the field body, or 0.
 */
char *
thisfield(const char *linebuf, const char *field)
{
	while (lowerconv(*linebuf&0377) == lowerconv(*field&0377)) {
		linebuf++;
		field++;
	}
	if (*field != '\0')
		return NULL;
	while (blankchar(*linebuf&0377))
		linebuf++;
	if (*linebuf++ != ':')
		return NULL;
	while (blankchar(*linebuf&0377))
		linebuf++;
	return (char *)linebuf;
}

/*
 * Get sender's name from this message.  If the message has
 * a bunch of arpanet stuff in it, we may have to skin the name
 * before returning it.
 */
char *
nameof(struct message *mp, int reptype)
{
	char *cp, *cp2;

	cp = skin(name1(mp, reptype));
	if (reptype != 0 || charcount(cp, '!') < 2)
		return(cp);
	cp2 = strrchr(cp, '!');
	cp2--;
	while (cp2 > cp && *cp2 != '!')
		cp2--;
	if (*cp2 == '!')
		return(cp2 + 1);
	return(cp);
}

/*
 * Start of a "comment".
 * Ignore it.
 */
char *
skip_comment(const char *cp)
{
	int nesting = 1;

	for (; nesting > 0 && *cp; cp++) {
		switch (*cp) {
		case '\\':
			if (cp[1])
				cp++;
			break;
		case '(':
			nesting++;
			break;
		case ')':
			nesting--;
			break;
		}
	}
	return (char *)cp;
}

/*
 * Return the start of a route-addr (address in angle brackets),
 * if present.
 */
char *
routeaddr(const char *name)
{
	const char	*np, *rp = NULL;

	for (np = name; *np; np++) {
		switch (*np) {
		case '(':
			np = skip_comment(&np[1]) - 1;
			break;
		case '"':
			while (*np) {
				if (*++np == '"')
					break;
				if (*np == '\\' && np[1])
					np++;
			}
			break;
		case '<':
			rp = np;
			break;
		case '>':
			return (char *)rp;
		}
	}
	return NULL;
}

/*
 * Skin an arpa net address according to the RFC 822 interpretation
 * of "host-phrase."
 */
char *
skin(char *name)
{
	int c;
	char *cp, *cp2;
	char *bufend;
	int gotlt, lastsp;
	char *nbuf;

	if (name == NULL)
		return(NULL);
	if (strchr(name, '(') == NULL && strchr(name, '<') == NULL
	    && strchr(name, ' ') == NULL)
		return(name);
	gotlt = 0;
	lastsp = 0;
	nbuf = ac_alloc(strlen(name) + 1);
	bufend = nbuf;
	for (cp = name, cp2 = bufend; (c = *cp++) != '\0'; ) {
		switch (c) {
		case '(':
			cp = skip_comment(cp);
			lastsp = 0;
			break;

		case '"':
			/*
			 * Start of a "quoted-string".
			 * Copy it in its entirety.
			 */
			*cp2++ = c;
			while ((c = *cp) != '\0') {
				cp++;
				if (c == '"') {
					*cp2++ = c;
					break;
				}
				if (c != '\\')
					*cp2++ = c;
				else if ((c = *cp) != '\0') {
					*cp2++ = c;
					cp++;
				}
			}
			lastsp = 0;
			break;

		case ' ':
			if (cp[0] == 'a' && cp[1] == 't' && cp[2] == ' ')
				cp += 3, *cp2++ = '@';
			else
			if (cp[0] == '@' && cp[1] == ' ')
				cp += 2, *cp2++ = '@';
#if 0
			/*
			 * RFC 822 specifies spaces are STRIPPED when
			 * in an adress specifier.
			 */
			else
				lastsp = 1;
#endif
			break;

		case '<':
			cp2 = bufend;
			gotlt++;
			lastsp = 0;
			break;

		case '>':
			if (gotlt) {
				gotlt = 0;
				while ((c = *cp) != '\0' && c != ',') {
					cp++;
					if (c == '(')
						cp = skip_comment(cp);
					else if (c == '"')
						while ((c = *cp) != '\0') {
							cp++;
							if (c == '"')
								break;
							if (c == '\\' && *cp)
								cp++;
						}
				}
				lastsp = 0;
				break;
			}
			/* Fall into . . . */

		default:
			if (lastsp) {
				lastsp = 0;
				*cp2++ = ' ';
			}
			*cp2++ = c;
			if (c == ',' && !gotlt) {
				*cp2++ = ' ';
				for (; *cp == ' '; cp++)
					;
				lastsp = 0;
				bufend = cp2;
			}
		}
	}
	*cp2 = 0;
	cp = savestr(nbuf);
	ac_free(nbuf);
	return cp;
}

/*
 * Fetch the real name from an internet mail address field.
 */
char *
realname(char *name)
{
	char	*cstart = NULL, *cend = NULL, *cp, *cq;
	char	*rname, *rp;
	struct str	in, out;
	int	quoted, good, nogood;

	if (name == NULL)
		return NULL;
	for (cp = name; *cp; cp++) {
		switch (*cp) {
		case '(':
			if (cstart)
				/*
				 * More than one comment in address, doesn't
				 * make sense to display it without context.
				 * Return the entire field,
				 */
				return mime_fromaddr(name);
			cstart = cp++;
			cp = skip_comment(cp);
			cend = cp--;
			if (cend <= cstart)
				cend = cstart = NULL;
			break;
		case '"':
			while (*cp) {
				if (*++cp == '"')
					break;
				if (*cp == '\\' && cp[1])
					cp++;
			}
			break;
		case '<':
			if (cp > name) {
				cstart = name;
				cend = cp;
			}
			break;
		case ',':
			/*
			 * More than one address. Just use the first one.
			 */
			goto brk;
		}
	}
brk:	if (cstart == NULL) {
		if (*name == '<')
			/*
			 * If name contains only a route-addr, the
			 * surrounding angle brackets don't serve any
			 * useful purpose when displaying, so they
			 * are removed.
			 */
			return prstr(skin(name));
		return mime_fromaddr(name);
	}
	rp = rname = ac_alloc(cend - cstart + 1);
	/*
	 * Strip quotes. Note that quotes that appear within a MIME-
	 * encoded word are not stripped. The idea is to strip only
	 * syntactical relevant things (but this is not necessarily
	 * the most sensible way in practice).
	 */
	quoted = 0;
	for (cp = cstart; cp < cend; cp++) {
		if (*cp == '(' && !quoted) {
			cq = skip_comment(++cp);
			if (--cq > cend)
				cq = cend;
			while (cp < cq) {
				if (*cp == '\\' && &cp[1] < cq)
					cp++;
				*rp++ = *cp++;
			}
		} else if (*cp == '\\' && &cp[1] < cend)
			*rp++ = *++cp;
		else if (*cp == '"') {
			quoted = !quoted;
			continue;
		} else
			*rp++ = *cp;
	}
	*rp = '\0';
	in.s = rname;
	in.l = rp - rname;
	mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
	ac_free(rname);
	rname = savestr(out.s);
	free(out.s);
	while (blankchar(*rname & 0377))
		rname++;
	for (rp = rname; *rp; rp++);
	while (--rp >= rname && blankchar(*rp & 0377))
		*rp = '\0';
	if (rp == rname)
		return mime_fromaddr(name);
	/*
	 * mime_fromhdr() has converted all nonprintable characters to
	 * question marks now. These and blanks are considered uninteresting;
	 * if the displayed part of the real name contains more than 25% of
	 * them, it is probably better to display the plain email address
	 * instead.
	 */
	good = 0;
	nogood = 0;
	for (rp = rname; *rp && rp < &rname[20]; rp++)
		if (*rp == '?' || blankchar(*rp & 0377))
			nogood++;
		else
			good++;
	if (good*3 < nogood)
		return prstr(skin(name));
	return rname;
}

/*
 * Fetch the sender's name from the passed message.
 * Reptype can be
 *	0 -- get sender's name for display purposes
 *	1 -- get sender's name for reply
 *	2 -- get sender's name for Reply
 */
char *
name1(struct message *mp, int reptype)
{
	char *namebuf;
	size_t namesize;
	char *linebuf = NULL;
	size_t linesize = 0;
	char *cp, *cp2;
	FILE *ibuf;
	int first = 1;

	if ((cp = hfield("from", mp)) != NULL && *cp != '\0')
		return cp;
	if (reptype == 0 && (cp = hfield("sender", mp)) != NULL &&
			*cp != '\0')
		return cp;
	namebuf = smalloc(namesize = 1);
	namebuf[0] = 0;
	if (mp->m_flag & MNOFROM)
		goto out;
	if ((ibuf = setinput(&mb, mp, NEED_HEADER)) == NULL)
		goto out;
	if (readline(ibuf, &linebuf, &linesize) < 0)
		goto out;
newname:
	if (namesize <= linesize)
		namebuf = srealloc(namebuf, namesize = linesize + 1);
	for (cp = linebuf; *cp && *cp != ' '; cp++)
		;
	for (; blankchar(*cp & 0377); cp++);
	for (cp2 = &namebuf[strlen(namebuf)];
	     *cp && !blankchar(*cp & 0377) && cp2 < namebuf + namesize - 1;)
		*cp2++ = *cp++;
	*cp2 = '\0';
	if (readline(ibuf, &linebuf, &linesize) < 0)
		goto out;
	if ((cp = strchr(linebuf, 'F')) == NULL)
		goto out;
	if (strncmp(cp, "From", 4) != 0)
		goto out;
	if (namesize <= linesize)
		namebuf = srealloc(namebuf, namesize = linesize + 1);
	while ((cp = strchr(cp, 'r')) != NULL) {
		if (strncmp(cp, "remote", 6) == 0) {
			if ((cp = strchr(cp, 'f')) == NULL)
				break;
			if (strncmp(cp, "from", 4) != 0)
				break;
			if ((cp = strchr(cp, ' ')) == NULL)
				break;
			cp++;
			if (first) {
				strncpy(namebuf, cp, namesize);
				first = 0;
			} else {
				cp2=strrchr(namebuf, '!')+1;
				strncpy(cp2, cp, (namebuf+namesize)-cp2);
			}
			namebuf[namesize-2]='\0';
			strcat(namebuf, "!");
			goto newname;
		}
		cp++;
	}
out:
	if (*namebuf != '\0' || ((cp = hfield("return-path", mp))) == NULL ||
			*cp == '\0')
		cp = savestr(namebuf);
	if (linebuf)
		free(linebuf);
	free(namebuf);
	return cp;
}

static int 
msgidnextc(const char **cp, int *status)
{
	int	c;

	for (;;) {
		if (*status & 01) {
			if (**cp == '"') {
				*status &= ~01;
				(*cp)++;
				continue;
			}
			if (**cp == '\\') {
				(*cp)++;
				if (**cp == '\0')
					goto eof;
			}
			goto dfl;
		}
		switch (**cp) {
		case '(':
			*cp = skip_comment(&(*cp)[1]);
			continue;
		case '>':
		case '\0':
		eof:
			return '\0';
		case '"':
			(*cp)++;
			*status |= 01;
			continue;
		case '@':
			*status |= 02;
			/*FALLTHRU*/
		default:
		dfl:
			c = *(*cp)++ & 0377;
			return *status & 02 ? lowerconv(c) : c;
		}
	}
}

int 
msgidcmp(const char *s1, const char *s2)
{
	int	q1 = 0, q2 = 0;
	int	c1, c2;

	do {
		c1 = msgidnextc(&s1, &q1);
		c2 = msgidnextc(&s2, &q2);
		if (c1 != c2)
			return c1 - c2;
	} while (c1 && c2);
	return c1 - c2;
}

/*
 * Count the occurances of c in str
 */
static int 
charcount(char *str, int c)
{
	char *cp;
	int i;

	for (i = 0, cp = str; *cp; cp++)
		if (*cp == c)
			i++;
	return(i);
}

/*
 * See if the given header field is supposed to be ignored.
 */
int
is_ign(char *field, size_t fieldlen, struct ignoretab ignore[2])
{
	char *realfld;
	int ret;

	if (ignore == NULL)
		return 0;
	if (ignore == allignore)
		return 1;
	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	realfld = ac_alloc(fieldlen + 1);
	i_strcpy(realfld, field, fieldlen + 1);
	if (ignore[1].i_count > 0)
		ret = !member(realfld, ignore + 1);
	else
		ret = member(realfld, ignore);
	ac_free(realfld);
	return ret;
}

int 
member(char *realfield, struct ignoretab *table)
{
	struct ignore *igp;

	for (igp = table->i_head[hash(realfield)]; igp != 0; igp = igp->i_link)
		if (*igp->i_field == *realfield &&
		    equal(igp->i_field, realfield))
			return (1);
	return (0);
}

/*
 * Fake Sender for From_ lines if missing, e. g. with POP3.
 */
char *
fakefrom(struct message *mp)
{
	char *name;

	if (((name = skin(hfield("return-path", mp))) == NULL ||
				*name == '\0' ) &&
			((name = skin(hfield("from", mp))) == NULL ||
				*name == '\0'))
		name = "-";
	return name;
}

char *
fakedate(time_t t)
{
	char *cp, *cq;

	cp = ctime(&t);
	for (cq = cp; *cq && *cq != '\n'; cq++);
	*cq = '\0';
	return savestr(cp);
}

char *
nexttoken(char *cp)
{
	for (;;) {
		if (*cp == '\0')
			return NULL;
		if (*cp == '(') {
			int nesting = 0;

			while (*cp != '\0') {
				switch (*cp++) {
				case '(':
					nesting++;
					break;
				case ')':
					nesting--;
					break;
				}
				if (nesting <= 0)
					break;
			}
		} else if (blankchar(*cp & 0377) || *cp == ',')
			cp++;
		else
			break;
	}
	return cp;
}

/*
 * From username Fri Jan  2 20:13:51 2004
 *               |    |    |    |    | 
 *               0    5   10   15   20
 */
time_t
unixtime(char *from)
{
	char	*fp, *xp;
	time_t	t;
	int	i, year, month, day, hour, minute, second;
	int	tzdiff;
	struct tm	*tmptr;

	for (fp = from; *fp && *fp != '\n'; fp++);
	fp -= 24;
	if (fp - from < 7)
		goto invalid;
	if (fp[3] != ' ')
		goto invalid;
	for (i = 0; month_names[i]; i++)
		if (strncmp(&fp[4], month_names[i], 3) == 0)
			break;
	if (month_names[i] == 0)
		goto invalid;
	month = i + 1;
	if (fp[7] != ' ')
		goto invalid;
	day = strtol(&fp[8], &xp, 10);
	if (*xp != ' ' || xp != &fp[10])
		goto invalid;
	hour = strtol(&fp[11], &xp, 10);
	if (*xp != ':' || xp != &fp[13])
		goto invalid;
	minute = strtol(&fp[14], &xp, 10);
	if (*xp != ':' || xp != &fp[16])
		goto invalid;
	second = strtol(&fp[17], &xp, 10);
	if (*xp != ' ' || xp != &fp[19])
		goto invalid;
	year = strtol(&fp[20], &xp, 10);
	if (xp != &fp[24])
		goto invalid;
	if ((t = combinetime(year, month, day, hour, minute, second)) ==
			(time_t)-1)
		goto invalid;
	tzdiff = t - mktime(gmtime(&t));
	tmptr = localtime(&t);
	if (tmptr->tm_isdst > 0)
		tzdiff += 3600;
	t -= tzdiff;
	return t;
invalid:
	time(&t);
	return t;
}

time_t
rfctime(char *date)
{
	char *cp = date, *x;
	time_t t;
	int i, year, month, day, hour, minute, second;

	if ((cp = nexttoken(cp)) == NULL)
		goto invalid;
	if (alphachar(cp[0] & 0377) && alphachar(cp[1] & 0377) &&
				alphachar(cp[2] & 0377) && cp[3] == ',') {
		if ((cp = nexttoken(&cp[4])) == NULL)
			goto invalid;
	}
	day = strtol(cp, &x, 10);
	if ((cp = nexttoken(x)) == NULL)
		goto invalid;
	for (i = 0; month_names[i]; i++) {
		if (strncmp(cp, month_names[i], 3) == 0)
			break;
	}
	if (month_names[i] == NULL)
		goto invalid;
	month = i + 1;
	if ((cp = nexttoken(&cp[3])) == NULL)
		goto invalid;
	year = strtol(cp, &x, 10);
	if ((cp = nexttoken(x)) == NULL)
		goto invalid;
	hour = strtol(cp, &x, 10);
	if (*x != ':')
		goto invalid;
	cp = &x[1];
	minute = strtol(cp, &x, 10);
	if (*x == ':') {
		cp = &x[1];
		second = strtol(cp, &x, 10);
	} else
		second = 0;
	if ((t = combinetime(year, month, day, hour, minute, second)) ==
			(time_t)-1)
		goto invalid;
	if ((cp = nexttoken(x)) != NULL) {
		int sign = -1;
		char buf[3];

		switch (*cp) {
		case '-':
			sign = 1;
			/*FALLTHRU*/
		case '+':
			cp++;
		}
		if (digitchar(cp[0] & 0377) && digitchar(cp[1] & 0377) &&
				digitchar(cp[2] & 0377) &&
				digitchar(cp[3] & 0377)) {
			buf[2] = '\0';
			buf[0] = cp[0];
			buf[1] = cp[1];
			t += strtol(buf, NULL, 10) * sign * 3600;
			buf[0] = cp[2];
			buf[1] = cp[3];
			t += strtol(buf, NULL, 10) * sign * 60;
		}
	}
	return t;
invalid:
	return 0;
}

#define	leapyear(year)	((year % 100 ? year : year / 100) % 4 == 0)

time_t
combinetime(int year, int month, int day, int hour, int minute, int second)
{
	time_t t;

	if (second < 0 || minute < 0 || hour < 0 || day < 1)
		return -1;
	t = second + minute * 60 + hour * 3600 + (day - 1) * 86400;
	if (year < 70)
		year += 2000;
	else if (year < 1900)
		year += 1900;
	if (month > 1)
		t += 86400 * 31;
	if (month > 2)
		t += 86400 * (leapyear(year) ? 29 : 28);
	if (month > 3)
		t += 86400 * 31;
	if (month > 4)
		t += 86400 * 30;
	if (month > 5)
		t += 86400 * 31;
	if (month > 6)
		t += 86400 * 30;
	if (month > 7)
		t += 86400 * 31;
	if (month > 8)
		t += 86400 * 31;
	if (month > 9)
		t += 86400 * 30;
	if (month > 10)
		t += 86400 * 31;
	if (month > 11)
		t += 86400 * 30;
	year -= 1900;
	t += (year - 70) * 31536000 + ((year - 69) / 4) * 86400 -
		((year - 1) / 100) * 86400 + ((year + 299) / 400) * 86400;
	return t;
}

void 
substdate(struct message *m)
{
	char *cp;
	time_t now;

	/*
	 * Determine the date to print in faked 'From ' lines. This is
	 * traditionally the date the message was written to the mail
	 * file. Try to determine this using RFC message header fields,
	 * or fall back to current time.
	 */
	time(&now);
	if ((cp = hfield_mult("received", m, 0)) != NULL) {
		while ((cp = nexttoken(cp)) != NULL && *cp != ';') {
			do
				cp++;
			while (alnumchar(*cp & 0377));
		}
		if (cp && *++cp)
			m->m_time = rfctime(cp);
	}
	if (m->m_time == 0 || m->m_time > now)
		if ((cp = hfield("date", m)) != NULL)
			m->m_time = rfctime(cp);
	if (m->m_time == 0 || m->m_time > now)
		m->m_time = now;
}

int
check_from_and_sender(struct name *fromfield, struct name *senderfield)
{
	if (fromfield && fromfield->n_flink && senderfield == NULL) {
		fprintf(stderr, "A Sender: field is required with multiple "
				"addresses in From: field.\n");
		return 1;
	}
	if (senderfield && senderfield->n_flink) {
		fprintf(stderr, "The Sender: field may contain "
				"only one address.\n");
		return 2;
	}
	return 0;
}

char *
getsender(struct message *mp)
{
	char	*cp;
	struct name	*np;

	if ((cp = hfield("from", mp)) == NULL ||
			(np = sextract(cp, GEXTRA|GSKIN)) == NULL)
		return NULL;
	return np->n_flink != NULL ? skin(hfield("sender", mp)) : np->n_name;
}

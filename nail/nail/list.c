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
static char sccsid[] = "@(#)list.c	2.62 (gritter) 12/11/08";
#endif
#endif /* not lint */

#include "rcv.h"
#include <ctype.h>
#include "extern.h"
#ifdef	HAVE_WCTYPE_H
#include <wctype.h>
#endif	/* HAVE_WCTYPE_H */

/*
 * Mail -- a mail program
 *
 * Message list handling.
 */

enum idfield {
	ID_REFERENCES,
	ID_IN_REPLY_TO
};

static char **add_to_namelist(char ***namelist, size_t *nmlsize,
		char **np, char *string);
static int markall(char *buf, int f);
static int evalcol(int col);
static int check(int mesg, int f);
static int scan(char **sp);
static void regret(int token);
static void scaninit(void);
static int matchsender(char *str, int mesg, int allnet);
static int matchmid(char *id, enum idfield idfield, int mesg);
static int matchsubj(char *str, int mesg);
static void unmark(int mesg);
static int metamess(int meta, int f);

static size_t	STRINGLEN;

static int	lexnumber;		/* Number of TNUMBER from scan() */
static char	*lexstring;		/* String from TSTRING, scan() */
static int	regretp;		/* Pointer to TOS of regret tokens */
static int	regretstack[REGDEP];	/* Stack of regretted tokens */
static char	*string_stack[REGDEP];	/* Stack of regretted strings */
static int	numberstack[REGDEP];	/* Stack of regretted numbers */
static int	threadflag;		/* mark entire threads */

/*
 * Convert the user string of message numbers and
 * store the numbers into vector.
 *
 * Returns the count of messages picked up or -1 on error.
 */
int 
getmsglist(char *buf, int *vector, int flags)
{
	int *ip;
	struct message *mp;
	int	mc;

	if (msgCount == 0) {
		*vector = 0;
		return 0;
	}
	if (markall(buf, flags) < 0)
		return(-1);
	ip = vector;
	if (inhook & 2) {
		mc = 0;
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if (mp->m_flag & MMARK) {
				if ((mp->m_flag & MNEWEST) == 0)
					unmark(mp - &message[0] + 1);
				else
					mc++;
			}
		if (mc == 0)
			return -1;
	}
	if (mb.mb_threaded == 0) {
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if (mp->m_flag & MMARK)
				*ip++ = mp - &message[0] + 1;
	} else {
		for (mp = threadroot; mp; mp = next_in_thread(mp))
			if (mp->m_flag & MMARK)
				*ip++ = mp - &message[0] + 1;
	}
	*ip = 0;
	return(ip - vector);
}

/*
 * Mark all messages that the user wanted from the command
 * line in the message structure.  Return 0 on success, -1
 * on error.
 */

/*
 * Bit values for colon modifiers.
 */

#define	CMNEW		01		/* New messages */
#define	CMOLD		02		/* Old messages */
#define	CMUNREAD	04		/* Unread messages */
#define	CMDELETED	010		/* Deleted messages */
#define	CMREAD		020		/* Read messages */
#define	CMFLAG		040		/* Flagged messages */
#define	CMANSWER	0100		/* Answered messages */
#define	CMDRAFT		0200		/* Draft messages */
#define	CMKILL		0400		/* Killed messages */
#define	CMJUNK		01000		/* Junk messages */

/*
 * The following table describes the letters which can follow
 * the colon and gives the corresponding modifier bit.
 */

static struct coltab {
	char	co_char;		/* What to find past : */
	int	co_bit;			/* Associated modifier bit */
	int	co_mask;		/* m_status bits to mask */
	int	co_equal;		/* ... must equal this */
} coltab[] = {
	{ 'n',		CMNEW,		MNEW,		MNEW },
	{ 'o',		CMOLD,		MNEW,		0 },
	{ 'u',		CMUNREAD,	MREAD,		0 },
	{ 'd',		CMDELETED,	MDELETED,	MDELETED },
	{ 'r',		CMREAD,		MREAD,		MREAD },
	{ 'f',		CMFLAG,		MFLAGGED,	MFLAGGED },
	{ 'a',		CMANSWER,	MANSWERED,	MANSWERED },
	{ 't',		CMDRAFT,	MDRAFTED,	MDRAFTED },
	{ 'k',		CMKILL,		MKILL,		MKILL },
	{ 'j',		CMJUNK,		MJUNK,		MJUNK },
	{ 0,		0,		0,		0 }
};

static	int	lastcolmod;

static char **
add_to_namelist(char ***namelist, size_t *nmlsize, char **np, char *string)
{
	size_t idx;

	if ((idx = np - *namelist) >= *nmlsize) {
		*namelist = srealloc(*namelist, (*nmlsize += 8) * sizeof *np);
		np = &(*namelist)[idx];
	}
	*np++ = string;
	return np;
}

#define	markall_ret(i)		{ \
					retval = i; \
					ac_free(lexstring); \
					goto out; \
				}

static int 
markall(char *buf, int f)
{
	char **np, **nq;
	int i, retval, gotheaders;
	struct message *mp, *mx;
	char **namelist, *bufp, *id = NULL, *cp;
	int tok, beg, mc, star, other, valdot, colmod, colresult, topen, tback;
	size_t nmlsize;
	enum idfield	idfield = ID_REFERENCES;

	lexstring = ac_alloc(STRINGLEN = 2 * strlen(buf) + 1);
	valdot = dot - &message[0] + 1;
	colmod = 0;
	for (i = 1; i <= msgCount; i++) {
		message[i-1].m_flag &= ~MOLDMARK;
		if (message[i-1].m_flag & MMARK)
			message[i-1].m_flag |= MOLDMARK;
		unmark(i);
	}
	bufp = buf;
	mc = 0;
	namelist = smalloc((nmlsize = 8) * sizeof *namelist);
	np = &namelist[0];
	scaninit();
	tok = scan(&bufp);
	star = 0;
	other = 0;
	beg = 0;
	topen = 0;
	tback = 0;
	gotheaders = 0;
	while (tok != TEOL) {
		switch (tok) {
		case TNUMBER:
number:
			if (star) {
				printf(catgets(catd, CATSET, 112,
					"No numbers mixed with *\n"));
				markall_ret(-1)
			}
			mc++;
			other++;
			if (beg != 0) {
				if (check(lexnumber, f))
					markall_ret(-1)
				i = beg;
				while (mb.mb_threaded ? 1 : i <= lexnumber) {
					if (!(message[i-1].m_flag&MHIDDEN) &&
							(f == MDELETED ||
							 (message[i-1].m_flag &
							  MDELETED) == 0))
						mark(i, f);
					if (mb.mb_threaded) {
						if (i == lexnumber)
							break;
						mx = next_in_thread(&message[i-1]);
						if (mx == NULL)
							markall_ret(-1)
						i = mx-message+1;
					} else
						i++;
				}
				beg = 0;
				break;
			}
			beg = lexnumber;
			if (check(beg, f))
				markall_ret(-1)
			tok = scan(&bufp);
			regret(tok);
			if (tok != TDASH) {
				mark(beg, f);
				beg = 0;
			}
			break;

		case TPLUS:
			if (beg != 0) {
				printf(catgets(catd, CATSET, 113,
					"Non-numeric second argument\n"));
				markall_ret(-1)
			}
			i = valdot;
			do {
				if (mb.mb_threaded) {
					mx = next_in_thread(&message[i-1]);
					i = mx ? mx-message+1 : msgCount+1;
				} else
					i++;
				if (i > msgCount) {
					printf(catgets(catd, CATSET, 114,
						"Referencing beyond EOF\n"));
					markall_ret(-1)
				}
			} while (message[i-1].m_flag == MHIDDEN ||
					(message[i-1].m_flag & MDELETED) != f ||
					message[i-1].m_flag & MKILL);
			mark(i, f);
			break;

		case TDASH:
			if (beg == 0) {
				i = valdot;
				do {
					if (mb.mb_threaded) {
						mx = prev_in_thread(
								&message[i-1]);
						i = mx ? mx-message+1 : 0;
					} else
						i--;
					if (i <= 0) {
						printf(catgets(catd, CATSET,
							115,
						"Referencing before 1\n"));
						markall_ret(-1)
					}
				} while (message[i-1].m_flag & MHIDDEN ||
						(message[i-1].m_flag & MDELETED)
							!= f ||
						message[i-1].m_flag & MKILL);
				mark(i, f);
			}
			break;

		case TSTRING:
			if (beg != 0) {
				printf(catgets(catd, CATSET, 116,
					"Non-numeric second argument\n"));
				markall_ret(-1)
			}
			other++;
			if (lexstring[0] == ':') {
				colresult = evalcol(lexstring[1]);
				if (colresult == 0) {
					printf(catgets(catd, CATSET, 117,
					"Unknown colon modifier \"%s\"\n"),
					    lexstring);
					markall_ret(-1)
				}
				colmod |= colresult;
			}
			else
				np = add_to_namelist(&namelist, &nmlsize,
						np, savestr(lexstring));
			break;

		case TOPEN:
			if (imap_search(lexstring, f) == STOP)
				markall_ret(-1)
			topen++;
			break;

		case TDOLLAR:
		case TUP:
		case TDOT:
		case TSEMI:
			lexnumber = metamess(lexstring[0], f);
			if (lexnumber == -1)
				markall_ret(-1)
			goto number;

		case TBACK:
			tback = 1;
			for (i = 1; i <= msgCount; i++) {
				if (message[i-1].m_flag&MHIDDEN ||
						(message[i-1].m_flag&MDELETED)
							!= f)
					continue;
				if (message[i-1].m_flag&MOLDMARK)
					mark(i, f);
			}
			break;

		case TSTAR:
			if (other) {
				printf(catgets(catd, CATSET, 118,
					"Can't mix \"*\" with anything\n"));
				markall_ret(-1)
			}
			star++;
			break;

		case TCOMMA:
			if (mb.mb_type == MB_IMAP && gotheaders++ == 0)
				imap_getheaders(1, msgCount);
			if (id == NULL && (cp = hfield("in-reply-to", dot))
					!= NULL) {
				id = savestr(cp);
				idfield = ID_IN_REPLY_TO;
			}
			if (id == NULL && (cp = hfield("references", dot))
					!= NULL) {
				struct name	*np;
				if ((np = extract(cp, GREF)) != NULL) {
					while (np->n_flink != NULL)
						np = np->n_flink;
					id = savestr(np->n_name);
					idfield = ID_REFERENCES;
				}
			}
			if (id == NULL) {
				printf(catgets(catd, CATSET, 227,
		"Cannot determine parent Message-ID of the current message\n"));
				markall_ret(-1)
			}
			break;

		case TERROR:
			markall_ret(-1)
		}
		threadflag = 0;
		tok = scan(&bufp);
	}
	lastcolmod = colmod;
	np = add_to_namelist(&namelist, &nmlsize, np, NULL);
	np--;
	mc = 0;
	if (star) {
		for (i = 0; i < msgCount; i++) {
			if (!(message[i].m_flag & MHIDDEN) &&
					(message[i].m_flag & MDELETED) == f) {
				mark(i+1, f);
				mc++;
			}
		}
		if (mc == 0) {
			if (!inhook)
				printf(catgets(catd, CATSET, 119,
					"No applicable messages.\n"));
			markall_ret(-1)
		}
		markall_ret(0)
	}

	if ((topen || tback) && mc == 0) {
		for (i = 0; i < msgCount; i++)
			if (message[i].m_flag & MMARK)
				mc++;
		if (mc == 0) {
			if (!inhook)
				printf(tback ?
					"No previously marked messages.\n" :
					"No messages satisfy (criteria).\n");
			markall_ret(-1)
		}
	}

	/*
	 * If no numbers were given, mark all of the messages,
	 * so that we can unmark any whose sender was not selected
	 * if any user names were given.
	 */

	if ((np > namelist || colmod != 0 || id) && mc == 0)
		for (i = 1; i <= msgCount; i++) {
			if (!(message[i-1].m_flag & MHIDDEN) &&
					(message[i-1].m_flag & MDELETED) == f)
				mark(i, f);
		}

	/*
	 * If any names were given, go through and eliminate any
	 * messages whose senders were not requested.
	 */

	if (np > namelist || id) {
		int	allnet = value("allnet") != NULL;

		if (mb.mb_type == MB_IMAP && gotheaders++ == 0)
			imap_getheaders(1, msgCount);
		for (i = 1; i <= msgCount; i++) {
			mc = 0;
			if (np > namelist) {
				for (nq = &namelist[0]; *nq != NULL; nq++) {
					if (**nq == '/') {
						if (matchsubj(*nq, i)) {
							mc++;
							break;
						}
					}
					else {
						if (matchsender(*nq, i,
								allnet)) {
							mc++;
							break;
						}
					}
				}
			}
			if (mc == 0 && id && matchmid(id, idfield, i))
				mc++;
			if (mc == 0)
				unmark(i);
		}

		/*
		 * Make sure we got some decent messages.
		 */

		mc = 0;
		for (i = 1; i <= msgCount; i++)
			if (message[i-1].m_flag & MMARK) {
				mc++;
				break;
			}
		if (mc == 0) {
			if (!inhook && np > namelist) {
				printf(catgets(catd, CATSET, 120,
					"No applicable messages from {%s"),
					namelist[0]);
				for (nq = &namelist[1]; *nq != NULL; nq++)
					printf(catgets(catd, CATSET, 121,
								", %s"), *nq);
				printf(catgets(catd, CATSET, 122, "}\n"));
			} else if (id) {
				printf(catgets(catd, CATSET, 227,
					"Parent message not found\n"));
			}
			markall_ret(-1)
		}
	}

	/*
	 * If any colon modifiers were given, go through and
	 * unmark any messages which do not satisfy the modifiers.
	 */

	if (colmod != 0) {
		for (i = 1; i <= msgCount; i++) {
			struct coltab *colp;

			mp = &message[i - 1];
			for (colp = &coltab[0]; colp->co_char; colp++)
				if (colp->co_bit & colmod)
					if ((mp->m_flag & colp->co_mask)
					    != colp->co_equal)
						unmark(i);
			
		}
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if (mp->m_flag & MMARK)
				break;
		if (mp >= &message[msgCount]) {
			struct coltab *colp;

			if (!inhook) {
				printf(catgets(catd, CATSET, 123,
						"No messages satisfy"));
				for (colp = &coltab[0]; colp->co_char; colp++)
					if (colp->co_bit & colmod)
						printf(" :%c", colp->co_char);
				printf("\n");
			}
			markall_ret(-1)
		}
	}
	markall_ret(0)
out:
	free(namelist);
	return retval;
}

/*
 * Turn the character after a colon modifier into a bit
 * value.
 */
static int 
evalcol(int col)
{
	struct coltab *colp;

	if (col == 0)
		return(lastcolmod);
	for (colp = &coltab[0]; colp->co_char; colp++)
		if (colp->co_char == col)
			return(colp->co_bit);
	return(0);
}

/*
 * Check the passed message number for legality and proper flags.
 * If f is MDELETED, then either kind will do.  Otherwise, the message
 * has to be undeleted.
 */
static int 
check(int mesg, int f)
{
	struct message *mp;

	if (mesg < 1 || mesg > msgCount) {
		printf(catgets(catd, CATSET, 124,
			"%d: Invalid message number\n"), mesg);
		return(-1);
	}
	mp = &message[mesg-1];
	if (mp->m_flag & MHIDDEN || (f != MDELETED &&
				(mp->m_flag & MDELETED) != 0)) {
		printf(catgets(catd, CATSET, 125,
			"%d: Inappropriate message\n"), mesg);
		return(-1);
	}
	return(0);
}

/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */
int
getrawlist(const char *line, size_t linesize, char **argv, int argc,
		int echolist)
{
	char c, *cp2, quotec;
	const char	*cp;
	int argn;
	char *linebuf;

	argn = 0;
	cp = line;
	linebuf = ac_alloc(linesize + 1);
	for (;;) {
		for (; blankchar(*cp & 0377); cp++);
		if (*cp == '\0')
			break;
		if (argn >= argc - 1) {
			printf(catgets(catd, CATSET, 126,
			"Too many elements in the list; excess discarded.\n"));
			break;
		}
		cp2 = linebuf;
		quotec = '\0';
		while ((c = *cp) != '\0') {
			cp++;
			if (quotec != '\0') {
				if (c == quotec) {
					quotec = '\0';
					if (echolist)
						*cp2++ = c;
				} else if (c == '\\')
					switch (c = *cp++) {
					case '\0':
						*cp2++ = '\\';
						cp--;
						break;
					/*
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						c -= '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						*cp2++ = c;
						break;
					case 'b':
						*cp2++ = '\b';
						break;
					case 'f':
						*cp2++ = '\f';
						break;
					case 'n':
						*cp2++ = '\n';
						break;
					case 'r':
						*cp2++ = '\r';
						break;
					case 't':
						*cp2++ = '\t';
						break;
					case 'v':
						*cp2++ = '\v';
						break;
					*/
					default:
						if (cp[-1]!=quotec || echolist)
							*cp2++ = '\\';
						*cp2++ = c;
					}
				/*else if (c == '^') {
					c = *cp++;
					if (c == '?')
						*cp2++ = '\177';
					/\* null doesn't show up anyway *\/
					else if ((c >= 'A' && c <= '_') ||
						 (c >= 'a' && c <= 'z'))
						*cp2++ = c & 037;
					else {
						*cp2++ = '^';
						cp--;
					}
				}*/ else
					*cp2++ = c;
			} else if (c == '"' || c == '\'') {
				if (echolist)
					*cp2++ = c;
				quotec = c;
			} else if (c == '\\' && !echolist) {
				if (*cp)
					*cp2++ = *cp++;
				else
					*cp2++ = c;
			} else if (blankchar(c & 0377))
				break;
			else
				*cp2++ = c;
		}
		*cp2 = '\0';
		argv[argn++] = savestr(linebuf);
	}
	argv[argn] = NULL;
	ac_free(linebuf);
	return argn;
}

/*
 * scan out a single lexical item and return its token number,
 * updating the string pointer passed **p.  Also, store the value
 * of the number or string scanned in lexnumber or lexstring as
 * appropriate.  In any event, store the scanned `thing' in lexstring.
 */

static struct lex {
	char	l_char;
	enum ltoken	l_token;
} singles[] = {
	{ '$',	TDOLLAR },
	{ '.',	TDOT },
	{ '^',	TUP },
	{ '*',	TSTAR },
	{ '-',	TDASH },
	{ '+',	TPLUS },
	{ '(',	TOPEN },
	{ ')',	TCLOSE },
	{ ',',	TCOMMA },
	{ ';',	TSEMI },
	{ '`',	TBACK },
	{ 0,	0 }
};

static int 
scan(char **sp)
{
	char *cp, *cp2;
	int c, level, inquote;
	struct lex *lp;
	int quotec;

	if (regretp >= 0) {
		strncpy(lexstring, string_stack[regretp], STRINGLEN);
		lexstring[STRINGLEN-1]='\0';
		lexnumber = numberstack[regretp];
		return(regretstack[regretp--]);
	}
	cp = *sp;
	cp2 = lexstring;
	c = *cp++;

	/*
	 * strip away leading white space.
	 */

	while (blankchar(c))
		c = *cp++;

	/*
	 * If no characters remain, we are at end of line,
	 * so report that.
	 */

	if (c == '\0') {
		*sp = --cp;
		return(TEOL);
	}

	/*
	 * Select members of a message thread.
	 */
	if (c == '&') {
		threadflag = 1;
		if (*cp == '\0' || spacechar(*cp&0377)) {
			lexstring[0] = '.';
			lexstring[1] = '\0';
			*sp = cp;
			return TDOT;
		}
		c = *cp++;
	}

	/*
	 * If the leading character is a digit, scan
	 * the number and convert it on the fly.
	 * Return TNUMBER when done.
	 */

	if (digitchar(c)) {
		lexnumber = 0;
		while (digitchar(c)) {
			lexnumber = lexnumber*10 + c - '0';
			*cp2++ = c;
			c = *cp++;
		}
		*cp2 = '\0';
		*sp = --cp;
		return(TNUMBER);
	}

	/*
	 * An IMAP SEARCH list. Note that TOPEN has always been included
	 * in singles[] in Mail and mailx. Thus although there is no formal
	 * definition for (LIST) lists, they do not collide with historical
	 * practice because a subject string (LIST) could never been matched
	 * this way.
	 */

	if (c == '(') {
		level = 1;
		inquote = 0;
		*cp2++ = c;
		do {
			if ((c = *cp++&0377) == '\0') {
			mtop:	fprintf(stderr, "Missing \")\".\n");
				return TERROR;
			}
			if (inquote && c == '\\') {
				*cp2++ = c;
				c = *cp++&0377;
				if (c == '\0')
					goto mtop;
			} else if (c == '"')
				inquote = !inquote;
			else if (inquote)
				/*EMPTY*/;
			else if (c == '(')
				level++;
			else if (c == ')')
				level--;
			else if (spacechar(c)) {
				/*
				 * Replace unquoted whitespace by single
				 * space characters, to make the string
				 * IMAP SEARCH conformant.
				 */
				c = ' ';
				if (cp2[-1] == ' ')
					cp2--;
			}
			*cp2++ = c;
		} while (c != ')' || level > 0);
		*cp2 = '\0';
		*sp = cp;
		return TOPEN;
	}

	/*
	 * Check for single character tokens; return such
	 * if found.
	 */

	for (lp = &singles[0]; lp->l_char != 0; lp++)
		if (c == lp->l_char) {
			lexstring[0] = c;
			lexstring[1] = '\0';
			*sp = cp;
			return(lp->l_token);
		}

	/*
	 * We've got a string!  Copy all the characters
	 * of the string into lexstring, until we see
	 * a null, space, or tab.
	 * If the lead character is a " or ', save it
	 * and scan until you get another.
	 */

	quotec = 0;
	if (c == '\'' || c == '"') {
		quotec = c;
		c = *cp++;
	}
	while (c != '\0') {
		if (quotec == 0 && c == '\\' && *cp)
			c = *cp++;
		if (c == quotec) {
			cp++;
			break;
		}
		if (quotec == 0 && blankchar(c))
			break;
		if (cp2 - lexstring < STRINGLEN-1)
			*cp2++ = c;
		c = *cp++;
	}
	if (quotec && c == 0) {
		fprintf(stderr, catgets(catd, CATSET, 127,
				"Missing %c\n"), quotec);
		return TERROR;
	}
	*sp = --cp;
	*cp2 = '\0';
	return(TSTRING);
}

/*
 * Unscan the named token by pushing it onto the regret stack.
 */
static void 
regret(int token)
{
	if (++regretp >= REGDEP)
		panic(catgets(catd, CATSET, 128, "Too many regrets"));
	regretstack[regretp] = token;
	lexstring[STRINGLEN-1] = '\0';
	string_stack[regretp] = savestr(lexstring);
	numberstack[regretp] = lexnumber;
}

/*
 * Reset all the scanner global variables.
 */
static void 
scaninit(void)
{
	regretp = -1;
	threadflag = 0;
}

/*
 * Find the first message whose flags & m == f  and return
 * its message number.
 */
int 
first(int f, int m)
{
	struct message *mp;

	if (msgCount == 0)
		return 0;
	f &= MDELETED;
	m &= MDELETED;
	for (mp = dot; mb.mb_threaded ? mp != NULL : mp < &message[msgCount];
			mb.mb_threaded ? mp = next_in_thread(mp) : mp++) {
		if (!(mp->m_flag & MHIDDEN) && (mp->m_flag & m) == f)
			return mp - message + 1;
	}
	if (dot > &message[0]) {
		for (mp = dot-1; mb.mb_threaded ?
					mp != NULL : mp >= &message[0];
				mb.mb_threaded ?
					mp = prev_in_thread(mp) : mp--) {
			if (!(mp->m_flag & MHIDDEN) && (mp->m_flag & m) == f)
				return mp - message + 1;
		}
	}
	return 0;
}

/*
 * See if the passed name sent the passed message number.  Return true
 * if so.
 */
static int 
matchsender(char *str, int mesg, int allnet)
{
	if (allnet) {
		char *cp = nameof(&message[mesg - 1], 0);

		do {
			if ((*cp == '@' || *cp == '\0') &&
					(*str == '@' || *str == '\0'))
				return 1;
			if (*cp != *str)
				break;
		} while (cp++, *str++ != '\0');
		return 0;
	}
	return !strcmp(str, (value("showname") ? realname : skin)
			(name1(&message[mesg - 1], 0)));
}

static int 
matchmid(char *id, enum idfield idfield, int mesg)
{
	struct name	*np;
	char *cp;

	if ((cp = hfield("message-id", &message[mesg - 1])) != NULL) {
		switch (idfield) {
		case ID_REFERENCES:
			return msgidcmp(id, cp) == 0;
		case ID_IN_REPLY_TO:
			if ((np = extract(id, GREF)) != NULL)
				do {
					if (msgidcmp(np->n_name, cp) == 0)
						return 1;
				} while ((np = np->n_flink) != NULL);
			break;
		}
	}
	return 0;
}

/*
 * See if the given string matches inside the subject field of the
 * given message.  For the purpose of the scan, we ignore case differences.
 * If it does, return true.  The string search argument is assumed to
 * have the form "/search-string."  If it is of the form "/," we use the
 * previous search string.
 */

static char lastscan[128];

static int 
matchsubj(char *str, int mesg)
{
	struct message *mp;
	char *cp, *cp2;
	struct str in, out;
	int	i;

	str++;
	if (strlen(str) == 0) {
		str = lastscan;
	} else {
		strncpy(lastscan, str, sizeof lastscan);
		lastscan[sizeof lastscan - 1]='\0';
	}
	mp = &message[mesg-1];
	
	/*
	 * Now look, ignoring case, for the word in the string.
	 */

	if (value("searchheaders") && (cp = strchr(str, ':'))) {
		*cp++ = '\0';
		cp2 = hfield(str, mp);
		cp[-1] = ':';
		str = cp;
	} else {
		cp = str;
		cp2 = hfield("subject", mp);
	}
	if (cp2 == NULL)
		return(0);
	in.s = cp2;
	in.l = strlen(cp2);
	mime_fromhdr(&in, &out, TD_ICONV);
	i = substr(out.s, cp);
	free(out.s);
	return i;
}

/*
 * Mark the named message by setting its mark bit.
 */
void 
mark(int mesg, int f)
{
	struct message	*mp;
	int i;

	i = mesg;
	if (i < 1 || i > msgCount)
		panic(catgets(catd, CATSET, 129, "Bad message number to mark"));
	if (mb.mb_threaded == 1 && threadflag) {
		if ((message[i-1].m_flag & MHIDDEN) == 0) {
			if (f == MDELETED ||
					(message[i-1].m_flag&MDELETED) == 0)
			message[i-1].m_flag |= MMARK;
		}
		if (message[i-1].m_child) {
			mp = message[i-1].m_child;
			mark(mp-message+1, f);
			for (mp = mp->m_younger; mp; mp = mp->m_younger)
				mark(mp-message+1, f);
		}
	} else
		message[i-1].m_flag |= MMARK;
}

/*
 * Unmark the named message.
 */
static void 
unmark(int mesg)
{
	int i;

	i = mesg;
	if (i < 1 || i > msgCount)
		panic(catgets(catd, CATSET, 130,
					"Bad message number to unmark"));
	message[i-1].m_flag &= ~MMARK;
}

/*
 * Return the message number corresponding to the passed meta character.
 */
static int 
metamess(int meta, int f)
{
	int c, m;
	struct message *mp;

	c = meta;
	switch (c) {
	case '^':
		/*
		 * First 'good' message left.
		 */
		mp = mb.mb_threaded ? threadroot : &message[0];
		while (mp < &message[msgCount]) {
			if (!(mp->m_flag & (MHIDDEN|MKILL)) &&
					(mp->m_flag & MDELETED) == f)
				return(mp - &message[0] + 1);
			if (mb.mb_threaded) {
				mp = next_in_thread(mp);
				if (mp == NULL)
					break;
			} else
				mp++;
		}
		if (!inhook)
			printf(catgets(catd, CATSET, 131,
						"No applicable messages\n"));
		return(-1);

	case '$':
		/*
		 * Last 'good message left.
		 */
		mp = mb.mb_threaded ? this_in_thread(threadroot, -1) :
			&message[msgCount-1];
		while (mp >= &message[0]) {
			if (!(mp->m_flag & (MHIDDEN|MKILL)) &&
					(mp->m_flag & MDELETED) == f)
				return(mp - &message[0] + 1);
			if (mb.mb_threaded) {
				mp = prev_in_thread(mp);
				if (mp == NULL)
					break;
			} else
				mp--;
		}
		if (!inhook)
			printf(catgets(catd, CATSET, 132,
						"No applicable messages\n"));
		return(-1);

	case '.':
		/* 
		 * Current message.
		 */
		m = dot - &message[0] + 1;
		if (dot->m_flag & MHIDDEN || (dot->m_flag & MDELETED) != f) {
			printf(catgets(catd, CATSET, 133,
				"%d: Inappropriate message\n"), m);
			return(-1);
		}
		return(m);

	case ';':
		/*
		 * Previously current message.
		 */
		if (prevdot == NULL) {
			printf(catgets(catd, CATSET, 228,
				"No previously current message\n"));
			return(-1);
		}
		m = prevdot - &message[0] + 1;
		if (prevdot->m_flag&MHIDDEN || (prevdot->m_flag&MDELETED)!=f) {
			printf(catgets(catd, CATSET, 133,
				"%d: Inappropriate message\n"), m);
			return(-1);
		}
		return(m);

	default:
		printf(catgets(catd, CATSET, 134,
				"Unknown metachar (%c)\n"), c);
		return(-1);
	}
}

/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*-
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
static char sccsid[] = "@(#)cmd1.c	2.97 (gritter) 6/16/07";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#ifdef	HAVE_WCWIDTH
#include <wchar.h>
#endif

/*
 * Mail -- a mail program
 *
 * User commands.
 */

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */

static int screen;
static void onpipe(int signo);
static int dispc(struct message *mp, const char *a);
static int scroll1(char *arg, int onlynew);
static void hprf(const char *fmt, int mesg, FILE *f, int threaded,
		const char *attrlist);
static int putindent(FILE *fp, struct message *mp, int maxwidth);
static int type1(int *msgvec, int doign, int page, int pipe, int decode,
		char *cmd, off_t *tstats);
static int pipe1(char *str, int doign);
void brokpipe(int signo);

char *
get_pager(void)
{
	char *cp;

	cp = value("PAGER");
	if (cp == NULL || *cp == '\0')
		cp = value("bsdcompat") ? "more" : "pg";
	return cp;
}

int 
headers(void *v)
{
	int *msgvec = v;
	int g, k, n, mesg, flag = 0, lastg = 1;
	struct message *mp, *mq, *lastmq = NULL;
	int size;
	enum mflag	fl = MNEW|MFLAGGED;

	size = screensize();
	n = msgvec[0];	/* n == {-2, -1, 0}: called from scroll() */
	if (screen < 0)
		screen = 0;
	k = screen * size;
	if (k >= msgCount)
		k = msgCount - size;
	if (k < 0)
		k = 0;
	if (mb.mb_threaded == 0) {
		g = 0;
		mq = &message[0];
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if (visible(mp)) {
				if (g % size == 0)
					mq = mp;
				if (mp->m_flag&fl) {
					lastg = g;
					lastmq = mq;
				}
				if (n>0 && mp==&message[n-1] ||
						n==0 && g==k ||
						n==-2 && g==k+size && lastmq ||
						n<0 && g>=k && mp->m_flag&fl)
					break;
				g++;
			}
		if (lastmq && (n==-2 || n==-1 && mp==&message[msgCount])) {
			g = lastg;
			mq = lastmq;
		}
		screen = g / size;
		mp = mq;
		mesg = mp - &message[0];
		if (dot != &message[n-1]) {
			for (mq = mp; mq < &message[msgCount]; mq++)
				if (visible(mq)) {
					setdot(mq);
					break;
				}
		}
		if (mb.mb_type == MB_IMAP)
			imap_getheaders(mesg+1, mesg + size);
		for (; mp < &message[msgCount]; mp++) {
			mesg++;
			if (!visible(mp))
				continue;
			if (flag++ >= size)
				break;
			printhead(mesg, stdout, 0);
		}
	} else {	/* threaded */
		g = 0;
		mq = threadroot;
		for (mp = threadroot; mp; mp = next_in_thread(mp))
			if (visible(mp) && (mp->m_collapsed <= 0 ||
					 mp == &message[n-1])) {
				if (g % size == 0)
					mq = mp;
				if (mp->m_flag&fl) {
					lastg = g;
					lastmq = mq;
				}
				if (n>0 && mp==&message[n-1] ||
						n==0 && g==k ||
						n==-2 && g==k+size && lastmq ||
						n<0 && g>=k && mp->m_flag&fl)
					break;
				g++;
			}
		if (lastmq && (n==-2 || n==-1 && mp==&message[msgCount])) {
			g = lastg;
			mq = lastmq;
		}
		screen = g / size;
		mp = mq;
		if (dot != &message[n-1]) {
			for (mq = mp; mq; mq = next_in_thread(mq))
				if (visible(mq) && mq->m_collapsed <= 0) {
					setdot(mq);
					break;
				}
		}
		while (mp) {
			if (visible(mp) && (mp->m_collapsed <= 0 ||
					 mp == &message[n-1])) {
				if (flag++ >= size)
					break;
				printhead(mp - &message[0] + 1, stdout,
						mb.mb_threaded);
			}
			mp = next_in_thread(mp);
		}
	}
	if (flag == 0) {
		printf(catgets(catd, CATSET, 6, "No more mail.\n"));
		return(1);
	}
	return(0);
}

/*
 * Scroll to the next/previous screen
 */
int
scroll(void *v)
{
	return scroll1(v, 0);
}

int
Scroll(void *v)
{
	return scroll1(v, 1);
}

static int
scroll1(char *arg, int onlynew)
{
	int size;
	int cur[1];

	cur[0] = onlynew ? -1 : 0;
	size = screensize();
	switch (*arg) {
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9': case '0':
		screen = atoi(arg);
		goto scroll_forward;
	case '\0':
		screen++;
		goto scroll_forward;
	case '$':
		screen = msgCount / size;
		goto scroll_forward;
	case '+':
		if (arg[1] == '\0')
			screen++;
		else
			screen += atoi(arg + 1);
scroll_forward:
		if (screen * size > msgCount) {
			screen = msgCount / size;
			printf(catgets(catd, CATSET, 7,
					"On last screenful of messages\n"));
		}
		break;

	case '-':
		if (arg[1] == '\0')
			screen--;
		else
			screen -= atoi(arg + 1);
		if (screen < 0) {
			screen = 0;
			printf(catgets(catd, CATSET, 8,
					"On first screenful of messages\n"));
		}
		if (cur[0] == -1)
			cur[0] = -2;
		break;

	default:
		printf(catgets(catd, CATSET, 9,
			"Unrecognized scrolling command \"%s\"\n"), arg);
		return(1);
	}
	return(headers(cur));
}

/*
 * Compute screen size.
 */
int 
screensize(void)
{
	int s;
	char *cp;

	if ((cp = value("screen")) != NULL && (s = atoi(cp)) > 0)
		return s;
	return scrnheight - 4;
}

static sigjmp_buf	pipejmp;

/*ARGSUSED*/
static void 
onpipe(int signo)
{
	siglongjmp(pipejmp, 1);
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */
int 
from(void *v)
{
	int *msgvec = v;
	int *ip, n;
	FILE *obuf = stdout;
	char *cp;

	(void)&obuf;
	(void)&cp;
	if (is_a_tty[0] && is_a_tty[1] && (cp = value("crt")) != NULL) {
		for (n = 0, ip = msgvec; *ip; ip++)
			n++;
		if (n > (*cp == '\0' ? screensize() : atoi(cp)) + 3) {
			cp = get_pager();
			if (sigsetjmp(pipejmp, 1))
				goto endpipe;
			if ((obuf = Popen(cp, "w", NULL, 1)) == NULL) {
				perror(cp);
				obuf = stdout;
			} else
				safe_signal(SIGPIPE, onpipe);
		}
	}
	for (ip = msgvec; *ip != 0; ip++)
		printhead(*ip, obuf, mb.mb_threaded);
	if (--ip >= msgvec)
		setdot(&message[*ip - 1]);
endpipe:
	if (obuf != stdout) {
		safe_signal(SIGPIPE, SIG_IGN);
		Pclose(obuf);
		safe_signal(SIGPIPE, dflpipe);
	}
	return(0);
}

static int 
dispc(struct message *mp, const char *a)
{
	int	dispc = ' ';

	/*
	 * Bletch!
	 */
	if ((mp->m_flag & (MREAD|MNEW)) == MREAD)
		dispc = a[3];
	if ((mp->m_flag & (MREAD|MNEW)) == (MREAD|MNEW))
		dispc = a[2];
	if (mp->m_flag & MANSWERED)
		dispc = a[8];
	if (mp->m_flag & MDRAFTED)
		dispc = a[9];
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = a[0];
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = a[1];
	if (mp->m_flag & MJUNK)
		dispc = a[13];
	if (mp->m_flag & MSAVED)
		dispc = a[4];
	if (mp->m_flag & MPRESERVE)
		dispc = a[5];
	if (mp->m_flag & (MBOX|MBOXED))
		dispc = a[6];
	if (mp->m_flag & MFLAGGED)
		dispc = a[7];
	if (mp->m_flag & MKILL)
		dispc = a[10];
	if (mb.mb_threaded == 1 && mp->m_collapsed > 0)
		dispc = a[12];
	if (mb.mb_threaded == 1 && mp->m_collapsed < 0)
		dispc = a[11];
	return dispc;
}

static void
hprf(const char *fmt, int mesg, FILE *f, int threaded, const char *attrlist)
{
	struct message	*mp = &message[mesg-1];
	char	*headline = NULL, *subjline, *name, *cp, *pbuf = NULL;
	struct headline	hl;
	size_t	headsize = 0;
	const char	*fp;
	int	B, c, i, n, s;
	int	headlen = 0;
	struct str	in, out;
	int	subjlen = scrnwidth, fromlen, isto = 0, isaddr = 0;
	FILE	*ibuf;

	if ((mp->m_flag & MNOFROM) == 0) {
		if ((ibuf = setinput(&mb, mp, NEED_HEADER)) == NULL)
			return;
		if ((headlen = readline(ibuf, &headline, &headsize)) < 0)
			return;
	}
	if ((subjline = hfield("subject", mp)) == NULL)
		subjline = hfield("subj", mp);
	if (subjline == NULL) {
		out.s = NULL;
		out.l = 0;
	} else {
		in.s = subjline;
		in.l = strlen(subjline);
		mime_fromhdr(&in, &out, TD_ICONV | TD_ISPR);
		subjline = out.s;
	}
	if ((mp->m_flag & MNOFROM) == 0) {
		pbuf = ac_alloc(headlen + 1);
		parse(headline, headlen, &hl, pbuf);
	} else {
		hl.l_from = /*fakefrom(mp);*/NULL;
		hl.l_tty = NULL;
		hl.l_date = fakedate(mp->m_time);
	}
	if (value("datefield") && (cp = hfield("date", mp)) != NULL)
		hl.l_date = fakedate(rfctime(cp));
	if (Iflag) {
		if ((name = hfield("newsgroups", mp)) == NULL)
			if ((name = hfield("article-id", mp)) == NULL)
				name = "<>";
		name = prstr(name);
	} else if (value("show-rcpt") == NULL) {
		name = name1(mp, 0);
		isaddr = 1;
		if (value("showto") && name && is_myname(skin(name))) {
			if ((cp = hfield("to", mp)) != NULL) {
				name = cp;
				isto = 1;
			}
		}
	} else {
		isaddr = 1;
		if ((name = hfield("to", mp)) != NULL)
			isto = 1;
	}
	if (name == NULL) {
		name = "";
		isaddr = 0;
	}
	if (isaddr) {
		if (value("showname"))
			name = realname(name);
		else {
			name = prstr(skin(name));
		}
	}
	for (fp = fmt; *fp; fp++) {
		if (*fp == '%') {
			if (*++fp == '-') {
				fp++;
			} else if (*fp == '+')
				fp++;
			while (digitchar(*fp&0377))
				fp++;
			if (*fp == '\0')
				break;
		} else {
#if defined (HAVE_MBTOWC) && defined (HAVE_WCWIDTH)
			if (mb_cur_max > 1) {
				wchar_t	wc;
				if ((s = mbtowc(&wc, fp, mb_cur_max)) < 0)
					n = s = 1;
				else {
					if ((n = wcwidth(wc)) < 0)
						n = 1;
				}
			} else
#endif  /* HAVE_MBTOWC && HAVE_WCWIDTH */
			{
				n = s = 1;
			}
			subjlen -= n;
			while (--s > 0)
				fp++;
		}
	}
	for (fp = fmt; *fp; fp++) {
		if (*fp == '%') {
			B = 0;
			n = 0;
			s = 1;
			if (*++fp == '-') {
				s = -1;
				fp++;
			} else if (*fp == '+')
				fp++;
			if (digitchar(*fp&0377)) {
				do
					n = 10*n + *fp - '0';
				while (fp++, digitchar(*fp&0377));
			}
			if (*fp == '\0')
				break;
			n *= s;
			switch (*fp) {
			case '%':
				putc('%', f);
				subjlen--;
				break;
			case '>':
			case '<':
				c = dot == mp ? *fp&0377 : ' ';
				putc(c, f);
				subjlen--;
				break;
			case 'a':
				c = dispc(mp, attrlist);
				putc(c, f);
				subjlen--;
				break;
			case 'm':
				if (n == 0) {
					n = 3;
					if (threaded)
						for (i=msgCount; i>999; i/=10)
							n++;
				}
				subjlen -= fprintf(f, "%*d", n, mesg);
				break;
			case 'f':
				if (n <= 0)
					n = 18;
				fromlen = n;
				if (isto)
					fromlen -= 3;
				fprintf(f, "%s%s", isto ? "To " : "",
						colalign(name, fromlen, 1));
				subjlen -= n;
				break;
			case 'd':
				if (n <= 0)
					n = 16;
				subjlen -= fprintf(f, "%*.*s", n, n, hl.l_date);
				break;
			case 'l':
				if (n == 0)
					n = 4;
				if (mp->m_xlines)
					subjlen -= fprintf(f, "%*ld", n,
							mp->m_xlines);
				else {
					subjlen -= n;
					while (n--)
						putc(' ', f);
				}
				break;
			case 'o':
				if (n == 0)
					n = -5;
				subjlen -= fprintf(f, "%*lu", n,
						(long)mp->m_xsize);
				break;
			case 'i':
				if (threaded)
					subjlen -= putindent(f, mp,
							scrnwidth - 60);
				break;
			case 'S':
				B = 1;
				/*FALLTHRU*/
			case 's':
				n = n>0 ? n : subjlen - 2;
				if (B)
					n -= 2;
				if (subjline != NULL && n >= 0) {
					/* pretty pathetic */
					fprintf(f, B ? "\"%s\"" : "%s",
						colalign(subjline, n, 0));
				}
				break;
			case 'U':
				if (n == 0)
					n = 9;
				subjlen -= fprintf(f, "%*lu", n, mp->m_uid);
				break;
			case 'e':
				if (n == 0)
					n = 2;
				subjlen -= fprintf(f, "%*u", n, threaded == 1 ?
						mp->m_level : 0);
				break;
			case 't':
				if (n == 0) {
					n = 3;
					if (threaded)
						for (i=msgCount; i>999; i/=10)
							n++;
				}
				fprintf(f, "%*ld", n, threaded ?
						mp->m_threadpos : mesg);
				subjlen -= n;
				break;
			case 'c':
				if (n == 0)
					n = 6;
				subjlen -= fprintf(f, "%*g", n, mp->m_score);
				break;
			}
		} else
			putc(*fp&0377, f);
	}
	putc('\n', f);
	if (out.s)
		free(out.s);
	if (headline)
		free(headline);
	if (pbuf)
		ac_free(pbuf);
}

/*
 * Print out the indenting in threaded display.
 */
static int
putindent(FILE *fp, struct message *mp, int maxwidth)
{
	struct message	*mq;
	int	indent, i;
	int	*us;
	char	*cs;
	int	important = MNEW|MFLAGGED;

	if (mp->m_level == 0)
		return 0;
	cs = ac_alloc(mp->m_level);
	us = ac_alloc(mp->m_level * sizeof *us);
	i = mp->m_level - 1;
	if (mp->m_younger && mp->m_younger->m_level == i + 1) {
		if (mp->m_parent && mp->m_parent->m_flag & important)
			us[i] = mp->m_flag & important ? 0x2523 : 0x2520;
		else
			us[i] = mp->m_flag & important ? 0x251D : 0x251C;
		cs[i] = '+';
	} else {
		if (mp->m_parent && mp->m_parent->m_flag & important)
			us[i] = mp->m_flag & important ? 0x2517 : 0x2516;
		else
			us[i] = mp->m_flag & important ? 0x2515 : 0x2514;
		cs[i] = '\\';
	}
	mq = mp->m_parent;
	for (i = mp->m_level - 2; i >= 0; i--) {
		if (mq) {
			if (i > mq->m_level - 1) {
				us[i] = cs[i] = ' ';
				continue;
			}
			if (mq->m_younger) {
				if (mq->m_parent &&
						mq->m_parent->m_flag&important)
					us[i] = 0x2503;
				else
					us[i] = 0x2502;
				cs[i] = '|';
			} else
				us[i] = cs[i] = ' ';
			mq = mq->m_parent;
		} else
			us[i] = cs[i] = ' ';
	}
	for (indent = 0; indent < mp->m_level && indent < maxwidth; indent++) {
		if (indent < maxwidth - 1)
			putuc(us[indent], cs[indent] & 0377, fp);
		else
			putuc(0x21B8, '^', fp);
	}
	ac_free(us);
	ac_free(cs);
	return indent;
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */
void
printhead(int mesg, FILE *f, int threaded)
{
	int bsdflags, bsdheadline, sz;
	char	*fmt, attrlist[30], *cp;

	bsdflags = value("bsdcompat") != NULL || value("bsdflags") != NULL ||
		getenv("SYSV3") != NULL;
	strcpy(attrlist, bsdflags ? "NU  *HMFATK+-J" : "NUROSPMFATK+-J");
	if ((cp = value("attrlist")) != NULL) {
		sz = strlen(cp);
		if (sz > sizeof attrlist - 1)
			sz = sizeof attrlist - 1;
		memcpy(attrlist, cp, sz);
	}
	bsdheadline = value("bsdcompat") != NULL ||
		value("bsdheadline") != NULL;
	if ((fmt = value("headline")) == NULL)
		fmt = bsdheadline ?
			"%>%a%m %20f  %16d %3l/%-5o %i%S" :
			"%>%a%m %18f %16d %4l/%-5o %i%s";
	hprf(fmt, mesg, f, threaded, attrlist);
}

/*
 * Print out the value of dot.
 */
/*ARGSUSED*/
int 
pdot(void *v)
{
	printf(catgets(catd, CATSET, 13, "%d\n"),
			(int)(dot - &message[0] + 1));
	return(0);
}

/*
 * Print out all the possible commands.
 */
/*ARGSUSED*/
int 
pcmdlist(void *v)
{
	extern const struct cmd cmdtab[];
	const struct cmd *cp;
	int cc;

	printf(catgets(catd, CATSET, 14, "Commands are:\n"));
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NULL)
			printf(catgets(catd, CATSET, 15, "%s, "), cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Type out the messages requested.
 */
static sigjmp_buf	pipestop;

static int
type1(int *msgvec, int doign, int page, int pipe, int decode,
		char *cmd, off_t *tstats)
{
	int *ip;
	struct message *mp;
	char *cp;
	int nlines;
	off_t mstats[2];
	/*
	 * Must be static to become excluded from sigsetjmp().
	 */
	static FILE *obuf;
#ifdef __GNUC__
	/* Avoid longjmp clobbering */
	(void) &cp;
	(void) &cmd;
	(void) &obuf;
#endif

	obuf = stdout;
	if (sigsetjmp(pipestop, 1))
		goto close_pipe;
	if (pipe) {
		cp = value("SHELL");
		if (cp == NULL)
			cp = SHELL;
		obuf = Popen(cmd, "w", cp, 1);
		if (obuf == NULL) {
			perror(cmd);
			obuf = stdout;
		} else {
			safe_signal(SIGPIPE, brokpipe);
		}
	} else if (value("interactive") != NULL &&
	    (page || (cp = value("crt")) != NULL)) {
		nlines = 0;
		if (!page) {
			for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
				if ((message[*ip-1].m_have & HAVE_BODY) == 0) {
					if ((get_body(&message[*ip - 1])) !=
							OKAY)
						return 1;
				}
				nlines += message[*ip - 1].m_lines;
			}
		}
		if (page || nlines > (*cp ? atoi(cp) : realscreenheight)) {
			cp = get_pager();
			obuf = Popen(cp, "w", NULL, 1);
			if (obuf == NULL) {
				perror(cp);
				obuf = stdout;
			} else
				safe_signal(SIGPIPE, brokpipe);
		}
	}
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		setdot(mp);
		uncollapse1(mp, 1);
		if (value("quiet") == NULL)
			fprintf(obuf, catgets(catd, CATSET, 17,
				"Message %2d:\n"), *ip);
		send(mp, obuf, doign ? ignore : 0, NULL,
			pipe && value("piperaw") ? SEND_MBOX :
				decode ? SEND_SHOW :
				doign ? SEND_TODISP : SEND_TODISP_ALL,
			mstats);
		if (pipe && value("page")) {
			putc('\f', obuf);
		}
		if (tstats) {
			tstats[0] += mstats[0];
			tstats[1] += mstats[1];
		}
	}
close_pipe:
	if (obuf != stdout) {
		/*
		 * Ignore SIGPIPE so it can't cause a duplicate close.
		 */
		safe_signal(SIGPIPE, SIG_IGN);
		Pclose(obuf);
		safe_signal(SIGPIPE, dflpipe);
	}
	return(0);
}

/*
 * Get the last, possibly quoted part of linebuf.
 */
char *
laststring(char *linebuf, int *flag, int strip)
{
	char *cp, *p;
	char quoted;

	*flag = 1;
	cp = strlen(linebuf) + linebuf - 1;

	/*
	 * Strip away trailing blanks.
	 */
	while (cp > linebuf && whitechar(*cp & 0377))
		cp--;
	*++cp = 0;
	if (cp == linebuf) {
		*flag = 0;
		return NULL;
	}

	/*
	 * Now search for the beginning of the command name.
	 */
	quoted = *(cp - 1);
	if (quoted == '\'' || quoted == '\"') {
		cp--;
		if (strip)
			*cp = '\0';
		cp--;
		while (cp > linebuf) {
			if (*cp != quoted) {
				cp--;
			} else if (*(cp - 1) != '\\') {
				break;
			} else {
				p = --cp;
				do {
					*p = *(p + 1);
				} while (*p++);
				cp--;
			}
		}
		if (cp == linebuf)
			*flag = 0;
		if (*cp == quoted) {
			if (strip)
				*cp++ = 0;
		} else
			*flag = 0;
	} else {
		while (cp > linebuf && !whitechar(*cp & 0377))
			cp--;
		if (whitechar(*cp & 0377))
			*cp++ = 0;
		else
			*flag = 0;
	}
	if (*cp == '\0') {
		return(NULL);
	}
	return(cp);
}

/*
 * Pipe the messages requested.
 */
static int 
pipe1(char *str, int doign)
{
	char *cmd;
	int f, *msgvec, ret;
	off_t stats[2];

	/*LINTED*/
	msgvec = (int *)salloc((msgCount + 2) * sizeof *msgvec);
	if ((cmd = laststring(str, &f, 1)) == NULL) {
		cmd = value("cmd");
		if (cmd == NULL || *cmd == '\0') {
			fputs(catgets(catd, CATSET, 16,
				"variable cmd not set\n"), stderr);
			return 1;
		}
	}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			if (inhook)
				return 0;
			puts(catgets(catd, CATSET, 18, "No messages to pipe."));
			return 1;
		}
		msgvec[1] = 0;
	} else if (getmsglist(str, msgvec, 0) < 0)
		return 1;
	if (*msgvec == 0) {
		if (inhook)
			return 0;
		printf("No applicable messages.\n");
		return 1;
	}
	printf(catgets(catd, CATSET, 268, "Pipe to: \"%s\"\n"), cmd);
	stats[0] = stats[1] = 0;
	if ((ret = type1(msgvec, doign, 0, 1, 0, cmd, stats)) == 0) {
		printf("\"%s\" ", cmd);
		if (stats[0] >= 0)
			printf("%lu", (long)stats[0]);
		else
			printf(catgets(catd, CATSET, 27, "binary"));
		printf("/%lu\n", (long)stats[1]);
	}
	return ret;
}

/*
 * Paginate messages, honor ignored fields.
 */
int 
more(void *v)
{
	int *msgvec = v;
	return (type1(msgvec, 1, 1, 0, 0, NULL, NULL));
}

/*
 * Paginate messages, even printing ignored fields.
 */
int 
More(void *v)
{
	int *msgvec = v;

	return (type1(msgvec, 0, 1, 0, 0, NULL, NULL));
}

/*
 * Type out messages, honor ignored fields.
 */
int 
type(void *v)
{
	int *msgvec = v;

	return(type1(msgvec, 1, 0, 0, 0, NULL, NULL));
}

/*
 * Type out messages, even printing ignored fields.
 */
int 
Type(void *v)
{
	int *msgvec = v;

	return(type1(msgvec, 0, 0, 0, 0, NULL, NULL));
}

/*
 * Show MIME-encoded message text, including all fields.
 */
int
show(void *v)
{
	int *msgvec = v;

	return(type1(msgvec, 0, 0, 0, 1, NULL, NULL));
}

/*
 * Pipe messages, honor ignored fields.
 */
int 
pipecmd(void *v)
{
	char *str = v;
	return(pipe1(str, 1));
}
/*
 * Pipe messages, not respecting ignored fields.
 */
int 
Pipecmd(void *v)
{
	char *str = v;
	return(pipe1(str, 0));
}

/*
 * Respond to a broken pipe signal --
 * probably caused by quitting more.
 */
/*ARGSUSED*/
void
brokpipe(int signo)
{
	siglongjmp(pipestop, 1);
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */
int 
top(void *v)
{
	int *msgvec = v;
	int *ip;
	struct message *mp;
	int c, topl, lines, lineb;
	char *valtop, *linebuf = NULL;
	size_t linesize;
	FILE *ibuf;

	topl = 5;
	valtop = value("toplines");
	if (valtop != NULL) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		setdot(mp);
		did_print_dot = 1;
		if (value("quiet") == NULL)
			printf(catgets(catd, CATSET, 19,
					"Message %2d:\n"), *ip);
		if (mp->m_flag & MNOFROM)
			printf("From %s %s\n", fakefrom(mp),
					fakedate(mp->m_time));
		if ((ibuf = setinput(&mb, mp, NEED_BODY)) == NULL)	/* XXX could use TOP */
			return 1;
		c = mp->m_lines;
		if (!lineb)
			printf("\n");
		for (lines = 0; lines < c && lines <= topl; lines++) {
			if (readline(ibuf, &linebuf, &linesize) < 0)
				break;
			puts(linebuf);
			lineb = blankline(linebuf);
		}
	}
	if (linebuf)
		free(linebuf);
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */
int 
stouch(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		setdot(&message[*ip-1]);
		dot->m_flag |= MTOUCH;
		dot->m_flag &= ~MPRESERVE;
		/*
		 * POSIX interpretation necessary.
		 */
		did_print_dot = 1;
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */
int 
mboxit(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		setdot(&message[*ip-1]);
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
		/*
		 * POSIX interpretation necessary.
		 */
		did_print_dot = 1;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
int 
folders(void *v)
{
	char	**argv = v;
	char dirname[PATHSIZE];
	char *cmd, *name;

	if (*argv)
		name = expand(*argv);
	else if (getfold(dirname, sizeof dirname) < 0) {
		printf(catgets(catd, CATSET, 20,
				"No value set for \"folder\"\n"));
		return 1;
	} else
		name = dirname;
	if (which_protocol(name) == PROTO_IMAP)
		imap_folders(name, *argv == NULL);
	else {
		if ((cmd = value("LISTER")) == NULL)
			cmd = "ls";
		run_command(cmd, 0, -1, -1, name, NULL, NULL);
	}
	return 0;
}

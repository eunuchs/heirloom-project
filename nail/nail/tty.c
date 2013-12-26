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
static char sccsid[] = "@(#)tty.c	2.29 (gritter) 3/9/07";
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Generally useful tty stuff.
 */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static	cc_t		c_erase;	/* Current erase char */
static	cc_t		c_kill;		/* Current kill char */
static	sigjmp_buf	rewrite;	/* Place to go when continued */
static	sigjmp_buf	intjmp;		/* Place to go when interrupted */
#ifndef TIOCSTI
static	int		ttyset;		/* We must now do erase/kill */
#endif
static	struct termios	ttybuf;
static	long		vdis;		/* _POSIX_VDISABLE char */

static void ttystop(int s);
static void ttyint(int s);
static int safe_getc(FILE *ibuf);
static char *rtty_internal(const char *pr, char *src);

/*
 * Receipt continuation.
 */
static void 
ttystop(int s)
{
	sighandler_type old_action = safe_signal(s, SIG_DFL);
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, s);
	sigprocmask(SIG_BLOCK, &nset, NULL);
	kill(0, s);
	sigprocmask(SIG_UNBLOCK, &nset, NULL);
	safe_signal(s, old_action);
	siglongjmp(rewrite, 1);
}

/*ARGSUSED*/
static void 
ttyint(int s)
{
	siglongjmp(intjmp, 1);
}

/*
 * Interrupts will cause trouble if we are inside a stdio call. As
 * this is only relevant if input comes from a terminal, we can simply
 * bypass it by read() then.
 */
static int
safe_getc(FILE *ibuf)
{
	if (fileno(ibuf) == 0 && is_a_tty[0]) {
		char c;
		int sz;

again:
		if ((sz = read(0, &c, 1)) != 1) {
			if (sz < 0 && errno == EINTR)
				goto again;
			return EOF;
		}
		return c & 0377;
	} else
		return getc(ibuf);
}

/*
 * Read up a header from standard input.
 * The source string has the preliminary contents to
 * be read.
 */
static char *
rtty_internal(const char *pr, char *src)
{
	char ch, canonb[LINESIZE];
	int c;
	char *cp, *cp2;

	(void) &c;
	(void) &cp2;
	fputs(pr, stdout);
	fflush(stdout);
	if (src != NULL && strlen(src) > sizeof canonb - 2) {
		printf(catgets(catd, CATSET, 200, "too long to edit\n"));
		return(src);
	}
#ifndef TIOCSTI
	if (src != NULL)
		cp = sstpcpy(canonb, src);
	else
		cp = sstpcpy(canonb, "");
	fputs(canonb, stdout);
	fflush(stdout);
#else
	cp = src == NULL ? "" : src;
	while ((c = *cp++) != '\0') {
		if ((c_erase != vdis && c == c_erase) ||
		    (c_kill != vdis && c == c_kill)) {
			ch = '\\';
			ioctl(0, TIOCSTI, &ch);
		}
		ch = c;
		ioctl(0, TIOCSTI, &ch);
	}
	cp = canonb;
	*cp = 0;
#endif
	cp2 = cp;
	while (cp2 < canonb + sizeof canonb)
		*cp2++ = 0;
	cp2 = cp;
	if (sigsetjmp(rewrite, 1))
		goto redo;
	safe_signal(SIGTSTP, ttystop);
	safe_signal(SIGTTOU, ttystop);
	safe_signal(SIGTTIN, ttystop);
	clearerr(stdin);
	while (cp2 < canonb + sizeof canonb - 1) {
		c = safe_getc(stdin);
		if (c == EOF || c == '\n')
			break;
		*cp2++ = c;
	}
	*cp2 = 0;
	safe_signal(SIGTSTP, SIG_DFL);
	safe_signal(SIGTTOU, SIG_DFL);
	safe_signal(SIGTTIN, SIG_DFL);
	if (c == EOF && ferror(stdin)) {
redo:
		cp = strlen(canonb) > 0 ? canonb : NULL;
		clearerr(stdin);
		return(rtty_internal(pr, cp));
	}
#ifndef TIOCSTI
	if (cp == NULL || *cp == '\0')
		return(src);
	cp2 = cp;
	if (!ttyset)
		return(strlen(canonb) > 0 ? savestr(canonb) : NULL);
	while (*cp != '\0') {
		c = *cp++;
		if (c_erase != vdis && c == c_erase) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2--;
			continue;
		}
		if (c_kill != vdis && c == c_kill) {
			if (cp2 == canonb)
				continue;
			if (cp2[-1] == '\\') {
				cp2[-1] = c;
				continue;
			}
			cp2 = canonb;
			continue;
		}
		*cp2++ = c;
	}
	*cp2 = '\0';
#endif
	if (equal("", canonb))
		return(NULL);
	return(savestr(canonb));
}

/*
 * Read all relevant header fields.
 */

#ifndef	TIOCSTI
#define	TTYSET_CHECK(h)	if (!ttyset && (h) != NULL) \
					ttyset++, tcsetattr(0, TCSADRAIN, \
					&ttybuf);
#else
#define	TTYSET_CHECK(h)
#endif

#define	GRAB_SUBJECT	if (gflags & GSUBJECT) { \
				TTYSET_CHECK(hp->h_subject) \
				hp->h_subject = rtty_internal("Subject: ", \
						hp->h_subject); \
			}

static struct name *
grabaddrs(const char *field, struct name *np, int comma, enum gfield gflags)
{
	struct name	*nq;

	TTYSET_CHECK(np);
	loop:
		np = sextract(rtty_internal(field, detract(np, comma)), gflags);
		for (nq = np; nq != NULL; nq = nq->n_flink)
			if (mime_name_invalid(nq->n_name, 1))
				goto loop;
	return np;
}

int 
grabh(struct header *hp, enum gfield gflags, int subjfirst)
{
	sighandler_type saveint;
#ifndef TIOCSTI
	sighandler_type savequit;
#endif
	sighandler_type savetstp;
	sighandler_type savettou;
	sighandler_type savettin;
	int errs;
	int comma;

	(void) &comma;
	(void) &saveint;
	savetstp = safe_signal(SIGTSTP, SIG_DFL);
	savettou = safe_signal(SIGTTOU, SIG_DFL);
	savettin = safe_signal(SIGTTIN, SIG_DFL);
	errs = 0;
	comma = value("bsdcompat") || value("bsdmsgs") ? 0 : GCOMMA;
#ifndef TIOCSTI
	ttyset = 0;
#endif
	if (tcgetattr(fileno(stdin), &ttybuf) < 0) {
		perror("tcgetattr");
		return(-1);
	}
	c_erase = ttybuf.c_cc[VERASE];
	c_kill = ttybuf.c_cc[VKILL];
#if defined (_PC_VDISABLE) && defined (HAVE_FPATHCONF)
	if ((vdis = fpathconf(0, _PC_VDISABLE)) < 0)
		vdis = '\377';
#elif defined (_POSIX_VDISABLE)
	vdis = _POSIX_VDISABLE;
#else
	vdis = '\377';
#endif
#ifndef TIOCSTI
	ttybuf.c_cc[VERASE] = 0;
	ttybuf.c_cc[VKILL] = 0;
	if ((saveint = safe_signal(SIGINT, SIG_IGN)) == SIG_DFL)
		safe_signal(SIGINT, SIG_DFL);
	if ((savequit = safe_signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
		safe_signal(SIGQUIT, SIG_DFL);
#else	/* TIOCSTI */
	saveint = safe_signal(SIGINT, SIG_IGN);
	if (sigsetjmp(intjmp, 1)) {
		/* avoid garbled output with C-c */
		printf("\n");
		fflush(stdout);
		goto out;
	}
	if (saveint != SIG_IGN)
		safe_signal(SIGINT, ttyint);
#endif	/* TIOCSTI */
	if (gflags & GTO)
		hp->h_to = grabaddrs("To: ", hp->h_to, comma, GTO|GFULL);
	if (subjfirst)
		GRAB_SUBJECT
	if (gflags & GCC)
		hp->h_cc = grabaddrs("Cc: ", hp->h_cc, comma, GCC|GFULL);
	if (gflags & GBCC)
		hp->h_bcc = grabaddrs("Bcc: ", hp->h_bcc, comma, GBCC|GFULL);
	if (gflags & GEXTRA) {
		if (hp->h_from == NULL)
			hp->h_from = sextract(myaddrs(hp), GEXTRA|GFULL);
		hp->h_from = grabaddrs("From: ", hp->h_from, comma,
				GEXTRA|GFULL);
		if (hp->h_replyto == NULL)
			hp->h_replyto = sextract(value("replyto"),
					GEXTRA|GFULL);
		hp->h_replyto = grabaddrs("Reply-To: ", hp->h_replyto, comma,
				GEXTRA|GFULL);
		if (hp->h_sender == NULL)
			hp->h_sender = sextract(value("sender"),
					GEXTRA|GFULL);
		hp->h_sender = grabaddrs("Sender: ", hp->h_sender, comma,
				GEXTRA|GFULL);
		if (hp->h_organization == NULL)
			hp->h_organization = value("ORGANIZATION");
		TTYSET_CHECK(hp->h_organization);
		hp->h_organization = rtty_internal("Organization: ",
				hp->h_organization);
	}
	if (!subjfirst)
		GRAB_SUBJECT
out:
	safe_signal(SIGTSTP, savetstp);
	safe_signal(SIGTTOU, savettou);
	safe_signal(SIGTTIN, savettin);
#ifndef TIOCSTI
	ttybuf.c_cc[VERASE] = c_erase;
	ttybuf.c_cc[VKILL] = c_kill;
	if (ttyset)
		tcsetattr(fileno(stdin), TCSADRAIN, &ttybuf);
	safe_signal(SIGQUIT, savequit);
#endif
	safe_signal(SIGINT, saveint);
	return(errs);
}

/*
 * Read a line from tty; to be called from elsewhere
 */

char *
readtty(char *prefix, char *string)
{
	char *ret = NULL;
	struct termios ttybuf;
	sighandler_type saveint = SIG_DFL;
#ifndef TIOCSTI
	sighandler_type savequit;
#endif
	sighandler_type savetstp;
	sighandler_type savettou;
	sighandler_type savettin;

	(void) &saveint;
	(void) &ret;
	savetstp = safe_signal(SIGTSTP, SIG_DFL);
	savettou = safe_signal(SIGTTOU, SIG_DFL);
	savettin = safe_signal(SIGTTIN, SIG_DFL);
#ifndef TIOCSTI
	ttyset = 0;
#endif
	if (tcgetattr(fileno(stdin), &ttybuf) < 0) {
		perror("tcgetattr");
		return NULL;
	}
	c_erase = ttybuf.c_cc[VERASE];
	c_kill = ttybuf.c_cc[VKILL];
#ifndef TIOCSTI
	ttybuf.c_cc[VERASE] = 0;
	ttybuf.c_cc[VKILL] = 0;
	if ((saveint = safe_signal(SIGINT, SIG_IGN)) == SIG_DFL)
		safe_signal(SIGINT, SIG_DFL);
	if ((savequit = safe_signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
		safe_signal(SIGQUIT, SIG_DFL);
#else
	if (sigsetjmp(intjmp, 1)) {
		/* avoid garbled output with C-c */
		printf("\n");
		fflush(stdout);
		goto out2;
	}
	saveint = safe_signal(SIGINT, ttyint);
#endif
	TTYSET_CHECK(string)
	ret = rtty_internal(prefix, string);
	if (ret != NULL && *ret == '\0')
		ret = NULL;
out2:
	safe_signal(SIGTSTP, savetstp);
	safe_signal(SIGTTOU, savettou);
	safe_signal(SIGTTIN, savettin);
#ifndef TIOCSTI
	ttybuf.c_cc[VERASE] = c_erase;
	ttybuf.c_cc[VKILL] = c_kill;
	if (ttyset)
		tcsetattr(fileno(stdin), TCSADRAIN, &ttybuf);
	safe_signal(SIGQUIT, savequit);
#endif
	safe_signal(SIGINT, saveint);
	return ret;
}

int 
yorn(char *msg)
{
	char	*cp;

	if (value("interactive") == NULL)
		return 1;
	do
		cp = readtty(msg, NULL);
	while (cp == NULL ||
		*cp != 'y' && *cp != 'Y' && *cp != 'n' && *cp != 'N');
	return *cp == 'y' || *cp == 'Y';
}

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
static char sccsid[] = "@(#)lex.c	2.86 (gritter) 12/25/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Mail -- a mail program
 *
 * Lexical processing of commands.
 */

static char	*prompt;
static sighandler_type	oldpipe;

static const struct cmd *lex(char *Word);
static void stop(int s);
static void hangup(int s);

/*
 * Set up editing on the given file name.
 * If the first character of name is %, we are considered to be
 * editing the file, otherwise we are reading our mail which has
 * signficance for mbox and so forth.
 *
 * newmail: Check for new mail in the current folder only.
 */
int
setfile(char *name, int newmail)
{
	FILE *ibuf;
	int i, compressed = 0;
	struct stat stb;
	char isedit;
	char *who = name[1] ? name + 1 : myname;
	static int shudclob;
	size_t offset;
	int omsgCount = 0;
	struct shortcut *sh;
	struct flock	flp;

	isedit = *name != '%' && ((sh = get_shortcut(name)) == NULL ||
			*sh->sh_long != '%');
	if ((name = expand(name)) == NULL)
		return -1;

	switch (which_protocol(name)) {
	case PROTO_FILE:
		break;
	case PROTO_MAILDIR:
		return maildir_setfile(name, newmail, isedit);
	case PROTO_POP3:
		shudclob = 1;
		return pop3_setfile(name, newmail, isedit);
	case PROTO_IMAP:
		shudclob = 1;
		if (newmail) {
			if (mb.mb_type == MB_CACHE)
				return 1;
			omsgCount = msgCount;
		}
		return imap_setfile(name, newmail, isedit);
	case PROTO_UNKNOWN:
		fprintf(stderr, catgets(catd, CATSET, 217,
				"Cannot handle protocol: %s\n"), name);
		return -1;
	}
	if ((ibuf = Zopen(name, "r", &compressed)) == NULL) {
		if ((!isedit && errno == ENOENT) || newmail) {
			if (newmail)
				goto nonewmail;
			goto nomail;
		}
		perror(name);
		return(-1);
	}

	if (fstat(fileno(ibuf), &stb) < 0) {
		Fclose(ibuf);
		if (newmail)
			goto nonewmail;
		perror("fstat");
		return (-1);
	}

	if (S_ISDIR(stb.st_mode)) {
		Fclose(ibuf);
		if (newmail)
			goto nonewmail;
		errno = EISDIR;
		perror(name);
		return (-1);
	} else if (S_ISREG(stb.st_mode)) {
		/*EMPTY*/
	} else {
		Fclose(ibuf);
		if (newmail)
			goto nonewmail;
		errno = EINVAL;
		perror(name);
		return (-1);
	}

	/*
	 * Looks like all will be well.  We must now relinquish our
	 * hold on the current set of stuff.  Must hold signals
	 * while we are reading the new file, else we will ruin
	 * the message[] data structure.
	 */

	holdsigs();
	if (shudclob && !newmail)
		quit();

#ifdef	HAVE_SOCKETS
	if (!newmail && mb.mb_sock.s_fd >= 0)
		sclose(&mb.mb_sock);
#endif	/* HAVE_SOCKETS */

	/*
	 * Copy the messages into /tmp
	 * and set pointers.
	 */

	flp.l_type = F_RDLCK;
	flp.l_start = 0;
	flp.l_whence = SEEK_SET;
	if (!newmail) {
		mb.mb_type = MB_FILE;
		mb.mb_perm = Rflag ? 0 : MB_DELE|MB_EDIT;
		mb.mb_compressed = compressed;
		if (compressed) {
			if (compressed & 0200)
				mb.mb_perm = 0;
		} else {
			if ((i = open(name, O_WRONLY)) < 0)
				mb.mb_perm = 0;
			else
				close(i);
		}
		if (shudclob) {
			if (mb.mb_itf) {
				fclose(mb.mb_itf);
				mb.mb_itf = NULL;
			}
			if (mb.mb_otf) {
				fclose(mb.mb_otf);
				mb.mb_otf = NULL;
			}
		}
		shudclob = 1;
		edit = isedit;
		initbox(name);
		offset = 0;
		flp.l_len = 0;
		if (!edit && fcntl(fileno(ibuf), F_SETLKW, &flp) < 0) {
			perror("Unable to lock mailbox");
			Fclose(ibuf);
			return -1;
		}
	} else /* newmail */{
		fseek(mb.mb_otf, 0L, SEEK_END);
		fseek(ibuf, mailsize, SEEK_SET);
		offset = mailsize;
		omsgCount = msgCount;
		flp.l_len = offset;
		if (!edit && fcntl(fileno(ibuf), F_SETLKW, &flp) < 0)
			goto nonewmail;
	}
	mailsize = fsize(ibuf);
	if (newmail && mailsize <= offset) {
		relsesigs();
		goto nonewmail;
	}
	setptr(ibuf, offset);
	setmsize(msgCount);
	if (newmail && mb.mb_sorted) {
		mb.mb_threaded = 0;
		sort((void *)-1);
	}
	Fclose(ibuf);
	relsesigs();
	if (!newmail)
		sawcom = 0;
	if ((!edit || newmail) && msgCount == 0) {
nonewmail:
		if (!newmail) {
			if (value("emptystart") == NULL)
nomail:				fprintf(stderr, catgets(catd, CATSET, 88,
						"No mail for %s\n"), who);
		}
		return 1;
	}
	if (newmail) {
		newmailinfo(omsgCount);
	}
	return(0);
}


int 
newmailinfo(int omsgCount)
{
	int	mdot;
	int	i;

	for (i = 0; i < omsgCount; i++)
		message[i].m_flag &= ~MNEWEST;
	if (msgCount > omsgCount) {
		for (i = omsgCount; i < msgCount; i++)
			message[i].m_flag |= MNEWEST;
		printf(catgets(catd, CATSET, 158, "New mail has arrived.\n"));
		if (msgCount - omsgCount == 1)
			printf(catgets(catd, CATSET, 214,
				"Loaded 1 new message\n"));
		else
			printf(catgets(catd, CATSET, 215,
				"Loaded %d new messages\n"),
				msgCount - omsgCount);
	} else
		printf("Loaded %d messages\n", msgCount);
	callhook(mailname, 1);
	mdot = getmdot(1);
	if (value("header")) {
		if (mb.mb_type == MB_IMAP)
			imap_getheaders(omsgCount+1, msgCount);
		while (++omsgCount <= msgCount)
			if (visible(&message[omsgCount-1]))
				printhead(omsgCount, stdout, 0);
	}
	return mdot;
}

static int	*msgvec;
static int	reset_on_stop;			/* do a reset() if stopped */

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */
void 
commands(void)
{
	int eofloop = 0;
	int n, x;
	char *linebuf = NULL, *av, *nv;
	size_t linesize = 0;

	(void)&eofloop;
	if (!sourcing) {
		if (safe_signal(SIGINT, SIG_IGN) != SIG_IGN)
			safe_signal(SIGINT, onintr);
		if (safe_signal(SIGHUP, SIG_IGN) != SIG_IGN)
			safe_signal(SIGHUP, hangup);
		safe_signal(SIGTSTP, stop);
		safe_signal(SIGTTOU, stop);
		safe_signal(SIGTTIN, stop);
	}
	oldpipe = safe_signal(SIGPIPE, SIG_IGN);
	safe_signal(SIGPIPE, oldpipe);
	setexit();
	for (;;) {
		interrupts = 0;
		handlerstacktop = NULL;
		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */
		if (!sourcing && value("interactive") != NULL) {
			av = (av = value("autoinc")) ? savestr(av) : NULL;
			nv = (nv = value("newmail")) ? savestr(nv) : NULL;
			if (is_a_tty[0] && (av != NULL || nv != NULL ||
					mb.mb_type == MB_IMAP)) {
				struct stat st;

				n = (av && strcmp(av, "noimap") &&
						strcmp(av, "nopoll")) |
					(nv && strcmp(nv, "noimap") &&
					 	strcmp(nv, "nopoll"));
				x = !(av || nv);
				if ((mb.mb_type == MB_FILE &&
						stat(mailname, &st) == 0 &&
						st.st_size > mailsize) ||
						(mb.mb_type == MB_IMAP &&
						imap_newmail(n) > x) ||
						(mb.mb_type == MB_MAILDIR &&
						n != 0)) {
					int odot = dot - &message[0];
					int odid = did_print_dot;

					setfile(mailname, 1);
					if (mb.mb_type != MB_IMAP) {
						dot = &message[odot];
						did_print_dot = odid;
					}
				}
			}
			reset_on_stop = 1;
			if ((prompt = value("prompt")) == NULL)
				prompt = value("bsdcompat") ? "& " : "? ";
			printf("%s", prompt);
		}
		fflush(stdout);
		sreset();
		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */
		n = 0;
		for (;;) {
			n = readline_restart(input, &linebuf, &linesize, n);
			if (n < 0)
				break;
			if (n == 0 || linebuf[n - 1] != '\\')
				break;
			linebuf[n - 1] = ' ';
		}
		reset_on_stop = 0;
		if (n < 0) {
				/* eof */
			if (loading)
				break;
			if (sourcing) {
				unstack();
				continue;
			}
			if (value("interactive") != NULL &&
			    value("ignoreeof") != NULL &&
			    ++eofloop < 25) {
				printf(catgets(catd, CATSET, 89,
						"Use \"quit\" to quit.\n"));
				continue;
			}
			break;
		}
		eofloop = 0;
		inhook = 0;
		if (execute(linebuf, 0, n))
			break;
	}
	if (linebuf)
		free(linebuf);
}

/*
 * Execute a single command.
 * Command functions return 0 for success, 1 for error, and -1
 * for abort.  A 1 or -1 aborts a load or source.  A -1 aborts
 * the interactive command loop.
 * Contxt is non-zero if called while composing mail.
 */
int
execute(char *linebuf, int contxt, size_t linesize)
{
	char *word;
	char *arglist[MAXARGC];
	const struct cmd *com = (struct cmd *)NULL;
	char *cp, *cp2;
	int c;
	int muvec[2];
	int e = 1;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */
	word = ac_alloc(linesize + 1);
	for (cp = linebuf; whitechar(*cp & 0377); cp++);
	if (*cp == '!') {
		if (sourcing) {
			printf(catgets(catd, CATSET, 90,
					"Can't \"!\" while sourcing\n"));
			goto out;
		}
		shell(cp+1);
		ac_free(word);
		return(0);
	}
	if (*cp == '#') {
		ac_free(word);
		return 0;
	}
	cp2 = word;
	if (*cp != '|') {
		while (*cp && strchr(" \t0123456789$^.:/-+*'\",;(`", *cp)
				== NULL)
			*cp2++ = *cp++;
	} else
		*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, bitch.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && *word == '\0') {
		ac_free(word);
		return(0);
	}
	com = lex(word);
	if (com == NULL) {
		printf(catgets(catd, CATSET, 91,
				"Unknown command: \"%s\"\n"), word);
		goto out;
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0) {
		if ((cond == CRCV && !rcvmode) ||
				(cond == CSEND && rcvmode) ||
				(cond == CTERM && !is_a_tty[0]) ||
				(cond == CNONTERM && is_a_tty[0])) {
			ac_free(word);
			return(0);
		}
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		printf(catgets(catd, CATSET, 92,
			"May not execute \"%s\" while sending\n"), com->c_name);
		goto out;
	}
	if (sourcing && com->c_argtype & I) {
		printf(catgets(catd, CATSET, 93,
			"May not execute \"%s\" while sourcing\n"),
				com->c_name);
		goto out;
	}
	if ((mb.mb_perm & MB_DELE) == 0 && com->c_argtype & W) {
		printf(catgets(catd, CATSET, 94,
		"May not execute \"%s\" -- message file is read only\n"),
		   com->c_name);
		goto out;
	}
	if (contxt && com->c_argtype & R) {
		printf(catgets(catd, CATSET, 95,
			"Cannot recursively invoke \"%s\"\n"), com->c_name);
		goto out;
	}
	if (mb.mb_type == MB_VOID && com->c_argtype & A) {
		printf(catgets(catd, CATSET, 257,
			"Cannot execute \"%s\" without active mailbox\n"),
				com->c_name);
		goto out;
	}
	switch (com->c_argtype & ~(F|P|I|M|T|W|R|A)) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			printf(catgets(catd, CATSET, 96,
				"Illegal use of \"message list\"\n"));
			break;
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c == 0) {
			if ((*msgvec = first(com->c_msgflag, com->c_msgmask))
					!= 0)
				msgvec[1] = 0;
		}
		if (*msgvec == 0) {
			if (!inhook)
				printf(catgets(catd, CATSET, 97,
					"No applicable messages\n"));
			break;
		}
		e = (*com->c_func)(msgvec);
		break;

	case NDMLIST:
		/*
		 * A message list with no defaults, but no error
		 * if none exist.
		 */
		if (msgvec == 0) {
			printf(catgets(catd, CATSET, 98,
				"Illegal use of \"message list\"\n"));
			break;
		}
		if (getmsglist(cp, msgvec, com->c_msgflag) < 0)
			break;
		e = (*com->c_func)(msgvec);
		break;

	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		while (whitechar(*cp & 0377))
			cp++;
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
	case ECHOLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, linesize, arglist,
				sizeof arglist / sizeof *arglist,
				(com->c_argtype&~(F|P|I|M|T|W|R|A))==ECHOLIST))
					< 0)
			break;
		if (c < com->c_minargs) {
			printf(catgets(catd, CATSET, 99,
				"%s requires at least %d arg(s)\n"),
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			printf(catgets(catd, CATSET, 100,
				"%s takes no more than %d arg(s)\n"),
				com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		panic(catgets(catd, CATSET, 101, "Unknown argtype"));
	}

out:
	ac_free(word);
	/*
	 * Exit the current source file on
	 * error.
	 */
	if (e) {
		if (e < 0)
			return 1;
		if (loading)
			return 1;
		if (sourcing)
			unstack();
		return 0;
	}
	if (com == (struct cmd *)NULL)
		return(0);
	if (value("autoprint") != NULL && com->c_argtype & P)
		if (visible(dot)) {
			muvec[0] = dot - &message[0] + 1;
			muvec[1] = 0;
			type(muvec);
		}
	if (!sourcing && !inhook && (com->c_argtype & T) == 0)
		sawcom = 1;
	return(0);
}

/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */
void 
setmsize(int sz)
{

	if (msgvec != 0)
		free(msgvec);
	msgvec = (int *)scalloc((sz + 1), sizeof *msgvec);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

static const struct cmd *
lex(char *Word)
{
	extern const struct cmd cmdtab[];
	const struct cmd *cp;

	for (cp = &cmdtab[0]; cp->c_name != NULL; cp++)
		if (is_prefix(Word, cp->c_name))
			return(cp);
	return(NULL);
}

/*
 * The following gets called on receipt of an interrupt.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * Also, unstack all source files.
 */

static int	inithdr;		/* am printing startup headers */

/*ARGSUSED*/
void 
onintr(int s)
{
	if (handlerstacktop != NULL) {
		handlerstacktop(s);
		return;
	}
	safe_signal(SIGINT, onintr);
	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		unstack();

	close_all_files();

	if (image >= 0) {
		close(image);
		image = -1;
	}
	if (interrupts != 1)
		fprintf(stderr, catgets(catd, CATSET, 102, "Interrupt\n"));
	safe_signal(SIGPIPE, oldpipe);
	reset(0);
}

/*
 * When we wake up after ^Z, reprint the prompt.
 */
static void 
stop(int s)
{
	sighandler_type old_action = safe_signal(s, SIG_DFL);
	sigset_t nset;

	sigemptyset(&nset);
	sigaddset(&nset, s);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *)NULL);
	kill(0, s);
	sigprocmask(SIG_BLOCK, &nset, (sigset_t *)NULL);
	safe_signal(s, old_action);
	if (reset_on_stop) {
		reset_on_stop = 0;
		reset(0);
	}
}

/*
 * Branch here on hangup signal and simulate "exit".
 */
/*ARGSUSED*/
static void 
hangup(int s)
{

	/* nothing to do? */
	exit(1);
}

/*
 * Announce the presence of the current Mail version,
 * give the message count, and print a header listing.
 */
void 
announce(int printheaders)
{
	int vec[2], mdot;

	mdot = newfileinfo();
	vec[0] = mdot;
	vec[1] = 0;
	dot = &message[mdot - 1];
	if (printheaders && msgCount > 0 && value("header") != NULL) {
		inithdr++;
		headers(vec);
		inithdr = 0;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
int 
newfileinfo(void)
{
	struct message *mp;
	int u, n, mdot, d, s, hidden, killed, moved;
	char fname[PATHSIZE], zname[PATHSIZE], *ename;

	if (mb.mb_type == MB_VOID)
		return 1;
	mdot = getmdot(0);
	s = d = hidden = killed = moved =0;
	for (mp = &message[0], n = 0, u = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW)
			n++;
		if ((mp->m_flag & MREAD) == 0)
			u++;
		if ((mp->m_flag & (MDELETED|MSAVED)) == (MDELETED|MSAVED))
			moved++;
		if ((mp->m_flag & (MDELETED|MSAVED)) == MDELETED)
			d++;
		if ((mp->m_flag & (MDELETED|MSAVED)) == MSAVED)
			s++;
		if (mp->m_flag & MHIDDEN)
			hidden++;
		if (mp->m_flag & MKILL)
			killed++;
	}
	ename = mailname;
	if (getfold(fname, sizeof fname - 1) >= 0) {
		strcat(fname, "/");
		if (which_protocol(fname) != PROTO_IMAP &&
				strncmp(fname, mailname, strlen(fname)) == 0) {
			snprintf(zname, sizeof zname, "+%s",
					mailname + strlen(fname));
			ename = zname;
		}
	}
	printf(catgets(catd, CATSET, 103, "\"%s\": "), ename);
	if (msgCount == 1)
		printf(catgets(catd, CATSET, 104, "1 message"));
	else
		printf(catgets(catd, CATSET, 105, "%d messages"), msgCount);
	if (n > 0)
		printf(catgets(catd, CATSET, 106, " %d new"), n);
	if (u-n > 0)
		printf(catgets(catd, CATSET, 107, " %d unread"), u);
	if (d > 0)
		printf(catgets(catd, CATSET, 108, " %d deleted"), d);
	if (s > 0)
		printf(catgets(catd, CATSET, 109, " %d saved"), s);
	if (moved > 0)
		printf(catgets(catd, CATSET, 109, " %d moved"), moved);
	if (hidden > 0)
		printf(catgets(catd, CATSET, 109, " %d hidden"), hidden);
	if (killed > 0)
		printf(catgets(catd, CATSET, 109, " %d killed"), killed);
	if (mb.mb_type == MB_CACHE)
		printf(" [Disconnected]");
	else if (mb.mb_perm == 0)
		printf(catgets(catd, CATSET, 110, " [Read only]"));
	printf("\n");
	return(mdot);
}

int 
getmdot(int newmail)
{
	struct message	*mp;
	char	*cp;
	int	mdot;
	enum mflag	avoid = MHIDDEN|MKILL|MDELETED;

	if (!newmail) {
		if (value("autothread"))
			thread(NULL);
		else if ((cp = value("autosort")) != NULL) {
			free(mb.mb_sorted);
			mb.mb_sorted = sstrdup(cp);
			sort(NULL);
		}
	}
	if (mb.mb_type == MB_VOID)
		return 1;
	if (newmail)
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if ((mp->m_flag & (MNEWEST|avoid)) == MNEWEST)
				break;
	if (!newmail || mp >= &message[msgCount]) {
		for (mp = mb.mb_threaded ? threadroot : &message[0];
				mb.mb_threaded ?
					mp != NULL : mp < &message[msgCount];
				mb.mb_threaded ?
					mp = next_in_thread(mp) : mp++)
			if ((mp->m_flag & (MNEW|avoid)) == MNEW)
				break;
	}
	if (mb.mb_threaded ? mp == NULL : mp >= &message[msgCount])
		for (mp = mb.mb_threaded ? threadroot : &message[0];
				mb.mb_threaded ? mp != NULL:
					mp < &message[msgCount];
				mb.mb_threaded ? mp = next_in_thread(mp) : mp++)
			if (mp->m_flag & MFLAGGED)
				break;
	if (mb.mb_threaded ? mp == NULL : mp >= &message[msgCount])
		for (mp = mb.mb_threaded ? threadroot : &message[0];
				mb.mb_threaded ? mp != NULL:
					mp < &message[msgCount];
				mb.mb_threaded ? mp = next_in_thread(mp) : mp++)
			if ((mp->m_flag & (MREAD|avoid)) == 0)
				break;
	if (mb.mb_threaded ? mp != NULL : mp < &message[msgCount])
		mdot = mp - &message[0] + 1;
	else if (value("showlast")) {
		if (mb.mb_threaded) {
			for (mp = this_in_thread(threadroot, -1); mp;
					mp = prev_in_thread(mp))
				if ((mp->m_flag & avoid) == 0)
					break;
			mdot = mp ? mp - &message[0] + 1 : msgCount;
		} else {
			for (mp = &message[msgCount-1]; mp >= &message[0]; mp--)
				if ((mp->m_flag & avoid) == 0)
					break;
			mdot = mp >= &message[0] ? mp-&message[0]+1 : msgCount;
		}
	} else if (mb.mb_threaded) {
		for (mp = threadroot; mp; mp = next_in_thread(mp))
			if ((mp->m_flag & avoid) == 0)
				break;
		mdot = mp ? mp - &message[0] + 1 : 1;
	} else {
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if ((mp->m_flag & avoid) == 0)
				break;
		mdot = mp < &message[msgCount] ? mp-&message[0]+1 : 1;
	}
	return mdot;
}

/*
 * Print the current version number.
 */

/*ARGSUSED*/
int 
pversion(void *v)
{
	printf(catgets(catd, CATSET, 111, "Version %s\n"), version);
	return(0);
}

/*
 * Load a file of user definitions.
 */
void 
load(char *name)
{
	FILE *in, *oldin;

	if ((in = Fopen(name, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	Fclose(in);
}

void 
initbox(const char *name)
{
	char *tempMesg;
	int dummy;

	if (mb.mb_type != MB_VOID) {
		strncpy(prevfile, mailname, PATHSIZE);
		prevfile[PATHSIZE-1]='\0';
	}
	if (name != mailname) {
		strncpy(mailname, name, PATHSIZE);
		mailname[PATHSIZE-1]='\0';
	}
	if ((mb.mb_otf = Ftemp(&tempMesg, "Rx", "w", 0600, 0)) == NULL) {
		perror(catgets(catd, CATSET, 87,
					"temporary mail message file"));
		exit(1);
	}
	fcntl(fileno(mb.mb_otf), F_SETFD, FD_CLOEXEC);
	if ((mb.mb_itf = safe_fopen(tempMesg, "r", &dummy)) == NULL) {
		perror(tempMesg);
		exit(1);
	}
	fcntl(fileno(mb.mb_itf), F_SETFD, FD_CLOEXEC);
	rm(tempMesg);
	Ftfree(&tempMesg);
	msgCount = 0;
	if (message) {
		free(message);
		message = NULL;
		msgspace = 0;
	}
	mb.mb_threaded = 0;
	free(mb.mb_sorted);
	mb.mb_sorted = NULL;
	mb.mb_flags = MB_NOFLAGS;
	prevdot = NULL;
	dot = NULL;
	did_print_dot = 0;
}

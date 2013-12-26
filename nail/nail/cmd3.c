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
static char sccsid[] = "@(#)cmd3.c	2.87 (gritter) 10/1/08";
#endif
#endif /* not lint */

#include <math.h>
#include <float.h>
#include "rcv.h"
#include "extern.h"
#include <unistd.h>
#include <errno.h>

/*
 * Mail -- a mail program
 *
 * Still more user commands.
 */

static int bangexp(char **str, size_t *size);
static void make_ref_and_cs(struct message *mp, struct header *head);
static int (*respond_or_Respond(int c))(int *, int);
static int respond_internal(int *msgvec, int recipient_record);
static char *reedit(char *subj);
static char *fwdedit(char *subj);
static void onpipe(int signo);
static void asort(char **list);
static int diction(const void *a, const void *b);
static int file1(char *name);
static int shellecho(const char *cp);
static int Respond_internal(int *msgvec, int recipient_record);
static int resend1(void *v, int add_resent);
static void list_shortcuts(void);
static enum okay delete_shortcut(const char *str);
static float huge(void);

/*
 * Process a shell escape by saving signals, ignoring signals,
 * and forking a sh -c
 */
int 
shell(void *v)
{
	char *str = v;
	sighandler_type sigint = safe_signal(SIGINT, SIG_IGN);
	char *shell;
	char *cmd;
	size_t cmdsize;

	cmd = smalloc(cmdsize = strlen(str) + 1);
	strcpy(cmd, str);
	if (bangexp(&cmd, &cmdsize) < 0)
		return 1;
	if ((shell = value("SHELL")) == NULL)
		shell = SHELL;
	run_command(shell, 0, -1, -1, "-c", cmd, NULL);
	safe_signal(SIGINT, sigint);
	printf("!\n");
	free(cmd);
	return 0;
}

/*
 * Fork an interactive shell.
 */
/*ARGSUSED*/
int 
dosh(void *v)
{
	sighandler_type sigint = safe_signal(SIGINT, SIG_IGN);
	char *shell;

	if ((shell = value("SHELL")) == NULL)
		shell = SHELL;
	run_command(shell, 0, -1, -1, NULL, NULL, NULL);
	safe_signal(SIGINT, sigint);
	putchar('\n');
	return 0;
}

/*
 * Expand the shell escape by expanding unescaped !'s into the
 * last issued command where possible.
 */

static char	*lastbang;
static size_t	lastbangsize;

static int
bangexp(char **str, size_t *size)
{
	char *bangbuf;
	int changed = 0;
	int dobang = value("bang") != NULL;
	size_t sz, i, j, bangbufsize;

	bangbuf = smalloc(bangbufsize = *size);
	i = j = 0;
	while ((*str)[i]) {
		if (dobang) {
			if ((*str)[i] == '!') {
				sz = strlen(lastbang);
				bangbuf = srealloc(bangbuf, bangbufsize += sz);
				changed++;
				strcpy(&bangbuf[j], lastbang);
				j += sz;
				i++;
				continue;
			}
		}
		if ((*str)[i] == '\\' && (*str)[i + 1] == '!') {
			bangbuf[j++] = '!';
			i += 2;
			changed++;
		}
		bangbuf[j++] = (*str)[i++];
	}
	bangbuf[j] = '\0';
	if (changed) {
		printf("!%s\n", bangbuf);
		fflush(stdout);
	}
	sz = j;
	if (sz >= *size)
		*str = srealloc(*str, *size = sz + 1);
	strcpy(*str, bangbuf);
	if (sz >= lastbangsize)
		lastbang = srealloc(lastbang, lastbangsize = sz + 1);
	strcpy(lastbang, bangbuf);
	free(bangbuf);
	return(0);
}

/*ARGSUSED*/
int 
help(void *v)
{
	const char *helptext =
"               %s commands\n\
type <message list>             type messages\n\
next                            goto and type next message\n\
from <message list>             give head lines of messages\n\
headers                         print out active message headers\n\
delete <message list>           delete messages\n\
undelete <message list>         undelete messages\n\
save <message list> folder      append messages to folder and mark as saved\n\
copy <message list> folder      append messages to folder without marking them\n\
write <message list> file       append message texts to file, save attachments\n\
preserve <message list>         keep incoming messages in mailbox even if saved\n\
Reply <message list>            reply to message senders\n\
reply <message list>            reply to message senders and all recipients\n\
mail addresses                  mail to specific recipients\n\
file folder                     change to another folder\n\
quit                            quit and apply changes to folder\n\
xit                             quit and discard changes made to folder\n\
!                               shell escape\n\
cd <directory>                  chdir to directory or home if none given\n\
list                            list names of all available commands\n\
\n\
A <message list> consists of integers, ranges of same, or other criteria\n\
separated by spaces.  If omitted, %s uses the last message typed.\n";

	fprintf(stdout, helptext, progname, progname);
	return(0);
}

/*
 * Change user's working directory.
 */
int 
schdir(void *v)
{
	char **arglist = v;
	char *cp;

	if (*arglist == NULL)
		cp = homedir;
	else
		if ((cp = expand(*arglist)) == NULL)
			return(1);
	if (chdir(cp) < 0) {
		perror(cp);
		return(1);
	}
	return 0;
}

static void 
make_ref_and_cs(struct message *mp, struct header *head)
{
	char *oldref, *oldmsgid, *newref, *cp;
	size_t reflen;
	unsigned i;
	struct name *n;

	oldref = hfield("references", mp);
	oldmsgid = hfield("message-id", mp);
	if (oldmsgid == NULL || *oldmsgid == '\0') {
		head->h_ref = NULL;
		return;
	}
	reflen = 1;
	if (oldref)
		reflen += strlen(oldref) + 2;
	if (oldmsgid)
		reflen += strlen(oldmsgid);
	newref = ac_alloc(reflen);
	if (oldref) {
		strcpy(newref, oldref);
		if (oldmsgid) {
			strcat(newref, ", ");
			strcat(newref, oldmsgid);
		}
	} else if (oldmsgid)
		strcpy(newref, oldmsgid);
	n = extract(newref, GREF);
	ac_free(newref);
	/*
	 * Limit the references to 21 entries.
	 */
	while (n->n_flink != NULL)
		n = n->n_flink;
	for (i = 1; i < 21; i++) {
		if (n->n_blink != NULL)
			n = n->n_blink;
		else
			break;
	}
	n->n_blink = NULL;
	head->h_ref = n;
	if (value("reply-in-same-charset") != NULL &&
			(cp = hfield("content-type", mp)) != NULL)
		head->h_charset = mime_getparam("charset", cp);
}

static int
(*respond_or_Respond(int c))(int *, int)
{
	int opt = 0;

	opt += (value("Replyall") != NULL);
	opt += (value("flipr") != NULL);
	return ((opt == 1) ^ (c == 'R')) ? Respond_internal : respond_internal;
}

int 
respond(void *v)
{
	return (respond_or_Respond('r'))((int *)v, 0);
}

int 
respondall(void *v)
{
	return respond_internal((int *)v, 0);
}

int 
respondsender(void *v)
{
	return Respond_internal((int *)v, 0);
}

int 
followup(void *v)
{
	return (respond_or_Respond('r'))((int *)v, 1);
}

int 
followupall(void *v)
{
	return respond_internal((int *)v, 1);
}

int 
followupsender(void *v)
{
	return Respond_internal((int *)v, 1);
}

/*
 * Reply to a list of messages.  Extract each name from the
 * message header and send them off to mail1()
 */
static int 
respond_internal(int *msgvec, int recipient_record)
{
	int Eflag;
	struct message *mp;
	char *cp, *rcv;
	enum gfield	gf = value("fullnames") ? GFULL : GSKIN;
	struct name *np = NULL;
	struct header head;

	memset(&head, 0, sizeof head);
	if (msgvec[1] != 0) {
		printf(catgets(catd, CATSET, 37,
			"Sorry, can't reply to multiple messages at once\n"));
		return(1);
	}
	mp = &message[msgvec[0] - 1];
	touch(mp);
	setdot(mp);
	if ((rcv = hfield("reply-to", mp)) == NULL)
		if ((rcv = hfield("from", mp)) == NULL)
			rcv = nameof(mp, 1);
	if (rcv != NULL)
		np = sextract(rcv, GTO|gf);
	if ((cp = hfield("to", mp)) != NULL)
		np = cat(np, sextract(cp, GTO|gf));
	np = elide(np);
	/*
	 * Delete my name from the reply list,
	 * and with it, all my alternate names.
	 */
	np = delete_alternates(np);
	if (np == NULL)
		np = sextract(rcv, GTO|gf);
	head.h_to = np;
	if ((head.h_subject = hfield("subject", mp)) == NULL)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	if ((cp = hfield("cc", mp)) != NULL) {
		np = elide(sextract(cp, GCC|gf));
		np = delete_alternates(np);
		head.h_cc = np;
	}
	make_ref_and_cs(mp, &head);
	Eflag = value("skipemptybody") != NULL;
	if (mail1(&head, 1, mp, NULL, recipient_record, 0, 0, Eflag) == OKAY &&
			value("markanswered") && (mp->m_flag & MANSWERED) == 0)
		mp->m_flag |= MANSWER|MANSWERED;
	return(0);
}

/*
 * Modify the subject we are replying to to begin with Re: if
 * it does not already.
 */
static char *
reedit(char *subj)
{
	char *newsubj;
	struct str in, out;

	if (subj == NULL || *subj == '\0')
		return NULL;
	in.s = subj;
	in.l = strlen(subj);
	mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
	if ((out.s[0] == 'r' || out.s[0] == 'R') &&
	    (out.s[1] == 'e' || out.s[1] == 'E') &&
	    out.s[2] == ':')
		return out.s;
	newsubj = salloc(out.l + 5);
	strcpy(newsubj, "Re: ");
	strcpy(newsubj + 4, out.s);
	return newsubj;
}

/*
 * Forward a message to a new recipient, in the sense of RFC 2822.
 */
static int
forward1(char *str, int recipient_record)
{
	int Eflag;
	int	*msgvec, f;
	char	*recipient;
	struct message	*mp;
	struct header	head;
	int	forward_as_attachment;

	forward_as_attachment = value("forward-as-attachment") != NULL;
	msgvec = salloc((msgCount + 2) * sizeof *msgvec);
	if ((recipient = laststring(str, &f, 0)) == NULL) {
		puts(catgets(catd, CATSET, 47, "No recipient specified."));
		return 1;
	}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			if (inhook)
				return 0;
			printf("No messages to forward.\n");
			return 1;
		}
		msgvec[1] = 0;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return 1;
	if (*msgvec == 0) {
		if (inhook)
			return 0;
		printf("No applicable messages.\n");
		return 1;
	}
	if (msgvec[1] != 0) {
		printf("Cannot forward multiple messages at once\n");
		return 1;
	}
	memset(&head, 0, sizeof head);
	if ((head.h_to = sextract(recipient,
			GTO | (value("fullnames") ? GFULL : GSKIN))) == NULL)
		return 1;
	mp = &message[*msgvec - 1];
	if (forward_as_attachment) {
		head.h_attach = csalloc(1, sizeof *head.h_attach);
		head.h_attach->a_msgno = *msgvec;
	} else {
		touch(mp);
		setdot(mp);
	}
	if ((head.h_subject = hfield("subject", mp)) == NULL)
		head.h_subject = hfield("subj", mp);
	head.h_subject = fwdedit(head.h_subject);
	Eflag = value("skipemptybody") != NULL;
	mail1(&head, 1, forward_as_attachment ? NULL : mp,
			NULL, recipient_record, 1, 0, Eflag);
	return 0;
}

/*
 * Modify the subject we are replying to to begin with Fwd:.
 */
static char *
fwdedit(char *subj)
{
	char *newsubj;
	struct str	in, out;

	if (subj == NULL || *subj == '\0')
		return NULL;
	in.s = subj;
	in.l = strlen(subj);
	mime_fromhdr(&in, &out, TD_ISPR|TD_ICONV);
	newsubj = salloc(strlen(out.s) + 6);
	strcpy(newsubj, "Fwd: ");
	strcpy(&newsubj[5], out.s);
	free(out.s);
	return newsubj;
}

/*
 * The 'forward' command.
 */
int
forwardcmd(void *v)
{
	return forward1(v, 0);
}

/*
 * Similar to forward, saving the message in a file named after the
 * first recipient.
 */
int
Forwardcmd(void *v)
{
	return forward1(v, 1);
}

/*
 * Preserve the named messages, so that they will be sent
 * back to the system mailbox.
 */
int 
preserve(void *v)
{
	int *msgvec = v;
	struct message *mp;
	int *ip, mesg;

	if (edit) {
		printf(catgets(catd, CATSET, 39,
				"Cannot \"preserve\" in edit mode\n"));
		return(1);
	}
	for (ip = msgvec; *ip != 0; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		mp->m_flag |= MPRESERVE;
		mp->m_flag &= ~MBOX;
		setdot(mp);
		/*
		 * This is now Austin Group Request XCU #20.
		 */
		did_print_dot = 1;
	}
	return(0);
}

/*
 * Mark all given messages as unread.
 */
int 
unread(void *v)
{
	int	*msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		setdot(&message[*ip-1]);
		dot->m_flag &= ~(MREAD|MTOUCH);
		dot->m_flag |= MSTATUS;
		if (mb.mb_type == MB_IMAP || mb.mb_type == MB_CACHE)
			imap_unread(&message[*ip-1], *ip);
		/*
		 * The "unread" command is not part of POSIX mailx.
		 */
		did_print_dot = 1;
	}
	return(0);
}

/*
 * Mark all given messages as read.
 */
int
seen(void *v)
{
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip; ip++) {
		setdot(&message[*ip-1]);
		touch(&message[*ip-1]);
	}
	return 0;
}

/*
 * Print the size of each message.
 */
int 
messize(void *v)
{
	int *msgvec = v;
	struct message *mp;
	int *ip, mesg;

	for (ip = msgvec; *ip != 0; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		printf("%d: ", mesg);
		if (mp->m_xlines > 0)
			printf("%ld", mp->m_xlines);
		else
			putchar(' ');
		printf("/%lu\n", (unsigned long)mp->m_xsize);
	}
	return(0);
}

/*
 * Quit quickly.  If we are sourcing, just pop the input level
 * by returning an error.
 */
/*ARGSUSED*/
int 
rexit(void *v)
{
	if (sourcing)
		return(1);
	exit(0);
	/*NOTREACHED*/
}

static sigjmp_buf	pipejmp;

/*ARGSUSED*/
static void 
onpipe(int signo)
{
	siglongjmp(pipejmp, 1);
}

/*
 * Set or display a variable value.  Syntax is similar to that
 * of sh.
 */
int 
set(void *v)
{
	char **arglist = v;
	struct var *vp;
	char *cp, *cp2;
	char **ap, **p;
	int errs, h, s;
	FILE *obuf = stdout;
	int bsdset = value("bsdcompat") != NULL || value("bsdset") != NULL;

	(void)&cp;
	(void)&ap;
	(void)&obuf;
	(void)&bsdset;
	if (*arglist == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				s++;
		/*LINTED*/
		ap = (char **)salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				*p++ = vp->v_name;
		*p = NULL;
		asort(ap);
		if (is_a_tty[0] && is_a_tty[1] && (cp = value("crt")) != NULL) {
			if (s > (*cp == '\0' ? screensize() : atoi(cp)) + 3) {
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
		for (p = ap; *p != NULL; p++) {
			if (bsdset)
				fprintf(obuf, "%s\t%s\n", *p, value(*p));
			else {
				if ((cp = value(*p)) != NULL && *cp)
					fprintf(obuf, "%s=\"%s\"\n",
							*p, value(*p));
				else
					fprintf(obuf, "%s\n", *p);
			}
		}
endpipe:
		if (obuf != stdout) {
			safe_signal(SIGPIPE, SIG_IGN);
			Pclose(obuf);
			safe_signal(SIGPIPE, dflpipe);
		}
		return(0);
	}
	errs = 0;
	for (ap = arglist; *ap != NULL; ap++) {
		char *varbuf;

		varbuf = ac_alloc(strlen(*ap) + 1);
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			printf(catgets(catd, CATSET, 41,
					"Non-null variable name required\n"));
			errs++;
			ac_free(varbuf);
			continue;
		}
		if (varbuf[0] == 'n' && varbuf[1] == 'o')
			errs += unset_internal(&varbuf[2]);
		else
			assign(varbuf, cp);
		ac_free(varbuf);
	}
	return(errs);
}

/*
 * Unset a bunch of variable values.
 */
int 
unset(void *v)
{
	int errs;
	char **ap;

	errs = 0;
	for (ap = (char **)v; *ap != NULL; ap++)
		errs += unset_internal(*ap);
	return(errs);
}

/*
 * Put add users to a group.
 */
int 
group(void *v)
{
	char **argv = v;
	struct grouphead *gh;
	struct group *gp;
	int h;
	int s;
	char **ap, *gname, **p;

	if (*argv == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NULL; gh = gh->g_link)
				s++;
		/*LINTED*/
		ap = (char **)salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NULL; gh = gh->g_link)
				*p++ = gh->g_name;
		*p = NULL;
		asort(ap);
		for (p = ap; *p != NULL; p++)
			printgroup(*p);
		return(0);
	}
	if (argv[1] == NULL) {
		printgroup(*argv);
		return(0);
	}
	gname = *argv;
	h = hash(gname);
	if ((gh = findgroup(gname)) == NULL) {
		gh = (struct grouphead *)scalloc(1, sizeof *gh);
		gh->g_name = vcopy(gname);
		gh->g_list = NULL;
		gh->g_link = groups[h];
		groups[h] = gh;
	}

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	for (ap = argv+1; *ap != NULL; ap++) {
		gp = (struct group *)scalloc(1, sizeof *gp);
		gp->ge_name = vcopy(*ap);
		gp->ge_link = gh->g_list;
		gh->g_list = gp;
	}
	return(0);
}

/*
 * Delete the passed groups.
 */
int 
ungroup(void *v)
{
	char **argv = v;

	if (*argv == NULL) {
		printf(catgets(catd, CATSET, 209,
				"Must specify alias or group to remove\n"));
		return 1;
	}
	do
		remove_group(*argv);
	while (*++argv != NULL);
	return 0;
}

/*
 * Sort the passed string vecotor into ascending dictionary
 * order.
 */
static void 
asort(char **list)
{
	char **ap;

	for (ap = list; *ap != NULL; ap++)
		;
	if (ap-list < 2)
		return;
	qsort(list, ap-list, sizeof(*list), diction);
}

/*
 * Do a dictionary order comparison of the arguments from
 * qsort.
 */
static int 
diction(const void *a, const void *b)
{
	return(strcmp(*(char **)a, *(char **)b));
}

/*
 * Change to another file.  With no argument, print information about
 * the current file.
 */
int 
cfile(void *v)
{
	char **argv = v;

	if (argv[0] == NULL) {
		newfileinfo();
		return 0;
	}
	strncpy(mboxname, expand("&"), sizeof mboxname)[sizeof mboxname-1]='\0';
	return file1(*argv);
}

static int 
file1(char *name)
{
	int	i;

	if (inhook) {
		fprintf(stderr, "Cannot change folder from within a hook.\n");
		return 1;
	}
	i = setfile(name, 0);
	if (i < 0)
		return 1;
	callhook(mailname, 0);
	if (i > 0 && value("emptystart") == NULL)
		return 1;
	announce(value("bsdcompat") != NULL || value("bsdannounce") != NULL);
	return 0;
}

static int
shellecho(const char *cp)
{
	int	cflag = 0, n;
	char	c;

	while (*cp) {
		if (*cp == '\\') {
			switch (*++cp) {
			case '\0':
				return cflag;
			case 'a':
				putchar('\a');
				break;
			case 'b':
				putchar('\b');
				break;
			case 'c':
				cflag = 1;
				break;
			case 'f':
				putchar('\f');
				break;
			case 'n':
				putchar('\n');
				break;
			case 'r':
				putchar('\r');
				break;
			case 't':
				putchar('\t');
				break;
			case 'v':
				putchar('\v');
				break;
			default:
				putchar(*cp&0377);
				break;
			case '0':
				c = 0;
				n = 3;
				while (n-- && octalchar(cp[1]&0377)) {
					c <<= 3;
					c |= cp[1] - '0';
					cp++;
				}
				putchar(c);
			}
		} else
			putchar(*cp & 0377);
		cp++;
	}
	return cflag;
}

/*
 * Expand file names like echo
 */
int 
echo(void *v)
{
	char **argv = v;
	char **ap;
	char *cp;
	int cflag = 0;

	for (ap = argv; *ap != NULL; ap++) {
		cp = *ap;
		if ((cp = expand(cp)) != NULL) {
			if (ap != argv)
				putchar(' ');
			cflag |= shellecho(cp);
		}
	}
	if (!cflag)
		putchar('\n');
	return 0;
}

int 
Respond(void *v)
{
	return (respond_or_Respond('R'))((int *)v, 0);
}

int 
Followup(void *v)
{
	return (respond_or_Respond('R'))((int *)v, 1);
}

/*
 * Reply to a series of messages by simply mailing to the senders
 * and not messing around with the To: and Cc: lists as in normal
 * reply.
 */
static int 
Respond_internal(int *msgvec, int recipient_record)
{
	int Eflag;
	struct header head;
	struct message *mp;
	enum gfield	gf = value("fullnames") ? GFULL : GSKIN;
	int *ap;
	char *cp;

	memset(&head, 0, sizeof head);
	for (ap = msgvec; *ap != 0; ap++) {
		mp = &message[*ap - 1];
		touch(mp);
		setdot(mp);
		if ((cp = hfield("reply-to", mp)) == NULL)
			if ((cp = hfield("from", mp)) == NULL)
				cp = nameof(mp, 2);
		head.h_to = cat(head.h_to, sextract(cp, GTO|gf));
	}
	if (head.h_to == NULL)
		return 0;
	mp = &message[msgvec[0] - 1];
	if ((head.h_subject = hfield("subject", mp)) == NULL)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	make_ref_and_cs(mp, &head);
	Eflag = value("skipemptybody") != NULL;
	if (mail1(&head, 1, mp, NULL, recipient_record, 0, 0, Eflag) == OKAY &&
			value("markanswered") && (mp->m_flag & MANSWERED) == 0)
		mp->m_flag |= MANSWER|MANSWERED;
	return 0;
}

/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */
int 
ifcmd(void *v)
{
	char **argv = v;
	char *cp;

	if (cond != CANY) {
		printf(catgets(catd, CATSET, 42, "Illegal nested \"if\"\n"));
		return(1);
	}
	cond = CANY;
	cp = argv[0];
	switch (*cp) {
	case 'r': case 'R':
		cond = CRCV;
		break;

	case 's': case 'S':
		cond = CSEND;
		break;

	case 't': case 'T':
		cond = CTERM;
		break;

	default:
		printf(catgets(catd, CATSET, 43,
				"Unrecognized if-keyword: \"%s\"\n"), cp);
		return(1);
	}
	return(0);
}

/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */
/*ARGSUSED*/
int 
elsecmd(void *v)
{

	switch (cond) {
	case CANY:
		printf(catgets(catd, CATSET, 44,
				"\"Else\" without matching \"if\"\n"));
		return(1);

	case CSEND:
		cond = CRCV;
		break;

	case CRCV:
		cond = CSEND;
		break;

	case CTERM:
		cond = CNONTERM;
		break;

	default:
		printf(catgets(catd, CATSET, 45,
				"Mail's idea of conditions is screwed up\n"));
		cond = CANY;
		break;
	}
	return(0);
}

/*
 * End of if statement.  Just set cond back to anything.
 */
/*ARGSUSED*/
int 
endifcmd(void *v)
{

	if (cond == CANY) {
		printf(catgets(catd, CATSET, 46,
				"\"Endif\" without matching \"if\"\n"));
		return(1);
	}
	cond = CANY;
	return(0);
}

/*
 * Set the list of alternate names.
 */
int 
alternates(void *v)
{
	char **namelist = v;
	int c;
	char **ap, **ap2, *cp;

	c = argcount(namelist) + 1;
	if (c == 1) {
		if (altnames == 0)
			return(0);
		for (ap = altnames; *ap; ap++)
			printf("%s ", *ap);
		printf("\n");
		return(0);
	}
	if (altnames != 0)
		free(altnames);
	altnames = scalloc(c, sizeof (char *));
	for (ap = namelist, ap2 = altnames; *ap; ap++, ap2++) {
		cp = scalloc(strlen(*ap) + 1, sizeof (char));
		strcpy(cp, *ap);
		*ap2 = cp;
	}
	*ap2 = 0;
	return(0);
}

/*
 * Do the real work of resending.
 */
static int 
resend1(void *v, int add_resent)
{
	char *name, *str;
	struct name *to;
	struct name *sn;
	int f, *ip, *msgvec;

	str = (char *)v;
	/*LINTED*/
	msgvec = (int *)salloc((msgCount + 2) * sizeof *msgvec);
	name = laststring(str, &f, 1);
	if (name == NULL) {
		puts(catgets(catd, CATSET, 47, "No recipient specified."));
		return 1;
	}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			if (inhook)
				return 0;
			puts(catgets(catd, CATSET, 48,
					"No applicable messages."));
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
	sn = nalloc(name, GTO);
	to = usermap(sn);
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		if (resend_msg(&message[*ip - 1], to, add_resent) != OKAY)
			return 1;
	}
	return 0;
}

/*
 * Resend a message list to a third person.
 */
int 
resendcmd(void *v)
{
	return resend1(v, 1);
}

/*
 * Resend a message list to a third person without adding headers.
 */
int 
Resendcmd(void *v)
{
	return resend1(v, 0);
}

/*
 * 'newmail' or 'inc' command: Check for new mail without writing old
 * mail back.
 */
/*ARGSUSED*/
int 
newmail(void *v)
{
	int val = 1, mdot;

	if ((mb.mb_type != MB_IMAP || imap_newmail(1)) &&
			(val = setfile(mailname, 1)) == 0) {
		mdot = getmdot(1);
		setdot(&message[mdot - 1]);
	}
	return val;
}

static void 
list_shortcuts(void)
{
	struct shortcut *s;

	for (s = shortcuts; s; s = s->sh_next)
		printf("%s=%s\n", s->sh_short, s->sh_long);
}

int 
shortcut(void *v)
{
	char **args = (char **)v;
	struct shortcut *s;

	if (args[0] == NULL) {
		list_shortcuts();
		return 0;
	}
	if (args[1] == NULL) {
		fprintf(stderr, catgets(catd, CATSET, 220,
				"expansion name for shortcut missing\n"));
		return 1;
	}
	if (args[2] != NULL) {
		fprintf(stderr, catgets(catd, CATSET, 221,
				"too many arguments\n"));
		return 1;
	}
	if ((s = get_shortcut(args[0])) != NULL) {
		free(s->sh_long);
		s->sh_long = sstrdup(args[1]);
	} else {
		s = scalloc(1, sizeof *s);
		s->sh_short = sstrdup(args[0]);
		s->sh_long = sstrdup(args[1]);
		s->sh_next = shortcuts;
		shortcuts = s;
	}
	return 0;
}

struct shortcut *
get_shortcut(const char *str)
{
	struct shortcut *s;

	for (s = shortcuts; s; s = s->sh_next)
		if (strcmp(str, s->sh_short) == 0)
			break;
	return s;
}

static enum okay 
delete_shortcut(const char *str)
{
	struct shortcut *sp, *sq;

	for (sp = shortcuts, sq = NULL; sp; sq = sp, sp = sp->sh_next) {
		if (strcmp(sp->sh_short, str) == 0) {
			free(sp->sh_short);
			free(sp->sh_long);
			if (sq)
				sq->sh_next = sp->sh_next;
			if (sp == shortcuts)
				shortcuts = sp->sh_next;
			free(sp);
			return OKAY;
		}
	}
	return STOP;
}

int 
unshortcut(void *v)
{
	char **args = (char **)v;
	int errs = 0;

	if (args[0] == NULL) {
		fprintf(stderr, catgets(catd, CATSET, 222,
				"need shortcut names to remove\n"));
		return 1;
	}
	while (*args != NULL) {
		if (delete_shortcut(*args) != OKAY) {
			errs = 1;
			fprintf(stderr, catgets(catd, CATSET, 223,
				"%s: no such shortcut\n"), *args);
		}
		args++;
	}
	return errs;
}

struct oldaccount {
	struct oldaccount	*ac_next;	/* next account in list */
	char	*ac_name;			/* name of account */
	char	**ac_vars;			/* variables to set */
};

static struct oldaccount	*oldaccounts;

struct oldaccount *
get_oldaccount(const char *name)
{
	struct oldaccount	*a;

	for (a = oldaccounts; a; a = a->ac_next)
		if (a->ac_name && strcmp(name, a->ac_name) == 0)
			break;
	return a;
}

int 
account(void *v)
{
	char	**args = (char **)v;
	struct oldaccount	*a;
	char	*cp;
	int	i, mc, oqf, nqf;
	FILE	*fp = stdout;

	if (args[0] == NULL) {
		if ((fp = Ftemp(&cp, "Ra", "w+", 0600, 1)) == NULL) {
			perror("tmpfile");
			return 1;
		}
		rm(cp);
		Ftfree(&cp);
		mc = listaccounts(fp);
		for (a = oldaccounts; a; a = a->ac_next)
			if (a->ac_name) {
				if (mc++)
					fputc('\n', fp);
				fprintf(fp, "%s:\n", a->ac_name);
				for (i = 0; a->ac_vars[i]; i++)
					fprintf(fp, "\t%s\n", a->ac_vars[i]);
			}
		if (mc)
			try_pager(fp);
		Fclose(fp);
		return 0;
	}
	if (args[1] && args[1][0] == '{' && args[1][1] == '\0') {
		if (args[2] != NULL) {
			fprintf(stderr, "Syntax is: account <name> {\n");
			return 1;
		}
		if ((a = get_oldaccount(args[0])) != NULL)
			a->ac_name = NULL;
		return define1(args[0], 1);
	}
	strncpy(mboxname, expand("&"), sizeof mboxname)[sizeof mboxname-1]='\0';
	oqf = savequitflags();
	if ((a = get_oldaccount(args[0])) == NULL) {
		if (args[1]) {
			a = scalloc(1, sizeof *a);
			a->ac_next = oldaccounts;
			oldaccounts = a;
		} else {
			if ((i = callaccount(args[0])) != CBAD)
				goto setf;
			printf("Account %s does not exist.\n", args[0]);
			return 1;
		}
	}
	if (args[1]) {
		delaccount(args[0]);
		a->ac_name = sstrdup(args[0]);
		for (i = 1; args[i]; i++);
		a->ac_vars = scalloc(i, sizeof *a->ac_vars);
		for (i = 0; args[i+1]; i++)
			a->ac_vars[i] = sstrdup(args[i+1]);
	} else {
		unset_allow_undefined = 1;
		set(a->ac_vars);
		unset_allow_undefined = 0;
	setf:	if (!starting) {
			nqf = savequitflags();
			restorequitflags(oqf);
			i = file1("%");
			restorequitflags(nqf);
			return i;
		}
	}
	return 0;
}

int 
cflag(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if ((m->m_flag & (MFLAG|MFLAGGED)) == 0)
			m->m_flag |= MFLAG|MFLAGGED;
	}
	return 0;
}

int 
cunflag(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if (m->m_flag & (MFLAG|MFLAGGED)) {
			m->m_flag &= ~(MFLAG|MFLAGGED);
			m->m_flag |= MUNFLAG;
		}
	}
	return 0;
}

int 
canswered(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if ((m->m_flag & (MANSWER|MANSWERED)) == 0)
			m->m_flag |= MANSWER|MANSWERED;
	}
	return 0;
}

int 
cunanswered(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if (m->m_flag & (MANSWER|MANSWERED)) {
			m->m_flag &= ~(MANSWER|MANSWERED);
			m->m_flag |= MUNANSWER;
		}
	}
	return 0;
}

int 
cdraft(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if ((m->m_flag & (MDRAFT|MDRAFTED)) == 0)
			m->m_flag |= MDRAFT|MDRAFTED;
	}
	return 0;
}

int 
cundraft(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		setdot(m);
		if (m->m_flag & (MDRAFT|MDRAFTED)) {
			m->m_flag &= ~(MDRAFT|MDRAFTED);
			m->m_flag |= MUNDRAFT;
		}
	}
	return 0;
}

static float 
huge(void)
{
#if defined (_CRAY)
	/*
	 * This is not perfect, but correct for machines with a 32-bit
	 * IEEE float and a 32-bit unsigned long, and does at least not
	 * produce SIGFPE on the Cray Y-MP.
	 */
	union {
		float	f;
		unsigned long	l;
	} u;

	u.l = 0xff800000; /* -inf */
	return u.f;
#elif defined (INFINITY)
	return -INFINITY;
#elif defined (HUGE_VALF)
	return -HUGE_VALF;
#elif defined (FLT_MAX)
	return -FLT_MAX;
#else
	return -1e10;
#endif
}

int 
ckill(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		m->m_flag |= MKILL;
		m->m_score = huge();
	}
	return 0;
}

int 
cunkill(void *v)
{
	struct message	*m;
	int	*msgvec = v;
	int	*ip;

	for (ip = msgvec; *ip != 0; ip++) {
		m = &message[*ip-1];
		m->m_flag &= ~MKILL;
		m->m_score = 0;
	}
	return 0;
}

int 
cscore(void *v)
{
	char	*str = v;
	char	*sscore, *xp;
	int	f, *msgvec, *ip;
	double	nscore;
	struct message	*m;

	msgvec = salloc((msgCount+2) * sizeof *msgvec);
	if ((sscore = laststring(str, &f, 0)) == NULL) {
		fprintf(stderr, "No score given.\n");
		return 1;
	}
	nscore = strtod(sscore, &xp);
	if (*xp) {
		fprintf(stderr, "Invalid score: \"%s\"\n", sscore);
		return 1;
	}
	if (nscore > FLT_MAX)
		nscore = FLT_MAX;
	else if (nscore < -FLT_MAX)
		nscore = -FLT_MAX;
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			if (inhook)
				return 0;
			fprintf(stderr, "No messages to score.\n");
			return 1;
		}
		msgvec[1] = 0;
	} else if (getmsglist(str, msgvec, 0) < 0)
		return 1;
	if (*msgvec == 0) {
		if (inhook)
			return 0;
		fprintf(stderr, "No applicable messages.\n");
		return 1;
	}
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		m = &message[*ip-1];
		if (m->m_score != huge()) {
			m->m_score += nscore;
			if (m->m_score < 0)
				m->m_flag |= MKILL;
			else if (m->m_score > 0)
				m->m_flag &= ~MKILL;
			if (m->m_score >= 0)
				setdot(m);
		}
	}
	return 0;
}

/*ARGSUSED*/
int 
cnoop(void *v)
{
	switch (mb.mb_type) {
	case MB_IMAP:
		imap_noop();
		break;
	case MB_POP3:
		pop3_noop();
		break;
	default:
		break;
	}
	return 0;
}

int 
cremove(void *v)
{
	char	vb[LINESIZE];
	char	**args = v;
	char	*name;
	int	ec = 0;

	if (*args == NULL) {
		fprintf(stderr, "Syntax is: remove mailbox ...\n");
		return 1;
	}
	do {
		if ((name = expand(*args)) == NULL)
			continue;
		if (strcmp(name, mailname) == 0) {
			fprintf(stderr,
				"Cannot remove current mailbox \"%s\".\n",
				name);
			ec |= 1;
			continue;
		}
		snprintf(vb, sizeof vb, "Remove \"%s\" (y/n) ? ", name);
		if (yorn(vb) == 0)
			continue;
		switch (which_protocol(name)) {
		case PROTO_FILE:
			if (unlink(name) < 0) {	/* do not handle .gz .bz2 */
				perror(name);
				ec |= 1;
			}
			break;
		case PROTO_POP3:
			fprintf(stderr, "Cannot remove POP3 mailbox \"%s\".\n",
					name);
			ec |= 1;
			break;
		case PROTO_IMAP:
			if (imap_remove(name) != OKAY)
				ec |= 1;
			break;
		case PROTO_MAILDIR:
			if (maildir_remove(name) != OKAY)
				ec |= 1;
			break;
		case PROTO_UNKNOWN:
			fprintf(stderr,
				"Unknown protocol in \"%s\". Not removed.\n",
				name);
			ec |= 1;
		}
	} while (*++args);
	return ec;
}

int 
crename(void *v)
{
	char	**args = v, *old, *new;
	enum protocol	oldp, newp;
	int	ec = 0;

	if (args[0] == NULL || args[1] == NULL || args[2] != NULL) {
		fprintf(stderr, "Syntax: rename old new\n");
		return 1;
	}
	old = expand(args[0]);
	oldp = which_protocol(old);
	new = expand(args[1]);
	newp = which_protocol(new);
	if (strcmp(old, mailname) == 0 || strcmp(new, mailname) == 0) {
		fprintf(stderr, "Cannot rename current mailbox \"%s\".\n", old);
		return 1;
	}
	if ((oldp == PROTO_IMAP || newp == PROTO_IMAP) && oldp != newp) {
		fprintf(stderr, "Can only rename folders of same type.\n");
		return 1;
	}
	if (newp == PROTO_POP3)
		goto nopop3;
	switch (oldp) {
	case PROTO_FILE:
		if (link(old, new) < 0) {
			switch (errno) {
			case EACCES:
			case EEXIST:
			case ENAMETOOLONG:
			case ENOENT:
			case ENOSPC:
			case EXDEV:
				perror(new);
				break;
			default:
				perror(old);
			}
			ec |= 1;
		} else if (unlink(old) < 0) {
			perror(old);
			ec |= 1;
		}
		break;
	case PROTO_MAILDIR:
		if (rename(old, new) < 0) {
			perror(old);
			ec |= 1;
		}
		break;
	case PROTO_POP3:
	nopop3:	fprintf(stderr, "Cannot rename POP3 mailboxes.\n");
		ec |= 1;
		break;
	case PROTO_IMAP:
		if (imap_rename(old, new) != OKAY)
			ec |= 1;
		break;
	case PROTO_UNKNOWN:
		fprintf(stderr, "Unknown protocol in \"%s\" and \"%s\". "
				"Not renamed.\n", old, new);
		ec |= 1;
	}
	return ec;
}

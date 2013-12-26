/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2002
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
static char sccsid[] = "@(#)pop3.c	2.43 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "md5.h"

/*
 * Mail -- a mail program
 *
 * POP3 client.
 */

#ifdef	HAVE_SOCKETS
static int	verbose;

#define	POP3_ANSWER()	if (pop3_answer(mp) == STOP) \
				return STOP;
#define	POP3_OUT(x, y)	if (pop3_finish(mp) == STOP) \
				return STOP; \
			if (verbose) \
				fprintf(stderr, ">>> %s", x); \
			mp->mb_active |= (y); \
			if (swrite(&mp->mb_sock, x) == STOP) \
				return STOP;

static char	*pop3buf;
static size_t	pop3bufsize;
static sigjmp_buf	pop3jmp;
static sighandler_type savealrm;
static int	reset_tio;
static struct termios	otio;
static int	pop3keepalive;
static volatile int	pop3lock;

static void pop3_timer_off(void);
static enum okay pop3_answer(struct mailbox *mp);
static enum okay pop3_finish(struct mailbox *mp);
static void pop3catch(int s);
static void maincatch(int s);
static enum okay pop3_noop1(struct mailbox *mp);
static void pop3alarm(int s);
static enum okay pop3_pass(struct mailbox *mp, const char *pass);
static char *pop3_find_timestamp(const char *bp);
static enum okay pop3_apop(struct mailbox *mp, char *xuser, const char *pass,
		const char *ts);
static enum okay pop3_apop1(struct mailbox *mp,
		const char *user, const char *xp);
static int pop3_use_starttls(const char *uhp);
static int pop3_use_apop(const char *uhp);
static enum okay pop3_user(struct mailbox *mp, char *xuser, const char *pass,
		const char *uhp, const char *xserver);
static enum okay pop3_stat(struct mailbox *mp, off_t *size, int *count);
static enum okay pop3_list(struct mailbox *mp, int n, size_t *size);
static void pop3_init(struct mailbox *mp, int n);
static void pop3_dates(struct mailbox *mp);
static void pop3_setptr(struct mailbox *mp);
static char *pop3_have_password(const char *server);
static enum okay pop3_get(struct mailbox *mp, struct message *m,
		enum needspec need);
static enum okay pop3_exit(struct mailbox *mp);
static enum okay pop3_delete(struct mailbox *mp, int n);
static enum okay pop3_update(struct mailbox *mp);

static void 
pop3_timer_off(void)
{
	if (pop3keepalive > 0) {
		alarm(0);
		safe_signal(SIGALRM, savealrm);
	}
}

static enum okay 
pop3_answer(struct mailbox *mp)
{
	int sz;
	enum okay ok = STOP;

retry:	if ((sz = sgetline(&pop3buf, &pop3bufsize, NULL, &mp->mb_sock)) > 0) {
		if ((mp->mb_active & (MB_COMD|MB_MULT)) == MB_MULT)
			goto multiline;
		if (verbose)
			fputs(pop3buf, stderr);
		switch (*pop3buf) {
		case '+':
			ok = OKAY;
			mp->mb_active &= ~MB_COMD;
			break;
		case '-':
			ok = STOP;
			mp->mb_active = MB_NONE;
			fprintf(stderr, catgets(catd, CATSET, 218,
					"POP3 error: %s"), pop3buf);
			break;
		default:
			/*
			 * If the answer starts neither with '+' nor with
			 * '-', it must be part of a multiline response,
			 * e. g. because the user interrupted a file
			 * download. Get lines until a single dot appears.
			 */
	multiline:	 while (pop3buf[0] != '.' || pop3buf[1] != '\r' ||
					pop3buf[2] != '\n' ||
					pop3buf[3] != '\0') {
				sz = sgetline(&pop3buf, &pop3bufsize,
						NULL, &mp->mb_sock);
				if (sz <= 0)
					goto eof;
			}
			mp->mb_active &= ~MB_MULT;
			if (mp->mb_active != MB_NONE)
				goto retry;
		}
	} else {
	eof: 	ok = STOP;
		mp->mb_active = MB_NONE;
	}
	return ok;
}

static enum okay 
pop3_finish(struct mailbox *mp)
{
	while (mp->mb_sock.s_fd > 0 && mp->mb_active != MB_NONE)
		pop3_answer(mp);
	return OKAY;
}

static void 
pop3catch(int s)
{
	if (reset_tio)
		tcsetattr(0, TCSADRAIN, &otio);
	switch (s) {
	case SIGINT:
		fprintf(stderr, catgets(catd, CATSET, 102, "Interrupt\n"));
		siglongjmp(pop3jmp, 1);
		break;
	case SIGPIPE:
		fprintf(stderr, "Received SIGPIPE during POP3 operation\n");
		break;
	}
}

static void
maincatch(int s)
{
	if (interrupts++ == 0) {
		fprintf(stderr, catgets(catd, CATSET, 102, "Interrupt\n"));
		return;
	}
	onintr(0);
}

static enum okay 
pop3_noop1(struct mailbox *mp)
{
	POP3_OUT("NOOP\r\n", MB_COMD)
	POP3_ANSWER()
	return OKAY;
}

enum okay 
pop3_noop(void)
{
	enum okay	ok = STOP;
	sighandler_type	saveint, savepipe;

	(void)&saveint;
	(void)&savepipe;
	(void)&ok;
	verbose = value("verbose") != NULL;
	pop3lock = 1;
	if ((saveint = safe_signal(SIGINT, SIG_IGN)) != SIG_IGN)
		safe_signal(SIGINT, maincatch);
	savepipe = safe_signal(SIGPIPE, SIG_IGN);
	if (sigsetjmp(pop3jmp, 1) == 0) {
		if (savepipe != SIG_IGN)
			safe_signal(SIGPIPE, pop3catch);
		ok = pop3_noop1(&mb);
	}
	safe_signal(SIGINT, saveint);
	safe_signal(SIGPIPE, savepipe);
	pop3lock = 0;
	return ok;
}

/*ARGSUSED*/
static void 
pop3alarm(int s)
{
	sighandler_type	saveint;
	sighandler_type savepipe;

	if (pop3lock++ == 0) {
		if ((saveint = safe_signal(SIGINT, SIG_IGN)) != SIG_IGN)
			safe_signal(SIGINT, maincatch);
		savepipe = safe_signal(SIGPIPE, SIG_IGN);
		if (sigsetjmp(pop3jmp, 1)) {
			safe_signal(SIGINT, saveint);
			safe_signal(SIGPIPE, savepipe);
			goto brk;
		}
		if (savepipe != SIG_IGN)
			safe_signal(SIGPIPE, pop3catch);
		if (pop3_noop1(&mb) != OKAY) {
			safe_signal(SIGINT, saveint);
			safe_signal(SIGPIPE, savepipe);
			goto out;
		}
		safe_signal(SIGINT, saveint);
		safe_signal(SIGPIPE, savepipe);
	}
brk:	alarm(pop3keepalive);
out:	pop3lock--;
}

static enum okay 
pop3_pass(struct mailbox *mp, const char *pass)
{
	char o[LINESIZE];

	snprintf(o, sizeof o, "PASS %s\r\n", pass);
	POP3_OUT(o, MB_COMD)
	POP3_ANSWER()
	return OKAY;
}

static char *
pop3_find_timestamp(const char *bp)
{
	const char	*cp, *ep;
	char	*rp;
	int	hadat = 0;

	if ((cp = strchr(bp, '<')) == NULL)
		return NULL;
	for (ep = cp; *ep; ep++) {
		if (spacechar(*ep&0377))
			return NULL;
		else if (*ep == '@')
			hadat = 1;
		else if (*ep == '>') {
			if (hadat != 1)
				return NULL;
			break;
		}
	}
	if (*ep != '>')
		return NULL;
	rp = salloc(ep - cp + 2);
	memcpy(rp, cp, ep - cp + 1);
	rp[ep - cp + 1] = '\0';
	return rp;
}

static enum okay 
pop3_apop(struct mailbox *mp, char *xuser, const char *pass, const char *ts)
{
	char	*user, *catp, *xp;
	unsigned char	digest[16];
	MD5_CTX	ctx;

retry:	if (xuser == NULL) {
		if ((user = getuser()) == NULL)
			return STOP;
	} else
		user = xuser;
	if (pass == NULL) {
		if ((pass = getpassword(&otio, &reset_tio, NULL)) == NULL)
			return STOP;
	}
	catp = savecat(ts, pass);
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char *)catp, strlen(catp));
	MD5Final(digest, &ctx);
	xp = md5tohex(digest);
	if (pop3_apop1(mp, user, xp) == STOP) {
		pass = NULL;
		goto retry;
	}
	return OKAY;
}

static enum okay 
pop3_apop1(struct mailbox *mp, const char *user, const char *xp)
{
	char	o[LINESIZE];

	snprintf(o, sizeof o, "APOP %s %s\r\n", user, xp);
	POP3_OUT(o, MB_COMD)
	POP3_ANSWER()
	return OKAY;
}

static int 
pop3_use_starttls(const char *uhp)
{
	char	*var;

	if (value("pop3-use-starttls"))
		return 1;
	var = savecat("pop3-use-starttls-", uhp);
	return value(var) != NULL;
}

static int 
pop3_use_apop(const char *uhp)
{
	char	*var;

	if (value("pop3-use-apop"))
		return 1;
	var = savecat("pop3-use-apop-", uhp);
	return value(var) != NULL;
}

static enum okay 
pop3_user(struct mailbox *mp, char *xuser, const char *pass,
		const char *uhp, const char *xserver)
{
	char o[LINESIZE], *user, *ts = NULL, *server, *cp;

	POP3_ANSWER()
	if (pop3_use_apop(uhp)) {
		if ((ts = pop3_find_timestamp(pop3buf)) == NULL) {
			fprintf(stderr, "Could not determine timestamp from "
				"server greeting. Impossible to use APOP.\n");
			return STOP;
		}
	}
	if ((cp = strchr(xserver, ':')) != NULL) {
		server = salloc(cp - xserver + 1);
		memcpy(server, xserver, cp - xserver);
		server[cp - xserver] = '\0';
	} else
		server = (char *)xserver;
#ifdef	USE_SSL
	if (mp->mb_sock.s_use_ssl == 0 && pop3_use_starttls(uhp)) {
		POP3_OUT("STLS\r\n", MB_COMD)
		POP3_ANSWER()
		if (ssl_open(server, &mp->mb_sock, uhp) != OKAY)
			return STOP;
	}
#else	/* !USE_SSL */
	if (pop3_use_starttls(uhp)) {
		fprintf(stderr, "No SSL support compiled in.\n");
		return STOP;
	}
#endif	/* !USE_SSL */
	if (ts != NULL)
		return pop3_apop(mp, xuser, pass, ts);
retry:	if (xuser == NULL) {
		if ((user = getuser()) == NULL)
			return STOP;
	} else
		user = xuser;
	snprintf(o, sizeof o, "USER %s\r\n", user);
	POP3_OUT(o, MB_COMD)
	POP3_ANSWER()
	if (pass == NULL) {
		if ((pass = getpassword(&otio, &reset_tio, NULL)) == NULL)
			return STOP;
	}
	if (pop3_pass(mp, pass) == STOP) {
		pass = NULL;
		goto retry;
	}
	return OKAY;
}

static enum okay
pop3_stat(struct mailbox *mp, off_t *size, int *count)
{
	char *cp;
	enum okay ok = OKAY;

	POP3_OUT("STAT\r\n", MB_COMD);
	POP3_ANSWER()
	for (cp = pop3buf; *cp && !spacechar(*cp & 0377); cp++);
	while (*cp && spacechar(*cp & 0377))
		cp++;
	if (*cp) {
		*count = (int)strtol(cp, NULL, 10);
		while (*cp && !spacechar(*cp & 0377))
			cp++;
		while (*cp && spacechar(*cp & 0377))
			cp++;
		if (*cp)
			*size = (int)strtol(cp, NULL, 10);
		else
			ok = STOP;
	} else
		ok = STOP;
	if (ok == STOP)
		fprintf(stderr, catgets(catd, CATSET, 260,
			"invalid POP3 STAT response: %s\n"), pop3buf);
	return ok;
}

static enum okay
pop3_list(struct mailbox *mp, int n, size_t *size)
{
	char o[LINESIZE], *cp;

	snprintf(o, sizeof o, "LIST %u\r\n", n);
	POP3_OUT(o, MB_COMD)
	POP3_ANSWER()
	for (cp = pop3buf; *cp && !spacechar(*cp & 0377); cp++);
	while (*cp && spacechar(*cp & 0377))
		cp++;
	while (*cp && !spacechar(*cp & 0377))
		cp++;
	while (*cp && spacechar(*cp & 0377))
		cp++;
	if (*cp)
		*size = (size_t)strtol(cp, NULL, 10);
	else
		*size = 0;
	return OKAY;
}

static void 
pop3_init(struct mailbox *mp, int n)
{
	struct message *m = &message[n];
	char *cp;

	m->m_flag = MUSED|MNEW|MNOFROM|MNEWEST;
	m->m_block = 0;
	m->m_offset = 0;
	pop3_list(mp, m - message + 1, &m->m_xsize);
	if ((cp = hfield("status", m)) != NULL) {
		while (*cp != '\0') {
			if (*cp == 'R')
				m->m_flag |= MREAD;
			else if (*cp == 'O')
				m->m_flag &= ~MNEW;
			cp++;
		}
	}
}

/*ARGSUSED*/
static void 
pop3_dates(struct mailbox *mp)
{
	int	i;

	for (i = 0; i < msgCount; i++)
		substdate(&message[i]);
}

static void 
pop3_setptr(struct mailbox *mp)
{
	int i;

	message = scalloc(msgCount + 1, sizeof *message);
	for (i = 0; i < msgCount; i++)
		pop3_init(mp, i);
	setdot(message);
	message[msgCount].m_size = 0;
	message[msgCount].m_lines = 0;
	pop3_dates(mp);
}

static char *
pop3_have_password(const char *server)
{
	char *var, *cp;

	var = ac_alloc(strlen(server) + 10);
	strcpy(var, "password-");
	strcpy(&var[9], server);
	if ((cp = value(var)) != NULL)
		cp = savestr(cp);
	ac_free(var);
	return cp;
}

int 
pop3_setfile(const char *server, int newmail, int isedit)
{
	struct sock	so;
	sighandler_type	saveint;
	sighandler_type savepipe;
	char *user;
	const char *cp, *sp = server, *pass, *uhp;
	int use_ssl = 0;

	(void)&sp;
	(void)&use_ssl;
	(void)&user;
	if (newmail)
		return 1;
	if (strncmp(sp, "pop3://", 7) == 0) {
		sp = &sp[7];
		use_ssl = 0;
#ifdef	USE_SSL
	} else if (strncmp(sp, "pop3s://", 8) == 0) {
		sp = &sp[8];
		use_ssl = 1;
#endif	/* USE_SSL */
	}
	uhp = sp;
	pass = pop3_have_password(uhp);
	if ((cp = last_at_before_slash(sp)) != NULL) {
		user = salloc(cp - sp + 1);
		memcpy(user, sp, cp - sp);
		user[cp - sp] = '\0';
		sp = &cp[1];
		user = strdec(user);
	} else
		user = NULL;
	verbose = value("verbose") != NULL;
	if (sopen(sp, &so, use_ssl, uhp, use_ssl ? "pop3s" : "pop3",
				verbose) != OKAY) {
		return -1;
	}
	quit();
	edit = isedit;
	if (mb.mb_sock.s_fd >= 0)
		sclose(&mb.mb_sock);
	if (mb.mb_itf) {
		fclose(mb.mb_itf);
		mb.mb_itf = NULL;
	}
	if (mb.mb_otf) {
		fclose(mb.mb_otf);
		mb.mb_otf = NULL;
	}
	initbox(server);
	mb.mb_type = MB_VOID;
	pop3lock = 1;
	mb.mb_sock = so;
	saveint = safe_signal(SIGINT, SIG_IGN);
	savepipe = safe_signal(SIGPIPE, SIG_IGN);
	if (sigsetjmp(pop3jmp, 1)) {
		sclose(&mb.mb_sock);
		safe_signal(SIGINT, saveint);
		safe_signal(SIGPIPE, savepipe);
		pop3lock = 0;
		return 1;
	}
	if (saveint != SIG_IGN)
		safe_signal(SIGINT, pop3catch);
	if (savepipe != SIG_IGN)
		safe_signal(SIGPIPE, pop3catch);
	if ((cp = value("pop3-keepalive")) != NULL) {
		if ((pop3keepalive = strtol(cp, NULL, 10)) > 0) {
			savealrm = safe_signal(SIGALRM, pop3alarm);
			alarm(pop3keepalive);
		}
	}
	mb.mb_sock.s_desc = "POP3";
	mb.mb_sock.s_onclose = pop3_timer_off;
	if (pop3_user(&mb, user, pass, uhp, sp) != OKAY ||
			pop3_stat(&mb, &mailsize, &msgCount) != OKAY) {
		sclose(&mb.mb_sock);
		pop3_timer_off();
		safe_signal(SIGINT, saveint);
		safe_signal(SIGPIPE, savepipe);
		pop3lock = 0;
		return 1;
	}
	mb.mb_type = MB_POP3;
	mb.mb_perm = Rflag ? 0 : MB_DELE;
	pop3_setptr(&mb);
	setmsize(msgCount);
	sawcom = 0;
	safe_signal(SIGINT, saveint);
	safe_signal(SIGPIPE, savepipe);
	pop3lock = 0;
	if (!edit && msgCount == 0) {
		if (mb.mb_type == MB_POP3 && value("emptystart") == NULL)
			fprintf(stderr, catgets(catd, CATSET, 258,
				"No mail at %s\n"), server);
		return 1;
	}
	return 0;
}

static enum okay 
pop3_get(struct mailbox *mp, struct message *m, enum needspec need)
{
	sighandler_type	saveint = SIG_IGN;
	sighandler_type savepipe = SIG_IGN;
	off_t offset;
	char o[LINESIZE], *line = NULL, *lp;
	size_t linesize = 0, linelen, size;
	int number = m - message + 1;
	int emptyline = 0, lines;

	(void)&saveint;
	(void)&savepipe;
	(void)&number;
	(void)&emptyline;
	(void)&need;
	verbose = value("verbose") != NULL;
	if (mp->mb_sock.s_fd < 0) {
		fprintf(stderr, catgets(catd, CATSET, 219,
				"POP3 connection already closed.\n"));
		return STOP;
	}
	if (pop3lock++ == 0) {
		if ((saveint = safe_signal(SIGINT, SIG_IGN)) != SIG_IGN)
			safe_signal(SIGINT, maincatch);
		savepipe = safe_signal(SIGPIPE, SIG_IGN);
		if (sigsetjmp(pop3jmp, 1)) {
			safe_signal(SIGINT, saveint);
			safe_signal(SIGPIPE, savepipe);
			pop3lock--;
			return STOP;
		}
		if (savepipe != SIG_IGN)
			safe_signal(SIGPIPE, pop3catch);
	}
	fseek(mp->mb_otf, 0L, SEEK_END);
	offset = ftell(mp->mb_otf);
retry:	switch (need) {
	case NEED_HEADER:
		snprintf(o, sizeof o, "TOP %u 0\r\n", number);
		break;
	case NEED_BODY:
		snprintf(o, sizeof o, "RETR %u\r\n", number);
		break;
	case NEED_UNSPEC:
		abort();
	}
	POP3_OUT(o, MB_COMD|MB_MULT)
	if (pop3_answer(mp) == STOP) {
		if (need == NEED_HEADER) {
			/*
			 * The TOP POP3 command is optional, so retry
			 * with the entire message.
			 */
			need = NEED_BODY;
			goto retry;
		}
		if (interrupts)
			onintr(0);
		return STOP;
	}
	size = 0;
	lines = 0;
	while (sgetline(&line, &linesize, &linelen, &mp->mb_sock) > 0) {
		if (line[0] == '.' && line[1] == '\r' && line[2] == '\n' &&
				line[3] == '\0') {
			mp->mb_active &= ~MB_MULT;
			break;
		}
		if (line[0] == '.') {
			lp = &line[1];
			linelen--;
		} else
			lp = line;
		/*
		 * Need to mask 'From ' lines. This cannot be done properly
		 * since some servers pass them as 'From ' and others as
		 * '>From '. Although one could identify the first kind of
		 * server in principle, it is not possible to identify the
		 * second as '>From ' may also come from a server of the
		 * first type as actual data. So do what is absolutely
		 * necessary only - mask 'From '.
		 *
		 * If the line is the first line of the message header, it
		 * is likely a real 'From ' line. In this case, it is just
		 * ignored since it violates all standards.
		 */
		if (lp[0] == 'F' && lp[1] == 'r' && lp[2] == 'o' &&
				lp[3] == 'm' && lp[4] == ' ') {
			if (lines != 0) {
				fputc('>', mp->mb_otf);
				size++;
			} else
				continue;
		}
		lines++;
		if (lp[linelen-1] == '\n' && (linelen == 1 ||
					lp[linelen-2] == '\r')) {
			emptyline = linelen <= 2;
			if (linelen > 2)
				fwrite(lp, 1, linelen - 2, mp->mb_otf);
			fputc('\n', mp->mb_otf);
			size += linelen - 1;
		} else {
			emptyline = 0;
			fwrite(lp, 1, linelen, mp->mb_otf);
			size += linelen;
		}
	}
	if (!emptyline) {
		/*
		 * This is very ugly; but some POP3 daemons don't end a
		 * message with \r\n\r\n, and we need \n\n for mbox format.
		 */
		fputc('\n', mp->mb_otf);
		lines++;
		size++;
	}
	m->m_size = size;
	m->m_lines = lines;
	m->m_block = mailx_blockof(offset);
	m->m_offset = mailx_offsetof(offset);
	fflush(mp->mb_otf);
	switch (need) {
	case NEED_HEADER:
		m->m_have |= HAVE_HEADER;
		break;
	case NEED_BODY:
		m->m_have |= HAVE_HEADER|HAVE_BODY;
		m->m_xlines = m->m_lines;
		m->m_xsize = m->m_size;
		break;
	case NEED_UNSPEC:
		break;
	}
	if (line)
		free(line);
	if (saveint != SIG_IGN)
		safe_signal(SIGINT, saveint);
	if (savepipe != SIG_IGN)
		safe_signal(SIGPIPE, savepipe);
	pop3lock--;
	if (interrupts)
		onintr(0);
	return OKAY;
}

enum okay 
pop3_header(struct message *m)
{
	return pop3_get(&mb, m, NEED_HEADER);
}


enum okay 
pop3_body(struct message *m)
{
	return pop3_get(&mb, m, NEED_BODY);
}

static enum okay 
pop3_exit(struct mailbox *mp)
{
	POP3_OUT("QUIT\r\n", MB_COMD)
	POP3_ANSWER()
	return OKAY;
}

static enum okay 
pop3_delete(struct mailbox *mp, int n)
{
	char o[LINESIZE];

	snprintf(o, sizeof o, "DELE %u\r\n", n);
	POP3_OUT(o, MB_COMD)
	POP3_ANSWER()
	return OKAY;
}

static enum okay 
pop3_update(struct mailbox *mp)
{
	FILE *readstat = NULL;
	struct message *m;
	int dodel, c, gotcha, held;

	if (Tflag != NULL) {
		if ((readstat = Zopen(Tflag, "w", NULL)) == NULL)
			Tflag = NULL;
	}
	if (!edit) {
		holdbits();
		for (m = &message[0], c = 0; m < &message[msgCount]; m++) {
			if (m->m_flag & MBOX)
				c++;
		}
		if (c > 0)
			makembox();
	}
	for (m = &message[0], gotcha=0, held=0; m < &message[msgCount]; m++) {
		if (readstat != NULL && (m->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("message-id", m)) != NULL ||
					(id = hfield("article-id", m)) != NULL)
				fprintf(readstat, "%s\n", id);
		}
		if (edit) {
			dodel = m->m_flag & MDELETED;
		} else {
			dodel = !((m->m_flag&MPRESERVE) ||
					(m->m_flag&MTOUCH) == 0);
		}
		if (dodel) {
			pop3_delete(mp, m - message + 1);
			gotcha++;
		} else
			held++;
	}
	if (readstat != NULL)
		Fclose(readstat);
	if (gotcha && edit) {
		printf(catgets(catd, CATSET, 168, "\"%s\" "), mailname);
		printf(value("bsdcompat") || value("bsdmsgs") ?
				catgets(catd, CATSET, 170, "complete\n") :
				catgets(catd, CATSET, 212, "updated.\n"));
	} else if (held && !edit) {
		if (held == 1)
			printf(catgets(catd, CATSET, 155,
				"Held 1 message in %s\n"), mailname);
		else if (held > 1)
			printf(catgets(catd, CATSET, 156,
				"Held %d messages in %s\n"), held, mailname);
	}
	fflush(stdout);
	return OKAY;
}

void 
pop3_quit(void)
{
	sighandler_type	saveint;
	sighandler_type savepipe;

	verbose = value("verbose") != NULL;
	if (mb.mb_sock.s_fd < 0) {
		fprintf(stderr, catgets(catd, CATSET, 219,
				"POP3 connection already closed.\n"));
		return;
	}
	pop3lock = 1;
	saveint = safe_signal(SIGINT, SIG_IGN);
	savepipe = safe_signal(SIGPIPE, SIG_IGN);
	if (sigsetjmp(pop3jmp, 1)) {
		safe_signal(SIGINT, saveint);
		safe_signal(SIGPIPE, saveint);
		pop3lock = 0;
		return;
	}
	if (saveint != SIG_IGN)
		safe_signal(SIGINT, pop3catch);
	if (savepipe != SIG_IGN)
		safe_signal(SIGPIPE, pop3catch);
	pop3_update(&mb);
	pop3_exit(&mb);
	sclose(&mb.mb_sock);
	safe_signal(SIGINT, saveint);
	safe_signal(SIGPIPE, savepipe);
	pop3lock = 0;
}
#else	/* !HAVE_SOCKETS */
static void 
nopop3(void)
{
	fprintf(stderr, catgets(catd, CATSET, 216,
				"No POP3 support compiled in.\n"));
}

int 
pop3_setfile(const char *server, int newmail, int isedit)
{
	nopop3();
	return -1;
}

enum okay 
pop3_header(struct message *mp)
{
	nopop3();
	return STOP;
}

enum okay 
pop3_body(struct message *mp)
{
	nopop3();
	return STOP;
}

void 
pop3_quit(void)
{
	nopop3();
}

enum okay 
pop3_noop(void)
{
	nopop3();
	return STOP;
}
#endif	/* HAVE_SOCKETS */

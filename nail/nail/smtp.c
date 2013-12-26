/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2000
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
static char sccsid[] = "@(#)smtp.c	2.43 (gritter) 8/4/07";
#endif
#endif /* not lint */

#include "rcv.h"

#include <sys/utsname.h>
#ifdef	HAVE_SOCKETS
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef	HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif	/* HAVE_ARPA_INET_H */
#endif	/* HAVE_SOCKETS */
#include <unistd.h>
#include <setjmp.h>

#include "extern.h"
#include "md5.h"

/*
 * Mail -- a mail program
 *
 * SMTP client and other internet related functions.
 */

#ifdef	HAVE_SOCKETS
static int verbose;
static int _debug;
#endif

/*
 * Return our hostname.
 */
char *
nodename(int mayoverride)
{
	static char *hostname;
	char *hn;
        struct utsname ut;
#ifdef	HAVE_SOCKETS
#ifdef	HAVE_IPv6_FUNCS
	struct addrinfo hints, *res;
#else	/* !HAVE_IPv6_FUNCS */
        struct hostent *hent;
#endif	/* !HAVE_IPv6_FUNCS */
#endif	/* HAVE_SOCKETS */

	if (mayoverride && (hn = value("hostname")) != NULL && *hn) {
		free(hostname);
		hostname = sstrdup(hn);
	}
	if (hostname == NULL) {
		uname(&ut);
		hn = ut.nodename;
#ifdef	HAVE_SOCKETS
#ifdef	HAVE_IPv6_FUNCS
		memset(&hints, 0, sizeof hints);
		hints.ai_socktype = SOCK_DGRAM;	/* dummy */
		hints.ai_flags = AI_CANONNAME;
		if (getaddrinfo(hn, "0", &hints, &res) == 0) {
			if (res->ai_canonname) {
				hn = salloc(strlen(res->ai_canonname) + 1);
				strcpy(hn, res->ai_canonname);
			}
			freeaddrinfo(res);
		}
#else	/* !HAVE_IPv6_FUNCS */
		hent = gethostbyname(hn);
		if (hent != NULL) {
			hn = hent->h_name;
		}
#endif	/* !HAVE_IPv6_FUNCS */
#endif	/* HAVE_SOCKETS */
		hostname = smalloc(strlen(hn) + 1);
		strcpy(hostname, hn);
	}
	return hostname;
}

/*
 * Return the user's From: address(es).
 */
char *
myaddrs(struct header *hp)
{
	char *cp, *hn;
	static char *addr;
	size_t sz;

	if (hp != NULL && hp->h_from != NULL) {
		if (hp->h_from->n_fullname)
			return savestr(hp->h_from->n_fullname);
		if (hp->h_from->n_name)
			return savestr(hp->h_from->n_name);
	}
	if ((cp = value("from")) != NULL)
		return cp;
	/*
	 * When invoking sendmail directly, it's its task
	 * to generate a From: address.
	 */
	if (value("smtp") == NULL)
		return NULL;
	if (addr == NULL) {
		hn = nodename(1);
		sz = strlen(myname) + strlen(hn) + 2;
		addr = smalloc(sz);
		snprintf(addr, sz, "%s@%s", myname, hn);
	}
	return addr;
}

char *
myorigin(struct header *hp)
{
	char	*cp;
	struct name	*np;

	if ((cp = myaddrs(hp)) == NULL ||
			(np = sextract(cp, GEXTRA|GFULL)) == NULL)
		return NULL;
	return np->n_flink != NULL ? value("sender") : cp;
}

#ifdef	HAVE_SOCKETS

static int read_smtp(struct sock *sp, int value, int ign_eof);
static int talk_smtp(struct name *to, FILE *fi, struct sock *sp,
		char *server, char *uhp, struct header *hp,
		const char *user, const char *password, const char *skinned);

char *
smtp_auth_var(const char *type, const char *addr)
{
	char	*var, *cp;
	int	len;

	var = ac_alloc(len = strlen(type) + strlen(addr) + 7);
	snprintf(var, len, "smtp-auth%s-%s", type, addr);
	if ((cp = value(var)) != NULL)
		cp = savestr(cp);
	else {
		snprintf(var, len, "smtp-auth%s", type);
		if ((cp = value(var)) != NULL)
			cp = savestr(cp);
	}
	ac_free(var);
	return cp;
}

static char	*smtpbuf;
static size_t	smtpbufsize;

/*
 * Get the SMTP server's answer, expecting value.
 */
static int 
read_smtp(struct sock *sp, int value, int ign_eof)
{
	int ret;
	int len;

	do {
		if ((len = sgetline(&smtpbuf, &smtpbufsize, NULL, sp)) < 6) {
			if (len >= 0 && !ign_eof)
				fprintf(stderr, catgets(catd, CATSET, 241,
					"Unexpected EOF on SMTP connection\n"));
			return -1;
		}
		if (verbose || debug || _debug)
			fputs(smtpbuf, stderr);
		switch (*smtpbuf) {
		case '1': ret = 1; break;
		case '2': ret = 2; break;
		case '3': ret = 3; break;
		case '4': ret = 4; break;
		default: ret = 5;
		}
		if (value != ret)
			fprintf(stderr, catgets(catd, CATSET, 191,
					"smtp-server: %s"), smtpbuf);
	} while (smtpbuf[3] == '-');
	return ret;
}

/*
 * Macros for talk_smtp.
 */
#define	_SMTP_ANSWER(x, ign_eof)	\
			if (!debug && !_debug) { \
				int	y; \
				if ((y = read_smtp(sp, x, ign_eof)) != (x) && \
					(!(ign_eof) || y != -1)) { \
					if (b != NULL) \
						free(b); \
					return 1; \
				} \
			}

#define	SMTP_ANSWER(x)	_SMTP_ANSWER(x, 0)

#define	SMTP_OUT(x)	if (verbose || debug || _debug) \
				fprintf(stderr, ">>> %s", x); \
			if (!debug && !_debug) \
				swrite(sp, x);

/*
 * Talk to a SMTP server.
 */
static int
talk_smtp(struct name *to, FILE *fi, struct sock *sp,
		char *xserver, char *uhp, struct header *hp,
		const char *user, const char *password, const char *skinned)
{
	struct name *n;
	char *b = NULL, o[LINESIZE];
	size_t blen, bsize = 0, count;
	char	*b64, *authstr, *cp;
	enum	{ AUTH_NONE, AUTH_PLAIN, AUTH_LOGIN, AUTH_CRAM_MD5 } auth;
	int	inhdr = 1, inbcc = 0;

	if ((authstr = smtp_auth_var("", skinned)) == NULL)
		auth = user && password ? AUTH_LOGIN : AUTH_NONE;
	else if (strcmp(authstr, "plain") == 0)
		auth = AUTH_PLAIN;
	else if (strcmp(authstr, "login") == 0)
		auth = AUTH_LOGIN;
	else if (strcmp(authstr, "cram-md5") == 0)
		auth = AUTH_CRAM_MD5;
	else {
		fprintf(stderr, "Unknown SMTP authentication "
				"method: \"%s\"\n", authstr);
		return 1;
	}
	if (auth != AUTH_NONE && (user == NULL || password == NULL)) {
		fprintf(stderr, "User and password are necessary "
				"for SMTP authentication.\n");
		return 1;
	}
	SMTP_ANSWER(2);
#ifdef	USE_SSL
	if (value("smtp-use-starttls") ||
			value("smtp-use-tls") /* v11.0 compatibility */) {
		char	*server;
		if ((cp = strchr(xserver, ':')) != NULL) {
			server = salloc(cp - xserver + 1);
			memcpy(server, xserver, cp - xserver);
			server[cp - xserver] = '\0';
		} else
			server = xserver;
		snprintf(o, sizeof o, "EHLO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2);
		SMTP_OUT("STARTTLS\r\n");
		SMTP_ANSWER(2);
		if (!debug && !_debug && ssl_open(server, sp, uhp) != OKAY)
			return 1;
	}
#else	/* !USE_SSL */
	if (value("smtp-use-starttls") || value("smtp-use-tls")) {
		fprintf(stderr, "No SSL support compiled in.\n");
		return 1;
	}
#endif	/* !USE_SSL */
	if (auth != AUTH_NONE) {
		snprintf(o, sizeof o, "EHLO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2);
		switch (auth) {
		default:
		case AUTH_LOGIN:
			SMTP_OUT("AUTH LOGIN\r\n");
			SMTP_ANSWER(3);
			b64 = strtob64(user);
			snprintf(o, sizeof o, "%s\r\n", b64);
			free(b64);
			SMTP_OUT(o);
			SMTP_ANSWER(3);
			b64 = strtob64(password);
			snprintf(o, sizeof o, "%s\r\n", b64);
			free(b64);
			SMTP_OUT(o);
			SMTP_ANSWER(2);
			break;
		case AUTH_PLAIN:
			SMTP_OUT("AUTH PLAIN\r\n");
			SMTP_ANSWER(3);
			snprintf(o, sizeof o, "%c%s%c%s", '\0', user, '\0',
							  password);
			b64 = memtob64(o, strlen(user)+strlen(password)+2);
			snprintf(o, sizeof o, "%s\r\n", b64);
			SMTP_OUT(o);
			SMTP_ANSWER(2);
			break;
		case AUTH_CRAM_MD5:
			SMTP_OUT("AUTH CRAM-MD5\r\n");
			SMTP_ANSWER(3);
			for (cp = smtpbuf; digitchar(*cp&0377); cp++);
			while (blankchar(*cp&0377)) cp++;
			cp = cram_md5_string(user, password, cp);
			SMTP_OUT(cp);
			SMTP_ANSWER(2);
			break;
		}
	} else {
		snprintf(o, sizeof o, "HELO %s\r\n", nodename(1));
		SMTP_OUT(o);
		SMTP_ANSWER(2);
	}
	snprintf(o, sizeof o, "MAIL FROM:<%s>\r\n", skinned);
	SMTP_OUT(o);
	SMTP_ANSWER(2);
	for (n = to; n != NULL; n = n->n_flink) {
		if ((n->n_type & GDEL) == 0) {
			snprintf(o, sizeof o, "RCPT TO:<%s>\r\n",
					skin(n->n_name));
			SMTP_OUT(o);
			SMTP_ANSWER(2);
		}
	}
	SMTP_OUT("DATA\r\n");
	SMTP_ANSWER(3);
	fflush(fi);
	rewind(fi);
	count = fsize(fi);
	while (fgetline(&b, &bsize, &count, &blen, fi, 1) != NULL) {
		if (inhdr) {
			if (*b == '\n') {
				inhdr = 0;
				inbcc = 0;
			} else if (inbcc && blankchar(*b & 0377))
				continue;
			/*
			 * We know what we have generated first, so
			 * do not look for whitespace before the ':'.
			 */
			else if (ascncasecmp(b, "bcc: ", 5) == 0) {
				inbcc = 1;
				continue;
			} else
				inbcc = 0;
		}
		if (*b == '.') {
			if (debug || _debug)
				putc('.', stderr);
			else
				swrite1(sp, ".", 1, 1);
		}
		if (debug || _debug) {
			fprintf(stderr, ">>> %s", b);
			continue;
		}
		b[blen-1] = '\r';
		b[blen] = '\n';
		swrite1(sp, b, blen+1, 1);
	}
	SMTP_OUT(".\r\n");
	SMTP_ANSWER(2);
	SMTP_OUT("QUIT\r\n");
	_SMTP_ANSWER(2, 1);
	if (b != NULL)
		free(b);
	return 0;
}

static sigjmp_buf	smtpjmp;

static void
onterm(int signo)
{
	siglongjmp(smtpjmp, 1);
}

/*
 * Connect to a SMTP server.
 */
int
smtp_mta(char *server, struct name *to, FILE *fi, struct header *hp,
		const char *user, const char *password, const char *skinned)
{
	struct sock	so;
	int	use_ssl, ret;
	sighandler_type	saveterm;

	memset(&so, 0, sizeof so);
	verbose = value("verbose") != NULL;
	_debug = value("debug") != NULL;
	saveterm = safe_signal(SIGTERM, SIG_IGN);
	if (sigsetjmp(smtpjmp, 1)) {
		safe_signal(SIGTERM, saveterm);
		return 1;
	}
	if (saveterm != SIG_IGN)
		safe_signal(SIGTERM, onterm);
	if (strncmp(server, "smtp://", 7) == 0) {
		use_ssl = 0;
		server += 7;
#ifdef	USE_SSL
	} else if (strncmp(server, "smtps://", 8) == 0) {
		use_ssl = 1;
		server += 8;
#endif
	} else
		use_ssl = 0;
	if (!debug && !_debug && sopen(server, &so, use_ssl, server,
				use_ssl ? "smtps" : "smtp", verbose) != OKAY) {
		safe_signal(SIGTERM, saveterm);
		return 1;
	}
	so.s_desc = "SMTP";
	ret = talk_smtp(to, fi, &so, server, server, hp,
			user, password, skinned);
	if (!debug && !_debug)
		sclose(&so);
	if (smtpbuf) {
		free(smtpbuf);
		smtpbuf = NULL;
		smtpbufsize = 0;
	}
	safe_signal(SIGTERM, saveterm);
	return ret;
}
#else	/* !HAVE_SOCKETS */
int
smtp_mta(char *server, struct name *to, FILE *fi, struct header *hp,
		const char *user, const char *password, const char *skinned)
{
	fputs(catgets(catd, CATSET, 194,
			"No SMTP support compiled in.\n"), stderr);
	return 1;
}
#endif	/* !HAVE_SOCKETS */

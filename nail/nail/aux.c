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
static char sccsid[] = "@(#)aux.c	2.83 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <sys/stat.h>
#include <utime.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>
#ifdef	HAVE_WCTYPE_H
#include <wctype.h>
#endif	/* HAVE_WCTYPE_H */
#ifdef	HAVE_WCWIDTH
#include <wchar.h>
#endif	/* HAVE_WCWIDTH */
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>

#include "md5.h"

/*
 * Mail -- a mail program
 *
 * Auxiliary functions.
 */

/*
 * Return a pointer to a dynamic copy of the argument.
 */
char *
savestr(const char *str)
{
	char *new;
	int size = strlen(str) + 1;

	if ((new = salloc(size)) != NULL)
		memcpy(new, str, size);
	return new;
}

/*
 * Make a copy of new argument incorporating old one.
 */
char *
save2str(const char *str, const char *old)
{
	char *new;
	int newsize = strlen(str) + 1;
	int oldsize = old ? strlen(old) + 1 : 0;

	if ((new = salloc(newsize + oldsize)) != NULL) {
		if (oldsize) {
			memcpy(new, old, oldsize);
			new[oldsize - 1] = ' ';
		}
		memcpy(new + oldsize, str, newsize);
	}
	return new;
}

char *
savecat(const char *s1, const char *s2)
{
	const char	*cp;
	char	*ns, *np;

	np = ns = salloc(strlen(s1) + strlen(s2) + 1);
	for (cp = s1; *cp; cp++)
		*np++ = *cp;
	for (cp = s2; *cp; cp++)
		*np++ = *cp;
	*np = '\0';
	return ns;
}

#include <stdarg.h>

#ifndef	HAVE_SNPRINTF
/*
 * Lazy vsprintf wrapper.
 */
int
snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vsprintf(str, format, ap);
	va_end(ap);
	return ret;
}
#endif	/* !HAVE_SNPRINTF */

/*
 * Announce a fatal error and die.
 */
void
panic(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	fprintf(stderr, catgets(catd, CATSET, 1, "panic: "));
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, catgets(catd, CATSET, 2, "\n"));
	fflush(stderr);
	abort();
}

void
holdint(void)
{
	sigset_t	set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

void
relseint(void)
{
	sigset_t	set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/*
 * Touch the named message by setting its MTOUCH flag.
 * Touched messages have the effect of not being sent
 * back to the system mailbox on exit.
 */
void 
touch(struct message *mp)
{

	mp->m_flag |= MTOUCH;
	if ((mp->m_flag & MREAD) == 0)
		mp->m_flag |= MREAD|MSTATUS;
}

/*
 * Test to see if the passed file name is a directory.
 * Return true if it is.
 */
int 
is_dir(char *name)
{
	struct stat sbuf;

	if (stat(name, &sbuf) < 0)
		return(0);
	return(S_ISDIR(sbuf.st_mode));
}

/*
 * Count the number of arguments in the given string raw list.
 */
int 
argcount(char **argv)
{
	char **ap;

	for (ap = argv; *ap++ != NULL;)
		;	
	return ap - argv - 1;
}

/*
 * Copy a string, lowercasing it as we go.
 */
void 
i_strcpy(char *dest, const char *src, int size)
{
	char *max;

	max=dest+size-1;
	while (dest<=max) {
		*dest++ = lowerconv(*src & 0377);
		if (*src++ == '\0')
			break;
	}
}

char *
i_strdup(const char *src)
{
	int	sz;
	char	*dest;

	sz = strlen(src) + 1;
	dest = salloc(sz);
	i_strcpy(dest, src, sz);
	return dest;
}

/*
 * Convert a string to lowercase, in-place and with multibyte-aware.
 */
void 
makelow(char *cp)
{
#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
	if (mb_cur_max > 1) {
		char	*tp = cp;
		wchar_t	wc;
		int	len;

		while (*cp) {
			len = mbtowc(&wc, cp, mb_cur_max);
			if (len < 0)
				*tp++ = *cp++;
			else {
				wc = towlower(wc);
				if (wctomb(tp, wc) == len)
					tp += len, cp += len;
				else
					*tp++ = *cp++;
			}
		}
	} else
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
	{
		do
			*cp = tolower(*cp & 0377);
		while (*cp++);
	}
}

int 
substr(const char *str, const char *sub)
{
	const char	*cp, *backup;

	cp = sub;
	backup = str;
	while (*str && *cp) {
#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
		if (mb_cur_max > 1) {
			wchar_t c, c2;
			int sz;

			if ((sz = mbtowc(&c, cp, mb_cur_max)) < 0)
				goto singlebyte;
			cp += sz;
			if ((sz = mbtowc(&c2, str, mb_cur_max)) < 0)
				goto singlebyte;
			str += sz;
			c = towupper(c);
			c2 = towupper(c2);
			if (c != c2) {
				if ((sz = mbtowc(&c, backup, mb_cur_max)) > 0) {
					backup += sz;
					str = backup;
				} else
					str = ++backup;
				cp = sub;
			}
		} else
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
		{
			int c, c2;

#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
	singlebyte:
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
			c = *cp++ & 0377;
			if (islower(c))
				c = toupper(c);
			c2 = *str++ & 0377;
			if (islower(c2))
				c2 = toupper(c2);
			if (c != c2) {
				str = ++backup;
				cp = sub;
			}
		}
	}
	return *cp == '\0';
}

char *
colalign(const char *cp, int col, int fill)
{
	int	n, sz;
	char	*nb, *np;

	np = nb = salloc(mb_cur_max * strlen(cp) + col + 1);
	while (*cp) {
#if defined (HAVE_MBTOWC) && defined (HAVE_WCWIDTH)
		if (mb_cur_max > 1) {
			wchar_t	wc;

			if ((sz = mbtowc(&wc, cp, mb_cur_max)) < 0) {
				n = sz = 1;
			} else {
				if ((n = wcwidth(wc)) < 0)
					n = 1;
			}
		} else
#endif	/* HAVE_MBTOWC && HAVE_WCWIDTH */
		{
			n = sz = 1;
		}
		if (n > col)
			break;
		col -= n;
		if (sz == 1 && spacechar(*cp&0377)) {
			*np++ = ' ';
			cp++;
		} else
			while (sz--)
				*np++ = *cp++;
	}
	if (fill)
		while (col-- > 0)
			*np++ = ' ';
	*np = '\0';
	return nb;
}

void
try_pager(FILE *fp)
{
	long	lines = 0;
	int	c;
	char	*cp;

	fflush(fp);
	rewind(fp);
	while ((c = getc(fp)) != EOF)
		if (c == '\n')
			lines++;
	rewind(fp);
	if (is_a_tty[0] && is_a_tty[1] && (cp = value("crt")) != NULL &&
			lines > (*cp ? atol(cp) : scrnheight))
		run_command(get_pager(), 0, fileno(fp), -1, NULL, NULL, NULL);
	else
		while ((c = getc(fp)) != EOF)
			putchar(c);
}

/*
 * The following code deals with input stacking to do source
 * commands.  All but the current file pointer are saved on
 * the stack.
 */

static	int	ssp;			/* Top of file stack */
struct sstack {
	FILE	*s_file;		/* File we were in. */
	enum condition	s_cond;		/* Saved state of conditionals */
	int	s_loading;		/* Loading .mailrc, etc. */
#define	SSTACK	20
} sstack[SSTACK];

/*
 * Pushdown current input file and switch to a new one.
 * Set the global flag "sourcing" so that others will realize
 * that they are no longer reading from a tty (in all probability).
 */
int 
source(void *v)
{
	char **arglist = v;
	FILE *fi;
	char *cp;

	if ((cp = expand(*arglist)) == NULL)
		return(1);
	if ((fi = Fopen(cp, "r")) == NULL) {
		perror(cp);
		return(1);
	}
	if (ssp >= SSTACK - 1) {
		printf(catgets(catd, CATSET, 3,
					"Too much \"sourcing\" going on.\n"));
		Fclose(fi);
		return(1);
	}
	sstack[ssp].s_file = input;
	sstack[ssp].s_cond = cond;
	sstack[ssp].s_loading = loading;
	ssp++;
	loading = 0;
	cond = CANY;
	input = fi;
	sourcing++;
	return(0);
}

/*
 * Pop the current input back to the previous level.
 * Update the "sourcing" flag as appropriate.
 */
int 
unstack(void)
{
	if (ssp <= 0) {
		printf(catgets(catd, CATSET, 4,
					"\"Source\" stack over-pop.\n"));
		sourcing = 0;
		return(1);
	}
	Fclose(input);
	if (cond != CANY)
		printf(catgets(catd, CATSET, 5, "Unmatched \"if\"\n"));
	ssp--;
	cond = sstack[ssp].s_cond;
	loading = sstack[ssp].s_loading;
	input = sstack[ssp].s_file;
	if (ssp == 0)
		sourcing = loading;
	return(0);
}

/*
 * Touch the indicated file.
 * This is nifty for the shell.
 */
void 
alter(char *name)
{
	struct stat sb;
	struct utimbuf utb;

	if (stat(name, &sb))
		return;
	utb.actime = time((time_t *)0) + 1;
	utb.modtime = sb.st_mtime;
	utime(name, &utb);
}

/*
 * Examine the passed line buffer and
 * return true if it is all blanks and tabs.
 */
int 
blankline(char *linebuf)
{
	char *cp;

	for (cp = linebuf; *cp; cp++)
		if (!blankchar(*cp & 0377))
			return(0);
	return(1);
}

/*
 * Are any of the characters in the two strings the same?
 */
int 
anyof(char *s1, char *s2)
{

	while (*s1)
		if (strchr(s2, *s1++))
			return 1;
	return 0;
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */
int 
is_prefix(const char *as1, const char *as2)
{
	const char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

char *
last_at_before_slash(const char *sp)
{
	const char	*cp;

	for (cp = sp; *cp; cp++)
		if (*cp == '/')
			break;
	while (cp > sp && *--cp != '@');
	return *cp == '@' ? (char *)cp : NULL;
}

enum protocol 
which_protocol(const char *name)
{
	register const char *cp;
	char	*np;
	size_t	sz;
	struct stat	st;
	enum protocol	p;

	if (name[0] == '%' && name[1] == ':')
		name += 2;
	for (cp = name; *cp && *cp != ':'; cp++)
		if (!alnumchar(*cp&0377))
			goto file;
	if (cp[0] == ':' && cp[1] == '/' && cp[2] == '/') {
		if (strncmp(name, "pop3://", 7) == 0)
			return PROTO_POP3;
		if (strncmp(name, "pop3s://", 8) == 0)
#ifdef	USE_SSL
			return PROTO_POP3;
#else	/* !USE_SSL */
			fprintf(stderr, catgets(catd, CATSET, 225,
					"No SSL support compiled in.\n"));
#endif	/* !USE_SSL */
		if (strncmp(name, "imap://", 7) == 0)
			return PROTO_IMAP;
		if (strncmp(name, "imaps://", 8) == 0)
#ifdef	USE_SSL
			return PROTO_IMAP;
#else	/* !USE_SSL */
			fprintf(stderr, catgets(catd, CATSET, 225,
					"No SSL support compiled in.\n"));
#endif	/* !USE_SSL */
		return PROTO_UNKNOWN;
	} else {
	file:	p = PROTO_FILE;
		np = ac_alloc((sz = strlen(name)) + 5);
		strcpy(np, name);
		if (stat(name, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				strcpy(&np[sz], "/tmp");
				if (stat(np, &st) == 0 && S_ISDIR(st.st_mode)) {
					strcpy(&np[sz], "/new");
					if (stat(np, &st) == 0 &&
							S_ISDIR(st.st_mode)) {
						strcpy(&np[sz], "/cur");
						if (stat(np, &st) == 0 &&
							S_ISDIR(st.st_mode))
						    p = PROTO_MAILDIR;
					}
				}
			}
		} else {
			strcpy(&np[sz], ".gz");
			if (stat(np, &st) < 0) {
				strcpy(&np[sz], ".bz2");
				if (stat(np, &st) < 0) {
					if ((cp = value("newfolders")) != 0 &&
						strcmp(cp, "maildir") == 0)
					p = PROTO_MAILDIR;
				}
			}
		}
		ac_free(np);
		return p;
	}
}

const char *
protfile(const char *xcp)
{
	const char	*cp = xcp;
	int	state = 0;

	while (*cp) {
		if (cp[0] == ':' && cp[1] == '/' && cp[2] == '/') {
			cp += 3;
			state = 1;
		}
		if (cp[0] == '/' && state == 1)
			return &cp[1];
		if (cp[0] == '/')
			return xcp;
		cp++;
	}
	return cp;
}

char *
protbase(const char *cp)
{
	char	*n = salloc(strlen(cp) + 1);
	char	*np = n;

	while (*cp) {
		if (cp[0] == ':' && cp[1] == '/' && cp[2] == '/') {
			*np++ = *cp++;
			*np++ = *cp++;
			*np++ = *cp++;
		} else if (cp[0] == '/')
			break;
		else
			*np++ = *cp++;
	}
	*np = '\0';
	return n;
}

int 
disconnected(const char *file)
{
	char	*cp, *cq, *vp;
	int	vs, r;

	if (value("disconnected"))
		return 1;
	cp = protbase(file);
	if (strncmp(cp, "imap://", 7) == 0)
		cp += 7;
	else if (strncmp(cp, "imaps://", 8) == 0)
		cp += 8;
	else
		return 0;
	if ((cq = strchr(cp, ':')) != NULL)
		*cq = '\0';
	vp = ac_alloc(vs = strlen(cp) + 14);
	snprintf(vp, vs, "disconnected-%s", cp);
	r = value(vp) != NULL;
	ac_free(vp);
	return r;
}

unsigned 
pjw(const char *cp)
{
	unsigned	h = 0, g;

	cp--;
	while (*++cp) {
		h = (h << 4 & 0xffffffff) + (*cp&0377);
		if ((g = h & 0xf0000000) != 0) {
			h = h ^ g >> 24;
			h = h ^ g;
		}
	}
	return h;
}

long 
nextprime(long n)
{
	const long	primes[] = {
			509, 1021, 2039, 4093, 8191, 16381, 32749, 65521,
			131071, 262139, 524287, 1048573, 2097143, 4194301,
			8388593, 16777213, 33554393, 67108859, 134217689,
			268435399, 536870909, 1073741789, 2147483647
		};
	long	mprime = 7;
	int	i;

	for (i = 0; i < sizeof primes / sizeof *primes; i++)
		if ((mprime = primes[i]) >= (n < 65536 ? n*4 :
					n < 262144 ? n*2 : n))
			break;
	if (i == sizeof primes / sizeof *primes)
		mprime = n;	/* not so prime, but better than failure */
	return mprime;
}

#define	Hexchar(n)	((n)>9 ? (n)-10+'A' : (n)+'0')
#define	hexchar(n)	((n)>9 ? (n)-10+'a' : (n)+'0')

char *
strenc(const char *cp)
{
	char	*n, *np;

	np = n = salloc(strlen(cp) * 3 + 1);
	while (*cp) {
		if (alnumchar(*cp&0377) || *cp == '_' || *cp == '@' ||
				(np > n && (*cp == '.' || *cp == '-' ||
					    *cp == ':')))
			*np++ = *cp;
		else {
			*np++ = '%';
			*np++ = Hexchar((*cp&0xf0) >> 4);
			*np++ = Hexchar(*cp&0x0f);
		}
		cp++;
	}
	*np = '\0';
	return n;
}

char *
strdec(const char *cp)
{
	char	*n, *np;

	np = n = salloc(strlen(cp) + 1);
	while (*cp) {
		if (cp[0] == '%' && cp[1] && cp[2]) {
			*np = (int)(cp[1]>'9'?cp[1]-'A'+10:cp[1]-'0') << 4;
			*np++ |= cp[2]>'9'?cp[2]-'A'+10:cp[2]-'0';
			cp += 3;
		} else
			*np++ = *cp++;
	}
	*np = '\0';
	return n;
}

char *
md5tohex(const void *vp)
{
	char	*hex;
	const char	*cp = vp;
	int	i;

	hex = salloc(33);
	for (i = 0; i < 16; i++) {
		hex[2*i] = hexchar((cp[i]&0xf0) >> 4);
		hex[2*i+1] = hexchar(cp[i]&0x0f);
	}
	hex[32] = '\0';
	return hex;
}

char *
cram_md5_string(const char *user, const char *pass, const char *b64)
{
	struct str	in, out;
	char	digest[16], *cp, *sp, *rp, *xp;
	int	ss, rs;

	in.s = (char *)b64;
	in.l = strlen(in.s);
	mime_fromb64(&in, &out, 0);
	hmac_md5((unsigned char *)out.s, out.l,
			(unsigned char *)pass, strlen(pass),
			digest);
	free(out.s);
	xp = md5tohex(digest);
	sp = ac_alloc(ss = strlen(user) + strlen(xp) + 2);
	snprintf(sp, ss, "%s %s", user, xp);
	cp = strtob64(sp);
	ac_free(sp);
	rp = salloc(rs = strlen(cp) + 3);
	snprintf(rp, rs, "%s\r\n", cp);
	free(cp);
	return rp;
}

char *
getuser(void)
{
	char *line = NULL, *user;
	size_t linesize = 0;

	if (is_a_tty[0]) {
		fputs("User: ", stdout);
		fflush(stdout);
	}
	if (readline(stdin, &line, &linesize) == 0) {
		if (line)
			free(line);
		return NULL;
	}
	user = savestr(line);
	free(line);
	return user;
}

char *
getpassword(struct termios *otio, int *reset_tio, const char *query)
{
	struct termios	tio;
	char *line = NULL, *pass;
	size_t linesize = 0;
	int	i;

	if (is_a_tty[0]) {
		fputs(query ? query : "Password:", stdout);
		fflush(stdout);
		tcgetattr(0, &tio);
		*otio = tio;
		tio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		*reset_tio = 1;
		tcsetattr(0, TCSAFLUSH, &tio);
	}
	i = readline(stdin, &line, &linesize);
	if (is_a_tty[0]) {
		fputc('\n', stdout);
		tcsetattr(0, TCSADRAIN, otio);
	}
	*reset_tio = 0;
	if (i < 0) {
		if (line)
			free(line);
		return NULL;
	}
	pass = savestr(line);
	free(line);
	return pass;
}

void 
transflags(struct message *omessage, long omsgCount, int transparent)
{
	struct message	*omp, *nmp, *newdot, *newprevdot;
	int	hf;

	omp = omessage;
	nmp = message;
	newdot = message;
	newprevdot = NULL;
	while (omp < &omessage[omsgCount] &&
			nmp < &message[msgCount]) {
		if (dot && nmp->m_uid == dot->m_uid)
			newdot = nmp;
		if (prevdot && nmp->m_uid == prevdot->m_uid)
			newprevdot = nmp;
		if (omp->m_uid == nmp->m_uid) {
			hf = nmp->m_flag & MHIDDEN;
			if (transparent && mb.mb_type == MB_IMAP)
				omp->m_flag &= ~MHIDDEN;
			*nmp++ = *omp++;
			if (transparent && mb.mb_type == MB_CACHE)
				nmp[-1].m_flag |= hf;
		} else if (omp->m_uid < nmp->m_uid)
			omp++;
		else
			nmp++;
	}
	dot = newdot;
	setdot(newdot);
	prevdot = newprevdot;
	free(omessage);
}

char *
getrandstring(size_t length)
{
	static unsigned char	nodedigest[16];
	static pid_t	pid;
	int	i, fd = -1;
	char	*data;
	char	*cp, *rp;
	MD5_CTX	ctx;

	data = salloc(length);
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0 ||
			read(fd, data, length) != length) {
		if (pid == 0) {
			pid = getpid();
			srand(pid);
			cp = nodename(0);
			MD5Init(&ctx);
			MD5Update(&ctx, (unsigned char *)cp, strlen(cp));
			MD5Final(nodedigest, &ctx);
		}
		for (i = 0; i < length; i++)
			data[i] = (int)(255 * (rand() / (RAND_MAX + 1.0))) ^
				nodedigest[i % sizeof nodedigest];
	}
	if (fd > 0)
		close(fd);
	cp = memtob64(data, length);
	rp = salloc(length+1);
	strncpy(rp, cp, length)[length] = '\0';
	free(cp);
	return rp;
}

void 
out_of_memory(void)
{
	panic("no memory");
}

void *
smalloc(size_t s)
{
	void *p;

	if (s == 0)
		s = 1;
	if ((p = malloc(s)) == NULL)
		out_of_memory();
	return p;
}

void *
srealloc(void *v, size_t s)
{
	void *r;

	if (s == 0)
		s = 1;
	if (v == NULL)
		return smalloc(s);
	if ((r = realloc(v, s)) == NULL)
		out_of_memory();
	return r;
}

void *
scalloc(size_t nmemb, size_t size)
{
	void *vp;

	if (size == 0)
		size = 1;
	if ((vp = calloc(nmemb, size)) == NULL)
		out_of_memory();
	return vp;
}

char *
sstpcpy(char *dst, const char *src)
{
	while ((*dst = *src++) != '\0')
		dst++;
	return dst;
}

char *
sstrdup(const char *cp)
{
	char	*dp;
	
	if (cp) {
		dp = smalloc(strlen(cp) + 1);
		strcpy(dp, cp);
		return dp;
	} else
		return NULL;
}

enum okay 
makedir(const char *name)
{
	int	e;
	struct stat	st;

	if (mkdir(name, 0700) < 0) {
		e = errno;
		if ((e == EEXIST || e == ENOSYS) &&
				stat(name, &st) == 0 &&
				(st.st_mode&S_IFMT) == S_IFDIR)
			return OKAY;
		return STOP;
	}
	return OKAY;
}

#ifdef	HAVE_FCHDIR
enum okay 
cwget(struct cw *cw)
{
	if ((cw->cw_fd = open(".", O_RDONLY)) < 0)
		return STOP;
	if (fchdir(cw->cw_fd) < 0) {
		close(cw->cw_fd);
		return STOP;
	}
	return OKAY;
}

enum okay 
cwret(struct cw *cw)
{
	if (fchdir(cw->cw_fd) < 0)
		return STOP;
	return OKAY;
}

void 
cwrelse(struct cw *cw)
{
	close(cw->cw_fd);
}
#else	/* !HAVE_FCHDIR */
enum okay 
cwget(struct cw *cw)
{
	if (getcwd(cw->cw_wd, sizeof cw->cw_wd) == NULL || chdir(cw->cw_wd) < 0)
		return STOP;
	return OKAY;
}

enum okay 
cwret(struct cw *cw)
{
	if (chdir(cw->cw_wd) < 0)
		return STOP;
	return OKAY;
}

/*ARGSUSED*/
void 
cwrelse(struct cw *cw)
{
}
#endif	/* !HAVE_FCHDIR */

void
makeprint(struct str *in, struct str *out)
{
	static int	print_all_chars = -1;
	char	*inp, *outp;
	size_t	msz, dist;

	out->s = smalloc(msz = in->l + 1);
	if (print_all_chars == -1)
		print_all_chars = value("print-all-chars") != NULL;
	if (print_all_chars) {
		memcpy(out->s, in->s, in->l);
		out->l = in->l;
		out->s[out->l] = '\0';
		return;
	}
	inp = in->s;
	outp = out->s;
#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
	if (mb_cur_max > 1) {
		wchar_t	wc;
		char	mb[MB_LEN_MAX+1];
		int	i, n;
		out->l = 0;
		while (inp < &in->s[in->l]) {
			if (*inp & 0200)
				n = mbtowc(&wc, inp, &in->s[in->l] - inp);
			else {
				wc = *inp;
				n = 1;
			}
			if (n < 0) {
				mbtowc(&wc, NULL, mb_cur_max);
				wc = utf8 ? 0xFFFD : '?';
				n = 1;
			} else if (n == 0)
				n = 1;
			inp += n;
			if (!iswprint(wc) && wc != '\n' && wc != '\r' &&
					wc != '\b' && wc != '\t') {
				if ((wc & ~(wchar_t)037) == 0)
					wc = utf8 ? 0x2400 | wc : '?';
				else if (wc == 0177)
					wc = utf8 ? 0x2421 : '?';
				else
					wc = utf8 ? 0x2426 : '?';
			}
			if ((n = wctomb(mb, wc)) <= 0)
				continue;
			out->l += n;
			if (out->l >= msz - 1) {
				dist = outp - out->s;
				out->s = srealloc(out->s, msz += 32);
				outp = &out->s[dist];
			}
			for (i = 0; i < n; i++)
				*outp++ = mb[i];
		}
	} else
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
	{
		int	c;
		while (inp < &in->s[in->l]) {
			c = *inp++ & 0377;
			if (!isprint(c) && c != '\n' && c != '\r' &&
					c != '\b' && c != '\t')
				c = '?';
			*outp++ = c;
		}
		out->l = in->l;
	}
	out->s[out->l] = '\0';
}

char *
prstr(const char *s)
{
	struct str	in, out;
	char	*rp;

	in.s = (char *)s;
	in.l = strlen(s);
	makeprint(&in, &out);
	rp = salloc(out.l + 1);
	memcpy(rp, out.s, out.l);
	rp[out.l] = '\0';
	free(out.s);
	return rp;
}

int
prout(const char *s, size_t sz, FILE *fp)
{
	struct str	in, out;
	int	n;

	in.s = (char *)s;
	in.l = sz;
	makeprint(&in, &out);
	n = fwrite(out.s, 1, out.l, fp);
	free(out.s);
	return n;
}

/*
 * Print out a Unicode character or a substitute for it.
 */
int
putuc(int u, int c, FILE *fp)
{
#if defined (HAVE_MBTOWC) && defined (HAVE_WCTYPE_H)
	if (utf8 && u & ~(wchar_t)0177) {
		char	mb[MB_LEN_MAX];
		int	i, n, r = 0;
		if ((n = wctomb(mb, u)) > 0) {
			for (i = 0; i < n; i++)
				r += putc(mb[i] & 0377, fp) != EOF;
			return r;
		} else if (n == 0)
			return putc('\0', fp) != EOF;
		else
			return 0;
	} else
#endif	/* HAVE_MBTOWC && HAVE_WCTYPE_H */
		return putc(c, fp) != EOF;
}

/*
 * Locale-independent character class functions.
 */
int 
asccasecmp(const char *s1, const char *s2)
{
	register int cmp;

	do
		if ((cmp = lowerconv(*s1 & 0377) - lowerconv(*s2 & 0377)) != 0)
			return cmp;
	while (*s1++ != '\0' && *s2++ != '\0');
	return 0;
}

int
ascncasecmp(const char *s1, const char *s2, size_t sz)
{
	register int cmp;
	size_t i = 1;

	if (sz == 0)
		return 0;
	do
		if ((cmp = lowerconv(*s1 & 0377) - lowerconv(*s2 & 0377)) != 0)
			return cmp;
	while (i++ < sz && *s1++ != '\0' && *s2++ != '\0');
	return 0;
}

char *
asccasestr(const char *haystack, const char *xneedle)
{
	char	*needle, *NEEDLE;
	int	i, sz;

	sz = strlen(xneedle);
	if (sz == 0)
		return (char *)haystack;
	needle = ac_alloc(sz);
	NEEDLE = ac_alloc(sz);
	for (i = 0; i < sz; i++) {
		needle[i] = lowerconv(xneedle[i]&0377);
		NEEDLE[i] = upperconv(xneedle[i]&0377);
	}
	while (*haystack) {
		if (*haystack == *needle || *haystack == *NEEDLE) {
			for (i = 1; i < sz; i++)
				if (haystack[i] != needle[i] &&
						haystack[i] != NEEDLE[i])
					break;
			if (i == sz)
				return (char *)haystack;
		}
		haystack++;
	}
	return NULL;
}

const unsigned char class_char[] = {
/*	000 nul	001 soh	002 stx	003 etx	004 eot	005 enq	006 ack	007 bel	*/
	C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,
/*	010 bs 	011 ht 	012 nl 	013 vt 	014 np 	015 cr 	016 so 	017 si 	*/
	C_CNTRL,C_BLANK,C_WHITE,C_SPACE,C_SPACE,C_SPACE,C_CNTRL,C_CNTRL,
/*	020 dle	021 dc1	022 dc2	023 dc3	024 dc4	025 nak	026 syn	027 etb	*/
	C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,
/*	030 can	031 em 	032 sub	033 esc	034 fs 	035 gs 	036 rs 	037 us 	*/
	C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,C_CNTRL,
/*	040 sp 	041  ! 	042  " 	043  # 	044  $ 	045  % 	046  & 	047  ' 	*/
	C_BLANK,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,
/*	050  ( 	051  ) 	052  * 	053  + 	054  , 	055  - 	056  . 	057  / 	*/
	C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,
/*	060  0 	061  1 	062  2 	063  3 	064  4 	065  5 	066  6 	067  7 	*/
	C_OCTAL,C_OCTAL,C_OCTAL,C_OCTAL,C_OCTAL,C_OCTAL,C_OCTAL,C_OCTAL,
/*	070  8 	071  9 	072  : 	073  ; 	074  < 	075  = 	076  > 	077  ? 	*/
	C_DIGIT,C_DIGIT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,
/*	100  @ 	101  A 	102  B 	103  C 	104  D 	105  E 	106  F 	107  G 	*/
	C_PUNCT,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,
/*	110  H 	111  I 	112  J 	113  K 	114  L 	115  M 	116  N 	117  O 	*/
	C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,
/*	120  P 	121  Q 	122  R 	123  S 	124  T 	125  U 	126  V 	127  W 	*/
	C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,C_UPPER,
/*	130  X 	131  Y 	132  Z 	133  [ 	134  \ 	135  ] 	136  ^ 	137  _ 	*/
	C_UPPER,C_UPPER,C_UPPER,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,
/*	140  ` 	141  a 	142  b 	143  c 	144  d 	145  e 	146  f 	147  g 	*/
	C_PUNCT,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,
/*	150  h 	151  i 	152  j 	153  k 	154  l 	155  m 	156  n 	157  o 	*/
	C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,
/*	160  p 	161  q 	162  r 	163  s 	164  t 	165  u 	166  v 	167  w 	*/
	C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,C_LOWER,
/*	170  x 	171  y 	172  z 	173  { 	174  | 	175  } 	176  ~ 	177 del	*/
	C_LOWER,C_LOWER,C_LOWER,C_PUNCT,C_PUNCT,C_PUNCT,C_PUNCT,C_CNTRL
};

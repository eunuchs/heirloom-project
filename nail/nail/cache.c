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
static char sccsid[] = "@(#)cache.c	1.61 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "config.h"

#ifdef	HAVE_SOCKETS

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

#include <termios.h>

/*
 * Mail -- a mail program
 *
 * A cache for IMAP.
 */

static char *encname(struct mailbox *mp, const char *name, int same,
		const char *box);
static char *encuid(struct mailbox *mp, unsigned long uid);
static FILE *clean(struct mailbox *mp, struct cw *cw);
static unsigned long *builds(long *contentelem);
static void purge(struct mailbox *mp, struct message *m, long mc,
		struct cw *cw, const char *name);
static int longlt(const void *a, const void *b);
static void remve(unsigned long n);
static FILE *cache_queue1(struct mailbox *mp, char *mode, char **xname);
static enum okay dequeue1(struct mailbox *mp);

static const char	infofmt[] = "%c %lu %u %lu %lu";
#define	INITSKIP	128L
#define	USEBITS(f)	\
	((f) & (MSAVED|MDELETED|MREAD|MBOXED|MNEW|MFLAGGED|MANSWERED|MDRAFTED))

static const char	README1[] = "\
This is a cache directory maintained by mailx(1). You should not change any\n\
files within. Nevertheless, the structure is as follows: Each subdirectory\n\
of the current directory represents an IMAP account, and each subdirectory\n\
below that represents a mailbox. Each mailbox directory contains a file\n\
named UIDVALIDITY which describes the validity in relation to the version\n\
on the server. Other files have names corresponding to their IMAP UID.\n";
static const char	README2[] = "\n\
The first 128 bytes of these files are used to store message attributes; the\n\
following data is equivalent to compress(1) output. So if you have to save a\n\
message by hand because of an emergency, throw away the first 128 bytes and\n\
decompress the rest, as e.g. 'dd if=MESSAGEFILE skip=1 bs=128 | zcat' does.\n";
static const char	README3[] = "\n\
Files named QUEUE contain data that will be sent do the IMAP server next\n\
time a connection is made in online mode.\n";
static const char	README4[] = "\n\
You can safely delete any file or directory here, unless it contains a QUEUE\n\
file that is not empty; mailx(1) will download the data again and will also\n\
write new cache entries if configured in this way. If you do not wish to use\n\
the cache anymore, delete the entire directory and unset the 'imap-cache'\n\
variable in mailx(1).\n";
static const char	README5[] = "\n\
For more information about mailx(1), visit\n\
<http://heirloom.sourceforge.net/mailx.html>.\n";

static char *
encname(struct mailbox *mp, const char *name, int same, const char *box)
{
	char	*cachedir, *eaccount, *emailbox, *ename, *res;
	int	resz;

	ename = strenc(name);
	if (mp->mb_cache_directory && same && box == NULL) {
		res = salloc(resz = strlen(mp->mb_cache_directory) +
				strlen(ename) + 2);
		snprintf(res, resz, "%s%s%s", mp->mb_cache_directory,
				*ename ? "/" : "", ename);
	} else {
		if ((cachedir = value("imap-cache")) == NULL)
			return NULL;
		cachedir = expand(cachedir);
		eaccount = strenc(mp->mb_imap_account);
		if (box)
			emailbox = strenc(box);
		else if (asccasecmp(mp->mb_imap_mailbox, "INBOX"))
			emailbox = strenc(mp->mb_imap_mailbox);
		else
			emailbox = "INBOX";
		res = salloc(resz = strlen(cachedir) + strlen(eaccount) +
				strlen(emailbox) + strlen(ename) + 4);
		snprintf(res, resz, "%s/%s/%s%s%s",
				cachedir, eaccount, emailbox,
				*ename ? "/" : "", ename);
	}
	return res;
}

static char *
encuid(struct mailbox *mp, unsigned long uid)
{
	char	buf[30];

	snprintf(buf, sizeof buf, "%lu", uid);
	return encname(mp, buf, 1, NULL);
}

enum okay 
getcache1(struct mailbox *mp, struct message *m, enum needspec need,
		int setflags)
{
	FILE	*fp;
	long	n = 0, size = 0, xsize, xtime, xlines = -1, lines = 0;
	int	lastc = EOF, i, xflag, inheader = 1;
	char	b, iob[32768];
	off_t	offset;
	void	*zp;

	if (setflags == 0 && ((mp->mb_type != MB_IMAP &&
					mp->mb_type != MB_CACHE) ||
			m->m_uid == 0))
		return STOP;
	if ((fp = Fopen(encuid(mp, m->m_uid), "r")) == NULL)
		return STOP;
	fcntl_lock(fileno(fp), F_RDLCK);
	if (fscanf(fp, infofmt, &b, &xsize, &xflag, &xtime, &xlines) < 4)
		goto fail;
	if (need != NEED_UNSPEC) {
		switch (b) {
		case 'H':
			if (need == NEED_HEADER)
				goto success;
			goto fail;
		case 'B':
			if (need == NEED_HEADER || need == NEED_BODY)
				goto success;
			goto fail;
		default:
			goto fail;
		}
	}
success:
	if (b == 'N')
		goto flags;
	fseek(fp, INITSKIP, SEEK_SET);
	zp = zalloc(fp);
	fseek(mp->mb_otf, 0L, SEEK_END);
	offset = ftell(mp->mb_otf);
	while (inheader && (n = zread(zp, iob, sizeof iob)) > 0) {
		size += n;
		for (i = 0; i < n; i++) {
			if (iob[i] == '\n') {
				lines++;
				if (lastc == '\n')
					inheader = 0;
			}
			lastc = iob[i]&0377;
		}
		fwrite(iob, 1, n, mp->mb_otf);
	}
	if (n > 0 && need == NEED_BODY) {
		while ((n = zread(zp, iob, sizeof iob)) > 0) {
			size += n;
			for (i = 0; i < n; i++)
				if (iob[i] == '\n')
					lines++;
			fwrite(iob, 1, n, mp->mb_otf);
		}
	}
	fflush(mp->mb_otf);
	if (zfree(zp) < 0 || n < 0 || ferror(fp) || ferror(mp->mb_otf))
		goto fail;
	m->m_size = size;
	m->m_lines = lines;
	m->m_block = mailx_blockof(offset);
	m->m_offset = mailx_offsetof(offset);
flags:	if (setflags) {
		m->m_xsize = xsize;
		m->m_time = xtime;
		if (setflags & 2) {
			m->m_flag = xflag | MNOFROM;
			if (b != 'B')
				m->m_flag |= MHIDDEN;
		}
	}
	if (xlines > 0 && m->m_xlines <= 0)
		m->m_xlines = xlines;
	switch (b) {
	case 'B':
		m->m_xsize = xsize;
		if (xflag == MREAD && xlines > 0)
			m->m_flag |= MFULLYCACHED;
		if (need == NEED_BODY) {
			m->m_have |= HAVE_HEADER|HAVE_BODY;
			if (m->m_lines > 0)
				m->m_xlines = m->m_lines;
			break;
		}
		/*FALLTHRU*/
	case 'H':
		m->m_have |= HAVE_HEADER;
		break;
	case 'N':
		break;
	}
	Fclose(fp);
	return OKAY;
fail:
	Fclose(fp);
	return STOP;
}

enum okay 
getcache(struct mailbox *mp, struct message *m, enum needspec need)
{
	return getcache1(mp, m, need, 0);
}

void 
putcache(struct mailbox *mp, struct message *m)
{
	FILE	*ibuf, *obuf;
	char	*name, ob;
	int	c, oflag;
	long	n, count, oldoffset, osize, otime, olines = -1;
	char	iob[32768];
	void	*zp;

	if ((mp->mb_type != MB_IMAP && mp->mb_type != MB_CACHE) ||
			m->m_uid == 0 || m->m_time == 0 ||
			(m->m_flag & (MTOUCH|MFULLYCACHED)) == MFULLYCACHED)
		return;
	if (m->m_have & HAVE_BODY)
		c = 'B';
	else if (m->m_have & HAVE_HEADER)
		c = 'H';
	else if (m->m_have == HAVE_NOTHING)
		c = 'N';
	else
		return;
	oldoffset = ftell(mp->mb_itf);
	if ((obuf = Fopen(name = encuid(mp, m->m_uid), "r+")) == NULL) {
		if ((obuf = Fopen(name, "w")) == NULL)
			return; 
		fcntl_lock(fileno(obuf), F_WRLCK);
	} else {
		fcntl_lock(fileno(obuf), F_WRLCK);
		if (fscanf(obuf, infofmt, &ob, &osize, &oflag, &otime,
					&olines) >= 4 && ob != '\0' &&
				(ob == 'B' || (ob == 'H' && c != 'B'))) {
			if (m->m_xlines <= 0 && olines > 0)
				m->m_xlines = olines;
			if ((c != 'N' && osize != m->m_xsize) ||
					oflag != USEBITS(m->m_flag) ||
					otime != m->m_time ||
					(m->m_xlines > 0 &&
					 olines != m->m_xlines)) {
				fflush(obuf);
				rewind(obuf);
				fprintf(obuf, infofmt, ob,
					(long)m->m_xsize,
					USEBITS(m->m_flag),
					(long)m->m_time,
					m->m_xlines);
				putc('\n', obuf);
			}
			Fclose(obuf);
			return;
		}
		fflush(obuf);
		rewind(obuf);
		ftruncate(fileno(obuf), 0);
	}
	if ((ibuf = setinput(mp, m, NEED_UNSPEC)) == NULL) {
		Fclose(obuf);
		return;
	}
	if (c == 'N')
		goto done;
	fseek(obuf, INITSKIP, SEEK_SET);
	zp = zalloc(obuf);
	count = m->m_size;
	while (count > 0) {
		n = count > sizeof iob ? sizeof iob : count;
		count -= n;
		if (fread(iob, 1, n, ibuf) != n || zwrite(zp, iob, n) != n) {
			unlink(name);
			zfree(zp);
			goto out;
		}
	}
	if (zfree(zp) < 0) {
		unlink(name);
		goto out;
	}
done:	rewind(obuf);
	fprintf(obuf, infofmt, c, (long)m->m_xsize,
			USEBITS(m->m_flag),
			(long)m->m_time,
			m->m_xlines);
	putc('\n', obuf);
	if (ferror(obuf)) {
		unlink(name);
		goto out;
	}
	if (c == 'B' && USEBITS(m->m_flag) == MREAD)
		m->m_flag |= MFULLYCACHED;
out:	if (Fclose(obuf) != 0) {
		m->m_flag &= ~MFULLYCACHED;
		unlink(name);
	}
	fseek(mp->mb_itf, oldoffset, SEEK_SET);
}

void 
initcache(struct mailbox *mp)
{
	char	*name, *uvname;
	FILE	*uvfp;
	unsigned long	uv;
	struct cw	cw;

	free(mp->mb_cache_directory);
	mp->mb_cache_directory = NULL;
	if ((name = encname(mp, "", 1, NULL)) == NULL)
		return;
	mp->mb_cache_directory = sstrdup(name);
	if ((uvname = encname(mp, "UIDVALIDITY", 1, NULL)) == NULL)
		return;
	if (cwget(&cw) == STOP)
		return;
	if ((uvfp = Fopen(uvname, "r+")) == NULL ||
			(fcntl_lock(fileno(uvfp), F_RDLCK), 0) ||
			fscanf(uvfp, "%lu", &uv) != 1 ||
			uv != mp->mb_uidvalidity) {
		if ((uvfp = clean(mp, &cw)) == NULL)
			goto out;
	} else {
		fflush(uvfp);
		rewind(uvfp);
	}
	fcntl_lock(fileno(uvfp), F_WRLCK);
	fprintf(uvfp, "%lu\n", mp->mb_uidvalidity);
	if (ferror(uvfp) || Fclose(uvfp) != 0) {
		unlink(uvname);
		mp->mb_uidvalidity = 0;
	}
out:	cwrelse(&cw);
}

void 
purgecache(struct mailbox *mp, struct message *m, long mc)
{
	char	*name;
	struct cw	cw;

	if ((name = encname(mp, "", 1, NULL)) == NULL)
		return;
	if (cwget(&cw) == STOP)
		return;
	purge(mp, m, mc, &cw, name);
	cwrelse(&cw);
}

static FILE *
clean(struct mailbox *mp, struct cw *cw)
{
	char	*cachedir, *eaccount, *emailbox, *buf;
	int	bufsz;
	DIR	*dirfd;
	struct dirent	*dp;
	FILE	*fp = NULL;

	if ((cachedir = value("imap-cache")) == NULL)
		return NULL;
	cachedir = expand(cachedir);
	eaccount = strenc(mp->mb_imap_account);
	if (asccasecmp(mp->mb_imap_mailbox, "INBOX"))
		emailbox = strenc(mp->mb_imap_mailbox);
	else
		emailbox = "INBOX";
	buf = salloc(bufsz = strlen(cachedir) + strlen(eaccount) +
			strlen(emailbox) + 40);
	if (makedir(cachedir) != OKAY)
		return NULL;
	snprintf(buf, bufsz, "%s/README", cachedir);
	if ((fp = Fopen(buf, "wx")) != NULL) {
		fputs(README1, fp);
		fputs(README2, fp);
		fputs(README3, fp);
		fputs(README4, fp);
		fputs(README5, fp);
		Fclose(fp);
	}
	snprintf(buf, bufsz, "%s/%s", cachedir, eaccount);
	if (makedir(buf) != OKAY)
		return NULL;
	snprintf(buf, bufsz, "%s/%s/%s", cachedir, eaccount, emailbox);
	if (makedir(buf) != OKAY)
		return NULL;
	if (chdir(buf) < 0)
		return NULL;
	if ((dirfd = opendir(".")) == NULL)
		goto out;
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		unlink(dp->d_name);
	}
	closedir(dirfd);
	fp = Fopen("UIDVALIDITY", "w");
out:	if (cwret(cw) == STOP) {
		fputs("Fatal: Cannot change back to current directory.\n",
				stderr);
		abort();
	}
	return fp;
}

static unsigned long *
builds(long *contentelem)
{
	unsigned long	n, *contents = NULL;
	long	contentalloc = 0;
	char	*x;
	DIR	*dirfd;
	struct dirent	*dp;

	*contentelem = 0;
	if ((dirfd = opendir(".")) == NULL)
		return NULL;
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		n = strtoul(dp->d_name, &x, 10);
		if (*x != '\0')
			continue;
		if (*contentelem >= contentalloc - 1)
			contents = srealloc(contents,
				(contentalloc += 200) * sizeof *contents);
		contents[(*contentelem)++] = n;
	}
	closedir(dirfd);
	if (*contentelem > 0) {
		contents[*contentelem] = 0;
		qsort(contents, *contentelem, sizeof *contents, longlt);
	}
	return contents;
}

static void 
purge(struct mailbox *mp, struct message *m, long mc, struct cw *cw,
		const char *name)
{
	unsigned long	*contents;
	long	i, j, contentelem;

	if (chdir(name) < 0)
		return;
	contents = builds(&contentelem);
	if (contents) {
		i = j = 0;
		while (j < contentelem) {
			if (i < mc && m[i].m_uid == contents[j]) {
				i++;
				j++;
			} else if (i < mc && m[i].m_uid < contents[j])
				i++;
			else
				remve(contents[j++]);
		}
	}
	if (cwret(cw) == STOP) {
		fputs("Fatal: Cannot change back to current directory.\n",
				stderr);
		abort();
	}
	free(contents);
}

static int 
longlt(const void *a, const void *b)
{
	return *(long *)a - *(long *)b;
}

static void 
remve(unsigned long n)
{
	char	buf[30];

	snprintf(buf, sizeof buf, "%lu", n);
	unlink(buf);
}

void 
delcache(struct mailbox *mp, struct message *m)
{
	char	*fn;

	fn = encuid(mp, m->m_uid);
	if (fn && unlink(fn) == 0)
		m->m_flag |= MUNLINKED;
}

enum okay 
cache_setptr(int transparent)
{
	int	i;
	struct cw	cw;
	char	*name;
	unsigned long	*contents;
	long	contentelem;
	enum okay	ok = STOP;
	struct message	*omessage = NULL;
	int	omsgCount = 0;

	if (transparent) {
		omessage = message;
		omsgCount = msgCount;
	}
	free(mb.mb_cache_directory);
	mb.mb_cache_directory = NULL;
	if ((name = encname(&mb, "", 1, NULL)) == NULL)
		return STOP;
	mb.mb_cache_directory = sstrdup(name);
	if (cwget(&cw) == STOP)
		return STOP;
	if (chdir(name) < 0)
		return STOP;
	contents = builds(&contentelem);
	msgCount = contentelem;
	message = scalloc(msgCount + 1, sizeof *message);
	if (cwret(&cw) == STOP) {
		fputs("Fatal: Cannot change back to current directory.\n",
				stderr);
		abort();
	}
	cwrelse(&cw);
	for (i = 0; i < msgCount; i++) {
		message[i].m_uid = contents[i];
		getcache1(&mb, &message[i], NEED_UNSPEC, 3);
	}
	ok = OKAY;
	if (ok == OKAY) {
		mb.mb_type = MB_CACHE;
		mb.mb_perm = Rflag ? 0 : MB_DELE;
		if (transparent)
			transflags(omessage, omsgCount, 1);
		else
			setdot(message);
	}
	return ok;
}

enum okay
cache_list(struct mailbox *mp, const char *base, int strip, FILE *fp)
{
	char	*name, *cachedir, *eaccount;
	DIR	*dirfd;
	struct dirent	*dp;
	const char	*cp, *bp, *sp;
	int	namesz;

	if ((cachedir = value("imap-cache")) == NULL)
		return STOP;
	cachedir = expand(cachedir);
	eaccount = strenc(mp->mb_imap_account);
	name = salloc(namesz = strlen(cachedir) + strlen(eaccount) + 2);
	snprintf(name, namesz, "%s/%s", cachedir, eaccount);
	if ((dirfd = opendir(name)) == NULL)
		return STOP;
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		cp = sp = strdec(dp->d_name);
		for (bp = base; *bp && *bp == *sp; bp++)
			sp++;
		if (*bp)
			continue;
		cp = strip ? sp : cp;
		fprintf(fp, "%s\n", *cp ? cp : "INBOX");
	}
	closedir(dirfd);
	return OKAY;
}

enum okay 
cache_remove(const char *name)
{
	struct stat	st;
	DIR	*dirfd;
	struct dirent	*dp;
	char	*path;
	int	pathsize, pathend, n;
	char	*dir;

	if ((dir = encname(&mb, "", 0, protfile(name))) == NULL)
		return OKAY;
	pathend = strlen(dir);
	path = smalloc(pathsize = pathend + 30);
	strcpy(path, dir);
	path[pathend++] = '/';
	path[pathend] = '\0';
	if ((dirfd = opendir(path)) == NULL) {
		free(path);
		return OKAY;
	}
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		n = strlen(dp->d_name);
		if (pathend + n + 1 > pathsize)
			path = srealloc(path, pathsize = pathend + n + 30);
		strcpy(&path[pathend], dp->d_name);
		if (stat(path, &st) < 0 || (st.st_mode&S_IFMT) != S_IFREG)
			continue;
		if (unlink(path) < 0) {
			perror(path);
			closedir(dirfd);
			free(path);
			return STOP;
		}
	}
	closedir(dirfd);
	path[pathend] = '\0';
	rmdir(path);	/* no error on failure, might contain submailboxes */
	free(path);
	return OKAY;
}

enum okay 
cache_rename(const char *old, const char *new)
{
	char	*olddir, *newdir;

	if ((olddir = encname(&mb, "", 0, protfile(old))) == NULL ||
			(newdir = encname(&mb, "", 0, protfile(new))) == NULL)
		return OKAY;
	if (rename(olddir, newdir) < 0) {
		perror(olddir);
		return STOP;
	}
	return OKAY;
}

unsigned long 
cached_uidvalidity(struct mailbox *mp)
{
	FILE	*uvfp;
	char	*uvname;
	unsigned long	uv;

	if ((uvname = encname(mp, "UIDVALIDITY", 1, NULL)) == NULL)
		return 0;
	if ((uvfp = Fopen(uvname, "r")) == NULL ||
			(fcntl_lock(fileno(uvfp), F_RDLCK), 0) ||
			fscanf(uvfp, "%lu", &uv) != 1)
		uv = 0;
	Fclose(uvfp);
	return uv;
}

static FILE *
cache_queue1(struct mailbox *mp, char *mode, char **xname)
{
	char	*name;
	FILE	*fp;

	if ((name = encname(mp, "QUEUE", 0, NULL)) == NULL)
		return NULL;
	if ((fp = Fopen(name, mode)) != NULL)
		fcntl_lock(fileno(fp), F_WRLCK);
	if (xname)
		*xname = name;
	return fp;
}

FILE *
cache_queue(struct mailbox *mp)
{
	FILE	*fp;

	fp = cache_queue1(mp, "a", NULL);
	if (fp == NULL)
		fputs("Cannot queue IMAP command. Retry when online.\n",
				stderr);
	return fp;
}

enum okay 
cache_dequeue(struct mailbox *mp)
{
	int	bufsz;
	char	*cachedir, *eaccount, *buf, *oldbox;
	DIR	*dirfd;
	struct dirent	*dp;

	if ((cachedir = value("imap-cache")) == NULL)
		return OKAY;
	cachedir = expand(cachedir);
	eaccount = strenc(mp->mb_imap_account);
	buf = salloc(bufsz = strlen(cachedir) + strlen(eaccount) + 2);
	snprintf(buf, bufsz, "%s/%s", cachedir, eaccount);
	if ((dirfd = opendir(buf)) == NULL)
		return OKAY;
	oldbox = mp->mb_imap_mailbox;
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		mp->mb_imap_mailbox = strdec(dp->d_name);
		dequeue1(mp);
	}
	closedir(dirfd);
	mp->mb_imap_mailbox = oldbox;
	return OKAY;
}

static enum okay 
dequeue1(struct mailbox *mp)
{
	FILE	*fp = NULL, *uvfp = NULL;
	char	*qname, *uvname;
	unsigned long	uv;
	off_t	is_size;
	int	is_count;

	fp = cache_queue1(mp, "r+", &qname);
	if (fp != NULL && fsize(fp) > 0) {
		if (imap_select(mp, &is_size, &is_count,
					mp->mb_imap_mailbox) != OKAY) {
			fprintf(stderr, "Cannot select \"%s\" for dequeuing.\n",
					mp->mb_imap_mailbox);
			goto save;
		}
		if ((uvname = encname(mp, "UIDVALIDITY", 0, NULL)) == NULL ||
				(uvfp = Fopen(uvname, "r")) == NULL ||
				(fcntl_lock(fileno(uvfp), F_RDLCK), 0) ||
				fscanf(uvfp, "%lu", &uv) != 1 ||
				uv != mp->mb_uidvalidity) {
			fprintf(stderr,
			"Unique identifiers for \"%s\" are out of date. "
				"Cannot commit IMAP commands.\n",
				mp->mb_imap_mailbox);
		save:	fputs("Saving IMAP commands to dead.letter\n", stderr);
			savedeadletter(fp);
			ftruncate(fileno(fp), 0);
			Fclose(fp);
			if (uvfp)
				Fclose(uvfp);
			return STOP;
		}
		Fclose(uvfp);
		printf("Committing IMAP commands for \"%s\"\n",
				mp->mb_imap_mailbox);
		imap_dequeue(mp, fp);
	}
	if (fp) {
		Fclose(fp);
		unlink(qname);
	}
	return OKAY;
}
#endif	/* HAVE_SOCKETS */

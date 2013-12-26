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
static char sccsid[] = "@(#)maildir.c	1.20 (gritter) 12/28/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * Mail -- a mail program
 *
 * Maildir folder support.
 */

static struct mditem {
	struct message	*md_data;
	unsigned	md_hash;
} *mdtable;
static long	mdprime;

static sigjmp_buf	maildirjmp;

static int maildir_setfile1(const char *name, int newmail, int omsgCount);
static int mdcmp(const void *a, const void *b);
static int subdir(const char *name, const char *sub, int newmail);
static void cleantmp(const char *name);
static void append(const char *name, const char *sub, const char *fn);
static void readin(const char *name, struct message *m);
static void maildir_update(void);
static void move(struct message *m);
static char *mkname(time_t t, enum mflag f, const char *pref);
static void maildircatch(int s);
static enum okay maildir_append1(const char *name, FILE *fp, off_t off1,
		long size, enum mflag flag);
static enum okay trycreate(const char *name);
static enum okay mkmaildir(const char *name);
static struct message *mdlook(const char *name, struct message *data);
static void mktable(void);
static enum okay subdir_remove(const char *name, const char *sub);

int 
maildir_setfile(const char *name, int newmail, int isedit)
{
	sighandler_type	saveint;
	struct cw	cw;
	int	i = -1, omsgCount;

	(void)&saveint;
	(void)&i;
	omsgCount = msgCount;
	if (cwget(&cw) == STOP) {
		fprintf(stderr, "Fatal: Cannot open current directory\n");
		return -1;
	}
	if (!newmail)
		quit();
	saveint = safe_signal(SIGINT, SIG_IGN);
	if (chdir(name) < 0) {
		fprintf(stderr, "Cannot change directory to \"%s\".\n", name);
		cwrelse(&cw);
		return -1;
	}
	if (!newmail) {
		edit = isedit;
		if (mb.mb_itf) {
			fclose(mb.mb_itf);
			mb.mb_itf = NULL;
		}
		if (mb.mb_otf) {
			fclose(mb.mb_otf);
			mb.mb_otf = NULL;
		}
		initbox(name);
		mb.mb_type = MB_MAILDIR;
	}
	mdtable = NULL;
	if (sigsetjmp(maildirjmp, 1) == 0) {
		if (newmail)
			mktable();
		if (saveint != SIG_IGN)
			safe_signal(SIGINT, maildircatch);
		i = maildir_setfile1(name, newmail, omsgCount);
	}
	if (newmail)
		free(mdtable);
	safe_signal(SIGINT, saveint);
	if (i < 0) {
		mb.mb_type = MB_VOID;
		*mailname = '\0';
		msgCount = 0;
	}
	if (cwret(&cw) == STOP) {
		fputs("Fatal: Cannot change back to current directory.\n",
				stderr);
		abort();
	}
	cwrelse(&cw);
	setmsize(msgCount);
	if (newmail && mb.mb_sorted && msgCount > omsgCount) {
		mb.mb_threaded = 0;
		sort((void *)-1);
	}
	if (!newmail)
		sawcom = 0;
	if (!newmail && !edit && msgCount == 0) {
		if (mb.mb_type == MB_MAILDIR && value("emptystart") == NULL)
			fprintf(stderr, "No mail at %s\n", name);
		return 1;
	}
	if (newmail && msgCount > omsgCount)
		newmailinfo(omsgCount);
	return 0;
}

static int 
maildir_setfile1(const char *name, int newmail, int omsgCount)
{
	int	i;

	if (!newmail)
		cleantmp(name);
	mb.mb_perm = Rflag ? 0 : MB_DELE;
	if ((i = subdir(name, "cur", newmail)) != 0)
		return i;
	if ((i = subdir(name, "new", newmail)) != 0)
		return i;
	append(name, NULL, NULL);
	for (i = newmail?omsgCount:0; i < msgCount; i++)
		readin(name, &message[i]);
	if (newmail) {
		if (msgCount > omsgCount)
			qsort(&message[omsgCount],
					msgCount - omsgCount,
					sizeof *message, mdcmp);
	} else {
		if (msgCount)
			qsort(message, msgCount, sizeof *message, mdcmp);
	}
	return msgCount;
}

/*
 * In combination with the names from mkname(), this comparison function
 * ensures that the order of messages in a maildir folder created by mailx
 * remains always the same. In effect, if a mbox folder is transferred to
 * a maildir folder by 'copy *', the order of the messages in mailx will
 * not change.
 */
static int 
mdcmp(const void *a, const void *b)
{
	long	i;

	if ((i = ((struct message *)a)->m_time -
				((struct message *)b)->m_time) == 0)
		i = strcmp(&((struct message *)a)->m_maildir_file[4],
				&((struct message *)b)->m_maildir_file[4]);
	return i;
}

static int 
subdir(const char *name, const char *sub, int newmail)
{
	DIR	*dirfd;
	struct dirent	*dp;

	if ((dirfd = opendir(sub)) == NULL) {
		fprintf(stderr, "Cannot open directory \"%s/%s\".\n",
				name, sub);
		return -1;
	}
	if (access(sub, W_OK) < 0)
		mb.mb_perm = 0;
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		if (dp->d_name[0] == '.')
			continue;
		if (!newmail || mdlook(dp->d_name, NULL) == NULL)
			append(name, sub, dp->d_name);
	}
	closedir(dirfd);
	return 0;
}

static void 
cleantmp(const char *name)
{
	struct stat	st;
	DIR	*dirfd;
	struct dirent	*dp;
	char	*fn = NULL;
	size_t	fnsz = 0, ssz;
	time_t	now;

	if ((dirfd = opendir("tmp")) == NULL)
		return;
	time(&now);
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		if (dp->d_name[0] == '.')
			continue;
		if ((ssz = strlen(dp->d_name)) + 5 > fnsz) {
			free(fn);
			fn = smalloc(fnsz = ssz + 40);
		}
		strcpy(fn, "tmp/");
		strcpy(&fn[4], dp->d_name);
		if (stat(fn, &st) < 0)
			continue;
		if (st.st_atime + 36*3600 < now)
			unlink(fn);
	}
	free(fn);
	closedir(dirfd);
}

static void 
append(const char *name, const char *sub, const char *fn)
{
	struct message	*m;
	size_t	sz;
	time_t	t = 0;
	enum mflag	f = MUSED|MNOFROM|MNEWEST;
	const char	*cp;
	char	*xp;

	if (fn && sub) {
		if (strcmp(sub, "new") == 0)
			f |= MNEW;
		t = strtol(fn, &xp, 10);
		if ((cp = strrchr(xp, ',')) != NULL &&
				cp > &xp[2] && cp[-1] == '2' && cp[-2] == ':') {
			while (*++cp) {
				switch (*cp) {
				case 'F':
					f |= MFLAGGED;
					break;
				case 'R':
					f |= MANSWERED;
					break;
				case 'S':
					f |= MREAD;
					break;
				case 'T':
					f |= MDELETED;
					break;
				case 'D':
					f |= MDRAFT;
					break;
				}
			}
		}
	}
	if (msgCount + 1 >= msgspace) {
		const int	chunk = 64;
		message = srealloc(message,
				(msgspace += chunk) * sizeof *message);
		memset(&message[msgCount], 0, chunk * sizeof *message);
	}
	if (fn == NULL || sub == NULL)
		return;
	m = &message[msgCount++];
	m->m_maildir_file = smalloc((sz = strlen(sub)) + strlen(fn) + 2);
	strcpy(m->m_maildir_file, sub);
	m->m_maildir_file[sz] = '/';
	strcpy(&m->m_maildir_file[sz+1], fn);
	m->m_time = t;
	m->m_flag = f;
	m->m_maildir_hash = ~pjw(fn);
	return;
}

static void 
readin(const char *name, struct message *m)
{
	char	*buf, *bp;
	size_t	bufsize, buflen, count;
	long	size = 0, lines = 0;
	off_t	offset;
	FILE	*fp;
	int	emptyline = 0;

	if ((fp = Fopen(m->m_maildir_file, "r")) == NULL) {
		fprintf(stderr, "Cannot read \"%s/%s\" for message %d\n",
				name, m->m_maildir_file, m - &message[0] + 1);
		m->m_flag |= MHIDDEN;
		return;
	}
	buf = smalloc(bufsize = LINESIZE);
	buflen = 0;
	count = fsize(fp);
	fseek(mb.mb_otf, 0L, SEEK_END);
	offset = ftell(mb.mb_otf);
	while (fgetline(&buf, &bufsize, &count, &buflen, fp, 1) != NULL) {
		bp = buf;
		if (buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' &&
				buf[3] == 'm' && buf[4] == ' ') {
			putc('>', mb.mb_otf);
			size++;
		}
		lines++;
		size += fwrite(bp, 1, buflen, mb.mb_otf);
		emptyline = *bp == '\n';
	}
	if (!emptyline) {
		putc('\n', mb.mb_otf);
		lines++;
		size++;
	}
	Fclose(fp);
	fflush(mb.mb_otf);
	m->m_size = m->m_xsize = size;
	m->m_lines = m->m_xlines = lines;
	m->m_block = mailx_blockof(offset);
	m->m_offset = mailx_offsetof(offset);
	free(buf);
	substdate(m);
}

void
maildir_quit(void)
{
	sighandler_type	saveint;
	struct cw	cw;

	(void)&saveint;
	if (cwget(&cw) == STOP) {
		fprintf(stderr, "Fatal: Cannot open current directory\n");
		return;
	}
	saveint = safe_signal(SIGINT, SIG_IGN);
	if (chdir(mailname) < 0) {
		fprintf(stderr, "Cannot change directory to \"%s\".\n",
				mailname);
		cwrelse(&cw);
		return;
	}
	if (sigsetjmp(maildirjmp, 1) == 0) {
		if (saveint != SIG_IGN)
			safe_signal(SIGINT, maildircatch);
		maildir_update();
	}
	safe_signal(SIGINT, saveint);
	if (cwret(&cw) == STOP) {
		fputs("Fatal: Cannot change back to current directory.\n",
				stderr);
		abort();
	}
	cwrelse(&cw);
}

static void
maildir_update(void)
{
	FILE	*readstat = NULL;
	struct message	*m;
	int	dodel, c, gotcha = 0, held = 0, modflags = 0;

	if (mb.mb_perm == 0)
		goto free;
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
			if (makembox() == STOP)
				goto bypass;
	}
	for (m = &message[0], gotcha=0, held=0; m < &message[msgCount]; m++) {
		if (readstat != NULL && (m->m_flag & (MREAD|MDELETED)) != 0) {
			char	*id;
			if ((id = hfield("message-id", m)) != NULL ||
					(id = hfield("article-id", m)) != NULL)
				fprintf(readstat, "%s\n", id);
		}
		if (edit)
			dodel = m->m_flag & MDELETED;
		else
			dodel = !((m->m_flag&MPRESERVE) ||
					(m->m_flag&MTOUCH) == 0);
		if (dodel) {
			if (unlink(m->m_maildir_file) < 0)
				fprintf(stderr, "Cannot delete file \"%s/%s\" "
						"for message %d.\n",
						mailname, m->m_maildir_file,
						m - &message[0] + 1);
			else
				gotcha++;
		} else {
			if ((m->m_flag&(MREAD|MSTATUS)) == (MREAD|MSTATUS) ||
					m->m_flag & (MNEW|MBOXED|MSAVED|MSTATUS|
						MFLAG|MUNFLAG|
						MANSWER|MUNANSWER|
						MDRAFT|MUNDRAFT)) {
				move(m);
				modflags++;
			}
			held++;
		}
	}
bypass:	if (readstat != NULL)
		Fclose(readstat);
	if ((gotcha || modflags) && edit) {
		printf(catgets(catd, CATSET, 168, "\"%s\" "), mailname);
		printf(value("bsdcompat") || value("bsdmsgs") ?
				catgets(catd, CATSET, 170, "complete\n") :
				catgets(catd, CATSET, 212, "updated.\n"));
	} else if (held && !edit && mb.mb_perm != 0) {
		if (held == 1)
			printf(catgets(catd, CATSET, 155,
				"Held 1 message in %s\n"), mailname);
		else if (held > 1)
			printf(catgets(catd, CATSET, 156,
				"Held %d messages in %s\n"), held, mailname);
	}
	fflush(stdout);
free:	for (m = &message[0]; m < &message[msgCount]; m++)
		free(m->m_maildir_file);
}

static void 
move(struct message *m)
{
	char	*fn, *new;

	fn = mkname(0, m->m_flag, &m->m_maildir_file[4]);
	new = savecat("cur/", fn);
	if (strcmp(m->m_maildir_file, new) == 0)
		return;
	if (link(m->m_maildir_file, new) < 0) {
		fprintf(stderr, "Cannot link \"%s/%s\" to \"%s/%s\": "
				"message %d not touched.\n",
				mailname, m->m_maildir_file,
				mailname, new,
				m - &message[0] + 1);
		return;
	}
	if (unlink(m->m_maildir_file) < 0)
		fprintf(stderr, "Cannot unlink \"%s/%s\".\n", 
				mailname, m->m_maildir_file);
}

static char *
mkname(time_t t, enum mflag f, const char *pref)
{
	static unsigned long	count;
	static pid_t	mypid;
	char	*cp;
	static char	*node;
	int	size, n, i;

	if (pref == NULL) {
		if (mypid == 0)
			mypid = getpid();
		if (node == NULL) {
			cp = nodename(0);
			n = size = 0;
			do {
				if (n < size + 8)
					node = srealloc(node, size += 20);
				switch (*cp) {
				case '/':
					node[n++] = '\\', node[n++] = '0',
					node[n++] = '5', node[n++] = '7';
					break;
				case ':':
					node[n++] = '\\', node[n++] = '0',
					node[n++] = '7', node[n++] = '2';
					break;
				default:
					node[n++] = *cp;
				}
			} while (*cp++);
		}
		size = 60 + strlen(node);
		cp = salloc(size);
		n = snprintf(cp, size, "%lu.%06lu_%06lu.%s:2,",
				(unsigned long)t,
				(unsigned long)mypid, ++count, node);
	} else {
		size = (n = strlen(pref)) + 13;
		cp = salloc(size);
		strcpy(cp, pref);
		for (i = n; i > 3; i--)
			if (cp[i-1] == ',' && cp[i-2] == '2' &&
					cp[i-3] == ':') {
				n = i;
				break;
			}
		if (i <= 3) {
			strcpy(&cp[n], ":2,");
			n += 3;
		}
	}
	if (n < size - 7) {
		if (f & MDRAFTED)
			cp[n++] = 'D';
		if (f & MFLAGGED)
			cp[n++] = 'F';
		if (f & MANSWERED)
			cp[n++] = 'R';
		if (f & MREAD)
			cp[n++] = 'S';
		if (f & MDELETED)
			cp[n++] = 'T';
		cp[n] = '\0';
	}
	return cp;
}

static void 
maildircatch(int s)
{
	siglongjmp(maildirjmp, s);
}

enum okay
maildir_append(const char *name, FILE *fp)
{
	char	*buf, *bp, *lp;
	size_t	bufsize, buflen, count;
	off_t	off1 = -1, offs;
	int	inhead = 1;
	int	flag = MNEW|MNEWEST;
	long	size = 0;
	enum okay	ok;

	if (mkmaildir(name) != OKAY)
		return STOP;
	buf = smalloc(bufsize = LINESIZE);
	buflen = 0;
	count = fsize(fp);
	offs = ftell(fp);
	do {
		bp = fgetline(&buf, &bufsize, &count, &buflen, fp, 1);
		if (bp == NULL || strncmp(buf, "From ", 5) == 0) {
			if (off1 != (off_t)-1) {
				ok = maildir_append1(name, fp, off1,
						size, flag);
				if (ok == STOP)
					return STOP;
				fseek(fp, offs+buflen, SEEK_SET);
			}
			off1 = offs + buflen;
			size = 0;
			inhead = 1;
			flag = MNEW;
		} else
			size += buflen;
		offs += buflen;
		if (bp && buf[0] == '\n')
			inhead = 0;
		else if (bp && inhead && ascncasecmp(buf, "status", 6) == 0) {
			lp = &buf[6];
			while (whitechar(*lp&0377))
				lp++;
			if (*lp == ':')
				while (*++lp != '\0')
					switch (*lp) {
					case 'R':
						flag |= MREAD;
						break;
					case 'O':
						flag &= ~MNEW;
						break;
					}
		} else if (bp && inhead &&
				ascncasecmp(buf, "x-status", 8) == 0) {
			lp = &buf[8];
			while (whitechar(*lp&0377))
				lp++;
			if (*lp == ':')
				while (*++lp != '\0')
					switch (*lp) {
					case 'F':
						flag |= MFLAGGED;
						break;
					case 'A':
						flag |= MANSWERED;
						break;
					case 'T':
						flag |= MDRAFTED;
						break;
					}
		}
	} while (bp != NULL);
	free(buf);
	return OKAY;
}

static enum okay
maildir_append1(const char *name, FILE *fp, off_t off1, long size,
		enum mflag flag)
{
	const int	attempts = 43200;
	struct stat	st;
	char	buf[4096];
	char	*fn, *tmp, *new;
	FILE	*op;
	long	n, z;
	int	i;
	time_t	now;

	for (i = 0; i < attempts; i++) {
		time(&now);
		fn = mkname(now, flag, NULL);
		tmp = salloc(n = strlen(name) + strlen(fn) + 6);
		snprintf(tmp, n, "%s/tmp/%s", name, fn);
		if (stat(tmp, &st) < 0 && errno == ENOENT)
			break;
		sleep(2);
	}
	if (i >= attempts) {
		fprintf(stderr,
			"Cannot create unique file name in \"%s/tmp\".\n",
			name);
		return STOP;
	}
	if ((op = Fopen(tmp, "w")) == NULL) {
		fprintf(stderr, "Cannot write to \"%s\".\n", tmp);
		return STOP;
	}
	fseek(fp, off1, SEEK_SET);
	while (size > 0) {
		z = size > sizeof buf ? sizeof buf : size;
		if ((n = fread(buf, 1, z, fp)) != z ||
				fwrite(buf, 1, n, op) != n) {
			fprintf(stderr, "Error writing to \"%s\".\n", tmp);
			Fclose(op);
			unlink(tmp);
			return STOP;
		}
		size -= n;
	}
	Fclose(op);
	new = salloc(n = strlen(name) + strlen(fn) + 6);
	snprintf(new, n, "%s/new/%s", name, fn);
	if (link(tmp, new) < 0) {
		fprintf(stderr, "Cannot link \"%s\" to \"%s\".\n", tmp, new);
		return STOP;
	}
	if (unlink(tmp) < 0)
		fprintf(stderr, "Cannot unlink \"%s\".\n", tmp);
	return OKAY;
}

static enum okay 
trycreate(const char *name)
{
	struct stat	st;

	if (stat(name, &st) == 0) {
		if (!S_ISDIR(st.st_mode)) {
			fprintf(stderr, "\"%s\" is not a directory.\n", name);
			return STOP;
		}
	} else if (makedir(name) != OKAY) {
		fprintf(stderr, "Cannot create directory \"%s\".\n", name);
		return STOP;
	} else
		imap_created_mailbox++;
	return OKAY;
}

static enum okay 
mkmaildir(const char *name)
{
	char	*np;
	size_t	sz;
	enum okay	ok = STOP;

	if (trycreate(name) == OKAY) {
		np = ac_alloc((sz = strlen(name)) + 5);
		strcpy(np, name);
		strcpy(&np[sz], "/tmp");
		if (trycreate(np) == OKAY) {
			strcpy(&np[sz], "/new");
			if (trycreate(np) == OKAY) {
				strcpy(&np[sz], "/cur");
				if (trycreate(np) == OKAY)
					ok = OKAY;
			}
		}
		ac_free(np);
	}
	return ok;
}

static struct message *
mdlook(const char *name, struct message *data)
{
	struct mditem	*md;
	unsigned	c, h, n = 0;

	if (data && data->m_maildir_hash)
		h = ~data->m_maildir_hash;
	else
		h = pjw(name);
	h %= mdprime;
	md = &mdtable[c = h];
	while (md->md_data != NULL) {
		if (strcmp(&md->md_data->m_maildir_file[4], name) == 0)
			break;
		c += n&1 ? -((n+1)/2) * ((n+1)/2) : ((n+1)/2) * ((n+1)/2);
		n++;
		while (c >= mdprime)
			c -= mdprime;
		md = &mdtable[c];
	}
	if (data != NULL && md->md_data == NULL)
		md->md_data = data;
	return md->md_data ? md->md_data : NULL;
}

static void 
mktable(void)
{
	int	i;

	mdprime = nextprime(msgCount);
	mdtable = scalloc(mdprime, sizeof *mdtable);
	for (i = 0; i < msgCount; i++)
		mdlook(&message[i].m_maildir_file[4], &message[i]);
}

static enum okay 
subdir_remove(const char *name, const char *sub)
{
	char	*path;
	int	pathsize, pathend, namelen, sublen, n;
	DIR	*dirfd;
	struct dirent	*dp;

	namelen = strlen(name);
	sublen = strlen(sub);
	path = smalloc(pathsize = namelen + sublen + 30);
	strcpy(path, name);
	path[namelen] = '/';
	strcpy(&path[namelen+1], sub);
	path[namelen+sublen+1] = '/';
	path[pathend = namelen + sublen + 2] = '\0';
	if ((dirfd = opendir(path)) == NULL) {
		perror(path);
		free(path);
		return STOP;
	}
	while ((dp = readdir(dirfd)) != NULL) {
		if (dp->d_name[0] == '.' &&
				(dp->d_name[1] == '\0' ||
				 (dp->d_name[1] == '.' &&
				  dp->d_name[2] == '\0')))
			continue;
		if (dp->d_name[0] == '.')
			continue;
		n = strlen(dp->d_name);
		if (pathend + n + 1 > pathsize)
			path = srealloc(path, pathsize = pathend + n + 30);
		strcpy(&path[pathend], dp->d_name);
		if (unlink(path) < 0) {
			perror(path);
			closedir(dirfd);
			free(path);
			return STOP;
		}
	}
	closedir(dirfd);
	path[pathend] = '\0';
	if (rmdir(path) < 0) {
		perror(path);
		free(path);
		return STOP;
	}
	free(path);
	return OKAY;
}

enum okay 
maildir_remove(const char *name)
{
	if (subdir_remove(name, "tmp") == STOP ||
			subdir_remove(name, "new") == STOP ||
			subdir_remove(name, "cur") == STOP)
		return STOP;
	if (rmdir(name) < 0) {
		perror(name);
		return STOP;
	}
	return OKAY;
}

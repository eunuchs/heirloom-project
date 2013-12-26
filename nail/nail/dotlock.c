/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)dotlock.c	2.9 (gritter) 3/20/06";
#endif
#endif

#include "rcv.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/utsname.h>
#include <errno.h>

#include "extern.h"

#ifndef O_SYNC
#define O_SYNC	0
#endif

static int maildir_access(const char *fname);
static int perhaps_setgid(const char *name, gid_t gid);
static int create_exclusive(const char *fname);

/* Check if we can write a lock file at all */
static int 
maildir_access(const char *fname)
{
	char *path;
	char *p;
	int i;

	path = ac_alloc(strlen(fname) + 2);
	strcpy(path, fname);
	p = strrchr(path, '/');
	if (p != NULL)
		*p = '\0';
	if (p == NULL || *path == '\0')
		strcpy(path, ".");
	i = access(path, R_OK|W_OK|X_OK);
	ac_free(path);
	return i;
}

/*
 * Set the gid if the path is in the normal mail spool
 */
static int
perhaps_setgid(const char *name, gid_t gid)
{
	char safepath[]= MAILSPOOL;

	if (strncmp(name, safepath, sizeof (safepath)-1) ||
			strchr(name + sizeof (safepath), '/'))
		return 0;
	return (setgid (gid));
}


#define	APID_SZ	40	/* sufficient for storign 128 bits pids */
/*
 * Create a unique file. O_EXCL does not really work over NFS so we follow
 * the following trick: [Inspired by  S.R. van den Berg]
 *
 * - make a mostly unique filename and try to create it.
 * - link the unique filename to our target
 * - get the link count of the target
 * - unlink the mostly unique filename
 * - if the link count was 2, then we are ok; else we've failed.
 */
static int 
create_exclusive(const char *fname)
{
	char path[MAXPATHLEN];
	char *hostname;
	char apid[APID_SZ];
	const char *ptr;
	time_t t;
	pid_t pid;
	size_t ntries, cookie;
	int fd, serrno, cc;
	struct stat st;
	struct utsname ut;

	time(&t);
	uname(&ut);
	hostname = ut.nodename;
	pid = getpid();

	cookie = (int)pid ^ (int)t;

	/*
	 * We generate a semi-unique filename, from hostname.(pid ^ usec)
	 */
	if ((ptr = strrchr(fname, '/')) == NULL)
		ptr = fname;
	else
		ptr++;

	snprintf(path, sizeof path, "%.*s.%s.%x", 
	    (int) (ptr - fname), fname, hostname, (unsigned int) cookie);

	/*
	 * We try to create the unique filename.
	 */
	for (ntries = 0; ntries < 5; ntries++) {
		perhaps_setgid(path, effectivegid);
		fd = open(path, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL|O_SYNC, 0);
		setgid(realgid);
		if (fd != -1) {
			snprintf(apid, APID_SZ, "%d", (int)getpid());
			write(fd, apid, strlen(apid));
			close(fd);
			break;
		}
		else if (errno == EEXIST)
			continue;
		else
			return -1;
	}
	/*
	 * We link the path to the name
	 */
	perhaps_setgid(fname, effectivegid);
	cc = link(path, fname);
	setgid(realgid);
   
	if (cc == -1)
		goto bad;

	/*
	 * Note that we stat our own exclusively created name, not the
	 * destination, since the destination can be affected by others.
	 */
	if (stat(path, &st) == -1)
		goto bad;

	perhaps_setgid(fname, effectivegid);
	unlink(path);
	setgid(realgid);

	/*
	 * If the number of links was two (one for the unique file and one
	 * for the lock), we've won the race
	 */
	if (st.st_nlink != 2) {
		errno = EEXIST;
		return -1;
	}
	return 0;

bad:
	serrno = errno;
	unlink(path);
	errno = serrno;
	return -1;
}

int 
fcntl_lock(int fd, int type)
{
	struct flock flp;

	flp.l_type = type;
	flp.l_start = 0;
	flp.l_whence = SEEK_SET;
	flp.l_len = 0;
	return fcntl(fd, F_SETLKW, &flp);
}

int
dot_lock(const char *fname, int fd, int pollinterval, FILE *fp, const char *msg)
#ifdef	notdef
	const char *fname;	/* Pathname to lock */
	int fd;			/* File descriptor for fname, for fcntl lock */
	int pollinterval;	/* Interval to check for lock, -1 return */
	FILE *fp;		/* File to print message */
	const char *msg;	/* Message to print */
#endif
{
	char path[MAXPATHLEN];
	sigset_t nset, oset;
	int i, olderrno;

	if (maildir_access(fname) != 0)
		return 0;

	sigemptyset(&nset);
	sigaddset(&nset, SIGHUP);
	sigaddset(&nset, SIGINT);
	sigaddset(&nset, SIGQUIT);
	sigaddset(&nset, SIGTERM);
	sigaddset(&nset, SIGTTIN);
	sigaddset(&nset, SIGTTOU);
	sigaddset(&nset, SIGTSTP);
	sigaddset(&nset, SIGCHLD);

	snprintf(path, sizeof(path), "%s.lock", fname);

	for (i=0;i<15;i++) {
		sigprocmask(SIG_BLOCK, &nset, &oset);
		if (create_exclusive(path) != -1) {
			sigprocmask(SIG_SETMASK, &oset, NULL);
			return 0;
		}
		else {
			olderrno = errno;
			sigprocmask(SIG_SETMASK, &oset, NULL);
		}

		fcntl_lock(fd, F_UNLCK);
		if (olderrno != EEXIST)
			return -1;

		if (fp && msg)
		    fputs(msg, fp);

		if (pollinterval) {
			if (pollinterval == -1) {
				errno = EEXIST;
				return -1;
			}
			sleep(pollinterval);
		}
		fcntl_lock(fd, F_WRLCK);
	}
        fprintf(stderr, catgets(catd, CATSET, 71,
		"%s seems a stale lock? Need to be removed by hand?\n"), path);
        return -1;
}

void 
dot_unlock(const char *fname)
{
	char path[MAXPATHLEN];

	if (maildir_access(fname) != 0)
		return;

	snprintf(path, sizeof(path), "%s.lock", fname);
	perhaps_setgid(path, effectivegid);
	unlink(path);
	setgid(realgid);
}

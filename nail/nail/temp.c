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
static char sccsid[] = "@(#)temp.c	2.8 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Mail -- a mail program
 *
 * Temporary file handling.
 */

static char	*tmpdir;

/*
 * Create a temporary file in tmpdir, use prefix for its name,
 * store the unique name in fn, and return a stdio FILE pointer
 * with access mode.
 * The permissions for the newly created file are given in bits.
 */
FILE *
Ftemp(char **fn, char *prefix, char *mode, int bits, int register_file)
{
	FILE *fp;
	int fd;

	*fn = smalloc(strlen(tmpdir) + strlen(prefix) + 8);
	strcpy(*fn, tmpdir);
	strcat(*fn, "/");
	strcat(*fn, prefix);
	strcat(*fn, "XXXXXX");
#ifdef	HAVE_MKSTEMP
	if ((fd = mkstemp(*fn)) < 0)
		goto Ftemperr;
	if (fchmod(fd, bits) < 0)
		goto Ftemperr;
#else	/* !HAVE_MKSTEMP */
	if (mktemp(*fn) == NULL)
		goto Ftemperr;
	if ((fd = open(*fn, O_CREAT|O_EXCL|O_RDWR, bits)) < 0)
		goto Ftemperr;
#endif	/* !HAVE_MKSTEMP */
	if (register_file)
		fp = Fdopen(fd, mode);
	else {
		fp = fdopen(fd, mode);
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	return fp;
Ftemperr:
	Ftfree(fn);
	return NULL;
}

/*
 * Free the resources associated with the given filename. To be
 * called after unlink().
 * Since this function can be called after receiving a signal,
 * the variable must be made NULL first and then free()d, to avoid
 * more than one free() call in all circumstances.
 */
void
Ftfree(char **fn)
{
	char *cp = *fn;

	*fn = NULL;
	free(cp);
}

void 
tinit(void)
{
	char *cp;

	if ((cp = getenv("TMPDIR")) != NULL) {
		tmpdir = smalloc(strlen(cp) + 1);
		strcpy(tmpdir, cp);
	} else {
		tmpdir = "/tmp";
	}
	if (myname != NULL) {
		if (getuserid(myname) < 0) {
			printf(catgets(catd, CATSET, 198,
				"\"%s\" is not a user of this system\n"),
			    myname);
			exit(1);
		}
	} else {
		if ((cp = username()) == NULL) {
			myname = "nobody";
			if (rcvmode)
				exit(1);
		} else {
			myname = smalloc(strlen(cp) + 1);
			strcpy(myname, cp);
		}
	}
	if ((cp = getenv("HOME")) == NULL)
		cp = ".";
	homedir = smalloc(strlen(cp) + 1);
	strcpy(homedir, cp);
	if (debug || value("debug"))
		printf(catgets(catd, CATSET, 199,
			"user = %s, homedir = %s\n"), myname, homedir);
}

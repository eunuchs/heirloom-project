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
static char sccsid[] = "@(#)quit.c	2.30 (gritter) 11/11/08";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

static int writeback(FILE *res, FILE *obuf);
static void edstop(void);

/*
 * The "quit" command.
 */
/*ARGSUSED*/
int 
quitcmd(void *v)
{
	/*
	 * If we are sourcing, then return 1 so execute() can handle it.
	 * Otherwise, return -1 to abort command loop.
	 */
	if (sourcing)
		return 1;
	return -1;
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  On any error, just return -1.  Else return 0.
 * Incorporate the any new mail that we found.
 */
static int
writeback(FILE *res, FILE *obuf)
{
	struct message *mp;
	int p, c;

	p = 0;
	fseek(obuf, 0L, SEEK_SET);
#ifndef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			putc(c, obuf);
#endif
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			p++;
			if (send(mp, obuf, NULL, NULL, SEND_MBOX, NULL) < 0) {
				perror(mailname);
				fseek(obuf, 0L, SEEK_SET);
				return(-1);
			}
		}
#ifdef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			putc(c, obuf);
#endif
	fflush(obuf);
	trunc(obuf);
	if (ferror(obuf)) {
		perror(mailname);
		fseek(obuf, 0L, SEEK_SET);
		return(-1);
	}
	if (res != NULL)
		Fclose(res);
	fseek(obuf, 0L, SEEK_SET);
	alter(mailname);
	if (p == 1)
		printf(catgets(catd, CATSET, 155,
				"Held 1 message in %s\n"), mailname);
	else
		printf(catgets(catd, CATSET, 156,
				"Held %d messages in %s\n"), p, mailname);
	return(0);
}

/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */
void 
quit(void)
{
	int p, modify, anystat;
	FILE *fbuf, *rbuf, *readstat = NULL, *abuf;
	struct message *mp;
	int c;
	char *tempResid;
	struct stat minfo;

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly. IMAP can set some
	 * flags (e.g. "\\Seen") so imap_quit must be
	 * called even then.
	 */
	if (mb.mb_perm == 0 && mb.mb_type != MB_IMAP)
		return;
	switch (mb.mb_type) {
	case MB_FILE:
		break;
	case MB_MAILDIR:
		maildir_quit();
		return;
	case MB_POP3:
		pop3_quit();
		return;
	case MB_IMAP:
	case MB_CACHE:
		imap_quit();
		return;
	case MB_VOID:
		return;
	}
	/*
	 * If editing (not reading system mail box), then do the work
	 * in edstop()
	 */
	if (edit) {
		edstop();
		return;
	}

	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 */

	fbuf = Zopen(mailname, "r+", &mb.mb_compressed);
	if (fbuf == NULL) {
		if (errno == ENOENT)
			return;
		goto newmail;
	}
	if (fcntl_lock(fileno(fbuf), F_WRLCK) == -1) {
nolock:
		perror(catgets(catd, CATSET, 157, "Unable to lock mailbox"));
		Fclose(fbuf);
		return;
	}
	if (dot_lock(mailname, fileno(fbuf), 1, stdout, ".") == -1)
		goto nolock;
	rbuf = NULL;
	if (fstat(fileno(fbuf), &minfo) >= 0 && minfo.st_size > mailsize) {
		printf(catgets(catd, CATSET, 158, "New mail has arrived.\n"));
		rbuf = Ftemp(&tempResid, "Rq", "w", 0600, 1);
		if (rbuf == NULL || fbuf == NULL)
			goto newmail;
#ifdef APPEND
		fseek(fbuf, (long)mailsize, SEEK_SET);
		while ((c = getc(fbuf)) != EOF)
			putc(c, rbuf);
#else
		p = minfo.st_size - mailsize;
		while (p-- > 0) {
			c = getc(fbuf);
			if (c == EOF)
				goto newmail;
			putc(c, rbuf);
		}
#endif
		Fclose(rbuf);
		if ((rbuf = Fopen(tempResid, "r")) == NULL)
			goto newmail;
		rm(tempResid);
		Ftfree(&tempResid);
	}

	anystat = holdbits();
	modify = 0;
	if (Tflag != NULL) {
		if ((readstat = Zopen(Tflag, "w", NULL)) == NULL)
			Tflag = NULL;
	}
	for (c = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (readstat != NULL && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("message-id", mp)) != NULL ||
					(id = hfield("article-id", mp)) != NULL)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (readstat != NULL)
		Fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			printf(catgets(catd, CATSET, 155,
				"Held 1 message in %s\n"), mailname);
		else if (p > 1)
			printf(catgets(catd, CATSET, 156,
				"Held %d messages in %s\n"), p, mailname);
		Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	if (c == 0) {
		if (p != 0) {
			writeback(rbuf, fbuf);
			Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		goto cream;
	}

	if (makembox() == STOP) {
		Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */

	if (p != 0) {
		writeback(rbuf, fbuf);
		Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}

	/*
	 * Finally, remove his /usr/mail file.
	 * If new mail has arrived, copy it back.
	 */

cream:
	if (rbuf != NULL) {
		abuf = fbuf;
		fseek(abuf, 0L, SEEK_SET);
		while ((c = getc(rbuf)) != EOF)
			putc(c, abuf);
		Fclose(rbuf);
		trunc(abuf);
		alter(mailname);
		Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	demail();
	Fclose(fbuf);
	dot_unlock(mailname);
	return;

newmail:
	printf(catgets(catd, CATSET, 166, "Thou hast new mail.\n"));
	if (fbuf != NULL) {
		Fclose(fbuf);
		dot_unlock(mailname);
	}
}

/*
 * Adjust the message flags in each message.
 */
int 
holdbits(void)
{
	struct message *mp;
	int anystat, autohold, holdbit, nohold;

	anystat = 0;
	autohold = value("hold") != NULL;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NULL)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MSTATUS|MFLAG|MUNFLAG|MANSWER|MUNANSWER|
					MDRAFT|MUNDRAFT))
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	return anystat;
}

/*
 * Create another temporary file and copy user's mbox file
 * darin.  If there is no mbox, copy nothing.
 * If he has specified "append" don't copy his mailbox,
 * just copy saveable entries at the end.
 */

enum okay 
makembox(void)
{
	struct message *mp;
	char *mbox, *tempQuit;
	int mcount, c;
	FILE *ibuf = NULL, *obuf, *abuf;
	enum protocol	prot;

	mbox = mboxname;
	mcount = 0;
	if (value("append") == NULL) {
		if ((obuf = Ftemp(&tempQuit, "Rm", "w", 0600, 1)) == NULL) {
			perror(catgets(catd, CATSET, 162,
					"temporary mail quit file"));
			return STOP;
		}
		if ((ibuf = Fopen(tempQuit, "r")) == NULL) {
			perror(tempQuit);
			rm(tempQuit);
			Ftfree(&tempQuit);
			Fclose(obuf);
			return STOP;
		}
		rm(tempQuit);
		Ftfree(&tempQuit);
		if ((abuf = Zopen(mbox, "r", NULL)) != NULL) {
			while ((c = getc(abuf)) != EOF)
				putc(c, obuf);
			Fclose(abuf);
		}
		if (ferror(obuf)) {
			perror(catgets(catd, CATSET, 163,
					"temporary mail quit file"));
			Fclose(ibuf);
			Fclose(obuf);
			return STOP;
		}
		Fclose(obuf);
		close(creat(mbox, 0600));
		if ((obuf = Zopen(mbox, "r+", NULL)) == NULL) {
			perror(mbox);
			Fclose(ibuf);
			return STOP;
		}
	}
	else {
		if ((obuf = Zopen(mbox, "a", NULL)) == NULL) {
			perror(mbox);
			return STOP;
		}
		fchmod(fileno(obuf), 0600);
	}
	prot = which_protocol(mbox);
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX) {
			mcount++;
			if (prot == PROTO_IMAP &&
					saveignore[0].i_count == 0 &&
					saveignore[1].i_count == 0 &&
					imap_thisaccount(mbox)) {
				if (imap_copy(mp, mp-message+1, mbox) == STOP)
					goto err;
			} else if (send(mp, obuf, saveignore,
						NULL, SEND_MBOX, NULL) < 0) {
				perror(mbox);
			err:	if (ibuf)
					Fclose(ibuf);
				Fclose(obuf);
				return STOP;
			}
			mp->m_flag |= MBOXED;
		}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (value("append") == NULL) {
		rewind(ibuf);
		c = getc(ibuf);
		while (c != EOF) {
			putc(c, obuf);
			if (ferror(obuf))
				break;
			c = getc(ibuf);
		}
		Fclose(ibuf);
		fflush(obuf);
	}
	trunc(obuf);
	if (ferror(obuf)) {
		perror(mbox);
		Fclose(obuf);
		return STOP;
	}
	if (Fclose(obuf) != 0) {
		if (prot != PROTO_IMAP)
			perror(mbox);
		return STOP;
	}
	if (mcount == 1)
		printf(catgets(catd, CATSET, 164, "Saved 1 message in mbox\n"));
	else
		printf(catgets(catd, CATSET, 165,
				"Saved %d messages in mbox\n"), mcount);
	return OKAY;
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
static void 
edstop(void)
{
	int gotcha, c;
	struct message *mp;
	FILE *obuf, *ibuf = NULL, *readstat = NULL;
	struct stat statb;

	if (mb.mb_perm == 0)
		return;
	holdsigs();
	if (Tflag != NULL) {
		if ((readstat = Zopen(Tflag, "w", NULL)) == NULL)
			Tflag = NULL;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS|MFLAG|MUNFLAG|
					MANSWER|MUNANSWER|MDRAFT|MUNDRAFT))
			gotcha++;
		if (readstat != NULL && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("message-id", mp)) != NULL ||
					(id = hfield("article-id", mp)) != NULL)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (readstat != NULL)
		Fclose(readstat);
	if (!gotcha || Tflag != NULL)
		goto done;
	ibuf = NULL;
	if (stat(mailname, &statb) >= 0 && statb.st_size > mailsize) {
		char *tempname;

		if ((obuf = Ftemp(&tempname, "mbox.", "w", 0600, 1)) == NULL) {
			perror(catgets(catd, CATSET, 167, "tmpfile"));
			relsesigs();
			reset(0);
		}
		if ((ibuf = Zopen(mailname, "r", &mb.mb_compressed)) == NULL) {
			perror(mailname);
			Fclose(obuf);
			rm(tempname);
			Ftfree(&tempname);
			relsesigs();
			reset(0);
		}
		fseek(ibuf, (long)mailsize, SEEK_SET);
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		Fclose(ibuf);
		Fclose(obuf);
		if ((ibuf = Fopen(tempname, "r")) == NULL) {
			perror(tempname);
			rm(tempname);
			Ftfree(&tempname);
			relsesigs();
			reset(0);
		}
		rm(tempname);
		Ftfree(&tempname);
	}
	printf(catgets(catd, CATSET, 168, "\"%s\" "), mailname);
	fflush(stdout);
	if ((obuf = Zopen(mailname, "r+", &mb.mb_compressed)) == NULL) {
		perror(mailname);
		relsesigs();
		reset(0);
	}
	trunc(obuf);
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (send(mp, obuf, NULL, NULL, SEND_MBOX, NULL) < 0) {
			perror(mailname);
			relsesigs();
			reset(0);
		}
	}
	gotcha = (c == 0 && ibuf == NULL);
	if (ibuf != NULL) {
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		Fclose(ibuf);
	}
	fflush(obuf);
	if (ferror(obuf)) {
		perror(mailname);
		relsesigs();
		reset(0);
	}
	Fclose(obuf);
	if (gotcha && value("emptybox") == NULL) {
		rm(mailname);
		printf(value("bsdcompat") || value("bsdmsgs") ?
				catgets(catd, CATSET, 169, "removed\n") :
				catgets(catd, CATSET, 211, "removed.\n"));
	} else
		printf(value("bsdcompat") || value("bsdmsgs") ?
				catgets(catd, CATSET, 170, "complete\n") :
				catgets(catd, CATSET, 212, "updated.\n"));
	fflush(stdout);

done:
	relsesigs();
}

enum quitflags {
	QUITFLAG_HOLD      = 001,
	QUITFLAG_KEEPSAVE  = 002,
	QUITFLAG_APPEND    = 004,
	QUITFLAG_EMPTYBOX  = 010
};

static const struct quitnames {
	enum quitflags	flag;
	const char	*name;
} quitnames[] = {
	{ QUITFLAG_HOLD,	"hold" },
	{ QUITFLAG_KEEPSAVE,	"keepsave" },
	{ QUITFLAG_APPEND,	"append" },
	{ QUITFLAG_EMPTYBOX,	"emptybox" },
	{ 0,			NULL }
};

int
savequitflags(void)
{
	enum quitflags	qf = 0;
	int	i;

	for (i = 0; quitnames[i].name; i++)
		if (value(quitnames[i].name))
			qf |= quitnames[i].flag;
	return qf;
}

void
restorequitflags(int qf)
{
	int	i;

	for (i = 0; quitnames[i].name; i++)
		if (qf & quitnames[i].flag) {
			if (value(quitnames[i].name) == NULL)
				assign(quitnames[i].name, "");
		} else if (value(quitnames[i].name))
			unset_internal(quitnames[i].name);
}

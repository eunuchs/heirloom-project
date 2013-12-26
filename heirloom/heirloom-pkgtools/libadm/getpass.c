/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "getpass.c	1.24	06/04/29 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)getpass.c	1.4 (gritter) 2/25/07
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stropts.h>
#include <termio.h>

static void catch(int);
static int intrupt;
static char *__getpass(const char *, int);

#define	MAXPASSWD	256	/* max significant characters in password */
#define	SMLPASSWD	8	/* unix standard  characters in password */


char *
getpass(const char *prompt)
{
	return ((char *)__getpass(prompt, SMLPASSWD));
}

char *
getpassphrase(const char *prompt)
{
	return ((char *)__getpass(prompt, MAXPASSWD));
}

static char *
__getpass(const char *prompt, int size)
{
	struct termio ttyb;
	unsigned short flags;
	char *p;
	int c;
	FILE	*fi;
#ifdef	__sun
	char *pbuf = tsdalloc(_T_GETPASS, MAXPASSWD + 1, NULL);
#else
	char *pbuf = malloc(MAXPASSWD+1);
#endif
	void	(*sig)(int);
#ifdef	__sun
	rmutex_t *lk;
#endif

	if (pbuf == NULL ||
	    (fi = fopen("/dev/tty", "rF")) == NULL)
		return (NULL);
	setbuf(fi, NULL);
	sig = signal(SIGINT, catch);
	intrupt = 0;
	(void) ioctl(fileno(fi), TCGETA, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	(void) ioctl(fileno(fi), TCSETAF, &ttyb);
#ifdef	__sun
	FLOCKFILE(lk, stderr);
#else
	flockfile(stderr);
#endif
	(void) fputs(prompt, stderr);
	p = pbuf;
	while (!intrupt &&
		(c = getc(fi)) != '\n' && c != '\r' && c != EOF) {
		if (p < &pbuf[ size ])
			*p++ = (char)c;
	}
	*p = '\0';
	ttyb.c_lflag = flags;
	(void) ioctl(fileno(fi), TCSETAW, &ttyb);
	(void) putc('\n', stderr);
#ifdef	__sun
	FUNLOCKFILE(lk);
#else
	funlockfile(stderr);
#endif
	(void) signal(SIGINT, sig);
	(void) fclose(fi);
	if (intrupt)
		(void) kill(getpid(), SIGINT);
	return (pbuf);
}

/* ARGSUSED */
static void
catch(int x)
{
	intrupt = 1;
}

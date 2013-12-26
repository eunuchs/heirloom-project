/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)bltin.c	1.11 (gritter) 7/3/05
 */
/* from OpenSolaris "bltin.c	1.14	05/06/08 SMI"	 SVr4.0 1.3.8.1 */
/*
 *
 * UNIX shell
 *
 */


#include	"defs.h"
#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/times.h>
#include	<dirent.h>

#define	setmode	sh_setmode

#ifdef	SPELL
static int	spellcheck(unsigned char *);
#endif

void
builtin(int type, int argc, unsigned char **argv, struct trenod *t)
{
	short index = initio(t->treio, (type != SYSEXEC));
	unsigned char *a1 = argv[1];

	switch (type)		
	{

	case SYSSUSP:
		syssusp(argc,(char **)argv);
		break;

	case SYSSTOP:
		sysstop(argc,(char **)argv);
		break;

	case SYSKILL:
		syskill(argc,(char **)argv);
		break;

	case SYSFGBG:
		sysfgbg(argc,(char **)argv);
		break;

	case SYSJOBS:
		sysjobs(argc,(char **)argv);
		break;

	case SYSDOT:
		if (a1)
		{
			register int	f;

			if ((f = pathopen(getpath(a1), a1)) < 0)
				failed(a1, notfound);
			else
				execexp(0, f);
		}
		break;

	case SYSTIMES:
		{
			struct tms tms;

			times(&tms);
			prt(tms.tms_cutime);
			prc_buff(SPACE);
			prt(tms.tms_cstime);
			prc_buff(NL);
		}
		break;

	case SYSEXIT:
		if ( tried_to_exit++ || endjobs(JOB_STOPPED) ){
			flags |= forcexit;	/* force exit */
			exitsh(a1 ? stoi(a1) : retval);
		}
		break;

	case SYSNULL:
		t->treio = 0;
		break;

	case SYSCONT:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
			else
				breakcnt = -breakcnt;
		}
		break;

	case SYSBREAK:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
		}
		break;

	case SYSTRAP:
		systrap(argc,(char **)argv);
		break;

	case SYSEXEC:
		argv++;
		ioset = 0;
		if (a1 == 0) {
			setmode(0);
			break;
		}
		/* FALLTHROUGH */

#ifdef RES	/* Research includes login as part of the shell */

	case SYSLOGIN:
		if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		oldsigs();
		execa(argv, -1);
		done(0);
#else

	case SYSNEWGRP:
		if (flags & rshflg)
			failed(argv[0], restricted);
		else if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		else
		{
			flags |= forcexit; /* bad exec will terminate shell */
			oldsigs();
			rmtemp(0);
			rmfunctmp();
#ifdef ACCT
			doacct();
#endif
			execa(argv, -1);
			done(0);
		}

#endif

	case SYSCD:
		if (flags & rshflg)
			failed(argv[0], restricted);
		else if ((a1 && *a1) || (a1 == 0 && (a1 = homenod.namval)))
		{
			unsigned char *cdpath;
			unsigned char *dir;
			int f;

			if ((cdpath = cdpnod.namval) == 0 ||
			     *a1 == '/' ||
			     cf(a1, ".") == 0 ||
			     cf(a1, "..") == 0 ||
			     (*a1 == '.' && (*(a1+1) == '/' || *(a1+1) == '.' && *(a1+2) == '/')))
				cdpath = (unsigned char *)nullstr;

			do
			{
				dir = cdpath;
				cdpath = catpath(cdpath,a1);
			}
			while ((f = (chdir((const char *) curstak()) < 0)) &&
			    cdpath);

#ifdef	SPELL
			if (flags & ttyflg && f && errno == ENOENT) {
				int	saverrno = errno;
				if (spellcheck(a1)) {
					int	c, d;
					prs_buff("cd ");
					prs_buff(curstak());
					prs_buff("? ");
					flushb();
					c = readwc();
					if (c != NL && c != EOF)
						while ((d = readwc()) != NL &&
								d != EOF);
					if (c != 'n' && c != 'N') {
						a1 = curstak();
						f = chdir((const char *)a1) < 0;
						if (f == 0)
							prs("ok\n");
					} else
						errno = saverrno;
				} else
					errno = saverrno;
			}
#endif	/* SPELL */

			if (f) {
				switch(errno) {
#ifdef	EMULTIHOP
						case EMULTIHOP:
							failed(a1, emultihop);
							break;
#endif
						case ENOTDIR:
							failed(a1, enotdir);
							break;
						case ENOENT:
							failed(a1, enoent);
							break;
						case EACCES:
							failed(a1, eacces);
							break;
#ifdef	ENOLINK
						case ENOLINK:
							failed(a1, enolink);
							break;
#endif
						default: 
						failed(a1, baddir);
						break;
						}
			}
			else 
			{
				cwd(curstak());
				if (cf(nullstr, dir) &&
				    *dir != ':' &&
					any('/', curstak()) &&
					flags & prompt)
				{
					prs_buff(cwdget());
					prc_buff(NL);
				}
			}
			zapcd();
		}
		else 
		{
			if (a1)
				error(nulldir);
			else
				error(nohome);
		}

		break;

	case SYSSHFT:
		{
			int places;

			places = a1 ? stoi(a1) : 1;

			if ((dolc -= places) < 0)
			{
				dolc = 0;
				error(badshift);
			}
			else
				dolv += places;
		}			

		break;

	case SYSWAIT:
		syswait(argc,(char **)argv);
		break;

	case SYSREAD:
		if(argc < 2)
			failed(argv[0],mssgargn);
		rwait = 1;
		exitval = readvar(&argv[1]);
		rwait = 0;
		break;

	case SYSSET:
		if (a1)
		{
			int	cnt;

			cnt = options(argc, argv);
			if (cnt > 1)
				setargs(argv + argc - cnt);
		}
		else if (comptr(t)->comset == 0)
		{
			/*
			 * scan name chain and print
			 */
			namscan(printnam);
		}
		break;

	case SYSRDONLY:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				attrib(lookup(*argv), N_RDONLY);
		}
		else
			namscan(printro);

		break;

	case SYSXPORT:
		{
			struct namnod 	*n;

			exitval = 0;
			if (a1)
			{
				while (*++argv)
				{
					n = lookup(*argv);
					if (n->namflg & N_FUNCTN)
						error(badexport);
					else
						attrib(n, N_EXPORT);
				}
			}
			else
				namscan(printexp);
		}
		break;

	case SYSEVAL:
		if (a1)
			execexp(a1, (intptr_t)&argv[2]);
		break;

#ifndef RES	
	case SYSULIMIT:
		sysulimit(argc, (char **)argv);
		break;
			
	case SYSUMASK:
		sysumask(argc, (char **)argv);
		break;
#endif

	case SYSTST:
		exitval = test(argc, argv);
		break;

	case SYSECHO:
		exitval = echo(argc, argv);
		break;

	case SYSHASH:
		exitval = 0;

		if (a1)
		{
			if (a1[0] == '-')
			{
				if (a1[1] == 'r')
					zaphash();
				else
					error(badopt);
			}
			else
			{
				while (*++argv)
				{
					if (hashtype(hash_cmd(*argv)) == NOTFOUND)
						failed(*argv, notfound);
				}
			}
		}
		else
			hashpr();

		break;

	case SYSPWD:
		{
			exitval = 0;
			cwdprint();
		}
		break;

	case SYSRETURN:
		if (funcnt == 0)
			error(badreturn);

		execbrk = 1;
		exitval = (a1 ? stoi(a1) : retval);
		break;
	
	case SYSTYPE:
		exitval = 0;
		if (a1)
		{
			/* return success only if all names are found */
			while (*++argv)
				exitval |= what_is_path(*argv);
		}
		break;

	case SYSUNS:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				unset_name(*argv);
		}
		break;

	case SYSGETOPT: {
		int getoptval;
		struct namnod *n;
		unsigned char *varnam = argv[2];
		unsigned char c[2];
		if(argc < 3) {
			failure(argv[0],mssgargn);
			break;
		}
		exitval = 0;
		n = lookup("OPTIND");
		optind = stoi(n->namval);
		if(argc > 3) {
			argv[2] = dolv[0];
			getoptval = getopt(argc-2, (char **)&argv[2], (char *)argv[1]);
		}
		else
			getoptval = getopt(dolc+1, (char **)dolv, (char *)argv[1]);
		if(getoptval == -1) {
			itos(optind);
			assign(n, numbuf);
			n = lookup(varnam);
			assign(n, nullstr);
			exitval = 1;
			break;
		}
		argv[2] = varnam;
		itos(optind);
		assign(n, numbuf);
		c[0] = getoptval;
		c[1] = 0;
		n = lookup(varnam);
		assign(n, c);
		n = lookup("OPTARG");
		assign(n, optarg);
		}
		break;

	default:
		prs_buff("unknown builtin\n");
	}


	flushb();
	restore(index);
	chktrap();
}

#ifdef	SPELL

#define	s0(c)	((c) == '/' ? '\0' : (c))

/*
 * Compare two directory names for spell checking. newpath is the best current
 * path collected so far. newcur is the best previous directory, newcand
 * is the candidate. old is the current part of the wrong path.
 */
static int
better(unsigned char *newpath, unsigned char *newcur,
		unsigned char *newcand, unsigned char *old)
{
	unsigned char	save[PATH_MAX+1];	/* can allocate only one path */
	unsigned char	*sp = save;		/* on the stack at a time */
	unsigned char	*np, *cp;
	unsigned char	*op = old;
	struct stat	st;
	int		val = 0, typo = 0, miss = 0;

	for (np = newcur; *np; np++) {
		if (sp >= &save[sizeof save - 2])
			return -1;
		*sp++ = *np;
	}
	*sp = '\0';
	cp = newcur;
	for (np = newcand; *np; np++) {
		if (cp+1 >= brkend)
			growstak(cp);
		*cp++ = *np;
	}
	*cp = '\0';
	if (stat(newpath, &st) < 0 || (st.st_mode&S_IFMT) != S_IFDIR ||
			access(newpath, X_OK) != 0)
		goto restore;
	np = newcand;
	do {
		if (*np != s0(*op)) {
			if (np[0] && s0(op[0]) && np[1] == s0(op[1]))
				typo++;
			else if (s0(op[0]) && np[0] == s0(op[1])) {
				op++;
				miss++;
			} else if (np[0] && np[1] == s0(op[0])) {
				np++;
				miss++;
			} else
				goto restore;
		}
	} while (*np++ && s0(*op) && op++);
	if (np[-1] == s0(*op)) {
		val = miss == 1 ? 1 :
			typo == 1 && miss == 0 ? 2 :
			miss == 0 && typo == 0 ? 3 : 0;
	}
restore:
	np = newcur;
	for (sp = save; *sp; sp++)
		*np++ = *sp;
	*np = '\0';
	return val;
}

static int
spellcheck(unsigned char *old)
{
	unsigned char	*new = locstak();
	unsigned char	*op = old, *oc, *np = new, *nc;
	const char	*cp;
	DIR		*dir;
	struct dirent	*dp;
	int		best, i, c;
	struct stat	st;

	if (*op == '/') {
		if (np+1 > brkend)
			growstak(np);
		*np++ = '/';
		*np = '\0';
		while (*op == '/')
			op++;
	}
	do {
		oc = op;
		nc = np;
		while (*op && *op != '/') {
			if (np >= brkend)
				growstak(np);
			*np++ = *op++;
		}
		while (*op == '/')
			op++;
		if (np >= brkend)
			growstak(np);
		*np = '\0';
		if (stat(new, &st) == 0 && (st.st_mode&S_IFMT) == S_IFDIR &&
				access(new, X_OK) == 0)
			goto next;
		c = *nc;
		*nc = '\0';
		if ((dir = opendir(nc>new?new:(unsigned char *)".")) == NULL)
			return 0;
		*nc = c;
		best = 0;
		while ((dp = readdir(dir)) != NULL) {
			if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
						dp->d_name[1] == '.' &&
						dp->d_name[2] == '\0'))
				continue;
			if ((i = better(new, nc, dp->d_name, oc)) > best) {
				best = i;
				np = nc;
				for (cp = dp->d_name; *cp; cp++) {
					if (np+1 > brkend)
						growstak(np);
					*np++ = *cp & 0377;
				}
				*np = '\0';
			}
		}
		closedir(dir);
		if (best == 0)
			return 0;
	next:	if (np+1 > brkend)
			growstak(np);
		*np++ = '/';
		*np = '\0';
	} while (*op);
	if (np > new)
		np[-1] = '\0';
	return np > new && *new;
}
#endif	/* SPELL */

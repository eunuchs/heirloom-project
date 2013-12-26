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
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)defs.h	1.21 (gritter) 7/3/05
 */

#ifndef	_DEFS_H
#define	_DEFS_H

/* from OpenSolaris "defs.h	1.25	05/06/08 SMI" */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 *	UNIX shell
 */

/* execute flags */
#define		XEC_EXECED	01
#define			XEC_LINKED	02
#define		XEC_NOSTOP	04
#define		XEC_PIPED	010

/* endjobs flags */
#define			JOB_STOPPED	01
#define			JOB_RUNNING	02

/* error exits from various parts of shell */
#define		ERROR		1
#define		SYNBAD		2
#define		SIGFAIL 	2000
#define		SIGFLG		0200

/* command tree */
#define		FPIN		0x0100
#define		FPOU		0x0200
#define		FAMP		0x0400
#define		COMMSK		0x00F0
#define			CNTMSK		0x000F

#define		TCOM		0x0000
#define		TPAR		0x0010
#define		TFIL		0x0020
#define		TLST		0x0030
#define		TIF			0x0040
#define		TWH			0x0050
#define		TUN			0x0060
#define		TSW			0x0070
#define		TAND		0x0080
#define		TORF		0x0090
#define		TFORK		0x00A0
#define		TFOR		0x00B0
#define			TFND		0x00C0

/* execute table */
#define		SYSSET		1
#define		SYSCD		2
#define		SYSEXEC		3

#ifdef RES	/*	include login code	*/
#define		SYSLOGIN	4
#else
#define		SYSNEWGRP 	4
#endif

#define		SYSTRAP		5
#define		SYSEXIT		6
#define		SYSSHFT 	7
#define		SYSWAIT		8
#define		SYSCONT 	9
#define		SYSBREAK	10
#define		SYSEVAL 	11
#define		SYSDOT		12
#define		SYSRDONLY 	13
#define		SYSTIMES 	14
#define		SYSXPORT	15
#define		SYSNULL 	16
#define		SYSREAD 	17
#define			SYSTST		18

#ifndef RES	/*	exclude umask code	*/
#define		SYSUMASK 	20
#define		SYSULIMIT 	21
#endif

#define		SYSECHO		22
#define			SYSHASH		23
#define			SYSPWD		24
#define		SYSRETURN	25
#define			SYSUNS		26
#define			SYSMEM		27
#define			SYSTYPE  	28
#define			SYSGETOPT	29
#define		SYSJOBS		30
#define		SYSFGBG		31
#define		SYSKILL		32
#define		SYSSUSP		33
#define		SYSSTOP		34

/* used for input and output of shell */
#define		INIO 		19

/* io nodes */
#define		USERIO		10
#define		IOUFD		15
#define		IODOC		16
#define		IOPUT		32
#define		IOAPP		64
#define		IOMOV		128
#define		IORDW		256
#define			IOSTRIP		512
#define		INPIPE		0
#define		OTPIPE		1

/* arg list terminator */
#define		ENDARGS		0

/* enable shell accounting */
#define		ACCT

#include	<unistd.h>
#include	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	<signal.h>
#include	<string.h>
#include	<sys/types.h>
#include	<inttypes.h>

/* Multibyte characters */
#include <stdlib.h>
#include <limits.h>
#define	MULTI_BYTE_MAX MB_LEN_MAX
extern int	mb_cur_max;
#define	nextc(wc, sp)	(mb_cur_max > 1 && *(sp) & 0200 ? \
				mbtowc(wc, sp, mb_cur_max) : \
				(*(wc) = *(sp) & 0377, *(wc) != 0))
#define	putb(mb, wc)	(mb_cur_max > 1 && wc & ~(wchar_t)0177 ? \
				wctomb(mb, wc) : \
				(*(mb) = wc, 1))

/* id's */
extern pid_t	mypid;
extern pid_t	mypgid;
extern pid_t	mysid;

/* getopt */

extern int		optind;
extern int		opterr;
extern int 		getopt_sp;
extern char 		*optarg;

#define	free	sh_free

/* result type declarations */
/* args.c */
int options(int, unsigned char **);
void setargs(unsigned char *[]);
struct dolnod *freeargs(struct dolnod *);
struct dolnod *clean_args(struct dolnod *);
void clearup(void);
struct dolnod *savargs(int);
void restorargs(struct dolnod *, int);
struct dolnod *useargs(void);
/* blok.c */
void *alloc(size_t);
void addblok(unsigned);
void sh_free(void *);
size_t blklen(char *);
/* bltin.c */
void builtin(int, int, unsigned char **, struct trenod *);
/* cmd.c */
unsigned char *getstor(int);
struct trenod *makefork(int, struct trenod *);
struct trenod *cmd(int, int);
/* ctype.c */
/* defs.c */
/* echo.c */
int echo(int, unsigned char **);
/* error.c */
void failed(const unsigned char *, const unsigned char *);
void error(const unsigned char *);
void exitsh(int);
void rmtemp(struct ionod *);
void rmfunctmp(void);
void failure(const unsigned char *, const unsigned char *);
/* expand.c */
int expand(unsigned char *, int);
void makearg(struct argnod *);
/* fault.c */
void done(int);
void fault(int);
int handle(int, void (*)(int));
void stdsigs(void);
void oldsigs(void);
void chktrap(void);
int systrap(int, char **);
#define	sleep(a)	sh_sleep(a)
void sleep(unsigned int);
void sigsegv(int, siginfo_t *);
void init_sigval(void);
/* func.c */
void freefunc(struct namnod *);
void freetree(struct trenod *);
void free_arg(struct argnod *);
void freeio(struct ionod *);
void freereg(struct regnod *);
void prbgnlst(void);
void prendlst(void);
void prcmd(struct trenod *);
void prf(struct trenod *);
void prarg(struct argnod *);
void prio(struct ionod *);
/* gmatch.c */
int gmatch(const char *, const char *);
/* hashserv.c */
short pathlook(unsigned char *, int, struct argnod *);
void zaphash(void);
void zapcd(void);
void hashpr(void);
void set_dotpath(void);
void hash_func(unsigned char *);
void func_unhash(unsigned char *);
short hash_cmd(unsigned char *);
int what_is_path(unsigned char *);
int findpath(unsigned char *, int);
int chk_access(unsigned char *, mode_t, int);
void pr_path(unsigned char *, int);
/* io.c */
void initf(int);
int estabf(unsigned char *);
void push(struct fileblk *);
int pop(void);
void pushtemp(int, struct tempblk *);
int poptemp(void);
void chkpipe(int *);
int chkopen(const unsigned char *, int);
void renamef(int, int);
int create(unsigned char *);
int tmpfil(struct tempblk *);
void copy(struct ionod *);
void link_iodocs(struct ionod *);
void swap_iodoc_nm(struct ionod *);
int savefd(int);
void restore(int);
/* jobs.c */
void collect_fg_job(void);
void freejobs(void);
int settgid(pid_t, pid_t);
void startjobs(void);
int endjobs(int);
void deallocjob(void);
void allocjob(char *, unsigned char *, int);
void clearjobs(void);
void makejob(int, int);
void postjob(pid_t, int);
void sysjobs(int, char *[]);
void sysfgbg(int, char *[]);
void syswait(int, char *[]);
void sysstop(int, char *[]);
void syskill(int, char *[]);
void syssusp(int, char *[]);
/* macro.c */
unsigned char *macro(unsigned char *);
void comsubst(int);
void subst(int, int);
void flush(int);
/* main.c */
void chkpr(void);
void settmp(void);
void Ldup(int, int);
void chkmail(void);
void setmail(unsigned char *);
void setwidth(void);
void sh_setmode(int);
/* msg.c */
/* name.c */
int syslook(unsigned char *, const struct sysnod [], int);
void setlist(struct argnod *, int);
void setname(unsigned char *, int);
void replace(unsigned char **, const unsigned char *);
void dfault(struct namnod *, const unsigned char *);
void assign(struct namnod *, const unsigned char *);
int readvar(unsigned char **);
void assnum(unsigned char **, long);
unsigned char *make(const unsigned char *);
struct namnod *lookup(unsigned char *);
BOOL chkid(unsigned char *);
void namscan(void (*)(struct namnod *));
void printnam(struct namnod *);
void exname(struct namnod *);
void printro(struct namnod *);
void printexp(struct namnod *);
void setup_env(void);
unsigned char **local_setenv(void);
void setvars(void);
struct namnod *findnam(unsigned char *);
void unset_name(unsigned char *);
/* print.c */
void prp(void);
void prs(const unsigned char *);
void prc(unsigned char);
void prwc(wchar_t);
void prt(long);
void prn(int);
void itos(int);
int stoi(const unsigned char *);
long long stoifll(const unsigned char *);
int ltos(long);
void prl(long);
int ulltos(unsigned long long);
void prull(unsigned long long);
void flushb(void);
void prc_buff(unsigned char);
void prs_buff(const unsigned char *);
unsigned char *octal(unsigned char, unsigned char *);
void prs_cntl(const unsigned char *);
void prl_buff(long);
void prull_buff(unsigned long long);
void prn_buff(int);
void prsp_buff(int);
int setb(int);
/* pwd.c */
void cwd(unsigned char *);
void cwd2(void);
unsigned char *cwdget(void);
void cwdprint(void);
/* service.c */
int initio(struct ionod *, int);
unsigned char *simple(unsigned char *);
unsigned char *getpath(unsigned char *);
int pathopen(const unsigned char *, const unsigned char *);
unsigned char *catpath(const unsigned char *, const unsigned char *);
unsigned char *nextpath(const unsigned char *);
void execa(unsigned char *[], int);
void trim(unsigned char *);
void trims(unsigned char *);
unsigned char *mactrim(unsigned char *);
unsigned char **scan(int);
int getarg(struct comnod *);
void suspacct(void);
void preacct(unsigned char *);
void doacct(void);
int compress(clock_t);
/* setbrk.c */
unsigned char *setbrk(int);
/* stak.c */
void tdystak(unsigned char *);
void stakchk(void);
/* string.c */
unsigned char *movstr(const unsigned char *, unsigned char *);
int any(wchar_t, const unsigned char *);
int anys(const unsigned char *, const unsigned char *);
int cf(const unsigned char *, const unsigned char *);
int length(const unsigned char *);
unsigned char *movstrn(const unsigned char *, unsigned char *, int);
/* strsig.c */
int str_2_sig(const char *, int *);
int sig_2_str(int, char *);
char *str_signal(int);
/* test.c */
int test(int, unsigned char *[]);
/* ulimit.c */
void sysulimit(int, char **);
/* umask.c */
void sysumask(int, char **);
/* word.c */
int word(void);
unsigned int skipwc(void);
unsigned int nextwc(void);
unsigned char *readw(wchar_t);
unsigned int readwc(void);
/* xec.c */
int execute(struct trenod *, int, int, int *, int *);
void execexp(unsigned char *, intptr_t);
void execprint(unsigned char **);

#define		attrib(n, f)		(n->namflg |= f)
#define		round(a, b)	(((intptr_t)(((char *)(a)+b)-1))&~((b)-1))
#define		closepipe(x)	(close(x[INPIPE]), close(x[OTPIPE]))
#define		eq(a, b)		(cf(a, b) == 0)
#define		max(a, b)		((a) > (b)?(a):(b))
#define		assert(x)

/* temp files and io */
extern int				output;
extern int				ioset;
extern struct ionod	*iotemp; /* files to be deleted sometime */
extern struct ionod	*fiotemp; /* function files to be deleted sometime */
extern struct ionod	*iopend; /* documents waiting to be read at NL */
extern struct fdsave	fdmap[];
extern int savpipe;

/* substitution */
extern int				dolc;
extern unsigned char				**dolv;
extern struct dolnod	*argfor;
extern struct argnod	*gchain;

/* stak stuff */
#include		"stak.h"

/* string constants */
extern const char				atline[];
extern const char				readmsg[];
extern const char				colon[];
extern const char				minus[];
extern const char				nullstr[];
extern const char				sptbnl[];
extern const char				unexpected[];
extern const char				endoffile[];
extern const char				synmsg[];

/* name tree and words */
extern const struct sysnod	reserved[];
extern const int				no_reserved;
extern const struct sysnod	commands[];
extern const int				no_commands;

extern int				wdval;
extern int				wdnum;
extern int				fndef;
extern int				nohash;
extern struct argnod	*wdarg;
extern int				wdset;
extern BOOL				reserv;

/* prompting */
extern const char				stdprompt[];
extern const char				supprompt[];
extern const char				profile[];
extern const char				sysprofile[];

/* built in names */
extern struct namnod	fngnod;
extern struct namnod	cdpnod;
extern struct namnod	ifsnod;
extern struct namnod	homenod;
extern struct namnod	mailnod;
extern struct namnod	pathnod;
extern struct namnod	ps1nod;
extern struct namnod	ps2nod;
extern struct namnod	mchknod;
extern struct namnod	acctnod;
extern struct namnod	mailpnod;
extern struct namnod	timeoutnod;

/* special names */
extern unsigned char				flagadr[];
extern unsigned char				*pcsadr;
extern unsigned char				*pidadr;
extern unsigned char				*cmdadr;

/* names always present */
extern const char				defpath[];
extern const char				mailname[];
extern const char				homename[];
extern const char				pathname[];
extern const char				cdpname[];
extern const char				ifsname[];
extern const char				ps1name[];
extern const char				ps2name[];
extern const char				mchkname[];
extern const char				acctname[];
extern const char				mailpname[];
extern const char				timeoutname[];

/* transput */
extern unsigned char				tmpout[];
extern unsigned char				*tmpname;
extern int				serial;

#define			TMPNAM 		7

extern struct fileblk	*standin;

#define		input		(standin->fdes)
#define		eof			(standin->feof)

extern int				peekc;
extern int				peekn;
extern unsigned char				*comdiv;
extern const char				devnull[];

/* flags */
#define			noexec		01
#define			sysflg		01
#define			intflg		02
#define			prompt		04
#define			setflg		010
#define			errflg		020
#define			ttyflg		040
#define			forked		0100
#define			oneflg		0200
#define			rshflg		0400
#define			subsh		01000
#define			stdflg		02000
#define			STDFLG		's'
#define			execpr		04000
#define			readpr		010000
#define			keyflg		020000
#define			hashflg		040000
#define			nofngflg	0200000
#define			exportflg	0400000
#define			monitorflg	01000000
#define			jcflg		02000000
#define			privflg		04000000
#define			forcexit	010000000
#define			jcoff		020000000
#define			waiting		040000000

extern long				flags;
extern int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
#include	<setjmp.h>
extern jmp_buf			subshell;
extern jmp_buf			errshell;

/* fault handling */
#include	"brkincr.h"

extern unsigned			brkincr;
#define		MINTRAP		0
#if defined (NSIG) && (NSIG) >= 36
#define		MAXTRAP		NSIG
#else
#define		MAXTRAP		36
#endif

#define		TRAPSET		2
#define		SIGSET		4
#define			SIGMOD		8
#define			SIGIGN		16

extern BOOL				trapnote;

/* name tree and words */
extern unsigned char				numbuf[];
extern const char				export[];
extern const char				duperr[];
extern const char				readonly[];

/* execflgs */
extern int				exitval;
extern int				retval;
extern BOOL				execbrk;
extern int				loopcnt;
extern int				breakcnt;
extern int				funcnt;
extern int				tried_to_exit;

/* messages */
extern const char				mailmsg[];
extern const char				coredump[];
extern const char				badopt[];
extern const char				badparam[];
extern const char				unset[];
extern const char				badsub[];
extern const char				nospace[];
extern const char				nostack[];
extern const char				notfound[];
extern const char				badtrap[];
extern const char				baddir[];
extern const char				badshift[];
extern const char				restricted[];
extern const char				execpmsg[];
extern const char				notid[];
extern const char 				badulimit[];
extern const char 				badresource[];
extern const char 				badscale[];
extern const char 				ulimit[];
extern const char				wtfailed[];
extern const char				badcreate[];
extern const char				nofork[];
extern const char				noswap[];
extern const char				piperr[];
extern const char				badopen[];
extern const char				badnum[];
extern const char				badsig[];
extern const char				badid[];
extern const char				arglist[];
extern const char				txtbsy[];
extern const char				toobig[];
extern const char				badexec[];
extern const char				badfile[];
extern const char				badreturn[];
extern const char				badexport[];
extern const char				badunset[];
extern const char				nohome[];
extern const char				badperm[];
extern const char				mssgargn[];
extern const char				libacc[];
extern const char				libbad[];
extern const char				libscn[];
extern const char				libmax[];
extern const char				emultihop[];
extern const char				nulldir[];
extern const char				enotdir[];
extern const char				enoent[];
extern const char				eacces[];
extern const char				enolink[];
extern const char				exited[];
extern const char				running[];
extern const char				ambiguous[];
extern const char				nosuchjob[];
extern const char				nosuchpid[];
extern const char				nosuchpgid[];
extern const char				usage[];
extern const char				nojc[];
extern const char				killuse[];
extern const char				jobsuse[];
extern const char				stopuse[];
extern const char				ulimuse[];
extern const char				nocurjob[];
extern const char				loginsh[];
extern const char				jobsstopped[];
extern const char				jobsrunning[];

/*	'builtin' error messages	*/

extern const char				btest[];
extern const char				badop[];

/*	fork constant	*/

#define		FORKLIM 	32

extern address			end[];

#include	"ctype.h"
#include	<ctype.h>
#include	<locale.h>

extern int				eflag;
extern int				ucb_builtins;

/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */

#define		sigchk()	if (trapnote & SIGSET)	\
					exitsh(exitval ? exitval : SIGFAIL)

#define		exitset()	retval = exitval


#ifdef	__cplusplus
}
#endif

#endif	/* _DEFS_H */

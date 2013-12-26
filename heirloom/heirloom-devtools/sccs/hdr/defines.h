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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from defines.h 1.21 06/12/12
 */

/*	from OpenSolaris "defines.h"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)defines.h	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:hdr/defines.h"	*/
# include	<sys/types.h>
# include	<sys/param.h>
# include	<sys/stat.h>
# include	<errno.h>
# include	<fcntl.h>
# include	<stdio.h>
# include	<stdlib.h>
# include	<unistd.h>
# include	<string.h>
# include	<locale.h>
# include	<macros.h>
# undef		abs
# include	<fatal.h>
# include	<time.h>
# include 	"fatal.h"

#define ALIGNMENT  	(sizeof(long long))
#define ROUND(x,base)   (((x) + (base-1)) & ~(base-1))

# define CTLSTR		"%c%c\n"

# define CTLCHAR	1
# define HEAD		'h'

# define STATS		's'

# define BDELTAB	'd'
# define INCLUDE	'i'
# define EXCLUDE	'x'
# define IGNORE		'g'
# define MRNUM		'm'
# define COMMENTS	'c'
# define EDELTAB	'e'

# define BUSERNAM	'u'
# define EUSERNAM	'U'

# define NFLAGS	28

# define FLAG		'f'
# define NULLFLAG	'n'
# define JOINTFLAG	'j'
# define DEFTFLAG	'd'
# define TYPEFLAG	't'
# define VALFLAG	'v'
# define CMFFLAG	'z'
# define BRCHFLAG	'b'
# define IDFLAG		'i'
# define MODFLAG	'm'
# define FLORFLAG	'f'
# define CEILFLAG	'c'
# define QSECTFLAG	'q'
# define LOCKFLAG	'l'
# define ENCODEFLAG	'e'
# define SCANFLAG	's'
# define EXPANDFLAG	'y'

# define BUSERTXT	't'
# define EUSERTXT	'T'

# define INS		'I'
# define DEL		'D'
# define END		'E'

# define MINR		1		/* minimum release number */
# define MAXR		9999		/* maximum release number */
# define FILESIZE	MAXPATHLEN
# define MAXLINE	BUFSIZ
# define DEF_LINE_SIZE	128
# undef  MAX
# define MAX		9999
# define DELIVER	'*'
# define LOGSIZE	(33)		/* TWCP SCCS compatibility */
# define MAXERRORLEN	(1024+MAXPATHLEN)	/* max length of SccsError buffer */

# define FAILPUT    fatal("fputs could not write to file (ut13)")
# define SCCS_LOCK_ATTEMPTS	4       /* maximum number of lock   attempts  */
# define SCCS_CREAT_ATTEMPTS	4       /* maximum number of create attempts  */

/*
	SCCS Internal Structures.
*/

struct apply {
	char	a_inline;	/* in the line of normally applied deltas */
	int	a_code;		/* APPLY, NOAPPLY or SX_EMPTY */
	int	a_reason;
};

#define SX_EMPTY	(0)
#define APPLY		(1)
#define NOAPPLY		(2)

# define IGNR		0100
# define USER		040
# define INCL		1
# define EXCL		2
# define CUTOFF		4
# define INCLUSER	(USER | INCL)
# define EXCLUSER	(USER | EXCL)
# define IGNRUSER	(USER | IGNR)

struct queue {
	struct queue *q_next;
	int	q_sernum;	/* serial number */
	char    q_keep;		/* keep switch setting */
	char	q_iord;		/* INS or DEL */
	char	q_ixmsg;	/* caused inex msg */
	char	q_user;		/* inex'ed by user */
};

#define YES	(1)
#define NO	(2)

struct	sid {
	int	s_rel;
	int	s_lev;
	int	s_br;
	int	s_seq;
};

struct	deltab {
	struct	sid	d_sid;
	int	d_serial;
	int	d_pred;
	time_t	d_datetime;
	char	d_pgmr[LOGSIZE];
	char	d_type;
};

struct	ixg {
	struct	ixg	*i_next;
	char	i_type;
	char	i_cnt;
	int	i_ser[1];
};

struct	idel {
	struct	sid	i_sid;
	struct	ixg	*i_ixg;
	int	i_pred;
	time_t	i_datetime;
};

# define maxser(pkt)	((pkt)->p_idel->i_pred)
# define sccsfile(f)	imatch("s.", sname(f))

struct packet {
	char	p_file[FILESIZE]; /* file name containing module */
	struct	sid p_reqsid;	/* requested SID, then new SID */
	struct	sid p_gotsid;	/* gotten SID */
	struct	sid p_inssid;	/* SID which inserted current line */
	char	p_verbose;	/* verbose flags (see #define's below) */
	char	p_upd;		/* update flag (!0 = update mode) */
	time_t	p_cutoff;	/* specified cutoff date-time */
	int	p_ihash;	/* initial (input) hash */
	int	p_chash;	/* current (input) hash */
	int	p_uchash;	/* current unsigned (input) hash */
	int	p_nhash;	/* new (output) hash */
	int	p_glnno;	/* line number of current gfile line */
	int	p_slnno;	/* line number of current input line */
	char	p_wrttn;	/* written flag (!0 = written) */
	char	p_keep;		/* keep switch for readmod() */
	struct	apply *p_apply;	/* ptr to apply array */
	struct	queue *p_q;	/* ptr to control queue */
	FILE	*p_iop;		/* input file */
	char	p_buf[BUFSIZ];	/* input file buffer */
	char	*p_line;	/* buffer for getline() */
	size_t	p_line_size;	/* size of the buffer for getline() */
	time_t	p_cdt;		/* date/time of newest applied delta */
	char	*p_lfile;	/* 0 = no l-file; else ptr to l arg */
	struct	idel *p_idel;	/* ptr to internal delta table */
	FILE	*p_stdout;	/* standard output for warnings and messages */
	FILE	*p_gout;	/* g-file output file */
	char	p_user;		/* !0 = user on user list */
	char	p_chkeof;	/* 0 = eof generates error */
	int	p_maxr;		/* largest release number */
	int	p_ixmsg;	/* inex msg counter */
	int	p_reopen;	/* reopen flag used by getline on eof */
	int	p_ixuser;	/* HADI | HADX (in get) */
	int	do_chksum;	/* for getline(), 1 = do check sum */
};

struct	stats {
	int	s_ins;
	int	s_del;
	int	s_unc;
};

struct	pfile	{
	struct	sid	pf_gsid;
	struct	sid	pf_nsid;
	char	pf_user[LOGSIZE];
	time_t	pf_date;
	char	*pf_ilist;
	char	*pf_elist;
	char 	*pf_cmrlist;
};


/*
 Declares for external functions in lib/cassi
*/

extern	int 	cmrcheck(char *, char *);
extern	int 	deltack(char [], char *, char *, char *);
extern	void 	error(const char *);
extern	int 	sweep(int, char *, char *, int, int, int, char *[], char *, char *[], int (*)(char *, int, char **), int (*)(char **, char **, int));
extern	char *	gf(char *);

/*
 Declares for external functions in lib/comobj
*/

extern	char *	auxf(register char *, int);
extern	int 	chkid(char *, char *);
extern	void 	chksid(char *, register struct sid *);
extern	int 	cmpdate(struct tm *, struct tm *);
extern	int 	date_ab(char *, time_t *);
extern	int 	parse_date(char *, time_t *);
extern	int 	mosize(int, int);
extern	int 	gN(char *, char **, int, int *, int *);
extern	char	*date_ba(time_t *, char *);
extern	char 	del_ab(register char *, register struct deltab *, struct packet *);
extern	void 	get_Del_Date_time(register char *, struct deltab *, struct packet *, struct tm *);
extern	char *	del_ba(register struct deltab *, char *);
extern	struct idel *	dodelt(register struct packet *, struct stats *, struct sid *, int);
extern	void 	do_file(register char *, void (*)(char *), int);
extern	void 	dohist(char *);
extern	int 	valmrs(struct packet *, char *);
extern	void 	mrfixup(void);
extern	char *	stalloc(register unsigned int);
extern	char *	savecmt(register char *);
extern	char *	get_Sccs_Comments(void);
extern	void 	doie(struct packet *, char *, char *, char *);
extern	void 	dolist(struct packet *, register char *, int);
extern	int 	eqsid(register struct sid *, register struct sid *);
extern	void 	flushto(register struct packet *, int, int);
extern	void 	fmterr(register struct packet *);
#undef getline
#define getline	xxgetline
extern	char *	getline(register struct packet *);
extern	int 	getser(register struct packet *);
extern	char *	logname(void);
extern	void 	newsid(register struct packet *, int);
extern	void 	newstats(register struct packet *, register char *, register char *);
extern	void 	finduser(register struct packet *);
extern	void 	doflags(struct packet *);
extern	void 	permiss(register struct packet *);
extern	void 	pf_ab(char *, register struct pfile *, int);
extern	void 	putline(register struct packet *, char *);
extern	void 	flushline(register struct packet *, register struct stats *);
extern	void 	xrm(void);
extern	int 	readmod(struct packet *);
extern	void 	addq(struct packet *, int, int, int, int);
extern	void 	remq(struct packet *, int);
extern	void 	setkeep(struct packet *);
extern	void 	setup(register struct packet *, int);
extern	void 	condset(register struct apply *, int, int);
extern	char *	sid_ab(register char *, register struct sid *);
extern	char *	omit_sid(char *);
extern	char *	sid_ba(register struct sid *, register char *);
extern	int 	sidtoser(register struct sid *, struct packet *);
extern	void 	sinit(register struct packet *, register char *, int);
extern	void 	stats_ab(register struct packet *, register struct stats *);
extern	int 	mystrptime(char *, struct tm *, int);

/*
 Declares for external functions in lib/mpwlib
*/

extern	char *	abspath(char *);
extern	int 	any(int, register char *);
extern	char *	cat(register char *, ...);
extern	char *	dname(char *);
extern	void	encode(FILE *, FILE *);
extern	void	decode(char *, FILE *);
extern	int 	fatal(char *);
extern	char *	nse_file_trim(char *, int);
extern	int 	check_permission_SccsDir(char *);
extern	FILE *	fdfopen(register int, register int);
extern	void *	fmalloc(unsigned);
extern	void 	ffree(void *);
extern	void 	ffreeall(void);
extern	int 	imatch(register char *, register char *);
extern	int 	sccs_index(char *, char *);
extern	int	lockit(char *, int, pid_t, char *);
extern	int	unlockit(char *, pid_t, char *);
extern	int	mylock(char *, pid_t, char *);
extern	int 	patoi(register char *);
extern	char *	repl(char *, char, char);
extern	char *	satoi(register char *, register int *);
extern	void 	setsig(void);
extern	char *	sname(char *);
extern	char *	strend(register char *);
extern	char *	trnslat(register char *, char *, char *, char *);
extern	int 	userexit(int);
extern	int	xcreat(char *, mode_t);
extern	int 	xlink(const char *, const char *);
extern	int 	xmsg(const char *, const char *);
extern	int 	xopen(char [], mode_t);
extern	int 	xpipe(int *);
extern	int 	xunlink(const char *);
extern	char *	zero(register char *, register int);

# define RESPSIZE	1024
# define NVARGS		64
# define VSTART		3

/*
**	The following five definitions (copy, xfopen, xfcreat, remove,
**	USXALLOC) are taken from macros.h 1.1
*/

# define copy(srce,dest)	cat(dest,srce,0)
# define xfopen(file,mode)	fdfopen(xopen(file,mode),mode)
# define xfcreat(file,mode)	fdfopen(xcreat(file,mode),1)
# define remove(file)		xunlink(file)
# define USXALLOC() \
		char *alloc(n) {return((char *)xalloc((unsigned)n));} \
		free(n) char *n; {xfree(n);} \
		char *malloc(n) unsigned n; {int p; p=xalloc(n); \
			return((char *)(p != -1?p:0));}


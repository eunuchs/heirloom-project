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
 * from admin.c 1.39 06/12/12
 */

/*	from OpenSolaris "admin.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)admin.c	1.5 (gritter) 2/26/07
 */
/*	from OpenSolaris "sccs:cmd/admin.c"	*/

# include	<defines.h>
# include	<had.h>
# include	<i18n.h>
# include	<dirent.h>
# include	<setjmp.h>
# include	<sys/utsname.h>
# include	<sys/wait.h>

/*
	Program to create new SCCS files and change parameters
	of existing ones. Arguments to the program may appear in
	any order and consist of keyletters, which begin with '-',
	and named files. Named files which do not exist are created
	and their parameters are initialized according to the given
	keyletter arguments, or are given default values if the
	corresponding keyletters were not supplied. Named files which
	do exist have those parameters corresponding to given key-letter
	arguments changed and other parameters are left as is.

	If a directory is given as an argument, each SCCS file within
	the directory is processed as if it had been specifically named.
	If a name of '-' is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.

	Files created are given mode 444.
*/

/*
TRANSLATION_NOTE
For all message strings the double quotation marks at the beginning
and end of the string MUST be included in the translation.  Formatting
characters, e.g. "%s" "%c" "%d" "\n" "\t" must appear in the
translated string exactly as they do in the msgid string.  Spaces
and/or tabs around these formats must be maintained.
 
The following are examples of text that should not be translated, but
should appear exactly as they do in the msgid string:
 
- Any SCCS error code, which will be one or two letters followed by one
  or two numbers all in parenthesis, e.g. "(ad3)",

- All descriptions of SCCS option flags, e.g. "-r" or " 'e'" , or "f",
  or "-fz"
 
- ".FRED", "sid", "SID", "MRs", "CMR", "p-file", "CASSI",  "cassi",
  function names, e.g. "getcwd()", 
*/

# define MAXNAMES 9
# define COPY 0
# define NOCOPY 1

struct stat Statbuf;

char	Null[1];
char	SccsError[MAXERRORLEN];
char	*Comments, *Mrs;

int	Did_id;
int	Domrs;

extern FILE *Xiop;
extern char *Sflags[];

static char	stdin_file_buf [20];
static char	*ifile, *tfile;
static time_t	ifile_mtime;
static char	*CMFAPPL;	/* CMF MODS */
static char	*z;			/* for validation program name */
static char	had_flag[NFLAGS], rm_flag[NFLAGS];
static char	Valpgm[]	=	NOGETTEXT("val");
static int	fexists, num_files;
static int	VFLAG  =  0;
static struct sid	new_sid;
static char	*anames[MAXNAMES], *enames[MAXNAMES];
static char	*unlock;
static char	*locks;
static char	*flag_p[NFLAGS];
static int	asub, esub;
static int	check_id;
static struct	utsname	un;
static char	*uuname;
static int 	Encoded = 0;
static off_t	Encodeflag_offset;	/* offset in file where encoded flag is stored */

static int	fgetchk(FILE *,char *,struct packet *,int);
static void	admin(char *);
static void	cmt_ba(register struct deltab *, char *);
static void	putmrs(struct packet *);
static char	*adjust(char *);
static char	*getval(register char *, register char *);
static int	val_list(register char *);
static int	pos_ser(char *, char *);
static int	range(register char *);
static void	code(FILE *, char *, int, int, struct packet *);
extern int	org_ihash;
extern int	org_chash;
extern int	org_uchash;

extern char	*saveid;

int 
main(int argc, char *argv[])
{
	register int j;
	register char *p;
	char  f;
	int i, testklt,c;
	extern int Fcnt;
	struct sid sid;
	int no_arg=0;
	int current_optind;

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;

	testklt = 1;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/

	current_optind = 1;
	optind = 1;
	opterr = 0;
	j = 1;
	/*CONSTCOND*/
	while (1) {
			if(current_optind < optind) {
			   current_optind = optind;
			   argv[j] = 0;
			   if (optind > j+1 ) {
			      if((argv[j+1][0] != '-') && !no_arg){
			        argv[j+1] = NULL;
			      }
			      else {
				optind = j+1;
			        current_optind = optind;
			      }
			   }
			}
			no_arg = 0;
			j = current_optind;
		        c = getopt(argc, argv, "-i:t:m:y:d:f:r:nhzbqa:e:");
		        	/*
				*  this takes care of options given after
				*  file names.
				*/
			if((c == EOF)) {
			   if (optind < argc) {
				/* if it's due to -- then break; */
			       if(argv[j][0] == '-' &&
				      argv[j][1] == '-') {
			          argv[j] = 0;
			          break;
			       }
			       optind++;
			       current_optind = optind;
			       continue;
			   } else {
			       break;
			   }
			}
			p = optarg;
			switch (c) {

			case 'i':	/* name of file of body */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   ifile = "";
				   break;
				}
				ifile = p;
				if (*ifile && exists(ifile))
				   if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				      sprintf(SccsError,
				        "directory `%s' specified as `%c' keyletter value (ad29)",
				        ifile, c);
				      fatal(SccsError);
				}
				break;
			case 't':	/* name of file of descriptive text */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   tfile = NULL;
				   break;
				}
				tfile = p;
				if (*tfile && exists(tfile))
				   if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				      sprintf(SccsError,
				        "directory `%s' specified as `%c' keyletter value (ad29)",
				        tfile, c);
				      fatal(SccsError);
				}
				break;
			case 'm':	/* mr flag */
				if (!(*p) || *p == '-')
					fatal("bad m argument (ad34)");
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'y':	/* comments flag for entry */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   Comments = "";
				} else {  
				   Comments = p;
				}
				break;
			case 'd':	/* flags to be deleted */
				testklt = 0;
				if ((f = *p) == '\0')
				     fatal("d has no argument (ad1)");
				p = p+1;

				switch (f) {

				case IDFLAG:	/* see 'f' keyletter */
				case BRCHFLAG:	/* for meanings of flags */
				case VALFLAG:
				case TYPEFLAG:
				case MODFLAG:
				case QSECTFLAG:
				case NULLFLAG:
				case FLORFLAG:
				case CEILFLAG:
				case DEFTFLAG:
				case JOINTFLAG:
				case SCANFLAG:
				case EXPANDFLAG:
				case CMFFLAG:	/* option installed by CMF */
					if (*p) {
						sprintf(SccsError,
						"value after %c flag (ad12)",
							f);
						fatal(SccsError);
					}
					break;
				case LOCKFLAG:
					if (*p == 0) {
						fatal("bad list format (ad27)");
					}
					if (*p) {
						/*
						set pointer to releases
						to be unlocked
						*/
						repl(p,',',' ');
						if (!val_list(p))
							fatal("bad list format (ad27)");
						if (!range(p))
							fatal("element in list out of range (ad28)");
						if (*p != 'a')
							unlock = p;
					}
					break;

				default:
					fatal("unknown flag (ad3)");
				}

				if (rm_flag[f - 'a']++)
					fatal("flag twice (ad4)");
				break;

			case 'f':	/* flags to be added */
				testklt = 0;
				if ((f = *p) == '\0')
					fatal("f has no argument (ad5)");
				p = p+1;

				switch (f) {

				case BRCHFLAG:	/* branch */
				case NULLFLAG:	/* null deltas */
				case JOINTFLAG:	/* joint edit flag */
					if (*p) {
						sprintf(SccsError,
						"value after %c flag (ad13)",
							f);
						fatal(SccsError);
					}
					break;

				case IDFLAG:	/* id-kwd message (err/warn) */
					break;

				case VALFLAG:	/* mr validation */
					VFLAG++;
					if (*p)
						z = p;
					break;

				case CMFFLAG: /* CMFNET MODS */
					if (*p)	/* CMFNET MODS */
						CMFAPPL = p; /* CMFNET MODS */
					else /* CMFNET MODS */
						fatal ("No application with application flag.");
					if (gf (CMFAPPL) == (char*) NULL)
						fatal ("No .FRED file exists for this application.");
					break; /* END CMFNET MODS */

				case FLORFLAG:	/* floor */
					if ((i = patoi(p)) == -1)
						fatal("floor not numeric (ad22)");
					if (((int) size(p) > 5) || (i < MINR) ||
							(i > MAXR))
						fatal("floor out of range (ad23)");
					break;

				case CEILFLAG:	/* ceiling */
					if ((i = patoi(p)) == -1)
						fatal("ceiling not numeric (ad24)");
					if (((int) size(p) > 5) || (i < MINR) ||
							(i > MAXR))
						fatal("ceiling out of range (ad25)");
					break;

				case DEFTFLAG:	/* default sid */
					if (!(*p))
						fatal("no default sid (ad14)");
					chksid(sid_ab(p,&sid),&sid);
					break;

				case TYPEFLAG:	/* type */
				case MODFLAG:	/* module name */
				case QSECTFLAG:	/* csect name */
					if (!(*p)) {
						sprintf(SccsError,
						"flag %c has no value (ad2)",
							f);
						fatal(SccsError);
					}
					break;
				case LOCKFLAG:	/* release lock */
					if (!(*p))
						/*
						lock all releases
						*/
						p = NOGETTEXT("a");
					/*
					replace all commas with
					blanks in SCCS file
					*/
					repl(p,',',' ');
					if (!val_list(p))
						fatal("bad list format (ad27)");
					if (!range(p))
						fatal("element in list out of range (ad28)");
					break;
				case SCANFLAG:	/* the number of lines that are scanned to expand of SCCS KeyWords. */
					if ((i = patoi(p)) == -1)
						fatal("line not numeric (ad33)");
					break;
				case EXPANDFLAG:	/* expand the SCCS KeyWords */
					/* replace all commas with blanks in SCCS file */
					repl(p,',',' ');
					break;

				default:
					fatal("unknown flag (ad3)");
				}

				if (had_flag[f - 'a']++)
					fatal("flag twice (ad4)");
				flag_p[f - 'a'] = p;
				break;

			case 'r':	/* initial release number supplied */
				 chksid(sid_ab(p,&new_sid),&new_sid);
				 if ((new_sid.s_rel < MINR) ||
				     (new_sid.s_rel > MAXR))
					fatal("r out of range (ad7)");
				 break;

			case 'n':	/* creating new SCCS file */
			case 'h':	/* only check hash of file */
			case 'z':	/* zero the input hash */
			case 'b':	/* force file to be encoded (binary) */
				break;

                        case 'q':       /* activate NSE features */
				if(p) {
                                  if (*p) {
                                        nsedelim = p;
				  }
                                } else {
                                        nsedelim = (char *) 0;
                                }
                                break;

			case 'a':	/* user-name allowed to make deltas */
				testklt = 0;
				if (!(*p) || *p == '-')
					fatal("bad a argument (ad8)");
				if (asub > MAXNAMES)
					fatal("too many 'a' keyletters (ad9)");
				anames[asub++] = p;
				break;

			case 'e':	/* user-name to be removed */
				testklt = 0;
				if (!(*p) || *p == '-')
					fatal("bad e argument (ad10)");
				if (esub > MAXNAMES)
					fatal("too many 'e' keyletters (ad11)");
				enames[esub++] = p;
				break;

			default:
				fatal("unknown key letter (cm1)");
			}

			if (had[c - 'a']++ && testklt++)
				fatal("key letter twice (cm2)");
	}

	for(i=1; i<argc; i++){
		if(argv[i]) {
		       num_files++;
		}
	}

	if (num_files == 0) 
		fatal("missing file arg (cm3)");

	if ((HADY || HADM) && ! (HADI || HADN))
		fatal("illegal use of 'y' or 'm' keyletter (ad30)");
	if (HADI && num_files > 1) /* only one file allowed with `i' */
		fatal("more than one file (ad15)");
	if ((HADI || HADN) && ! logname())
		fatal("USER ID not in password file (cm9)");

	setsig();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'admin' routine for each file argument.
	*/
	for (j=1; j<argc; j++)
		if ((p = argv[j]) != NULL)
			do_file(p, admin, HADN|HADI ? 0 : 1);

	return (Fcnt ? 1 : 0);
}


/*
	Routine that actually does admin's work on SCCS files.
	Existing s-files are copied, with changes being made, to a
	temporary file (x-file). The name of the x-file is the same as the
	name of the s-file, with the 's.' replaced by 'x.'.
	s-files which are to be created are processed in a similar
	manner, except that a dummy s-file is first created with
	mode 444.
	At end of processing, the x-file is renamed with the name of s-file
	and the old s-file is removed.
*/

static struct packet gpkt;	/* see file defines.h */
static char	Zhold[MAXPATHLEN];	/* temporary z-file name */

static void 
admin(char *afile)
{
	struct deltab dt;	/* see file defines.h */
	struct stats stats;	/* see file defines.h */
	struct stat sbuf;
	FILE	*iptr = NULL;
	register int k;
	register char *cp;
	register signed char *q;
	char	*in_f;		/* ptr holder for lockflag vals in SCCS file */
	char	nline[BUFSIZ];
	char	*p_lval, *tval;
	char	*lval = 0;
	char	f;		/* character holder for flag character */
	char	line[BUFSIZ];
	int	i;		/* used in forking procedure */
	int	ck_it  =  0;	/* used for lockflag checking */
	int     offset;
	int     thash;
	int	status;		/* used for status return from fork */
	int	from_stdin = 0; /* used for ifile */
	extern	char had_dir;

	(void) &iptr;
	(void) &lval;
	(void) &ck_it;
	(void) &from_stdin;
	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of admin */

	zero((char *) &stats,sizeof(stats));

	if (HADI && had_dir) /* directory not allowed with `i' keyletter */
		fatal("directory named with `i' keyletter (ad26)");

	fexists = exists(afile);

	if (HADI)
		HADN = 1;
	if (HADI || HADN) {
			if (VFLAG && had_flag[CMFFLAG - 'a'])
			fatal("Can't have two verification routines.");

		if (HADM && !VFLAG && !had_flag[CMFFLAG - 'a'])
			fatal("MRs not allowed (de8)");

		if (VFLAG && !HADM)
			fatal("MRs required (de10)");

	}

	if (!(HADI||HADN) && HADR)
		fatal("r only allowed with i or n (ad16)");

	if (HADN && HADT && !tfile)
		fatal("t has no argument (ad17)");

	if (HADN && HADD)
		fatal("d not allowed with n (ad18)");

	if (HADN && fexists) {
		sprintf(SccsError, "file %s exists (ad19)",
			afile);
		fatal(SccsError);
	}

	if (!HADN && !fexists) {
		sprintf(SccsError, "file %s does not exist (ad20)",
			afile);
		fatal(SccsError);
	}
	if (HADH) {
		/*
		   fork here so 'admin' can execute 'val' to
		   check for a corrupted file.
		*/
		if ((i = fork()) < 0)
			fatal("cannot fork, try again");
		if (i == 0) {		/* child */
			/*
			   perform 'val' with appropriate keyletters
			*/
			execlp(Valpgm, Valpgm, "-s", afile, NULL);
			sprintf(SccsError, "cannot execute '%s'",
				Valpgm);
			fatal(SccsError);
		}
		else {
			wait(&status);	   /* wait on status from 'execlp' */
			if (status)
				fatal("corrupted file (co6)");
			return;		/* return to caller of 'admin' */
		}
	}

	/*
	Lock out any other user who may be trying to process
	the same file.
	*/
	uname(&un);
	uuname = un.nodename;
	if (!HADH && lockit(copy(auxf(afile,'z'),Zhold),SCCS_LOCK_ATTEMPTS,getpid(),uuname))
		fatal("cannot create lock file (cm4)");

	if (fexists) { /* modifying */

		sinit(&gpkt,afile,1);	/* init pkt & open s-file */
	
		/* Modify checksum if corrupted */

	        if ((int) strlen(gpkt.p_line) > 8 && gpkt.p_line[0] == '\001'
		         && gpkt.p_line[1] == '\150' ) 
		{
			gpkt.p_line[7] = '\012';
		   	gpkt.p_line[8] = '\000';
	
		}

	}


	

	else {
		if ((int) strlen(sname(afile)) > MAXNAMLEN) {
			sprintf(SccsError, "file name is greater than %d characters",
				MAXNAMLEN);
			fatal(SccsError);
		}
		if (exists(auxf(afile,'g')) && (S_IEXEC & Statbuf.st_mode)) {
			xfcreat(afile,0555);	/* create dummy s-file */
		} else {
			xfcreat(afile,0444);	/* create dummy s-file */
		}
		sinit(&gpkt,afile,0);	/* and init pkt */
	}

	if (!HADH)
		/*
		   set the flag for 'putline' routine to open
		   the 'x-file' and allow writing on it.
		*/
		gpkt.p_upd = 1;

	if (HADZ) {
		gpkt.do_chksum = 0;	/* ignore checksum processing */
		org_ihash = gpkt.p_ihash;
		gpkt.p_ihash = 0;
	}

	/*
	Get statistics of latest delta in old file.
	*/
	if (!HADN) {
		stats_ab(&gpkt,&stats);
		gpkt.p_wrttn++;
		newstats(&gpkt,line,"0");
	}

	if (HADN) {		/*   N E W   F I L E   */

		/*
		Beginning of SCCS file.
		*/
		sprintf(line,"%c%c%s\n",CTLCHAR,HEAD,"00000");
		putline(&gpkt,line);

		/*
		Statistics.
		*/
		newstats(&gpkt,line,"0");

		dt.d_type = 'D';	/* type of delta */

		/*
		Set initial release, level, branch and
		sequence values.
		*/
		if (HADR)
			{
			 dt.d_sid.s_rel = new_sid.s_rel;
			 dt.d_sid.s_lev = new_sid.s_lev;
			 dt.d_sid.s_br  = new_sid.s_br ;
			 dt.d_sid.s_seq = new_sid.s_seq;
			 if (dt.d_sid.s_lev == 0) dt.d_sid.s_lev = 1;
			 if ((dt.d_sid.s_br) && ( ! dt.d_sid.s_seq))
				dt.d_sid.s_seq = 1;
			}
		else
			{
			 dt.d_sid.s_rel = 1;
			 dt.d_sid.s_lev = 1;
			 dt.d_sid.s_br = dt.d_sid.s_seq = 0;
			}
		time(&dt.d_datetime);		/* get time and date */
                if (HADN && HADI && HADQ && (ifile_mtime != 0)) {
                        /* for NSE when putting existing file under sccs the
                         * delta time is the mtime of the clear file.
                         */
                        dt.d_datetime = ifile_mtime;
                }

		copy(logname(),dt.d_pgmr);	/* get user's name */

		dt.d_serial = 1;
		dt.d_pred = 0;

		del_ba(&dt,line);	/* form and write */
		putline(&gpkt,line);	/* delta-table entry */

		/*
		If -m flag, enter MR numbers
		*/

		if (Mrs) {
			if (had_flag[CMFFLAG - 'a']) {	/* CMF check routine */
				if (cmrcheck (Mrs, CMFAPPL) != 0) {	/* check them */
					fatal ("Bad CMR number(s).");
					}
				}
			mrfixup();
			if (z && valmrs(&gpkt,z))
				fatal("invalid MRs (de9)");
			putmrs(&gpkt);
		}

		/*
		Enter comment line for `chghist'
		*/

		if (HADY) {
		        char *comment = savecmt(Comments);
			sprintf(line,"%c%c ",CTLCHAR,COMMENTS);
			putline(&gpkt,line);
			putline(&gpkt,comment);
			putline(&gpkt,"\n");
		}
		else {
			/*
			insert date/time and pgmr into comment line
			*/
			cmt_ba(&dt,line);
			putline(&gpkt,line);
		}
		/*
		End of delta-table.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EDELTAB);
		putline(&gpkt,line);

		/*
		Beginning of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERNAM);
		putline(&gpkt,line);
	}
	else
		/*
		For old file, copy to x-file until user-name section
		is found.
		*/
		flushto(&gpkt,BUSERNAM,COPY);

	/*
	Write user-names to be added to list of those
	allowed to make deltas.
	*/
	if (HADA)
		for (k = 0; k < asub; k++) {
			sprintf(line,"%s\n",anames[k]);
			putline(&gpkt,line);
		}

	/*
	Do not copy those user-names which are to be erased.
	*/
	if (HADE && !HADN)
		while (((cp = getline(&gpkt)) != NULL) &&
				!(*cp++ == CTLCHAR && *cp == EUSERNAM)) {
			for (k = 0; k < esub; k++) {
				cp = gpkt.p_line;
				while (*cp)	/* find and */
					cp++;	/* zero newline */
				*--cp = '\0';	/* character */

				if (equal(enames[k],gpkt.p_line)) {
					/*
					Tell getline not to output
					previously read line.
					*/
					gpkt.p_wrttn = 1;
					break;
				}
				else
					*cp = '\n';	/* restore newline */
			}
		}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERNAM);
		putline(&gpkt,line);
	}
	else
		/*
		For old file, copy to x-file until end of
		user-names section is found.
		*/
		if (!HADE)
			flushto(&gpkt,EUSERNAM,COPY);

	/*
	For old file, read flags and their values (if any), and
	store them. Check to see if the flag read is one that
	should be deleted.
	*/
	if (!HADN)
		while (((cp = getline(&gpkt)) != NULL) &&
				(*cp++ == CTLCHAR && *cp++ == FLAG)) {

			gpkt.p_wrttn = 1;	/* don't write previous line */

			NONBLANK(cp);	/* point to flag character */
			k = *cp - 'a';
			f = *cp++;
			NONBLANK(cp);
			if (f == LOCKFLAG) {
				p_lval = cp;
				tval = fmalloc(size(gpkt.p_line)- (unsigned)5);
				copy(++p_lval,tval);
				lval = tval;
				while(*tval)
					++tval;
				*--tval = '\0';
			}

			if (!had_flag[k] && !rm_flag[k]) {
				had_flag[k] = 2;	/* indicate flag is */
							/* from file, not */
							/* from arg list */

				if (*cp != '\n') {	/* get flag value */
					q = (signed char *) fmalloc(size(gpkt.p_line) - (unsigned)5);
					copy(cp,q);
					flag_p[k] = (char*) q;
					while (*q)	/* find and */
						q++;	/* zero newline */
					*--q = '\0';	/* character */
				}
			}
			if (rm_flag[k])
				if (f == LOCKFLAG) {
					if (unlock) {
						in_f = lval;
						if (((lval = adjust(in_f)) != NULL) &&
							!had_flag[k])
							ck_it = had_flag[k] = 1;
					}
					else had_flag[k] = 0;
				}
				else had_flag[k] = 0;
		}


	/*
	Write out flags.
	*/
	/* test to see if the CMFFLAG is safe */
	if (had_flag[CMFFLAG - 'a']) {
		if (had_flag[VALFLAG - 'a'] && !rm_flag[VALFLAG - 'a'])
			fatal ("Can't use -fz with -fv.");
		}
	for (k = 0; k < NFLAGS; k++)
		if (had_flag[k]) {
			if (flag_p[k] || lval ) {
				if (('a' + k) == LOCKFLAG && had_flag[k] == 1) {
					if ((flag_p[k] && *flag_p[k] == 'a') || (lval && *lval == 'a'))
						locks = NOGETTEXT("a");
					else if (lval && flag_p[k])
						locks =
						cat(nline,lval," ",flag_p[k],0);
					else if (lval)
						locks = lval;
					else locks = flag_p[k];
					sprintf(line,"%c%c %c %s\n",
						CTLCHAR,FLAG,'a' + k,locks);
					locks = 0;
					if (lval) {
						ffree(lval);
						tval = lval = 0;
					}
					if (ck_it)
						had_flag[k] = ck_it = 0;
				}
				else if (flag_p[k])
					sprintf(line,"%c%c %c %s\n",
					 CTLCHAR,FLAG,'a'+k,flag_p[k]);
				     else
					sprintf(line,"%c%c %c\n",
					 CTLCHAR,FLAG,'a'+k);
			}
			else
				sprintf(line,"%c%c %c\n",
					CTLCHAR,FLAG,'a'+k);

			/* flush imbeded newlines from flag value */
			i = 4;
			if (line[i] == ' ')
				for (i++; line[i+1]; i++)
					if (line[i] == '\n')
						line[i] = ' ';
			putline(&gpkt,line);

			if (had_flag[k] == 2) {	/* flag was taken from file */
				had_flag[k] = 0;
				if (flag_p[k]) {
					ffree(flag_p[k]);
					flag_p[k] = 0;
				}
			}
		}

	if (HADN) {
		if (HADI) {
			/*
			If the "encoded" flag was not present, put it in
			with a value of 0; this acts as a place holder,
			so that if we later discover that the file contains
			non-ASCII characters we can flag it as encoded
			by setting the value to 1.
		 	*/
			Encodeflag_offset = ftell(Xiop);
			sprintf(line,"%c%c %c 0\n",
				CTLCHAR,FLAG,ENCODEFLAG);
			putline(&gpkt,line);
		}
		/*
		Beginning of descriptive (user) text.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERTXT);
		putline(&gpkt,line);
	}
	else
		/*
		Write out BUSERTXT record which was read in
		above loop that processes flags.
		*/
		gpkt.p_wrttn = 0;
		putline(&gpkt,(char *) 0);

	/*
	Get user description, copy to x-file.
	*/
	if (HADT) {
		if (tfile) {
		   if (*tfile) {
		      iptr = xfopen(tfile,0);
		      fgetchk(iptr, tfile, &gpkt, 0);
		      fclose(iptr);
		   }
		}

		/*
		If old file, ignore any previously supplied
		commentary. (i.e., don't copy it to x-file.)
		*/
		if (!HADN)
			flushto(&gpkt,EUSERTXT,NOCOPY);
	}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user description.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERTXT);
		putline(&gpkt,line);

		/*
		Beginning of body (text) of first delta.
		*/
		sprintf(line,"%c%c %d\n",CTLCHAR,INS,1);
		putline(&gpkt,line);

		if (HADI) {		/* get body */

			/*
			Set indicator to check lines of body of file for
			keyword definitions.
			If no keywords are found, a warning
			will be produced.
			*/
			check_id = 1;
			/*
			Set indicator that tells whether there
			were any keywords to 'no'.
			*/
			Did_id = 0;
			if (ifile) {
			   if (*ifile) {
				/* from a file */
				from_stdin = 0;
			   } else {
				/* from standard input */
				int    err = 0, cnt;
				char   buf[BUFSIZ];
				FILE * out;	
				mode_t cur_umask;

				from_stdin = 1;
				ifile 	   = stdin_file_buf;
				strcpy(stdin_file_buf, "/tmp/admin.XXXXXX");
				close(mkstemp(stdin_file_buf));
				cur_umask = umask((mode_t)((S_IRWXU|S_IRWXG|S_IRWXO)&~(S_IRUSR|S_IWUSR)));
				if ((out = fopen(ifile, "w")) == NULL) {
					xmsg(ifile, NOGETTEXT("admin"));
				}
				umask(cur_umask);
				/*CONSTCOND*/
				while (1) {
					if ((cnt = fread(buf, 1, BUFSIZ, stdin))) {
						if (fwrite(buf, 1, cnt, out) == cnt) {
							continue;
						}
						err = 1;
						break;
					} else {
						if (!feof(stdin)) {
							err = 1;
						}
						break;
					}
				}
				if (err) {
					unlink(ifile);
					xmsg(ifile, NOGETTEXT("admin"));
				}
				fclose(out);
			   }
			   iptr = xfopen(ifile,0);
			}

			/* save an offset to x-file in case need to encode
                           file.  Then won't have to start all over.  Also
                           save the hash value up to this point.
			 */
			offset = ftell(Xiop);
			thash = gpkt.p_nhash;

			/*
			If we haven't already been told that the file
			should be encoded, read and copy to x-file,
			while checking for control characters (octal 1),
			and also check if file ends in newline.  If
			control char or no newline, the file needs to
			be encoded.
			Also, count lines read, and set statistics'
			structure appropriately.
			The 'fgetchk' routine will check for keywords.
			*/
			if (!HADB) {
			   stats.s_ins = fgetchk(iptr, ifile, &gpkt, 1);
			   if (stats.s_ins == -1 ) {
			      Encoded = 1;
			   }
			} else {
			   Encoded = 1;
			}
			if (Encoded) {
			   /* non-ascii characters in file, encode them */
			   code(iptr, afile, offset, thash, &gpkt);
			   stats.s_ins = fgetchk(iptr, ifile, &gpkt, 0);
			}
			stats.s_del = stats.s_unc = 0;

			/*
			 *If no keywords were found, issue warning unless in
			 * NSE mode..
			 */
			if (!Did_id && !HADQ) {
				if (had_flag[IDFLAG - 'a'])
					if(!(flag_p[IDFLAG -'a']))
						fatal("no id keywords (cm6)");
					else
						fatal("invalid id keywords (cm10)");
				else
					fprintf(stderr, "No id keywords (cm7)\n");
			}

			check_id = 0;
			Did_id = 0;
		}

		/*
		End of body of first delta.
		*/
		sprintf(line,"%c%c %d\n",CTLCHAR,END,1);
		putline(&gpkt,line);
	}
	else {
		/*
		Indicate that EOF at this point is ok, and
		flush rest of (old) s-file to x-file.
		*/
		gpkt.p_chkeof = 1;
		while (getline(&gpkt)) ;
	}

	/* If encoded file, put change "fe" flag and recalculate
	   the hash value
	 */
	
	if (Encoded)
	{
		strcpy(line,"0");
		q = (signed char *) line;
		while (*q)
			gpkt.p_nhash -= *q++;
		strcpy(line,"1");
		q = (signed char *) line;
		while (*q)
			gpkt.p_nhash += *q++;
		fseek(Xiop,Encodeflag_offset,0);
		fprintf(Xiop,"%c%c %c 1\n",
			CTLCHAR, FLAG, ENCODEFLAG);
	}

	/*
	Flush the buffer, take care of rewinding to insert
	checksum and statistics in file, and close.
	*/
	org_chash = gpkt.p_chash;
	org_uchash = gpkt.p_uchash;
	flushline(&gpkt,&stats);

	/*
	Change x-file name to s-file, and delete old file.
	Unlock file before returning.
	*/
	if (!HADH) {
		if (!HADN) stat(gpkt.p_file,&sbuf);
		rename(auxf(gpkt.p_file,'x'),(char *)&gpkt);
		if (!HADN) {
			chmod(gpkt.p_file, sbuf.st_mode);
			chown(gpkt.p_file,sbuf.st_uid, sbuf.st_gid);
		}
		xrm();
		uname(&un);
		uuname = un.nodename;
		unlockit(auxf(afile,'z'),getpid(),uuname);
	}

	if (HADI)
		unlink(auxf(gpkt.p_file,'e'));
	if (from_stdin)
		unlink(stdin_file_buf);
}

static int
fgetchk(
FILE	*inptr,
char	*file,
struct	packet *pkt,
int	fflag)
{
	int	nline, index = 0, search_on = 0;
	char	lastchar;
	char	line[BUFSIZ];	

	/*
	 * This gives the illusion that a zero-length file ends
	 * in a newline so that it won't be mistaken for a 
	 * binary file.
	 */
	lastchar = '\n';
	
	nline = 0;
	memset(line, '\377', BUFSIZ);
	while (fgets(line, BUFSIZ, inptr) != NULL) {
	   if (line[0] == CTLCHAR) {
	      nline++;
	      goto err;
	   }
	   search_on = 0;
	   for (index=BUFSIZ-1; index >= 0; index--) {
	      if (search_on == 1) {
		 if (line[index] == '\0') {
	err:
		    if (fflag) {
		       return(-1);
		    } else {
		       sprintf(SccsError, 
			 "file '%s' contains illegal data on line %d (ad21)",
			 file, nline);
		       fatal(SccsError);
		    }
		 }
	      } else {
		 if (line[index] == '\0') {
		    search_on = 1;
		    lastchar = line[index-1];
	   	    if (lastchar == '\n') {
		       nline++;
		    }
		 }
	      }
	   }   
	   if (check_id) {
	      chkid(line, flag_p['i'-'a']);
	   }
	   putline(pkt, line);
	   memset(line, '\377', BUFSIZ);
	}
	if (lastchar != '\n'){
	   if (fflag) {
	      return(-1);
	   } else {
	      sprintf(SccsError,
		"No newline at end of file '%s' (ad31)",
		file);
	      fatal(SccsError);
	   }
	}
	return(nline);
}

void 
clean_up(void)
{
	xrm();
	if (Xiop)
		fclose(Xiop);
	if(gpkt.p_file[0]) {
		unlink(auxf(gpkt.p_file,'x'));
		if (HADI)
			unlink(auxf(gpkt.p_file,'e'));
		if (HADN)
			unlink(gpkt.p_file);
	}
	if (!HADH) {
		uname(&un);
		uuname = un.nodename;
		unlockit(Zhold, getpid(),uuname);
	}
}

static void 
cmt_ba(register struct deltab *dt, char *str)
{
	register char *p;

	p = str;
	*p++ = CTLCHAR;
	*p++ = COMMENTS;
	*p++ = ' ';
	copy(NOGETTEXT("date and time created"),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	date_ba(&dt->d_datetime,p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(NOGETTEXT("by"),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(dt->d_pgmr,p);
	while (*p++)
		;
	--p;
	*p++ = '\n';
	*p = 0;
}

static void 
putmrs(struct packet *pkt)
{
	register char **argv;
	char str[64];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}


static char *
adjust(char *line)
{
	register int k;
	register int i;
	char	*t_unlock;
	char	t_line[BUFSIZ];
	char	rel[5];

	t_unlock = unlock;
	while(*t_unlock) {
		NONBLANK(t_unlock);
		t_unlock = getval(t_unlock,rel);
		while ((k = pos_ser(line,rel)) != -1) {
			for(i = k; i < ((int) size(rel) + k); i++) {
				line[i] = '+';
				if (line[i++] == ' ')
					line[i] = '+';
				else if (line[i] == '\0')
					break;
				else --i;
			}
			k = 0;
			for(i = 0; i < (int) length(line); i++)
				if (line[i] == '+')
					continue;
				else if (k == 0 && line[i] == ' ')
					continue;
				else t_line[k++] = line[i];
			if (t_line[(int) strlen(t_line) - 1] == ' ')
				t_line[(int) strlen(t_line) - 1] = '\0';
			t_line[k] = '\0';
			line = t_line;
		}
	}
	if (length(line))
		return(line);
	else return(0);
}

static char *
getval(register char *sourcep, register char *destp)
{
	while (*sourcep != ' ' && *sourcep != '\t' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return(sourcep);
}

static int 
val_list(register char *list)
{
	register int i;

	if (list[0] == 'a')
		return(1);
	else for(i = 0; list[i] != '\0'; i++)
		if (list[i] == ' ' || numeric(list[i]))
			continue;
		else if (list[i] == 'a') {
			list[0] = 'a';
			list[1] = '\0';
			return(1);
		}
		else return(0);
	return(1);
}

static int 
pos_ser(char *s1, char *s2)
{
	register int offset;
	register char *p;
	char	num[5];

	p = s1;
	offset = 0;

	while(*p) {
		NONBLANK(p);
		p = getval(p,num);
		if (equal(num,s2)) {
			return(offset);
		}
		offset = offset + (int) size(num);
	}
	return(-1);
}

static int 
range(register char *line)
{
	register char *p;
	char	rel[BUFSIZ];

	p = line;
	while(*p) {
		NONBLANK(p);
		p = getval(p,rel);
		if ((int) size(rel) > 5)
			return(0);
	}
	return(1);
}

static void
code(
FILE *iptr,
char *afile,
int offset,
int thash,
struct packet *gpkt)
{
	FILE *eptr;


	/* issue a warning that file is non-text */
	if (!HADB)
		fprintf(stderr, "Not a text file (ad32)\n");
	rewind(iptr);
	eptr = fopen(auxf(afile,'e'),"w");

	encode(iptr,eptr);
	fclose(eptr);
	fclose(iptr);
	iptr = fopen(auxf(afile,'e'),"r");
	/* close the stream to xfile and reopen it at offset.  Offset is
	 * the end of sccs header info and before gfile contents
	 */
	putline(gpkt,0);
	fseek(Xiop,offset,0);
	gpkt->p_nhash = thash;
}

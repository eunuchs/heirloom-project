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
/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from prt.c 1.22 06/12/12
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*	from OpenSolaris "ucbprt:prt.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)prt.c	1.4 (gritter) 01/21/07
 */

/*
	Program to print parts or all of an SCCS file.
	Arguments to the program may appear in any order
	and consist of keyletters, which begin with '-',
	and named files.

	If a directory is given as an argument, each
	SCCS file within the directory is processed as if
	it had been specifically named. If a name of '-'
	is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.
*/

# include	<defines.h>
# include	<had.h>
# include	<i18n.h>

# define NOEOF	0
# define BLANK(p)	while (!(*p == ' ' || *p == '\t')) p++;

char SccsError[MAXERRORLEN];
struct stat Statbuf;
static FILE *iptr;
static char *line = NULL;
static size_t line_size = 0;
static char statistics[25];
static struct delent {
	char type;
	char *osid;
	char *datetime;
	char *pgmr;
	char *serial;
	char *pred;
} del;
static int num_files;
static int prefix;
static time_t	cutoff;
static time_t	revcut;
static int linenum;
static char *ysid;
static char *flagdesc[26] = {
			NOGETTEXT(""),
			NOGETTEXT("branch"),
			NOGETTEXT("ceiling"),
			NOGETTEXT("default SID"),
			NOGETTEXT("encoded"),
			NOGETTEXT("floor"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("id keywd err/warn"),
			NOGETTEXT("joint edit"),
			NOGETTEXT(""),
			NOGETTEXT("locked releases"),
			NOGETTEXT("module"),
			NOGETTEXT("null delta"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("csect name"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("type"),
			NOGETTEXT(""),
			NOGETTEXT("validate MRs"),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT(""),
			NOGETTEXT("")
};

static void	prt(char *);
static void	getdel(register struct delent *, register char *);
static char	*read_to(int);
static char	*lineread(register int);
static void	printdel(register char *, register struct delent *);
static void	printit(register char *, register char *, register char *);

int 
main(int argc, char *argv[])
{
	register int j;
	register char *p;
	int  c;
	int testklt;
	extern int Fcnt;
	int current_optind;

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine, and terminate processing.
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
			   if (optind > j+1 ) {
			      if((argv[j+1][0] != '-') &&
				 ((argv[j][0] == 'c' && argv[j+1][0] <= '9') || 
				  (argv[j][0] == 'r' && argv[j+1][0] <= '9') ||
				  (argv[j][0] == 'y' && argv[j+1][0] <= '9'))) {
			        argv[j+1] = NULL;
			      } else {
				optind = j+1;
				current_optind = optind;
			      }
			      
			   }
			   if(argv[j][0] == '-') {
			     argv[j] = 0;
			   }
			   current_optind = optind;
			}
			j = current_optind;
		        c = getopt(argc, argv, "-r:c:y:esdaiuftbt");

				/* this takes care of options given after
				** file names.
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
			case 'e':	/* print everything but body */
			case 's':	/* print only delta desc. and stats */
			case 'd':	/* print whole delta table */
			case 'a':	/* print all deltas */
			case 'i':	/* print inc, exc, and ignore info */
			case 'u':	/* print users allowed to do deltas */
			case 'f':	/* print flags */
			case 't':	/* print descriptive user-text */
			case 'b':	/* print body */
				break;
			case 'y':	/* delta cutoff */
				ysid = p;
				if(p[0] < '0'  || p[0] > '9') {
				  ysid = "";
				}
				prefix++;
				break;

			case 'c':	/* time cutoff */
				if (HADR) {
				   fatal("both 'c' and 'r' keyletters specified (pr2)");
				}
				if(p[0] ==  '-') {
				  /* no cutoff date given */
				  prefix++;
				  break;
				}
				if(p[0] > '9') {
				  prefix++;
				  break;
				}
				if (*p && parse_date(p,&cutoff))
					fatal("bad date/time (cm5)");
				prefix++;
				break;

			case 'r':	/* reverse time cutoff */
				if (HADC) {
				   fatal("both 'c' and 'r' keyletters specified (pr2)");
				}
				if(p[0] ==  '-') {
				  /* no cutoff date given */
				  prefix++;
				  break;
				}
				if(p[0] > '9') {
				  prefix++;
				  break;
				}
				if (*p && parse_date(p,&revcut))
					fatal ("bad date/time (cm5)");
				prefix++;
				break;

			default:
				fatal("unknown key letter (cm1)");
			}

			if (had[c - 'a']++ && testklt++)
				fatal("key letter twice (cm2)");
#if 0
			if(((c == 'c') || (c == 'r')||(c == 'y')) && (p[0] > '9')) {
			   argv[j] = NULL;
			   break;
			}
#endif
	}

	for(j=1; j<argc; j++){
		if(argv[j]) {
		       num_files++;
		}
	}

	if (num_files == 0)
		fatal("missing file arg (cm3)");

	if (HADC && HADR)
		fatal("both 'c' and 'r' keyletters specified (pr2)");

	setsig();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'prt' routine for each file argument.
	*/
	for (j = 1; j < argc; j++)
		if ((p = argv[j]) != NULL)
			do_file(p, prt, 1);

	return (Fcnt ? 1 : 0);
}


/*
	Routine that actually performs the 'prt' functions.
*/

static void 
prt(char *file)
{
	int stopdel;
	int user, flag, text;
	char *p;
	time_t	bindate;
#if defined(BUG_1205145) || defined(GMT_TIME)
	time_t	tim;

	tzset();
#endif

	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of prt */

	if (HADE)
		HADD = HADI = HADU = HADF = HADT = 1;

	if (!HADU && !HADF && !HADT && !HADB)
		HADD = 1;

	if (!HADD)
		HADR = HADS = HADA = HADI = HADY = HADC = 0;

	if (HADS && HADI)
		fatal("s and i conflict (pr1)");

	iptr = xfopen(file,0);
	linenum = 0;

	p = lineread(NOEOF);
	if (*p++ != CTLCHAR || *p != HEAD)
		fatal("not an sccs file (co2)");

	stopdel = 0;

	if (!prefix) {
		printf("\n%s:\n",file);
	}
	if (HADD) {
		while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR &&
				*p++ == STATS && !stopdel) {
			NONBLANK(p);
			copy(p,statistics);

			p = lineread(NOEOF);
			getdel(&del,p);
#if defined(BUG_1205145) || defined(GMT_TIME)
			date_ab(del.datetime,&bindate);
			/*
			Local time corrections before date_ba() call.
			Because this function uses gmtime() instead of localtime().
			*/
			tim = bindate - timezone;		/* GMT correction */
			if((localtime(&tim))->tm_isdst)
				tim += 1*60*60;			/* daylight savings */
			date_ba(&tim,del.datetime);
#endif

			if (!HADA && del.type != 'D') {
				read_to(EDELTAB);
				continue;
			}
			if (HADC) {
#if !(defined(BUG_1205145) || defined(GMT_TIME))
				date_ab(del.datetime,&bindate);
#endif
				if (bindate < cutoff) {
					stopdel = 1;
					break;
				}
			}
			if (HADR) {
#if !(defined(BUG_1205145) || defined(GMT_TIME))
				date_ab(del.datetime,&bindate);
#endif
				if (bindate >= revcut) {
					read_to(EDELTAB);
					continue;
				}
			}
			if (HADY && (equal(del.osid,ysid) || !(*ysid)))
				stopdel = 1;

			printdel(file,&del);

			while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR) {
				if (*p == EDELTAB)
					break;
				switch (*p) {
				case INCLUDE:
					if (HADI)
						printit(file,"Included:\t",p);
					break;

				case EXCLUDE:
					if (HADI)
						printit(file,"Excluded:\t",p);
					break;

				case IGNORE:
					if (HADI)
						printit(file,"Ignored:\t",p);
					break;

				case MRNUM:
					if (!HADS)
						printit(file,"MRs:\t",p);
					break;

				case COMMENTS:
					if (!HADS)
						printit(file,"",p);
					break;

				default:
					sprintf(SccsError, "format error at line %d (co4)", linenum);
					fatal(SccsError);
				}
			}
		}
		if (prefix)
			printf("\n");

		if (stopdel && !(line[0] == CTLCHAR && line[1] == BUSERNAM))
			read_to(BUSERNAM);
	}
	else
		read_to(BUSERNAM);

	if (HADU) {
		user = 0;
		printf("\n Users allowed to make deltas -- \n");
		while (((p = lineread(NOEOF)) != NULL) && *p != CTLCHAR) {
			user = 1;
			printf("\t%s",p);
		}
		if (!user)
			printf("\teveryone\n");
	}
	else
		read_to(EUSERNAM);

	if (HADF) {
		flag = 0;
		printf("\n%s\n", "Flags --");
		while (((p = lineread(NOEOF)) != NULL) && *p++ == CTLCHAR &&
				*p++ == FLAG) {
			flag = 1;
			NONBLANK(p);
			/*
			 * The 'e' flag (file in encoded form) requires
			 * special treatment, as some versions of admin
			 * force the flag to be present (with operand 0)
			 * even when the user didn't explicitly specify
			 * it.  Stated differently, this flag has somewhat
			 * different semantics than the other binary-
			 * valued flags.
			 */
			if (*p == 'e') {
				/*
				 * Look for operand value; print description
				 * only if the operand value exists and is '1'.
				 */
				if (*++p) {
					NONBLANK(p);
					if (*p == '1')
						printf("\t%s",
							flagdesc['e' - 'a']);
				}
			}
			else {
				/*
				 * Standard flag: print description and
				 * operand value if present.
				 */
				printf("\t%s", flagdesc[*p - 'a']);

				if (*++p) {
					NONBLANK(p);
					printf("\t%s", p);
				}
			}
		}
		if (!flag)
			printf("\tnone\n");
	}
	else
		read_to(BUSERTXT);

	if (HADT) {
		text = 0;
		printf("\nDescription --\n");
		while (((p = lineread(NOEOF)) != NULL) && *p != CTLCHAR) {
			text = 1;
			printf("\t%s",p);
		}
		if (!text)
			printf("\tnone\n");
	}
	else
		read_to(EUSERTXT);

	if (HADB) {
		printf("\n");
		while (p = lineread(EOF))
			if (*p == CTLCHAR)
				printf("*** %s", ++p);
			else
				printf("\t%s", p);
	}

	fclose(iptr);
}


static void 
getdel(register struct delent *delp, register char *lp)
{
	lp += 2;
	NONBLANK(lp);
	delp->type = *lp++;
	NONBLANK(lp);
	delp->osid = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->datetime = lp;
	BLANK(lp);
	NONBLANK(lp);
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pgmr = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->serial = lp;
	BLANK(lp);
	*lp++ = '\0';
	NONBLANK(lp);
	delp->pred = lp;
	repl(lp,'\n','\0');
}


static char *
read_to(int ch)
{
	char *n;

	while (((n = lineread(NOEOF)) != NULL) &&
			!(*n++ == CTLCHAR && *n == ch))
		continue;

	return(n);
}


static char *
lineread(register int eof)
{
	char	buf[512];
	size_t	read, used = 0;

	do {
		if (fgets(buf, sizeof(buf), iptr) == NULL) {
			if (!eof) {
				fatal("premature eof (co5)");
			}
			return (used) ? line : NULL;
		}

		read = strlen(buf);

		if ((used + read) >= line_size) {
			line_size += sizeof(buf);
			line = (char*) realloc(line, line_size);
			if (line == NULL) {
				fatal("OUT OF SPACE (ut9)");
			}
		}

		strcpy(line + used, buf);
		used += read;
	} while (line[used - 1] != '\n');

	linenum++;

	return line;
}


static void 
printdel(register char *file, register struct delent *delp)
{
	printf("\n");

	if (prefix) {
		statistics[length(statistics) - 1] = '\0';
		printf("%s:\t",file);
	}

	printf("%c %s\t%s %s\t%s %s\t%s",delp->type,delp->osid,
		delp->datetime,delp->pgmr,delp->serial,delp->pred,statistics);
}


/*ARGSUSED*/
static void 
printit(register char *file, register char *str, register char *cp)
{
	cp++;
	NONBLANK(cp);

	if (prefix) {
		cp[length(cp) - 1] = '\0';
		printf(" ");
	}

	printf("%s%s",str,cp);
}

/* for fatal() */
void 
clean_up(void)
{
}

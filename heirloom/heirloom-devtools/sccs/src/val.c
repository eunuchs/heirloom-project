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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from val.c 1.22 06/12/12
 */

/*	from OpenSolaris "val.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)val.c	1.5 (gritter) 01/21/07
 */
/*	from OpenSolaris "sccs:cmd/val.c"	*/
/************************************************************************/
/*									*/
/*  val -                                                               */
/*  val [-mname] [-rSID] [-s] [-ytype] file ...                         */
/*                                                                      */
/************************************************************************/

# include	<defines.h>
# include	<had.h>
# include	<i18n.h>
# include	<ccstypes.h>

# define	FILARG_ERR	0200	/* no file name given */
# define	UNKDUP_ERR	0100	/* unknown or duplicate keyletter */
# define	CORRUPT_ERR	040	/* corrupt file error code */
# define	FILENAM_ERR	020	/* file name error code */
# define	INVALSID_ERR	010	/* invalid or ambiguous SID error  */
# define	NONEXSID_ERR	04	/* non-existent SID error code */
# define	TYPE_ERR	02	/* type arg value error code */
# define	NAME_ERR	01	/* name arg value error code */
# define	TRUE		1
# define	FALSE		0
# define	BLANK(l)	while (!(*l == ' ' || *l == '\t')) l++;

static int	ret_code;	/* prime return code from 'main' program */
static int	inline_err=0;	/* input line error code (from 'process') */
static int	infile_err=0;	/* file error code (from 'validate') */
static int	inpstd;		/* TRUE = args from standard input */

static struct packet gpkt;

static char	path[FILESIZE];	/* storage for file name value */
char	sid[50];	/* storage for sid (-r) value */
static char	type[50];	/* storage for type (-y) value */
static char	name[50];	/* storage for name (-m) value */
static char	*Argv[BUFSIZ];
static char	line[BUFSIZ];
static char	save_line[BUFSIZ];

static struct delent {		/* structure for delta table entry */
	char type;
	char *osid;
	char *datetime;
	char *pgmr;
	char *serial;
	char *pred;
} del;

static void	process(char *, int, char *[]);
static void	validate(char *, char *, char *, char *);
static void	getdel(register struct delent *, register char *);
static void	read_to(int, register struct packet *);
static void	report(register int, register char *, register char *);
static int	invalid(register char *);
static char	*get_line(register struct packet *);
static void	s_init(register struct packet *, register char *);
static int	read_mod(register struct packet *);
static void	add_q(struct packet *, int, int, int, int);
static void	rem_q(register struct packet *, int);
static void	set_keep(register struct packet *);
static int	chk_ix(register struct queue *, struct queue *);
static int	do_delt(register struct packet *, register int, register char *);
static int	getstats(register struct packet *);

/* This is the main program that determines whether the command line
 * comes from the standard input or read off the original command
 * line.  See VAL(I) for more information.
*/

int 
main(int argc, char *argv[])
{
	FILE	*iop;
	register int j;
	char *lp;

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;


	ret_code = 0;
	if (argc == 2 && argv[1][0] == '-' && !(argv[1][1])) {
		inpstd = TRUE;
		iop = stdin;		/* read from standard input */
		while (fgets(line,BUFSIZ,iop) != NULL) {
		   if (line[0] != '\n') {
		      repl(line,'\n','\0');
		      strcpy(save_line,line);
		      argv = Argv;
		      *argv++ = "val";
		      lp = save_line;
		      argc = 1;
		      while ( *lp != '\0' ) {
			 while ((*lp == ' ') || (*lp == '\t'))
			    *lp++ = '\0'; 
			 *argv++ = lp++;
			 argc++;
			 while ( (*lp != ' ')&&(*lp != '\t')&&(*lp != '\0') )
			    lp++;
		     }
		     *argv = NULL;
		     argv = Argv;
		     process(line,argc,argv);
		     ret_code |= inline_err;
		   } 
		}
	}
	else {
		inpstd = FALSE;
		for (j = 1; j < argc; j++)
			sprintf(line,"%s",argv[j]);
		j = strlen(line);
		line[j > 0 ? j : 0] = 0;
		process(line,argc,argv);
		ret_code = inline_err;
	}

	return (ret_code);
}


/* This function processes the line sent by the main routine.  It
 * determines which keyletter values are present on the command
 * line and assigns the values to the correct storage place.  It
 * then calls validate for each file name on the command line
 * It will return to main if the input line contains an error,
 * otherwise it returns any error code found by validate.
*/

static void 
process(char *p_line, int argc, char *argv[])
{
	register int	j;
	register int	line_sw;
	register char   *p;

	int	silent;
	int	num_files;

	char	*filelist[50];
	char	*savelinep;
	int 	c;

	int current_optind;
	
	silent = FALSE;
	path[0] = sid[0] = type[0] = name[0] = 0;
	num_files = inline_err = 0;

	/*
	make copy of 'line' for use later
	*/
	savelinep = p_line;
	/*
	clear out had flags for each 'line' processed
	*/
	for (j = 0; j < 27; j++)
		had[j] = 0;
	/*
	execute loop until all characters in 'line' are checked.
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
			      argv[j+1] = NULL;
			   }
			}
			j = current_optind;
		        c = getopt(argc, argv, "-r:sm:y:");

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
				case 's':
					silent = TRUE;
					break;
				case 'r':
					strcpy(sid,p);
					break;
				case 'y':
					strcpy(type,p);
					break;
				case 'm':
					strcpy(name,p);
					break;
				default:
					Fflags &= ~FTLEXIT;
					inline_err |= UNKDUP_ERR;
			}
			/*
			use 'had' array and determine if the keyletter
			was given twice.
			*/
			if (had[c - 'a']++)
				inline_err |= UNKDUP_ERR;
	}

	for(j=1; j<argc; j++){
		if(argv[j]) {
			if (num_files >= 50)
				fatal("too many files (va1)");
			filelist[num_files] = argv[j];
		        num_files++;
		}
	}
	/*
	check if any files were named as arguments
	*/
	if (num_files == 0)
		inline_err |= FILARG_ERR;
	/*
	check for error in command line.
	*/
	if (inline_err) {
	   if (!silent) {
	      if (inpstd)
		 report(inline_err,savelinep,"");
	      else
		 report(inline_err,"","");
	   }
	   return;		/* return to 'main' routine */
	}
	line_sw = 1;		/* print command line flag */
	/*
	loop through 'validate' for each file on command line.
	*/
	for (j = 0; j < num_files; j++) {
		/*
		read a file from 'filelist' and place into 'path'.
		*/
		if (size(filelist[j]) > FILESIZE) {
			extern char *Ffile;
			Ffile = filelist[j];
			fatal("too long (co7)");
		}
		strcpy(path, filelist[j]);
		validate(path,sid,type,name);
		inline_err |= infile_err;
		/*
		check for error from 'validate' and call 'report'
		depending on 'silent' flag.
		*/
		if (infile_err && !silent) {
			if (line_sw && inpstd) {
				report(infile_err,savelinep,path);
				line_sw = 0;
			}
			else report(infile_err,"",path);
		}
	}
	return;		/* return to 'main' routine */
}


/* This function actually does the validation on the named file.
 * It determines whether the file is an SCCS-file or if the file
 * exists.  It also determines if the values given for type, SID,
 * and name match those in the named file.  An error code is returned
 * if any mismatch occurs.  See VAL(I) for more information.
*/

static void 
validate(char *c_path, char *c_sid, char *c_type, char *c_name)
{
	register char	*l;
	int	goods,goodt,goodn,hadmflag;

	infile_err = goods = goodt = goodn = hadmflag = 0;
	s_init(&gpkt,c_path);
	if (!sccsfile(c_path) || (gpkt.p_iop = fopen(c_path,"r")) == NULL)
		infile_err |= FILENAM_ERR;
	else {
		l = get_line(&gpkt);		/* read first line in file */
		/*
		check that it is header line.
		*/
		if (l == NULL || *l++ != CTLCHAR || *l++ != HEAD) {
			infile_err |= CORRUPT_ERR;
		}
		else {
			/*
			get old file checksum count
			*/
			satoi(l,&gpkt.p_ihash);
			gpkt.p_chash = 0;
			gpkt.p_uchash = 0;
			if (HADR)
				/*
				check for invalid or ambiguous SID.
				*/
				if (invalid(c_sid))
					infile_err |= INVALSID_ERR;
			/*
			read delta table checking for errors and/or
			SID.
			*/
			if (do_delt(&gpkt,goods,c_sid)) {
				fclose(gpkt.p_iop);
				infile_err |= CORRUPT_ERR;
				return;
			}

			read_to(EUSERNAM,&gpkt);

			if (HADY || HADM) {
				/*
				read flag section of delta table.
				*/
				while (((l = get_line(&gpkt)) != NULL) &&
					*l++ == CTLCHAR &&
					*l++ == FLAG) {
					NONBLANK(l);
					repl(l,'\n','\0');
					if (*l == TYPEFLAG) {
						l += 2;
						if (equal(c_type,l))
							goodt++;
					}
					else if (*l == MODFLAG) {
						hadmflag++;
						l += 2;
						if (equal(c_name,l))
							goodn++;
					}
				}
				if (*(--l) != BUSERTXT) {
					fclose(gpkt.p_iop);
					infile_err |= CORRUPT_ERR;
					return;
				}
				/*
				check if 'y' flag matched '-y' arg value.
				*/
				if (!goodt && HADY)
					infile_err |= TYPE_ERR;
				/*
				check if 'm' flag matched '-m' arg value.
				*/
				if (HADM && !hadmflag) {
					if (!equal(auxf(sname(c_path),'g'),c_name))
						infile_err |= NAME_ERR;
				}
				else if (HADM && hadmflag && !goodn)
						infile_err |= NAME_ERR;
			}
			else read_to(BUSERTXT,&gpkt);
			read_to(EUSERTXT,&gpkt);
			gpkt.p_chkeof = 1;
			/*
			read remainder of file so 'read_mod'
			can check for corruptness.
			*/
			while (read_mod(&gpkt))
				;
		}
	fclose(gpkt.p_iop);	/* close file pointer */
	}
	return;		/* return to 'process' function */
}


/* This function reads the 'delta' line from the named file and stores
 * the information into the structure 'del'.
*/

static void 
getdel(register struct delent *delp, register char *lp)
{
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


/* This function does a read through the named file until it finds
 * the character sent over as an argument.
*/

static void 
read_to(int ch, register struct packet *pkt)
{
	register char *n;
	while (((n = get_line(pkt)) != NULL) &&
			!(*n++ == CTLCHAR && *n == ch))
		;
	return;
}


/* This function will report the error that occured on the command
 * line.  It will print one diagnostic message for each error that
 * was found in the named file.
*/

static void 
report(register int code, register char *inp_line, register char *file)
{
	char	percent;
	percent = '%';		/* '%' for -m and/or -y messages */
/* xpg4 behaviour
	if (*inp_line)
		printf("%s:\n",inp_line);
	if (code & NAME_ERR)
	   printf(" %s: %cM%c -m mismatch\n",file,percent,percent);
	if (code & TYPE_ERR)
	   printf(" %s: %cY%c -y mismatch\n",file,percent,percent);
	if (code & NONEXSID_ERR)
	   printf(" %s: SID nonexistent\n",file);
	if (code & INVALSID_ERR)
	   printf(" %s: SID invalid or ambiguous\n",file);
	if (code & FILENAM_ERR)
	   printf(" %s: can't open file or file not SCCS\n",file);
	if (code & CORRUPT_ERR)
	   printf(" %s: corrupted SCCS file\n",file);
	if (code & UNKDUP_ERR)
	   printf(" %s: Unknown or duplicate keyletter argument\n",file);
	if (code & FILARG_ERR)
		printf(" %s: missing file argument\n",file);
*/
	if (*inp_line)
		printf("%s\n\n",inp_line);
	if (code & NAME_ERR)
	   printf("    %s: %cM%c -m mismatch\n",file,percent,percent);
	if (code & TYPE_ERR)
	   printf("    %s: %cY%c -y mismatch\n",file,percent,percent);
	if (code & NONEXSID_ERR)
	   printf("    %s: SID nonexistent\n",file);
	if (code & INVALSID_ERR)
	   printf("    %s: SID invalid or ambiguous\n",file);
	if (code & FILENAM_ERR)
	   printf("    %s: can't open file or file not SCCS\n",file);
	if (code & CORRUPT_ERR)
	   printf("    %s: corrupted SCCS file\n",file);
	if (code & UNKDUP_ERR)
	   printf("    %s: Unknown or duplicate keyletter argument\n",file);
	if (code & FILARG_ERR)
		printf("    %s: missing file argument\n",file);
	return;
}


/* This function takes as its argument the SID inputed and determines
 * whether or not it is valid (e. g. not ambiguous or illegal).
*/

static int 
invalid(register char *i_sid)
{
	register int count;
	register int digits;
	count = digits = 0;
	if (*i_sid == '0' || *i_sid == '.')
		return (1);
	i_sid++;
	digits++;
	while (*i_sid != '\0') {
		if (*i_sid++ == '.') {
			digits = 0;
			count++;
			if (*i_sid == '0' || *i_sid == '.')
				return (1);
		}
		digits++;
		if (digits > 5)
			return (1);
	}
	if (*(--i_sid) == '.' )
		return (1);
	if (count == 1 || count == 3)
		return (0);
	return (1);
}


/*
	Routine to read a line into the packet.  The main reason for
	it is to make sure that pkt->p_wrttn gets turned off,
	and to increment pkt->p_slnno.
*/

static char *
get_line(register struct packet *pkt)
{
	char	buf[DEF_LINE_SIZE];
	int	eof;
	register size_t read = 0;
	register size_t used = 0;
	register signed char *p;
	register unsigned char *u_p;

	/* read until EOF or newline encountered */
	do {
		if (!(eof = (fgets(buf, sizeof(buf), pkt->p_iop) == NULL))) {
			read = strlen(buf);

			if ((used + read) >=  pkt->p_line_size) {
				pkt->p_line_size += sizeof(buf);
				pkt->p_line = (char*) realloc(pkt->p_line, pkt->p_line_size);
				if (pkt->p_line == NULL) {
					fatal("OUT OF SPACE (ut9)");
				}
			}

			strcpy(pkt->p_line + used, buf);
			used += read;
		}
	} while (!eof && (pkt->p_line[used-1] != '\n'));

	/* check end of file condition */
	if (eof && (used == 0)) {
		if (!pkt->p_chkeof) {
			infile_err |= CORRUPT_ERR;
		}
		if (pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash)&0xFFFF) {
		   if (pkt->do_chksum && (pkt->p_uchash ^ pkt->p_ihash)&0xFFFF) {
		   	infile_err |= CORRUPT_ERR;
		   }
		}
		return NULL;
	}

	/* increment line number */
	if (pkt->p_line[used-1] == '\n') {
		pkt->p_slnno++;
	}

	/* update check sum */
	for (p = (signed char *)pkt->p_line,u_p = (unsigned char *)pkt->p_line; *p; ) {
		pkt->p_chash += *p++;
		pkt->p_uchash += *u_p++;
	}

	return pkt->p_line;
}


/*
	Does initialization for sccs files and packet.
*/

static void 
s_init(register struct packet *pkt, register char *file)
{

	zero((char*) pkt, sizeof(*pkt));
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
}

static int 
read_mod(register struct packet *pkt)
{
	register char *p;
	int ser;
	int iord;
	register struct apply *ap;

	while (get_line(pkt) != NULL) {
		p = pkt->p_line;
		if (*p++ != CTLCHAR)
			continue;
		else {
			if (!((iord = *p++) == INS || iord == DEL || iord == END)) {
				infile_err |= CORRUPT_ERR;
				return(0);
			}
			NONBLANK(p);
			satoi(p,&ser);
			if (iord == END)
				rem_q(pkt,ser);
			else if (pkt->p_apply != 0) {
				ap = &pkt->p_apply[ser];
				if (ap->a_code == APPLY)
					add_q(pkt,ser,iord == INS ? YES : NO,
					    iord,ap->a_reason & USER);
				else
					add_q(pkt,ser,iord == INS ? NO : 0,
					    iord,ap->a_reason & USER);
			} else
				add_q(pkt,ser,iord == INS ? NO : 0,iord,0);
		}
	}
	if (pkt->p_q){
		infile_err |= CORRUPT_ERR;
	}
	return(0);
}

static void 
add_q(struct packet *pkt, int ser, int keep, int iord, int user)
{
	register struct queue *cur, *prev, *q;

	for (cur = (struct queue *) (&pkt->p_q); (cur = (prev = cur)->q_next) != NULL; )
		if (cur->q_sernum <= ser)
			break;
	if (cur && cur != (struct queue *)&pkt->p_q && cur->q_sernum == ser)
		infile_err |= CORRUPT_ERR;
	prev->q_next = q = (struct queue *) fmalloc(sizeof(*q));
	q->q_next = cur;
	q->q_sernum = ser;
	q->q_keep = keep;
	q->q_iord = iord;
	q->q_user = user;
	if (pkt->p_ixuser && (q->q_ixmsg = chk_ix(q,pkt->p_q)))
		++(pkt->p_ixmsg);
	else
		q->q_ixmsg = 0;

	set_keep(pkt);
}

static void 
rem_q(register struct packet *pkt, int ser)
{
	register struct queue *cur, *prev;

	for (cur = (struct queue *) (&pkt->p_q); (cur = (prev = cur)->q_next) != NULL; )
		if (cur->q_sernum == ser)
			break;
	if (cur && cur != (struct queue *)&pkt->p_q ) {
		if (cur->q_ixmsg)
			--(pkt->p_ixmsg);
		prev->q_next = cur->q_next;
		ffree((char *) cur);
		set_keep(pkt);
	}
	else {
		infile_err |= CORRUPT_ERR;
	}
}

static void 
set_keep(register struct packet *pkt)
{
	register struct queue *q;
	register struct sid *sp;

	for (q = pkt->p_q; q; q = q->q_next )
		if (q->q_keep != 0) {
			if ((pkt->p_keep = q->q_keep) == YES) {
				sp = &pkt->p_idel[q->q_sernum].i_sid;
				pkt->p_inssid.s_rel = sp->s_rel;
				pkt->p_inssid.s_lev = sp->s_lev;
				pkt->p_inssid.s_br = sp->s_br;
				pkt->p_inssid.s_seq = sp->s_seq;
			}
			return;
		}
	pkt->p_keep = NO;
}


# define apply(qp)	((qp->q_iord == INS && qp->q_keep == YES) || \
			 (qp->q_iord == DEL && qp->q_keep == NO))
static int 
chk_ix(register struct queue *new, struct queue *head)
{
	register int retval;
	register struct queue *cur;
	int firstins, lastdel;

	if (!apply(new))
		return(0);
	for (cur = head; cur; cur = cur->q_next)
		if (cur->q_user)
			break;
	if (!cur)
		return(0);
	retval = 0;
	firstins = 0;
	lastdel = 0;
	for (cur = head; cur; cur = cur->q_next) {
		if (apply(cur)) {
			if (cur->q_iord == DEL)
				lastdel = cur->q_sernum;
			else if (firstins == 0)
				firstins = cur->q_sernum;
		}
		else if (cur->q_iord == INS)
			retval++;
	}
	if (retval == 0) {
		if (lastdel && (new->q_sernum > lastdel))
			retval++;
		if (firstins && (new->q_sernum < firstins))
			retval++;
	}
	return(retval);
}


/* This function reads the delta table entries and checks for the format
 * as specifed in sccsfile(V).  If the format is incorrect, a corrupt
 * error will be issued by 'val'.  This function also checks
 * if the sid requested is in the file (depending if '-r' was specified).
*/

static int 
do_delt(register struct packet *pkt, register int goods, register char *d_sid)
{
	char *l;

	while(getstats(pkt)) {
		if (((l = get_line(pkt)) != NULL) && *l++ != CTLCHAR || *l++ != BDELTAB)
			return(1);
		if (HADR && !(infile_err & INVALSID_ERR)) {
			getdel(&del,l);
			if (equal(d_sid,del.osid) && del.type == 'D')
				goods++;
		}
		while ((l = get_line(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch(pkt->p_line[1]) {
				case EDELTAB:
					break;
				case COMMENTS:
				case MRNUM:
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
					continue;
				default:
					return(1);
				}
				break;
			}
		if (l == NULL || pkt->p_line[0] != CTLCHAR)
			return(1);
	}
	if (pkt->p_line[1] != BUSERNAM)
		return(1);
	if (HADR && !goods && !(infile_err & INVALSID_ERR))
		infile_err |= NONEXSID_ERR;
	return(0);
}


/* This function reads the stats line from the sccsfile */

static int 
getstats(register struct packet *pkt)
{
	register char *p = get_line(pkt);
	if ( p == NULL || *p++ != CTLCHAR || *p != STATS)
		return(0);
	return(1);
}

/* for fatal() */
void 
clean_up(void)
{
}

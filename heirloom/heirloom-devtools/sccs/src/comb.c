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
 * from comb.c 1.15 06/12/12
 */

/*	from OpenSolaris "comb.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)comb.c	1.4 (gritter) 01/21/07
 */
/*	from OpenSolaris "sccs:cmd/comb.c"	*/
# include	<defines.h>
# include	<had.h>
# include       <i18n.h>

struct stat Statbuf;
char SccsError[MAXERRORLEN];

struct sid sid;

static struct packet gpkt;
static int	num_files;
static int	Do_prs;
static char	*clist;
static char	*Val_ptr;
static char	Blank[]    =    " ";
static int	*Cvec;
static int	Cnt;
static FILE	*iop;

static void	comb(char *);
static int	getpred(struct idel *, int *, int);
static struct sid	*prtget(struct idel *, int, FILE *, char *);

int 
main(int argc, register char *argv[])
{
	register int i;
	register char *p;
	int  c;
	int testmore;
	extern int Fcnt;
	int current_optind;

	Fflags = FTLEXIT | FTLMSG | FTLCLN;

	current_optind = 1;
	optind = 1;
	opterr = 0;
	i = 1;
	/*CONSTCOND*/
	while (1) {
			if(current_optind < optind) {
			   current_optind = optind;
			   argv[i] = 0;
			   if (optind > i+1 ) {
			      argv[i+1] = NULL;
			   }
			}
			i = current_optind;
		        c = getopt(argc, argv, "-p:c:os");

				/* this takes care of options given after
				** file names.
				*/
			if((c == EOF)) {
			   if (optind < argc) {
				/* if it's due to -- then break; */
			       if(argv[i][0] == '-' &&
				      argv[i][1] == '-') {
			          argv[i] = 0;
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
			testmore = 0;
			switch (c) {

			case 'p':
				if (!p[0]) {
					argv[i] = 0;
					continue;
				}
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'c':
				clist = p;
				break;
			case 'o':
				testmore++;
				break;
			case 's':
				testmore++;
				break;
			default:
				fatal("unknown key letter (cm1)");
			}

			if (testmore) {
				testmore = 0;
				if (p) {
				   if (*p) {
				        sprintf(SccsError,
						"value after %c arg (cm7)",
						c);
					fatal(SccsError);
				   }
				}
			}
			if (had[c - 'a']++)
				fatal("key letter twice (cm2)");
	}

	for(i=1; i<argc; i++){
		if(argv[i]) {
		       num_files++;
		}
	}
	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (HADP && HADC)
		fatal("can't have both -p and -c (cb2)");
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	iop = stdout;
	for (i = 1; i < argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, comb, 1);
	fclose(iop);

	return (Fcnt ? 1 : 0);
}

static void 
comb(char *file)
{
	register int i, n;
	register struct idel *rdp;
	char *p;
	char rarg[32];
	int succnt;
	struct sid *sp;
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	struct stats stats;

	if (setjmp(Fjmp))
		return;
	sinit(&gpkt, file, 1);
	gpkt.p_verbose = -1;
	gpkt.p_stdout = stderr;
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (exists(auxf(gpkt.p_file, 'p')))
		fatal("p-file exists (cb1)");

	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);

	Cvec = (int *) fmalloc((unsigned) (n = ((maxser(&gpkt) + 1) * sizeof(*Cvec))));
	zero((char *) Cvec, n);
	Cnt = 0;

	if (HADP) {
		rdp = gpkt.p_idel;
		if (!(n = sidtoser(&sid, &gpkt)))
			fatal("sid doesn't exist (cb3)");
		while (n <= maxser(&gpkt)) {
			if (rdp[n].i_sid.s_rel == 0 &&
			    rdp[n].i_sid.s_lev == 0 &&
			    rdp[n].i_sid.s_br == 0  &&
			    rdp[n].i_sid.s_seq == 0) {
				n++;
				continue;
			}
			Cvec[Cnt++] = n++;
		}
	}
	else if (HADC) {
		dolist(&gpkt, clist, 0);
	}
	else {
		rdp = gpkt.p_idel;
		for (i = 1; i <= maxser(&gpkt); i++) {
			succnt = 0;
			if (rdp[i].i_sid.s_rel == 0 &&
			    rdp[i].i_sid.s_lev == 0 &&
			    rdp[i].i_sid.s_br == 0  &&
			    rdp[i].i_sid.s_seq == 0)
				continue;
			for (n = i + 1; n <= maxser(&gpkt); n++)
				if (rdp[n].i_pred == i)
					succnt++;
			if (succnt != 1)
				Cvec[Cnt++] = i;
		}
	}
	finduser(&gpkt);
	doflags(&gpkt);
	fclose(gpkt.p_iop);
	gpkt.p_iop = 0;
	if (!Cnt)
		fatal("nothing to do (cb4)");
	rdp = gpkt.p_idel;
	Do_prs = 0;
	fprintf(iop,"trap \"rm -f COMB$$ comb$$ s.COMB$$; exit 2\" 1 2 3 15\n");
	sp = prtget(rdp, Cvec[0], iop, gpkt.p_file);
	sid_ba(sp,rarg);
	if ((Val_ptr = Sflags[VALFLAG - 'a']) == NULL)
		Val_ptr = Blank;
	fprintf(iop, "v=`prs -r%s -d:MR: %s`\n", rarg, gpkt.p_file);
	fprintf(iop, "if test \"$v\"\n");
	fprintf(iop, "then\n");	
	fprintf(iop, "admin -iCOMB$$ -r%s -fv%s -m\"$v\" -y'This was COMBined' s.COMB$$\n", rarg,Val_ptr);
	fprintf(iop, "else\n");
	fprintf(iop, "admin -iCOMB$$ -r%s -y'This was COMBined' s.COMB$$\n", rarg);
	fprintf(iop, "fi\n");
	Do_prs = 1;
	fprintf(iop, "rm -f COMB$$\n");
#if defined(BUG_1205145) || defined(GMT_TIME)
	fprintf(iop, "TZ=GMT\n");
	fprintf(iop, "export TZ\n");
#endif
	for (i = 1; i < Cnt; i++) {
		n = getpred(rdp, Cvec, i);
		if (HADO)
			fprintf(iop, "get -s -r%d -g -e -t s.COMB$$\n",
				rdp[Cvec[i]].i_sid.s_rel);
		else
			fprintf(iop, "get -s -a%d -r%d -g -e s.COMB$$\n",
				n + 1, rdp[Cvec[i]].i_sid.s_rel);
		prtget(rdp, Cvec[i], iop, gpkt.p_file);
		fprintf(iop, "if test \"$b\"\n");
		fprintf(iop, "then\n");	        
		fprintf(iop, "delta -s -m\"$b\" -y\"$a\" s.COMB$$\n");
		fprintf(iop, "admin -fv s.COMB$$\n");
		fprintf(iop, "else\n");
		fprintf(iop, "delta -s -y\"$a\" s.COMB$$ </dev/null\n");
		fprintf(iop, "fi\n");
		fprintf(iop, "c1=`prs -d:D: s.COMB$$`\n");
		fprintf(iop, "d1=`prs -d:T: s.COMB$$`\n");
		fprintf(iop, "e1=`prs -d:P: s.COMB$$`\n");
		fprintf(iop, "s=`prs -d:I: s.COMB$$`\n");
		fprintf(iop, "sed '/d D '$s'/s|'$c1'|'$c'|' s.COMB$$ >comb$$\n");
                fprintf(iop, "rm -f s.COMB$$\n");
                fprintf(iop, "sed '/d D '$s'/s|'$d1'|'$d'|' comb$$ >COMB$$\n");
     		fprintf(iop, "sed '/d D '$s'/s|'$e1'|'$e'|' COMB$$ >s.COMB$$\n");
               	fprintf(iop, "if test \"$v\"\n");
	        fprintf(iop, "then\n");
	        fprintf(iop, "admin -z -fv s.COMB$$\n");
	        fprintf(iop, "else\n");
		fprintf(iop, "admin -z s.COMB$$\n");
		fprintf(iop, "fi\n");
	}
	fprintf(iop, "sed -n '/^%c%c$/,/^%c%c$/p' %s >comb$$\n",
		CTLCHAR, BUSERTXT, CTLCHAR, EUSERTXT, gpkt.p_file);
	fprintf(iop, "ed - comb$$ <<\\!\n");
	fprintf(iop, "1d\n");
	fprintf(iop, "$c\n");
	fprintf(iop, "*** DELTA TABLE PRIOR TO COMBINE ***\n");
	fprintf(iop, ".\n");
	fprintf(iop, "w\n");
	fprintf(iop, "q\n");
	fprintf(iop, "!\n");
	fprintf(iop, "prs -e %s >>comb$$\n", gpkt.p_file);
	fprintf(iop, "admin -tcomb$$ s.COMB$$\\\n");
	for (i = 0; i < NFLAGS; i++)
		if ((p = Sflags[i]) != NULL)
		   if ( i != ('e'-'a') )
			fprintf(iop, " -f%c%s\\\n", i + 'a', p);
	fprintf(iop, "\n");
	fprintf(iop, "sed -n '/^%c%c$/,/^%c%c$/p' %s >comb$$\n",
		CTLCHAR, BUSERNAM, CTLCHAR, EUSERNAM, gpkt.p_file);
	fprintf(iop, "ed - comb$$ <<\\!\n");
	fprintf(iop, "v/^%c/s/.*/ -a& \\\\/\n", CTLCHAR);
	fprintf(iop, "1c\n");
	fprintf(iop, "admin s.COMB$$\\\n");
	fprintf(iop, ".\n");
	fprintf(iop, "$c\n");
	fprintf(iop, "\n");
	fprintf(iop, ".\n");
	fprintf(iop, "w\n");
	fprintf(iop, "q\n");
	fprintf(iop, "!\n");
	fprintf(iop, ". comb$$\n");
	fprintf(iop, "rm comb$$\n");
	if (!HADS) {
		fprintf(iop, "rm -f %s\n", gpkt.p_file);
		fprintf(iop, "mv s.COMB$$ %s\n", gpkt.p_file);
		if (!Sflags[VALFLAG - 'a'])
			fprintf(iop, "admin -dv %s\n", gpkt.p_file);
	} else {
		fprintf(iop, "set `ls -st s.COMB$$ %s`\n",gpkt.p_file);
		fprintf(iop, "c=`expr 100 - 100 '*' $1 / $3`\n");
		fprintf(iop, "echo '%s\t' ${c}'%%\t' $1/$3\n", gpkt.p_file);
		fprintf(iop, "rm -f s.COMB$$\n");
	}
}

/*ARGSUSED*/
void 
enter(struct packet *pkt, int ch, int n, struct sid *sidp)
{
	Cvec[Cnt++] = n;
}


static struct sid *
prtget(
struct idel *idp,
int ser,
FILE *fptr,
char *file)
{
	char buf[32];
	struct sid *sp;

	sid_ba(sp = &idp[ser].i_sid, buf);
	fprintf(fptr, "get -s -k -r%s -p %s > COMB$$\n", buf, file);
	if (Do_prs) {
		fprintf(fptr, "a=`prs -r%s -d:C: %s`\n",buf,file);
		fprintf(fptr, "b=`prs -r%s -d:MR: %s`\n",buf,file);
		fprintf(fptr, "c=`prs -r%s -d:D: %s`\n",buf,file);
		fprintf(fptr, "d=`prs -r%s -d:T: %s`\n",buf,file);
		fprintf(fptr, "e=`prs -r%s -d:P: %s`\n",buf,file);
	}
	return(sp);
}


static int 
getpred(struct idel *idp, int *vec, int i)
{
	int ser, pred, acpred;

	ser = vec[i];
	while (--i) {
		pred = vec[i];
		for (acpred = idp[ser].i_pred; acpred; acpred = idp[acpred].i_pred)
			if (pred == acpred)
				break;
		if (pred == acpred)
			break;
	}
	return(i);
}

void 
clean_up(void)
{
	ffreeall();
}

/*ARGSUSED*/
void
escdodelt(struct packet *pkt)	/* dummy for dodelt() */
{
}

/*ARGSUSED*/
void
fredck(struct packet *pkt) /*dummy for dodelt() */
{
}

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
 * from get.c 1.59 06/12/12
 */

/*	from OpenSolaris "get.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)get.c	1.8 (gritter) 2/13/07
 */
/*	from OpenSolaris "sccs:cmd/get.c"	*/

#include	<defines.h>
#include	<had.h>
#include	<i18n.h>
#include	<sys/utsname.h>
#include	<ccstypes.h>
#include	<limits.h>

#define	DATELEN	12

/*
 * Get is now much more careful than before about checking
 * for write errors when creating the g- l- p- and z-files.
 * However, it remains deliberately unconcerned about failures
 * in writing statistical information (e.g., number of lines
 * retrieved).
 */

struct stat Statbuf;
char Null[1];
char SccsError[MAXERRORLEN];

struct sid sid;
int	Did_id;

static struct packet gpkt;
static unsigned	Ser;
static int	num_files;
static int	num_ID_lines;
static int	cnt_ID_lines;
static int	expand_IDs;
static char	*list_expand_IDs;
static char    *Whatstr = NULL;
static char	Pfilename[FILESIZE];
static char	*ilist, *elist, *lfile;
static time_t	cutoff = (time_t)0X7FFFFFFFL;	/* max positive long */
static char	Gfile[PATH_MAX];
static char	gfile[PATH_MAX];
static char	*Type;
static struct utsname un;
static char *uuname;
static char	*Prefix = "";

/* Beginning of modifications made for CMF system. */
#define CMRLIMIT 128
static char	cmr[CMRLIMIT];
static int	cmri = 0;
/* End of insertion */

static void	get(char *);
static void	gen_lfile(register struct packet *);
static void	idsetup(register struct packet *);
static void	makgdate(register char *, register char *);
static char	*idsubst(register struct packet *, char []);
static char	*_trans(register char *, register char *, register char *);
static void	prfx(register struct packet *);
static void	wrtpfile(register struct packet *, char *, char *);
static int	cmrinsert(void);

int 
main(int argc, register char *argv[])
{
	register int i;
	register char *p;
	int  c;
	extern int Fcnt;
	int current_optind;
	int no_arg;

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
	current_optind = 1;
	optind = 1;
	opterr = 0;
	no_arg = 0;
	i = 1;
	/*CONSTCOND*/
	while (1) {
			if (current_optind < optind) {
			   current_optind = optind;
			   argv[i] = 0;
			   if (optind > i+1) {
			      if ((argv[i+1][0] != '-') && (no_arg == 0)) {
			         argv[i+1] = NULL;
			      } else {
				 optind = i+1;
			         current_optind = optind;
			      }
			   }
			}
			no_arg = 0;
			i = current_optind;
		        c = getopt(argc, argv,
				"-r:c:ebi:x:kl:Lpsmngta:G:w:zqdP:");
				/* this takes care of options given after
				** file names.
				*/
			if (c == EOF) {
			   if (optind < argc) {
			      /* if it's due to -- then break; */
			      if (argv[i][0] == '-' &&
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
			switch (c) {
    			case CMFFLAG:
				/* Concatenate the rest of this argument with 
				 * the existing CMR list. */
				if (p) {
				   while (*p) {
				      if (cmri == CMRLIMIT)
					 fatal("CMR list is too long.");
					 cmr[cmri++] = *p++;
				   }
				}
				cmr[cmri] = 0;
				break;
			case 'a':
				if (*p == 0) continue;
				Ser = patoi(p);
				break;
			case 'r':
				if (argv[i][2] == '\0') {
				   if (*(omit_sid(p)) != '\0') {
				      no_arg = 1;
				      continue;
				   }
				}
				chksid(sid_ab(p, &sid), &sid);
				if ((sid.s_rel < MINR) || (sid.s_rel > MAXR)) {
				   fatal("r out of range (ge22)");
				}
				break;
			case 'w':
				if (p) Whatstr = p;
				break;
			case 'c':
				if (*p == 0) continue;
				if (parse_date(p,&cutoff))
				   fatal("bad date/time (cm5)");
				break;
			case 'L':
				/* treat this as if -lp was given */
				lfile = NOGETTEXT("stdout");
				c = 'l';
				break;
			case 'l':
				if (argv[i][2] != '\0') {
				   if ((*p == 'p') && (strlen(p) == 1)) {
				      p = NOGETTEXT("stdout");
				   }
				   lfile = p;
				} else {
				   no_arg = 1;
				   lfile = NULL;
				}	
				break;
			case 'i':
				if (*p == 0) continue;
				ilist = p;
				break;
			case 'x':
				if (*p == 0) continue;
				elist = p;
				break;
			case 'G':
				{
				char *cp;

				if (*p == 0) continue;
				copy(p, gfile);
 				cp = auxf(p, 'G');
				copy(cp, Gfile);
				break;
				}
			case 'b':
			case 'g':
			case 'e':
			case 'p':
			case 'k':
			case 'm':
			case 'n':
			case 's':
			case 't':
			case 'd':
				if (p) {
				   sprintf(SccsError,
				     "value after %c arg (cm8)", 
				     c);
				   fatal(SccsError);
				}
				break;
                        case 'q': /* enable NSE mode */
				if (p) {
                                   if (*p) {
                                      nsedelim = p;
				   }
                                } else {
                                   nsedelim = (char *) 0;
                                }
                                break;
			case 'P':
				Prefix = p;
				break;
			default:
			   fatal("unknown key letter (cm1)");
			
			}

			/* The following is necessary in case the */
			/* user types some localized character,  */
			/* which will exceed the limits of the */
			/* array "had", defined in ../hdr/had.h . */
			/* This guard is also necessary in case the */
			/* user types a capital ascii character, in */
			/* which case the had[] array reference will */
			/* be out of bounds.  */
			if (!((c - 'a') < 0 || (c - 'a') > 25)) {
			   if (had[c - 'a']++)
			      fatal("key letter twice (cm2)");
			}
	}
	for (i=1; i<argc; i++) {
		if (argv[i]) {
		       num_files++;
		}
	}
	if(num_files == 0)
		fatal("missing file arg (cm3)");
	if (HADE && HADM)
		fatal("e not allowed with m (ge3)");
	if (HADE)
		HADK = 1;
	if (HADE && !logname())
		fatal("User ID not in password file (cm9)");
	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, get, 1);

	return (Fcnt ? 1 : 0);
}

extern char *Sflags[];

static void 
get(char *file)
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp;
	struct stats stats;
	char	str[32];
	char template[] = NOGETTEXT("/get.XXXXXX");
	char buf1[PATH_MAX];
	uid_t holduid;
	gid_t holdgid;
	static int first = 1;

	if (setjmp(Fjmp))
		return;
	if (HADE) {
		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		pass both the PID and machine name to lockit
		*/
		sinit(&gpkt,file,0);
		uname(&un);
		uuname = un.nodename;
		if (lockit(auxf(file,'z'),10, getpid(),uuname))
			fatal("cannot create lock file (cm4)");
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(&gpkt,file,1);
	gpkt.p_ixuser = (HADI | HADX);
	gpkt.p_reqsid.s_rel = sid.s_rel;
	gpkt.p_reqsid.s_lev = sid.s_lev;
	gpkt.p_reqsid.s_br = sid.s_br;
	gpkt.p_reqsid.s_seq = sid.s_seq;
	gpkt.p_verbose = (HADS) ? 0 : 1;
	gpkt.p_stdout  = (HADP||lfile) ? stderr : stdout;
	gpkt.p_cutoff = cutoff;
	gpkt.p_lfile = lfile;
	if (Gfile[0] == 0 || !first) {
		cat(gfile,Prefix,auxf(gpkt.p_file,'g'),NULL);
		cat(Gfile,Prefix,auxf(gpkt.p_file,'A'),NULL);
	}
	strcpy(buf1, dname(Gfile));
	strcat(buf1, template);
	close(mkstemp(buf1));
	copy(buf1, Gfile);
	unlink(Gfile);
	first = 0;

	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	finduser(&gpkt);
	doflags(&gpkt);
	if ((p = Sflags[SCANFLAG - 'a']) == NULL) {
		num_ID_lines = 0;
	} else {
		num_ID_lines = atoi(p);
	}
	expand_IDs = 0;
	if ((list_expand_IDs = Sflags[EXPANDFLAG - 'a']) != NULL) {
		if (*list_expand_IDs) {
			expand_IDs = 1;
		}
	}
	if (HADE) {
		expand_IDs = 0;
	}
	if (!HADK) {
		expand_IDs = 1;
	}
	if (!HADA) {
		ser = getser(&gpkt);
	} else {
		if ((ser = Ser) > maxser(&gpkt))
			fatal("serial number too large (ge19)");
		gpkt.p_gotsid = gpkt.p_idel[ser].i_sid;
		if (HADR && sid.s_rel != gpkt.p_gotsid.s_rel) {
			zero((char *) &gpkt.p_reqsid, sizeof(gpkt.p_reqsid));
			gpkt.p_reqsid.s_rel = sid.s_rel;
		} else {
			gpkt.p_reqsid = gpkt.p_gotsid;
		}
	}
	doie(&gpkt,ilist,elist,(char *) 0);
	setup(&gpkt,(int) ser);
	if ((Type = Sflags[TYPEFLAG - 'a']) == NULL)
		Type = Null;
	if (!(HADP || HADG)) {
		if (exists(gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(SccsError,"writable `%s' exists (ge4)",
				gfile);
			fatal(SccsError);
		}
	}
	if (gpkt.p_verbose) {
		sid_ba(&gpkt.p_gotsid,str);
		fprintf(gpkt.p_stdout,"%s\n",str);
	}
	if (HADE) {
		if (HADC || gpkt.p_reqsid.s_rel == 0)
			gpkt.p_reqsid = gpkt.p_gotsid;
		newsid(&gpkt,Sflags[BRCHFLAG - 'a'] && HADB);
		permiss(&gpkt);
		wrtpfile(&gpkt,ilist,elist);
	}
	if (!HADG || HADL) {
		if (gpkt.p_stdout) {
			fflush(gpkt.p_stdout);
			fflush (stderr);
		}
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		if (HADL)
			gen_lfile(&gpkt);
		if (HADG)
			goto unlock;
		flushto(&gpkt,EUSERTXT,1);
		idsetup(&gpkt);
		gpkt.p_chkeof = 1;
		Did_id = 0;
		/*
		call `xcreate' which deletes the old `g-file' and
		creates a new one except if the `p' keyletter is set in
		which case any old `g-file' is not disturbed.
		The mod of the file depends on whether or not the `k'
		keyletter has been set.
		*/
		if (gpkt.p_gout == 0) {
			if (HADP) {
				gpkt.p_gout = stdout;
			} else {
				if (exists(gpkt.p_file) && (S_IEXEC & Statbuf.st_mode)) {
					gpkt.p_gout = xfcreat(Gfile,HADK ? 
						((mode_t)0755) : ((mode_t)0555));
				} else {
					gpkt.p_gout = xfcreat(Gfile,HADK ? 
						((mode_t)0644) : ((mode_t)0444));
				}
			}
		}
		if (Sflags[ENCODEFLAG - 'a'] &&
		    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
			while (readmod(&gpkt)) {
				decode(gpkt.p_line,gpkt.p_gout);
			}
		} else {
			while (readmod(&gpkt)) {
				prfx(&gpkt);
				p = idsubst(&gpkt,gpkt.p_line);
				if (fputs(p,gpkt.p_gout)==EOF)
					xmsg(gfile, NOGETTEXT("get"));
			}
		}
		if (gpkt.p_gout) {
			if (fflush(gpkt.p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
			fflush (stderr);
		}
		if (gpkt.p_gout && gpkt.p_gout != stdout) {
			/*
			 * Force g-file to disk and verify
			 * that it actually got there.
			 */
#ifdef	USE_FSYNC
			if (fsync(fileno(gpkt.p_gout)) < 0)
				xmsg(gfile, NOGETTEXT("get"));
#endif
			if (fclose(gpkt.p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
		}
		if (gpkt.p_verbose) {
#ifdef XPG4		
		   fprintf(gpkt.p_stdout, NOGETTEXT("%d lines\n"), gpkt.p_glnno);
#else
		   if (HADD == 0)
		      fprintf(gpkt.p_stdout,"%d lines\n",gpkt.p_glnno);
#endif
		}		
		if (!Did_id && !HADK && !HADQ) {
		   if (Sflags[IDFLAG - 'a']) {
		      if (!(*Sflags[IDFLAG - 'a'])) {
		         fatal("no id keywords (cm6)");
		      } else {
			 fatal("invalid id keywords (cm10)");
		      }
		   } else {
		      if (gpkt.p_verbose) {
			 fprintf(stderr, "No id keywords (cm7)\n");
		      }
		   }
		}
		if ( exists(Gfile) ) {
			rename(Gfile, gfile);
		}
		setuid(holduid);
		setgid(holdgid);
	}
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);
unlock:
	if (HADE) {
		copy(auxf(gpkt.p_file,'p'),Pfilename);
		rename(auxf(gpkt.p_file,'q'),Pfilename);
		uname(&un);
		uuname = un.nodename;
		unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);
	}
	ffreeall();
}

void 
enter(struct packet *pkt, int ch, int n, struct sid *sidp)
{
	char str[32];
	register struct apply *ap;

	sid_ba(sidp,str);
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"%s\n",str);
	ap = &pkt->p_apply[n];
	switch (ap->a_code) {

	case SX_EMPTY:
		if (ch == INCLUDE)
			condset(ap,APPLY,INCLUSER);
		else
			condset(ap,NOAPPLY,EXCLUSER);
		break;
	case APPLY:
		sid_ba(sidp,str);
		sprintf(SccsError, "%s already included (ge9)", str);
		fatal(SccsError);
		break;
	case NOAPPLY:
		sid_ba(sidp,str);
		sprintf(SccsError, "%s already excluded (ge10)", str);
		fatal(SccsError);
		break;
	default:
		fatal("internal error in get/enter() (ge11)");
		break;
	}
}

static void 
gen_lfile(register struct packet *pkt)
{
	char *n;
	int reason;
	char str[32];
	char line[BUFSIZ];
	struct deltab dt;
	FILE *in;
	FILE *out;
	char *outname = NOGETTEXT("stdout");

#define	OUTPUTC(c) \
	if (putc((c), out) == EOF) \
		xmsg(outname, NOGETTEXT("gen_lfile"));

	in = xfopen(pkt->p_file,0);
	if (pkt->p_lfile) {
		out = stdout;
	} else {
		outname = auxf(pkt->p_file, 'l');
		out = xfcreat(outname,(mode_t)0444);
	}
	fgets(line,sizeof(line),in);
	while (fgets(line,sizeof(line),in) != NULL &&
	       line[0] == CTLCHAR && line[1] == STATS) {
		fgets(line,sizeof(line),in);
		del_ab(line,&dt,pkt);
		if (dt.d_type == 'D') {
			reason = pkt->p_apply[dt.d_serial].a_reason;
			if (pkt->p_apply[dt.d_serial].a_code == APPLY) {
				OUTPUTC(' ');
				OUTPUTC(' ');
			} else {
				OUTPUTC('*');
				if (reason & IGNR) {
					OUTPUTC(' ');
				} else {
					OUTPUTC('*');
				}
			}
			switch (reason & (INCL | EXCL | CUTOFF)) {
	
			case INCL:
				OUTPUTC('I');
				break;
			case EXCL:
				OUTPUTC('X');
				break;
			case CUTOFF:
				OUTPUTC('C');
				break;
			default:
				OUTPUTC(' ');
				break;
			}
			OUTPUTC(' ');
			sid_ba(&dt.d_sid,str);
			if (fprintf(out, "%s\t", str) == EOF)
				xmsg(outname, NOGETTEXT("gen_lfile"));
			date_ba(&dt.d_datetime,str);
			if (fprintf(out, "%s %s\n", str, dt.d_pgmr) == EOF)
				xmsg(outname, NOGETTEXT("gen_lfile"));
		}
		while ((n = fgets(line,sizeof(line),in)) != NULL) {
			if (line[0] != CTLCHAR) {
				break;
			} else {
				switch (line[1]) {

				case EDELTAB:
					break;
				default:
					continue;
				case MRNUM:
				case COMMENTS:
					if (dt.d_type == 'D') {
					   if (fprintf(out,"\t%s",&line[3]) == EOF)
					      xmsg(outname, 
					         NOGETTEXT("gen_lfile"));
					}
					continue;
				}
				break;
			}
		}
		if (n == NULL || line[0] != CTLCHAR)
			break;
		OUTPUTC('\n');
	}
	fclose(in);
	if (out != stdout) {
#ifdef	USE_FSYNC
		if (fsync(fileno(out)) < 0)
			xmsg(outname, NOGETTEXT("gen_lfile"));
#endif
		if (fclose(out) == EOF)
			xmsg(outname, NOGETTEXT("gen_lfile"));
	}

#undef	OUTPUTC
}

static char	Curdate[18];
static char	*Curtime;
static char	Gdate[DATELEN];
static char	Chgdate[18];
static char	*Chgtime;
static char	Gchgdate[DATELEN];
static char	Sid[32];
static char	Mod[FILESIZE];
static char	Olddir[BUFSIZ];
static char	Pname[BUFSIZ];
static char	Dir[BUFSIZ];
static char	*Qsect;

static void 
idsetup(register struct packet *pkt)
{
	extern time_t Timenow;
	register int n;
	register char *p;

	date_ba(&Timenow,Curdate);
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate,Gdate);
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n) {
		date_ba(&pkt->p_idel[n].i_datetime,Chgdate);
	} else {
#ifdef XPG4		
		if (exists(gfile))
			unlink(gfile);
		xfcreat(gfile, HADK ? ((mode_t)0644) : ((mode_t)0444));
#endif
		fatal("No sid prior to cutoff date-time (ge23)");
	}
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate,Gchgdate);
	sid_ba(&pkt->p_gotsid,Sid);
	if ((p = Sflags[MODFLAG - 'a']) != NULL)
		copy(p,Mod);
	else
		copy(auxf(gpkt.p_file,'g'), Mod);
	if ((Qsect = Sflags[QSECTFLAG - 'a']) == NULL)
		Qsect = Null;
}

static void 
makgdate(register char *old, register char *new)
{
	if ((*new = old[3]) != '0')
		new++;
	*new++ = old[4];
	*new++ = '/';
	if ((*new = old[6]) != '0')
		new++;
	*new++ = old[7];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	*new = 0;
}

static char Zkeywd[5] = "@(#)";
static char *tline = NULL;
static size_t tline_size = 0;

#define trans(a, b)	_trans(a, b, lp)

static char *
idsubst(register struct packet *pkt, char line[])
{
	static char *hold = NULL;
	static size_t hold_size = 0;
	size_t new_hold_size;
	static char str[32];
	register char *lp, *tp;
	int recursive = 0;
	extern char *Type;
	extern char *Sflags[];
	char *expand_ID;
		
	cnt_ID_lines++;
	if (HADK) {
		if (!expand_IDs)
			return(line);
	} else {
		if (!any('%', line))
			return(line);
	}
	if (cnt_ID_lines > num_ID_lines) {
		if (!expand_IDs) {
			return(line);
		}
		if (num_ID_lines) {
			expand_IDs = 0;
			return(line);
		}
	}
	tp = tline;
	for (lp=line; *lp != 0; lp++) {
		if ((!Did_id) && (Sflags['i'-'a']) &&
		    (!(strncmp(Sflags['i'-'a'],lp,strlen(Sflags['i'-'a'])))))
				++Did_id;
		if (lp[0] == '%' && lp[1] != 0 && lp[2] == '%') {
			expand_ID = ++lp;
			if (!HADK && expand_IDs && (list_expand_IDs != NULL)) {
				if (!any(*expand_ID, list_expand_IDs)) {
					str[3] = 0;
					strncpy(str, lp - 1, 3);
					tp = trans(tp, str);
					lp++;
					continue;
				}
			}
			switch (*expand_ID) {

			case 'M':
				tp = trans(tp,Mod);
				break;
			case 'Q':
				tp = trans(tp,Qsect);
				break;
			case 'R':
				sprintf(str,"%d",pkt->p_gotsid.s_rel);
				tp = trans(tp,str);
				break;
			case 'L':
				sprintf(str,"%d",pkt->p_gotsid.s_lev);
				tp = trans(tp,str);
				break;
			case 'B':
				sprintf(str,"%d",pkt->p_gotsid.s_br);
				tp = trans(tp,str);
				break;
			case 'S':
				sprintf(str,"%d",pkt->p_gotsid.s_seq);
				tp = trans(tp,str);
				break;
			case 'D':
				tp = trans(tp,Curdate);
				break;
			case 'H':
				tp = trans(tp,Gdate);
				break;
			case 'T':
				tp = trans(tp,Curtime);
				break;
			case 'E':
				tp = trans(tp,Chgdate);
				break;
			case 'G':
				tp = trans(tp,Gchgdate);
				break;
			case 'U':
				tp = trans(tp,Chgtime);
				break;
			case 'Z':
				tp = trans(tp,Zkeywd);
				break;
			case 'Y':
				tp = trans(tp,Type);
				break;
			/*FALLTHRU*/
			case 'W':
				if ((Whatstr != NULL) && (recursive == 0)) {
					recursive = 1;
					lp += 2;
					new_hold_size = strlen(lp) + strlen(Whatstr) + 1;
					if (new_hold_size > hold_size) {
						if (hold)
							free(hold);
						hold_size = new_hold_size;
						hold = (char *)malloc(hold_size);
						if (hold == NULL) {
							fatal("OUT OF SPACE (ut9)");
						}
					}
					strcpy(hold,Whatstr);
					strcat(hold,lp);
					lp = hold;
					lp--;
					continue;
				}
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Mod);
				tp = trans(tp,"\t");
			case 'I':
				tp = trans(tp,Sid);
				break;
			case 'P':
				copy(pkt->p_file,Dir);
				dname(Dir);
				if (getcwd(Olddir,sizeof(Olddir)) == NULL)
				   fatal("getcwd() failed (ge20)");
				if (chdir(Dir) != 0)
				   fatal("cannot change directory (ge21)");
			  	if (getcwd(Pname,sizeof(Pname)) == NULL)
				   fatal("getcwd() failed (ge20)");
				if (chdir(Olddir) != 0)
				   fatal("cannot change directory (ge21)");
				tp = trans(tp,Pname);
				tp = trans(tp,"/");
				tp = trans(tp,(sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp,pkt->p_file);
				break;
			case 'C':
				sprintf(str,"%d",pkt->p_glnno);
				tp = trans(tp,str);
				break;
			case 'A':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Type);
				tp = trans(tp," ");
				tp = trans(tp,Mod);
				tp = trans(tp," ");
				tp = trans(tp,Sid);
				tp = trans(tp,Zkeywd);
				break;
			default:
				str[0] = '%';
				str[1] = *lp;
				str[2] = '\0';
				tp = trans(tp,str);
				continue;
			}
			if (!(Sflags['i'-'a']))
				++Did_id;
			lp++;
		} else {
			str[0] = *lp;
			str[1] = '\0';
			tp = trans(tp,str);
		}
	}
	return(tline);
}

static char *
_trans(register char *tp, register char *str, register char *rest)
{
	register size_t filled_size	= tp - tline;
	register size_t new_size	= filled_size + strlen(str) + 1;

	if (new_size > tline_size) {
		tline_size = new_size + strlen(rest) + DEF_LINE_SIZE;
		tline = (char*) realloc(tline, tline_size);
		if (tline == NULL) {
			fatal("OUT OF SPACE (ut9)");
		}
		tp = tline + filled_size;
	}
	while(*tp++ = *str++)
		;
	return(tp-1);
}

static void 
prfx(register struct packet *pkt)
{
	char str[32];

	if (HADN)
		if (fprintf(pkt->p_gout, "%s\t", Mod) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		if (fprintf(pkt->p_gout, "%s\t", str) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	}
}

void 
clean_up(void)
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_iop)
		fclose(gpkt.p_iop);
	if (gpkt.p_gout)
		fflush(gpkt.p_gout);
	if (gpkt.p_gout && gpkt.p_gout != stdout) {
		fclose(gpkt.p_gout);
		unlink(Gfile);
	}
	if (HADE) {
	   uname(&un);
	   uuname = un.nodename;
	   if (mylock(auxf(gpkt.p_file,'z'), getpid(), uuname)) {
	      unlink(auxf(gpkt.p_file,'q'));
	      unlockit(auxf(gpkt.p_file,'z'), getpid(), uuname);
	   }
	}
	ffreeall();
}

static	char	warn[] = NOGETTEXT("WARNING: being edited: `%s' (ge18)\n");

static void 
wrtpfile(register struct packet *pkt, char *inc, char *exc)
{
	char line[64], str1[32], str2[32];
	char *user, *pfile;
	FILE *in, *out;
	struct pfile pf;
	extern time_t Timenow;
	int fd;

	if ((user=logname()) == NULL)
	   fatal("User ID not in password file (cm9)");
	if ((fd=open(auxf(pkt->p_file,'q'),O_WRONLY|O_CREAT|O_EXCL,0444)) < 0) {
	   fatal("cannot create lock file (cm4)");
	}
	fchmod(fd, (mode_t)0644);
	out = fdfopen(fd, 1);
	if (exists(pfile = auxf(pkt->p_file,'p'))) {
		in = fdfopen(xopen(pfile,(mode_t)0), (mode_t)0);
		while (fgets(line,sizeof(line),in) != NULL) {
			if (fputs(line, out) == EOF)
			   xmsg(pfile, NOGETTEXT("wrtpfile"));
			pf_ab(line,&pf,0);
			if (!(Sflags[JOINTFLAG - 'a'])) {
				if ((pf.pf_gsid.s_rel == pkt->p_gotsid.s_rel &&
     				   pf.pf_gsid.s_lev == pkt->p_gotsid.s_lev &&
				   pf.pf_gsid.s_br == pkt->p_gotsid.s_br &&
				   pf.pf_gsid.s_seq == pkt->p_gotsid.s_seq) ||
				   (pf.pf_nsid.s_rel == pkt->p_reqsid.s_rel &&
				   pf.pf_nsid.s_lev == pkt->p_reqsid.s_lev &&
				   pf.pf_nsid.s_br == pkt->p_reqsid.s_br &&
				   pf.pf_nsid.s_seq == pkt->p_reqsid.s_seq)) {
					fclose(in);
					sprintf(SccsError,
					   "being edited: `%s' (ge17)",
					   line);
					fatal(SccsError);
				}
				if (!equal(pf.pf_user,user))
					fprintf(stderr,warn,line);
			} else {
			   fprintf(stderr,warn,line);
			}
		}
		fclose(in);
	}
	if (fseek(out,0L,2) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	sid_ba(&pkt->p_gotsid,str1);
	sid_ba(&pkt->p_reqsid,str2);
	date_ba(&Timenow,line);
	if (fprintf(out,"%s %s %s %s",str1,str2,user,line) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (inc)
		if (fprintf(out," -i%s",inc) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (exc)
		if (fprintf(out," -x%s",exc) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (cmrinsert () > 0)	/* if there are CMRS and they are okay */
		if (fprintf (out, " -z%s", cmr) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (fprintf(out, "\n") == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (fflush(out) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
#ifdef	USE_FSYNC
	if (fsync(fileno(out)) < 0)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
#endif
	if (fclose(out) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (pkt->p_verbose)
                if (HADQ)
                   fprintf(pkt->p_stdout, "new version %s\n",
				str2);
                else
                   fprintf(pkt->p_stdout, "new delta %s\n",
				str2);
}

/* Null routine to satisfy external reference from dodelt() */
/*ARGSUSED*/
void
escdodelt(struct packet *pkt)
{
}

/* NULL routine to satisfy external reference from dodelt() */
/*ARGSUSED*/
void
fredck(struct packet *pkt)
{
}

/* cmrinsert -- insert CMR numbers in the p.file. */

static int 
cmrinsert(void)
{
	extern char *Sflags[];
	char holdcmr[CMRLIMIT];
	char tcmr[CMRLIMIT];
	char *p;
	int bad;
	int valid;

	if (Sflags[CMFFLAG - 'a'] == 0)	{ /* CMFFLAG was not set. */
		return (0);
	}
	if (HADP && !HADZ) { /* no CMFFLAG and no place to prompt. */
		fatal("Background CASSI get with no CMRs\n");
	}
retry:
	if (cmr[0] == 0) {	/* No CMR list.  Make one. */
		if (HADZ && ((!isatty(0)) || (!isatty(1)))) {
		   fatal("Background CASSI get with invalid CMR\n");
		}
		fprintf (stdout,
		   "Input Comma Separated List of CMRs: ");
		fgets(cmr, CMRLIMIT, stdin);
		p=strend(cmr);
		*(--p) = 0;
		if ((int)(p - cmr) == CMRLIMIT) {
		   fprintf(stdout, "?Too many CMRs.\n");
		   cmr[0] = 0;
		   goto retry; /* Entry was too long. */
		}
	}
	/* Now, check the comma seperated list of CMRs for accuracy. */
	bad = 0;
	valid = 0;
	strcpy(tcmr, cmr);
	while (p=strrchr(tcmr,',')) {
		++p;
		if (cmrcheck(p,Sflags[CMFFLAG - 'a'])) {
			++bad;
		} else {
			++valid;
			strcat(holdcmr,",");
			strcat(holdcmr,p);
		}
		*(--p) = 0;
	}
	if (*tcmr)
		if (cmrcheck(tcmr,Sflags[CMFFLAG - 'a'])) {
			++bad;
		} else {
			++valid;
			strcat(holdcmr,",");
			strcat(holdcmr,tcmr);
		}
	if (!bad && holdcmr[1]) {
	   strcpy(cmr,holdcmr+1);
	   return(1);
	} else {
	   if ((isatty(0)) && (isatty(1))) {
	      if (!valid)
		 fprintf(stdout,
		    "Must enter at least one valid CMR.\n");
	      else
		 fprintf(stdout,
		    "Re-enter invalid CMRs, or press return.\n");
	   }
	   cmr[0] = 0;
	   goto retry;
	}
}

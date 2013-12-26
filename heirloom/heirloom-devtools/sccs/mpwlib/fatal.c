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
 * from fatal.c 1.8 06/12/12
 */

/*	from OpenSolaris "fatal.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fatal.c	1.4 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/fatal.c"	*/
# include	<defines.h>
# include	<fatal.h>
# include	<had.h>

/*
	General purpose error handler.
	Typically, low level subroutines which detect error conditions
	(an open or create routine, for example) return the
	value of calling fatal with an appropriate error message string.
	E.g.,	return(fatal("can't do it"));
	Higher level routines control the execution of fatal
	via the global word Fflags.
	The macros FSAVE and FRSTR in <fatal.h> can be used by higher
	level subroutines to save and restore the Fflags word.
 
	The argument to fatal is a pointer to an error message string.
	The action of this routine is driven completely from
	the "Fflags" global word (see <fatal.h>).
	The following discusses the interpretation of the various bits
	of Fflags.
 
	The FTLMSG bit controls the writing of the error
	message on file descriptor 2.  The message is preceded
	by the string "ERROR: ", unless the global character pointer
	"Ffile" is non-zero, in which case the message is preceded
	by the string "ERROR [<Ffile>]: ".  A newline is written
	after the user supplied message.
 
	If the FTLCLN bit is on, clean_up is called with an
	argument of 0 (see clean.c).
 
	If the FTLFUNC bit is on, the function pointed to by the global
	function pointer "Ffunc" is called with the user supplied
	error message pointer as argument.
	(This feature can be used to log error messages).
 
	The FTLACT bits determine how fatal should return.
	If the FTLJMP bit is on longjmp(Fjmp) is called
       (Fjmp is a global vector, see <setjmp.h>).
 
	If the FTLEXIT bit is on the value of userexit(1) is
	passed as an argument to exit(II)
	(see userexit.c).
 
	If none of the FTLACT bits are on
	(the default value for Fflags is 0), the global word
	"Fvalue" (initialized to -1) is returned.
 
	If all fatal globals have their default values, fatal simply
	returns -1.
*/

int	Fcnt;
int	Fflags;
char	*Ffile;
int	Fvalue = -1;
int	(*Ffunc)(char *);
jmp_buf	Fjmp;
char    *nsedelim = (char *) 0;

/* default value for NSE delimiter (currently correct, if NSE ever
 * changes implementation it will have to pass new delimiter as
 * value for -q option)
 */
# define NSE_VCS_DELIM "vcs"
# define NSE_VCS_GENERIC_NAME "<history file>"
static  int     delimlen = 0;

int 
fatal(char *msg)
{
	void	clean_up(void);

	++Fcnt;
	if (Fflags & FTLMSG) {
		write(2,"ERROR",5);
		if (Ffile) {
			write(2," [",2);
			write(2,Ffile,length(Ffile));
			write(2,"]",1);
		}
		write(2,": ",2);
		write(2,msg,length(msg));
		write(2,"\n",1);
	}
	if (Fflags & FTLCLN)
		clean_up();
	if (Fflags & FTLFUNC)
		(*Ffunc)(msg);
	switch (Fflags & FTLACT) {
	case FTLJMP:
		longjmp(Fjmp, 1);
	/*FALLTHRU*/
	case FTLEXIT:
		exit(userexit(1));
	/*FALLTHRU*/
	case FTLRET:
		return(Fvalue);
	}
	return(-1);
}

/* if running under NSE, the path to the directory which heads the
 * vcs hierarchy and the "s." is removed from the names of s.files
 *
 * if `vcshist' is true, a generic name for the history file is returned.
 */
 
char *
nse_file_trim(char *f, int vcshist)
{
        register char   *p;
        char            *q;
        char            *r;
        char            c;
 
        r = f;
        if (HADQ) {
                if (vcshist && Ffile && (0 == strcmp(Ffile, f))) {
                        return NSE_VCS_GENERIC_NAME;
                }
                if (!nsedelim) {
                        nsedelim = NSE_VCS_DELIM;
                }
                if (delimlen == 0) {
                        delimlen = strlen(nsedelim);
                }
                p = f;
                while (c = *p++) {
                        /* find the NSE delimiter path component */
                        if ((c == '/') && (*p == nsedelim[0]) &&
                            (0 == strncmp(p, nsedelim, delimlen)) &&
                            (*(p + delimlen) == '/')) {
                                break;
                        }
                }
                if (c) {
                        /* if found, new name starts with second "/" */
                        p += delimlen;
                        q = strrchr(p, '/');
                        /* find "s." */
                        if (q && (q[1] == 's') && (q[2] == '.')) {
                                /* build the trimmed name.
                                 * <small storage leak here> */
                                q[1] = '\0';
                                r = (char *) malloc((unsigned) (strlen(p) +
                                                strlen(q+3) + 1));
                                strcpy(r, p);
                                q[1] = 's';
                                strcat(r, q+3);
                        }
                }
        }
        return r;
}

int
check_permission_SccsDir(char *path)
{
	int  desc;
	char dirname[MAXPATHLEN];

	strcpy(dirname, sname(path));
	strcat(dirname, "/.");
	if ((desc = open(dirname, O_RDONLY)) < 0) {
		if (errno == EACCES) {
			fprintf(stderr, "%s: Permission denied\n", dname(dirname));
			++Fcnt;
			return (0);
		}
		return (1);
	}
	close(desc);
	return(1);

}


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
 * from help2.c 1.10 06/12/12
 */

/*	from OpenSolaris "help2.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)help.c	1.5 (gritter) 12/21/06
 */
/*	from OpenSolaris "sccs:cmd/help2.c"	*/
#include	<defines.h>
#include	<i18n.h>
#include	<ccstypes.h>

/*
	Program to locate helpful info in an ascii file.
	The program accepts a variable number of arguments.

	File is formatted as follows:

		* comment
		* comment
		-str1
		text
		-str2
		text
		* comment
		text
		-str3
		text

	The "str?" that matches the key is found and
	the following text lines are printed.
	Comments are ignored.

	If the argument is omitted, the program requests it.
*/

#define HELPLOC LIBDIR "/help"
#define	DFTFILE	"default"


char	SccsError[MAXERRORLEN];
static char	hfile[256];
struct	stat	Statbuf;
static FILE	*iop;
static char	line [MAXLINE+1];

static int findprt(char *);
static char *ask(void);
static int lochelp(char *, char *);


int 
main(int argc, char *argv[])
{
	register int i;
	int numerrs=0;

	Fflags = FTLMSG;
	if (argc == 1)
		numerrs += findprt(ask());
	else
		for (i = 1; i < argc; i++)
			numerrs += findprt(argv[i]);

	return ((numerrs == (argc-1)) ? 1 : 0);
}


static int 
findprt (
    char *p		/* "p" is user specified error code. */
)
{
	register char *q;
	char key[150];

	if ((int) size(p) > 50) 
		return(1);

	q = p;

	while (*q && !numeric(*q))
		q++;

	if (*q == '\0') {		/* all alphabetics */
		strcpy(key,p);
		sprintf(hfile,"%s/cmds",HELPLOC);
		if (!exists(hfile))
			sprintf(hfile,"%s/%s",HELPLOC,DFTFILE);
	}
	else
		if (q == p) {		/* first char numeric */
			strcpy(key,p);
			sprintf(hfile,"%s/%s",HELPLOC,DFTFILE);
		}
	else {				/* first char alpha, then numeric */
		strcpy(key,p);		/* key used as temporary */
		*(key + (q - p)) = '\0';
		if(!lochelp(key,hfile)) 
		        sprintf(hfile,"%s/%s",HELPLOC,key);
		else
			cat(hfile,hfile,"/",key,0);
		strcpy(key,q);
		if (!exists(hfile)) {
			strcpy(key,p);
			sprintf(hfile,"%s/%s",HELPLOC,DFTFILE);
		}
	}

	if((iop = fopen(hfile,NOGETTEXT("r"))) == NULL)
		return(1);

	/*
	Now read file, looking for key.
	*/
	while ((q = fgets(line,sizeof(line)-1,iop)) != NULL) {
		repl(line,'\n','\0');		/* replace newline char */
		if (line[0] == '-' && equal(&line[1],key))
			break;
	}

	if (q == NULL) {	/* endfile? */
		fclose(iop);
		sprintf(SccsError, "Key '%s' not found (he1)",p);
		fatal(SccsError);
		return(1);
	}

	printf("\n%s:\n",p);

	while (fgets(line,sizeof(line)-1,iop) != NULL && line[0] == '-')
		;

	do {
		if (line[0] != '*')
			printf("%s",line);
	} while (fgets(line,sizeof(line)-1,iop) != NULL && line[0] != '-');

	fclose(iop);
	return(0);
}

static char *
ask(void)
{
	static char resp[51];

	iop = stdin;

	printf("Enter the message number or SCCS command name: ");
	fgets(resp,51,iop);
	return(repl(resp,'\n','\0'));
}


/* lochelp finds the file which contains the help messages.
 * if none found returns 0.  If found, as a side effect, lochelp
 * modifies the actual second parameter passed to lochelp to contain
 * the file name of the found file (pointed to by the automatic
 * variable fi).
 */
static int 
lochelp (
    char *ky,
    char *fi /*ky is key  fi is found file name */
)
{
	FILE *fp;
	char locfile[MAXLINE + 1];
	char *hold;
	if(!(fp = fopen(HELPLOC,"r")))
	{
		/*no lochelp file*/
		return(0); 
	}
	while(fgets(locfile,sizeof(locfile)-1,fp)!=NULL)
	{
		hold=(char *)strtok(locfile,"\t ");
		if(!(strcmp(ky,hold)))
		{
			hold=(char *)strtok(0,"\n");
			strcpy(fi,hold); /* copy file name to fi */
			fclose(fp);
			return(1); /* entry found */
		}
	}
	fclose(fp);
	return(0); /* no entry found */
}

/* for fatal() */
void 
clean_up(void)
{
}

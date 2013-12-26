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
 * from dofile.c 1.12 06/12/12
 */

/*	from OpenSolaris "dofile.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dofile.c	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/comobj/dofile.c"	*/
# include	<defines.h>
# include	<dirent.h>

char	had_dir;
char	had_standinp;

void 
do_file(register char *p, void (*func)(char *), int check_file)
{
	extern char *Ffile;
	int fd;
	char str[FILESIZE];
	char ibuf[FILESIZE];
	DIR	*dirf;
	struct dirent *dir[2];
	char	*cp;

	if ((p[0] == '-' ) && (!p[1])) {
		/* this is make sure that the arguements starting with
		** a hyphen are handled as regular files and stdin
		** is used for accepting file names when a hyphen is
		** not followed by any characters.
		*/
		had_standinp = 1;
		while (fgets(ibuf, sizeof ibuf, stdin) != NULL) {
			if ((cp = strchr(ibuf, '\n')) != NULL)
				*cp = 0;
			if (exists(ibuf) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				had_dir = 1;
				Ffile = ibuf;
				if((dirf = opendir(ibuf)) == NULL)
					return;
				dir[0] = readdir(dirf);   /* skip "."  */
				dir[0] = readdir(dirf);   /* skip ".."  */
				while(dir[0] = readdir(dirf)) {
					if(dir[0]->d_ino == 0) continue;
					sprintf(str,"%s/%s",ibuf,dir[0]->d_name);
					if(sccsfile(str)) {
					   if (check_file && (fd=open(str,0)) < 0) {
					      errno = 0;
					   } else {
					        if (check_file) close(fd);
						Ffile = str;
						(*func)(str);
					   } 
					}
				}
				closedir(dirf);
			}
			else if (sccsfile(ibuf)) {
				if (check_file && (fd=open(ibuf,0)) < 0) {
				   errno = 0;
				} else {
				   if (check_file) close(fd);
				   Ffile = ibuf;
				   (*func)(ibuf);
				}   
			}
		}
	}
	else if (exists(p) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
		had_dir = 1;
		Ffile = p;
		if (!check_permission_SccsDir(p)) {
			return;
		}
		if((dirf = opendir(p)) == NULL)
			return;
		dir[0] = readdir(dirf); /* skip "." */
		dir[0] = readdir(dirf); /* skip ".." */
		while(dir[0] = readdir(dirf)){
			if(dir[0]->d_ino == 0) continue;
			sprintf(str,"%s/%s",p,dir[0]->d_name);
			if(sccsfile(str)) {
			   if (check_file && (fd=open(str,0)) < 0) {
			      errno = 0;
			   } else {
			      if (check_file) close(fd);
			      Ffile = str;
			      (*func)(str);
			   }	
			}
		}
		closedir(dirf);
	}
	else {
		if (strlen(p) < sizeof(str)) {
			strcpy(str, p);
		} else {
			strncpy(str, p, sizeof(str));
		}
		if (!check_permission_SccsDir(dname(str))) {
			return;
		}
		Ffile = p;
		(*func)(p);
	}
}

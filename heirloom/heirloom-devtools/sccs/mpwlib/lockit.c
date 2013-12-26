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
 * from lockit.c 1.20 06/12/12
 */

/*	from OpenSolaris "lockit.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)lockit.c	1.5 (gritter) 12/25/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/lockit.c"	*/

/*
	Process semaphore.
	Try repeatedly (`count' times) to create `lockfile' mode 444.
	Sleep 10 seconds between tries.
	If `tempfile' is successfully created, write the process ID
	`pid' in `tempfile' (in binary), link `tempfile' to `lockfile',
	and return 0.
	If `lockfile' exists and it hasn't been modified within the last
	minute, and either the file is empty or the process ID contained
	in the file is not the process ID of any existing process,
	`lockfile' is removed and it tries again to make `lockfile'.
	After `count' tries, or if the reason for the create failing
	is something other than EACCES, return xmsg().
 
	Unlockit will return 0 if the named lock exists, contains
	the given pid, and is successfully removed; -1 otherwise.
*/

# include	<defines.h>
# include	<i18n.h>
# include	<sys/utsname.h>
# include	<ccstypes.h>
# include	<signal.h>
# include	<limits.h>
# include	<stdlib.h>

static	int	onelock(pid_t, char *, char *);

int
lockit(
char	*lockfile,
int	count,
pid_t	pid,
char	*uuname)
{  
	int	fd;
	pid_t	opid;
	char	*ouuname;
	long	ltime, omtime;
	char	uniqfilename[PATH_MAX+48];
	char	tempfile[PATH_MAX];
	int	uniq_nmbr;
	int	nodenamelength;

	copy(lockfile, tempfile);
	sprintf(uniqfilename, "%s/%lu.%s%ld", dname(tempfile),
	   (unsigned long)pid, uuname, (long)time((time_t *)0));
	if (length(uniqfilename) >= PATH_MAX) {
	   uniq_nmbr = (int)pid + (int)time((time_t *)0);   
	   copy(lockfile, tempfile);
	   sprintf(uniqfilename, "%s/%X", dname(tempfile),
	      uniq_nmbr&0xffffffff);
	   uniqfilename[PATH_MAX-1] = '\0';
	}
	fd = open(uniqfilename, O_WRONLY|O_CREAT, 0666);
        if (fd < 0) {
	   return(-1);
	} else {
	   close(fd);
	   unlink(uniqfilename);
	}
	nodenamelength = strlen(uuname) + 1;
	if ((ouuname = calloc(nodenamelength, 1)) == NULL)
		return(-1);
	for (++count; --count; sleep(10)) {
		if (onelock(pid, uuname, lockfile) == 0) {
		   free(ouuname);
		   return(0);
		}
		if (!exists(lockfile))
		   continue;
		omtime = Statbuf.st_mtime;
		if ((fd = open(lockfile, O_RDONLY)) < 0)
		   continue;
		read(fd, (char *)&opid, sizeof(opid));
		read(fd, ouuname, nodenamelength);
		close(fd);
		/* check for pid */
		if (equal(ouuname, uuname)) {
		   if (kill((int) opid,0) == -1 && errno == ESRCH) {
		      if ((exists(lockfile)) &&
			  (omtime == Statbuf.st_mtime)) {
			 unlink(lockfile);
			 continue;
		      }
		   }
		}
		if ((ltime = time((time_t *)0) - Statbuf.st_mtime) < 60L) {
		   if (ltime >= 0 && ltime < 60) {
		      sleep((unsigned) (60 - ltime));
		   } else {
		      sleep(60);
		   }
		}
		continue;
	}
	free(ouuname);
	return(-1);
}

int
unlockit(
char	*lockfile,
pid_t	pid,
char	*uuname)
{
	int	fd, n;
	pid_t	opid;
	char	*ouuname;
	int	nodenamelength;

	if ((fd = open(lockfile, O_RDONLY)) < 0) {
	   return(-1);
	}
	nodenamelength = strlen(uuname) + 1;
	if ((ouuname = calloc(nodenamelength, 1)) == NULL)
	   return(-1);
	n = read(fd, (char *)&opid, sizeof(opid));
	read(fd, ouuname, nodenamelength);
	close(fd);
	if (n == sizeof(opid) && opid == pid && (equal(ouuname,uuname))) {
	   free(ouuname);
	   return(unlink(lockfile));
	} else {
	   free(ouuname);
	   return(-1);
	}
}

static int
onelock(
pid_t	pid,
char	*uuname,
char	*lockfile)
{
	int	fd;
	int	nodenamelength = strlen(uuname) + 1;
	
        if ((fd = open(lockfile, O_WRONLY|O_CREAT|O_EXCL, 0444)) >= 0) {
	   if (write(fd, (char *)&pid, sizeof(pid)) != sizeof(pid)) {
	      close(fd);
	      unlink(lockfile);
	      return(xmsg(lockfile, NOGETTEXT("lockit")));
	   }
	   if (write(fd, uuname, nodenamelength) != nodenamelength) {
	      close(fd);
	      unlink(lockfile);
	      return(xmsg(lockfile, NOGETTEXT("lockit")));
	   }
	   close(fd);
	   return(0);
	}
	if ((errno == ENFILE) || (errno == EACCES) || (errno == EEXIST)) {
	   return(-1);
	} else {
	   return(xmsg(lockfile, NOGETTEXT("lockit")));
	}
}

int
mylock(
char	*lockfile,
pid_t	pid,
char	*uuname)
{
	int	fd, n;
	pid_t	opid;
	char	*ouuname;
	int	nodenamelength;

	if ((fd = open(lockfile, O_RDONLY)) < 0)
	   return(0);
	nodenamelength = strlen(uuname) + 1;
	if ((ouuname = calloc(nodenamelength, 1)) == NULL)
	   return(0);
	n = read(fd, (char *)&opid, sizeof(opid));
	read(fd, ouuname, nodenamelength);
	close(fd);
	if (n == sizeof(opid) && opid == pid && (equal(ouuname, uuname))) {
	   free(ouuname);
	   return(1);
	} else {
	   free(ouuname);
	   return(0);
	}
}

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 1993 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "flex_dev.c	1.9	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)flex_dev.c	1.2 (gritter) 2/24/07
 */

/*LINTLIBRARY*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <pkglib.h>
#include "libadm.h"

#define	ERR_CHDIR	"unable to chdir back to <%s>, errno=%d"
#define	ERR_GETCWD	"unable to determine the current working directory, " \
			"errno=%d"

char *
flex_device(char *device_name, int dev_ok)
{
	char		new_device_name[PATH_MAX];
	char		*np = device_name;
	char		*cwd = NULL;

	if (!device_name || !*device_name)		/* NULL or empty */
		return (np);

	if (dev_ok == 1 && listdev(np) != (char **) NULL) /* device.tab */
		return (np);

	if (!strncmp(np, "/dev", 4))			/* /dev path */
		return (np);

	if ((cwd = getcwd(NULL, PATH_MAX)) == NULL) {
		progerr(gettext(ERR_GETCWD), errno);
		exit(99);
	}

	if (realpath(np, new_device_name) == NULL) {	/* path */
		if (chdir(cwd) == -1) {
			progerr(gettext(ERR_CHDIR), cwd, errno);
			(void) free(cwd);
			exit(99);
		}
		if (*np != '/' && dev_ok == 2) {
			(void) sprintf(new_device_name, "%s/%s", cwd, np);
			canonize(new_device_name);
			if ((np = strdup(new_device_name)) == NULL)
				np = device_name;
		}
		(void) free(cwd);
		return (np);
	}

	if (strcmp(np, new_device_name)) {
		if ((np = strdup(new_device_name)) == NULL)
			np = device_name;
	}

	(void) free(cwd);
	return (np);
}

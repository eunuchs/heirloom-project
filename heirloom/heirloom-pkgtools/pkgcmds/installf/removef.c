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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "removef.c	1.16	06/11/17 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)removef.c	1.2 (gritter) 2/24/07
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <locale.h>
#include <libintl.h>
#include <pkglib.h>
#include <install.h>
#include <libinst.h>
#include <libadm.h>
#include "installf.h"

#define	MALSIZ	64

void
removef(int argc, char *argv[])
{
	struct cfextra *new;
	char	path[PATH_MAX];
	int	flag;

	flag = strcmp(argv[0], "-") == 0;

	/* read stdin to obtain entries, which need to be sorted */
	extlist = calloc(MALSIZ, sizeof (struct cfextra *));

	eptnum = 0;
	new = NULL;
	for (;;) {
		if (flag) {
			if (fgets(path, PATH_MAX, stdin) == NULL)
				break;
		} else {
			if (argc-- <= 0)
				break;

			/*
			 * Make sure argv (path) contains at least one char
			 * so strcpy doesn't core dump
			 */

			if (strlen(argv[argc]) == 0) {
				logerr(gettext(
				    "ERROR: no pathname was provided"));
				if (new)
					free(new);
				warnflag++;
				continue;
			}

			/*
			 * This strips the install root from the path using
			 * a questionable algorithm. This should go away as
			 * we define more precisely the command line syntax
			 * with our '-R' option. - JST
			 */
			(void) strcpy(path, orig_path(argv[argc]));
		}
		if (path[0] != '/') {
			logerr(gettext(
			    "WARNING: relative pathname <%s> ignored"), path);
			if (new)
				free(new);
			warnflag++;
			continue;
		}
		new = calloc(1, sizeof (struct cfextra));
		if (new == NULL) {
			progerr(strerror(errno));
			quit(99);
		}
		new->cf_ent.ftype = '-';

		eval_path(&(new->server_path), &(new->client_path),
		    &(new->map_path), path);

		new->cf_ent.path = new->client_path;

		extlist[eptnum] = new;
		if ((++eptnum % MALSIZ) == 0) {
			extlist = realloc(extlist,
			    sizeof (struct cfextra)*(eptnum+MALSIZ));
			if (extlist == NULL) {
				progerr(strerror(errno));
				quit(99);
			}
		}
	}
	extlist[eptnum] = (struct cfextra *)NULL;

	qsort((char *)extlist,
	    (unsigned)eptnum, sizeof (struct cfextra *), cfentcmp);
}

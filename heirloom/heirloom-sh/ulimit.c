/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1996,1997 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)ulimit.c	1.11 (gritter) 11/21/05
 */
/* from OpenSolaris "ulimit.c	1.12	05/06/08 SMI" */

/*
 * ulimit builtin
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include "defs.h"

static const struct rlimtab {
	int	resource;
	char	*name;
	char	*scale;
	rlim_t	divisor;
} rlimtab[] = {
	{ RLIMIT_CORE,	 	"coredump",	"blocks",	512 },
	{ RLIMIT_DATA,	 	"data",		"kbytes",	1024 },
	{ RLIMIT_FSIZE, 	"file",		"blocks",	512 },
#ifdef	RLIMIT_MEMLOCK
	{ RLIMIT_MEMLOCK,	"memlock",	"kbytes",	1024 },
#endif
#ifdef	RLIMIT_RSS
	{ RLIMIT_RSS,		"rss",		"kbytes",	1024 },
#endif
	{ RLIMIT_STACK, 	"stack",	"kbytes",	1024 },
	{ RLIMIT_CPU,		"time",		"seconds",	1 },
	{ RLIMIT_NOFILE, 	"nofiles",	"descriptors",	1 },
#ifdef	RLIMIT_NPROC
	{ RLIMIT_NPROC,		"processes",	"count",	1 },
#endif
#ifdef	RLIMIT_AS
	{ RLIMIT_AS,	 	"memory",	"kbytes",	1024 },
#endif
	{ -1,			NULL,		NULL,		0 }
};

void
sysulimit(int argc, char **argv)
{
	int savopterr, savoptind, savsp;
	char *savoptarg;
	char *args;
	int hard, soft, c, i, res;
	rlim_t limit, new_limit;
	struct rlimit rlimit;
	int resources[sizeof rlimtab / sizeof *rlimtab];
	const struct rlimtab	*rp = NULL;

	savoptind = optind;
	savopterr = opterr;
	savsp = getopt_sp;
	savoptarg = optarg;
	optind = 1;
	getopt_sp = 1;
	opterr = 0;
	hard = 0;
	soft = 0;
	res = 0;

	while ((c = getopt(argc, argv, "HSacdflmnstuv")) != -1) {
		if (res == sizeof resources / sizeof *resources)
			goto fail;
		switch (c) {
		case 'S':
			soft++;
			continue;
		case 'H':
			hard++;
			continue;
		case 'a':
			if (res)
				goto fail;
			resources[res++] = RLIMIT_CORE;
			resources[res++] = RLIMIT_DATA;
			resources[res++] = RLIMIT_FSIZE;
#ifdef	RLIMIT_MEMLOCK
			resources[res++] = RLIMIT_MEMLOCK;
#endif
#ifdef	RLIMIT_RSS
			resources[res++] = RLIMIT_RSS;
#endif
			resources[res++] = RLIMIT_STACK;
			resources[res++] = RLIMIT_CPU;
			resources[res++] = RLIMIT_NOFILE;
#ifdef	RLIMIT_NPROC
			resources[res++] = RLIMIT_NPROC;
#endif
#ifdef	RLIMIT_AS
			resources[res++] = RLIMIT_AS;
#endif
			continue;
		case 'c':
			resources[res++] = RLIMIT_CORE;
			break;
		case 'd':
			resources[res++] = RLIMIT_DATA;
			break;
		case 'f':
			resources[res++] = RLIMIT_FSIZE;
			break;
#ifdef	RLIMIT_MEMLOCK
		case 'l':
			resources[res++] = RLIMIT_MEMLOCK;
			break;
#endif
#ifdef	RLIMIT_RSS
		case 'm':
			resources[res++] = RLIMIT_RSS;
			break;
#endif
		case 'n':
			resources[res++] = RLIMIT_NOFILE;
			break;
		case 's':
			resources[res++] = RLIMIT_STACK;
			break;
		case 't':
			resources[res++] = RLIMIT_CPU;
			break;
#ifdef	RLIMIT_CPU
		case 'u':
			resources[res++] = RLIMIT_CPU;
#endif
#ifdef	RLIMIT_AS
		case 'v':
			resources[res++] = RLIMIT_AS;
			break;
#endif
		default:
		case '?':
			goto fail;
		}
	}

	if (res == 0)
		resources[res++] = RLIMIT_FSIZE;

	/*
	 * if out of arguments, then print the specified resources
	 */

	if (optind == argc) {
		if (!hard && !soft)
			soft++;
		for (c = 0; c < res; c++) {
			rp = NULL;
			for (i = 0; i < sizeof resources / sizeof *resources;
					i++)
				if (rlimtab[i].resource == resources[c]) {
					rp = &rlimtab[i];
					break;
				}
			if (rp == NULL)
				continue;
			if (getrlimit(rp->resource, &rlimit) < 0) {
				continue;
			}
			if (res > 1) {
				prs_buff(rp->name);
				prc_buff('(');
				prs_buff(rp->scale);
				prc_buff(')');
				prc_buff(' ');
			}
			if (soft) {
				if (rlimit.rlim_cur == RLIM_INFINITY) {
					prs_buff("unlimited");
				} else  {
					prull_buff(rlimit.rlim_cur /
					    rp->divisor);
				}
			}
			if (hard && soft) {
				prc_buff(':');
			}
			if (hard) {
				if (rlimit.rlim_max == RLIM_INFINITY) {
					prs_buff("unlimited");
				} else  {
					prull_buff(rlimit.rlim_max /
					    rp->divisor);
				}
			}
			prc_buff('\n');
		}
		goto err;
	}

	if (res > 1 || optind + 1 != argc)
		goto fail;

	if (eq(argv[optind], "unlimited")) {
		limit = RLIM_INFINITY;
	} else {
		args = argv[optind];

		new_limit = limit = 0;
		do {
			if (*args < '0' || *args > '9')
				goto fail;
			/* Check for overflow! */
			new_limit = (limit * 10) + (*args - '0');
			if (new_limit >= limit) {
				limit = new_limit;
			} else
				goto fail;
		} while (*++args);

		rp = NULL;
		for (i = 0; i < sizeof resources / sizeof *resources; i++)
			if (rlimtab[i].resource == resources[0]) {
				rp = &rlimtab[i];
				break;
			}
		if (rp == NULL)
			goto fail;
		/* Check for overflow! */
		new_limit = limit * rp->divisor;
		if (new_limit >= limit) {
			limit = new_limit;
		} else
			goto fail;
	}

	if (getrlimit(resources[0], &rlimit) < 0)
		goto fail;

	if (!hard && !soft) {
		hard++;
		soft++;
	}
	if (hard) {
		rlimit.rlim_max = limit;
	}
	if (soft) {
		rlimit.rlim_cur = limit;
	}

	if (setrlimit(resources[0], &rlimit) < 0) {
	fail:	failure(argv[0], badulimit);
	}

err:
	optind = savoptind;
	opterr = savopterr;
	getopt_sp = savsp;
	optarg = savoptarg;
}

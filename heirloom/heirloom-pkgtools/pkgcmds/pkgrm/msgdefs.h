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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "msgdefs.h	1.10	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)msgdefs.h	1.2 (gritter) 2/24/07
 */

#define	MSG_REVERTING	"\
Converting the SQL-format package database to an ascii file."

#define	ERR_CANNOT_REVERT "Unable to convert SQL format package database to "\
			"ascii file: %s\nPlease run the command again with "\
			"root user privileges"

#define	ERR_UNABLE_TO_INSPECT	"unable to inspect SQL database %s/%s"

#define	ASK_CONTINUE	"Do you want to continue with package removal?"

#define	ERR_NOPKGS	"no packages were found in <%s>"

#define	ERR_CHDIR	"unable to change directory to <%s>"

#define	MSG_SUSPEND	"Removals of <%s> has been suspended."

#define	MSG_1MORETODO	"\nThere is 1 more package to be removed."

#define	MSG_MORETODO	"\nThere are %d more packages to be removed."

#define	ERR_NOTROOT	"You must be \"root\" for %s to execute properly."

#define	INFO_SPOOLED	"\nThe following package is currently spooled:"

#define	INFO_INSTALL	"\nThe following package is currently installed:"

#define	INFO_RMSPOOL	"\nRemoving spooled package instance <%s>"

#define	ASK_CONFIRM "Do you want to remove this package?"

#define	ERR_ROOT_CMD    "Command line install root contends with environment."

#define	ERR_ROOT_SET    "Could not set install root from the environment."

#define	ERR_WITH_S  "illegal combination of options with \"s\"."

#define	ERR_WITH_A  "illegal option combination \"-M\" with \"-A\"."

#define	ERR_CAT_LNGTH "The category argument exceeds the SVr4 ABI\n" \
		"        defined maximum supported length of 16 characters."

#define	ERR_CAT_FND "Category argument <%s> cannot be found."

#define	ERR_CAT_SYS "Unable to remove packages that are part of the " \
		"SYSTEM category with the -Y option."

#define	ERR_CAT_INV "Category argument <%s> is invalid."

#define	ERR_USAGE   "usage:\n" \
			"\t%s [-a admin] [-n] [[-M|-A] -R host_path] " \
			"[-V fs_file] [-v] [-Y category[,category ...] | " \
			"pkg [pkg ...]]" \
			"\n\t%s -s spool [-Y category[,category ...] | " \
			"pkg [pkg ...]]\n"

#define	MAX_CAT_ARGS 64

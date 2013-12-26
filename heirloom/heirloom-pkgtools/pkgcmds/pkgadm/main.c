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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "main.c	1.9	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)main.c	1.3 (gritter) 2/26/07
 */

/* unix system includes */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <locale.h>
#include <sys/param.h>
#include <openssl/bio.h>
#include "pkgadm.h"
#include "pkglib.h"
#include "pkgerr.h"
#include "keystore.h"
#include "pkgadm_msgs.h"

/* initial error message buffer size */

#define	ERR_BUFSIZE		2048

/* Local Function Prototypes */

static void			print_version();

/* holds subcommands and their definitions */
static struct cmd {
	char		*c_name;
	int		(*c_func)(int argc, char **argv);
} cmds[] = {
	{ "addcert",		addcert},
	{ "dbstatus",		get_dbstatus},
	{ "listcert",		listcert},
	{ "lock",		admin_lock},
#ifdef SQLDB_IS_USED
	/* SQL format contents database is supported */
	{ "refresh",		get_contents},
#endif
	{ "removecert",		removecert},
#ifdef SQLDB_IS_USED
	/* SQL format contents database is supported */
	{ "revert",		convert_from_db},
#endif
#ifdef SQLDB_IS_USED
	/* SQL format contents database is supported */
	{ "special",		gen_special},
	{ "upgrade",		convert_to_db},
#endif
	/* last one must be all NULLs */
	{ NULL, NULL }
};

/*
 * Function:	main
 *
 * Return:	0	- subprocessing successful
 *			  scripts and reboot
 *	[other]	- subprocessing-specific failure
 */
int
main(int argc, char **argv)
{
	char	cur_cmd;
	int	newargc;
	char	**newargv;
	int	i;

	/* Should be defined by cc -D */
#if	!defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN "SYS_TEST"
#endif

	/* set the default text domain for messaging */
	(void) textdomain(TEXT_DOMAIN);

	if (getenv("PKGADM_VERBOSE")) {
	    set_verbose(B_TRUE);
	}

	/* Superficial check of the arguments. */
	if (argc <= 1) {
		log_msg(LOG_MSG_INFO, MSG_USAGE);
		return (1);
	}

	/* initialize security library */
	sec_init();

	/* first, process any arguments that can appear before the subcommand */
	while ((i = getopt(argc, argv, "vV?")) != EOF) {
		switch (i) {
		case 'v':	/* verbose mode enabled */
			set_verbose(B_TRUE);
			break;
		case 'V':
			print_version();
			return (0);
		case '?':
			log_msg(LOG_MSG_INFO, MSG_USAGE);
			return (0);
		}
	}

	/* OK, hand it off to the subcommand processors */
	for (cur_cmd = 0; cmds[cur_cmd].c_name != NULL; cur_cmd++) {
		if (ci_streq(argv[optind], cmds[cur_cmd].c_name)) {
			/* make subcommand the first option */
			newargc = argc - optind;
			newargv = argv + optind;
			opterr = optind = 1; optopt = 0;
			return (cmds[cur_cmd].c_func(newargc, newargv));
		}
	}

	/* bad subcommand */
	log_msg(LOG_MSG_ERR, MSG_BAD_SUB, argv[optind]);
	log_msg(LOG_MSG_INFO, MSG_USAGE);
	return (1);
}

/*
 * Name:	set_verbose
 * Description:	Turns on verbose output
 * Scope:	public
 * Arguments:	verbose = B_TRUE indicates verbose mode
 * Returns:	none
 */
void
set_verbose(boolean_t setting)
{
	log_set_verbose(setting);
}

/*
 * Name:	get_verbose
 * Description:	Returns whether or not to output verbose messages
 * Scope:	public
 * Arguments:	none
 * Returns:	B_TRUE - verbose messages should be output
 */
boolean_t
get_verbose()
{
	return (log_get_verbose());
}

/*
 * Name:	log_pkgerr
 * Description:	Outputs pkgerr messages to logging facility.
 * Scope:	public
 * Arguments:	type - the severity of the message
 *		err - error stack to dump to facility
 * Returns:	none
 */
void
log_pkgerr(LogMsgType type, PKG_ERR *err)
{
	int i;
	for (i = 0; i < pkgerr_num(err); i++) {
		log_msg(type, "%s", pkgerr_get(err, i));
	}
}

/*
 * Name:	print_Version
 * Desc:  Prints Version of packaging tools
 * Arguments: none
 * Returns: none
 */
static void
print_version()
{
	/* ignore any and all arguments, print version only */
	(void) fprintf(stdout, "%s\n", SUNW_PKGVERS);
}

/*
 * usage
 *
 * Outputs the usage string.
 *
 * Return:1
 * Side effects: none
 */
static int
usage()
{
	log_msg(LOG_MSG_INFO, MSG_USAGE);
	return (1);
}

/*
 * get_dbstatus
 *
 * Return either 'text' or 'db' depending on the db status.
 * Use the command line to determine if there is an alternate root.
 *
 * Return: 0 on success, nonzero on failure
 * Side effects: none
 */
int
get_dbstatus(int argc, char **argv)
{
	char *pcroot = NULL;
	int e = 0;

	/* Either accept 1 argument or 3 arguments where the second is -R */
	if (argc != 1 && (argc != 3 || strcmp(argv[1], "-R")))
		return (usage());

	(void) printf("%s\n", PKGADM_DBSTATUS_TEXT);

	return (0);
}

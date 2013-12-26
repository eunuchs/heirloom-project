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
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "genericdb.c	1.4	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)genericdb.c	1.6 (gritter) 2/25/07
 */

/*
 *
 * Minimal Generic database API implementation.
 *
 * NOTE WELL:
 *
 *    This library encapsulates all the access logic for the install database.
 *    This includes hard-coded path names to the install database file.
 *    The location and existence of this file is a PRIVATE INTERFACE which
 *    will eventually change.
 *
 *    In order to use this library for any other purpose (for examly to
 *    access a different database file) the additional parameters would
 *    have to be supplied using the command line options passed to
 *    genericdb_open() or genericdb_setting().
 *
 * Please see the extensive documentation below which precedes the public
 * functions.  This is only a synopsis.
 *
 * genericdb_close	This ends a session with the database.
 * genericdb_errstr	This returns an error string describing the error
 *			value.
 * genericdb_exists	This determines if a database file already exists.
 * genericdb_open	This initates a session with the database.
 *			Note that a database file will be created if one
 *			does not yet exist.
 * genericdb_querySQL	Given an SQL query, return a result if possible.
 * genericdb_setting	Change the session characteristics.
 * genericdb_submitSQL	Submit SQL which will not result in a result.
 *
 * Additional functions relate to use of results.
 * One should always initialize results so that they can be freed
 * safely using
 *     genericdb_result r;
 *     memset(&r, 0, GENERICDB_RESULT_SIZE);
 *
 * genericdb_result_free	Frees a result, of whatever kind.
 * genericdb_result_table_int	Returns an int val at a given column & row.
 * genericdb_result_table_ncols Returns how many columns in the table.
 * genericdb_result_table_nrows	Returns how many rows in the table.
 * genericdb_result_table_null	Returns whether a given column & row are null.
 * genericdb_result_table_str	Returns a copy of a string in a column & row.
 * genericdb_result_type	Determine the type of the result.
 *				This should be genericdb_TABLE.
 *
 */

/*LINTLIBRARY*/

#include <stdio.h>
#include <stdlib.h> /* for atoi() */
#include <sys/param.h> /* for MAXPATHLEN */
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libintl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sqlite.h"
#include "genericdb.h"
#include "genericdb_private.h"

/*
 * Internationalization note:
 * debug log level code is not internationalized.
 */

#ifndef	TEXT_DOMAIN
#define	TEXT_DOMAIN	"SYS_TEST"
#endif

#define	MSG_GENDB_OK		dgettext(TEXT_DOMAIN, "No error")
#define	MSG_GENDB_NOMEM		dgettext(TEXT_DOMAIN, \
	"Database access failed due to insufficient memory")
#define	MSG_GENDB_SQLERR	dgettext(TEXT_DOMAIN, \
	"Database access failed due to one or more incorrect SQL statements")
#define	MSG_GENDB_ACCESS	dgettext(TEXT_DOMAIN, \
	"Database access failed due to inability " \
	"to access the specified database file")
#define	MSG_GENDB_PERMS		dgettext(TEXT_DOMAIN, \
	"Database access failed due to insufficient " \
	"privileges to operate on the database file")
#define	MSG_GENDB_PARAMS	dgettext(TEXT_DOMAIN, \
	"genericdb API call failed due to invalid parameters in procedure call")
#define	MSG_GENDB_NULL		dgettext(TEXT_DOMAIN, \
	"Result requested has a NULL value")
#define	MSG_GENDB_UNKNOWN	dgettext(TEXT_DOMAIN, "genericdb error")
#define	MSG_GENDB_ERROR		dgettext(TEXT_DOMAIN, "Error")
#define	MSG_GENDB_WARNING	dgettext(TEXT_DOMAIN, "Warning")
#define	MSG_GENDB_DEBUG		dgettext(TEXT_DOMAIN, "Debug")
#define	MSG_GENDB_UNSPECIFIED	dgettext(TEXT_DOMAIN, "unspecified")

static char	*genericdbpriv_create_dbpath(const char *pcRoot);
static char	*genericdbpriv_debug_level(int argc, char *argv[]);
static int    genericdbpriv_log_level(int argc, char *argv[]);

/* ------------------------------------------------------------------------- */
/*
 * genericdb_errstr
 *
 * This function returns a descriptive string for each error code.
 *
 *    e		The error code to explain.
 *
 * Returns: A char * constant corresponding to message text.
 * Side effects: None.
 */
char *
genericdb_errstr(genericdb_Error e)
{
	switch (e) {
		case genericdb_OK: return (MSG_GENDB_OK);
		case genericdb_NOMEM: return (MSG_GENDB_NOMEM);
		case genericdb_SQLERR: return (MSG_GENDB_SQLERR);
		case genericdb_ACCESS: return (MSG_GENDB_ACCESS);
		case genericdb_PERMS: return (MSG_GENDB_PERMS);
		case genericdb_PARAMS: return (MSG_GENDB_PARAMS);
		case genericdb_NULL: return (MSG_GENDB_NULL);
	default: return (MSG_GENDB_UNKNOWN);
	}
}

#define	genericdbpriv_LOG_ERROR	1
#define	genericdbpriv_LOG_WARNING	2
#define	genericdbpriv_LOG_DEBUG	4

/*
 * genericdbpriv_log
 *
 * Perform logging as requested.  This is normally done only as part of
 * development tracing.
 *
 *   level	A message level.
 *   pg		The session handle.  A log level in the session will be
 *		compared with the message level to determine whether to
 *		output the message.
 *   pc		Additional text for the log message.
 *
 * Returns: Nothing.
 * Side effects: Text is emitted to standard output.
 */
static void
genericdbpriv_log(int level, const genericdb_private *pg, const char *pc)
{
	if (pc == NULL)
		pc = MSG_GENDB_UNSPECIFIED;

	switch (level) {
	case genericdbpriv_LOG_ERROR:
		if ((pg->log & genericdbpriv_LOG_ERROR) != 0)
			(void) fprintf(stderr, "%s: %s\n",
			MSG_GENDB_ERROR, pc);
		break;

	case genericdbpriv_LOG_WARNING:
		if ((pg->log & genericdbpriv_LOG_WARNING) != 0)
			(void) fprintf(stderr, "%s: %s\n",
			MSG_GENDB_WARNING, pc);
		break;

	case genericdbpriv_LOG_DEBUG:
		if ((pg->log & genericdbpriv_LOG_DEBUG) != 0)
			(void) fprintf(stderr, "%s: %s\n",
			MSG_GENDB_DEBUG, pc);
		break;

	default:
		assert(0);
	}
}

/*
 * genericdbpriv_create_dbpath
 *
 * Create a full path to the database.  Note that this is determined to a
 * known and constrainted relative path.  This is what makes libgendb
 * specific to install.
 *
 *	pcRoot	The relative root.
 *
 * Returns: A copy of a string.  The caller must free the string.
 *      A NULL is returned on error.
 *	If the string cannot be allocated errno is set to ENOMEM.
 *      If the filename would be too long errno is set to ENOENT.
 * Side effects: None.
 */
static char *
genericdbpriv_create_dbpath(const char *pcRoot)
{
	char buf[MAXPATHLEN];
	char *pc = NULL;
	errno = 0;

	if (pcRoot == NULL) pcRoot = "/";

	if ((strlen(pcRoot) + strlen(INSTALLDB_PATH) + 1)
	    >= MAXPATHLEN) {
		errno = ENOENT;
		return (NULL);
	}

	if (pcRoot[strlen(pcRoot) - 1] == '/') {
		if (snprintf(buf, PATH_MAX, "%s%s", pcRoot, INSTALLDB_PATH)
		    >= PATH_MAX) {
			errno = ENOENT;
			return (NULL);
		}
	} else {
		if (snprintf(buf, PATH_MAX, "%s/%s", pcRoot, INSTALLDB_PATH)
		    >= PATH_MAX) {
			errno = ENOENT;
			return (NULL);
		}
	}

	if ((pc = strdup(buf)) == NULL) {
		errno = ENOMEM;
	}

	return (pc);
}

/*
 * genericdbpriv_debug_level
 *
 * Returns a copy of a string for a debug level, if one is defined in
 * the command line.  If more than one debug levels are given, a copy of
 * the first one is returned.
 *
 *    argc	The number of arguments.
 *    argv	The arguments.
 *
 * Returns: A copy of the string (which the caller must free), or NULL
 *   if there is no debug level.
 * Side effects: None.
 */
static char *
genericdbpriv_debug_level(int argc, char *argv[])
{
	int i;

	for (i = 0; (i + 1) < argc; i++) {
		if (0 == strcmp(argv[i], "-d"))
			return (strdup(argv[i+1]));
	}

	return (NULL);
}

/*
 * genericdbpriv_log_level
 *
 * Parses the command line to find if there is a specified log level.
 * If more than one of these is specified in the command line, only the
 * first one is returned.
 *
 * 	argc	The number of args.
 *	argv	The args.
 *
 * Returns: An integer value.  0 is the default.  This means, no log level.
 * Side effects: None.
 */
static int
genericdbpriv_log_level(int argc, char *argv[])
{
	int i;

	for (i = 0; (i + 1) < argc; i++) {
		if (0 == strcmp(argv[i], "-l"))
			return (atoi(argv[i+1]));
	}

	return (0);
}


/* ------------------------------------------------------------------------- */

/*
 * genericdb_exists
 *
 * This routine determines whether a genericdb exists.  This is not a simple
 * operation since the mere existence does not mean that the database has
 * been populated properly.  This is because the genericdb_open call may
 * create a database *file* but the initial database transaction needed to
 * correctly populate the database file may have failed.  The program may
 * not have had an opportunity to remove the database file before exiting
 * with an error condition.
 *
 * For this reason, genericdb_exists is supplied with enough information to
 *
 * (a) to see if a database file exists at all, and if so
 *
 * (b) check if a database has been properly initialized (CHECK vs VAL)
 *     and if not,
 *
 * (c) to initialize the STATUS_TABLE with a version number which
 *     indicates that the table is not yet initialized.  This step is
 *     necessary since we expect clients of the genericdb interface to
 *     add SET_OK to their transactions which leave the DB in a new OK
 *     state.  Were the STATUS_TABLE note yet initialized, the client
 *     transaction would fail!
 *
 * The SQL used to check the reg4_status_table, to create it or to determine
 * the expected result for success are all set as #define values in genericdb.h
 * If genericdb is to be used for anything other than install, one would have
 * to define new settings to pass in to genericdb_open or genericdb_setting
 * to override these default values.
 *
 * Invariant conditions needed to call this interface and use it
 * successfully:
 *
 *  1.  Call genericdb_exists before calling genericdb_open.  In some
 *      cases, it makes no sense to call genericdb_open if genericdb_exists
 *      returns 0.  In all cases it will ensure the database is in a proper
 *      state to accept SET_OK statements.
 *
 *  2.  Call an *external lock function* before calling genericdb_exists.
 *	This will ensure that there is no race condition between the time
 *	that genericdb_exists is called and the result code is used.  If
 *	such a lock were not used, it would be possible to determine that
 *	there was no db and initiate an operation to create one, when in
 *	fact another application (process) determined the same thing and
 *	began to create one simultaneously.  Which of the two, if either
 *	would succeed is indeterminite.  To remove this problem, one needs
 *	to
 *  2.1	establish the lock before calling genericdb_exists
 *  2.2 release the lock if genericdb_exists returns 0 and there is no
 *      more processing to do, or
 *  2.3 call genericdb_open while still holding the lock
 *  2.4 if read only operations will be performed, release the lock before
 *      performing the queries.
 *  2.5 if any writes are to be performed, hold the lock till after these
 *	have all completed.
 *
 *      NOTE ABOUT LOCKING:  If this locking is not done, it is possible
 *	that utilities will fail (get genericdb_exists result 0) when in
 *	fact they should succeed.  It is not the case that the result will
 *	be a spurious success, unless the database is removed during the
 *      course of this call.  While this is possible, it will simply cause
 *	the utility to fail later on when genericdb_open is attempted.
 *	So:  For utilities which have no access to the lock used by the
 *	package utilities - especially those which are *read-only* it is
 *	acceptable to not perform the recommended locking actions.
 *
 *	NOTE ABOUT IGNORING THE LOCK AS A WRITER:  It is possible, though
 *	highly unlikely, that two callers will simultaneously attempt to
 *	access genericdb_exist at the same time.  The first caller will
 *	advance as far as finding out that there is a file but no table
 *	then it will (for the purpose of this example) be suspended.
 *	The second caller will create the table and complete an installation
 *	action leaving the database in a state where the status field in the
 *	status table is set to 'OK'.  The first caller may  resulme and then
 *	attempt to create the database table (with the default value).  This
 *	operation will fail, since the table already exists.  This is the
 *	reason 'failure to create the table' is not a serious error in the
 *	code below, though it will result in the spurious response that the
 *	database does not exist.  This is considered a rare enough case that
 *	a warning is generated to try again and no attempt is made to correct
 *	for this in the software.
 *
 *  3.  very important:  Add GENERICDB_INSTALL_DB_STATUS_SET_OK to the
 *	transaction which will leave the database in a state where it can
 *	be used by others.
 *
 *      An example of sufficient locking functions can be found in
 *         $AIGATE/src/bundled/app/pkgcmds/libinst/lockinst.c
 *
 *  Following the example:
 *
 *      lockinst("my program", "*");
 *      if (genericdb_exists("/") == 0) {
 *		unlockinst();
 *		return "the database does not exist";
 *      }
 *
 *      if ((pdb = genericdb_open("/", 0, NULL, &err)) == NULL ||
 *	    err != genericdb_OK)
 *		return genericdb_errstr(err);
 *
 *      if (read_operation) {
 *		unlockinst();
 *		perform_read_operation(pdb);
 *		genericdb_close(pdb);
 *		return "result is ...";
 *
 *	} else {
 *		char buf[BIGNUMBER];
 *		buf[0] = '\0';
 *
 *		strlcat(buf, "BEGIN TRANSACTION", BIGNUMBER);
 *		add_insert_rows_to_buf(buf, BIGNUMBER);
 *		strlcat(buf, GENERICDB_INSTALL_DB_STATUS_SET_OK, BIGNUMBER);
 *		strlcat(buf, "COMMIT;" BIGNUMBER);
 *
 *		err = genericdb_submitSQL(pdb, buf);
 *
 *		additional_processing_if_any();
 *
 *		genericdb_close(pdb);
 *		unlockinst();
 *		return "ok";
 *	}
 *
 *   pcRoot	This defines the 'root' of the file system under
 *		which the database file will be found.  This is not
 *		a path to the database file.  Rather, this is either
 *		NULL (default, which is "/") or an alternate path.
 *
 * Returns: B_TRUE if a genericdb exists and is usable, B_FALSE otherwise
 *
 * Side effects: This routine may result in the creation of the table as
 *   specified by the pcSetup command.  This will not cause future calls
 *   to genericdb_exists() to succeed necessarily, but it will prevent
 *   future sql sent to the db to set the table (as
 *   GENERICDB_INSTALL_DB_STATUS_SET_OK) from failing.
 */

boolean_t
genericdb_exists(const char *pcRoot)
{
	char *pcpath = genericdbpriv_create_dbpath(pcRoot);
	boolean_t result = B_FALSE;
	genericdb *pdb = NULL;
	genericdb_result r;
	int iDB, record_exists = 0;
	genericdb_Error err = genericdb_OK;
	char *pcStatus = NULL;

	r = NULL;

	iDB = access(pcpath, R_OK);

	if (pcpath != NULL && iDB == 0) {

		if ((pdb = genericdb_open(pcRoot, GENERICDB_R_MODE, 0, NULL,
		    &err)) != NULL && genericdb_OK == err) {

			/* See if the db is already initialized properly */

			err = genericdb_querySQL(pdb,
			    GENERICDB_INSTALL_DB_STATUS_CHECK, &r);

			if (err == genericdb_OK &&
			    genericdb_result_table_str(r, 0, 0, &pcStatus)
			    == genericdb_OK) {
				if (strcmp(pcStatus,
				    GENERICDB_INSTALL_DB_STATUS_OK_VAL) == 0) {

					result = B_TRUE;
				}
				record_exists = 1;
			}
			genericdb_close(pdb);
			pdb = NULL;
		}
	}

#ifdef SQLDB_IS_USED
	/*
	 * *********************************************************************
	 * This supports both the SQL and flat text format contents databases.
	 * *********************************************************************
	 */

	if (record_exists == 0) {

		/*
		 * This condition occurs if there is no db file or a db file
		 * that does not yet have a status table (or if pcCheck is
		 * bad, but that is an unimportant case).  We must try to
		 * open the database again, read-write mode, and create the
		 * tables required by pcSetup.  This may fail due to the user
		 * lacking the privileges to open the db to write.  In this
		 * case we silently fail.   This will be handled by the first
		 * client who reaches this condition who has write privileges.
		 */

		if ((pdb = genericdb_open(pcRoot, GENERICDB_WR_MODE, 0, NULL,
		    &err)) != NULL && genericdb_OK == err) {

			(void) genericdb_submitSQL(pdb,
			    GENERICDB_INSTALL_DB_STATUS_TABLE);
			/*
			 * Ignore the result!  A user who can write will
			 * succeed.  We only need that user to succeed,
			 * once.  Since this function will be called before
			 * the database is used the first time the status
			 * table will be created for the 'initial write'
			 * to change the table status to OK if it succeeds.
			 */
		}
	}
#endif

	if (pcStatus)
		free(pcStatus);

	if (pcpath)
		free(pcpath);

	if (pdb != NULL)
		genericdb_close(pdb);

	genericdb_result_free(r);

	return (result);
}

/*
 * genericdb_open
 *
 * This routine will encapsulate all necessary database-specific
 * opening and initializing code.  Multiple handles should be opened
 * for a multithreaded application, and operations on handles should
 * be treated as if they were not thread safe.  The handle must be
 * closed with genericdb_close in order to not leak resources and to
 * best ensure orderly release of database-specific resources.
 *
 *   pcRoot	This defines the 'root' of the file system under
 *		which the database file will be found.  This is not
 *		a path to the database file.  Rather, this is either
 *		NULL (default, which is "/") or an alternate path.
 *
 *   mode	Zero or more of the mode options, added together.
 *		If neither read or write permission is sought, then
 *		the database will simply be opened and possibly settings
 *		applied.  Use the mode settings supplied by <sys/fcntl.h>
 *		or unsigned convention values: 0644 is a common value.
 *		This gives read/write privileges to oneslef and read only
 * 		to group and the rest of the world.
 *
 *   argc       The number of arguments which are provided in argv.
 *		This argument may be 0.
 *
 *   argv       Starting with the first argument (not a command name)
 *		this is a list of arguments to use to determine the
 *		initial session behavior of the genericdb.  This
 *		argument may be NULL.  Defined arguments at the present
 *		time are:
 *
 *			-l <#>	Log level.  0 = silent, 1 = error,
 *				2 = warnings, 4 = debug.  'Or' them.
 *			-d <s>	The string is used to turn on certain
 *				debug instruments.  "all" turns them
 *				ALL on.
 *
 *   perr	[OUTPUT PARAMETER]
 *		This parameter is set to genericdb_OK if no error
 *		occurred and a genericdb handle is returned.  Otherwise
 *		the parameter is set an error value.  This argument must
 *		not be NULL.
 *
 * Returns: Either a ready to use genericdb handle or NULL on error.
 * Side Effects: Resources will likely be allocated and associated
 *    with the genericdb handle.
 */
genericdb *
genericdb_open(const char *pcRoot, int mode, int argc, char *argv[],
    genericdb_Error *perr)
{
	genericdb_private *pg = NULL;
	int i;

	/* This is a parameter error, but we cannot pass back the result. */
	if (perr == NULL) {
		return (NULL);
	}

	*perr = genericdb_OK;

	if ((mode & 0400) == 0 || argc < 0 || (argc != 0 && argv == NULL) ||
	    perr == NULL) {
		*perr = genericdb_PARAMS;
		return (NULL);
	}

	/* check for a malformed parameter list */
	for (i = 0; i < argc; i++) {
		if (argv[i] == NULL) {
			*perr = genericdb_PARAMS;
			return (NULL);
		}
	}

	pg = (genericdb_private *) malloc(sizeof (genericdb_private));
	if (pg == NULL) {
		*perr = genericdb_NOMEM;
	} else {
		char *pcpath = genericdbpriv_create_dbpath(pcRoot);
		char *pcmsg;

		if (pcpath == NULL) {
			*perr = genericdb_NOMEM;
			return (NULL);
		}

#ifdef SQLDB_IS_USED
		pg->psql  = sqlite_open(pcpath, mode, &pcmsg);
#else
		pg->psql = NULL;
		pcmsg = "no SQLite support";
#endif

		free(pcpath);

		if (pg->psql == NULL) {
			genericdbpriv_log(genericdbpriv_LOG_ERROR, pg, pcmsg);
			free(pg);
			pg = NULL;
			*perr = genericdb_ACCESS;
		} else {
			pg->debug = genericdbpriv_debug_level(argc, argv);
			pg->log   = genericdbpriv_log_level(argc, argv);
		}
	}

	return ((genericdb *) pg);
}

/*
 * genericdb_close
 *
 * This routine frees all resources associated with a genericdb
 * handle.  This routine should be called when a handle is no longer
 * needed, and before a program exits to ensure an orderly release of
 * resources held by the database.  After a handle has been closed it
 * cannot be used to access the database.
 *
 *  pgdb	The generic database handle to close.
 *
 * Results: None
 * Side Effects: None
 */
void
genericdb_close(genericdb *pgdb)
{
	genericdb_private *pg = (genericdb_private *) pgdb;

	if (pg == NULL) {
		return;
	}

	if (pg->psql == NULL) {
		return;
	}

#ifdef SQLDB_IS_USED
	sqlite_close(pg->psql);
#endif

	if (pg->debug) {
		free(pg->debug);
		pg->debug = NULL;
	}

	free(pg);
}

/*
 * genericdb_setting
 *
 * This command is used to alter the behavior of the database
 * access session.
 *
 *   pgdb	The already properly opened generic DB handle.
 *
 *   argc	This must be set to 1 or more.
 *
 *   argv	This is a series of one or more strings defining
 *		session parameters (see genericdb_open).
 *
 * Returns: Error code - if the setting does not succeed.
 * Side Effects: As per the setting.
 */
genericdb_Error
genericdb_setting(genericdb *pgdb, int argc, char *argv[])
{
	genericdb_private *pgrp = (genericdb_private *) pgdb;
	int i;

	if (pgdb == NULL || pgrp->psql == NULL ||
	    argc < 0 || (argc > 0 && argv == NULL))

		return (genericdb_PARAMS);

	for (i = 0; i < argc; i++) {

		if (argv[i] == NULL)
			return (genericdb_PARAMS);

		if ((0 == strcmp(argv[i], "-l")) && ((i + 1) < argc)) {
			pgrp->log = atoi(argv[i + 1]);
			i++;
			continue;
		}

		if ((0 == strcmp(argv[i], "-d")) && ((i + 1) < argc)) {
			pgrp->debug = strdup(argv[i + 1]);
			i++;
			continue;
		}

	}

#ifndef NDEBUG
	{
		char pc[256];
		(void) snprintf(pc, 256, "settings: log level [%d] = [%s%s%s] "
		    "debug string [%s]\n",
		    pgrp->log,
		    ((pgrp->log & genericdbpriv_LOG_ERROR) != 0)?"ERR ":"",
		    ((pgrp->log & genericdbpriv_LOG_WARNING) != 0)?"WARN ":"",
		    ((pgrp->log & genericdbpriv_LOG_DEBUG) != 0)?"DEBUG":"",
		    (pgrp->debug)?(pgrp->debug):"NULL");
		genericdbpriv_log(genericdbpriv_LOG_DEBUG, pgrp, pc);
	}
#endif

	return (genericdb_OK);
}

/*
 * genericdb_submitSQL
 *
 * This procedure is used to submit SQL for which no result
 * is expected.
 *
 *   pgdb	The already properly opened generic DB handle.
 *   pcSQL	A null terminated string containing a properly
 *              formatted and expressed set of statements in
 *		SQL92.
 *
 * Returns: Error code
 * Side Effects: As per the SQL submitted.
 */
genericdb_Error
genericdb_submitSQL(genericdb *pgdb, const char *pcSQL)
{
	genericdb_result gr;
	genericdb_Error err = genericdb_querySQL(pgdb, pcSQL, &gr);
	genericdb_result_free(gr);
	return (err);
}

/*
 * This returns the type of the result.  This should be checked before the
 * result is used.
 */
genericdb_resulttype
genericdb_result_type(const genericdb_result gr)
{
	genericdb_result_private *pgrp = (genericdb_result_private *) gr;
	if (pgrp == NULL)
		return (genericdb_UNKNOWN);

	return (pgrp->type);
}

/*
 * This returns the number of rows for a table result.
 * The result is -1 if the result is not a table result.
 */
int
genericdb_result_table_nrows(const genericdb_result gr)
{
	genericdb_result_private *pgrp = (genericdb_result_private *) gr;
	if (pgrp == NULL ||
	    pgrp->version != 1 ||
	    pgrp->type != genericdb_TABLE ||
	    pgrp->nrow < 0)
		return (-1);

	return (pgrp->nrow);
}

/*
 * This returns the number of columns for a table result.
 * The result is -1 if the result is not a table result.
 */
int
genericdb_result_table_ncols(const genericdb_result gr)
{
	genericdb_result_private *pgrp = (genericdb_result_private *) gr;
	if (pgrp == NULL ||
	    pgrp->version != 1 ||
	    pgrp->type != genericdb_TABLE ||
	    pgrp->ncol < 0)
		return (-1);

	return (pgrp->ncol);
}

/*
 * genericdb_result_table_str
 *
 * This returns a copy of a string in a particular cell of a table result.
 *
 *	gdbr		A table result, returned by genericdb_querySQL.
 *	irow		The row number of the table result to retrieve.
 *	icol		The column number of the table result to retrieve.
 *	ppcString	[OUTPUT PARAMETER] A pointer to a char *.  This
 *		        pointer will be set to an allocated copy of the
 *			string value of the cell requested.
 *
 * Result: genericdb_OK if no error, or an error code.
 *         genericdb_NULL is returned if a NULL value is requested and
 *            the returned string parameter is left NULL.
 * Side Effects: the allocated string buffer must be freed by the caller,
 *    using free().
 */
genericdb_Error
genericdb_result_table_str(const genericdb_result gr, int irow, int icol,
    char **ppcString)
{

	genericdb_result_private *pgrp = (genericdb_result_private *) gr;
	*ppcString = NULL;

	if (pgrp == NULL || pgrp->version != 1 ||
	    pgrp->type != genericdb_TABLE ||
	    pgrp->ncol < icol || pgrp->nrow < irow ||
	    icol < 0 || irow < 0 ||
	    ppcString == NULL || pgrp->ppc == NULL)
		return (genericdb_PARAMS);

	if (pgrp->ppc[(irow * pgrp->ncol) + icol] == NULL) {
		return (genericdb_NULL);
	}

	*ppcString = strdup(pgrp->ppc[(irow * pgrp->ncol) + icol]);
	return (genericdb_OK);
}

/*
 * genericdb_result_table_null
 *
 * Check whether a value is NULL.
 *
 *	gdbr		A table result, returned by genericdb_querySQL.
 *	irow		The row number of the table result to check.
 *	icol		The column number of the table result to check.
 *
 * Result: 1 if NULL, 0 if not NULL, -1 if parameters are bad.
 * Side Effects: None.
 */
int
genericdb_result_table_null(const genericdb_result gdbr, int irow, int icol) {

	genericdb_result_private *pgrp = (genericdb_result_private *) gdbr;

	if (pgrp == NULL || pgrp->version != 1 ||
	    pgrp->type != genericdb_TABLE ||
	    pgrp->ncol < icol || pgrp->nrow < irow ||
	    icol < 0 || irow < 0 || pgrp->ppc == NULL)
		return (-1);

	if (pgrp->ppc[(irow * pgrp->ncol) + icol] == NULL)
		return (1);

	return (0);
}

/*
 * genericdb_result_table_int
 *
 * This returns the string value in a particular cell of a table result.
 *
 *	gdbr		A table result, returned by genericdb_querySQL.
 *	irow		The row number of the table result to retrieve.
 *	icol		The column number of the table result to retrieve.
 *	ppcString	[OUTPUT PARAMETER] A pointer to an int.  This
 *		        pointer will be set to the value of the cell
 *			requested.
 *
 * Result: genericdb_OK if no error, or an error code.
 *         genericdb_NULL if the requested row & col are a NULL value.
 * Side Effects: None.
 */
genericdb_Error
genericdb_result_table_int(const genericdb_result gdbr, int irow, int icol,
    int *piInt)
{
	genericdb_result_private *pgrp = (genericdb_result_private *) gdbr;
	*piInt = 0;

	if (gdbr == NULL || pgrp->type != genericdb_TABLE ||
	    pgrp->ncol < icol || pgrp->nrow < irow ||
	    icol < 0 || irow < 0)
		return (genericdb_PARAMS);
	if (pgrp->ppc[(irow * pgrp->ncol) + icol] == NULL)
		return (genericdb_NULL);

	*piInt = atoi(pgrp->ppc[(irow * pgrp->ncol) + icol]);
	return (genericdb_OK);
}

/*
 * This routine frees all memory and resources associated with a particular
 * result.  The result may not be used once it has been freed.
 *
 * In order to minimize the risk of freeing an uninitialized result,
 * we check that the type is genericdb_TABLE and the version is 1.
 * That way we can create a genericdb_result on the stack and pass
 * it directly to genericdb_result_free with very low probability
 * of an attempt to free unallocated memory.
 *
 * To fully prevent that a user can call
 * {
 *    genericdb_result r;
 *    memset(&r, 0, GENERICDB_RESULT_SIZE);
 *    genericdb_result_free(r);
 * }
 */
void
genericdb_result_free(genericdb_result gdbr)
{
	genericdb_result_private *pgrp = (genericdb_result_private *) gdbr;

	if (gdbr == NULL || pgrp->type != genericdb_TABLE ||
	    pgrp->version != 1)
		return;

	if (pgrp->ppc) {
		int ii, jj;

		for (ii = 0; ii < pgrp->nrow; ii++) {
			for (jj = 0; (jj < pgrp->ncol); jj++) {

				if (pgrp->ppc[(ii * pgrp->ncol) + jj])
					free(pgrp->ppc[(ii * pgrp->ncol) +
					    jj]);

			}
		}

		free(pgrp->ppc);
	}
	free(pgrp);
}

/*
 * genericdb_querySQL
 *
 * This procedure is used to submit SQL for which a result table
 * is expected.
 *
 *   pgdb	The already properly opened generic DB handle.
 *   pcSQL	A null terminated string containing a properly
 *              formatted and expressed set of statements in
 *		SQL92.
 *   pppcValue	[OUTPUT PARAMETER] The user supplies a pointer
 *		to a ptr to a ptr to a character.  If results
 *		can be obtained, a two dimensional array of result
 *		strings is allocated, NumRows by NumCols in size.
 *
 *		EXAMPLE:
 *		{
 *			char ***pppcValue;
 *			int iNumRows, iNumCols;
 *			genericdb_Error gdbe = genericdb_OK;
 *			genericdb_result gdbr;
 *
 *			genericdb *pgdb =
 *			    genericdb_open("/",
 *				genericdb_READ | genericdb_WRITE, 0, NULL,
 *				&gdbe);
 *
 *			if (gdbe != genericdb_OK) return;
 *
 *                      gdbe = genericdb_query(pgdb, pcSQL, &gdbr);
 *			if (gdbe != genericdb_OK) ...
 *
 *			for (i = 0; i < genericdb_result_nrows(gdbr); i++) {
 *				for (j = 0; j < genericdb_result_ncols(gdbr);
 *				    j++) {
 *
 *					char *pc;
 *
 *					if (genericdb_result_table_str(i, j,
 *					    &pc) != genericdb_OK) ...
 *
 *					do_something(pc);
 *				}
 *			}
 *		}
 *
 *		NOTE:	A NULL value in the db is denoted as a NULL
 *			pointer for the value (in the row/column
 *			position).  An empty string, with only the
 *			content of one NULL byte ('\0') denotes an
 *			empty string value - which is different than
 *			a NULL value in the database.
 *
 *		NOTE:	Type information is lost using this encoding,
 *			but it is assumed that most of the data accessed
 *			using the genericdb API will either be a string
 *			or integer (which can be converted trivially
 *			from a C string.)  This API requires that the
 *			user check values returned from the database
 *			to ensure they are of the expected form.
 *
 * Returns: Error code.
 * Side Effects: As per the SQL submitted.
 */
genericdb_Error
genericdb_querySQL(genericdb *pgdb, const char *pcSQL, genericdb_result *pr)
{
#ifdef SQLDB_IS_USED
	genericdb_private *pg = (genericdb_private *) pgdb;
	int i, j, nrow, ncol;
	char *pcErr = NULL, *pc, **ppcResults, **ppc;
	genericdb_result_private *prp = NULL;

	if (pgdb == NULL || pg->psql == NULL || pcSQL == NULL || pr == NULL)
#endif
		return (genericdb_PARAMS);

#ifdef SQLDB_IS_USED
	/* The default result, in case of an error, is no result. */
	*pr = NULL;

#ifndef NDEBUG
	/* Do not do the expensive copy of the query unless we are debugging. */
	if ((pg->log & genericdbpriv_LOG_DEBUG) != 0) {
		pc = (char *)malloc(strlen(pcSQL) + 81);
		if (pc == NULL) {
			genericdbpriv_log(genericdbpriv_LOG_DEBUG, pg,
			    "submit sql[<could not allocate string>]");
		} else {
			(void) snprintf(pc, strlen(pcSQL) + 81,
			    "submit sql [%s]", pcSQL);
			genericdbpriv_log(genericdbpriv_LOG_DEBUG, pg, pc);
			free(pc);
		}
	}
#endif

	if (sqlite_get_table(pg->psql, pcSQL, &ppc, &nrow, &ncol, &pcErr)
	    != SQLITE_OK)
	{

		if (pcErr == NULL)
			pcErr = "";
		pc = (char *)malloc(strlen(pcErr) + 81);
		(void) snprintf(pc, 81 + strlen(pcErr), "sqlite [%s]", pcErr);
		genericdbpriv_log(genericdbpriv_LOG_ERROR, pg, pc);
		free(pc);

		return (genericdb_SQLERR);
	}

	/* Copy results from the array returned by SQLite to private form. */
	ppcResults = (char **)malloc((nrow * ncol) * sizeof (char *));
	for (i = 0; i < nrow; i++) {
		for (j = 0; j < ncol; j++) {

			/* Skip the first row.  SQLite returns column names. */
			int x = ((i + 1) * ncol) + j;
			if (ppc[x] == NULL) {
				ppcResults[(i * ncol) + j] = NULL;
			} else {
				ppcResults[(i * ncol) + j] = strdup(ppc[x]);
			}
		}
	}
	sqlite_free_table(ppc);

	/* Create the complete private result to return. */
	prp = (genericdb_result_private *)
	    malloc(sizeof (genericdb_result_private));
	prp->version = 1;
	prp->type = genericdb_TABLE;
	prp->nrow = nrow;
	prp->ncol = ncol;
	prp->ppc  = ppcResults;
	*pr = (genericdb_result *) prp;

	return (genericdb_OK);
#endif
}

/*
 * genericdb_remove_db
 *
 * This function will remove the db entirely.  This way, the caller does
 * not need to know the location or path of the db.  In fact, the db is
 * saved in a 'save file' transparently.
 *
 *    pcroot	The alternate install root.
 *
 * Return 0 on success 1 on failure.
 */
genericdb_Error
genericdb_remove_db(const char *pcroot)
{
	char *pc = genericdbpriv_create_dbpath(pcroot);
	char buf[PATH_MAX];
	genericdb *pdb = NULL;
	genericdb_Error err;
	genericdb_result r;
	char *version = NULL;

	int result = 0;
	if (pc == NULL) {

		return (1);

	}

	r = NULL;

	pdb = genericdb_open(pcroot, GENERICDB_R_MODE, 0, NULL, &err);
	if (pdb != NULL && err == genericdb_OK &&
	    genericdb_querySQL(pdb, GENERICDB_INSTALL_DB_STATUS_CHECK,
		&r) == genericdb_OK &&
	    genericdb_result_table_str(r, 0, 0, &version) == genericdb_OK &&
	    strcmp(version, GENERICDB_INSTALL_DB_STATUS_OK_VAL) != 0) {

		if (remove(pc)) {
			result = 1;
		}

	} else {
		buf[0] = '\0';
		if (snprintf(buf, PATH_MAX, "%s.save", pc) >= PATH_MAX ||
		    rename(pc, buf))  {

			result = 1;
		}
	}

	free(pc);
	if (pdb)
		genericdb_close(pdb);
	if (version)
		free(version);
	genericdb_result_free(&r);

	return (result);
}

/*
 * genericdb_remove_save
 *
 * This function will remove the saved db entirely.  This way, the caller does
 * not need to know the location or path of the db.  This is the db file which
 * was saved in a 'save file' transparently by genericdb_remove_db.  If there
 * is no such file, this routine succeeds.
 *
 *    pcroot	The alternate install root.
 *
 * Return 0 on success 1 on failure.
 */
genericdb_Error
genericdb_remove_save(const char *pcroot)
{
	char *pc = genericdbpriv_create_dbpath(pcroot);
	char buf[PATH_MAX];
	int result = 0;

	if (pc == NULL) {
		return (1);
	}

	buf[0] = '\0';
	if (snprintf(buf, PATH_MAX, "%s.save", pc) >= PATH_MAX ||
	    remove(buf)) {
		result = 1;
	}

	free(pc);
	return (result);
}
/*
 * genericdb_db_size
 *
 *    pcroot	The install root, relative to which the db file may be found.
 *
 * Returns the size of the database file in bytes.  Note the result can be 0
 * if the file does not exist.  A return value of -1 indicates that the
 * parameter was bad in that it could not be used to create a db path or
 * it pointed at a path which exists which is not a file!
 *
 * Side effects: None.
 */
long
genericdb_db_size(const char *pcroot)
{
	char *pc = genericdbpriv_create_dbpath(pcroot);
	struct stat sb;

	if (pc == NULL)
		return (-1);

	if (access(pc, F_OK) != 0)
		return (0);

	if (stat(pc, &sb) != 0 || S_ISREG(sb.st_mode) == 0)
		return (-1);

	return ((long)sb.st_size);
}

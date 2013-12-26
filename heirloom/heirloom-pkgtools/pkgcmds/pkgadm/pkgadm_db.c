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

/*	from OpenSolaris "pkgadm_db.c	1.4	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgadm_db.c	1.3 (gritter) 2/24/07
 */

/*
 * pkgadm_db.c
 *
 * Handle operations on the registry4 database - converting from or to
 * the text format from or to the binary format.
 */
 
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#include "pkgadm_msgs.h"
#include "pkgadm.h"
#include "libinst.h"
#include "dbtables.h"
#include "genericdb.h"
#include "dbsql.h"

#define	PKGADM_NAME	"pkgadm"

/*
 * global variable!
 *
 * This variable is needed because pkgadm calling convention is fixed
 * and we need to override the 'pkgadm_revert' interface to both do
 * a reversion and not.  Note that the variable is only consumed in
 * two place and written in one place.
 */
static int global_get_contents = 0;

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
 * get_alt_root
 *
 * Gets the root directory path using the established rules.  Use the
 * environment variable PKG_INSTALL_ROOT if defined, or prefer the -R
 * alternate root, if set.
 * Returns a string, which must be freed by the caller.
 * Side effects: none (note - this does NOT change any global variables).
 */
static char *
get_alt_root(int argc, char **argv)
{
	int i;
	char *pc = "/";

	if (getenv("PKG_INSTALL_ROOT"))
		pc = getenv("PKG_INSTALL_ROOT");

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-R") == 0 && ((i + 1) < argc))
			pc = argv[i + 1];
	}

	return (strdup(pc));
}

/*
 * wants_to_exit
 *
 * This is an interactive loop which asks whether the user really wants to
 * clobber a db which exists with most likely stale data.  This routine
 * should be enhanced to determine if the contents file is older than the
 * database.
 *
 * Returns: 1 if the user wants to exit (not perform the action), or 0
 * if the user does want to perform the action.
 * Side effects: none
 */
static int
wants_to_exit()
{
	int done = 0;
	char buf[BUFSZ];
	while (!done) {
		/* TRANSLATION_NOTE: do not translate 'y' and 'n' */
		(void) printf(DB_EXISTS_PROMPT);
		if (fgets(buf, BUFSZ, stdin) == NULL) {
			buf[0] = '\0';
		}
		switch (buf[0]) {
		case 'n':
		case 'N':
			return (1);

		case 'Y':
		case 'y':
		case '\n':
			return (0);

		default:
			/*
			 * Trim off the trailing carriage return.
			 * Or, in the case of excessively long input,
			 * terminate a chunk of it to output.
			 */
			buf[strlen(buf) - 1] = '\0';
			/* TRANSLATION_NOTE: do not translate 'y' and 'n' */
			(void) printf(DB_EXISTS_ADMONISH, buf);
			break;
		}
	}
	/* Cannot reach this point */
	return (1);
}

/*
 * init_db
 *
 * Initialize the db so that it is surely going to be in a fresh state by
 * the time this routine returns with a '0' result.
 *
 *    pdb	An initialized database session handle.
 *    exists	Set to 1 if the db exists, 0 otherwise.
 *
 * Return: 0 on success, nonzero otherwise.
 * Side effects: Drops existing tables and creates new ones.
 */
static int
init_db(genericdb *pdb)
{
#ifdef SQLDB_IS_USED
	genericdb_Error E;

	if ((E = genericdb_submitSQL(pdb, pcContents)) != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, pcDepComments))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, pcSQL_pkginfo))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, pcSQL_depend))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, pcSQL_platform))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, pcPatch))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else {
		return (0);
	}

	return (1);
#else
	return (0);
#endif
}

/*
 * Build an index on the pkg_table and patch_table.
 *
 * Parameters:
 *             pdb - pointer to genericdb struct
 * Returns:
 *             0 - Success
 *            -1 - Failure
 */
static int
build_indexes(genericdb *pdb)
{
#ifdef SQLDB_IS_USED
	genericdb_Error E;

	if ((E = genericdb_submitSQL(pdb, SQL_CREATE_INDEX_PATH))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, SQL_CREATE_INDEX_PKGS))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else if ((E = genericdb_submitSQL(pdb, SQL_CREATE_INDEX_PATCH))
	    != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));

	} else {
		return (0);
	}

	return (-1);
#else
	return (0);
#endif
}

/*
 * convert_to_db
 *
 * Return: 0 on success.  Will be returned to the shell user who evoked
 *   pkgadm upgrade as the result value.
 * Side Effects: Will drop existing tables and regenerate them from the
 *   var/sadm/pkg and var/sadm/install/contents files.
 */
int
convert_to_db(int argc, char **argv)
{
#ifdef SQLDB_IS_USED
	DIR *pd = NULL;
	struct dirent *psd = NULL;
	struct dstr d;
	FILE *fp = NULL;
	char buf[PATH_MAX];
	char path[PATH_MAX];
	char contents[PATH_MAX];
	char contents_save[PATH_MAX];
	int result = 0; /* Assume success. */
	genericdb *pdb = NULL;
	genericdb_Error E;
	int r = 0, exists = 0;
	char *pcroot = NULL;
#ifndef NDEBUG
	char *params[] = { "-l", "3", NULL };
	int nparams = 2;
#else
	char *params[] = { NULL };
	int nparams = 0;
#endif

	init_dstr(&d);

	/* Either accept 1 argument or 3 arguments where the second is -R */

	if (argc != 1 && (argc != 3 || strcmp(argv[1], "-R")))
		return (usage());

	pcroot = get_alt_root(argc, argv);

	if (set_inst_root(pcroot) == 0) {
		return (1);
	}

	set_PKGpaths(get_inst_root());

	if (lockinst(PKGADM_NAME, "all packages", "convert_to_db") == 0) {
		if (pcroot)
			free(pcroot);
		return (1);
	}


	/* If db exists warn the user and likely - exit. */
	if ((exists = genericdb_exists(pcroot))) {

		(void) printf(DB_EXISTS_WARN);
		if (wants_to_exit()) {
			unlockinst();
			if (pcroot)
				free(pcroot);
			return (0);
		}
	}

	/*
	 * To clean up the existing db, just 'remove it' to saved file.
	 * This must be done whether exists is set or not, since exists
	 * does not tell us whether the db file exists, but rather whether
	 * an initialized and properly set up db table exists.
	 */
	r = genericdb_remove_db(pcroot);

	/* It is an error if the db exists and we were unable to remove it. */
	if (exists && r != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_NO_DROP_DB);
		unlockinst();
		if (pcroot)
			free(pcroot);
		return (1);
	}

	/* Try to open the db.  This will create a new file. */

	pdb = genericdb_open(pcroot, 0644, nparams, params, &E);
	if (pdb == NULL || E != genericdb_OK) {

		log_msg(LOG_MSG_ERR, MSG_NO_OPEN_DB);
		result = 1;
		goto to_db_done;
	}

	lockupd("initialize database and generate conversion string.");

	if (init_db(pdb)) {
		result = 1;
		goto to_db_done;
	}

	/* Convert the pkginfo and depend data to database form. */

	if (pcroot[strlen(pcroot) - 1] == '/') {

		if (snprintf(contents, PATH_MAX, "%s" VSADMREL "/install/contents",
		    pcroot) >= PATH_MAX ||
		    snprintf(contents_save, PATH_MAX,
			"%s" VSADMREL "/install/contents.save",
			pcroot) >= PATH_MAX) {
			result = 1;
			goto to_db_done;
		}

		if (snprintf(path, PATH_MAX, "%s" VSADMREL "/pkg", pcroot)
		    >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			result = 1;
			goto to_db_done;
		}
	} else {
		if (snprintf(contents, PATH_MAX, "%s" VSADMDIR "/install/contents",
		    pcroot) >= PATH_MAX ||
		    snprintf(contents_save, PATH_MAX,
			"%s" VSADMDIR "/install/contents.save",
			pcroot) >= PATH_MAX) {
			result = 1;
			goto to_db_done;
		}

		if (snprintf(path, PATH_MAX, "%s" VSADMDIR "/pkg", pcroot)
		    >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			result = 1;
			goto to_db_done;
		}
	}

	if ((pd = opendir(path)) == NULL) {
		log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, path, strerror(errno));
		result = 1;
		goto to_db_done;
	}

	/* Generate a transaction consisting of all needed pkg inserts. */

	if (append_dstr(&d, "BEGIN TRANSACTION;")) {
		result = 1;
		goto to_db_done;
	}

	/* Generate SQL for depend and pkginfo. */

	while ((psd = readdir(pd)) != NULL) {

		/* Rule out . .. and hidden files. */
		if (psd->d_name[0] == '.')
			continue;

		if (snprintf(buf, PATH_MAX, "%s/%s/pkginfo",
		    path, psd->d_name) >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			result = 1;
			goto to_db_done;
		}

		if (access(buf, R_OK) != 0) {
			log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, buf,
			    strerror(errno));
			result = 1;
			goto to_db_done;
		} else {
			if ((fp = fopen(buf, "r")) == NULL) {
				log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, buf,
				    strerror(errno));
				result = 1;
				goto to_db_done;
			}

			/* process the pkg */
			if (convert_pkginfo_to_sql(&d, fp, psd->d_name)) {
				result = 1;
				goto to_db_done;
			}

			if (fclose(fp) != 0) {
				log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, buf,
				    strerror(errno));
				result = 1;
				goto to_db_done;
			}
			fp = NULL;
		}

		if (snprintf(buf, PATH_MAX, "%s/%s/install/depend", path,
		    psd->d_name) >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			result = 1;
			goto to_db_done;
		}

		if (access(buf, R_OK) != 0) {
			if (access(buf, F_OK) == 0) {
				log_msg(LOG_MSG_ERR, MSG_NOT_READABLE, buf);
				result = 1;
				goto to_db_done;
			}
		} else {
			if ((fp = fopen(buf, "r")) == NULL) {
				log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS,
				    buf, strerror(errno));
				result = 1;
				goto to_db_done;
			}

			convert_depend_to_sql(&d, fp, psd->d_name);

			if (fclose(fp) != 0) {
				log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS,
				    psd->d_name, strerror(errno));
				result = 1;
				goto to_db_done;
			}
			fp = NULL;
		}

	}

	if (convert_contents_to_sql(&d, pcroot)) {
		result = 1;
		goto to_db_done;
	}

	/*
	 * We have to create the dbstatus table since the db file was
	 * removed.  This would normally be done in genericdb_exists,
	 * but we have to remove the entire db file before getting here.
	 */
	if (append_dstr(&d, GENERICDB_INSTALL_DB_CREATE_TABLE) ||
	    append_dstr(&d, GENERICDB_INSTALL_DB_STATUS_SET_OK) ||
	    append_dstr(&d, "COMMIT;")) {
		result = 1;
		goto to_db_done;
	}

	/* Insert all information. */

	lockupd("commit contents changes to db");

	if ((E = genericdb_submitSQL(pdb, d.pc)) != genericdb_OK) {

#ifndef NDEBUG
		FILE *fpx = fopen("debug.txt", "w");
		if (fpx) {
			(void) fwrite(d.pc, 1, strlen(d.pc), fpx);
			(void) printf("wrote query in debug.txt\n");
			(void) fclose(fpx);
		} else {
			(void) printf("could not write query in debug.txt\n");
		}
#endif

		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(E));
		result = 1;
		goto to_db_done;
	}

	lockupd("put old patchinfo");

	if (put_patch_info_old(pcroot, pdb) == -1) {
		log_msg(LOG_MSG_ERR, MSG_PATCH_UPGD);
	}

	lockupd("rebuild indexes");

	if (build_indexes(pdb)) {
		log_msg(LOG_MSG_INFO, MSG_BUILD_INDEXES);
		result = 1;
		goto to_db_done;
	}

	lockupd("commit changes");

	/* Commit the changes.  Copy contents to contents.save. */
	if (result == 0) {

		if (rename(contents, contents_save) != 0) {
			log_msg(LOG_MSG_ERR, MSG_RENAME_FAILED, contents,
			    contents_save, strerror(errno));
			result = 1;
			goto to_db_done;
		}

		/*
		 * Remove the saved version of the database, from
		 * genericdb_remove_db, if there was one.   This is for
		 * good hygene only, so we do not fail if this operation
		 * does not succeed.
		 */
		(void) genericdb_remove_save(pcroot);
	}

to_db_done:

	genericdb_close(pdb);

	unlockinst();

	/* Remove a partially created database file if there was an error. */
	if (result != 0) {

		if (genericdb_exists(pcroot) &&
		    genericdb_remove_db(pcroot) != 0) {
			log_msg(LOG_MSG_INFO, MSG_NO_DB_REMOVE,
			    strerror(errno));
		}
	}

	if (pcroot)
		free(pcroot);

	if (fp)
		(void) fclose(fp);

	free_dstr(&d);
	if (pd)
		(void) closedir(pd);
	return (result);
#else
	return (0);
#endif
}

/*
 * convert_from_db
 *
 * Regenerates contents, pkginfo and depend files from the db.
 * The database is removed, unless the global_get_contents flag is nonzero.
 *
 * IMPLEMENTOR's NOTE:  The global flag global_get_contents was used here
 * instead of an additional parameter because the function prototype is
 * determined by the calling convention established in pkgadm.
 *
 * Return 0 on success, nonzero on failure.
 * Side effects: Big effects above are the only effects.
 */
int
convert_from_db(int argc, char **argv)
{
	int result = 0; /* Assume we succeed. */
	genericdb *pdb = NULL;
	genericdb_Error E;
	char *pcroot = NULL;

#ifndef NDEBUG
	char *params[] = { "-l", "3", NULL };
	int nparams = 2;
#else
	char *params[] = { NULL };
	int nparams = 0;
#endif

	/* Either accept 1 argument or 3 arguments where the second is -R */
	if (argc != 1 && (argc != 3 || strcmp(argv[1], "-R")))
		return (usage());

	pcroot = get_alt_root(argc, argv);

	if (set_inst_root(pcroot) == 0) {
		return (1);
	}

	set_PKGpaths(get_inst_root());

	if (lockinst(PKGADM_NAME, "all packages", "convert_from_db") == 0) {
		return (1);
	}

	if (genericdb_exists(pcroot) == 0) {
		log_msg(LOG_MSG_ERR, DB_NONEXISTENCE);
		result = 1;
		goto to_pkg_done;
	}

	/* Open db */
	pdb = genericdb_open(pcroot, GENERICDB_WR_MODE, nparams, params, &E);
	if (pdb == NULL || E != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_NO_OPEN_DB);
		result = 1;
		goto to_pkg_done;
	}

	if (convert_db_to_contents(pdb, pcroot, global_get_contents)) {
		result = 1;
		goto to_pkg_done;
	}


to_pkg_done:

	if (result == 0 && global_get_contents == 0) {

		int e;
		char contents_save[PATH_MAX];

		if (genericdb_remove_db(pcroot) != 0) {
			log_msg(LOG_MSG_ERR, MSG_NO_DB_REMOVE, strerror(errno));
			result = 1;
		}

		/*
		 * Cleaning up the old contents.save file on a revert is
		 * good hygene, but we wont fail if it doesnt work for
		 * some reason.
		 */

		if (pcroot[strlen(pcroot) - 1] == '/') {

			e = snprintf(contents_save, PATH_MAX,
			    "%s" VSADMREL "/install/contents.save", pcroot);

		} else {

			e = snprintf(contents_save, PATH_MAX,
			    "%s" VSADMDIR "/install/contents.save", pcroot);

		}

		if (e > 0 && e < PATH_MAX) {
			(void) remove(contents_save);
		}

	}

	if (pcroot)
		free(pcroot);

	if (pdb)
		genericdb_close(pdb);

	unlockinst();

	return (result);
}

/*
 * get_contents
 *
 * This is the same as convert_from_db except that the db is not
 * removed afterwards.
 *
 * Return: 0 on success, 1 on failure.
 * Side effects:  The contents file will be replaced with a new file which
 *    is generated from the rows of the pkg_table.
 */
int
get_contents(int argc, char **argv)
{
#ifdef SQLDB_IS_USED
	/*
	 * *********************************************************************
	 * This supports both the SQL and flat text format contents databases.
	 * *********************************************************************
	 */
	global_get_contents = 1;
	return (convert_from_db(argc, argv));
#else
	/*
	 * *********************************************************************
	 * This supports only the flat text format contents database.
	 * *********************************************************************
	 */
	return (0);
#endif
}

/*
 * gen_special
 *
 * Evokes the command to create a new contents file on the basis of the
 * special_contents file's entries.  The command line is used to get the
 * alternate root if one is defined.
 *
 * Return: 0 on success, 1 on failure.
 * Side effects: This function has a large effect on success:  It replaces
 *    the contents file.
 */
int
gen_special(int argc, char **argv)
{
#ifdef SQLDB_IS_USED
	char *pcroot = get_alt_root(argc, argv);
	int result = 0;

	/* If the db has not been upgraded, there is nothing to do. */
	if (genericdb_exists(pcroot) == 0) {
		log_msg(LOG_MSG_ERR, MSG_NO_DB_NO_SPECIAL);
		result = 1;
		goto gen_special_done;
	}

	if (set_inst_root(pcroot) == 0) {
		return (1);
	}

	set_PKGpaths(get_inst_root());

	if (lockinst(PKGADM_NAME, "all packages", "gen_special") == 0) {
		result = 1;
		goto gen_special_done;
	}

	result = special_contents_generator(pcroot);

gen_special_done:
	if (pcroot)
		free(pcroot);
	unlockinst();

	return (result);
#else
	return (0);
#endif
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

	pcroot = get_alt_root(argc, argv);
	e = genericdb_exists(pcroot);

	if (pcroot)
		free(pcroot);
	if (e)
		(void) printf("%s\n", PKGADM_DBSTATUS_DB);
	else
		(void) printf("%s\n", PKGADM_DBSTATUS_TEXT);

	return (0);
}

/*
 * quit
 *
 * This function is needed only because we must call set_inst_root in order
 * to obtain the proper behavior from lockinst and unlockinst.  set_inst_root
 * may call quit() which it defines as an external function.
 *
 *   retcode   The error code which should be returned.
 *
 * In our case we do NOTHING HERE.  This is only present in order to satisfy
 * the linker which demands the external 'quit' function be defined.
 * pkgadm exits in an orderly way through main, so it would be a mistake to
 * quit due to a call back from a library.
 */
/* ARGUSED */
void
quit(int retcode)
{

}

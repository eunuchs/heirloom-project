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

/*	from OpenSolaris "dbsql.c	1.13	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dbsql.c	1.4 (gritter) 2/25/07
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pkglib.h>
#include <pkgstrct.h>
#include <assert.h>
#include <errno.h>

#include "pkglocale.h"
#include "genericdb.h"
#include "dbtables.h"
#include "dbsql.h"
#include "cfext.h"
#include "pkglibmsgs.h"
#include "libadm.h"

/* Entry is ready to installf -f */
#define	INST_RDY	'+'

/* Entry is ready for removef -f */
#define	RM_RDY	'-'

/* Default pkg status value SQL safe */
#define	PSTATUS_OK '0'

/* Pkg status indicating that a status flag exists */
#define	PSTATUS_NOT_OK '1'

/* Default status value not SQL safe */
#define	PINFO_STATUS_OK '\0'

/* Contents of the FS object need to be confirmed */
#define	CONFIRM_CONT '*'

/* Flag indicating a delete pinfo operation */
#define	DELETE  1

/* Flag indicating an update pinfo operation */
#define	UPDATE  0

/* max size of string used when constructing SQL strings */
#define	MAXSQLBUF	2048

/* how big to increase dynamic buffer when it is filled */
#define	VALSIZ	1024

#define	NEWLINE	'\n'

#define	ESCAPE	'\\'

#define	APOSTRAPHE	  39

#define	DEBUG		(void)printf

#define	LSIZE   2048

/*
 * Constant used to increase memory allocated for the sql_buf array.
 * The additional memory is needed to accomodate the non pkg_table
 * entries, i.e. pkginfo_table, dep_table...
 */
#define	ADDL_TBL_BUF 1024

int eptnum;
struct cfent **eptlist;

/* private globals */
static char sepset[] = ":=\n";
static char qset[] = "'\"";
static char **sql_buf;
static char *db_path = NULL;
static int sql_db = 0;
static int entries = 0;
static int del_ctr = 0;
static int put_ctr = 0;
static int get_ctr = 0;

genericdb_result gdbrg;

/* public globals */

/* Defined in pkginstall/main.c installf/main.c */
extern struct cfent **extlist;

/* externally defined functions */

/* Defined in libadm:pkgparam.c */
extern char *get_install_root(void);

/* Defined in libpkg:webpkg.c */
extern void echo_out(int, char *, ...);

/* private functions */
static int construct_SQL(struct cfextra **, struct dstr *, char *);
static int prepare_db_struct(genericdb_result);
static int fill_db_struct(int, char **);
static void set_db_entries(int);
static int mem_db_put(struct cfent *, char *, char, char *);
static int mem_db_replace(struct cfent *, char *, char, char *);
static int process_pinfo(struct pinfo *, char *, struct dstr *, int, char *);
static int output_patch_row(char **);
static int output_patch_pkg(int, char **, int, struct dstr *);
static void output_backout_dir(char **col);
static char *quote(const char *);
static int get_installroot_len(char *ir);

/*
 * Returns whether or not the current package process is using the SQL DB.
 *
 * Returns:
 *		   0 for not using the SQL DB
 *			 non 0 if the SQL DB is being used.
 */

int
get_SQL_db()
{
	return (sql_db);
}

/*
 * This function sets the path to the SQL DB and sets a flag that tells the
 * the caller the DB is in use. Use get_SQL_db to determine if the SQL DB
 * is being used.
 *
 * Parameters:
 *			 ir - The install root
 *
 * Returns:
 *			0 if the SQL DB is not being used
 *			 1 if the SQL DB is being used
 *			-1 for failure
 */

int
set_SQL_db(char *ir)
{
	sql_db = 0;

	if (set_SQL_db_path(ir)) {
		return (-1);
	}

	if (genericdb_exists(get_install_root())) {
		sql_db = 1;
		return (1);
	}

	return (0);
}

/*
 * This function gets the path to the SQL DB.
 *
 * Parameters:
 *				None
 * Returns:
 *			NULL - if the path cannot be set
 *			 db_path - the path toe the SQL DB
 */

char *
get_SQL_db_path(void)
{
	if (db_path && *db_path) {
		return (db_path);
	} else {
		return (NULL);
	}
}

/*
 * This function gets the path to the SQL DB.
 *
 * Parameters: path - The partial path to the SQL DB.
 *
 * Returns:
 *			0 - If the path is set
 *			-1 - If the path cannot be set
 */

int
set_SQL_db_path(char *path)
{
	if (path && *path) {
		if ((db_path = (char *)malloc(strlen(path) +
				strlen(DB) + 2)) == NULL) {
			return (-1);
		}
		if (snprintf(db_path, PATH_MAX, "%s/%s", path, DB) < 0) {
			return (-1);
		} else {
			return (0);
		}
	} else {
		return (-1);
	}
}

/*
 * Create and build a SQL index for all the tables. This only done at the
 * time the very first package is being installed.
 *
 * Parameters: The install root
 *
 * Returns:
 *			0 for success
 *			-1 for failure
 */

int
create_SQL_db(char *pkginstallroot)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb *gdb = NULL;
	genericdb_Error gdbe;
#ifdef NDEBUG
	char *log_lvl[] = { "-l", "7", NULL };
	int logargs = 2;
#else
	char *log_lvl[] = { NULL };
	int logargs = 0;
#endif

	if (set_SQL_db_path(pkginstallroot)) {
		return (-1);
	}

	/*
	 * If genericdb_exists returns 1, an initialized db exists and a
	 * new db should not be created.  If it returns 0, there will be
	 * a db file and a single table created - at least: reg4_admin_table
	 * This table is needed because the initial install must succeed
	 * when GENERICDB_INSTALL_DB_STATUS_SET_OK is used to set the field
	 * 'version' in reg4_admin_table to '1.0' upon the first successful
	 * installation.
	 */
	if (genericdb_exists(get_install_root()) != 0) {
		return (-1);
	}

	if ((gdb = genericdb_open(get_install_root(), W_MODE, logargs,
	    log_lvl, &gdbe)) != NULL && gdbe == genericdb_OK) {

		/* Create and submit all needed tables & indexes. */

		/* SQL_ENTRY_MAX should be enough. */
		assert((strlen(TRANS_INS) +
		    strlen(GENERICDB_INSTALL_DB_STATUS_SET_OK) +
		    strlen(pcContents) +
		    strlen(pcPatch) +
		    strlen(pcSQL_pkginfo) +
		    strlen(pcSQL_platform),
		    strlen(pcDepComments) +
		    strlen(pcSQL_depend) +
		    strlen(SQL_CREATE_INDEX_PKGS) +
		    strlen(SQL_CREATE_INDEX_PATH) +
		    strlen(SQL_CREATE_INDEX_PATCH) +
		    strlen(TRANS_INS_TRL) + 12) < SQL_ENTRY_MAX);

		(void) memset(sql_str, 0, SQL_ENTRY_MAX);

		if (snprintf(sql_str, SQL_ENTRY_MAX,
		    "%s %s %s %s %s %s %s %s %s %s %s %s",
		    /* create reg4_admin_table */
		    TRANS_INS,			/* BEGIN TRANSACTION */
		    /* set reg4_admin_table */
		    GENERICDB_INSTALL_DB_STATUS_SET_OK,
		    pcContents,			/* pkg_table */
		    pcPatch,			/* patch_table */
		    pcSQL_pkginfo,		/* pkginfo_table */
		    pcSQL_platform,		/* depplatform_table */
		    pcDepComments,		/* depcomment_table */
		    pcSQL_depend,		/* depend_table */
		    SQL_CREATE_INDEX_PKGS,	/* p2 on pkg_table(pkgs) */
		    SQL_CREATE_INDEX_PATH,	/* p1 on pkgtable(path) */
		    SQL_CREATE_INDEX_PATCH,	/* p3 on patch_table */
						/*   (patch, pkg) */
		    TRANS_INS_TRL)		/* COMMIT TRANSACTION */
		    >= SQL_ENTRY_MAX) {
			/*
			 * This should never happen, but failure here is
			 * better than handing garbage truncated SQL to
			 * the database.
			 */
			return (-1);
		}

		if ((gdbe = genericdb_submitSQL(gdb, sql_str))
		    != genericdb_OK) {
			progerr(gettext(ERR_CREATE_TABLES_DB),
			    genericdb_errstr(gdbe));
			return (-1);
		}

		genericdb_close(gdb);

	} else {
		return (-1);
	}

	return (0);
}

/*
 * This function results in eptlist being filled with entries from the SQL
 * DB that match any partial path.
 *
 * Parameters:
 *			 path - A partial path to search for in the SQL DB
 *
 * Returns:
 *			0 for success
 *			-1 for failure
 *
 * Side effects:  Fills eptlist with the results from the query.
 */

int
get_partial_path_db(char *path)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;

	snprintf(sql_str, SQL_ENTRY_MAX, "%s \'%c%s%c\'",
		SQL_SELECT_PPATH, '%', path, '%');

	if ((gdbe = query_db(get_install_root(), sql_str, &gdbr, R_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	if (gdbr) free(gdbr);

	return (0);
}

/*
 * This function results in eptlist being filled with entries from the SQL
 * DB that match a specific path.
 *
 * Parameters:
 *			 path - The path to search for.
 *			 db - A pointer to the genericdb structure
 *
 * Returns:
 *			0 - Success
 *			!0 - Failure
 *
 * Side effects:  Fills eptlist with the results from the query.
 */

int
get_path_db(char *path, genericdb *db)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;

	/* first try the exact match */
	snprintf(sql_str, SQL_ENTRY_MAX, "%s%s%s", SQL_SELECT,
			path, SQL_TRL_MOD);

	if (db == NULL) {
		if ((gdbe = query_db(get_install_root(), sql_str, &gdbr,
				R_MODE)) != genericdb_OK)
			return ((int)gdbe);
	} else {
		gdbe = genericdb_querySQL(db, sql_str, &gdbr);
	}


	if (prepare_db_struct(gdbr) != 0) {
		if (gdbr) free(gdbr);
		return (-1);
	}

	eptnum = get_db_entries();

	if (eptnum > 0) {
		return (0);
	}

	/* now try a LIKE match for symlinks and return them all */
	snprintf(sql_str, SQL_ENTRY_MAX, "%s \'%s%%%s", SQL_SELECT_PPATH,
			path, SQL_TRL_MOD);

	if (db == NULL) {
		if ((gdbe = query_db(get_install_root(), sql_str, &gdbr,
				R_MODE)) != genericdb_OK) {
			return ((int)gdbe);
		}
	} else {
		gdbe = genericdb_querySQL(db, sql_str, &gdbr);
	}

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	if (gdbr) free(gdbr);

	eptnum = get_db_entries();

	return (0);
}

/*
 * This function results in eptlist being filled with entries from the SQL
 * DB that match a specific path and a specific package.
 *
 * Parameters:
 *			 ept - Pointer to a cfent struct.
 *			 pkginst - The package to search for
 * Returns:
 *			0 - Success
 *			!0 - Failure
 *
 * Side effects:  Fills eptlist with the results from the query.
 */

int
get_pkg_path_db(struct cfent *ept, char *pkginst)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;

	snprintf(sql_str, SQL_ENTRY_MAX, "%s \'%c%s%c\' %s%s%s %s",
		SQL_SELECT_PKGS, '%', pkginst, '%', SQL_AND_PATH, ept->path,
		SQL_APOST, SQL_ORDER_LNGTH);

	if ((gdbe = query_db(get_install_root(), sql_str, &gdbr, R_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	if (gdbr) free(gdbr);

	return (0);
}

/*
 * This function results in eptlist being filled with all the entries
 * from the SQL DB that match a specific package.
 *
 * Parameters:
 *			 pkginst - The package to search for.
 *			 prog - The program name of the caller.
 *
 * Returns:
 *			0 - Success
 *			!0 - Failure
 *
 * Side effects:  Fills eptlist with the results from the query.
 */

int
get_pkg_db_all(char *pkginst, char *prog)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;
	struct dstr d;

	init_dstr(&d);

	if (prog == NULL) {
		snprintf(sql_str, SQL_ENTRY_MAX, "%s \'%c%s%c\' %s",
			SQL_SELECT_PKGS, '%', pkginst, '%', SQL_ORDER_LNGTH);
	} else {
		snprintf(sql_str, SQL_ENTRY_MAX, "%s %s \'%c%s%c%s", SQL_STATUS,
			SQL_STATUS_PKG, '%', pkginst, '%', SQL_TRL_MOD);
	}

	if (append_dstr(&d, sql_str))
		return (-1);

	if ((gdbe = query_db(get_install_root(), d.pc, &gdbr, R_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	eptnum = get_db_entries();

	if (gdbr) free(gdbr);
	free_dstr(&d);

	return (0);
}

/*
 * This function searches the SQL DB for a package or all paths and fills
 * the array of cfent structs with the results.
 *
 * Parameters:
 *			 pkginst - The package to search for.
 *			 prog - The program name of the caller.
 *			 gdb - A pointer to a genericdb struct.
 *
 * Returns:
 *			0 - Success
 *			!0 - Failure
 *
 * Side effects:  Fills eptlist with the results from the query.
 */

int
get_pkg_db(char *pkginst, char *prog, genericdb *gdb)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;
	char *fmt = "%s \'%s%s\' %s \'%s %s%s%s%s\' %s \'%s%s%s\'" \
		" %s \'%s%s%s\' %s \'%s %s\' %s \'%s %s%s%s\' %s \'%s %s%s\'" \
		" %s \'%s\' %s";

	/* Get all paths in the DB */
	if (pkginst == NULL) {
		if (snprintf(sql_str, SQL_ENTRY_MAX, "%s", SQL_SELECT_ALL)
			>= SQL_ENTRY_MAX) {
			progerr(gettext(ERR_MEM));
			return (-1);
		}
	} else {
		/*
		 * The following SQL is used primarily by pkgrm, installf,
		 * removef and pkgchk or any other command needing to get
		 * all the paths associated with aspecific pkgabbrev.
		 *
		 * sql_str results in a SQL statement that looks as follows:
		 *
		 * SELECT * FROM pkg_table WHERE pkgs GLOB
		 * pkgs GLOB [*+-!%@#~]SUNWcsu OR
		 * pkgs GLOB [*+-!%@#~]SUNWcsu[ :]* OR
		 * pkgs GLOB * [*+-!%@#~]SUNWcsu[ :]* OR
		 * pkgs GLOB * [*+-!%@#~]SUNWcsu OR
		 * pkgs GLOB * SUNWcsu[ :]* OR
		 * pkgs GLOB * SUNWcsu OR
		 * pkgs GLOB SUNWcsu[ :]* OR
		 * pkgs GLOB SUNWcsu
		 * ORDER BY length(path);
		 */
		if (snprintf(sql_str, sizeof (sql_str), fmt,
			SQL_SELECT_PKGS_GLOB, STATUS_INDICATORS, pkginst,
			SQL_GLOB, GLOB_WILD, STATUS_INDICATORS, pkginst,
						COLON_SPACE, GLOB_WILD,
			SQL_GLOB, STATUS_INDICATORS, pkginst, COLON_SPACE,
			SQL_GLOB, pkginst, COLON_SPACE, GLOB_WILD,
			SQL_GLOB, GLOB_WILD, pkginst,
			SQL_GLOB, GLOB_WILD, pkginst, COLON_SPACE, GLOB_WILD,
			SQL_GLOB, GLOB_WILD, STATUS_INDICATORS, pkginst,
			SQL_GLOB, pkginst,
			SQL_ORDER_LNGTH) >= SQL_ENTRY_MAX) {
				progerr(gettext(ERR_MEM));
				return (-1);
		}
	}

	if (gdb == NULL) {
		if ((gdbe = query_db(get_install_root(), sql_str,
				&gdbr, R_MODE)) != genericdb_OK)
			return ((int)gdbe);
	} else {
		gdbe = genericdb_querySQL(gdb, sql_str, &gdbr);
	}

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	if (gdbr) free(gdbr);

	eptnum = get_db_entries();

	return (0);
}

/*
 * Frees memory used by an eptlist.
 *
 * Parameters:
 *			 size - the number of entries allocated.
 *
 * Returns:
 *			None
 */

void
free_mem_ept(int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (eptlist[i] != NULL)
			free(eptlist[i]);
	}

	if (eptlist) free(eptlist);
}

/*
 * This function gets the pkgs associated with a given patchid and populates
 * the pkgs parameter that is passed in.
 *
 * Parameters:
 *			 ir - The installation root
 *			 patch - The patch to query.
 *			 pkgs - The memory reference used to hold the
 *				  queried packages. The caller must free()
 *				  this pointer. This parameter must be a
 *				  valid char *.
 *			 gdb - A pointer to the genericdb struct
 *
 * Returns:
 *			0 - Success
 *			!0 - Failure
 */

int
get_pkgs_from_patch_db(char *ir, char *patch, char **pkgs)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;

	snprintf(sql_str, SQL_ENTRY_MAX, "%s%s%s", SQL_SELECT_PKG_PATCH,
			patch, SQL_TRL_MOD);
	if ((gdbe = query_db(ir, sql_str, &gdbr, R_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	if (genericdb_result_table_nrows(gdbr) != 1 &&
			genericdb_result_table_ncols(gdbr) != 1)
		return (1);

	*pkgs = NULL;

	if (genericdb_result_table_str(gdbr, 0, 0, pkgs) != genericdb_OK)
		return (-1);

	return (0);
}

/*
 * This function causes the backout directory to be output to stdout.
 *
 * Parameters:
 *			 gdbr - The result of a previous SQL DB query.
 *
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
get_backout_dir(genericdb_result gdbr)
{
	int i, j;
	int nrows = 0;
	int ncols = 0;
	char **row;

	if ((row = (char **)calloc(genericdb_result_table_ncols(gdbr),
			sizeof (char *))) == NULL) {
		return (-1);
	}

	nrows = genericdb_result_table_nrows(gdbr);
	ncols = genericdb_result_table_ncols(gdbr);

	set_db_entries(nrows);

	for (i = 0; i < nrows; i++) {
		char *pc = NULL;
		for (j = 0; j < ncols; j++) {
			if (genericdb_result_table_str(gdbr, i, j, &pc)
					== genericdb_OK)
				row[j] = pc;
		}
		output_backout_dir(row);
	}

	if (row) free(row);

	return (0);
}

/*
 * This function parses a string that is formatted as follows:
 * PACH_INFO_<patchid>=Installed: <date>
 *					 From: <system>
 *					 Obsoletes: <patchlist>
 *					 Requires: <patchlist>
 *					 Incompatibles: <patchlist>
 * i.e.
 * PATCH_INFO_113964-08=Installed: Fri Jun 13 14:26:30 PDT 2003
 * From: fern Obsoletes: 113153-01 112914-04 Requires: 112903-03
 * 112911-02 115016-01 Incompatibles: 114543-09
 *
 * It parses the string and prepares the key variables to be
 * inserted into the patch_table. It then calls put_patchinfo_db()
 * with the parsed variables for insertion into the patch_table.
 *
 * Parameters:
 *			 patchinfo - Patch information originally stored in
 *						 the pkginfo.
 *			 pkgs - A string of packages.
 *			 patch - The patchid
 *			 ir - The installation root.
 *			 bo - The backout directory.
 *			 gdb - A pointer to a generic DB struct.
 *
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
parse_patchinfo(char *patch_info, char *pkgs, char *patch, char *bd,
		genericdb *gdb)
{
	char *p, *p2;
	char *date = NULL, *obs = NULL, *req = NULL, *inc = NULL;
	char *rev = NULL, *bcode = NULL, *sep = NULL;
	int obsf = 0; int reqf = 0; int incf = 0;

	if (patch == NULL) {
		return (-1);
	} else {
		if (strstr(patch_info, "Obsoletes:") == NULL ||
			strstr(patch_info, "Requires:") == NULL ||
			strstr(patch_info, "Incompatibles:") == NULL) {
			return (-1);
		}
		/*
		 * Parse the patch into 3 sections: base code, separator,
		 * revision. 111111-01 -> 111111 bcode; - sep; 01 rev
		 */
		p2 = p = patch;
		while (isalnum(*p)) p++;
		bcode = xstrdup(p2);
		bcode[p - p2] = '\0';
		sep = xstrdup(p);
		sep[1] = '\0';
		p++;
		p2 = p;
		while (isalnum(*p)) p++;
		rev = xstrdup(p2);
		rev[p - p2] = '\0';
	}


	if (patch_info == NULL) {
		return (-1);
	}

	if ((p = strstr(patch_info, "Installed:")) == NULL)
		return (-1);
	else
		p += 11;

	/* p points at the date string */
	p2 = strstr(patch_info, " From:");
	date = xstrdup(p);
	date[p2 - p] = '\0';

	/* Skip the system name */

	p = strstr(patch_info, "Obsoletes:");
	p += 11;
	while (isspace(*p)) p++;

	/* Where either pointing at an 'R'equires or a patchid */
	if (*p == 'R') {
		p += 10;
	} else {
		p2 = p;
		while (*p != 'R' && *(p + 1) != 'e') p++; p--;
		while (isspace(*p)) p--; p++;
		obs = xstrdup(p2);
		obs[p - p2] = '\0';
		obsf++;
		p += 10;
	}
	while (isspace(*p)) p++;

	/* Where either pointing at an 'I'ncompats or a patchid */
	if (*p == 'I') {
		p += 14;
	} else {
		p2 = p;
		while (*p != 'I' && *(p + 1) != 'n') p++; p--;
		while (isspace(*p)) p--; p++;
		req = xstrdup(p2);
		req[p - p2] = '\0';
		reqf++;
		p += 14;
	}
	while (isspace(*p)) p++;

	/* We're either pointing at "Packages:", EOL or a patchid */

	if (strncmp(p, "Packages:", 9) == 0) {
		/* There are no incompats defined. */
		p += 10;
		while (isspace(*p)) p++;
		p2 = p;
		/* There must be at least one package here */
		while (*p != '\r' && *p != '\n' && *p != '\0') p++;
		pkgs = xstrdup(p2);
		pkgs[p - p2] = '\0';
	} else if (*p != '\r' && *p != '\n' && *p != '\0') {
		p2 = p;
		while (*p != '\r' && *p != '\n' && *p != '\0') p++;
		if (*p2 != '\r' && *p2 != '\n' && *p != '\0') {
			inc = xstrdup(p2);
			inc[p - p2] = '\0';
			incf++;
		}
	}

	if (put_patch_info_db(patch, rev, bcode, sep, obs, req, inc,
			bd ? bd : "null", date, pkgs, obsf, reqf, incf, gdb))
		return (-1);

	if (bcode) free(bcode);
	if (date) free(date);
	if (obs) free(obs);
	if (inc) free(inc);
	if (req) free(req);
	if (sep) free(sep);
	if (rev) free(rev);

	return (0);
}

/*
 * This function returns the packages from the patch_table.
 *
 * Parameters:
 *			 gdbr - The result of a previous SQL DB query.
 *
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
get_patch_pkg(genericdb_result gdbr, char **str)
{
	int i, j, k;
	int nrows = 0;
	int ncols = 0;
	char **row;
	struct dstr dstr;

	if ((row = (char **)calloc(genericdb_result_table_ncols(gdbr),
			sizeof (char *))) == NULL) {
		return (-1);
	}

	init_dstr(&dstr);

	nrows = genericdb_result_table_nrows(gdbr);
	ncols = genericdb_result_table_ncols(gdbr);

	set_db_entries(nrows);

	for (i = 0; i < nrows; i++) {
		char *pc = NULL;
		for (j = 0; j < ncols; j++) {
			if (genericdb_result_table_str(gdbr, i, j, &pc)
					== genericdb_OK)
				row[j] = pc;
		}
		if (output_patch_pkg(ncols, row, 0, &dstr) == -1)
			return (-1);
		for (k = 0; k < ncols; k++) {
			row[k] = NULL;
		}
	}
	if (output_patch_pkg(ncols, row, 1, &dstr) == -1)
		return (-1);

	if (row) free(row);

	*str = strdup(dstr.pc);

	return (0);
}

/*
 * This function calls output_patch_row for each row returned by the SQL
 * DB.
 *
 * Parameters:
 *			 gdbr - The result of a previous SQL DB query.
 *
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
get_patch_entries(genericdb_result gdbr)
{
	int i, j, k;
	int nrows = 0;
	int ncols = 0;
	char **row;

	if ((row = (char **)calloc(genericdb_result_table_ncols(gdbr),
			sizeof (char *))) == NULL) {
		return (-1);
	}

	nrows = genericdb_result_table_nrows(gdbr);
	ncols = genericdb_result_table_ncols(gdbr);

	set_db_entries(nrows);

	for (i = 0; i < nrows; i++) {
		char *pc = NULL;
		for (j = 0; j < ncols; j++) {
			if (genericdb_result_table_str(gdbr, i, j, &pc)
					== genericdb_OK)
				row[j] = pc;
		}
		if (output_patch_row(row)) {
			return (-1);
		}
		for (k = 0; k < ncols; k++) {
			row[k] = NULL;
		}
	}

	if (row) free(row);

	return (0);
}

/*
 * Append string 2 to string 1
 * This function returns the patch information from the SQL DB.
 *
 * Parameters:
 *				str1 - The variable that holds the resultant
 *					appended character
 *				str2 - The string to append.
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
appendstr(char **str1, const char *str2)
{
	char *res;

	if (str2 == NULL)
		return (-1);

	if (*str1 == NULL) {
		*str1 = strdup(str2);
	} else {
		res = (char *)malloc(strlen(*str1) + strlen(str2) + 1);
		(void) sprintf(res, "%s%s", *str1, str2);
		*str1 = strdup(res);
		if (res) free(res);
	}

	return (0);
}

/*
 * Append string 2 to string 1
 * This function returns the patch information from the SQL DB.
 *
 * Parameters:
 *				str1 - The variable that holds the resultant
 *					appended character
 *				str2 - The string to append.
 *
 * Returns:
 *			0 - Success
 *			-1 - Failure
 */

int
appendstr_noalloc(char **str1, const char *str2)
{
	if (str2 == NULL)
		return (-1);

	if (*str1 == NULL) {
		*str1 = strdup(str2);
	} else {
		(void) sprintf(*str1, "%s%s", *str1, str2);
	}

	return (0);
}

/*
 * Append string 2 to string 1.
 * Caller must ensure memory has been allocated for str1.
 *
 * Parameters:
 *				str1 - The variable that holds the resultant
 *					appended character
 *				c - The character to append.
 *
 * Returns:
 *			None
 */

void
appendchar_noalloc(char **str1, char c)
{
	if (*str1 == NULL) {
		*str1[0] = c;
		*str1[1] = '\0';
	} else {
		(void) sprintf(*str1, "%s%c", *str1, c);
	}
}

/*
 * Append character 2 to string 1 allocating memory as needed.
 *
 * Parameters:
 *				str1 - The variable that holds the resultant
 *					appended character
 *				c - The character to append.
 * Returns:
 *				None
 */

void
appendchar(char **str1, char c)
{
	char *res;

	if (*str1 == NULL) {
		res = (char *)malloc(sizeof (char) + 1);
		*str1 = strdup(res);
		*str1[0] = c;
		*str1[1] = '\0';
		if (res) free(res);
	} else {
		res = (char *)malloc(strlen(*str1) + sizeof (char) + 1);
		(void) sprintf(res, "%s%c", *str1, c);
		*str1 = strdup(res);
		if (res) free(res);
	}
}

/*
 * This takes all the entries from a pkgmap or similarly formatted file
 * and prepares an SQL SELECT command. The gobal structure **eptlist gets
 * filled as result of calling this function.
 *
 * Parameters:
 *			 extlist - pointer to pointer to a cfextra struct
 *					   that holds the
 *					   information from the pkgmap file.
 *
 * Returns:
 * 			0 for success
 *			-1 for failure
 */

int
get_SQL_entries(struct cfextra **extlist, char *prog)
{
	int val;
	genericdb_Error gdbe;
	genericdb_result gdbr;
	struct dstr d;

	/*
	 * This is a very compute intensive function. In order to speed
	 * it up we use one memory allocation call based on the number
	 * entries in the pkgmap and the length of the static SQL
	 * commands.
	 */

	init_dstr(&d);

	if ((val = construct_SQL(extlist, &d, prog)) == 1) {
		/*
		 * No entries to search for. This can happen when installing
		 * an empty package which is legal
		 */
		return (0);
	} else if (val == -1) {
		return (-1);
	}

	if ((gdbe = query_db(get_install_root(), d.pc, &gdbr, R_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	free_dstr(&d);

	if (prepare_db_struct(gdbr) != 0)
		return (-1);

	if (gdbr) free(gdbr);

	return (0);
}

/*
 * Initialize a cfent struct.
 *
 * Parameters:
 *			 db - pointer to a cfent struct.
 *
 * Returns:
 *			None
 */

void
init_dbstruct(struct cfent *db)
{
	db->volno = 0;
	db->path = (char *)malloc(PATH_MAX);
	db->ftype = BADFTYPE;
	(void) strcpy(db->pkg_class, BADCLASS);
	db->pkg_class_idx = -1;
	db->ainfo.local = NULL;
	db->ainfo.mode = BADMODE;
	(void) strcpy(db->ainfo.owner, BADOWNER);
	(void) strcpy(db->ainfo.group, BADGROUP);
	db->cinfo.size = db->cinfo.cksum = db->cinfo.modtime = BADCONT;
	db->ainfo.major = db->ainfo.minor = BADMAJOR;
	db->pinfo = NULL;
	db->npkgs = 0;
}

/*
 * This function points the passed in cfent pointer to a cfent pointer
 * from the array of cfent's based off the path passed in. The caller
 * must not free the memory associated with the returned cfent pointer.
 *
 * Parameters:
 *			 db - pointer to to a cfent struct. Points to the
 *				  matched struct from the array of structs.
 *			 path - File system path to search for.
 *			 lastentry - Last array index of matched entry.
 *
 * Returns:
 *			 0 for no entry
 *			 1 for a match
 *			 2 for an entry lexically larger than the path
 */

int
get_SQL_entry(struct cfent *db, char *path, int *lastentry)
{
	int i, n, res;

	if (get_db_entries() == 0)
		return (0);

	if (path == NULL) {
		return (0);
	}

	for (i = *lastentry, n = 0; i < get_db_entries(); i++) {
		if ((res = strcmp(eptlist[i]->path, path)) == 0) {
			*db = *eptlist[i];
			*lastentry = ++i;
			n = 1;
			break;
		} else if (res > 0) {
			n = 2;
			break;
		}
	}

	/* Reset lastentry to 0 if we have searched all entries in eptlist */
	if (i >= get_db_entries())
		*lastentry = get_db_entries();

	return (n);
}

/*
 * Sorts two cfextra structures pased on the path.
 *
 * Parameters: Pointers to two cfextra structures.
 *
 * Returns:
 *			An int that tells the caller which path is greater,
 *			equal to or less than.
 */

int
pmapentcmp(const void *p1, const void *p2)
{
	struct cfextra *ext1 = *((struct cfextra **)p1);
	struct cfextra *ext2 = *((struct cfextra **)p2);

	return (strcmp(ext1->cf_ent.path, ext2->cf_ent.path));
}

/*
 * This functions prepares an SQL string for removal from the SQL DB.
 *
 * Parameters:
 *			 db - pointer to to a cfent struct. The struct
 *				  to prepare the SQL strings from.
 *
 * Returns:
 *			 0 - Success
 *			-1 - Failure
 */

int
putSQL_delete(struct cfent *db, char *pkginst)
{
	char *path = NULL;
	struct dstr pkgs;
	char pkgstatus = PSTATUS_OK;
	char buf[PATH_MAX];

	init_dstr(&pkgs);

	if (sql_buf == NULL) {
		if ((sql_buf = (char **)calloc(get_db_entries() + ADDL_TBL_BUF,
				SQL_ENTRY_MAX)) == NULL) {
			return (-1);
		}
	}

	if (db->path == NULL)
		return (-1);

	if (db->ainfo.local && db->ainfo.local[0] != '~') {
		if (sprintf(buf, "%s=%s", db->path, db->ainfo.local) < 0)
			return (-1);
		path = buf;
	} else {
		path = db->path;
	}

	/*
	 * If db->npkgs = 1 then the DB entry is removed completely.
	 * There are no pkgs that share the object so it can be safely
	 * primed for deletion.
	 */

	if (db->npkgs == 1) {
		sql_buf[del_ctr] = (char *)malloc(SQL_ENTRY_MAX);
		snprintf(sql_buf[del_ctr++], SQL_ENTRY_MAX, "%s%s%s",
			SQL_DEL, path, SQL_TRL_MOD);
	} else {
		/*
		 * The record contains a path that is shared with another pkg
		 * so we update the current record with the new pkg list.
		 * Any other columns in the DB that happen to change
		 * (modtime mode...) will automatically get updated as well
		 */

		if (process_pinfo(db->pinfo, &pkgstatus, &pkgs, DELETE,
				pkginst))
			return (-1);

		sql_buf[del_ctr] = (char *)malloc(SQL_ENTRY_MAX);
		snprintf(sql_buf[del_ctr++], SQL_ENTRY_MAX, "%s%s%s %s%s%s",
				SQL_UPDT, pkgs.pc, SQL_APOST,
				SQL_WHERE_PATH, path, SQL_TRL_MOD);
	}

	free_dstr(&pkgs);

	return (0);
}

/*
 * This functions uses the SQL strings putSQL_delete() prepared and
 * queries the SQL DB with them.
 *
 * Parameters:
 *
 * Returns:
 *			 0 - Success
 *			!0 - Failure
 */

int
putSQL_delete_commit()
{
	genericdb_Error gdbe;
	genericdb_result gdbr;
	int i;
	struct dstr d;

	init_dstr(&d);

	for (i = 0; i < del_ctr; i++) {
		if (i == 0) {
			if (append_dstr(&d, TRANS_MOD))
				return (-1);
		}
		if (append_dstr(&d, sql_buf[i]))
			return (-1);
		if (sql_buf[i]) free(sql_buf[i]);
	}
	if (append_dstr(&d, TRANS_MOD_TRL))
		return (-1);

	if (eptlist) free(eptlist);
	if (sql_buf) {
		free(sql_buf);
		sql_buf = NULL;
		put_ctr = 0;
	}


	if ((gdbe = query_db(get_install_root(), d.pc, &gdbr, W_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	free_dstr(&d);

	return (0);
}

/*
 * This functions uses the SQL strings putSQL() prepared and
 * queries the SQL DB with them.
 *
 * Parameters:
 *
 * Returns:
 *			 0 - Success
 *			!0 - Failure
 */

int
putSQL_commit()
{
	genericdb_Error gdbe;
	genericdb_result gdbr;
	int i;
	struct dstr d;

	/* This can happen if an empty package is being installed */
	if (put_ctr == 0)
		return (0);

	init_dstr(&d);

	for (i = 0; i < put_ctr; i++) {
		if (i == 0 && put_ctr > 1) {
			if (append_dstr(&d, TRANS_INS))
				return (-1);
		}
		if (append_dstr(&d, sql_buf[i]))
			return (-1);
		if (sql_buf[i]) free(sql_buf[i]);
	}
	if (put_ctr > 1) {
		if (append_dstr(&d, TRANS_INS_TRL))
			return (-1);
	}

	if (sql_buf) {
		free(sql_buf);
		sql_buf = NULL;
		put_ctr = 0;
	}

	if ((gdbe = query_db(get_install_root(), d.pc, &gdbr, W_MODE))
			!= genericdb_OK)
		return ((int)gdbe);

	free_dstr(&d);

	return (0);
}

/*
 * This functions determines how to construct a SQL string that holds
 * patch information destined for the patch_table.
 *
 * Parameters:
 *			 ir - installation root
 *			 patch - patch to insert
 *			 rev - revision of the patch
 *			 bcode - base code of the patch
 *			 sep - base code and revision seperator '-'
 *			 obs - list of obsolete patches
 *			 req - list of required patches
 *			 inc - list of incompatible patches
 *			 bd - backout directory
 *			 ts - timestamp
 *			 pkgs - list of packages
 *			 obsf - obsolete flag
 *			 reqf - required flag
 *			 incf - incompatible flag
 *			 gdb - genericdb pointer
 * Returns:
 *			 0 - Success
 *			!0 - Failure
 */

int
put_patch_info_db(char *patch, char *rev, char *bcode,
		char *sep, char *obs, char *req, char *inc, char *bd,
		char *ts, char *pkgs, int obsf, int reqf, int incf,
		genericdb *gdb)
{
	int boflag = 0;
	char sql_str[SQL_ENTRY_MAX];
	genericdb_result gdbr;

	if (strcmp(bd, "null") != 0)
		boflag++;

	if (obsf && reqf && incf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, reqs, incs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s' " \
				" '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || req == NULL ||
				inc == NULL || bd == NULL || ts == NULL ||
				pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, req, inc, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, reqs, incs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || req == NULL ||
				inc == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, req, inc, ts, pkgs);
		}
	} else if (obsf && reqf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, reqs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || req == NULL ||
				bd == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, req, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, reqs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || req == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, req, ts, pkgs);
		}
	} else if (obsf && incf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, incs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || inc == NULL ||
				bd == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, inc, bd, ts, pkgs);
	} else {
		char *sql = "INSERT OR REPLACE INTO patch_table(" \
			"patch, rev, bcode, sep, obs, incs, " \
			"time, pkgs) VALUES ('%s', '%s', " \
			"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || inc == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, inc, ts, pkgs);
		}
	} else if (reqf && incf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, reqs, incs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || req == NULL || inc == NULL ||
				bd == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, req, inc, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, reqs, incs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || req == NULL || inc == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, req, inc, ts, pkgs);
		}
	} else if (obsf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || bd == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || ts == NULL ||
				pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, ts, pkgs);
		}
	} else if (reqf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, reqs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || req == NULL || bd == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, req, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, reqs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || req == NULL || ts == NULL ||
				pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, req, ts, pkgs);
		}
	} else if (incf) {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, incs, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || inc == NULL || bd == NULL ||
				ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, inc, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, obs, reqs, incs, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || obs == NULL || req == NULL ||
				inc == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, obs, req, inc, ts, pkgs);
		}
	} else {
		if (boflag) {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, " \
				"backout, time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || bd == NULL || ts == NULL ||
				pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, bd, ts, pkgs);
		} else {
			char *sql = "INSERT OR REPLACE INTO patch_table(" \
				"patch, rev, bcode, sep, " \
				"time, pkgs) VALUES ('%s', '%s', " \
				"'%s', '%s', '%s', '%s');";
			if (patch == NULL || rev == NULL || bcode == NULL ||
				sep == NULL || ts == NULL || pkgs == NULL) {
				return (-1);
			}
			(void) snprintf(sql_str, SQL_ENTRY_MAX, sql, patch,
				rev, bcode, sep, ts, pkgs);
		}
	}

	if (genericdb_querySQL(gdb, sql_str, &gdbr) != genericdb_OK)
		return (-1);

	return (0);
}

/*
 * This function prepares SQL strings in memory that are later inserted
 * into the SQL DB by putSQL_commit().
 *
 * Parameters:
 *			 db - cfent struct to seed the SQL strings
 *			 firsttimethru - flag
 * Returns:
 *			 0 - Success
 *			-1 - Failure
 */

int
putSQL(struct cfent *ept, int firsttimethru, char *pkginst)
{
	struct dstr pkgs;
	char pkgstatus = PSTATUS_OK;
	char *path = NULL;

	init_dstr(&pkgs);

	if (sql_buf == NULL) {
		if ((sql_buf = (char **)calloc(eptnum + ADDL_TBL_BUF,
				sizeof (char *))) == NULL) {
			return (-1);
		}
	}

	if (ept->ftype == 'i')
		return (0); /* no ifiles stored in DB */

	if (ept->path == NULL)
		return (-1);

	path = ept->path;

	if (process_pinfo(ept->pinfo, &pkgstatus, &pkgs, UPDATE, pkginst))
		return (-1);

	if (firsttimethru) {
		if (mem_db_put(ept, path, pkgstatus, pkgs.pc))
			return (-1);
	} else {
		if (mem_db_replace(ept, path, pkgstatus, pkgs.pc) == -1)
			return (-1);
	}

	free_dstr(&pkgs);

	return (0);
}

/*
 * This function searches the array of SQL strings for a path.
 *
 * Parameters:
 *			 path - patch to search for
 * Returns:
 *			 1 - Found
 *			 2  Not found
 */

int
mem_db_find(char *path)
{
	static int lastfnd;
	int i, res;
	int found = 0;
	char *p = NULL, *p2;

	if (path == NULL)
		return (-1);

	for (i = lastfnd; sql_buf[i] != NULL; i++) {
		p = strdup(sql_buf[i]);

		if ((p2 = strchr(p, '/')) == NULL) {
			if (p) free(p);
			continue;
		}

		if ((res = strncmp(p2, path, strlen(path))) == 0) {
			found = 1;
			lastfnd = ++i;
			break;
		} else if (res > 0) {
			found = 2;
			break;
		}
	}

	if (p) free(p);

	if (sql_buf[i] == NULL)
		lastfnd = 0;

	return (found);
}

/*
 * Open, query and close the SQL DB.
 *
 * Parameters:
 *			 ir - Installation root
 *			 sql_str - SQL string to query the DB
 *			 gdbr - Genericdb result to store the result in
 *			 mode - The mode in which to open the SQL DB with.
 * Returns:
 *			 0 - Success
 *			!0 - Failure
 */

genericdb_Error
query_db(char *ir, char *sql_str, genericdb_result *gdbr, int mode)
{
	genericdb_Error gdbe;
	genericdb *pdb = NULL;

#ifndef NDEBUG
	char *log_lvl[] = { "-l", "1", NULL };
	int logargs = 2;
#else
	char *log_lvl[] = { NULL };
	int logargs = 0;
#endif

	pdb = genericdb_open(ir, mode, logargs, log_lvl, &gdbe);
	if (pdb != NULL && gdbe == genericdb_OK) {
		if ((gdbe = genericdb_querySQL(pdb, sql_str, gdbr))
				!= genericdb_OK) {
			progerr(gettext(genericdb_errstr(gdbe)));
		}
		genericdb_close(pdb);
	}

	return ((int)gdbe);
}

/*
 * This returns the number of rows returned by a SQL read query
 *
 * Parameters:
 *			 None
 * Returns:
 *			 Number of rows return by the latest DB query.
 */

int
get_db_entries()
{
	return (entries);
}

/*
 * This removes any installation root from a path.
 * The caller must free the returned string.
 *
 * Parameters:
 *			 path - The path to parse
 * Returns:
 *			 The naked path
 *			 NULL
 */

char *
strip_ir(char *path)
{
	int irlen = 0;

	if (path == NULL) {
		return (NULL);
	} else if (irlen = get_installroot_len(get_install_root())) {
		return (strdup(path + irlen));
	} else {
		return (strdup(path));
	}
}


/*
 * Name:		get_pkg_db_rowcount
 * Description: Counts number of rows a given
 *	  DB query would return.
 *
 * Arguments:   pkginst - Name of package whose contents entries
 *	  should be counted, or NULL for all packages.
 *
 *		  prog - Name of calling program, if non-null,
 *	  only returns entries whose pkgstatus=1
 *
 *	  gdb - Handle of open genericdb, or NULL
 *	  to open the default DB.
 *
 * Returns :	count of # of rows that would be returned
 *	  or -1 on error.
 */
long
get_pkg_db_rowcount(char *pkginst, char *prog, genericdb *gdb)
{
	char sql_str[SQL_ENTRY_MAX];
	genericdb_Error gdbe;
	genericdb_result gdbr;
	char *resp;
	long res;

	/* We get all paths in the DB */
	if (pkginst == NULL) {
		snprintf(sql_str, SQL_ENTRY_MAX, "%s", SQL_SELECT_ALL_COUNT);
	/* We get all paths associated with a pkgremove */
	} else if (prog == NULL) {
		snprintf(sql_str, SQL_ENTRY_MAX, "%s \'%c%s%c\'",
			SQL_SELECT_PKGS_COUNT, '%', pkginst, '%');
	/* We get all paths associated with installf and removef */
	} else {
		snprintf(sql_str, SQL_ENTRY_MAX, "%s %s \'%c%s%c%s",
			SQL_STATUS_COUNT, SQL_STATUS_PKG, '%', pkginst, '%',
			SQL_TRL_MOD);
	}

	if (gdb == NULL) {
		if ((gdbe = query_db(get_install_root(), sql_str,
				&gdbr, R_MODE)) != genericdb_OK)
			return ((int)gdbe);
	} else {
		gdbe = genericdb_querySQL(gdb, sql_str, &gdbr);
	}

	if (genericdb_result_table_str(gdbr, 0, 0, &resp) != genericdb_OK) {
		return (-1);
	}

	errno = 0;
	if (((res = atol(resp)) == 0) && (errno != 0)) {
		/* could not convert count to an int */
		return (-1);
	}

	return (res);
}

/*
 * Append the string passed in to the array of strings used to accumulate
 * the SQL statements.
 *
 * Parameters:
 *		   db - pointer to a dstr struct
 * Returns:
 *		   0 for success
 *		  -1 for failure
 */

int
putSQL_str_del(struct dstr *str)
{
	/*
	 * The pointer to str can be valid while pc is NULL. Need to also
	 * check that str->pc is not NULL before strdup'ing later.
	 */
	if (str == NULL || str->pc == NULL) {
		return (-1);
	}

	if (sql_buf == NULL) {
		if ((sql_buf = (char **)calloc(ADDL_TBL_BUF,
				SQL_ENTRY_MAX)) == NULL) {
			return (-1);
		}
		del_ctr = 0;
	}

	if (del_ctr >= SQL_ENTRY_MAX) {
		return (-1);
	}

	sql_buf[del_ctr++] = strdup(str->pc);

	return (0);
}

/*
 * Append the string passed in to the array of strings used to accumulate
 * the SQL statements.
 *
 * Parameters:
 *		   db - pointer to a dstr struct
 * Returns:
 *		   0 for success
 *		  -1 for failure
 */

int
putSQL_str(struct dstr *str)
{
	/*
	 * The pointer to str can be valid while pc is NULL. Need to also
	 * check that str->pc is not NULL before strdup'ing later.
	 */
	if (str == NULL || str->pc == NULL) {
		return (-1);
	}

	if (sql_buf == NULL) {
		if ((sql_buf = (char **)calloc(ADDL_TBL_BUF,
				SQL_ENTRY_MAX)) == NULL) {
			return (-1);
		}
		put_ctr = 0;
	}

	if (put_ctr >= SQL_ENTRY_MAX) {
		return (-1);
	}

	sql_buf[put_ctr++] = strdup(str->pc);

	return (0);
}

/*
 * Free a cfent struct.
 *
 * Parameters:
 *		   db - pointer to a cfent struct
 * Returns:
 *		   None
 */

void
free_db_struct(struct cfent *db)
{
	if (db->path) free(db->path);
	if (db) free((struct cfent *)db);
}


/* ~~~~~~~~~~~~~~~~~~~~~ Private Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * This function frees memory associated with eptlist and allocates
 * memory the size of "size"
 */

static int
get_mem_ept(int size)
{
	int i;

	if (size == 0) {
		return (0);
	}

	/*
	 * Need to reset the counter that keeps track of the number of entries
	 * retrieved from the SQL DB. Each time this function is entered
	 * eptlist is being reconstructed from a new SQL query.
	 */

	get_ctr = 0;

	if (eptlist != NULL) {
		for (i = 0; eptlist[i]; i++) {
			free(eptlist[i]);
			eptlist[i] = NULL;
		}
		free(eptlist);
		eptlist = NULL;
	}
	if ((eptlist = (struct cfent **)
			calloc(size + 100, sizeof (struct cfent *))) == NULL) {
		return (-1);
	}
	return (0);
}

/*
 * This function ouptputs patch information to stdout.
 * col[0] = patchid
 * col[1] = obsolete
 * col[2] = requires
 * col[3] = incompats
 * col[4] = packages
 */
static int
output_patch_row(char **col)
{
	if (col[1] == NULL && col[2] == NULL && col[3] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: Requires: " \
			"Incompatibles: Packages: %s"), col[0], col[4]);
	} else if (col[1] == NULL && col[2] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: Requires: " \
			"Incompatibles: %s Packages: %s"),
			col[0], col[3], col[4]);
	} else if (col[1] == NULL && col[3] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: Requires: %s " \
			"Incompatibles: Packages: %s"),
			col[0], col[2], col[4]);
	} else if (col[2] == NULL && col[3] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: %s Requires: " \
			"Incompatibles: Packages: %s"),
			col[0], col[1], col[4]);
	} else if (col[1] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: Requires: %s " \
			"Incompatibles: %s Packages: %s"),
			col[0], col[2], col[3], col[4]);
	} else if (col[2] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: %s Requires: " \
			"Incompatibles: %s Packages: %s"),
			col[0], col[1], col[3], col[4]);
	} else if (col[3] == NULL) {
		echo_out(0, gettext("Patch: %s Obsoletes: %s Requires: %s " \
			"Incompatibles: Packages: %s"),
			col[0], col[1], col[2], col[4]);
	} else  {
		echo_out(0, gettext("Patch: %s Obsoletes: %s Requires: %s " \
			"Incompatibles: %s Packages: %s"),
			col[0], col[1], col[2], col[3], col[4]);
	}

	return (0);
}

static void
output_backout_dir(char **col)
{
	if (col[0])
		fprintf(stdout, "%s", col[0]);
}

static int
output_patch_pkg(int ncols, char **col, int last, struct dstr *dstr)
{

	if (ncols && !last) {
		if (append_dstr(dstr, col[0]) != 0 ||
				append_dstr(dstr, " ") != 0)
			return (-1);
	}

	return (0);
}

static int
prepare_db_struct(genericdb_result gdbr)
{
	int i, j, k;
	char **row;
	int nrows = 0;
	int ncols = 0;

	if ((row = (char **)calloc(genericdb_result_table_ncols(gdbr),
			sizeof (char *))) == NULL) {
		return (-1);
	}

	nrows = genericdb_result_table_nrows(gdbr);
	ncols = genericdb_result_table_ncols(gdbr);

	set_db_entries(nrows);

	(void) get_mem_ept(get_db_entries());

	for (i = 0; i < nrows; i++) {
		char *pc = NULL;
		for (j = 0; j < ncols; j++) {
			if (genericdb_result_table_str(gdbr, i, j, &pc)
					== genericdb_OK)
				row[j] = pc;
		}
		if (fill_db_struct(j, row)) {
			return (-1);
		}
		for (k = 0; k < ncols; k++) {
			row[k] = NULL;
		}
	}

	for (k = 0; k < ncols; k++) {
		if (row[k]) free(row[k]);
	}

	if (row) free(row);

	qsort((char *)eptlist, nrows, sizeof (struct cfent *), pmapentcmp);

	return (0);
}

/*
 * Prepare an sql string for for later insertion into the DB.
 *
 * Return:
 *    1 if the entry is 'informational' and can be ignored.
 *    0 if the entry is not informational, and  goes into the db.
 *   -1 if the SQL construction fails due to memory allocation problems.
 *
 * Side effects: none except that additional string data is written to the
 *	             dynamic string sql_str.
 */
static int
construct_SQL(struct cfextra **extlist, struct dstr *sql_str, char *prog)
{
	struct cfent db;
	char path[256];
	int i;
	int onedbentry = 0;

	for (i = 0; extlist[i]; i++) {
		db = extlist[i]->cf_ent;
		if (db.ftype == 'i') {
			continue;
		}
		onedbentry++;
		strcpy(path, db.path);

		if (strcmp(prog, "removef") == 0) {
			if (append_dstr(sql_str, SQL_SELECT) ||
				append_dstr(sql_str, path) ||
				append_dstr(sql_str, SQL_APOST) ||
				append_dstr(sql_str, SQL_WHERE_PATH_LIKE) ||
				append_dstr(sql_str, path) ||
				append_dstr(sql_str, SQL_EQ_WILD) ||
				append_dstr(sql_str, SQL_TRL_MOD)) {
				return (-1);
			}
		} else {
			if (append_dstr(sql_str, SQL_SELECT) ||
				append_dstr(sql_str, path) ||
				append_dstr(sql_str, SQL_TRL_MOD)) {
				return (-1);
			}
		}
	}

	if (onedbentry == 0)
		return (1);
	else
		return (0);
}

/*
 * Fill the internal array of structures from the DB entries.
 */
static int
fill_db_struct(int argc, char **argv)
{
	struct pinfo *pinfo, *lastpinfo;
	struct cfent *db;
	char *buf = NULL;
	char *pkgname = NULL;
	char pkgstatus;
	char *tmpclass = NULL;
	int i = 0;

	if (argc == 0)
		return (-1);

	if ((db = (struct cfent *)calloc(1, (unsigned)sizeof (struct cfent)))
			== NULL) {
		return (-1);
	}

	(void) init_dbstruct(db);

	pkgstatus = PSTATUS_OK;

	/*
	 * Need to allocate more memory for db->path and db->ainfo.local
	 * then libgendb provides due to later prepending of BASEDIR
	 * and/or PKG_INSTALL_ROOT.
	 */
	if ((db->path = malloc(PATH_MAX)) == NULL) {
		return (-1);
	}
	if (strlcpy(db->path, argv[i], PATH_MAX) >= PATH_MAX) {
		return (-1);
	} else {
		free(argv[i++]);
	}

	if ((buf = strchr(db->path, '=')) != NULL) {
		db->ainfo.local = malloc(PATH_MAX);
		if (strlcpy(db->ainfo.local, buf + 1, PATH_MAX) >= PATH_MAX) {
			return (-1);
		} else {
			db->path[buf - db->path] = '\0';
		}
	}

	if ((db->ftype = argv[i++][0]) == '\0') {
		return (-1);
	}
	if (strlcpy(db->pkg_class, argv[i], CLSSIZ+1) >= CLSSIZ+1) {
		return (-1);
	} else {
		free(argv[i++]);
	}

	errno = 0;
	if (((db->ainfo.mode = strtol(argv[i++], NULL, 10)) == 0) &&
			(errno != 0)) {
		return (-1);
	}

	if (strlcpy(db->ainfo.owner, argv[i], ATRSIZ+1) >= ATRSIZ+1) {
		return (-1);
	} else {
		free(argv[i++]);
	}
	if (strlcpy(db->ainfo.group, argv[i], ATRSIZ+1) >= ATRSIZ+1) {
		return (-1);
	} else {
		free(argv[i++]);
	}
	if (strcmp(argv[i], "0") == 0) {
		db->ainfo.major = 0;
	} else if (((db->ainfo.major = atol(argv[i])) == 0) &&
			(errno != 0)) {
		return (-1);
	}
	i++;
	if (strcmp(argv[i], "0") == 0) {
		db->ainfo.minor = 0;
	} else if (((db->ainfo.minor = atol(argv[i])) == 0) &&
			(errno != 0)) {
		return (-1);
	}
	i++;
	if (strcmp(argv[i], "0") == 0) {
		db->cinfo.size = 0;
	} else if (((db->cinfo.size = atol((char *)argv[i])) == 0) &&
			(errno != 0)) {
		return (-1);
	}
	i++;
	if (strcmp(argv[i], "0") == 0) {
		db->cinfo.cksum = 0;
	} else if (((db->cinfo.cksum = atol((char *)argv[i])) == 0) &&
			(errno != 0)) {
		return (-1);
	}
	i++;
	if (((db->cinfo.modtime = atol(argv[i++])) == 0) &&
			(errno != 0)) {
		return (-1);
	}

	pkgstatus = argv[i++][0];

	/* determine list of packages which reference this entry */

	if ((pkgname = strtok(argv[i++], DB_PKG_SEPARATOR)) == NULL) {
		return (-1);
	}

	lastpinfo = (struct pinfo *)0;
	do {
		/* a package was listed */
		if ((pinfo = (struct pinfo *)
				calloc(1, sizeof (struct pinfo))) == NULL)
			return (-1);
		if (!lastpinfo) {
			db->pinfo = pinfo; /* first one */
		} else {
			lastpinfo->next = pinfo; /* link list */
		}
		lastpinfo = pinfo;

		tmpclass = strchr((char *)pkgname, ':');

		if (pkgstatus == PSTATUS_NOT_OK) {
			if (strchr("-+*~!%", pkgname[0])) {
				pinfo->status = pkgname[0];
				(void) strcpy(pinfo->pkg, pkgname+1);
			} else {
				(void) strcpy(pinfo->pkg, pkgname);
			}
		} else {
			(void) strcpy(pinfo->pkg, pkgname);
		}

		/* pkg\[:ftype][:class]] */
		if (strchr(pkgname, '\\') != NULL) {
			/* get alternate ftype */
			pinfo->editflag++;
		}

		if (tmpclass != NULL) {
			if (strlcpy(db->pkg_class, tmpclass + 1, CLSSIZ)
					>= CLSSIZ + 1) {
				return (-1);
			}
			/* strip off the alternate class from pinfo->pkg */
			pinfo->pkg[strlen(pinfo->pkg) - strlen(tmpclass)]
					= '\0';
		}
		db->npkgs++;
	} while ((pkgname = strtok(NULL, DB_PKG_SEPARATOR)) != NULL);

	eptlist[get_ctr++] = db;

	return (0);
}

static int
mem_db_put(struct cfent *db, char *path, char pkgstatus, char *pkgs)
{
	char buf[PATH_MAX];
	char buf2[SQL_ENTRY_MAX];

	char *sql_str = "INSERT or REPLACE INTO pkg_table VALUES " \
		"('%s', '%c', '%s', '%ld', '%s', '%s', '%ld', '%ld', " \
		"'%ld', '%ld', '%ld', '%c', '%s');\n";

	if (db->ainfo.local && db->ainfo.local[0] != '~') {
		/*
		 * If path has now changed to a link then
		 * the original path must be removed.
		 * Add the DELETE SQL entry to the
		 * end of the array.
		 */
		(void) snprintf(buf2, sizeof (buf2), "%s%s%s",
			SQL_DEL, path, SQL_TRL_MOD);

		sql_buf[put_ctr++] = strdup(buf2);

		if (snprintf(buf, sizeof (buf), "%s=%s", db->path,
			db->ainfo.local) < 0) {
				return (-1);
		}
		(void) snprintf(buf2, sizeof (buf2), sql_str, buf, db->ftype,
			db->pkg_class, db->ainfo.mode, db->ainfo.owner,
			db->ainfo.group, db->ainfo.major, db->ainfo.minor,
			db->cinfo.size, db->cinfo.cksum, db->cinfo.modtime,
			pkgstatus, pkgs);
	} else {
		(void) snprintf(buf2, sizeof (buf2), sql_str, path, db->ftype,
			db->pkg_class, db->ainfo.mode, db->ainfo.owner,
			db->ainfo.group, db->ainfo.major, db->ainfo.minor,
			db->cinfo.size, db->cinfo.cksum, db->cinfo.modtime,
			pkgstatus, pkgs);
	}

	sql_buf[put_ctr++] = strdup(buf2);

	return (0);
}

static int
mem_db_replace(struct cfent *db, char *path, char pkgstatus, char *pkgs)
{
	static int last;
	int i, res;
	int found = 0;
	char *p, *p2;
	char buf[PATH_MAX];
	char buf2[SQL_ENTRY_MAX];

	char *sql_str = "INSERT or REPLACE INTO pkg_table VALUES " \
		"('%s', '%c', '%s', '%ld', '%s', '%s', '%ld', '%ld', " \
		"'%ld', '%ld', '%ld', '%c', '%s');\n";

	if (path == NULL)
		return (-1);

	for (i = last; sql_buf[i] != NULL; i++) {
		/* Skip over any previously deleted objects */
		if (strncmp(sql_buf[i], "DELETE FROM", 11) == 0) {
			continue;
		}

		p = strdup(sql_buf[i]);

		if ((p2 = strchr(p, '/')) == NULL) {
			if (p) free(p);
			continue;
		}

		if ((res = strncmp(p2, path, strlen(path))) == 0) {
			if (db->ainfo.local &&
					db->ainfo.local[0] != '~') {
				if (snprintf(buf, sizeof (buf),
					"%s=%s", db->path,
					db->ainfo.local) < 0) {
						return (-1);
				}
				/*
				 * If path has now changed to a link then
				 * the original path must be removed.
				 * Add the DELETE SQL entry to the
				 * end of the array.
				 */
				(void) snprintf(buf2,
					sizeof (buf2), "%s%s%s",
					SQL_DEL, path, SQL_TRL_MOD);

				sql_buf[put_ctr++] = strdup(buf2);

				/* Insert new path with new attributes */
				(void) snprintf(sql_buf[i], SQL_ENTRY_MAX,
					sql_str, buf, db->ftype, db->pkg_class,
					db->ainfo.mode, db->ainfo.owner,
					db->ainfo.group, db->ainfo.major,
					db->ainfo.minor, db->cinfo.size,
					db->cinfo.cksum, db->cinfo.modtime,
					pkgstatus, pkgs);
			} else {
				/* Insert existing path with new attributes */
				(void) snprintf(sql_buf[i], SQL_ENTRY_MAX,
					sql_str, path, db->ftype, db->pkg_class,
					db->ainfo.mode, db->ainfo.owner,
					db->ainfo.group, db->ainfo.major,
					db->ainfo.minor, db->cinfo.size,
					db->cinfo.cksum, db->cinfo.modtime,
					pkgstatus, pkgs);
			}

			found = 1;
			last = ++i;
			break;
		} else if (res > 0) {
			found = 2;
			break;
		}
		if (p) free(p);
	}

	if (sql_buf[i] == NULL)
		last = 0;

	if (!found) {
		sql_buf[put_ctr] = (char *)malloc(SQL_ENTRY_MAX);
		snprintf(sql_buf[put_ctr], SQL_ENTRY_MAX, sql_str, path,
			db->ftype, db->pkg_class, db->ainfo.mode,
			db->ainfo.owner, db->ainfo.group,
			db->ainfo.major, db->ainfo.minor,
			db->cinfo.size, db->cinfo.cksum,
			db->cinfo.modtime, pkgstatus, pkgs);
	}

	return (found);
}

static void
set_db_entries(int rows)
{
	entries = rows;
}

static int
get_installroot_len(char *ir)
{
	if (ir == NULL)
		return (0);
	else
		return (strlen(ir));
}

static int
process_pinfo(struct pinfo *pinfo, char *pkgstatus, struct dstr *pkgs,
		int delete, char *pkginst)
{
	int pctr = 0;
	char tmpstatus[2];

	*pkgstatus = PSTATUS_OK;

	while (pinfo) {
		if (delete) {
			if (strncmp(pinfo->pkg, pkginst, strlen(pkginst))
					== 0) {
				if (pinfo->next) {
					pinfo = pinfo->next;
				} else {
					break;
				}
			}
		}
		pctr++;

		if (pinfo->status != PINFO_STATUS_OK) {
			*pkgstatus = PSTATUS_NOT_OK;
			tmpstatus[0] = pinfo->status; tmpstatus[1] = '\0';
			if (pctr == 1) {
				if (append_dstr(pkgs, tmpstatus)) {
					return (-1);
				}
			} else {
				if (append_dstr(pkgs, DB_PKG_SEPARATOR) ||
						append_dstr(pkgs, tmpstatus)) {
					return (-1);
				}
			}
		}

		if (pctr == 1 || pinfo->status != PINFO_STATUS_OK) {
			if (append_dstr(pkgs, pinfo->pkg)) {
				return (-1);
			}
		} else {
			if (append_dstr(pkgs, DB_PKG_SEPARATOR) ||
				append_dstr(pkgs, pinfo->pkg)) {
				return (-1);
			}
		}

		if (pinfo->editflag) {
			pinfo->editflag = '\\';
		}

		if (pinfo->aclass[0]) {
			/*
			 * If identical aclass already exists then don't
			 * append it
			 */
			if (strstr(pkgs->pc, pinfo->aclass) == NULL) {
				if (append_dstr(pkgs, DB_CLASS_SEPARATOR) ||
					append_dstr(pkgs, pinfo->aclass))
					return (-1);
			}
		}
		pinfo = pinfo->next;
	}

	return (0);
}

/*
 * init_dstr
 *
 * Clean up a dynamic string buffer.
 *
 * Return: Nothing
 * Side effects: erases data in the dstr.
 */
void
init_dstr(struct dstr *pd)
{
	if (pd)
		memset(pd, 0, sizeof (struct dstr));
}

/*
 * free_dstr
 *
 * Free the dynamic string in a dstr struct.
 *
 *   pd		The string to free.
 *
 * Return: nothing
 * Side effects: frees the data in the dstr
 */
void
free_dstr(struct dstr *pd)
{
	if (pd && pd->pc)
		free(pd->pc);
	pd->pc = NULL;
}

/*
 * append_dstr
 *
 *   pd  A pointer to a struct dstr allocated by the caller.
 *   str	A pointer to a string to append.
 *
 * Returns: An allocated string in pd.  Can be freed by the user or
 *		with free_dstr.  0 return code = success, -1 = malloc failed.
 * Side effects: May result in pd->pc being reallocated.
 */
int
append_dstr(struct dstr *pd, const char *str)
{
	int len = 0;

	if (str == NULL)
		return (0);

	len = strlen(str);

	if (pd->max == 0) {

		/* Initialize if necessary */
		pd->len = 0;
		pd->max = APPEND_INCR + len;
		pd->pc = (char *)calloc(pd->max, 1);
		if (pd->pc == NULL)
			return (-1);

	} else if ((pd->len + len + 2) >= pd->max) {

		/*
		 * Grow the array.
		 * Always leave room for a single NULL end item:  That is
		 * why we grow when +2 equals the max, not +1.
		 */
		size_t s = (pd->max + APPEND_INCR) + len;
		char *pcTemp = realloc(pd->pc, s);
		if (pcTemp == NULL)
			return (-1);

		(void) memset(&(pcTemp[pd->max]), 0, APPEND_INCR + len);
		pd->pc = pcTemp;
		pd->max += (APPEND_INCR + len);
	}

	(void) memcpy(&(pd->pc[pd->len]), str, len);
	pd->len += len;

	return (0);
}

/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from a pkginfo file to a database.
 * ----------------------------------------------------------------------
 */

/*
 * add_to_pkginfo_table
 *
 * Add the pkginfo param for the specific package to the database by
 * creating an appropriate INSERT command and appending it to the dyanamic
 * string.
 *
 *  pd		The dynamic string.
 *  pkg		The package to add a param row for in the pkginfo_table.
 *  param	The param string for the parameter.
 *  value	The value string for the parameter.
 *  seqno	The ordered step in which this parameter is being added for pkg
 *		which allows reconstruction of the pkginfo file in the same
 *		order it was read.
 *
 * Return: 0 for success, nonzero for failure.
 * Side effects: Will add a string (SQL) to the dynamic string.
 */
static int
add_to_pkginfo_table(struct dstr *pd, const char *pkg,
	const char *param, const char *value, int seqno)
{
	char *pcp = NULL, *pcv = NULL;
	char *pcSQL = "INSERT INTO pkginfo_table (pkg, param, value, seqno) "\
		"VALUES ('%s', '%s', '%s', '%d');\n";
	char pcsqlbuf[MAXSQLBUF];

	(void) memset(pcsqlbuf, 0, MAXSQLBUF);

	/* Quote defensively */
	if (strchr(param, APOSTRAPHE)) {
		pcp = quote(param);
		param = pcp;
	}

	if (strchr(value, APOSTRAPHE)) {
		pcv = quote(value);
		value = pcv;
	}

	(void) sprintf(pcsqlbuf, pcSQL, pkg, param, value, seqno);
	if (append_dstr(pd, pcsqlbuf))
		return (-1);

	if (pcp)
		free(pcp);
	if (pcv)
		free(pcv);

	return (0);
}

/*
 * convert_pkginfo_to_sql
 *
 * Take a pkginfo file (already open, just read from fp).  Read each line
 * to parse out params.  Generate SQL insert statements and append them to
 * the end of the dynamic string for the specified package pkg.
 *
 * Note - if this code looks different than the rest it is because it was
 * adapted from existing pkgparam code, specifically pkgparam.c currently
 * part of libadm.
 *
 *  pd		The dynamic string.
 *  fp		The pkginfo input file stream.
 *  pkg		The pkg whose pkginfo file is to be read
 *
 * Return: 0 on success, nonzero on failure.
 * Side Effects: Advances fp read index.  Appends SQL to pd.
 */
int
convert_pkginfo_to_sql(struct dstr *pd, FILE *fp, const char *pkg)
{
	int result = 0; /* Nothing is wrong yet. */
	int true = 1;
	char param[1024];
	char value[1024];
	char *copy, ch, *mempt = NULL;
	int c, n, escape, begline, quoted, seqno = 0;

	while (true) {
		(void) memset(param, 0, 1024);
		(void) memset(value, 0, 1024);

		copy = param;
		n = 0;
		while ((c = getc(fp)) != EOF) {
			ch = (char)c;
			if (strchr(sepset, ch))
				break;
			if (++n < VALSIZ)
				*copy++ = ch;
		}

		/* No more entries left. */
		if (c == EOF)
			break;

		if (c == NEWLINE)
			continue;

		/* copy now points to the end of a valid parameter. */
		*copy = '\0';	   /* Terminate the string. */
		if (param[0] == '#')   /* If it's a comment, drop thru. */
			copy = NULL;	/* Comments don't get saved in db! */
		else
			copy = value;

		n = quoted = escape = 0;
		begline = 1;

		/* Now read the parameter value. */
		while ((c = getc(fp)) != EOF) {
			ch = (char)c;
			if (begline && ((ch == ' ') || (ch == '\t')))
				continue; /* ignore leading white space */

			if (ch == NEWLINE) {
				if (!escape)
					break; /* end of entry */
				if (copy) {
					if (escape) {
						copy--; /* eat previous esc */
						n--;
					}
					*copy++ = NEWLINE;
				}
				escape = 0;
				begline = 1; /* new input line */
			} else {
				if (!escape && strchr(qset, ch)) {
					/* handle quotes */
					if (begline) {
						quoted++;
						begline = 0;
						continue;
					} else if (quoted) {
						quoted = 0;
						continue;
					}
				}
				if (ch == ESCAPE)
					escape++;
				else if (escape)
					escape = 0;
				if (copy) *copy++ = ch;
				begline = 0;
			}

			if (copy && ((++n % VALSIZ) == 0)) {
				if (mempt) {
					mempt = realloc(mempt,
						(n+VALSIZ)*sizeof (char));
					if (!mempt)
						return (0);
				} else {
					mempt = calloc((size_t)(2*VALSIZ),
						sizeof (char));
					if (!mempt)
						return (0);
					(void) strncpy(mempt, value, n);
				}
				copy = &mempt[n];
			}
		}

		/*
		 * Don't allow trailing white space.
		 * NOTE : White space in the middle is OK, since this may
		 * be a list. At some point it would be a good idea to let
		 * this function know how to validate such a list. -- JST
		 *
		 * Now while there's a parametric value and it ends in a
		 * space and the actual remaining string length is still
		 * greater than 0, back over the space.
		 */
		while (copy && isspace((unsigned char)*(copy - 1)) && n-- > 0)
			copy--;
		if (quoted) {
			if (mempt)
				(void) free(mempt);
			/* missing closing quote */
			progerr(gettext("Pkginfo file for %s corrupted - "
				"missing closing quote"), pkg);
			result = 1;
			goto to_sql_done;
		}

		if (copy) {
			*copy = '\0';
		}

		if (c == EOF) {
			progerr(gettext("Pkginfo file for %s corrupted - "
				"parameter not found"), pkg);
			result = 1;
			goto to_sql_done;
		}

		if (!mempt) {
			if (copy) {
				if (add_to_pkginfo_table(pd,
					pkg, param, value, seqno++)) {

					result = 1;
					goto to_sql_done;

				}
			}
		} else {
			if (copy) {
				if (add_to_pkginfo_table(pd,
					pkg, param, mempt, seqno++)) {

					result = 1;
					goto to_sql_done;

				}
			}

			free(mempt);
			mempt = NULL;
		}
	}

to_sql_done:
	return (result);
}

/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from a database to a pkginfo file.
 * ----------------------------------------------------------------------
 */

/*
 * convert_db_to_pkginfo
 *
 * This routine generates a new pkginfo file in the right subdirectory under
 * the specified root.
 *
 *  pcPkg	The package name of the package to generate a pkginfo file for.
 *  pdb		The session ahndel for the database where the pkginfo file
 *		has already been inserted.
 *  pcroot	The install directory root.
 *
 * Return:  0 success, nonzero failure.
 * Side effects: This will drive recreation of a pkginfo file from data
 *	stored in the database.  The old pkginfo file will be replaced, if
 *	one existed.
 */
int
convert_db_to_pkginfo(const char *pcPkg, genericdb *pdb, const char *pcroot)
{
	char *pcP = "SELECT param, value FROM pkginfo_table " \
		"WHERE pkg='%s' ORDER BY seqno;";
	char path[PATH_MAX];
	char pcS[2048];
	FILE *fp = NULL;
	genericdb_Error err;
	genericdb_result r2;
	int nrd, k;
	int result = 0; /* Be optimistic. */

	/* Take care of rebuilding the pkginfo file */

	(void) sprintf(pcS, pcP, pcPkg);

	if ((err = genericdb_querySQL(pdb, pcS, &r2)) != genericdb_OK) {
		progerr(gettext("Could not get params: %s"),
			genericdb_errstr(err));
		result = 1;
		goto to_pkginfo_done;
	}

	if ((nrd = genericdb_result_table_nrows(r2)) == 0) {
		(void) DEBUG("debug: no pkginfo rows for [%s] "
			"- continue\n", pcPkg);
		genericdb_result_free(&r2);
	}

	(void) sprintf(path, "%s" VSADMDIR "/pkg/%s/pkginfo", pcroot, pcPkg);

	if ((fp = fopen(path, "w")) == NULL) {
		DEBUG("fopen pkginfo to write: %s", strerror(errno));
		result = 1;
		goto to_pkginfo_done;
	}

	for (k = 0; k < nrd; k++) {
		char *pc1, *pc2;
		pc1 = pc2 = NULL;
		if (genericdb_result_table_str(r2, k,
			0, &pc1) != genericdb_OK ||
			genericdb_result_table_str(r2, k,
			1, &pc2) != genericdb_OK) {
			DEBUG("get parameter & value");
			result = 1;
			goto to_pkginfo_done;
		}

		(void) fprintf(fp, "%s=%s\n", (pc1)?(pc1):"",
			(pc2)?(pc2):"");
		if (pc1)
			free(pc1);
		if (pc2)
			free(pc2);
	}

to_pkginfo_done:
	genericdb_result_free(&r2);
	fclose(fp);
	return (result);

}


/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from depend files to a database.
 * ----------------------------------------------------------------------
 */

/*
 * add_to_depend_table
 *
 * This function takes a set of data which can be formulated as a set of
 * columns for a single row.  The correct INSERT statement is generated and
 * appended to the dynamic string.
 *
 *   pd		The dynamic string.
 *   pkg	The package whose depend file is being ingested.
 *   type	The type of dependency.
 *   pkgdep	The package depended upon
 *   arch	Architecture of platform specific dependency.
 *   vers	Version of the platform specific dependency.
 *   seqno	The number of the entry, in sequential order of insertion.
 *
 * Return: 0 success, nonzero failure.
 * Side effects: writes to the dynamic string.
 */
static int
add_to_depend_table(struct dstr *pd, const char *pkg,
	char type, const char *pkgdep, const char *name, const char *arch,
	const char *vers, int seqno)
{
	int result = 0;

	char *pcA = "INSERT INTO depend_table " \
		"(pkg, pkgdep, type, name, seqno) " \
		"VALUES ('%s', '%s', '%c', '%s', '%d');\n";
	char *pcB = "INSERT INTO depplatform_table " \
		"(pkg, pkgdep, arch, version, seqno) " \
		"VALUES ('%s', '%s', '%s', '%s', '%d');\n";
	char *pcn = NULL, *pcv = NULL, *pca = NULL;

	char pcsqlbuf[MAXSQLBUF];
	(void) memset(pcsqlbuf, 0, MAXSQLBUF);

	/* Quote defensively */
	if (name && strchr(name, APOSTRAPHE)) {
		pcn = quote(name);
		if (pcn == NULL) {
			result = 1;
			goto add_done;
		}
		name = pcn;
	}
	if (vers && strchr(vers, APOSTRAPHE)) {
		pcv = quote(vers);
		if (pcv == NULL) {
			result = 1;
			goto add_done;
		}
		vers = pcv;
	}
	if (arch && strchr(arch, APOSTRAPHE)) {
		pca = quote(arch);
		if (pca == NULL) {
			result = 1;
			goto add_done;
		}
		arch = pca;
	}

	if (arch && vers) {
		(void) sprintf(pcsqlbuf, pcB,
			pkg, pkgdep, arch, vers, seqno);
	} else {
		(void) sprintf(pcsqlbuf, pcA, pkg, pkgdep, type, name, seqno);
	}

	if (append_dstr(pd, pcsqlbuf))
		return (-1);

add_done:
	if (pcn)
		free(pcn);
	if (pcv)
		free(pcv);
	if (pca)
		free(pca);

	return (result);
}

/*
 * end_trim_and_quote
 *
 * Remove a trailing carriage return and call quote to ensure apostraphes are
 * made SQL-safe by escaping them.
 *
 *   pc	A string to make safe and trim the trailing CR off of.
 *
 * Return: NULL if pc is NULL or if strdup fails.  The caller must free the
 *	returned string.
 * Side effects, none.
 */
static char *
end_trim_and_quote(const char *pc)
{
	int i;
	char *pcT, *pcT2;
	if (pc == NULL)
		return (NULL);
	pcT = strdup(pc);
	if (pcT == NULL) {
		progerr(gettext("could not malloc: %s"), strerror(errno));
		return (NULL);
	}
	i = strlen(pc);
	/* If the string ends with a CR, trim that off. */
	if (pc[i - 1] == '\n')
		pcT[i - 1] = '\0';
	pcT2 = quote(pcT);
	free(pcT);

	if (pcT2 == NULL)
		progerr(gettext("could not malloc: %s"), strerror(errno));

	return (pcT2);
}

/*
 * convert_depend_to_sql
 *
 * Read and parse the depend file from the FILE stream.  Convert comments
 * to sequential entries in the depcomment_table.  Convert entries into
 * entries in the depend_table and depplatform_table as appropriate.
 *
 * One shortcoming to this routine is that it doesn't pay attention to whether
 * comments have white space between them, or are interspersed with the
 * depend line listings.  All this ordering and white spaceing is thrown away
 * and will not be reproduceable.
 *
 *	pd	The dynamic string to append SQL to.
 *	fp	The depend file stream, ready to read.
 *	pkg	The name of the package whose dependency file
 *
 * Return: 0 if OK, -1 if failed.
 * Side effects:  Reads through fp, generates strings and appends them to pd.
 */
int
convert_depend_to_sql(struct dstr *pd, FILE *fp, const char *pkg)
{
	int result = 0; /* Assume success. */
	char abbrev[128], name[128], type;
	register int c;
	int seqno = 0;
	char *pt, *new, line[LSIZE];
	char *arch = NULL;
	char *version = NULL;
	char buf[2048];

	abbrev[0] = name[0] = type = '\0';

	while ((c = getc(fp)) != EOF) {
		(void) ungetc(c, fp);

		if (!fgets(line, LSIZE, fp))
			break;

		for (pt = line; isspace(*pt); /* void */)
			pt++;


		/* Skip empty lines and comment lines. */
		if (!*pt)
			continue;

		if (*pt == '#') {
			char *pctrim = end_trim_and_quote(line);
			if (pctrim == NULL) {
				result = 1;
				goto depend_sql_done;
			}

			if (append_dstr(pd, "INSERT INTO depcomment_table " \
				"(pkg, comment, seqno) VALUES ("))
				return (-1);
			(void) sprintf(buf, "'%s', '%s', '%d');\n",
				pkg, pctrim, seqno);
			free(pctrim);
			if (append_dstr(pd, buf))
				return (-1);
			if (0 == strcmp(buf, "#"))
				DEBUG("%s depend cmt line in [%s]\n",
					pkg, buf);
			seqno++;
			continue;
		}

		/* Depend lines begin at the beginning of the line. */
		if (pt == line) {

			abbrev[0] = name[0] = type = '\0';

			(void) sscanf(line, "%c %s %[^\n]",
				&type, abbrev, name);

			if (add_to_depend_table(pd, pkg, type, abbrev, name,
				NULL, NULL, seqno++)) {
				result = 1;
				goto depend_sql_done;
			}
			continue;
		}

		/*
		 * (ARCH) VERSION lines are preceded by a space and
		 * use the '(',  ')' and '\n' delimeters.
		 * This case happens so seldom, it is not expensive
		 * to copy strings, etc. here.
		 */
		if (*pt == '(') {
			arch = NULL;
			/* architecture is specified */
			if (new = strchr(pt, ')'))
				*new++ = '\0';
			else {
				/* bad specification */
				result = 1;
				goto depend_sql_done;
			}
			arch = strdup(pt+1);
			pt = new;
		}

		while (isspace(*pt))
			pt++;
		if (*pt) {
			version = strdup(pt);
			if (pt = strchr(version, '\n'))
				*pt = '\0';

			/*
			 * Use the same type, abbrev, name from the
			 * preceding line.
			 */
			if (add_to_depend_table(pd, pkg, type, abbrev, name,
				arch, version, seqno++)) {
				result = 1;
				goto depend_sql_done;
			}
		}
		if (arch) {
			free(arch);
			arch = NULL;
		}
		if (version) {
			free(version);
			version = NULL;
		}
	}

depend_sql_done:

	if (arch) {
		free(arch);
		arch = NULL;
	}

	if (version) {
		free(version);
		version = NULL;
	}

	return (result);
}

/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from a database to depends files.
 * ----------------------------------------------------------------------
 */
/*
 * output_platform
 *
 * Some entries in the depend file have specific subentries describing
 * what platform they qualify for.  This information is stored in the
 * depplatform_table.  The information for the particular package and pkg
 * it depends on.  These are output in the order they were input.  The
 * formatting may differ from the way it was originally presented in hte
 * depend file since white space is not recorded in the depplatform_table.
 *
 *   pcPkg	The package whose dependency file is being regenerated.
 *   pcPkgdep	The package pcPkg depends on.
 *   pdb	The database session handle
 *   fp		The file being output (depend).
 *
 * Result: 0 success, nonzero failure
 * Side effects:  Besides writing to fp, none.
 */
static int
output_platform(const char *pcPkg, const char *pcPkgdep,
	genericdb *pdb, FILE *fp)
{
	int result = 0; /* Assume success. */
	int nrows = 0;
	char *pcArch = NULL, *pcVersion = NULL;
	genericdb_Error err;
	genericdb_result r;
	const char *pcPF = "SELECT arch, version " \
		"FROM depplatform_table " \
		"WHERE (pkg = '%s') AND (pkgdep = '%s') " \
		"ORDER BY seqno;";
	char buf[2048];
	int j;

	/* Create the query. */
	(void) sprintf(buf, pcPF, pcPkg, pcPkgdep);

	/* Find whether there are any distinct platforms for the dependency */
	if ((err = genericdb_querySQL(pdb, buf, &r)) != genericdb_OK) {
		progerr(gettext("Cannot query for distinct platforms in "
			"depplatform_table: %s"), genericdb_errstr(err));
		result = 1;
		goto platform_done;
	}

	/* For each platform dependency, decompose the result and output it. */
	nrows = genericdb_result_table_nrows(r);

	for (j = 0; j < nrows; j++) {

		pcArch = NULL;
		pcVersion = NULL;

		if (genericdb_result_table_str(r, j,
			0, &pcArch) != genericdb_OK ||
			genericdb_result_table_str(r, j,
			1, &pcVersion) != genericdb_OK) {
			progerr(gettext("Cannot get arch & version for "
				"depend file"));
			result = 1;
			goto platform_done;
		}

		(void) fprintf(fp, "  \t(%s) %s\n",
			(pcArch)?(pcArch):"", (pcVersion)?(pcVersion):"");
		if (pcArch) {
			free(pcArch);
			pcArch = NULL;
		}
		if (pcVersion) {
			free(pcVersion);
			pcVersion = NULL;
		}
	}

platform_done:

	if (pcArch)
		free(pcArch);
	if (pcVersion)
		free(pcVersion);

	genericdb_result_free(&r);
	return (result);
}

/*
 * convert_db_to_depend
 *
 * This routine outputs a depend file for the package requested by querying
 * the database tables which must previously have been populated for it.
 * One table 'depend_table' includes all dependency info.  depcomment_table
 * includes the comments, which are expected to all be 'at the top' of the
 * original depend file.  Most important is to reproduce these so that the
 * copyright info which might be in the depend file is still present in the
 * reproduced version.  Embedded comments, if there were any, would not be
 * reproduced.  That is, any comments which are not stored by
 * convert_depend_to_db.
 *
 * The 'seqno' column was populated with increasing numbers as the rows got
 * generated.  This allows the package's depend file to be reproduced.
 *
 *   pcPkg	The package whose depend file must be produced
 *   pdb	The database session handle
 *   pcroot	The alternate root
 *
 * Return: 0 success, nonzero failure.
 * Side effects: if successful, depend file will be created, potentially
 *	replacing the existing file.  Note that depend files are not removed
 *	when converted.  Note also that the regenerated depend file is not
 *	identical, since it uses its own formatting and depend files vary
 *	with respect to internal white space.
 */
int
convert_db_to_depend(const char *pcPkg, genericdb *pdb, const char *pcroot)
{
	char path[PATH_MAX];
	char pcSdep[2048];
	char pcSdepcomment[2048];
	FILE *fp = NULL;
	genericdb_Error err;
	genericdb_result r2, r3;
	int k, nrc, nrd;
	int result = 0; /* Assume the best. */

	char *pcDep = "SELECT type, pkgdep, name FROM depend_table " \
		"WHERE pkg='%s' ORDER BY seqno;";
	char *pcDepComment = "SELECT comment FROM depcomment_table " \
		"WHERE pkg='%s' ORDER BY seqno;";

	(void) memset(path, 0, PATH_MAX);
	(void) memset(pcSdep, 0, 2048);
	(void) memset(pcSdepcomment, 0, 2048);

	(void) sprintf(pcSdepcomment, pcDepComment, pcPkg);
	(void) sprintf(pcSdep, pcDep, pcPkg);


	if ((err = genericdb_querySQL(pdb, pcSdepcomment, &r3))
		!= genericdb_OK) {
		progerr(gettext("Cannot query for depend entry comment: %s"),
			genericdb_errstr(err));
		result = 1;
		goto convert_to_done;
	}

	if ((err = genericdb_querySQL(pdb, pcSdep, &r2)) != genericdb_OK) {
		progerr(gettext("Cannot query for depend entry"),
			genericdb_errstr(err));
		result = 1;
		goto convert_to_done;
	}

	if ((nrc = genericdb_result_table_nrows(r3)) == 0) {
		genericdb_result_free(&r3);
	}

	if ((nrd = genericdb_result_table_nrows(r2)) == 0) {
		genericdb_result_free(&r2);
	}

	if (nrc < 0 || nrd < 0) {
		progerr(gettext("query for depend information failed"));
		result = 1;
		goto convert_to_done;
	}

	/* If there is no depend data, do not write out a depend file. */
	if (nrd == 0 && nrc == 0) {
		goto convert_to_done;
	}

	(void) sprintf(path, "%s" VSADMDIR "/pkg/%s/install/depend",
		pcroot, pcPkg);

	if ((fp = fopen(path, "w")) == NULL) {
		progerr(gettext("fopen depend to write: %s"), strerror(errno));
		result = 1;
		goto convert_to_done;
	}

	for (k = 0; k < nrc; k++) {
		char *pcCmt;
		if (genericdb_result_table_str(r3, k, 0, &pcCmt)
			!= genericdb_OK) {
			progerr(gettext("get depcomment failed"));
			result = 1;
			goto convert_to_done;
		}

		(void) fprintf(fp, "%s\n", (pcCmt)?(pcCmt):"");

		if (pcCmt)
			free(pcCmt);
		pcCmt = NULL;
	}

	for (k = 0; k < nrd; k++) {
		char *pc1, *pc2, *pc3;
		pc1 = pc2 = pc3 = NULL;
		if (genericdb_result_table_str(r2, k, 0, &pc1)
			!= genericdb_OK ||
			genericdb_result_table_str(r2, k, 1, &pc2)
			!= genericdb_OK) {
			progerr(gettext("get parameter, value and name"));
			result = 1;
			goto convert_to_done;
		}

		/* The 'name' parameter in col 3 can be NULL. */
		err = genericdb_result_table_str(r2, k,
			2, &pc3);
		if (err != genericdb_NULL && err != genericdb_OK) {
			progerr(gettext("get parameter, value and name"));
			result = 1;
			goto convert_to_done;
		}

		(void) fprintf(fp, "%s %s\t\t%s\n", (pc1)?(pc1):"",
			(pc2)?(pc2):"", (pc3)?(pc3):"");

		if (output_platform(pcPkg, pc2, pdb, fp)) {
			result = 1;
			goto convert_to_done;
		}

		if (pc1)
			free(pc1);
		if (pc2)
			free(pc2);
		if (pc3)
			free(pc3);
		pc1 = pc2 = pc3 = NULL;
	}

convert_to_done:

	genericdb_result_free(&r2);
	(void) fclose(fp);

	return (result);
}

/*
 * quote
 *
 * Each single apostraphe is doubled, so as to escape it correctly for
 * SQL submission.  If there are no apostraphes a simple copy of the string
 * is made.
 *
 *  pc   A string which might include apostraphes.
 *
 * Return: A newly allocated string.  The caller must free it.  NULL
 *	may be returned if malloc fails.
 *
 */
static char *
quote(const char *pc)
{
	char *pq = NULL, *pap;
	int i, len = strlen(pc);

	/* Allocate enough to handle ALL apostraphes. */
	pq = (char *)malloc(2 * len);
	if (pq == NULL) {
		progerr(gettext("Could not allocate a quoted string: %s"),
			strerror(errno));
		return (NULL);
	}

	(void) memset(pq, 0, 2*len);
	pap = pq;

	for (i = 0; i < len; i++) {
		if (pc[i] == APOSTRAPHE) {
			*pap = APOSTRAPHE;
			pap++;
			*pap = APOSTRAPHE;
			pap++;
		} else {
			*pap = pc[i];
			pap++;
		}
	}
	return (pq);
}

/*
 * get_eptlist
 *
 * Returns the currently-populated or unpopulated eptlist.  This global variable
 * will hopefully be privatized, and accessed through a routine such as this
 * one.  Currently it is still public.
 *
 * This routine's primary role is to supply a functional interface for
 * accessing the 'eptlist' global variable.  Some applications need
 * functional interfaces in order to enable a runtime lazy linkage to
 * said variable (think Live Upgrade)
 *
 * Return: The global 'eptlist' variable.
 *
 */
struct cfent **
get_eptlist(void)
{
	return (eptlist);
}

/*
 * get_eptnum
 *
 * Returns the eptnum value.  This global variable will hopefully be
 * privatized, and accessed through a routine such as this one.
 * Currently it is still public.
 *
 * This routine's primary role is to supply a functional interface for
 * accessing the 'eptnum' global variable.  Some applications need
 * functional interfaces in order to enable a runtime lazy linkage to
 * said variable (think Live Upgrade)
 *
 * Return: The global 'eptnum' variable.
 *
 */
int
get_eptnum(void)
{
	return (eptnum);
}

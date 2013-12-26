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
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "pkgadm_special.c	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgadm_special.c	1.3 (gritter) 2/24/07
 */

/*
 * pkgadm_special.c
 *
 * This file contains code to generate an up to date contents file using
 * the entries in the special_contents file, if it exists.
 *
 * Version: 1.2
 * Date:    02/27/06
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <pkgstrct.h>

#include "pkgadm.h"
#include "pkgadm_msgs.h"
#include "dbsql.h"

#define	TCONTENTS	VSADMREL "/install/t.contents"
#define	CONTENTS	VSADMREL "/install/contents"
/*
 * registry_timestamp
 *
 * The registry_timestamp is used to keep track of when the special_contents
 * file was read in and cached.  That way, the data can be used for operations
 * until such a time as the file changes.  We can find out what the change time
 * of the file was when we cached it from source_mtime.  If the copy is stale,
 * we reread the data and once done, alter the registry_timestamp field.
 *
 *   source_name is the table data source file(s), ie. 'special_contents'
 *      that is cached.
 *   source_version is the version of the source, which has been
 *      translated.  This defaults to '1.0', but in the future this
 *      could redirect the code
 *   source_mtime is the modification time of the source file(s).  This
 *      can be compared against the latest version of a file.
 *
 * This table is used the following way:
 *     struct stat sb;
 *     genericdb_Error err;
 *     genericdb_result r;
 *     char *pc;
 *     unsigned long ul;
 *     char *query = "SELECT source_mtime FROM registry_timestamp " \
 *         "WHERE source_name='special_contents';";
 *
 *     assert(stat(filename, &sb) == 0);
 *     assert(genericdb_querySQL(pdb, query, &r) == genericdb_OK);
 *     if (genericdb_result_table_nrows(r) != 0 ||
 *         genericdb_result_table_str(r, 0, 0, &pc) != genericdb_OK ||
 *         (ul = strtoul(pc, NULL, 10)) == 0 ||
 *         (ul == ULONG_MAX) ||
 *         (errno != 0)) {
 *             assert(0);
 *     }
 *     if (genericdb_result_table_nrows(r) == 0 || ul < sb.st_mtime)
 *         recreate_special_table();
 *
 *     use_special_table();
 *
 * There is one special_contents entry per line in the special_contents file.
 * Two queries are performed to generate a new set of contents.  The first
 * finds all entries which are common to both.
 *
 *     SELECT pkg_table.path (etc) FROM special_contents, pkg_table
 *         WHERE special_contents.path=pkg_table.path
 *           AND special_contents.wild='false';
 *
 * The second handles wildcard entries:
 *
 *     SELECT path FROM special_contents WHERE special_contents.wild='true';
 *
 * Then, one by one, new queries are generated.
 *
 *     SELECT pkg_table.path (etc) FROM pkg_table WHERE path LIKE '%s%';
 *
 * All the entries from the pkg_table table must be sorted and written out
 * to the contents file.
 *
 * The existing contents file is read in, line by line.  Entries from the
 * pkg_table table selected due to the above queries take precedence over the
 * existing entries.
 *
 * Output is written to a new file: t.contents.
 * If everything is correctly handled, t.contents will be renamed contents.
 */

static const char *pcDropTimestamp = \
	"DROP TABLE registry_timestamp;";

static const char *pcTimestamp = \
	"CREATE TABLE registry_timestamp (" \
	"source_name VARCHAR(40) NOT NULL, " \
	"source_version VARCHAR(40) NOT NULL, " \
	"source_mtime VARCHAR(40) NOT NULL, " \
	"PRIMARY KEY (source_name) ON CONFLICT REPLACE);";

static const char *pcDropSpecialEntries = \
	"DROP TABLE special_contents;";

static const char *pcSpecialEntries = \
	"CREATE TABLE special_contents (" \
	"path VARCHAR(1024) NOT NULL, "\
	"wild CHAR(5) DEFAULT 'false', " \
	"PRIMARY KEY (path) ON CONFLICT REPLACE);";

#define	FREE(x)	free(x); x = NULL

/*
 * free_cfent
 *
 *   Used to clean up after having created a cfent.
 *
 *      ept	The cfent to free.
 *
 * Return: nothing
 * Side effects: none
 */
static void
free_cfent(struct cfent *ept)
{

	struct pinfo *pi;

	if (ept == NULL)
		return;

	if (ept->path)
		free(ept->path);
	if (ept->ainfo.local)
		free(ept->ainfo.local);
	for (pi = ept->pinfo; pi; ) {
		struct pinfo *ptemp = pi;
		pi = pi->next;
		free(ptemp);
	}
}

/*
 * pkgs_to_pinfo_list
 *
 *  pcPkgs    Space delimited list of package names.
 *
 * Returns: Null terminated linked list of struct pinfo entries corresponding
 *    to the names in the pcPkgs list.  The caller must free these.
 * Side effects: None.
 */
static struct pinfo *
pkgs_to_pinfo_list(const char *pcPkgs)
{
	struct pinfo *pi, *piEnd = NULL, *piBegin = NULL;
	unsigned int i, j, k;

	i = 0; j = 0;
	if (pcPkgs == NULL)
		return (NULL);

	k = strlen(pcPkgs);
	while (i < k && j < k) {
		if (pcPkgs[j] == '\t' || pcPkgs[j] == ' ' ||
		    pcPkgs[j + 1] == '\0') {

			/* The algorithm expects j to be 1 past the string. */
			if (pcPkgs[j + 1] == '\0')
				j++;

			if (i < j) {
				pi = (struct pinfo *)
				    calloc(sizeof (struct pinfo), 1);
				if (pi == NULL) {
					return (NULL);
				}
				(void) memset(pi->pkg, 0, PKGSIZ + 1);
				(void) memcpy(pi->pkg, &(pcPkgs[i]), (j - i));

				if (piBegin == NULL)
					piBegin = pi;
				if (piEnd == NULL) {
					piEnd = pi;
				} else {
					/* Append to the end to keep order. */
					piEnd->next = pi;
					piEnd = pi;
				}
			}
			i = j + 1;
			j++;
		} else {
			j++;
		}
	}

	return (piBegin);
}

/*
 * fill_cfent
 *
 * Takes field values and fills a cfent.  Either the fields will be
 * used to create the cfent or something will go wrong.  Either way, all
 * memory allocated by the fields will be freed by the time this function
 * returns except that used by the fields, which are returned.
 *
 * Returns: 0 on OK, nonzero on error.
 * Side effects:  Frees all the parameters passed in!!!!
 */
static int
fill_cfent(char *pcPath, char *pcFtype, char *pcClass,
    char *pcMode, char *pcOwner, char *pcGrp,
    char *pcMajor, char *pcMinor, char *pcSz,
    char *pcSum, char *pcModtime, char *pcPkgstatus,
    char *pcPkgs, struct cfent *ept)
{

	if (ept == NULL || !pcPath || !pcFtype || !pcClass || !pcMode ||
	    !pcGrp || !pcMajor || !pcMinor || !pcSz || !pcSum ||
	    !pcModtime || !pcOwner || !pcPkgstatus || !pcPkgs)
		goto fail_create_cfent;

	(void) memset(ept, 0, sizeof (struct cfent));

	ept->path = pcPath;

	/* This is derived in the db, not in cfent per se. */
	FREE(pcPkgstatus);

	ept->ftype = pcFtype[0];
	FREE(pcFtype);

	(void) strncpy(ept->pkg_class, pcClass, CLSSIZ + 1);
	FREE(pcClass);

	errno = 0;
	ept->cinfo.cksum = atol(pcSum);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcSum);

	ept->cinfo.size = atol(pcSz);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcSz);

	ept->cinfo.modtime = (time_t)atol(pcModtime);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcModtime);

	errno = 0;
	ept->ainfo.mode = (mode_t)atol(pcMode);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcMode);

	(void) strncpy(ept->ainfo.owner, pcOwner, ATRSIZ + 1);
	(void) strncpy(ept->ainfo.group, pcGrp, ATRSIZ + 1);
	FREE(pcOwner);
	FREE(pcGrp);

	errno = 0;
	ept->ainfo.major = (major_t)atol(pcMajor);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcMajor);

	ept->ainfo.minor = (minor_t)atol(pcMinor);
	if (errno != 0)
		goto fail_create_cfent;
	FREE(pcMinor);

	ept->pinfo = pkgs_to_pinfo_list(pcPkgs);
	return (0);

fail_create_cfent:

	if (pcPath)
		free(pcPath);
	if (pcFtype)
		free(pcFtype);
	if (pcClass)
		free(pcClass);
	if (pcMode)
		free(pcMode);
	if (pcOwner)
		free(pcOwner);
	if (pcGrp)
		free(pcGrp);
	if (pcMajor)
		free(pcMajor);
	if (pcMinor)
		free(pcMinor);
	if (pcSz)
		free(pcSz);
	if (pcModtime)
		free(pcModtime);
	if (pcPkgstatus)
		free(pcPkgstatus);
	if (pcPkgs)
		free(pcPkgs);

	return (1);
}

/*
 * fill_entry
 *
 * Fill entry takes contents which have been retrieved from a query and
 * adds them to an array of entries.  Several queries will be needed to
 * find values which have wildcards, so this routine will be called with
 * different values of r.  fill_entry is called once per row, for each
 * relevant query result.
 *
 *    r result from query of contents, a particularly query.
 *    i the content row number.
 *    ientry the 'entry' number - index into the pcfe array.
 *    pcfe   the array of cfents which will be filled in from results.
 *
 * Result: 0 on success, nonzero on error.
 * Side effects: pcfe fields will get filled in, memory allocated for
 *    certain fields which must be freed later by the caller.  See
 *    free_cfent() above.
 */
static int
fill_entry(genericdb_result r, int i, int ientry, struct cfent *pcfe)
{
	genericdb_Error e;
	char *pce[13];
	int ii;
	for (ii = 0; ii < 13; ii++)
		pce[ii] = NULL;

	/* path */
	if (genericdb_result_table_str(r, i, 0, &pce[0]) != genericdb_OK ||
	    /* ftype */
	    genericdb_result_table_str(r, i, 1, &pce[1]) != genericdb_OK ||
	    /* class */
	    genericdb_result_table_str(r, i, 2, &pce[2]) != genericdb_OK ||
	    /* mode - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 3, &pce[3]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* owner - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 4, &pce[4]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* grp - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 5, &pce[5]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* major - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 6, &pce[6]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* minor - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 7, &pce[7]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* sz - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 8, &pce[8]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* sum - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 9, &pce[9]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* modtime - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 10, &pce[10]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* pkgstatus - can be NULL */
	    ((e = genericdb_result_table_str(r, i, 11, &pce[11]))
		!= genericdb_OK && e != genericdb_NULL) ||
	    /* pkgs */
	    genericdb_result_table_str(r, i, 12, &pce[12])
	    != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(e));
		goto fill_entry_fail;
	}

	/* It is not necessary to free pce strings; fill_cfent does this. */
	return (fill_cfent(pce[0], pce[1], pce[2], pce[3], pce[4],
	    pce[5], pce[6], pce[7], pce[8], pce[9], pce[10], pce[11],
	    pce[12], &(pcfe[ientry])));

fill_entry_fail:
	for (ii = 0; ii < 13; ii++)
		if (pce[ii])
			free(pce[ii]);

	return (1);
}

/*
 * cfent_comp
 *
 * Compares two cfent path values for use with qsort.
 * Returns: 0, -1, 1 for equivalence, 1 less than 2 or 1 more than 2
 *   as expected by qsort.
 * Side effects: none
 */
int
cfent_comp(const void *pv1, const void *pv2)
{
	const struct cfent *pcfe1 = (const struct cfent *) pv1;
	const struct cfent *pcfe2 = (const struct cfent *) pv2;
	int i = 0;
	assert(pv1 != NULL && pv2 != NULL);
	i = strcmp(pcfe1->path, pcfe2->path);
	if (i < 0)
		return (-1);
	if (i > 0)
		return (1);
	return (0);
}

/*
 * generate_contents_from_special
 *
 * First, we get all exact matches - special_contents entries without a
 * wildcard which match those in the contents table pkg_table.  Then, for
 * each wildcard entry, we query those matching entries in the pkg_table
 * tables.  All these results are collated in an array of cfent structs.
 * This array could be out of order and have redundent entries.  We
 * first sort the entry (using qsort in strcmp order) and then remove
 * redundent entries.
 *
 * The resulting array of cfent structures is output to a new contents
 * file in $root/var/sadm/install/contents - replacing whatever one
 * existed previously.  This is the correct behavior for this routine.
 * Only if the complete 'write out' of this file is successful is the
 * contents file changed.  Otherwise, t.contents is written to.
 *
 * Returns: 0 on success, nonzero on error.
 * Side effects:  This may generate a cached special_contents table
 *    and modify the contents file.
 */
static int
generate_contents_from_special(const char *pcroot, genericdb *pdb)
{
	genericdb_result *rwild, rx, rn;
	genericdb_Error e;
	FILE *fp = NULL;
	int result = 0, i = 0, j, n = 0, nrows = 0;
	struct cfent *pcfe = NULL;
	time_t t;

	char *query1 = "SELECT pkg_table.path, pkg_table.ftype, " \
		"pkg_table.class, pkg_table.mode, pkg_table.owner, " \
		"pkg_table.grp, pkg_table.major, pkg_table.minor, " \
	    "pkg_table.sz, pkg_table.sum, pkg_table.modtime, " \
		"pkg_table.pkgstatus, pkg_table.pkgs " \
	    "FROM special_contents, pkg_table " \
	    "WHERE special_contents.path=pkg_table.path " \
	    "AND special_contents.wild='false';";

	char *query2 = "SELECT path FROM special_contents " \
	    "WHERE special_contents.wild='true';";

	char *query3 = "SELECT path, ftype, class, " \
	    "mode, owner, grp, major, minor, " \
	    "sz, sum, modtime, pkgstatus, pkgs " \
	    "FROM pkg_table " \
	    "WHERE path LIKE '%s%%';";
	char buf[BUFSZ];
	char path[PATH_MAX];

	(void) memset(&rx, 0, GENERICDB_RESULT_SIZE);
	(void) memset(&rn, 0, GENERICDB_RESULT_SIZE);

	/* Get exact matches and wildcard entries. */
	if ((e = genericdb_querySQL(pdb, query1, &rx)) != genericdb_OK ||
	    (e = genericdb_querySQL(pdb, query2, &rn)) != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(e));
		result = 1;
		goto generate_done;
	}

	n = genericdb_result_table_nrows(rx);
	nrows = genericdb_result_table_nrows(rn);

	if (nrows > 0) {
		rwild = (genericdb_result *)
		    calloc(nrows + 1, sizeof (genericdb_result));
		if (rwild == NULL) {
			log_msg(LOG_MSG_ERR, MSG_MEM);
			result = 1;
			goto generate_done;
		}
	}

	/* For each wild entry, get all matching contents. */
	for (i = 0; i < nrows; i++) {
		char *pcPath = NULL;

		e = genericdb_result_table_str(rn, i, 0, &pcPath);

		if (e != genericdb_OK) {
			log_msg(LOG_MSG_INFO, MSG_DB_OP_FAILSTR,
			    genericdb_errstr(e));
			result = 1;
			goto generate_done;
		}

		if (pcPath == NULL) {
			log_msg(LOG_MSG_INFO, MSG_DB_OP_FAILED);
			result = 1;
			goto generate_done;
		}

		if (snprintf(buf, BUFSZ, query3, pcPath) >= 2048) {
			result = 1;
			goto generate_done;
		}

		(void) memset(&(rwild[i]), 0, GENERICDB_RESULT_SIZE);

		if ((e = genericdb_querySQL(pdb, buf, &(rwild[i]))) !=
		    genericdb_OK) {
			log_msg(LOG_MSG_INFO, MSG_DB_OP_FAILSTR,
			    genericdb_errstr(e));
			result = 1;
			goto generate_done;
		}

		if (genericdb_result_table_nrows(rwild[i]) < 0) {
			log_msg(LOG_MSG_INFO, MSG_DB_OP_FAILED);
			result = 1;
			goto generate_done;
		}

		n += genericdb_result_table_nrows(rwild[i]);
	}

	/*
	 * We now have the exact and wild card matches.  Create an array of
	 * cfent structures.  We have to put this into one single array so
	 * we can sort it and remove duplicates.  A duplicate could occur if
	 * special_contents includes duplicate entries or redundent rules.
	 */
	pcfe = (struct cfent *)calloc(n + 1, sizeof (struct cfent));
	if (pcfe == NULL) {
		log_msg(LOG_MSG_ERR, MSG_MEM);
		result = 1;
		goto generate_done;
	}

	/* The variable 'n' holds the number of entries actually filled. */
	nrows = genericdb_result_table_nrows(rx);
	for (n = 0, i = 0; i < nrows; i++, n++) {
		if (fill_entry(rx, i, n, pcfe) != 0) {
			result = 1;
			goto generate_done;
		}
	}

	nrows = genericdb_result_table_nrows(rn);
	for (j = 0; j < nrows; j++) {
		for (i = 0; i < genericdb_result_table_nrows(rwild[j]); i++) {
			if (fill_entry(rwild[j], i, n, pcfe) != 0) {
				result = 1;
				goto generate_done;
			}
			n++;
		}
	}

	/* Sort results. */
	qsort(pcfe, n, sizeof (struct cfent), cfent_comp);

	/* Remove duplicate entries. */
	for (i = 0; i < (n - 1); i++) {
		while (strcmp(pcfe[i].path, pcfe[i + 1].path) == 0) {
			for (j  = i; j < (n - 1); j++) {
				pcfe[j] = pcfe[j + 1];
			}
			n -= 1;
			if (i == (n - 1))
				break;
		}
	}

	/* Output results to the contents file. */
	if (pcroot == NULL)
		pcroot = "/";
	if (pcroot[strlen(pcroot) - 1] == '/') {
		if (snprintf(buf, PATH_MAX, "%s%s", pcroot, TCONTENTS)
		    >= PATH_MAX ||
		    snprintf(path, PATH_MAX, "%s%s", pcroot, CONTENTS)
		    >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);
			result = 1;
			goto generate_done;
		}

	} else {
		if (snprintf(buf, PATH_MAX, "%s/%s", pcroot, TCONTENTS)
		    >= PATH_MAX ||
		    snprintf(path, PATH_MAX, "%s/%s", pcroot, CONTENTS)
		    >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			result = 1;
			goto generate_done;
		}
	}

	fp = fopen(buf, "w");
	if (fp == NULL) {
		result = 1;
		goto generate_done;
	}

	for (i = 0; i < n; i++) {
		if (putcfile(&(pcfe[i]), fp) < 0) {
			(void) fclose(fp);
			(void) remove(buf);
			result = 1;
			goto generate_done;
		}
	}

	t = time(NULL);
	(void) fprintf(fp, "# Last modified by pkgadm special\n");
	(void) fprintf(fp, "# %s", ctime(&t));

	(void) fclose(fp);
	fp = NULL;

generate_done:
	for (i = 0; rwild && rwild[i]; i++)
		genericdb_result_free(rwild[i]);
	if (rwild)
		free(rwild);

	genericdb_result_free(rx);
	genericdb_result_free(rn);

	if (fp != NULL)
		(void) fclose(fp);

	if (result == 0) {
		(void) rename(buf, path);
	} else {
		(void) remove(buf);
	}
	free_cfent(pcfe);
	if (pcfe)
		free(pcfe);

	return (result);
}

/*
 * refresh_special_contents
 *
 * If the named special_contents file is newer than the cached version
 * the entries in that file are read in and cached.
 *
 *   ulTime     The timestamp of the cache.
 *   pcFile     The special_content file's filename and path.
 *   pdb        The db session.
 *
 * Returns: 0 on success, nonzero otherwise.
 * Side effects:  If successful, the registry_timestamp table and
 *    special_contents table may be updated with the contents of the
 *    $root/var/sadm/install/special_contents file entries.
 */
static int
refresh_special_contents(unsigned long ulTime, const char *pcfile,
    genericdb *pdb)
{
	FILE *fp = fopen(pcfile, "r");
	genericdb_Error e;
	char line[PATH_MAX + 256];
	char buf[BUFSZ];
	char *pcIt = "INSERT INTO registry_timestamp " \
	    "(source_name, source_version, source_mtime) "
	    "VALUES ('special_contents', '1.0', '%lu');";
	char *pcIs = "INSERT INTO special_contents " \
	    "(path, wild) VALUES ('%s', '%s');";
	struct dstr d;
	int result = 0;

	init_dstr(&d);

	if (fp == NULL) {
		log_msg(LOG_MSG_ERR, MSG_BAD_SPECIAL_ACCESS, pcfile);
		result = 1;
		goto refresh_done;
	}

	/*
	 * Create a transaction to update the registry_timestamp and
	 * special_contents table to reflect the special_contents file.
	 */
	if (append_dstr(&d, "BEGIN TRANSACTION;") ||
	    snprintf(buf, BUFSZ, pcIt, ulTime) >= BUFSZ ||
	    append_dstr(&d, buf)) {
		result = 1;
		goto refresh_done;
	}

	while (fgets(line, (PATH_MAX + 256), fp) != NULL) {
		int i = strlen(line);

		if (line[0] == '#' || isspace((int)line[0]))
			continue;

		/* Remove the newline from the input string. */
		if (line[i - 1] == '\n') {
			line[i - 1] = '\0';
			i--;
		}

		/*
		 * Check if its a wildcard entry or not.  If so remove the
		 * '*'.  Create the insert line (either way).
		 */
		if (line[i - 1] == '*') {
			line[i - 1] = '\0';
			if (snprintf(buf, BUFSZ, pcIs, line, "true")
			    >= BUFSZ) {
				result = 1;
				goto refresh_done;
			}
		} else {
			if (snprintf(buf, BUFSZ, pcIs, line, "false")
			    >= BUFSZ) {
				result = 1;
				goto refresh_done;
			}
		}

		if (append_dstr(&d, buf)) {
			result = 1;
			goto refresh_done;
		}

	}

	if (append_dstr(&d, "COMMIT;")) {
		result = 1;
		goto refresh_done;
	}
	if ((e = genericdb_submitSQL(pdb, d.pc)) != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(e));
		result = 1;
	}

refresh_done:
	if (fp)
		(void) fclose(fp);
	free_dstr(&d);
	return (result);
}

/*
 * special_contents_generator
 *
 * This routine does all the work of generating the special contents file.
 * If there is no special_contents file in $root/var/sadm/install then
 * there is nothing to do.  If there is such a file, we check to see if
 * we have its entries cached.  If it is not cached, we read it in and
 * cache it.  To do the actual generation of the contents file, we call
 * generate_contents from special.  In any case, we attempt to create the
 * registry_timestamp and special_contents tables, since we will need them
 * later and creating them will fail if they already exist.
 *
 * Returns: 0 on success, nonzero on failure.
 * Side effects: May later registry_timestamp and special_contents tables.
 *    may result in overwriting or creating a contents file.
 */
int
special_contents_generator(const char *pcroot)
{
	struct stat s;
	int result = 0, e;
	char path[PATH_MAX];
	char *pcMtime;
	char *pc = VSADMREL "/install/special_contents";
	char *pcQ = "SELECT source_mtime FROM registry_timestamp " \
	    "WHERE source_name='special_contents';";
	genericdb *pdb;
	genericdb_Error err;
	genericdb_result r;
	unsigned long ul = 0;
#ifndef NDEBUG
	int nparams = 2;
	char *params[] = { "-l", "3", NULL };
#else
	int nparams = 0;
	char *params[] = { NULL };
#endif

	(void) memset(&r, 0, GENERICDB_RESULT_SIZE);

	if (pcroot == NULL) {
		pcroot = "/";
	}

	if (pcroot[strlen(pcroot) - 1] == '/') {
		if (snprintf(path, PATH_MAX, "%s%s", pcroot, pc) >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			return (1);
		}
	} else {
		if (snprintf(path, PATH_MAX, "%s/%s", pcroot, pc)
		    >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);

			return (1);
		}
	}

	errno = 0;
	e = stat(path, &s);
	if (e != 0 && errno == ENOENT)
		return (0); /* No special contents file.  Do nothing. */

	if (access(path, R_OK) != 0) {
		log_msg(LOG_MSG_ERR, MSG_BAD_SPECIAL_ACCESS, path);
		return (1);
	}

	if ((pdb = genericdb_open(pcroot, GENERICDB_WR_MODE,
	    nparams, params, &err)) == NULL || err != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_NO_OPEN_DB);
		return (1);
	}

	err = genericdb_submitSQL(pdb, pcDropTimestamp);
	err = genericdb_submitSQL(pdb, pcDropSpecialEntries);
	if (genericdb_submitSQL(pdb, pcTimestamp) != genericdb_OK ||
	    genericdb_submitSQL(pdb, pcSpecialEntries) != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_BAD_SPECIAL_ACCESS);
		return (1);
	}
	
	/*
	 * If there is no timestamp entry or the value in the db is old,
	 * refresh the database with new special_contents.
	 */
	if (refresh_special_contents(s.st_mtime, path, pdb)) {
		result = 1;
		goto special_done;
	}

	if (generate_contents_from_special(pcroot, pdb)) {
		result = 1;
		goto special_done;
	}

special_done:
	genericdb_result_free(r);

	if (pdb)
		genericdb_close(pdb);

	return (result);
}

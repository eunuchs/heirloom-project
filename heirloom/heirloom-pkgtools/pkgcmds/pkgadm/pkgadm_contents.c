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

/*	from OpenSolaris "pkgadm_contents.c	1.4	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgadm_contents.c	1.3 (gritter) 2/24/07
 */

/*
 * This file contains the means to convert contents files to db format
 * and back to contents file form.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <pkgstrct.h>
#include "pkglib.h"
#include "pkgadm.h"
#include "pkgadm_msgs.h"
#include "dbsql.h"

/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from a contents file to a database.
 * ----------------------------------------------------------------------
 */

/*
 * This appends a single content item to the insert buffer as SQL.
 * This buffer will be executed subsequently.
 *
 * Return: 0 = success, 1 = failure.
 * Side effects: none.
 */
static int
append_contents_sql(struct dstr *pd, struct cfent *pcfent)
{
	int result = 0; /* Assume success. */
	int type = 0;
	char buf[BUFSZ];
	struct pinfo *p;
	int hasStatus = 0;
	int i;

	if (append_dstr(pd, "INSERT INTO pkg_table ")) {
		result = 1;
		goto append_done;
	}

	switch (pcfent->ftype) {
	case 'i':
		goto append_done; /* This does not get put into the db. */

	case 'e':
	case 'f':
	case 'v':
		/* File */
		type = 1;
		if (append_dstr(pd, "(path, ftype, class, mode, owner, grp, "
		    "sz, sum, modtime, pkgs, pkgstatus) ")) {
			result = 1;
			goto append_done;
		}
		break;

	case 'b':
	case 'c':
		/* Device */
		type = 2;
		if (append_dstr(pd, "(path, ftype, class, major, minor, "
		    "mode, owner, grp, pkgs, pkgstatus) ")) {
			result = 1;
			goto append_done;
		}
		break;

	case 's':
	case 'l':
	case '?':
		/* Link or unknown type */
		type = 3;
		if (append_dstr(pd, "(path, ftype, class, pkgs, pkgstatus) ")) {
			result = 1;
			goto append_done;
		}
		break;

	case 'd':
	case 'x':
		/* Directory */
		type = 4;
		if (append_dstr(pd, "(path, ftype, class, mode,"
		    "owner, grp, pkgs, pkgstatus) ")) {
			result = 1;
			goto append_done;
		}
		break;

	default:
		log_msg(LOG_MSG_ERR, MSG_CONTENTS_FORMAT);
		result = 1;
		goto append_done;
	}

	/* add path, type and class - for all types */
	if (pcfent->ainfo.local) {
		if (snprintf(buf, BUFSZ, "VALUES ('%s=%s', '%c', '%s', ",
		    pcfent->path, pcfent->ainfo.local,
		    pcfent->ftype, pcfent->pkg_class) >= BUFSZ) {
			result = 1;
			goto append_done;
		}

	} else {
		if (snprintf(buf, BUFSZ, "VALUES ('%s', '%c', '%s', ",
		    pcfent->path, pcfent->ftype, pcfent->pkg_class) >= BUFSZ) {
			result = 1;
			goto append_done;
		}
	}

	if (append_dstr(pd, buf)) {
		result = 1;
		goto append_done;
	}

	/* Add major minor */
	if (type == 2) {

		i = 0;

		if (pcfent->ainfo.major != BADMAJOR) {
			if ((i += snprintf(buf, BUFSZ, "'%ld', ",
			    pcfent->ainfo.major)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}
		} else {
			if ((i += snprintf(buf, BUFSZ, "'-1', ")) >= BUFSZ) {
				result = 1;
				goto append_done;
			}
		}

		if (pcfent->ainfo.minor != BADMINOR) {
			if ((i += snprintf(&buf[i], BUFSZ, "'%ld', ",
			    pcfent->ainfo.minor)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}
		} else {
			if ((i += snprintf(&buf[i], BUFSZ, "'-1', "))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}
		}

		if (append_dstr(pd, buf)) {
			result = 1;
			goto append_done;
		}
	}

	/* Add perm owner grp */
	if (type == 1 || type == 2 || type == 4) {

		i = 0;

		if (pcfent->ainfo.mode != BADMODE) {
			if ((i += snprintf(&buf[i], BUFSZ, "'%ld', ",
			    pcfent->ainfo.mode)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		} else {
			if ((i += snprintf(&buf[i], BUFSZ, "'-1', "))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}
		}

		if ((i += snprintf(&buf[i], BUFSZ, "'%s', '%s', ",
		    pcfent->ainfo.owner, pcfent->ainfo.group)) >= BUFSZ) {
			result = 1;
			goto append_done;
		}

		if (append_dstr(pd, buf)) {
			result = 1;
			goto append_done;
		}
	}

	/* Add sz sum modtime */
	if (type == 1) {

		i = 0;

		if (pcfent->cinfo.size != BADCONT) {
			if ((i += snprintf(&buf[i], BUFSZ, "'%ld', ",
			    pcfent->cinfo.size)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		} else {
			if ((i += snprintf(&buf[i], BUFSZ, "'-1', "))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		}

		if (pcfent->cinfo.cksum != BADCONT) {
			if ((i += snprintf(&buf[i], BUFSZ, "'%ld', ",
			    pcfent->cinfo.cksum)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		} else {
			if ((i += snprintf(&buf[i], BUFSZ, "'-1', "))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		}

		if (pcfent->cinfo.modtime != BADCONT) {
			if ((i += snprintf(&buf[i], BUFSZ, "'%ld', ",
			    pcfent->cinfo.modtime)) >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		} else {
			if ((i += snprintf(&buf[i], BUFSZ, "'-1', "))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		}

		if (append_dstr(pd, buf)) {
			result = 1;
			goto append_done;
		}
	}

	/* add packages - all types */
	hasStatus = 0;
	if (append_dstr(pd, "'")) {
		result = 1;
		goto append_done;
	}

	for (p = pcfent->pinfo; p != NULL; p = p->next) {
		i = 0;

		/* A pkg can be [status]pkg/[:ftype][:class]] for any entry. */

		if (p->status != '\0') {
			if ((i = snprintf(buf, BUFSZ, "%c", p->status))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}

			hasStatus = 1;

		} else {
			i = 0;
		}

		if ((i += snprintf(&buf[i], BUFSZ, "%s", p->pkg)) >= BUFSZ) {
			result = 1;
			goto append_done;
		}

		if (p->editflag) {
			if ((i += snprintf(&buf[i], BUFSZ, "\\")) >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		}

		if (p->aclass[0] != '\0') {
			if ((i += snprintf(&buf[i], BUFSZ, ":%s", p->aclass))
			    >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		}

		if (p->next != NULL)
			if (snprintf(&buf[i], BUFSZ, " ") >= BUFSZ) {
				result = 1;
				goto append_done;
			}

		if (append_dstr(pd, buf)) {
			result = 1;
			goto append_done;
		}
	}

	/* Terminate the pkgs string */
	if (append_dstr(pd, "', ")) {
		result = 1;
		goto append_done;
	}

	/*
	 * Add the pkgstatus flag.  If it is 1 then one or more pkgs have
	 * a non-default status indicator.
	 */
	if (hasStatus) {
		if (append_dstr(pd, "'1'")) {
			result = 1;
			goto append_done;
		}
	} else {
		if (append_dstr(pd, "'0'")) {
			result = 1;
			goto append_done;
		}
	}

	/* Complete the SQL statement */
	if (append_dstr(pd, ");\n")) {
		result = 1;
		goto append_done;
	}

append_done:
	return (result);
}

/*
 * convert_contents_to_sql
 *
 * Converts the entire contents file into a set of SQL transactions which
 * populate the database.  The reason that a single transaction is not used
 * is that some systems run out of memory in this case.
 *
 * Return: 0 on success, nonzero on failure
 */
int
convert_contents_to_sql(struct dstr *pd, const char *pcroot)
{
	VFP_T		*vfp;
	char		path[PATH_MAX];
	int		n;
	int		total = 0;
	struct cfent	entry;

	if (pcroot == NULL) {
		pcroot = "/";
	}

	if (pcroot[strlen(pcroot) - 1] == '/') {
		if (snprintf(path, sizeof (path),
			"%s" VSADMREL "/install/contents", pcroot)
						>= sizeof (path)) {
			return (1);
		}
	} else {
		if (snprintf(path, sizeof (path),
			"%s" VSADMDIR "/install/contents", pcroot)
						>= sizeof (path)) {
			return (1);
		}
	}

	if (vfpOpen(&vfp, path, "r", VFP_NEEDNOW) != 0) {
		log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, path, strerror(errno));
		return (1);
	}

	while ((n = srchcfile(&entry, "*", vfp, (VFP_T *)NULL)) > 0) {
		if (append_contents_sql(pd, &entry)) {
			(void) vfpClose(&vfp);
			return (1);
		}

		total++;
	}

	(void) vfpClose(&vfp);

	if (n < 0) {
		log_msg(LOG_MSG_ERR, MSG_CONTENTS_FORMAT);
		return (1);
	}

	return (0);
}

/*
 * ----------------------------------------------------------------------
 * These routines are used to convert from a database to a contents file.
 * ----------------------------------------------------------------------
 */

/*
 * generate_contents_line
 *
 * Take a bunch of strings and generate a line in the contents file.
 *
 *    pce	This is a row by row dump of the contents in the
 *		pkg_table database.
 *
 *	[0]	path
 *	[1]	ftype
 *	[2]	class
 *	[3]	mode
 *	[4]	owner
 *	[5]	grp
 *	[6]	major
 *	[7]	minor
 *	[8]	sz
 *	[9]	sum
 *	[10]	modtime
 *	[11]	pkgstatus
 *	[12]	pkgs
 *
 * Returns: 0 on success, nonzero on failure.
 * Side effects: writes to a file.  The strings passed in are freed.
 * This frees the pce[] array of strings.
 */
static int
generate_contents_line(FILE *fp, char *pce[])
{

	int j, code = 0;
	long mode_l = 0;
	int bad_mode = 0;

	(void) fprintf(fp, "%s %c %s ", pce[0], pce[1][0], pce[2]);

	errno = 0;
	mode_l = strtol(pce[3], NULL, 10);
	if (errno != 0) {
		bad_mode = 1;
	}

	switch (pce[1][0]) {
	case 'i':
		goto cleanup;

	case 'e':
	case 'f':
	case 'v':
		if (bad_mode == 0 && !streq(pce[3], "-1")) {
			(void) fprintf(fp, "%04o %s %s %s %s %s %s\n",
			    (int)mode_l,
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    (pce[8] && !streq(pce[8], "-1"))?(pce[8]):"?",
			    (pce[9] && !streq(pce[9], "-1"))?(pce[9]):"?",
			    (pce[10] && !streq(pce[10], "-1")) ?
						(pce[10]):"?",
			    pce[12]);
		} else {
			(void) fprintf(fp, "%s %s %s %s %s %s %s\n",
			    "?",
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    (pce[8] && !streq(pce[8], "-1"))?(pce[8]):"?",
			    (pce[9] && !streq(pce[9], "-1"))?(pce[9]):"?",
			    (pce[10] && !streq(pce[10], "-1"))?
						(pce[10]):"?",
			    pce[12]);
		}
		break;

	case 'b':
	case 'c':
		if (bad_mode == 0) {
			(void) fprintf(fp, "%s %s %04o %s %s %s\n",
			    (pce[6])?(pce[6]):"?",
			    (pce[7])?(pce[7]):"?",
			    (int)mode_l,
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    pce[12]);
		} else {
			(void) fprintf(fp, "%s %s %s %s %s %s\n",
			    (pce[6])?(pce[6]):"?",
			    (pce[7])?(pce[7]):"?",
			    "?",
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    pce[12]);
		}
		break;

	case 's':
	case 'l':
	case '?':
		(void) fprintf(fp, "%s\n", pce[12]);
		break;

	case 'x':
	case 'd':
		if (bad_mode == 0) {
			(void) fprintf(fp, "%04o %s %s %s\n",
			    (int)mode_l,
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    pce[12]);
		} else {
			(void) fprintf(fp, "%s %s %s %s\n",
			    "?",
			    (pce[4] && !streq(pce[4], "-1"))?(pce[4]):"?",
			    (pce[5] && !streq(pce[5], "-1"))?(pce[5]):"?",
			    pce[12]);
		}
		break;

	default:
		log_msg(LOG_MSG_ERR, MSG_CONTENTS_FORMAT);
		code = 1;
		goto cleanup;
	}

cleanup:
	for (j = 0; j < 13; j++) {
		if (pce[j])
			free(pce[j]);
		pce[j] = NULL;
	}

	return (code);
}

/*
 * compare_index
 *
 * This is a utility function used for qsort to sort an index over
 * the contents which have been returned from the database.  The problem
 * is that the SQL sort order differs from strcmp().  Using the compare_index
 * routine we can impose a correct ordering on the array of content rows.
 *
 * return 0, 1 or -1 as per qsort.
 * side effects: none
 */
typedef struct index {
	char *pc;
	int d;
} Index;

static int
compare_index(const void *pv1, const void *pv2)
{
	Index *pI1 = (Index *) pv1;
	Index *pI2 = (Index *) pv2;
	char *pc = NULL;
	int i;

	assert(pI1 && pI2);

	if ((pc = strchr(pI1->pc, '=')) != NULL) {
		*pc = '\0';
	}
	if ((pc = strchr(pI2->pc, '=')) != NULL) {
		*pc = '\0';
	}

	i = strcmp(pI1->pc, pI2->pc);

	if (i < 0)
		return (-1);
	if (i > 0)
		return (1);

	return (0);
}

/*
 * convert_db_to_contents
 *
 * Retrieve the entire set of contents from the db and write out to
 * the contents file.  This will replace the existing contents file if
 * one already exists.  Note that the database file will be removed
 * if this operation succeeds!
 *
 *    pdb	A handle to the database.
 *    pcroot	The alternate root.
 *    refresh	If 1, this is a refresh, if 0, a revert.
 *
 * Return 0 on success or nonzero on failure.
 * Side effects: huge - see above.
 */
int
convert_db_to_contents(genericdb *pdb, const char *pcroot, int refresh)
{
	int result = 0; /* Assume success. */
	FILE *fp = NULL;
	char path[PATH_MAX];
	char *pce[13];
	int i = 0, j = 0, k = 0, nrows = 0;
	genericdb_Error err;
	genericdb_result r;
	time_t t;
	Index *pI = NULL;

	char *pcQuery = "SELECT path, ftype, class, mode, owner, grp, " \
	    "major, minor, sz, sum, modtime, pkgstatus, pkgs " \
	    "FROM pkg_table ORDER BY path;";

	(void) memset(&r, 0, GENERICDB_RESULT_SIZE);

	if ((err = genericdb_querySQL(pdb, pcQuery, &r)) != genericdb_OK) {
		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILSTR, genericdb_errstr(err));
		result = 1;
		goto generate_done;
	}

	if (pcroot == NULL)
		pcroot = "/";

	if (pcroot[strlen(pcroot) -1] == '/') {
		if (snprintf(path, PATH_MAX, "%s" VSADMREL "/install/contents",
		    pcroot) >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);
			result = 1;
			goto generate_done;
		}
	} else {
		if (snprintf(path, PATH_MAX, "%s" VSADMDIR "/install/contents",
		    pcroot) >= PATH_MAX) {
			log_msg(LOG_MSG_ERR,
			    gettext(MSG_FILE_NAME_TOO_LONG), pcroot);
			result = 1;
			goto generate_done;
		}
	}

	if ((fp = fopen(path, "w")) == NULL) {
		log_msg(LOG_MSG_ERR, MSG_FILE_ACCESS, path, strerror(errno));
		result = 1;
		goto generate_done;
	}

	if ((i = genericdb_result_table_ncols(r)) != 13) {
		log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILED);
		result = 1;
		goto generate_done;
	}

	/*
	 * Sort the results - since the DB may not use the same sorting
	 * algorithm as the hosts expect.
	 */
	pI = (Index *) malloc(sizeof (Index) *
	    genericdb_result_table_nrows(r) + 1);
	if (pI == NULL) {
		log_msg(LOG_MSG_ERR, MSG_MEM);
		result = 1;
		goto generate_done;
	}

	nrows = genericdb_result_table_nrows(r);
	for (i = 0; i < nrows; i++) {
		char *pcXX;
		if (genericdb_result_table_str(r, i, 0, &pcXX)
		    != genericdb_OK) {
			log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILED);
			result = 1;
			goto generate_done;
		}

		pI[i].pc = pcXX;
		pI[i].d = i;
	}

	qsort(pI, nrows, sizeof (Index),
	    compare_index);

	/* Free the memory used by the many strings in the index. */
	for (i = 0; i < nrows; i++) {
		free(pI[i].pc);
	}

	/* This can handle up to 2048 32 char long pkgs.. a safe bound. */

	for (j = 0; j < nrows; j++) {
		genericdb_Error e;

		/* Use the sorted index not the row order returned by SQL. */
		i = pI[j].d;

		/* Convert the row into a struct cfent entry. */

		for (k = 0; k < 13; k++) pce[k] = NULL;

		/* path */
		if (genericdb_result_table_str(r, i, 0, &pce[0])
		    != genericdb_OK ||
		/* ftype */
		    genericdb_result_table_str(r, i, 1, &pce[1])
		    != genericdb_OK ||
		/* class */
		    genericdb_result_table_str(r, i, 2, &pce[2])
		    != genericdb_OK ||
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
			log_msg(LOG_MSG_ERR, MSG_DB_OP_FAILED);
			goto generate_done;
		}

		if (generate_contents_line(fp, pce)) {
			result = 1;
			goto generate_done;
		}
	}

	t = time(NULL);
	if (refresh) {
		(void) fprintf(fp, "# Last modified by pkgadm refresh\n");
	} else {
		(void) fprintf(fp, "# Last modified by pkgadm revert\n");
	}
	(void) fprintf(fp, "# %s", ctime(&t));

generate_done:
	if (fp)
		(void) fclose(fp);

	genericdb_result_free(r);

	/* The permuted index of content entries, in strcmp order. */
	if (pI)
		free(pI);

	if (result)
		(void) remove(path);

	return (result);
}

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

/*	from OpenSolaris "pkgpatch.c	1.3	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgpatch.c	1.4 (gritter) 2/25/07
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <wait.h>
#include <limits.h>
#include <dirent.h>
#include <libintl.h>
#include <pkgstrct.h>
#include <ftw.h>
#include "pkglib.h"
#include "install.h"
#include "genericdb.h"
#include "libinst.h"
#include "libadm.h"
#include "dbsql.h"

static char *target_file = NULL;
static char *target_path = NULL;

static int find_file(char *, char *);
static int is_file_found(const char *, const struct stat *, int, struct FTW *);
static char *parse_remote(char *);
#ifdef SQLDB_IS_USED
static int insert_backout_patch_db(char *, char *, genericdb *);
#endif
static int set_target_file(char *);
static char *get_target_file(void);
static int get_fs_bo_dir(char *, char *, char *, char **);

/*
 * This function checks to see if the installing package has been patched
 * in its spooled state and is now being installed.
 * If the PATCHLIST pkginfo parameter is set then the package has been
 * patched and pkginstall needs to update the patchdb table.
 *
 * Parameters:
 *             prog_flag - Program flag that states whether or not to
 *                         check the backout package.
 *
 *             pkginst - The package under inspection.
 *
 *             pkginfo -  The pkginfo file of the package under inspection.
 *                         check the backout package.
 *
 *             ir - The installation root.
 *
 *             gdb - A genericdb pointer to the SQL DB.
 *
 * Returns:
 *             0  if patched
 *		      -1 for fatal error
 *             1  not patched
 */

int
put_patched_pkg(int prog_flag, char *pkginst, char *pkginfo, char *ir,
		genericdb *gdb)
{
	int ret = 0; int i = 0;
	int installed = 0;
	char param[256], *value;
	char *patchlist = NULL;
	char *patch = NULL;
	char *bo = NULL;
	char **patch_arr;
	char *pkg = NULL;
	char *inst_pkgs = NULL;
	char patch_info[PATH_MAX];
	FILE *fp;
	struct dstr pkgs;

	if (pkginst == NULL || pkginfo == NULL)
		return (-1);

	if (ir == NULL)
		ir = strdup("/");

	if (access(pkginfo, R_OK) != 0) {
		progerr(gettext("could not access [%s]\n"), pkginfo);
		return (-1);
	} else {
		if ((fp = fopen(pkginfo, "r")) == NULL) {
			progerr(gettext("could not open [%s]: %s\n"), pkginfo,
				strerror(errno));
			return (-1);
		}
	}

	if ((patch_arr = (char **)calloc(PATH_MAX,
			sizeof (char *))) == NULL) {
		fclose(fp);
		return (-1);
	}

	strcpy(param, "PATCHLIST");

	while (value = fpkgparam(fp, param)) {
		if (strcmp(param, "PATCHLIST") == 0) {
			patchlist = value;
			break;
		}
		param[0] = '\0';
	}

	if (patchlist == NULL) {
		fclose(fp);
		return (1);
	}

	patch = strtok(patchlist, " \t");

	if (patch == NULL) {
		fclose(fp);
		return (1);
	}

	do {
		patch_arr[i++] = patch;
	} while ((patch = strtok(NULL, " \t")) != NULL);
	patch_arr[i] = NULL;

	init_dstr(&pkgs);

	param[0] = '\0';

	for (i = 0; patch_arr[i]; i++) {
		while (value = fpkgparam(fp, param)) {
			if (snprintf(patch_info, PATH_MAX, "PATCH_INFO_%s",
					patch_arr[i]) >= PATH_MAX)
				return (-1);
			if (strcmp(param, patch_info) == 0) {
				if ((ret = get_pkgs_from_patch_db(
						ir, patch_arr[i],
						&inst_pkgs)) == -1) {
					return (-1);
				} else if (ret == 0) {
					/* There must be at least one package */
					if ((pkg = strtok(inst_pkgs,
						DB_PKG_SEPARATOR)) == NULL)
						return (-1);

					do {
						if (strcmp(pkg, pkginst) == 0 &&
							strlen(pkginst) ==
							strlen(pkg)) {
							installed++;
							break;
						} else {
							append_dstr(&pkgs, pkg);
							append_dstr(&pkgs,
							DB_PKG_SEPARATOR);
						}
					} while ((pkg = strtok(NULL,
						DB_PKG_SEPARATOR))  != NULL);

					if (!installed) {
						append_dstr(&pkgs, pkginst);
						if (!prog_flag) {
						(void) get_fs_bo_dir(
							ir, patch_arr[i],
							pkginst, &bo);
						}
						if (parse_patchinfo(
							value, pkgs.pc,
							patch_arr[i],
							bo ? bo : NULL, gdb)) {
							return (-1);
						}
					}
					init_dstr(&pkgs);
					free(inst_pkgs);
					inst_pkgs = NULL;
				/* Insert entire new patch record */
				} else {
					if (!prog_flag) {
						(void) get_fs_bo_dir(
							ir, patch_arr[i],
							pkginst, &bo);
					}
					if (parse_patchinfo(
						value, pkginst,
						patch_arr[i],
						bo, gdb)) {
						return (-1);
					}
				}
				param[0] = '\0';
				break;
			}
		}
	}

	fclose(fp);
	free_dstr(&pkgs);
	free(patch_arr);

	return (0);
}

int
get_fs_bo_dir(char *ir, char *patch, char *pkginst, char **bo)
{
	char patchdir[PATH_MAX];
	char pkginfo_file[PATH_MAX];
	char *bdir;
	char *p;
	int ret;
	DIR *dfp;
	struct dirent *dp;

	*bo = NULL;

	if (ir == NULL || patch == NULL || pkginst == NULL)
		return (-1);

	if (snprintf(pkginfo_file, PATH_MAX, "%s" VSADMDIR "/pkg/%s/pkginfo",
			ir, pkginst) >= PATH_MAX)
		return (-1);

	if (access(pkginfo_file, R_OK) != 0)
		return (1);

	if (snprintf(patchdir, PATH_MAX, "%s" VSADMDIR "/pkg/%s/save/%s",
			ir, pkginst, patch) >= PATH_MAX)
		return (-1);

	if (dfp = opendir(patchdir)) {
		while (dp = readdir(dfp)) {
			if (dp->d_name[0] == '.')
				continue;
			if ((ret = find_file(dp->d_name, "remote")) == 1) {
				if ((bdir = parse_remote(target_path))
						!= NULL) {
					if ((p = strstr(bdir, "/undo.Z"))
							!= NULL) {
						bdir[p - bdir] = '\0';
						*bo = bdir;
					} else {
						*bo = target_path;
					}
					break;
				}
			} else if ((ret = find_file(dp->d_name, "undo.Z"))
					== 1) {
				if ((p = strstr(target_path, "/undo.Z"))
						!= NULL) {
					target_path[p - target_path] = '\0';
					*bo = target_path;
				} else {
					*bo = target_path;
				}
				break;
			}
		}
	}

	return (0);
}

/*
 * Find a file name from any absolute directory.
 *
 * Parameters: path - Path to the directory to start searching from.
 *
 *             target - the filename to search for
 *
 * Returns:    1 for found.
 *             0 for not found.
 */

static int
find_file(char *path, char *target)
{
	set_target_file(target);

	if (opendir(path) == NULL)
		return (0);

	if (nftw(path, is_file_found, 200, FTW_PHYS) == 0) {
		/* No file found */
		return (0);
	} else {
		return (1);
	}
}

static int
is_file_found(const char *path, const struct stat *sb, int type,
		struct FTW *state)
{
	char *p;

	if (type == FTW_F) {
		/* Strip file name from path */
		if ((p = strrchr(path, '/')) != NULL) {
			p++;
			if (strncmp(p, get_target_file(),
					strlen(get_target_file())) == 0) {
				target_path = strdup(path);
				return (1);
			}
		}
	}
	return (0);
}

static char *
parse_remote(char *path)
{
	FILE *fp;
	char *buf;

	if ((fp = fopen(path, "r")) == NULL) {
		return (NULL);
	}

	if ((buf = (char *)malloc(PATH_MAX)) == NULL)
		return (NULL);

	while (fgets(buf, PATH_MAX, fp) != NULL) {
		if (strncmp(buf, "FIND_AT", 7) == 0) {
			fclose(fp);
			return (strdup(buf + 8));
		}
	}

	fclose(fp);
	return (NULL);
}

/*
 * Insert the patch information into the patch_table after stripping
 * the patchid from the path.
 */

#ifdef SQLDB_IS_USED
int
insert_backout_patch_db(char *bodir, char *ir, genericdb *gdb)
{
	char *p;
	char *p2;
	char *patch = NULL;
	char *sql = "INSERT OR REPLACE INTO patch_table (patch, backout) "\
			"VALUES ('%s', '%s');";
	char sql_str[SQL_ENTRY_MAX];
	genericdb_result gdbr;

	if (bodir == NULL)
		return (-1);

	/*
	 * Need to extract the patchid out of the bodir.
	 * A patchid can consist of any format so instead of checking for
	 * format of the patchid we backup to where it must reside in the
	 * directory path. i.e. /tmp/111111-01/SUNWjunk/undo.Z\n
	 */

	p = bodir;
	while (*p != '\0') {
		while (*p != '-' && *p != '\0') p++;

		if (*p == '\0')
			return (-1);
		/*
		 * The '-' must be followed by digits and a slash to
		 * distinguish it from a random dash.
		 */

		if (!isdigit(p[1]) && !isdigit(p[2]) && p[3] != '/') {
			p++;
			continue;
		}

		p2 = p;

		while (*p != '/') p--; p++;
		while (*p2 != '/') p2++;

		patch = strdup(p);
		patch[p2 - p] = '\0';
		break;
	}

	/* Chop off the '\n'" */

	if ((p = strchr(bodir, '\n')) == 0)
		bodir[p - bodir] = '\0';

	/*
	 * The patch info must have already been added in order to get here
	 * so we can just insert the backout directory
	 */

	if (snprintf(sql_str, SQL_ENTRY_MAX, sql, patch, bodir)
			>= SQL_ENTRY_MAX)
		return (-1);

	free(patch);

	if (genericdb_querySQL(gdb, sql_str, &gdbr) != genericdb_OK)
		return (-1);

	return (0);
}
#endif

char *
get_target_file()
{
	if (target_file != NULL)
		return (target_file);
	else
		return (NULL);
}

int
set_target_file(char *target)
{
	if (target != NULL)
		target_file = target;
	else
		return (1);

	return (0);
}

/*
 * This function inserts the patch information from installed pkginfo
 * files into an SQL DB.
 *
 * Parameters:
 *             ir - The installation root.
 *
 *             gdb - A genericdb pointer to the SQL DB.
 *
 * Returns:
 *             0  Success
 *            -1 for fatal error
 */

int
put_patch_info_old(char *ir, genericdb *gdb)
{
	DIR *dfp;
	struct dirent *dp;
	char pkgdir[PATH_MAX];
	char pkginfo_file[PATH_MAX];

	if (ir == NULL)
		return (-1);

	if (snprintf(pkgdir, PATH_MAX, "%s" VSADMDIR "/pkg", ir) >= PATH_MAX)
		return (-1);

	if (dfp = opendir(pkgdir)) {
		while (dp = readdir(dfp)) {
			if (dp->d_name[0] == '.')
				continue;
			if (snprintf(pkginfo_file, PATH_MAX, "%s/%s/pkginfo",
					pkgdir, dp->d_name) >= PATH_MAX)
				return (-1);
			if (access(pkginfo_file, R_OK) != 0)
				continue;
			if (put_patched_pkg(0, dp->d_name, pkginfo_file,
					ir, gdb) == -1) {
				return (-1);
			}
		}
	}

	return (0);
}

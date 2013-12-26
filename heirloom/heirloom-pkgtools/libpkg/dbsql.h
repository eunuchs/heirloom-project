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

#ifndef _DBSQL_H
#define	_DBSQL_H

/*	from OpenSolaris "dbsql.h	1.6	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dbsql.h	1.2 (gritter) 2/24/07
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "genericdb.h"

extern int get_SQL_entries(struct cfextra **, char *);
extern int create_SQL_db(char *);
extern int get_db_entries(void);
extern int pmapentcmp(const void *, const void *);
extern int get_SQL_db(void);
extern int get_SQL_entry(struct cfent *, char *, int *);
extern char *get_SQL_db_path(void);
extern int set_SQL_db_path(char *);
extern int get_SQL_db(void);
extern int putSQL(struct cfent *, int, char *);
extern int putSQL_commit(void);
extern int putSQL_delete(struct cfent *, char *);
extern int set_SQL_db(char *);
extern int get_pkg_db(char *, char *, genericdb *);
extern long get_pkg_db_rowcount(char *, char *, genericdb *);
extern char *strip_ir(char *);
extern int putSQL_delete_commit(void);
extern void init_dstr(struct dstr *);
extern void free_dstr(struct dstr *);
extern int append_dstr(struct dstr *, const char *);
extern int get_partial_path_db(char *);
extern int append_dstr(struct dstr *, const char *);
extern int get_path_db(char *, genericdb *);
extern void free_mem_ept(int);
extern genericdb_Error query_db(char *, char *, genericdb_result *, int);
extern int db_open(char *, genericdb **, int);
extern int get_patch_entries(genericdb_result);
extern int get_patch_pkg(genericdb_result, char **);
extern int get_backout_dir(genericdb_result);
extern int put_patch_info_db(char *, char *, char *, char *, char *,
		char *, char *, char *, char *, char *, int, int, int,
		genericdb *);
extern int get_pkgs_from_patch_db(char *, char *, char **);
extern int parse_patchinfo(char *, char *, char *, char *, genericdb *);
extern int put_patch_commit(char *, char *);
extern int putSQL_str(struct dstr *);
extern int putSQL_str_del(struct dstr *);
extern int put_patch_info_old(char *, genericdb *);
extern int put_patched_pkg(int, char *, char *, char *, genericdb *);
extern int convert_pkginfo_to_sql(struct dstr *, FILE *, const char *);
extern struct cfent **get_eptlist(void);
extern int get_eptnum(void);

#define	DB_PKG_SEPARATOR	" "
#define	DB_CLASS_SEPARATOR	":"
#define	DB_SEPARATOR	"|"
#define	DB	"install.db"
#define	EOL	"\r\n"

#define	R_MODE 0400
#define	W_MODE 0600

#define	SQL_CREATE_INDEX_PATH "CREATE UNIQUE INDEX p1 on pkg_table (path) " \
		"ON CONFLICT REPLACE;"
#define	SQL_CREATE_INDEX_PKGS "CREATE INDEX p2 on pkg_table (pkgs) " \
		"ON CONFLICT REPLACE;"
#define	SQL_CREATE_INDEX_PATCH "CREATE UNIQUE INDEX p3 on patch_table " \
		"(patch) ON CONFLICT REPLACE;"
#define	SQL_CREATE_INDEX_PATCH_CNTS "CREATE UNIQUE index p4 on " \
		"patch_contents_table (patch) ON CONFLICT REPLACE;"
#define	SQL_CREATE_INDEX_PATCH_PKG "CREATE UNIQUE index p5 on " \
		"patch_pkg_table (patch, pkg) ON CONFLICT REPLACE;"

#define	SQL_SELECT "SELECT * FROM pkg_table WHERE path=\'"
#define	SQL_SELECT_ALL "SELECT * FROM pkg_table"
#define	SQL_SELECT_ALL_COUNT "SELECT count() FROM pkg_table"
#define	SQL_SELECT_ALL_PATCH "SELECT * FROM patch_table"
#define	SQL_SELECT_PATCH "SELECT patch, obs, reqs, incs, pkgs FROM patch_table"
#define	SQL_SELECT_BODIR "SELECT backout FROM patch_table WHERE patch=\'"
#define	SQL_SELECT_PATH "SELECT path FROM pkg_table WHERE path=\'"
#define	SQL_SELECT_PKGS "SELECT * FROM pkg_table WHERE pkgs LIKE"
#define	SQL_SELECT_PKGS_GLOB "SELECT * FROM pkg_table WHERE pkgs GLOB"
#define	SQL_SELECT_PKGS_COUNT "SELECT count() FROM pkg_table WHERE pkgs LIKE"
#define	SQL_SELECT_PPATH "SELECT * FROM pkg_table WHERE path LIKE"
#define	SQL_SELECT_PATCH_PKG "SELECT patch FROM patch_table WHERE pkgs LIKE"
#define	SQL_SELECT_PKG_PATCH "SELECT pkgs FROM patch_table WHERE patch=\'"
#define	SQL_NOT_GLOB "AND pkgs NOT GLOB"
#define	SQL_GLOB "OR pkgs GLOB"
#define	UPPER_ALPHA "[A-Z]"
#define	LOWER_ALPHA "[a-z]"
#define	NUMERIC "[0-9]"
#define	HYPHEN "-"
#define	SQL_WILD "%"

#define	STATUS_INDICATORS "[*+!%@#~-]"
#define	COLON_SPACE "[ :]"
#define	GLOB_WILD "*"

#define	SQL_STATUS "SELECT * FROM pkg_table WHERE pkgstatus=\'1\'"
#define	SQL_STATUS_COUNT "SELECT count() FROM pkg_table WHERE pkgstatus=\'1\'"
#define	SQL_STATUS_PKG "AND pkgs LIKE"

#define	SQL_AND_PATH "AND path=\'"
#define	SQL_ORDER_LNGTH "ORDER BY length(path);"
#define	SQL_INS "INSERT or REPLACE into pkg_table values("
#define	SQL_INS_PATCH "INSERT or REPLACE into patch_table values("
#define	SQL_DEL "DELETE FROM pkg_table WHERE path=\'"
#define	SQL_DEL_PATCH "DELETE FROM patch_table WHERE patch=\'"
#define	SQL_UPD_PATCH "UPDATE patch_table SET pkgs=\'"
#define	SQL_UPDT "UPDATE pkg_table SET pkgs=\'"
#define	SQL_WHERE_PATH "WHERE path=\'"
#define	SQL_WHERE_PATCH "WHERE patch=\'"
#define	SQL_WHERE_PATH_OR " OR path=\'"
#define	SQL_WHERE_PATH_LIKE " OR path LIKE \'"
#define	SQL_WHERE_PKG "WHERE pkgs=\'"
#define	SQL_TRL ");"
#define	SQL_TRL_MOD "\';"
#define	SQL_EQ_WILD "=%"
#define	SQL_APOST "\'"

#define	TRANS_INS "BEGIN TRANSACTION insert_record;"
#define	TRANS_SEL "BEGIN TRANSACTION select_record;"
#define	TRANS_SEL_TRL "END TRANSACTION select_record;"
#define	TRANS_INS_TRL "COMMIT TRANSACTION insert_record;"
#define	TRANS_MOD "BEGIN TRANSACTION modify_record ON CONFLICT ROLLBACK;"
#define	TRANS_MOD_TRL "COMMIT TRANSACTION modify_record;"

#define	SQL_LINE_LGTH 80
#define	MAX_PKGMAP_LINE 1024
#define	SQL_ENTRY_MAX 7500
#define	SQL_PKG_MAX 4200

#define	APPEND_INCR 65536

#ifdef __cplusplus
}
#endif

#endif /* _DBSQL_H */

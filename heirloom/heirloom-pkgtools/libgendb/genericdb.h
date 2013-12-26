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

/*
 * genericdb.h
 *
 * Minimal Generic database API.
 *
 * PLEASE NOTE:  API documentation is included in genericdb.c above the
 * relevant functions.
 *
 * Version: 1.2
 * Date:    02/27/06
 */

#ifndef __GENERICDB__
#define	__GENERICDB__

/*	from OpenSolaris "genericdb.h	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)genericdb.h	1.3 (gritter) 2/24/07
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	B_TRUE
typedef enum { _B_FALSE, _B_TRUE } boolean_t;
#define	B_TRUE	_B_TRUE
#define	B_FALSE	_B_FALSE
#endif

/* Error Codes */
#define	genericdb_OK		0
#define	genericdb_NOMEM		1	/* Occurs when malloc fails. */
#define	genericdb_SQLERR	2	/* SQL given fails on submission. */
#define	genericdb_ACCESS	3	/* Cannot access the SQL db at all. */
#define	genericdb_PERMS		4	/* The requested perms can't be set. */
#define	genericdb_PARAMS	5	/* Bad parameters passed in on call. */
#define	genericdb_NULL		6	/* A requested result value was NULL */

/* Common use modes */
#define	GENERICDB_WR_MODE	0644
#define	GENERICDB_R_MODE	0444

typedef int genericdb_Error;

/*
 * It is recommended that a genericdb_result be initialized to 0 using
 * the following sized definition.  This is coupled with the internal
 * definition in genericdb_private.h which cannot be imported here.
 *
 * Use it like this:
 * {
 *	genericdb_result r;
 *	(void) memset(&r, 0, GENERICDB_RESULT_SIZE);
 *	... more code which may or may not set the result...
 *	genericdb_result_free(&r);
 * }
 *
 * Note to user: By initializing the buffer in this way, it is always
 * safe to perform genericdb_result_free().  Otherwise, despite there
 * being sanity checks, there is always the chance that an uninitialized
 * pointer will be passed to free(), causing a crash.
 *
 * Note to maintainer:  zeroing the first two fields is enough.  All entry
 * points which evaluate a genericdb_result check the version and type field
 * are set before proceeding.
 */
#define	GENERICDB_RESULT_SIZE	(2 * sizeof (int))

/*
 * genericdb_exists requires special handling.  One cannot determine whether
 * a db exists merely by the presense of the db file.  It could be that
 * the file was created through the use of genericdb_open, but it was never
 * correctly populated with data.  For this reason, genericdb_exists uses
 * 3 additional strings.  These are 'baked into' genericdb_exists
 * though genericdb is supposed to be generic with respect to the underlying
 * database and also, at least in principle, support other applications than the
 * install database.  In the future, 'parameters' passed into genericdb_open
 * or genericdb_setting will be used to reset these strings for other dbs.
 *
 *   GENERICDB_INSTALL_DB_STATUS_TABLE   (sets up the install_db_status_table.
 *      the initial value for the status will be 'not ok.')
 *   GENERICDB_INSTALL_DB_STATUS_CHECK   (checks to see if install has
 *      completed to the extent the database can be said to exist)
 *   GENERICDB_INSTALL_DB_STATUS_OK_VAL  (the value corresponding to OK)
 *
 * Software creating a transaction which completes database creation should
 * include the following SQL string in the transaction:
 *   GENERICDB_INSTALL_DB_STATUS_SET_OK  (sets the value is now OK)
 *
 * Future software other than the install database which uses this interface
 * should define similar constants.  These do not necessarily have to be
 * defined in this header file.
 *
 * For discussion of how to call genericdb_exists, please see the comments
 * in the header file.  These comments pertain to the constants defined
 * below.
 */

#define	GENERICDB_INSTALL_DB_CREATE_TABLE	\
"CREATE TABLE reg4_admin_table (" \
"product CHAR(256) NOT NULL PRIMARY KEY ON CONFLICT REPLACE, "\
"version CHAR(256) NOT NULL);\n"

#define	GENERICDB_INSTALL_DB_STATUS_TABLE	\
"BEGIN TRANSACTION;\n" \
GENERICDB_INSTALL_DB_CREATE_TABLE \
"INSERT INTO reg4_admin_table (product, version) VALUES('reg4', '0.0');\n" \
"COMMIT;\n"

#define	GENERICDB_INSTALL_DB_STATUS_CHECK	\
"SELECT version FROM reg4_admin_table WHERE product='reg4';\n"

#define	GENERICDB_INSTALL_DB_STATUS_OK_VAL	\
"1.0"

#define	GENERICDB_INSTALL_DB_STATUS_SET_OK	\
"INSERT INTO reg4_admin_table (product, version) VALUES('reg4', '1.0');\n"

/*
 * The implementation of the generic db will cast the handle to
 * an internally defined private data structure.
 */
typedef void * genericdb;

boolean_t genericdb_exists(const char *);

genericdb * genericdb_open(const char *, int, int, char *[], genericdb_Error *);

void genericdb_close(genericdb *);

genericdb_Error genericdb_setting(genericdb *, int, char *[]);

genericdb_Error genericdb_submitSQL(genericdb *, const char *);

/*
 * The internal definition of the result type is private and encapsulates
 * the returned data.  Accessors are needed to obtain and free the result.
 */
typedef void * genericdb_result;

/*
 * Only generic_TABLE is currently defined, which is a 2d array of values.
 */
typedef enum { genericdb_UNKNOWN = 0, genericdb_TABLE = 1 }
    genericdb_resulttype;

genericdb_resulttype genericdb_result_type(const genericdb_result);

int genericdb_result_table_nrows(const genericdb_result);

int genericdb_result_table_ncols(const genericdb_result);

genericdb_Error genericdb_result_table_str(const genericdb_result,
    int, int, char **);

genericdb_Error genericdb_result_table_int(const genericdb_result,
    int, int, int *);

int genericdb_result_table_null(const genericdb_result, int, int);

void genericdb_result_free(genericdb_result);

genericdb_Error genericdb_querySQL(genericdb *, const char *,
    genericdb_result *);

char *genericdb_errstr(genericdb_Error);

genericdb_Error genericdb_remove_db(const char *);

genericdb_Error genericdb_remove_save(const char *);

long genericdb_db_size(const char *);

#ifdef __cplusplus
}
#endif

#endif	/* __GENERICDB__ */

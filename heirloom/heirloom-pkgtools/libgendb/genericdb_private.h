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
 * genericdb_private.h
 *
 * Private Definitions For A Minimal Generic database API.
 * Specific wrappers for sqlite are included here.  This is a very simple
 * from of portable database interface.
 *
 * Version: 1.1
 * Date:    05/06/03
 */

#ifndef	__GENERICDBPRIV__
#define	__GENERICDBPRIV__

/*	from OpenSolaris "genericdb_private.h	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)genericdb_private.h	1.5 (gritter) 2/24/07
 */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * This structure is used only internally by genericdb code.
 * It contains all that is necessary for a session, opened with
 * genericdb_open().
 */
typedef struct genericdb_private_ {

	sqlite	*psql;
	char	*debug;
	int	log;

} genericdb_private;

/*
 * This structure contains the opaque information which encapsualtes a
 * result returned by genericdb_querySQL().  It must be accessed using
 * the helper functions in the genericdb.h API.
 *
 * If new result types are supported, all that is necessary is adding
 * additional accessor functions to the API and enough internal state
 * to support this information in genericdb_result_private.
 *
 * MAINTAINER NOTE:
 *
 *   If the structure genericdb_result_private changes, a corresponding
 *   change should be made in GENERICDB_RESLT_SIZE in genericdb.h
 *
 * IMPLEMENTOR NOTE:
 * Layout of the results -
 *    ppc[0]    = result [0][0]
 *    ppc[1]    = result [0][1]
 *    ppc[nrow] = result [1][0]
 * Example nrow = 3, ncol = 3
 *    ppc[0]    = result [0][0]
 *    ppc[1]    = result [0][1]
 *    ppc[2]    = result [0][2]
 *    ppc[3]    = result [1][0]
 *    ppc[4]    = result [1][1]
 *    ppc[5]    = result [1][2]
 *    ppc[6]    = result [2][0]
 *    ppc[7]    = result [2][1]
 *    ppc[8]    = result [2][2]
 */
typedef struct genericdb_result_private_ {
	int 			version; /* 1 */
	genericdb_resulttype 	type; /* genericdb_TABLE */
	char 			**ppc;
	int 			nrow;
	int 			ncol;
} genericdb_result_private;

#define	INSTALLDB_PATH	VSADMREL "/install/install.db"

#ifdef	__cplusplus
}
#endif

#endif	/* __GENERICDBPRIV__ */

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

#ifndef __WSREG_PKGRM__
#define	__WSREG_PKGRM__

/*	from OpenSolaris "wsreg_pkgrm.h	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)wsreg_pkgrm.h	1.3 (gritter) 2/24/07
 */

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#ifdef __sun
#include <wsreg.h>
#endif
#include <fcntl.h>

int wsreg_pkgrm_check(const char *, const char *, char ***, char ***);

#ifdef	__cplusplus
}
#endif

#endif	/* __WSREG_PKGRM__ */

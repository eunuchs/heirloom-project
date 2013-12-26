/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 1996 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)libmksdmsi18n_init.cc 1.5 06/12/12
 */

/*	from OpenSolaris "libmksdmsi18n_init.cc	1.5	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)libmksdmsi18n_init.cc	1.5 (gritter) 01/13/07
 */

#include <avo/intl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef I18N_VERSION
#define	I18N_VERSION	0.1
#endif

/*
 * Open the catalog file for libmksdmsi18n.  Users of this library must set
 * NSLPATH first.  See avo_18n_init().
 */
int
libmksdmsi18n_init()
{
	return 0;
}
 
/*
 * Close the catalog file for libmksdmsi18n
 */
void
libmksdmsi18n_fini()
{
}


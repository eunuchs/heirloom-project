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
 * @(#)mksdmsi18n.h 1.3 06/12/12
 */

/*	from OpenSolaris "mksdmsi18n.h	1.3	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)mksdmsi18n.h	1.2 (gritter) 01/07/07
 */

#ifndef _AVO_MKSDMSI18N_H
#define _AVO_MKSDMSI18N_H

#ifndef _AVO_INTL_H
#include <avo/intl.h>
#endif

extern	nl_catd	libmksdmsi18n_catd;

int	libmksdmsi18n_init();
void	libmksdmsi18n_fini();

#endif


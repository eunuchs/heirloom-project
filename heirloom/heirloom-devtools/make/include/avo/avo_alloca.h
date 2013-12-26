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
 * Copyright 1998 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)avo_alloca.h 1.4 06/12/12
 */

/*	from OpenSolaris "avo_alloca.h 1.4     06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)avo_alloca.h	1.4 (gritter) 01/21/07
 */

#ifndef _AVO_ALLOCA_H
#define _AVO_ALLOCA_H

#if defined (__sun) || defined (__linux__) || defined (__hpux)
#include <alloca.h>
#endif

#ifdef __SunOS_5_4
// The following prototype declaration is necessary when compiling on Solaris
// 2.4 using 5.0 compilers. On Solaris 2.4 the necessary prototypes are not
// included in alloca.h. The 4.x compilers provide a workaround by declaring the
// prototype as a pre-defined type. The 5.0 compilers do not implement this workaround.
// This can be removed when support for 2.4 is dropped

#include <stdlib.h> 	// for size_t
extern "C" void *__builtin_alloca(size_t);

#endif  // ifdef __SunOS_5_4

#endif  // ifdef _AVO_ALLOCA_H


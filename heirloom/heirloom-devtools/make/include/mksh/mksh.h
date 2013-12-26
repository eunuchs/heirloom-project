#ifndef _MKSH_MKSH_H
#define _MKSH_MKSH_H
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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)mksh.h 1.7 06/12/12
 */

/*	from OpenSolaris "mksh.h	1.7	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)mksh.h	1.2 (gritter) 01/07/07
 */

/*
 * Included files
 */
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
#	include <dm/Avo_DmakeCommand.h>
#endif

#include <mksh/defs.h>
#include <unistd.h>

#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */

extern int	do_job(Avo_DmakeCommand *cmd_list[], char *env_list[], char *stdout_file, char *stderr_file, char *cwd, char *cnwd, int ignore, int silent, pathpt vroot_path, char *shell, int nice_prio);

#endif /* TEAMWARE_MAKE_CMN */

#endif

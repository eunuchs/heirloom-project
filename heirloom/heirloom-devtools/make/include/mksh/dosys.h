#ifndef _MKSH_DOSYS_H
#define _MKSH_DOSYS_H
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
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)dosys.h 1.9 06/12/12
 */

/*	from OpenSolaris "dosys.h	1.9	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dosys.h	1.2 (gritter) 01/07/07
 */

#include <mksh/defs.h>
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
#	include <rw/xdrstrea.h>
#endif
#include <vroot/vroot.h>

#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
extern Boolean	await(register Boolean ignore_error, register Boolean silent_error, Name target, wchar_t *command, pid_t running_pid, Boolean send_mtool_msgs, XDR *xdrs, int job_msg_id);
#else
extern Boolean	await(register Boolean ignore_error, register Boolean silent_error, Name target, wchar_t *command, pid_t running_pid, Boolean send_mtool_msgs, void *xdrs, int job_msg_id);
#endif
extern int	doexec(register wchar_t *command, register Boolean ignore_error, Boolean redirect_out_err, char *stdout_file, char *stderr_file, pathpt vroot_path, int nice_prio);
extern int	doshell(wchar_t *command, register Boolean ignore_error, Boolean redirect_out_err, char *stdout_file, char *stderr_file, int nice_prio);
extern Doname	dosys_mksh(register Name command, register Boolean ignore_error, register Boolean call_make, Boolean silent_error, Boolean always_exec, Name target, Boolean redirect_out_err, char *stdout_file, char *stderr_file, pathpt vroot_path, int nice_prio);
extern void	redirect_io(char *stdout_file, char *stderr_file);
extern void	sh_command2string(register String command, register String destination);

#endif

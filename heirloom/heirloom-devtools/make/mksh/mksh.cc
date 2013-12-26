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
 * @(#)mksh.cc 1.22 06/12/12
 */

/*	from OpenSolaris "mksh.cc	1.22	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)mksh.cc	1.4 (gritter) 01/13/07
 */

/*
 *	mksh.cc
 *
 *	Execute the command(s) of one Make or DMake rule
 */

/*
 * Included files
 */
#if defined(TEAMWARE_MAKE_CMN) || defined(MAKETOOL) /* tolik */
#	include <avo/util.h>
#endif

#include <mksh/dosys.h>		/* redirect_io() */
#include <mksh/misc.h>		/* retmem() */
#include <mksh/mksh.h>
#include <mksdmsi18n/mksdmsi18n.h>
#include <errno.h>
#include <signal.h>

#ifdef HP_UX
	extern void (*sigset(int, void (*)(__harg)))(__harg);
#endif

/*
 * Workaround for NFS bug. Sometimes, when running 'chdir' on a remote
 * dmake server, it fails with "Stale NFS file handle" error.
 * The second attempt seems to work.
 */
int
my_chdir(char * dir) {
	int res = chdir(dir);
	if (res != 0 && (errno == ESTALE || errno == EAGAIN)) {
		/* Stale NFS file handle. Try again */
		res = chdir(dir);
	}
	return res;
}


#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
/*
 * File table of contents
 */
static void	change_sunpro_dependencies_value(char *oldpath, char *newpath);
static void	init_mksh_globals(char *shell);
static void	set_env_vars(char *env_list[]);

/*
 *	Execute the command(s) of one Make or DMake rule
 */
int
do_job(Avo_DmakeCommand *cmd_list[], char *env_list[], char *stdout_file, char *stderr_file, char *cwd, char *cnwd, int ignore, int silent, pathpt vroot_path, char *shell, int nice_prio)
{
	Boolean			always_exec_flag;
	char			*cmd;
	Avo_DmakeCommand	**cmd_list_p;
	Name			command;
	Boolean			do_not_exec_flag;
	Boolean			ignore_flag;
	int			length;
	Boolean			make_refd_flag;
	Boolean			meta_flag;
	char			pathname[MAXPATHLEN];
	Doname			result;
	Boolean			silent_flag;
	wchar_t			*tmp_wcs_buffer;

	if ((childPid = fork()) < 0) {	/* error */
		;
	} else if (childPid > 0) {		/* parent */
		;
	} else {			/* child, mksh */
		sigset(SIGCHLD, SIG_DFL);
		enable_interrupt(handle_interrupt_mksh);
		/* set environment variables */
		set_env_vars(env_list);
		/* redirect stdout and stderr to temp files */
		dup2(1, 2);	// Because fatal_mksh() prints error messages into
				// stderr but dmake uses stderr for XDR communications
				// and stdout for errors messages.
		redirect_io(stdout_file, stderr_file);
		/* try cd'ing to cwd */
		if (my_chdir(cwd) != 0) {
			/* try the netpath machine:pathname */
			if (!avo_netpath_to_path(cnwd, pathname)) {
				fatal_mksh("`cd %s' failed, and conversion of %s to automounter pathname also failed: %s", cwd, cnwd, strerror(errno));
			} else if (my_chdir(pathname) != 0) {
				fatal_mksh("`cd %s' and `cd %s' both failed: %s", cwd, pathname, strerror(errno));
			}
			/*
			 * change the value of SUNPRO_DEPENDENCIES
			 * to the new path.
			 */
			change_sunpro_dependencies_value(cwd, pathname);
		}
		init_mksh_globals(shell);
		for (cmd_list_p = cmd_list;
		     *cmd_list_p != (Avo_DmakeCommand *) NULL;
		     cmd_list_p++) {
			if ((*cmd_list_p)->ignore()) {
				ignore_flag = true;
			} else {
				ignore_flag = false;
			}
			if ((*cmd_list_p)->silent()) {
				silent_flag = true;
			} else {
				silent_flag = false;
			}
/*
			if ((*cmd_list_p)->always_exec()) {
				always_exec_flag = true;
			} else {
				always_exec_flag = false;
			}
 */
			always_exec_flag = false;
			if ((*cmd_list_p)->meta()) {
				meta_flag = true;
			} else {
				meta_flag = false;
			}
			if ((*cmd_list_p)->make_refd()) {
				make_refd_flag = true;
			} else {
				make_refd_flag = false;
			}
			if ((*cmd_list_p)->do_not_exec()) {
				do_not_exec_flag = true;
			} else {
				do_not_exec_flag = false;
			}
			do_not_exec_rule = do_not_exec_flag;
			cmd = (*cmd_list_p)->getCmd();
			if ((length = strlen(cmd)) >= MAXPATHLEN) {
				tmp_wcs_buffer = ALLOC_WC(length + 1);
				mbstowcs(tmp_wcs_buffer, cmd, length + 1);
				command = GETNAME(tmp_wcs_buffer, FIND_LENGTH);
				retmem(tmp_wcs_buffer);
			} else {
				MBSTOWCS(wcs_buffer, cmd);
				command = GETNAME(wcs_buffer, FIND_LENGTH);
			}
			if ((command->hash.length > 0) &&
			    (!silent_flag || do_not_exec_flag)) {
				printf("%s\n", command->string_mb);
			}
			result = dosys_mksh(command,
					    ignore_flag,
					    make_refd_flag,
					    false, /* bugs #4085164 & #4990057 */
					    /* BOOLEAN(silent_flag && ignore_flag), */
					    always_exec_flag,
					    (Name) NULL,
					    false,
					    NULL,
					    NULL,
					    vroot_path,
					    nice_prio);
			if (result == build_failed) {

#ifdef PRINT_EXIT_STATUS
				warning_mksh(NOCATGETS("I'm in do_job(), and dosys_mksh() returned result of build_failed."));
#endif

				if (silent_flag) {
					printf("The following command caused the error:\n%s\n",
						      command->string_mb);
				}
				if (!ignore_flag && !ignore) {

#ifdef PRINT_EXIT_STATUS
					warning_mksh(NOCATGETS("I'm in do_job(), and dosys_mksh() returned result of build_failed, exiting 1."));
#endif

					exit(1);
				}
			}
		}

#ifdef PRINT_EXIT_STATUS
		warning_mksh(NOCATGETS("I'm in do_job(), exiting 0."));
#endif

	        exit(0);
	}
        return childPid;
}
#endif /* TEAMWARE_MAKE_CMN */

#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
static void
set_env_vars(char *env_list[])
{
	char			**env_list_p;

	for (env_list_p = env_list;
	     *env_list_p != (char *) NULL;
	     env_list_p++) {
		putenv(*env_list_p);
	}
}

static void
init_mksh_globals(char *shell)
{
/*
	MBSTOWCS(wcs_buffer, NOCATGETS("SHELL"));
	shell_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, shell);
	SETVAR(shell_name, GETNAME(wcs_buffer, FIND_LENGTH), false);
 */
	char * dmake_shell;
	if ((dmake_shell = getenv(NOCATGETS("DMAKE_SHELL"))) == NULL) {
		dmake_shell = shell;
	} 
	MBSTOWCS(wcs_buffer, dmake_shell);
	shell_name = GETNAME(wcs_buffer, FIND_LENGTH);
}

/*
 * Change the pathname in the value of the SUNPRO_DEPENDENCIES env variable
 * from oldpath to newpath.
 */
static void
change_sunpro_dependencies_value(char *oldpath, char *newpath)
{
	char		buf[MAXPATHLEN];
	static char	*env;
	int		length;
	int		oldpathlen;
	char		*sp_dep_value;

	/* check if SUNPRO_DEPENDENCIES is set in the environment */
	if ((sp_dep_value = getenv(NOCATGETS("SUNPRO_DEPENDENCIES"))) != NULL) {
		oldpathlen = strlen(oldpath);
		/* check if oldpath is indeed in the value of SUNPRO_DEPENDENCIES */
		if (strncmp(oldpath, sp_dep_value, oldpathlen) == 0) {
			sprintf(buf,
				       "%s%s",
				       newpath,
				       sp_dep_value + oldpathlen);
			length = 2 +
				strlen(NOCATGETS("SUNPRO_DEPENDENCIES")) +
				strlen(buf);
			env = getmem(length);
			sprintf(env,
				       "%s=%s",
				       NOCATGETS("SUNPRO_DEPENDENCIES"),
				       buf);
			putenv(env);
		}
	}
}
#endif



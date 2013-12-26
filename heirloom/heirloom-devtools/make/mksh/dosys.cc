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
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)dosys.cc 1.38 06/12/12
 */

/*	from OpenSolaris "dosys.cc	1.38	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dosys.cc	1.10 (gritter) 3/7/07
 */

/*
 *	dosys.cc
 *
 *	Execute one commandline
 */

/*
 * Included files
 */
#include <sys/wait.h>			/* WIFEXITED(status) */
#include <avo/avo_alloca.h>		/* alloca() */

#if defined(TEAMWARE_MAKE_CMN) || defined(MAKETOOL) /* tolik */
#	include <avo/strings.h>	/* AVO_STRDUP() */
#if defined(DISTRIBUTED)
#	include <dm/Avo_CmdOutput.h>
#	include <rw/xdrstrea.h>
#endif
#endif

#include <stdio.h>		/* errno */
#include <errno.h>		/* errno */
#include <fcntl.h>		/* open() */
#include <mksh/dosys.h>
#include <mksh/macro.h>		/* getvar() */
#include <mksh/misc.h>		/* getmem(), fatal_mksh(), errmsg() */
#include <mksdmsi18n/mksdmsi18n.h>	/* libmksdmsi18n_init() */
#include <sys/signal.h>		/* SIG_DFL */
#include <sys/stat.h>		/* open() */
#include <sys/wait.h>		/* wait() */
#include <ulimit.h>		/* ulimit() */
#include <unistd.h>		/* close(), dup2() */

#ifndef __sun
#	include <sys/param.h>
#	include <wctype.h>
#	include <wchar.h>
#endif

#ifndef __sun
#	undef wslen
#	define wslen(x) wcslen(x)
#	undef wscpy
#	define wscpy(x,y) wcscpy(x,y)
#endif

#ifndef	O_DSYNC
#define	O_DSYNC	0
#endif

/*
 * Defined macros
 */
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
#define SEND_MTOOL_MSG(cmds) \
	if (send_mtool_msgs) { \
		cmds \
	}
#else
#define SEND_MTOOL_MSG(cmds)
#endif

/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */
static Boolean	exec_vp(register char *name, register char **argv, char **envp, register Boolean ignore_error, pathpt vroot_path);

/*
 * Workaround for NFS bug. Sometimes, when running 'open' on a remote
 * dmake server, it fails with "Stale NFS file handle" error.
 * The second attempt seems to work.
 */
int
my_open(const char *path, int oflag, mode_t mode) {
	int res = open(path, oflag, mode);
#ifdef linux
// Workaround for NFS problem: even when all directories in 'path'
// exist, 'open' (file creation) fails with ENOENT.
	int nattempt = 0;
	while (res < 0 && (errno == ESTALE || errno == EAGAIN || errno == ENOENT)) {
		nattempt++;
		if(nattempt > 30) {
			break;
		}
		sleep(1);
#else
	if (res < 0 && (errno == ESTALE || errno == EAGAIN)) {
#endif
		/* Stale NFS file handle. Try again */
		res = open(path, oflag, mode);
	}
	return res;
}

/*
 *	void
 *	redirect_io(char *stdout_file, char *stderr_file)
 *
 *	Redirects stdout and stderr for a child mksh process.
 */
void
redirect_io(char *stdout_file, char *stderr_file)
{        
	long		descriptor_limit;
	int		i;

#ifndef __sun
        /*
         *  HP-UX does not support the UL_GDESLIM command for ulimit().
	 *  NOFILE == max num open files per process (from <sys/param.h>)
         */
	descriptor_limit = NOFILE;
#else
	if ((descriptor_limit = ulimit(UL_GDESLIM)) < 0) {
		fatal_mksh("ulimit() failed: %s", errmsg(errno));
	}
#endif
	for (i = 3; i < descriptor_limit; i++) {
		close(i);
	}
	if (stdout_file != NULL) {
	    if ((i = my_open(stdout_file,
	         O_WRONLY | O_CREAT | O_TRUNC | O_DSYNC,
	         S_IREAD | S_IWRITE)) < 0) {
		fatal_mksh("Couldn't open standard out temp file `%s': %s",
		      stdout_file,
		      errmsg(errno));
	    } else {
		if (dup2(i, 1) == -1) {
			fatal_mksh(NOCATGETS("*** Error: dup2(3, 1) failed: %s"),
				errmsg(errno));
		}
		close(i);
	    }
	}
	if (stderr_file == NULL) {
		if (stdout_file != NULL && dup2(1, 2) == -1) {
			fatal_mksh(NOCATGETS("*** Error: dup2(1, 2) failed: %s"),
				errmsg(errno));
		}
	} else if ((i = my_open(stderr_file,
	                O_WRONLY | O_CREAT | O_TRUNC | O_DSYNC,
	                S_IREAD | S_IWRITE)) < 0) {
		fatal_mksh("Couldn't open standard error temp file `%s': %s",
		      stderr_file,
		      errmsg(errno));
	} else {
		if (dup2(i, 2) == -1) {
			fatal_mksh(NOCATGETS("*** Error: dup2(3, 2) failed: %s"),
				errmsg(errno));
		}
		close(i);
	}
}

/*
 *	dosys_mksh(command, ignore_error, call_make, silent_error, target)
 *
 *	Check if command string contains meta chars and dispatch to
 *	the proper routine for executing one command line.
 *
 *	Return value:
 *				Indicates if the command execution failed
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should we abort when an error is seen?
 *		call_make	Did command reference $(MAKE) ?
 *		silent_error	Should error messages be suppressed for dmake?
 *		target		Target we are building
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		working_on_targets We started processing real targets
 */
Doname
dosys_mksh(register Name command, register Boolean ignore_error, register Boolean call_make, Boolean silent_error, Boolean always_exec, Name target, Boolean redirect_out_err, char *stdout_file, char *stderr_file, pathpt vroot_path, int nice_prio)
{
	register int		length = command->hash.length;
	register wchar_t	*p;
	register wchar_t	*q;
	register wchar_t	*cmd_string;
	//struct stat		before;
	Doname			result;
	Boolean			working_on_targets_mksh = true;
	Wstring wcb(command);
	p = wcb.get_string();
	cmd_string = p;

	/* Strip spaces from head of command string */
	while (iswspace(*p)) {
		p++, length--;
	}
	if (*p == (int) nul_char) {
		return build_failed;
	}
	/* If we are faking it we just return */
	if (do_not_exec_rule &&
	    working_on_targets_mksh &&
	    !call_make &&
	    !always_exec) {
		return build_ok;
	}

	/* Copy string to make it OK to write it. */
	q = ALLOC_WC(length + 1);
	wscpy(q, p);
	/* Write the state file iff this command uses make. */
/* XXX - currently does not support recursive make's, $(MAKE)'s
	if (call_make && command_changed) {
		write_state_file(0, false);
	}
	stat(make_state->string_mb, &before);
 */
	/*
	 * Run command directly if it contains no shell meta chars,
	 * else run it using the shell.
	 */
	/* XXX - command->meta *may* not be set correctly */
	if (await(ignore_error,
		  silent_error,
		  target,
		  cmd_string,
                  command->meta ?
	            doshell(q, ignore_error, redirect_out_err, stdout_file, stderr_file, nice_prio) :
	            doexec(q, ignore_error, redirect_out_err, stdout_file, stderr_file, vroot_path, nice_prio),
	          false,
	          NULL,
	          -1)) {

#ifdef PRINT_EXIT_STATUS
		warning_mksh(NOCATGETS("I'm in dosys_mksh(), and await() returned result of build_ok."));
#endif

		result = build_ok;
	} else {

#ifdef PRINT_EXIT_STATUS
		warning_mksh(NOCATGETS("I'm in dosys_mksh(), and await() returned result of build_failed."));
#endif

		result = build_failed;
	}
	retmem(q);

/* XXX - currently does not support recursive make's, $(MAKE)'s
	if ((report_dependencies_level == 0) &&
	    call_make) {
		make_state->stat.time = (time_t)file_no_time;
		exists(make_state);
		if (before.st_mtime == make_state->stat.time) {
			return result;
		}
		makefile_type = reading_statefile;
		if (read_trace_level > 1) {
			trace_reader = true;
		}
		read_simple_file(make_state,
					false,
					false,
					false,
					false,
					false,
					true);
		trace_reader = false;
	}
 */
	return result;
}

/*
 *	doshell(command, ignore_error)
 *
 *	Used to run command lines that include shell meta-characters.
 *	The make macro SHELL is supposed to contain a path to the shell.
 *
 *	Return value:
 *				The pid of the process we started
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		filter_stderr	If -X is on we redirect stderr
 *		shell_name	The Name "SHELL", used to get the path to shell
 */
int
doshell(wchar_t *command, register Boolean ignore_error, Boolean redirect_out_err, char *stdout_file, char *stderr_file, int nice_prio)
{
	char			*argv[6];
	int			argv_index = 0;
	int			cmd_argv_index;
	int			length;
	char			nice_prio_buf[MAXPATHLEN];
	register Name		shell = getvar(shell_name);
	register char		*shellname;
	char			*tmp_mbs_buffer;


	if (IS_EQUAL(shell->string_mb, "")) {
		shell = shell_name;
	}
	if ((shellname = strrchr(shell->string_mb, (int) slash_char)) == NULL) {
		shellname = shell->string_mb;
	} else {
		shellname++;
	}

	/*
	 * Only prepend the /usr/bin/nice command to the original command
	 * if the nice priority, nice_prio, is NOT zero (0).
	 * Nice priorities can be a positive or a negative number.
	 */
	if (nice_prio != 0) {
		argv[argv_index++] = NOCATGETS("nice");
		sprintf(nice_prio_buf, NOCATGETS("-%d"), nice_prio);
		argv[argv_index++] = strdup(nice_prio_buf);
	}
	argv[argv_index++] = shellname;
	argv[argv_index++] = (char*)(ignore_error ? NOCATGETS("-c") : NOCATGETS("-ce"));
	if ((length = wslen(command)) >= MAXPATHLEN) {
		tmp_mbs_buffer = getmem((length * MB_LEN_MAX) + 1);
                wcstombs(tmp_mbs_buffer, command, (length * MB_LEN_MAX) + 1);
		cmd_argv_index = argv_index;
                argv[argv_index++] = strdup(tmp_mbs_buffer);
                retmem_mb(tmp_mbs_buffer);
	} else {
		WCSTOMBS(mbs_buffer, command);
		cmd_argv_index = argv_index;
#ifndef __sun
		int mbl = strlen(mbs_buffer);
		if(mbl > 2) {
			if(mbs_buffer[mbl-1] == '\n' && mbs_buffer[mbl-2] == '\\') {
				mbs_buffer[mbl] = '\n';
				mbs_buffer[mbl+1] = 0;
			}
		}
#endif
		argv[argv_index++] = strdup(mbs_buffer);
	}
	argv[argv_index] = NULL;
	fflush(stdout);
	if ((childPid = fork()) == 0) {
		enable_interrupt((void (*) (int)) SIG_DFL);
		if (redirect_out_err) {
			redirect_io(stdout_file, stderr_file);
		}
#if 0
		if (filter_stderr) {
			redirect_stderr();
		}
#endif
		if (nice_prio != 0) {
			execve(NOCATGETS("/usr/bin/nice"), argv, environ);
			fatal_mksh("Could not load `/usr/bin/nice': %s",
			      errmsg(errno));
		} else {
			execve(shell->string_mb, argv, environ);
			if (sun_style)
				fatal_mksh("Could not load Shell from `%s': %s",
			      	shell->string_mb,
			      	errmsg(errno));
			else
				fatal_mksh("couldn't load shell (bu22)");
		}
	}
	if (childPid  == -1) {
		fatal_mksh("fork failed: %s",
		      errmsg(errno));
	}
	retmem_mb(argv[cmd_argv_index]);
	return childPid;
}

/*
 *	exec_vp(name, argv, envp, ignore_error)
 *
 *	Like execve, but does path search.
 *	This starts command when make invokes it directly (without a shell).
 *
 *	Return value:
 *				Returns false if the exec failed
 *
 *	Parameters:
 *		name		The name of the command to run
 *		argv		Arguments for the command
 *		envp		The environment for it
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		shell_name	The Name "SHELL", used to get the path to shell
 *		vroot_path	The path used by the vroot package
 */
static Boolean
exec_vp(register char *name, register char **argv, char **envp, register Boolean ignore_error, pathpt vroot_path)
{
	register Name		shell = getvar(shell_name);
	register char		*shellname;
	char			*shargv[4];
	Name			tmp_shell;

	if (IS_EQUAL(shell->string_mb, "")) {
		shell = shell_name;
	}

	for (int i = 0; i < 5; i++) {
		execve_vroot(name,
				    argv + 1,
				    envp,
				    vroot_path,
				    VROOT_DEFAULT);
		switch (errno) {
		case ENOEXEC:
		case ENOENT:
			/* That failed. Let the shell handle it */
			shellname = strrchr(shell->string_mb, (int) slash_char);
			if (shellname == NULL) {
				shellname = shell->string_mb;
			} else {
				shellname++;
			}
			shargv[0] = shellname;
			shargv[1] = (char*)(ignore_error ? NOCATGETS("-c") : NOCATGETS("-ce"));
			shargv[2] = argv[0];
			shargv[3] = NULL;
			tmp_shell = getvar(shell_name);
			if (IS_EQUAL(tmp_shell->string_mb, "")) {
				tmp_shell = shell_name;
			}
			execve_vroot(tmp_shell->string_mb,
					    shargv,
					    envp,
					    vroot_path,
					    VROOT_DEFAULT);
			return failed;
		case ETXTBSY:
			/*
			 * The program is busy (debugged?).
			 * Wait and then try again.
			 */
			sleep((unsigned) i);
		case EAGAIN:
			break;
		default:
			return failed;
		}
	}
	return failed;
}

/*
 *	doexec(command, ignore_error)
 *
 *	Will scan an argument string and split it into words
 *	thus building an argument list that can be passed to exec_ve()
 *
 *	Return value:
 *				The pid of the process started here
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		filter_stderr	If -X is on we redirect stderr
 */
int
doexec(register wchar_t *command, register Boolean ignore_error, Boolean redirect_out_err, char *stdout_file, char *stderr_file, pathpt vroot_path, int nice_prio)
{
	int			arg_count = 5;
	char			**argv;
	int			length;
	char			nice_prio_buf[MAXPATHLEN];
	register char		**p;
	wchar_t			*q;
	register wchar_t	*t;
	char			*tmp_mbs_buffer;

	/*
	 * Only prepend the /usr/bin/nice command to the original command
	 * if the nice priority, nice_prio, is NOT zero (0).
	 * Nice priorities can be a positive or a negative number.
	 */
	if (nice_prio != 0) {
		arg_count += 2;
	}
	for (t = command; *t != (int) nul_char; t++) {
		if (iswspace(*t)) {
			arg_count++;
		}
	}
	argv = (char **)alloca(arg_count * (sizeof(char *)));
	/*
	 * Reserve argv[0] for sh in case of exec_vp failure.
	 * Don't worry about prepending /usr/bin/nice command to argv[0].
	 * In fact, doing it may cause the sh command to fail!
	 */
	p = &argv[1];
	if ((length = wslen(command)) >= MAXPATHLEN) {
		tmp_mbs_buffer = getmem((length * MB_LEN_MAX) + 1);
		wcstombs(tmp_mbs_buffer, command, (length * MB_LEN_MAX) + 1);
		argv[0] = strdup(tmp_mbs_buffer);
		retmem_mb(tmp_mbs_buffer);
        } else {
		WCSTOMBS(mbs_buffer, command);
		argv[0] = strdup(mbs_buffer);
	}

	if (nice_prio != 0) {
		*p++ = strdup(NOCATGETS("/usr/bin/nice"));
		sprintf(nice_prio_buf, NOCATGETS("-%d"), nice_prio);
		*p++ = strdup(nice_prio_buf);
	}
	/* Build list of argument words. */
	for (t = command; *t;) {
		if (p >= &argv[arg_count]) {
			/* This should never happen, right? */
			WCSTOMBS(mbs_buffer, command);
			fatal_mksh("Command `%s' has more than %d arguments",
			      mbs_buffer,
			      arg_count);
		}
		q = t;
		while (!iswspace(*t) && (*t != (int) nul_char)) {
			t++;
		}
		if (*t) {
			for (*t++ = (int) nul_char; iswspace(*t); t++);
		}
		if ((length = wslen(q)) >= MAXPATHLEN) {
			tmp_mbs_buffer = getmem((length * MB_LEN_MAX) + 1);
			wcstombs(tmp_mbs_buffer, q, (length * MB_LEN_MAX) + 1);
			*p++ = strdup(tmp_mbs_buffer);
			retmem_mb(tmp_mbs_buffer);
		} else {
			WCSTOMBS(mbs_buffer, q);
			*p++ = strdup(mbs_buffer);
		}
	}
	*p = NULL;

	/* Then exec the command with that argument list. */
	fflush(stdout);
	if ((childPid = fork()) == 0) {
		enable_interrupt((void (*) (int)) SIG_DFL);
		if (redirect_out_err) {
			redirect_io(stdout_file, stderr_file);
		}
#if 0
		if (filter_stderr) {
			redirect_stderr();
		}
#endif
		exec_vp(argv[1], argv, environ, ignore_error, vroot_path);
		if (sun_style)
			fatal_mksh("Cannot load command `%s': %s",
				argv[1], errmsg(errno));
		else
			fatal_mksh("cannot load %s (bu24).");
	}
	if (childPid  == -1) {
		fatal_mksh("fork failed: %s",
		      errmsg(errno));
	}
	for (int i = 0; argv[i] != NULL; i++) {
		retmem_mb(argv[i]);
	}
	return childPid;
}

/*
 *	await(ignore_error, silent_error, target, command, running_pid)
 *
 *	Wait for one child process and analyzes
 *	the returned status when the child process terminates.
 *
 *	Return value:
 *				Returns true if commands ran OK
 *
 *	Parameters:
 *		ignore_error	Should we abort on error?
 *		silent_error	Should error messages be suppressed for dmake?
 *		target		The target we are building, for error msgs
 *		command		The command we ran, for error msgs
 *		running_pid	The pid of the process we are waiting for
 *		
 *	Static variables used:
 *		filter_file	The fd for the filter file
 *		filter_file_name The name of the filter file
 *
 *	Global variables used:
 *		filter_stderr	Set if -X is on
 */
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
Boolean
await(register Boolean ignore_error, register Boolean silent_error, Name target, wchar_t *command, pid_t running_pid, Boolean send_mtool_msgs, XDR *xdrs_p, int job_msg_id)
#else
Boolean
await(register Boolean ignore_error, register Boolean silent_error, Name target, wchar_t *command, pid_t running_pid, Boolean send_mtool_msgs, void *xdrs_p, int job_msg_id)
#endif
{
#ifdef SUN5_0
        int                     status;
#else
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat)       stat.w_T.w_Retcode
#endif
#ifndef WTERMSIG
#define WTERMSIG(stat)          stat.w_T.w_Termsig
#endif
#ifndef WCOREDUMP
#define WCOREDUMP(stat)         stat.w_T.w_Coredump
#endif
	int			status;
#endif
	int			core_dumped;
	int			exit_status;
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
	Avo_CmdOutput		*make_output_msg;
#endif
	register pid_t		pid;
	int			termination_signal;
	char			tmp_buf[MAXPATHLEN];
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
	RWCollectable		*xdr_msg;
#endif

	while ((pid = wait(&status)) != running_pid) {
		if (pid == -1) {
			fatal_mksh("wait() failed: %s", errmsg(errno));
		}
	}
	fflush(stdout);
	fflush(stderr);

        if (status == 0) {

#ifdef PRINT_EXIT_STATUS
		warning_mksh(NOCATGETS("I'm in await(), and status is 0."));
#endif

                return succeeded;
	}

#ifdef PRINT_EXIT_STATUS
	warning_mksh(NOCATGETS("I'm in await(), and status is *NOT* 0."));
#endif

        exit_status = WEXITSTATUS(status);

#ifdef PRINT_EXIT_STATUS
	warning_mksh(NOCATGETS("I'm in await(), and exit_status is %d."), exit_status);
#endif

        termination_signal = WTERMSIG(status);
#ifndef _AIX
        core_dumped = WCOREDUMP(status);
#else
	core_dumped = 0;
#endif

	/*
	 * If the child returned an error, we now try to print a
	 * nice message about it.
	 */
	SEND_MTOOL_MSG(
		make_output_msg = new Avo_CmdOutput();
		sprintf(tmp_buf, "%d", job_msg_id);
		make_output_msg->appendOutput(AVO_STRDUP(tmp_buf));
	);

	tmp_buf[0] = (int) nul_char;
	if (!silent_error) {
		if (exit_status != 0) {
			fprintf(stdout,
				       "*** Error code %d%s",
				       exit_status,
				       sun_style ? "" : " (bu21)");
			SEND_MTOOL_MSG(
				sprintf(&tmp_buf[strlen(tmp_buf)],
					       "*** Error code %d",
					       exit_status);
			);
		} else {
#if 0
			if (termination_signal > NSIG) {
#endif
				fprintf(stdout,
					       "*** Signal %d",
					       termination_signal);
				SEND_MTOOL_MSG(
					sprintf(&tmp_buf[strlen(tmp_buf)],
						       "*** Signal %d",
						       termination_signal);
				);
#if 0
			} else {
				fprintf(stdout,
					       "*** %s",
					       sys_siglist[termination_signal]);
				SEND_MTOOL_MSG(
					sprintf(&tmp_buf[strlen(tmp_buf)],
						       "*** %s",
						       sys_siglist[termination_signal]);
				);
			}
#endif
			if (core_dumped) {
				fprintf(stdout,
					       " - core dumped");
				SEND_MTOOL_MSG(
					sprintf(&tmp_buf[strlen(tmp_buf)],
						       " - core dumped");
				);
			}
		}
		if (ignore_error) {
			fprintf(stdout,
				       " (ignored)");
			SEND_MTOOL_MSG(
				sprintf(&tmp_buf[strlen(tmp_buf)],
					       " (ignored)");
			);
		}
		fprintf(stdout, "\n");
		fflush(stdout);
		SEND_MTOOL_MSG(
			make_output_msg->appendOutput(AVO_STRDUP(tmp_buf));
		);
	}
	SEND_MTOOL_MSG(
		xdr_msg = (RWCollectable*) make_output_msg;
		xdr(xdrs_p, xdr_msg);
		delete make_output_msg;
	);

#ifdef PRINT_EXIT_STATUS
	warning_mksh(NOCATGETS("I'm in await(), returning failed."));
#endif

	return failed;
}

/*
 *	sh_command2string(command, destination)
 *
 *	Run one sh command and capture the output from it.
 *
 *	Return value:
 *
 *	Parameters:
 *		command		The command to run
 *		destination	Where to deposit the output from the command
 *		
 *	Static variables used:
 *
 *	Global variables used:
 */
void
sh_command2string(register String command, register String destination)
{
	register FILE		*fd;
	register int		chr;
	int			status;
	Boolean			command_generated_output = false;

	command->text.p = (int) nul_char;
	WCSTOMBS(mbs_buffer, command->buffer.start);
	if ((fd = popen(mbs_buffer, "r")) == NULL) {
		WCSTOMBS(mbs_buffer, command->buffer.start);
		fatal_mksh("Could not run command `%s' for :sh transformation",
		      mbs_buffer);
	}
	while ((chr = getc(fd)) != EOF) {
		if (chr == (int) newline_char) {
			chr = (int) space_char;
		}
		command_generated_output = true;
		append_char(chr, destination);
	}

	/*
	 * We don't want to keep the last LINE_FEED since usually
	 * the output of the 'sh:' command is used to evaluate
	 * some MACRO. ( /bin/sh and other shell add a line feed
	 * to the output so that the prompt appear in the right place.
	 * We don't need that
	 */
	if (command_generated_output){
		if ( *(destination->text.p-1) == (int) space_char) {
			* (-- destination->text.p) = '\0';
		} 
	} else {
		/*
		 * If the command didn't generate any output,
		 * set the buffer to a null string.
		 */
		*(destination->text.p) = '\0';
	}
			
	status = pclose(fd);
	if (status != 0) {
		WCSTOMBS(mbs_buffer, command->buffer.start);
		fatal_mksh("The command `%s' returned status `%d'",
		      mbs_buffer,
		      WEXITSTATUS(status));
	}
}



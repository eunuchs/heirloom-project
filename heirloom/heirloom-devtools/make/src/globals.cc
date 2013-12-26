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
 * @(#)globals.cc 1.42 06/12/12
 */

/*	from OpenSolaris "globals.cc	1.42	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)globals.cc	1.8 (gritter) 3/6/07
 */

/*
 *	globals.cc
 *
 *	This declares all global variables
 */

/*
 * Included files
 */
#include <nl_types.h>
#include <mk/defs.h>
#include <sys/stat.h>

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Global variables used by make only
 */
	FILE		*dependency_report_file;

/*
 * Global variables used by make
 */
	Boolean		allrules_read=false;
	Name		posix_name;
	Name		svr4_name;
	Boolean		sdot_target;	/* used to identify s.m(/M)akefile */
	Boolean		all_parallel;			/* TEAMWARE_MAKE_CMN */
	Boolean		assign_done;
	int foo;	
	Boolean		Bflag;
	Boolean		build_failed_seen;
#ifdef DISTRIBUTED
	Boolean		building_serial;
#endif
	Name		built_last_make_run;
	Name		c_at;
#ifdef DISTRIBUTED
	Boolean		called_make = false;
#endif
	Boolean		cleanup;
	Boolean		close_report;
	Boolean		command_changed;
	Boolean		commands_done;
	Chain		conditional_targets;
	Name		conditionals;
	Boolean		continue_after_error;		/* `-k' */
	Property	current_line;
	Name		current_make_version;
	Name		current_target;
	short		debug_level;
	Cmd_line	default_rule;
	Name		default_rule_name;
	Name		default_target_to_build;
	Name		dmake_group;
	Name		dmake_max_jobs;
	Name		dmake_mode;
	DMake_mode	dmake_mode_type;
	Name		dmake_output_mode;
	DMake_output_mode	output_mode = txt1_mode;
	Name		dmake_odir;
	Name		dmake_rcfile;
	Name		done;
	Name		dot;
	Name		dot_keep_state;
	Name		dot_keep_state_file;
	Name		empty_name;
	int		exit_status;
	Boolean		fatal_in_progress;
	int		file_number;
#if 0
	Boolean		filter_stderr;			/* `-X' */
#endif
	Name		force;
	Name		ignore_name;
	Boolean		ignore_errors;			/* `-i' */
	Boolean		ignore_errors_all;		/* `-i' */
	Name		init;
	int		job_msg_id;
	Boolean		keep_state;
	Name		make_state;
	timestruc_t	make_state_before;
	Dependency	makefiles_used;
	Name		makeflags;
//	Boolean		make_state_locked; // Moved to lib/mksh
	Name		make_version;
	char		mbs_buffer2[(MAXPATHLEN * MB_LEN_MAX)];
	char		*mbs_ptr;
	char		*mbs_ptr2;
	int		mtool_msgs_fd;
	Boolean		depinfo_already_read = false;
#ifdef NSE
        Name		derived_src;
	Boolean		nse;				/* NSE on */
        Name            nse_backquote_seen;
	char		nse_depinfo_lockfile[MAXPATHLEN];
	Boolean		nse_depinfo_locked;
        Boolean         nse_did_recursion;
        Name            nse_shell_var_used;
        Boolean         nse_watch_vars = false;
	wchar_t		current_makefile[MAXPATHLEN];
#endif
	Boolean		no_action_was_taken = true;	/* true if we've not **
							** run any command   */

	Boolean		no_parallel = false;		/* TEAMWARE_MAKE_CMN */
#ifdef SGE_SUPPORT
	Boolean		grid = false;			/* TEAMWARE_MAKE_CMN */
#endif
	Name		**mutexlist;
	unsigned	nmutexlist;
	Name		mutex_name;
	Name		no_parallel_name;
	Name		not_auto;
	Boolean		only_parallel;			/* TEAMWARE_MAKE_CMN */
	Boolean		parallel;			/* TEAMWARE_MAKE_CMN */
	Name		parallel_name;
	Name		localhost_name;
	int		parallel_process_cnt;
	int		pmake_max_jobs = 0;
	Percent		percent_list;
	Dyntarget	dyntarget_list;
	Name		plus;
	Name		pmake_machinesfile;
	Name		precious;
        Name		primary_makefile;
	Boolean		quest;				/* `-q' */
	short		read_trace_level;
        Boolean 	reading_dependencies = false;
	Name		recursive_name;
	int		recursion_level;
	short		report_dependencies_level = 0;	/* -P */
	Boolean		report_pwd;
	Boolean		rewrite_statefile;
	Running		running_list;
	char		*sccs_dir_path;
	Name		sccs_get_name;
	Name		sccs_get_posix_name;
	Cmd_line	sccs_get_rule;
	Cmd_line	sccs_get_org_rule;
	Cmd_line	sccs_get_posix_rule;
	Name		get_name;
	Cmd_line	get_rule;
	Name		get_posix_name;
	Cmd_line	get_posix_rule;
	Boolean		send_mtool_msgs;		/* `-K' */
	Boolean		all_precious;
	Boolean		silent_all;			/* `-s' */
	Boolean		report_cwd;			/* `-w' */
	Boolean		silent;				/* `-s' */
	Name		silent_name;
	char		*stderr_file = NULL;
	char		*stdout_file = NULL;
#ifdef SGE_SUPPORT
	char		script_file[MAXPATHLEN] = "";
#endif
	Boolean		stdout_stderr_same;
	Dependency	suffixes;
	Name		suffixes_name;
	Name		sunpro_dependencies;
        Boolean		target_variants;
	char		*tmpdir = NOCATGETS("/tmp");
	char		*temp_file_directory = NOCATGETS(".");
	Name		temp_file_name;
	short		temp_file_number;
	time_t		timing_start;
	wchar_t		*top_level_target;
	Boolean		touch;				/* `-t' */
	Boolean		trace_reader;			/* `-D' */
	Boolean		build_unconditional;		/* `-u' */
	pathpt		vroot_path = VROOT_DEFAULT;
	Name		wait_name;
	wchar_t		wcs_buffer2[MAXPATHLEN];
	wchar_t		*wcs_ptr;
	wchar_t		*wcs_ptr2;
	long int	hostid;
	const char	bourne_shell[] = SHELL;
	const char	posix_shell[] = POSIX_SHELL;

/*
 * File table of contents
 */


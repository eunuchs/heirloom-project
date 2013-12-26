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
 * @(#)misc.cc 1.50 06/12/12
 */

/*	from OpenSolaris "misc.cc	1.34	95/10/04"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)misc.cc	1.12 (gritter) 3/7/07
 */

/*
 *	misc.cc
 *
 *	This file contains various unclassified routines. Some main groups:
 *		getname
 *		Memory allocation
 *		String handling
 *		Property handling
 *		Error message handling
 *		Make internal state dumping
 *		main routine support
 */

/*
 * Included files
 */
#include <errno.h>
#include <mk/defs.h>
#include <mksh/macro.h>		/* SETVAR() */
#include <mksh/misc.h>		/* enable_interrupt() */
#include <stdarg.h>		/* va_list, va_start(), va_end() */
#include <vroot/report.h>	/* SUNPRO_DEPENDENCIES */

#include <unistd.h>

#ifdef TEAMWARE_MAKE_CMN
#define MAXJOBS_ADJUST_RFE4694000

#ifdef MAXJOBS_ADJUST_RFE4694000
extern void job_adjust_fini();
#endif /* MAXJOBS_ADJUST_RFE4694000 */
#endif /* TEAMWARE_MAKE_CMN */

extern const char *progname;

#include <time.h>		/* localtime() */

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */
static	void		print_rule(register Name target);
static	void		print_target_n_deps(register Name target);

/*****************************************
 *
 *	getname
 */

/*****************************************
 *
 *	Memory allocation
 */

/*
 *	free_chain()
 *
 *	frees a chain of Name_vector's
 *
 *	Parameters:
 *		ptr		Pointer to the first element in the chain
 *				to be freed.
 *
 *	Global variables used:
 */
void 
free_chain(Name_vector ptr)
{
	if (ptr != NULL) {
		if (ptr->next != NULL) {
			free_chain(ptr->next);
		}
		free((char *) ptr);
	}
}

/*****************************************
 *
 *	String manipulation
 */

/*****************************************
 *
 *	Nameblock property handling
 */

/*****************************************
 *
 *	Error message handling
 */

/*
 *	fatal(format, args...)
 *
 *	Print a message and die
 *
 *	Parameters:
 *		format		printf type format string
 *		args		Arguments to match the format
 *
 *	Global variables used:
 *		fatal_in_progress Indicates if this is a recursive call
 *		parallel_process_cnt Do we need to wait for anything?
 *		report_pwd	Should we report the current path?
 */
/*VARARGS*/
void
fatal(const char * message, ...)
{
	va_list args;

	va_start(args, message);
	fflush(stdout);
	if (message == NULL)
		fprintf(stderr, "%s: fatal error.\n", progname);
	else {
		if (sun_style)
			fprintf(stderr, "%s: Fatal error: ", progname);
		else
			fprintf(stderr, "%s: fatal error: ", progname);
#if 0
#ifdef DISTRIBUTED
		fprintf(stderr, "dmake: Fatal error: ");
#else
		fprintf(stderr, "make: Fatal error: ");
#endif
#endif
		vfprintf(stderr, message, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
	if (report_pwd) {
		fprintf(stderr,
			       "Current working directory %s\n",
			       get_current_path());
	}
	fflush(stderr);
	if (fatal_in_progress) {
		exit_status = 1;
		exit(1);
	}
	fatal_in_progress = true;
#ifdef TEAMWARE_MAKE_CMN
	/* Let all parallel children finish */
	if ((dmake_mode_type == parallel_mode) &&
	    (parallel_process_cnt > 0)) {
		fprintf(stderr,
			       "Waiting for %d %s to finish\n",
			       parallel_process_cnt,
			       parallel_process_cnt == 1 ?
			       "job" : "jobs");
		fflush(stderr);
	}
#endif

	while (parallel_process_cnt > 0) {
#ifdef DISTRIBUTED
		if (dmake_mode_type == distributed_mode) {
			await_dist(false);
		} else {
			await_parallel(true);
		}
#else
		await_parallel(true);
#endif
		finish_children(false);
	}

#if defined (TEAMWARE_MAKE_CMN) && defined (MAXJOBS_ADJUST_RFE4694000)
	job_adjust_fini();
#endif

	exit_status = 1;
	exit(1);
}

/*
 *	warning(format, args...)
 *
 *	Print a message and continue.
 *
 *	Parameters:
 *		format		printf type format string
 *		args		Arguments to match the format
 *
 *	Global variables used:
 *		report_pwd	Should we report the current path?
 */
/*VARARGS*/
void
warning(const char * message, ...)
{
	va_list args;

	if (wflag)
		return;
	va_start(args, message);
	fflush(stdout);
	if (sun_style)
		fprintf(stderr, "%s: Warning: ", progname);
	else
		fprintf(stderr, "%s: ", progname);
#if 0
#ifdef DISTRIBUTED
	fprintf(stderr, "dmake: Warning: ");
#else
	fprintf(stderr, "make: Warning: ");
#endif
#endif
	vfprintf(stderr, message, args);
	fprintf(stderr, "\n");
	va_end(args);
	if (report_pwd) {
		fprintf(stderr,
			       "Current working directory %s\n",
			       get_current_path());
	}
	fflush(stderr);
}

void
print_command(char *cp)
{
	int	c = '\n';

	while (*cp) {
		if (!sun_style && c == '\n')
			putchar('\t');
		c = *cp++ & 0377;
		putchar(c);
	}
	putchar('\n');
}

/*
 *	time_to_string(time)
 *
 *	Take a numeric time value and produce
 *	a proper string representation.
 *
 *	Return value:
 *				The string representation of the time
 *
 *	Parameters:
 *		time		The time we need to translate
 *
 *	Global variables used:
 */
char *
time_to_string(const timestruc_t &time)
{
	struct tm		*tm;
	char			buf[128];
	const char		*fmt = NOCATGETS("%c %Z");

        if (time == file_doesnt_exist) {
                return "File does not exist";
        }
        if (time == file_max_time) {
                return "Younger than any file";
        }
	tm = localtime(&time.tv_sec);
	strftime(buf, sizeof (buf), fmt, tm);
        buf[127] = (int) nul_char;
        return strdup(buf);
}

/*
 *	get_current_path()
 *
 *	Stuff current_path with the current path if it isnt there already.
 *
 *	Parameters:
 *
 *	Global variables used:
 */
char *
get_current_path(void)
{
	char			pwd[(MAXPATHLEN * MB_LEN_MAX)];
	static char		*current_path;

	if (current_path == NULL) {
		getcwd(pwd, sizeof(pwd));
		if (pwd[0] == (int) nul_char) {
			pwd[0] = (int) slash_char;
			pwd[1] = (int) nul_char;
#ifdef DISTRIBUTED
			current_path = strdup(pwd);
		} else if (IS_EQUALN(pwd, NOCATGETS("/tmp_mnt"), 8)) {
			current_path = strdup(pwd + 8);
		} else {
			current_path = strdup(pwd);
		}
#else
		}
		current_path = strdup(pwd);
#endif
	}
	return current_path;
}

/*****************************************
 *
 *	Make internal state dumping
 *
 *	This is a set  of routines for dumping the internal make state
 *	Used for the -p option
 */

/*
 *	dump_make_state()
 *
 *	Dump make's internal state to stdout
 *
 *	Parameters:
 *
 *	Global variables used:
 *		svr4 			Was ".SVR4" seen in makefile?
 *		svr4_name		The Name ".SVR4", printed
 *		posix			Was ".POSIX" seen in makefile?
 *		posix_name		The Name ".POSIX", printed
 *		default_rule		Points to the .DEFAULT rule
 *		default_rule_name	The Name ".DEFAULT", printed
 *		default_target_to_build	The first target to print
 *		dot_keep_state		The Name ".KEEP_STATE", printed
 *		dot_keep_state_file	The Name ".KEEP_STATE_FILE", printed
 *		hashtab			The make hash table for Name blocks
 *		ignore_errors		Was ".IGNORE" seen in makefile?
 *		ignore_name		The Name ".IGNORE", printed
 *		keep_state		Was ".KEEP_STATE" seen in makefile?
 *		percent_list		The list of % rules
 *		precious		The Name ".PRECIOUS", printed
 *		sccs_get_name		The Name ".SCCS_GET", printed
 *		sccs_get_posix_name	The Name ".SCCS_GET_POSIX", printed
 *		get_name		The Name ".GET", printed
 *		get_posix_name		The Name ".GET_POSIX", printed
 *		sccs_get_rule		Points to the ".SCCS_GET" rule
 *		silent			Was ".SILENT" seen in makefile?
 *		silent_name		The Name ".SILENT", printed
 *		suffixes		The suffix list from ".SUFFIXES"
 *		suffixes_name		The Name ".SUFFIX", printed
 */
void
dump_make_state(void)
{
	Name_set::iterator	p, e;
	register Property	prop;
	register Dependency	dep;
	register Cmd_line	rule;
	Percent			percent, percent_depe;

	/* Default target */
	if (default_target_to_build != NULL) {
		print_rule(default_target_to_build);
	}
	printf("\n");

	/* .POSIX */
	if (posix) {
		printf("%s:\n", posix_name->string_mb);
	}

	/* .DEFAULT */
	if (default_rule != NULL) {
		printf("%s:\n", default_rule_name->string_mb);
		for (rule = default_rule; rule != NULL; rule = rule->next) {
			printf("\t%s\n", rule->command_line->string_mb);
		}
	}

	/* .IGNORE */
	if (ignore_errors) {
		printf("%s:\n", ignore_name->string_mb);
	}

	/* .KEEP_STATE: */
	if (keep_state) {
		printf("%s:\n\n", dot_keep_state->string_mb);
	}

	/* .PRECIOUS */
	printf("%s:", precious->string_mb);
	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			if ((p->stat.is_precious) || (all_precious)) {
				printf(" %s", p->string_mb);
			}
	}
	printf("\n");

	/* .SCCS_GET */
	if (sccs_get_rule != NULL) {
		printf("%s:\n", sccs_get_name->string_mb);
		for (rule = sccs_get_rule; rule != NULL; rule = rule->next) {
			printf("\t%s\n", rule->command_line->string_mb);
		}
	}

	/* .SILENT */
	if (silent) {
		printf("%s:\n", silent_name->string_mb);
	}

	/* .SUFFIXES: */
	printf("%s:", suffixes_name->string_mb);
	for (dep = suffixes; dep != NULL; dep = dep->next) {
		printf(" %s", dep->name->string_mb);
		build_suffix_list(dep->name);
	}
	printf("\n\n");

	/* % rules */
	for (percent = percent_list;
	     percent != NULL;
	     percent = percent->next) {
		printf("%s:",
			      percent->name->string_mb);
		
		for (percent_depe = percent->dependencies;
		     percent_depe != NULL;
		     percent_depe = percent_depe->next) {
			printf(" %s", percent_depe->name->string_mb);
		}
		
		printf("\n");

		for (rule = percent->command_template;
		     rule != NULL;
		     rule = rule->next) {
			printf("\t%s\n", rule->command_line->string_mb);
		}
	}

	/* Suffix rules */
	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			Wstring wcb(p);
			if (wcb.get_string()[0] == (int) period_char) {
				print_rule(p);
			}
	}

	/* Macro assignments */
	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			if (((prop = get_prop(p->prop, macro_prop)) != NULL) &&
			    (prop->body.macro.value != NULL)) {
				printf("%s", p->string_mb);
				print_value(prop->body.macro.value,
					    (Daemon) prop->body.macro.daemon);
			}
	}
	printf("\n");

	/* Conditional macro assignments */
	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			for (prop = get_prop(p->prop, conditional_prop);
			     prop != NULL;
			     prop = get_prop(prop->next, conditional_prop)) {
				printf("%s := %s",
					      p->string_mb,
					      prop->body.conditional.name->
					      string_mb);
				if (prop->body.conditional.append) {
					printf(" +");
				}
				else {
					printf(" ");
				}
				print_value(prop->body.conditional.value,
					    no_daemon);
			}
	}
	printf("\n");

	/* All other dependencies */
	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			if (p->colons != no_colon) {
				print_rule(p);
			}
	}
	printf("\n");
}

/*
 *	print_rule(target)
 *
 *	Print the rule for one target
 *
 *	Parameters:
 *		target		Target we print rule for
 *
 *	Global variables used:
 */
static void
print_rule(register Name target)
{
	register Cmd_line	rule;
	register Property	line;
	register Dependency	dependency;

	if (target->dependency_printed ||
	    ((line = get_prop(target->prop, line_prop)) == NULL) ||
	    ((line->body.line.command_template == NULL) &&
	     (line->body.line.dependencies == NULL))) {
		return;
	}
	target->dependency_printed = true;

	printf("%s:", target->string_mb);

	for (dependency = line->body.line.dependencies;
	     dependency != NULL;
	     dependency = dependency->next) {
		printf(" %s", dependency->name->string_mb);
	}

	printf("\n");

	for (rule = line->body.line.command_template;
	     rule != NULL;
	     rule = rule->next) {
		printf("\t%s\n", rule->command_line->string_mb);
	}
}

void
dump_target_list(void)
{
	Name_set::iterator	p, e;
	Wstring	str;

	for (p = hashtab.begin(), e = hashtab.end(); p != e; p++) {
			str.init(p);
			wchar_t * wcb = str.get_string();
			if ((p->colons != no_colon) &&
			    ((wcb[0] != (int) period_char) ||
			     ((wcb[0] == (int) period_char) &&
			      (wschr(wcb, (int) slash_char))))) {
				print_target_n_deps(p);
			}
	}
}

static void
print_target_n_deps(register Name target)
{
	register Property	line;
	register Dependency	dependency;

	if (target->dependency_printed) {
		return;
	}
	target->dependency_printed = true;

	printf("%s\n", target->string_mb);

	if ((line = get_prop(target->prop, line_prop)) == NULL) {
		return;
	}
	for (dependency = line->body.line.dependencies;
	     dependency != NULL;
	     dependency = dependency->next) {
		if (!dependency->automatic) {
			print_target_n_deps(dependency->name);
		}
	}
}

/*****************************************
 *
 *	main() support
 */

/*
 *	load_cached_names()
 *
 *	Load the vector of cached names
 *
 *	Parameters:
 *
 *	Global variables used:
 *		Many many pointers to Name blocks.
 */
void
load_cached_names(void)
{
	char		*cp;
	Name		dollar;

	/* Load the cached_names struct */
	MBSTOWCS(wcs_buffer, NOCATGETS(".BUILT_LAST_MAKE_RUN"));
	built_last_make_run = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("@"));
	c_at = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(" *conditionals* "));
	conditionals = GETNAME(wcs_buffer, FIND_LENGTH);
	/*
	 * A version of make was released with NSE 1.0 that used
	 * VERSION-1.1 but this version is identical to VERSION-1.0.
	 * The version mismatch code makes a special case for this
	 * situation.  If the version number is changed from 1.0
	 * it should go to 1.2.
	 */
	MBSTOWCS(wcs_buffer, NOCATGETS("VERSION-1.0"));
	current_make_version = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".SVR4"));
	svr4_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".POSIX"));
	posix_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".DEFAULT"));
	default_rule_name = GETNAME(wcs_buffer, FIND_LENGTH);
#ifdef NSE
	MBSTOWCS(wcs_buffer, NOCATGETS(".DERIVED_SRC"));
        derived_src= GETNAME(wcs_buffer, FIND_LENGTH);
#endif
	MBSTOWCS(wcs_buffer, NOCATGETS("$"));
	dollar = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".DONE"));
	done = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("."));
	dot = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".KEEP_STATE"));
	dot_keep_state = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".KEEP_STATE_FILE"));
	dot_keep_state_file = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(""));
	empty_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(" FORCE"));
	force = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("HOST_ARCH"));
	host_arch = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("HOST_MACH"));
	host_mach = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".IGNORE"));
	ignore_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".INIT"));
	init = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".LOCAL"));
	localhost_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".make.state"));
	make_state = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("MAKEFLAGS"));
	makeflags = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".MAKE_VERSION"));
	make_version = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".MUTEX"));
	mutex_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".NO_PARALLEL"));
	no_parallel_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".NOT_AUTO"));
	not_auto = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".PARALLEL"));
	parallel_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("PATH"));
	path_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("+"));
	plus = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".PRECIOUS"));
	precious = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("?"));
	query = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("^"));
	hat = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".RECURSIVE"));
	recursive_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".SCCS_GET"));
	sccs_get_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".SCCS_GET_POSIX"));
	sccs_get_posix_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".GET"));
	get_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".GET_POSIX"));
	get_posix_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("SHELL"));
	shell_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".SILENT"));
	silent_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".SUFFIXES"));
	suffixes_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, SUNPRO_DEPENDENCIES);
	sunpro_dependencies = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("TARGET_ARCH"));
	target_arch = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("TARGET_MACH"));
	target_mach = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("VIRTUAL_ROOT"));
	virtual_root = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS("VPATH"));
	vpath_name = GETNAME(wcs_buffer, FIND_LENGTH);
	MBSTOWCS(wcs_buffer, NOCATGETS(".WAIT"));
	wait_name = GETNAME(wcs_buffer, FIND_LENGTH);

	wait_name->state = build_ok;

	/* Mark special targets so that the reader treats them properly */
	svr4_name->special_reader = svr4_special;
	posix_name->special_reader = posix_special;
	built_last_make_run->special_reader = built_last_make_run_special;
	default_rule_name->special_reader = default_special;
#ifdef NSE
        derived_src->special_reader= derived_src_special;
#endif
	dot_keep_state->special_reader = keep_state_special;
	dot_keep_state_file->special_reader = keep_state_file_special;
	ignore_name->special_reader = ignore_special;
	make_version->special_reader = make_version_special;
	mutex_name->special_reader = mutex_special;
	no_parallel_name->special_reader = no_parallel_special;
	parallel_name->special_reader = parallel_special;
	localhost_name->special_reader = localhost_special;
	precious->special_reader = precious_special;
	sccs_get_name->special_reader = sccs_get_special;
	sccs_get_posix_name->special_reader = sccs_get_posix_special;
	get_name->special_reader = get_special;
	get_posix_name->special_reader = get_posix_special;
	silent_name->special_reader = silent_special;
	suffixes_name->special_reader = suffixes_special;

	/* The value of $$ is $ */
	SETVAR(dollar, dollar, false);
	dollar->dollar = false;

	/* Set the value of $(SHELL) */
	if (posix) {
	  MBSTOWCS(wcs_buffer, posix_shell);
	} else {
	  MBSTOWCS(wcs_buffer, bourne_shell);
	}
	SETVAR(shell_name, GETNAME(wcs_buffer, FIND_LENGTH), false);

	/*
	 * Use " FORCE" to simulate a FRC dependency for :: type
	 * targets with no dependencies.
	 */
	append_prop(force, line_prop);
	force->stat.time = file_max_time;

	/* Make sure VPATH is defined before current dir is read */
	if ((cp = getenv(vpath_name->string_mb)) != NULL) {
		MBSTOWCS(wcs_buffer, cp);
		SETVAR(vpath_name,
			      GETNAME(wcs_buffer, FIND_LENGTH),
			      false);
	}

	/* Check if there is NO PATH variable. If not we construct one. */
	if (getenv(path_name->string_mb) == NULL) {
		vroot_path = NULL;
		add_dir_to_path(NOCATGETS("."), &vroot_path, -1);
		add_dir_to_path(NOCATGETS("/bin"), &vroot_path, -1);
		add_dir_to_path(NOCATGETS("/usr/bin"), &vroot_path, -1);
	}
}

/* 
 * iterate on list of conditional macros in np, and place them in 
 * a String_rec starting with, and separated by the '$' character.
 */
void
cond_macros_into_string(Name np, String_rec *buffer)
{
	Macro_list	macro_list;

	/* 
	 * Put the version number at the start of the string
	 */
	MBSTOWCS(wcs_buffer, DEPINFO_FMT_VERSION);
	append_string(wcs_buffer, buffer, FIND_LENGTH);
	/* 
	 * Add the rest of the conditional macros to the buffer
	 */
	if (np->depends_on_conditional){
		for (macro_list = np->conditional_macro_list; 
		     macro_list != NULL; macro_list = macro_list->next){
			append_string(macro_list->macro_name, buffer, 
				FIND_LENGTH);
			append_char((int) equal_char, buffer);
			append_string(macro_list->value, buffer, FIND_LENGTH);
			append_char((int) dollar_char, buffer);
		}
	}
}
/*
 *	Copyright (c) 1987-1992 Sun Microsystems, Inc.  All Rights Reserved.
 *	Sun considers its source code as an unpublished, proprietary
 *	trade secret, and it is available only under strict license
 *	provisions.  This copyright notice is placed here only to protect
 *	Sun in the event the source is deemed a published work.  Dissassembly,
 *	decompilation, or other means of reducing the object code to human
 *	readable form is prohibited by the license agreement under which
 *	this code is provided to the user or company in possession of this
 *	copy.
 *	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *	Government is subject to restrictions as set forth in subparagraph
 *	(c)(1)(ii) of the Rights in Technical Data and Computer Software
 *	clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *	NASA FAR Supplement.
 *
 * 1.3 91/09/30
 */


/* Some includes are commented because of the includes at the beginning */
/* #include <signal.h> */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
/* #include <string.h> */
#include <unistd.h>
#include <stdlib.h>
/* #include <stdio.h> */
/* #include <avo/find_dir.h> */
/* #ifndef TEAMWARE_MAKE_CMN
#include <avo/find_dir.h>
#endif */

/* Routines to find the base directory name from which the various components
 * -executables, *crt* libraries etc will be accessed
 */

/* This routine checks to see if a given filename is an executable or not.
   Logically similar to the csh statement : if  ( -x $i && ! -d $i )
 */

static int
check_if_exec(char *file)
{
        struct stat stb;
        if (stat(file, &stb) < 0) {
                return ( -1);
        }
        if (S_ISDIR(stb.st_mode)) {
                return (-1);
        }
        if (!(stb.st_mode & S_IEXEC)) {
                return ( -1);
        }
        return (0);
}

/* resolve - check for specified file in specified directory
 *	sets up dir, following symlinks.
 *	returns zero for success, or
 *	-1 for error (with errno set properly)
 */
static int
resolve (char	*indir,	/* search directory */
	 char	*cmd,	/* search for name */
	 char	*dir,	/* directory buffer */
	 char	**run)	/* resultion name ptr ptr */
{
    char               *p;
    int                 rv = -1;
    int                 sll;
    char                symlink[MAXPATHLEN + 1];

    do {
	errno = ENAMETOOLONG;
	if ((strlen (indir) + strlen (cmd) + 2) > (size_t) MAXPATHLEN)
	    break;

	sprintf(dir, "%s/%s", indir, cmd);
	if (check_if_exec(dir) != 0)  /* check if dir is an executable */
	{
		break;		/* Not an executable program */
	}

	/* follow symbolic links */
	while ((sll = readlink (dir, symlink, MAXPATHLEN)) >= 0) {
	    symlink[sll] = 0;
	    if (*symlink == '/')
		strcpy (dir, symlink);
	    else
		sprintf (strrchr (dir, '/'), "/%s", symlink);
	}
	if (errno != EINVAL)
	    break;

	p = strrchr (dir, '/');
	*p++ = 0;
	if (run)		/* user wants resolution name */
	    *run = p;
	rv = 0;			/* complete, with success! */

    } while (0);

    return rv;
}

/* 
 *find_run_directory - find executable file in PATH
 *
 * PARAMETERS:
 *	cmd	filename as typed by user (argv[0])
 *	cwd	buffer from which is read the working directory
 *		 if first character is '/' or into which is
 *		 copied working directory name otherwise
 *	dir	buffer into which is copied program's directory
 *	pgm	where to return pointer to tail of cmd (may be NULL
 *		 if not wanted) 
 *	run	where to return pointer to tail of final resolved
 *		 name ( dir/run is the program) (may be NULL
 *		 if not wanted)
 *	path	user's path from environment
 *
 * Note: run and pgm will agree except when symbolic links have
 *	renamed files
 *
 * RETURNS:
 *	returns zero for success,
 *	-1 for error (with errno set properly).
 *
 * EXAMPLE:
 *	find_run_directory (argv[0], ".", &charray1, (char **) 0, (char **) 0,
 *                          getenv(NOGETTEXT("PATH")));
 */
extern int
find_run_directory (char	*cmd,
		    char	*cwd,
		    char	*dir,
		    char	**pgm,
		    char	**run,
		    char	*path)
{
    int                 rv = 0;
    char 		*f, *s;
    int			i;
    char		tmp_path[MAXPATHLEN];

    if (!cmd || !*cmd || !cwd || !dir) {
	errno = EINVAL;		/* stupid arguments! */
	return -1;
    }

    if (*cwd != '/')
	if (!(getcwd (cwd, MAXPATHLEN)))
	    return -1;		/* can not get working directory */

    f = strrchr (cmd, '/');
    if (pgm)			/* user wants program name */
	*pgm = f ? f + 1 : cmd;

    /* get program directory */
    rv = -1;
    if (*cmd == '/')	/* absname given */
	rv = resolve ("", cmd + 1, dir, run);
    else if (f)		/* relname given */
	rv = resolve (cwd, cmd, dir, run);
    else {  /* from searchpath */
	    if (!path || !*path) {	/* if missing or null path */
		    tmp_path[0] = '.';	/* assume sanity */
		    tmp_path[1] = '\0';
	    } else {
		    strcpy(tmp_path, path);
	    }
	f = tmp_path;
	rv = -1;
	errno = ENOENT;	/* errno gets this if path empty */
	while (*f && (rv < 0)) {
	    s = f;
	    while (*f && (*f != ':'))
		++f;
	    if (*f)
		*f++ = 0;
	    if (*s == '/')
		rv = resolve (s, cmd, dir, run);
	    else {
		char                abuf[MAXPATHLEN];

		sprintf (abuf, "%s/%s", cwd, s);
		rv = resolve (abuf, cmd, dir, run);
	    }
	}
    }

    /* Remove any trailing /. */
    i = strlen(dir);
    if ( dir[i-2] == '/' && dir[i-1] == '.') {
	    dir[i-2] = '\0';
    }

    return rv;
}


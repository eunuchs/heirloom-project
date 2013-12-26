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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)nse.cc 1.15 06/12/12
 */

/*	from OpenSolaris "nse.cc	1.15	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)nse.cc	1.2 (gritter) 01/07/07
 */

#ifdef NSE

/*
 * Included files
 */
#include <mk/defs.h>
#include <mksh/macro.h>		/* expand_value() */
#include <mksh/misc.h>		/* get_prop() */

/*
 * This file does some extra checking on behalf of the NSE.
 * It does some stuff that is analogous to lint in that it
 * looks for things which may be legal but that give the NSE
 * trouble.  Currently it looks for:
 *	1) recursion by cd'ing to a directory specified by a 
 *	   make variable that is defined from the shell environment.
 *	2) recursion by cd'ing to a directory specified by a
 *         make variable that has backquotes in it.
 *	3) recursion to another makefile in the same directory.
 *	4) a dependency upon an SCCS file (SCCS/s.*)
 *	5) backquotes in a file name
 *	6) a make variable being defined on the command-line that
 *	   ends up affecting dependencies
 *	7) wildcards (*[) in dependencies
 *	8) recursion to the same directory
 *	9) potential source files on the left-hand-side so
 *	   that they appear as derived files
 *
 * Things it should look for:
 *	1) makefiles that are symlinks (why are these a problem?)
 */

#define TARG_SUFX	"/usr/nse/lib/nse_targ.sufx"

typedef	struct _Nse_suffix	*Nse_suffix, Nse_suffix_rec;
struct _Nse_suffix {
	wchar_t			*suffix;	/* The suffix */
	struct _Nse_suffix	*next;		/* Linked list */
};
static Nse_suffix	sufx_hdr;
static int		our_exit_status;

static void		nse_warning(void);
static Boolean		nse_gettoken(wchar_t **, wchar_t *);

/*
 * Given a command that has just recursed to a sub make
 * try to determine if it cd'ed to a directory that was
 * defined by a make variable imported from the shell
 * environment or a variable with backquotes in it.
 * This routine will find something like:
 *	cd $(DIR); $(MAKE)
 * where DIR is imported from the shell environment.
 * However it well not find:
 *	CD = cd
 *		$(CD) $(DIR); $(MAKE)
 * or
 *	CD = cd $(DIR)
 *		$(CD); $(MAKE)
 *
 * This routine also checks for recursion to the same
 * directory.
 */
void
nse_check_cd(Property prop)
{
	wchar_t		tok[512];
	wchar_t		*p;
	wchar_t		*our_template;
	int		len;
	Boolean		cd;
#ifdef SUNOS4_AND_AFTER
	String_rec	string;
#else
	String		string;
#endif
	Name		name;
	Name		target;
	struct	Line	*line;
	struct	Recursive *r;
	Property	recurse;
	wchar_t		strbuf[STRING_BUFFER_LENGTH];
	wchar_t		tmpbuf[STRING_BUFFER_LENGTH];

#ifdef LTEST
	printf("In nse_check_cd, nse = %d, nse_did_recursion = %d\n", nse, nse_did_recursion);
#endif
#ifdef SUNOS4_AND_AFTER
	if (!nse_did_recursion || !nse) {
#else
	if (is_false(nse_did_recursion) || is_false(flag.nse)) {
#endif
#ifdef LTEST
		printf ("returning,  nse = %d,  nse_did_recursion = %d\n", nse, nse_did_recursion);
#endif
		return;
	}
	line = &prop->body.line;
#ifdef LTEST
	printf("string = %s\n", line->command_template->command_line->string_mb);
#endif

	wscpy(tmpbuf, line->command_template->command_line->string);
	our_template = tmpbuf;
	cd = false;
	while (nse_gettoken(&our_template, tok)) {
#ifdef LTEST
		printf("in gettoken loop\n");
#endif
#ifdef SUNOS4_AND_AFTER
		if (IS_WEQUAL(tok, (wchar_t *) "cd")) {
#else
		if (is_equal(tok, "cd")) {
#endif
			cd = true;
		} else if (cd && tok[0] == '$') {
			nse_backquote_seen = NULL;
			nse_shell_var_used = NULL;
			nse_watch_vars = true;
#ifdef SUNOS4_AND_AFTER
			INIT_STRING_FROM_STACK(string, strbuf);
			name = GETNAME(tok, FIND_LENGTH);
#else
			init_string_from_stack(string, strbuf);
			name = getname(tok, FIND_LENGTH);
#endif
			expand_value(name, &string, false);
			nse_watch_vars = false;

#ifdef LTEST
			printf("cd = %d, tok = $\n", cd);
#endif
			/*
			 * Try to trim tok to just
			 * the variable.
			 */
			if (nse_shell_var_used != NULL) {
				nse_warning();
				fprintf(stderr, "\tMake invoked recursively by cd'ing to a directory\n\tdefined by the shell environment variable %s\n\tCommand line: %s\n",
				    nse_shell_var_used->string_mb,
				    line->command_template->command_line->string_mb);
			}
			if (nse_backquote_seen != NULL) {
				nse_warning();
				fprintf(stderr, "\tMake invoked recursively by cd'ing to a directory\n\tdefined by a variable (%s) with backquotes in it\n\tCommand line: %s\n",
				    nse_backquote_seen->string_mb,
				    line->command_template->command_line->string_mb);
			}
			cd = false;
		} else if (cd && nse_backquotes(tok)) {
			nse_warning();
			fprintf(stderr, "\tMake invoked recursively by cd'ing to a directory\n\tspecified by a command in backquotes\n\tCommand line: %s\n",
			    line->command_template->command_line->string_mb);
			cd = false;
		} else {
			cd = false;
		}
	}

	/*
	 * Now check for recursion to ".".
	 */
	if (primary_makefile != NULL) {
		target = prop->body.line.target;
		recurse = get_prop(target->prop, recursive_prop);
		while (recurse != NULL) {
			r = &recurse->body.recursive;
#ifdef SUNOS4_AND_AFTER
			if (IS_WEQUAL(r->directory->string, (wchar_t *) ".") &&
			    !IS_WEQUAL(r->makefiles->name->string, 
                            primary_makefile->string)) {
#else
			if (is_equal(r->directory->string, ".") &&
			    !is_equal(r->makefiles->name->string, 
			    primary_makefile->string)) {
#endif
				nse_warning();
				fprintf(stderr, "\tRecursion to makefile `%s' in the same directory\n\tCommand line: %s\n",
				    r->makefiles->name->string_mb,
				    line->command_template->command_line->string_mb);
			}
			recurse = get_prop(recurse->next, recursive_prop);
		}
	}
}

/*
 * Print an NSE make warning line.
 * If the -P flag was given then consider this a fatal
 * error, otherwise, just a warning.
 */
static void
nse_warning(void)
{
#ifdef SUNOS4_AND_AFTER
	if (report_dependencies_level > 0) {
#else
	if (is_true(flag.report_dependencies)) {
#endif
		our_exit_status = 1;
	}
	if (primary_makefile != NULL) {
		fprintf(stderr, "make: NSE warning from makefile %s/%s:\n",
			get_current_path(), primary_makefile->string_mb);
	} else {
		fprintf(stderr, "make: NSE warning from directory %s:\n",
			get_current_path());
	}
}

/*
 * Get the next whitespace delimited token pointed to by *cp.
 * Return it in tok.
 */
static Boolean
nse_gettoken(wchar_t **cp, wchar_t *tok)
{
	wchar_t		*to;
	wchar_t		*p;

	p = *cp;
	while (*p && iswspace(*p)) {
		p++;
	}
	if (*p == '\0') {
		return false;
	}
	to = tok;
	while (*p && !iswspace(*p)) {
		*to++ = *p++;
	}
	if (*p == '\0') {
		return false;
	}
	*to = '\0';
	*cp = p;
	return true;
}

/*
 * Given a dependency and a target, see if the dependency
 * is an SCCS file.  Check for the last component of its name
 * beginning with "s." and the component before that being "SCCS".
 * The NSE does not consider a source file to be derived from
 * an SCCS file.
 */
void
nse_check_sccs(wchar_t *targ, wchar_t *dep)
{
	wchar_t		*slash;
	wchar_t		*p;

#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
#ifdef SUNOS4_AND_AFTER
	slash = wsrchr(dep, (int) slash_char); 
#else
	slash = rindex(dep, '/');
#endif
	if (slash == NULL) {
		return;
	}
	if (slash[1] != 's' || slash[2] != '.') {
		return;
	}

	/*
	 * Find the next to last filename component.
	 */
	for (p = slash - 1; p >= dep; p--) {
		if (*p == '/') {
			break;
		}
	}
	p++;
#ifdef SUNOS4_AND_AFTER
	MBSTOWCS(wcs_buffer, "SCCS/");
	if (IS_WEQUALN(p, wcs_buffer, wslen(wcs_buffer))) {
#else
	if (is_equaln(p, "SCCS/", 5)) {
#endif
		nse_warning();
		WCSTOMBS(mbs_buffer, targ);
		WCSTOMBS(mbs_buffer2, dep);
		fprintf(stderr, "\tFile `%s' depends upon SCCS file `%s'\n",
			mbs_buffer, mbs_buffer2);
	}
	return;
}

/*
 * Given a filename check to see if it has 2 backquotes in it.
 * Complain about this because the shell expands the backquotes
 * but make does not so the files always appear to be out of date.
 */
void
nse_check_file_backquotes(wchar_t *file)
{
#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
	if (nse_backquotes(file)) {
		nse_warning();
		WCSTOMBS(mbs_buffer, file);
		fprintf(stderr, "\tFilename \"%s\" has backquotes in it\n",
			mbs_buffer);
	}
}

/*
 * Return true if the string has two backquotes in it.
 */
Boolean
nse_backquotes(wchar_t *str)
{
	wchar_t		*bq;

#ifdef SUNOS4_AND_AFTER
	bq = wschr(str, (int) backquote_char);
	if (bq) {
		bq = wschr(&bq[1], (int) backquote_char);
#else
	bq = index(str, '`');
	if (bq) {
		bq = index(&bq[1], '`');
#endif
		if (bq) {
			return true;
		}
	}
	return false;
}

/*
 * A macro that was defined on the command-line was found to affect the
 * set of dependencies.  The NSE "target explode" will not know about
 * this and will not get the same set of dependencies.
 */
void
nse_dep_cmdmacro(wchar_t *macro)
{
#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
	nse_warning();
	WCSTOMBS(mbs_buffer, macro);
	fprintf(stderr, "\tVariable `%s' is defined on the command-line and\n\taffects dependencies\n",
		mbs_buffer);
}

/*
 * A macro that was defined on the command-line was found to
 * be part of the argument to a cd before a recursive make.
 * This make cause the make to recurse to different places
 * depending upon how it is invoked.
 */
void
nse_rule_cmdmacro(wchar_t *macro)
{
#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
	nse_warning();
	WCSTOMBS(mbs_buffer, macro);
	fprintf(stderr, "\tMake invoked recursively by cd'ing to a directory\n\tspecified by a variable (%s) defined on the command-line\n",
		mbs_buffer);
}

/*
 * A dependency has been found with a wildcard in it.
 * This causes the NSE problems because the set of dependencies
 * can change without changing the Makefile.
 */
void
nse_wildcard(wchar_t *targ, wchar_t *dep)
{
#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
	nse_warning();
	WCSTOMBS(mbs_buffer, targ);
	WCSTOMBS(mbs_buffer2, dep);
	fprintf(stderr, "\tFile `%s' has a wildcard in dependency `%s'\n",
		mbs_buffer, mbs_buffer2);
}

/*
 * Read in the list of suffixes that are interpreted as source
 * files.
 */
void
nse_init_source_suffixes(void)
{
	FILE		*fp;
	wchar_t		suffix[100];
	Nse_suffix	sufx;
	Nse_suffix	*bpatch;

	fp = fopen(TARG_SUFX, "r");
	if (fp == NULL) {
		return;
	}
	bpatch = &sufx_hdr;
	while (fscanf(fp, "%s %*s", suffix) == 1) {
#ifdef SUNOS4_AND_AFTER
		sufx = ALLOC(Nse_suffix);  
		sufx->suffix = wscpy(ALLOC_WC(wslen(suffix) + 1), suffix);
#else
		sufx = alloc(Nse_suffix);
		sufx->suffix = strcpy(malloc(strlen(suffix) + 1), suffix);
#endif
		sufx->next = NULL;
		*bpatch = sufx;
		bpatch = &sufx->next;
	}
	fclose(fp);
}

/*
 * Check if a derived file (something with a dependency) appears
 * to be a source file (by its suffix) but has no rule to build it.
 * If so, complain.
 *
 * This generally arises from the old-style of make-depend that
 * produces:
 *	foo.c:	foo.h
 */
void
nse_check_derived_src(Name target, wchar_t *dep, Cmd_line command_template)
{
	Nse_suffix	sufx;
	wchar_t		*suffix;
	wchar_t		*depsufx;

#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
#ifdef SUNOS4_AND_AFTER
	if (target->stat.is_derived_src) {
#else
	if (is_true(target->stat.is_derived_src)) {
#endif
		return;
	}
	if (command_template != NULL) {
		return;
	}
#ifdef SUNOS4_AND_AFTER
	suffix = wsrchr(target->string, (int) period_char ); 
#else
	suffix = rindex(target->string, '.');
#endif
	if (suffix != NULL) {
		for (sufx = sufx_hdr; sufx != NULL; sufx = sufx->next) {
#ifdef SUNOS4_AND_AFTER
			if (IS_WEQUAL(sufx->suffix, suffix)) {
#else
			if (is_equal(sufx->suffix, suffix)) {
#endif
				nse_warning();
				WCSTOMBS(mbs_buffer, dep);
				fprintf(stderr, "\tProbable source file `%s' appears as a derived file\n\tas it depends upon file `%s', but there is\n\tno rule to build it\n",
					target->string_mb, mbs_buffer);
				break;
			}
		}
	}
}

/*
 * See if a target is a potential source file and has no
 * dependencies and no rule but shows up on the right-hand
 * side.  This tends to occur from old "make depend" output.
 */
void
nse_check_no_deps_no_rule(Name target, Property line, Property command)
{
	Nse_suffix	sufx;
	wchar_t		*suffix;

#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
#ifdef SUNOS4_AND_AFTER
	if (target->stat.is_derived_src) {
#else
	if (is_true(target->stat.is_derived_src)) {
#endif
		return;
	}
	if (line != NULL && line->body.line.dependencies != NULL) {
		return;
	}
#ifdef SUNOS4_AND_AFTER
	if (command->body.line.sccs_command) {
#else
	if (is_true(command->body.line.sccs_command)) {
#endif
		return;
	}
#ifdef SUNOS4_AND_AFTER
	suffix = wsrchr(target->string, (int) period_char); 
#else
	suffix = rindex(target->string, '.');
#endif
	if (suffix != NULL) {
		for (sufx = sufx_hdr; sufx != NULL; sufx = sufx->next) {
#ifdef SUNOS4_AND_AFTER
			if (IS_WEQUAL(sufx->suffix, suffix)) {
#else
			if (is_equal(sufx->suffix, suffix)) {
#endif
				if (command->body.line.command_template == NULL) {
					nse_warning();
					fprintf(stderr, "\tProbable source file `%s' appears as a derived file because\n\tit is on the left-hand side, but it has no dependencies and\n\tno rule to build it\n",
						target->string_mb);
				}
			}
		}
	} 
} 

/*
 * Detected a situation where a recursive make derived a file
 * without using a makefile.
 */
void
nse_no_makefile(Name target)
{
#ifdef SUNOS4_AND_AFTER
	if (!nse) {
#else
	if (is_false(flag.nse)) {
#endif
		return;
	}
	nse_warning();
	fprintf(stderr, "Recursive make to derive %s did not use a makefile\n",
		target->string_mb);
}

/*
 * Return the NSE exit status.
 * If the -P flag was given then a warning is considered fatal
 */
int
nse_exit_status(void)
{
	return our_exit_status;
}
#endif

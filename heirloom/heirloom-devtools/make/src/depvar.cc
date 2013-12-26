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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)depvar.cc 1.14 06/12/12
 */

/*	from OpenSolaris "depvar.cc	1.14	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)depvar.cc	1.3 (gritter) 01/13/07
 */

/*
 * Included files
 */
#include <mk/defs.h>
#include <mksh/misc.h>		/* getmem() */

/*
 * This file deals with "Dependency Variables".
 * The "-V var" command line option is used to indicate
 * that var is a dependency variable.  Used in conjunction with
 * the -P option the user is asking if the named variables affect
 * the dependencies of the given target.
 */

struct _Depvar {
	Name		name;		/* Name of variable */
	struct _Depvar	*next;		/* Linked list */
	Boolean		cmdline;	/* Macro defined on the cmdline? */
};

typedef	struct _Depvar	*Depvar;

static	Depvar		depvar_list;
static	Depvar		*bpatch = &depvar_list;
static	Boolean		variant_deps;

/*
 * Add a name to the list.
 */

void
depvar_add_to_list(Name name, Boolean cmdline)
{
	Depvar		dv;

#ifdef SUNOS4_AND_AFTER
	dv = ALLOC(Depvar);
#else
	dv = (Depvar) Malloc(sizeof(struct _Depvar));
#endif
	dv->name = name;
	dv->next = NULL;
	dv->cmdline = cmdline;
	*bpatch = dv;
	bpatch = &dv->next;
}

/*
 * The macro `name' has been used in either the left-hand or
 * right-hand side of a dependency.  See if it is in the
 * list.  Two things are looked for.  Names given as args
 * to the -V list are checked so as to set the same/differ
 * output for the -P option.  Names given as macro=value
 * command-line args are checked and, if found, an NSE
 * warning is produced.
 */
void
depvar_dep_macro_used(Name name)
{
	Depvar		dv;

	for (dv = depvar_list; dv != NULL; dv = dv->next) {
		if (name == dv->name) {
#ifdef NSE
#ifdef SUNOS4_AND_AFTER
			if (dv->cmdline) {
#else
			if (is_true(dv->cmdline)) {
#endif
				nse_dep_cmdmacro(dv->name->string);
			}
#endif
			variant_deps = true;
			break;
		}
	}
}

#ifdef NSE
/*
 * The macro `name' has been used in either the argument
 * to a cd before a recursive make.  See if it was
 * defined on the command-line and, if so, complain.
 */
void
depvar_rule_macro_used(Name name)
{
	Depvar		dv;

	for (dv = depvar_list; dv != NULL; dv = dv->next) {
		if (name == dv->name) {
#ifdef SUNOS4_AND_AFTER
			if (dv->cmdline) {
#else
			if (is_true(dv->cmdline)) {
#endif
				nse_rule_cmdmacro(dv->name->string);
			}
			break;
		}
	}
}
#endif

/*
 * Print the results.  If any of the Dependency Variables
 * affected the dependencies then the dependencies potentially
 * differ because of these variables.
 */
void
depvar_print_results(void)
{
	if (variant_deps) {
		printf("differ\n");
	} else {
		printf("same\n");
	}
}


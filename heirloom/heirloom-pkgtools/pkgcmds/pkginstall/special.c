/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "special.c	1.3	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)special.c	1.4 (gritter) 2/25/07
 */

/*
 * This module contains code required to write special contents to
 * the contents file when a pkgadd is done on a system upgraded to use
 * the new database.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libintl.h>
#include <pkgstrct.h>
/* To get the definition of cfextra... */
#include <pkginfo.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include <pkglib.h>
#include <dbsql.h>
#include "libadm.h"
#include "libinst.h"
#include "dryrun.h"
#include "pkginstall.h"
#include "messages.h"

/*
 * This is used as the max size of a line, to read in contents file
 * lines.
 */
#define	LINESZ	8192

/*
 * strcompare
 *
 * This function is used by qsort to sort an array of special contents
 * rule strings.  This array must be sorted to facilitate efficient
 * rule processing.  See qsort(3c) regarding qsort compare functions.
 */
static int
strcompare(const void *pv1, const void *pv2)
{
	char **ppc1 = (char **)pv1;
	char **ppc2 = (char **)pv2;
	int i = strcmp(*ppc1, *ppc2);
	if (i < 0)
		return (-1);
	if (i > 0)
		return (1);
	return (0);
}

/*
 * pathcmp
 *
 * Compare a path to a cfent.  It will match either if the path is
 * equal to the cfent path, or if the cfent is a symbolic or link
 * and *that* matches.
 *
 *    path	a path
 *    pent      a contents entry
 *
 * Returns: as per strcmp
 * Side effects: none.
 */
static int
pathcmp(const char *pc, const struct cfent *pent)
{
	int i;
	if ((pent->ftype == 's' || pent->ftype == 'l') &&
	    pent->ainfo.local) {
		char *p, *q;
		if ((p = strstr(pc, "=")) == NULL) {

			i = strcmp(pc, pent->path);

			/* A path without additional chars strcmp's to less */
			if (i == 0)
				i = -1;

		} else {
			/* Break the link path into two pieces. */
			*p = '\0';

			/* Compare the first piece. */
			i = strcmp(pc, pent->path);

			/* If equal we must compare the second piece. */
			if (i == 0) {
				q = p + 1;
				i = strcmp(q, pent->ainfo.local);
			}

			/* Restore the link path. */
			*p = '=';
		}
	} else {
		i = strcmp(pc, pent->path);
	}

	return (i);
}


/*
 * match
 *
 * This function determines whether a file name (pc) matches a rule
 * from the special contents file (pcrule).  We assume that neither
 * string is ever NULL.
 *
 * Return: 1 on match, 0 on no match.
 * Side effects: none.
 */
static int
match(const char *pc, char *pcrule)
{
	int n = strlen(pcrule);
	int wild = 0;
	if (pcrule[n - 1] == '*') {
		wild = 1;
		pcrule[n - 1] = '\0';
	}

	if (!wild) {
		if (fnmatch(pc, pcrule, FNM_PATHNAME) == 0 ||
		    fnmatch(pc, pcrule, 0) == 0)
		return (1);
	} else {
		int j;
		j = strncmp(pc, pcrule, n - 1);
		pcrule[n - 1] = '*';
		if (j == 0)
			return (1);
	}
	return (0);
}

/*
 * search_special_contents
 *
 * This function assumes that a series of calls will be made requesting
 * whether a given path matches the special contents rules or not.  We
 * assume that
 *
 *   a) the special_contents array is sorted
 *   b) the calls will be made with paths in a sorted order
 *
 * Given that, we can keep track of where the last search ended and
 * begin the new search at that point.  This reduces the cost of a
 * special contents matching search to O(n) from O(n^2).
 *
 *   ppcSC  A pointer to an array of special contents obtained via
 *	  get_special_contents().
 *   path   A path: determine whether it matches the special
 *	  contents rules or not.
 *   piX    The position in the special_contents array we have already
 *	  arrived at through searching.  This must be initialized to
 *	  zero before initiating a series of search_special_contents
 *	  operations.
 *
 * Example:
 * {
 *	int i = 0, j;
 *	char **ppSC = NULL;
 *	if (get_special_contents(NULL, &ppcSC) != 0) exit(1);
 *	for (j = 0; paths != NULL && paths[j] != NULL; j++) {
 *		if (search_special_contents(ppcSC, path[j], &i)) {
 *			do_something_with_special_path(path[j]);
 *		}
 *	}
 * }
 *
 * Return: 1 if there is a match, 0 otherwise.
 * Side effects: The value of *piX will be set between calls to this
 *    function.  To make this function thread safe, use search arrays.
 *    Also:  Nonmatching entries are eliminated, set to NULL.
 */
static int
search_special_contents(char **ppcSC, const char *pcpath, int *piX, int max)
{
	int wild;
	if (ppcSC == NULL || *piX == max)
		return (0);

	while (*piX < max) {

		int j, k;
		if (ppcSC[*piX] == NULL) {
			(*piX)++;
			continue;
		}

		j = strlen(ppcSC[*piX]);
		k = strcmp(pcpath, ppcSC[*piX]);
		wild = (ppcSC[*piX][j - 1] == '*');

		/*
		 * Depending on whether the path string compared with the
		 * rule, we take different actions.  If the path is less
		 * than the rule, we keep the rule.  If the path equals
		 * the rule, we advance the rule (as long as the rule is
		 * not a wild card).  If the path is greater than the rule,
		 * we have to advance the rule list until we are less or equal
		 * again.  This way we only have to make one pass through the
		 * rules, as we make one pass through the path strings.  We
		 * assume that the rules and the path strings are sorted.
		 */
		if (k < 0) {

			if (wild == 0)
				return (0);

			if (match(pcpath, ppcSC[*piX]))
				return (1);
			break;

		} else if (k == 0) {

			int x = match(pcpath, ppcSC[*piX]);
			if (wild == 0) (*piX)++;
			return (x);

		} else {
			/* One last try. */
			if (match(pcpath, ppcSC[*piX]))
				return (1);

			/*
			 * As pcpath > ppcSC[*piX] we have passed up this
			 * rule - it cannot apply.  Therefore, we do not
			 * need to retain it.  Removing the rule will make
			 * subsequent searching more efficient.
			 */
			free(ppcSC[*piX]);
			ppcSC[*piX] = NULL;

			(*piX)++;
		}
	}
	return (0);
}

/*
 * get_special_contents
 *
 * Retrieves the special contents file entries, if they exist.  These
 * are sorted.  We do not assume the special_contents file is in sorted
 * order.
 *
 *   pcroot   The root of the install database.  If NULL assume '/'.
 *   pppcSC   A pointer to a char **.  This pointer will be set to
 *		point at NULL if there is no special_contents file or
 *		to a sorted array of strings, NULL terminated, otherwise.
 *   piMax    The # of entries in the special contents result.
 *
 * Returns:  0 on no error, nonzero on error.
 * Side effects:  the pppcSC pointer is set to point at a newly
 *   allocated array of pointers to strings..  The caller must
 *   free this buffer.  The value of *piMax is set to the # of
 *   entries in ppcSC.
 */
static int
get_special_contents(const char *pcroot, char ***pppcSC, int *piMax)
{
	int e, i;
	FILE *fp;
	char line[2048];
	char **ppc;
	char *pc = VSADMREL "/install/special_contents";
	char path[PATH_MAX];
	struct stat s;

	/* Initialize the return values. */
	*piMax = 0;
	*pppcSC = NULL;

	if (pcroot == NULL) {
		pcroot = "/";
	}

	if (pcroot[strlen(pcroot) - 1] == '/') {
		if (snprintf(path, PATH_MAX, "%s%s", pcroot, pc) >= PATH_MAX)
			return (-1);
	} else {
		if (snprintf(path, PATH_MAX, "%s/%s", pcroot, pc) >= PATH_MAX)
			return (-1);
	}

	errno = 0;
	e = stat(path, &s);
	if (e != 0 && errno == ENOENT)
		return (0); /* No special contents file.  Do nothing. */

	if (access(path, R_OK) != 0 || (fp = fopen(path, "r")) == NULL) {
		/* If there is no contents file, we have nothing to do. */
		return (0);
	}

	for (i = 0; fgets(line, 2048, fp) != NULL; i++);
	rewind(fp);
	if ((ppc = (char **)calloc(i + 1, sizeof (char *))) == NULL) {
		progerr(SPECIAL_MALLOC, strerror(errno));
		return (1);
	}

	for (i = 0; fgets(line, 2048, fp) != NULL; ) {
		int n;
		if (line[0] == '#' || line[0] == ' ' || line[0] == '\n' ||
		    line[0] == '\t' || line[0] == '\r')
			continue;
		n = strlen(line);
		if (line[n - 1] == '\n')
			line[n - 1] = '\0';
		ppc[i++] = strdup(line);
	}

	qsort(ppc, i, sizeof (char *), strcompare);

	*pppcSC = ppc;
	*piMax = i;
	return (0);
}

/*
 * free_special_contents
 *
 * This function frees special_contents which have been allocated using
 * get_special_contents.
 *
 *   pppcSC    A pointer to a buffer allocated using get_special_contents.
 *   max       The number of entries in *pppcSC, some might be NULL already.
 *
 * Result: None.
 * Side effects: Frees memory allocated using get_special_contents and
 *    sets the pointer passed in to NULL.
 */
static void
free_special_contents(char ***pppcSC, int max)
{
	int i;
	char **ppc = NULL;
	if (*pppcSC == NULL)
		return;

	ppc = *pppcSC;
	for (i = 0; ppc != NULL && i < max; i++)
		if (ppc[i] != NULL)
			free(ppc[i]);

	if (ppc != NULL)
		free(ppc);

	*pppcSC = NULL;
}

/*
 * get_path
 *
 * Return the first field of a string delimited by a space.
 *
 *   pcline	A line from the contents file.
 *
 * Return: NULL if an error.  Otherwise a string allocated by this
 *   function.  The caller must free the string.
 * Side effects: none.
 */
static char *
get_path(const char *pcline)
{
	int i = strcspn(pcline, " ");
	char *pc = NULL;
	if (i <= 1 || (pc = (char *)calloc(i + 1, 1)) == NULL)
		return (NULL);
	(void) memcpy(pc, pcline, i);
	return (pc);
}

/*
 * putcfile_wrapper
 *
 * Normally pkginstall does not write ainfo.path into the contents file.
 * Temporarily set this field to NULL to perform putcfile, then reset it
 * to its previous value to prevent side effects.
 *
 *    pent	The entry to output.
 *    fp	The file stream to output to.
 *
 * Returns the result from putcfile.
 * Side effects: The entry is output as text to the stream fp.
 */
static int
putcfile_wrapper(struct cfent *pent, FILE *fp)
{
	char *pc = pent->ainfo.local;
	int i;
	if (pc != NULL && pent->ftype == 'f')
		pent->ainfo.local = NULL;
	i = putcfile(pent, fp);
	pent->ainfo.local = pc;
	return (i);
}

/*
 * generate_special_contents_rules
 *
 * This procedure will generate an array of integers which will be a mask
 * to apply to the ppcfextra array.  If set to 1, then the content must be
 * added to the contents file.  Otherwise it will not be:  The old contents
 * file will be used for this path value, if one even exists.
 *
 *    ient	The number of ppcfextra contents installed.
 *    ppcfextra	The contents installed.
 *    ppcSC	The rules (special contents)
 *    max	The number of special contents rules.
 *    ppcfextra	The array of integer values, determining whether
 *		individual ppcfextra items match special contents rules.
 *		This array will be created and set in this function and
 *		returned.
 *
 * Return: 0 success, nonzero failure
 * Side effects: allocates an array of integers that the caller must free.
 */
static int
generate_special_contents_rules(int ient, struct cfextra **ppcfextra,
    char **ppcSC, int max, int **ppiIndex)
{
	int i, j;
	int *pi = (int *)calloc(ient, sizeof (int));
	if (pi == NULL) {
		progerr(SPECIAL_MALLOC, strerror(errno));
		return (1);
	}

	/*
	 * For each entry in ppcfextra, check if it matches a rule.
	 * If it does not, set the entry in the index to -1.
	 */
	for (i = 0, j = 0; i < ient && j < max; i++) {
		if (search_special_contents(ppcSC, ppcfextra[i]->cf_ent.path,
		    &j, max) == 1) {
			pi[i] = 1;

		} else {
			pi[i] = 0;
		}
	}

	/*
	 * In case we ran out of rules before contents, we will not use
	 * those contents.  Make sure these contents are set to 0 and
	 * will not be copied from the ppcfextra array into the contents
	 * file.
	 */
	for (i = i; i < ient; i++)
		pi[i] = 0;

	*ppiIndex = pi;
	return (0);
}

/*
 * -----------------------------------------------------------------------
 * Externally visible function.
 */

/*
 * special_contents_add
 *
 * Given a set of entries to add and an alternate root, this function
 * will do everything required to ensure that the entries are added to
 * the contents file if they are listed in the special_contents
 * file.  The contents file will get changed only in the case that the
 * entire operation has succeeded.
 *
 *  ient	The number of entries.
 *  pcfent	The entries to add.
 *  pcroot	The alternate install root.  Could be NULL.  In this
 *		case, assume root is '/'
 *
 * Result: 0 on success, nonzero on failure.  If an error occurs, an
 *    error string will get output to standard error alerting the user.
 * Side effects: The contents file may change as a result of this call,
 *    such that lines in the in the file will be changed or removed.
 *    If the call fails, a t.contents file may be left behind.  This
 *    temporary file should be removed subsequently.
 */
int
special_contents_add(int ient, struct cfextra **ppcfextra, const char *pcroot)
{
	int result = 0;		/* Assume we will succeed.  Return result. */
	char **ppcSC = NULL;	/* The special contents rules, sorted. */
	int i;			/* Index into contents & special contents */
	FILE *fpi = NULL,	/* Input of contents file */
	    *fpo = NULL;	/* Output to temp contents file */
	char cpath[PATH_MAX],	/* Contents file path */
	    tcpath[PATH_MAX];	/* Temp contents file path */
	const char *pccontents = VSADMREL "/install/contents";
	const char *pctcontents = VSADMREL "/install/t.contents";
	char line[LINESZ];	/* Reads in and writes out contents lines. */
	time_t t;		/* We use this to create a timestamp comment */
	int max;		/* Max number of special contents entries. */
	int no_contents = 0;	/* Set to 1 only when contents file absent */
	int *piIndex = NULL;	/* An array of indexes of cfent which match */
	int writes = 0;		/* Keep track of writes to spare work at end */

	cpath[0] = tcpath[0] = '\0';

	if (ient == 0 || ppcfextra == NULL || ppcfextra[0] == NULL) {
		goto add_done;
	}

	if ((get_special_contents(pcroot, &ppcSC, &max)) != 0) {
		result = 1;
		goto add_done;
	}

	/* Check if there are no special contents actions to take. */
	if (ppcSC == NULL) {
		goto add_done;
	}

	if (pcroot == NULL) pcroot = "/";

	if (pcroot[strlen(pcroot) - 1] == '/') {
		if (snprintf(cpath, PATH_MAX, "%s%s", pcroot, pccontents)
		    >= PATH_MAX ||
		    snprintf(tcpath, PATH_MAX, "%s%s", pcroot, pctcontents)
		    >= PATH_MAX) {
			progerr(SPECIAL_INPUT);
			result = -1;
			goto add_done;
		}
	} else {
		if (snprintf(cpath, PATH_MAX, "%s/%s", pcroot, pccontents)
		    >= PATH_MAX ||
		    snprintf(tcpath, PATH_MAX, "%s/%s", pcroot, pctcontents)
		    >= PATH_MAX) {
			progerr(SPECIAL_INPUT);
			result = -1;
			goto add_done;
		}
	}

	/* Open the temporary contents file to write, contents to read. */
	if (access(cpath, F_OK) != 0 && errno == ENOENT) {
		no_contents = 1;
	} else {
		if (access(cpath, R_OK) != 0) {
			progerr(SPECIAL_ACCESS, strerror(errno));
			result = 1;
			goto add_done;
		}
	}

	if (no_contents == 0 && (fpi = fopen(cpath, "r")) == NULL) {
		progerr(SPECIAL_ACCESS, strerror(errno));
		result = 1;
		goto add_done;
	}

	if ((fpo = fopen(tcpath, "w")) == NULL) {
		progerr(SPECIAL_ACCESS, strerror(errno));
		result = 1;
		goto add_done;
	}


	if (generate_special_contents_rules(ient, ppcfextra, ppcSC, max,
	    &piIndex) < 0) {
		result = 1;
		goto add_done;
	}

	if (no_contents) {
		for (i = 0; i < ient; i++) {
			if (piIndex[i]) {
				writes++;
				if (putcfile_wrapper(&(ppcfextra[i]->cf_ent),
				    fpo)) {
					progerr(SPECIAL_ACCESS,
						strerror(errno));
					result = 1;
					goto add_done;
				}
			}
		}
	}

	/*
	 * Copy contents to t.contents unless there is an entry in
	 * ppcfextra array (added package contents) which matches an
	 * entry in ppcSC (the special_contents rules).  In this case,
	 * write the new entry in ppcfextra in place of the entry in
	 * the current contents file.
	 *
	 * Since both the contents and the rules are sorted, we can
	 * make a single efficient pass.
	 */
	(void) memset(line, 0, LINESZ);

	for (i = 0; no_contents == 0 && fgets(line, LINESZ, fpi) != NULL; ) {

		char *pcpath = NULL;

		/*
		 * Note:  This could be done better:  We should figure out
		 * which are the last 2 lines and only trim those off.
		 * This will suffice to do this and will only be done as
		 * part of special_contents handling.
		 */
		if (line[0] == '#')
			continue; /* Do not copy the final 2 comment lines */

		pcpath = get_path(line);

		/*
		 * Compare the item against the rules.  All the rules which
		 * are still listed apply to items in the pkgmap we are
		 * adding.
		 *
		 * In the case where pcpath is before the next rule, search...
		 * will return 0 and just copy the contents file line to the
		 * temporary contents file.  If pcpath matches a rule, we
		 * will replace the contents entry in the contents file with
		 * the appropriate indexed item in the ppcfextra array.
		 * If a rule applies (with a corresponding contents entry
		 * which does not correspond to the contents file, it is
		 * inserted into the contents file.
		 */
		if (pcpath != NULL && i < ient) {

			int k = 0;

			/* Skip all the next non-special content. */
			while (piIndex[i] == 0)
				i++;

			if (i < ient)
				k = pathcmp(pcpath, &(ppcfextra[i]->cf_ent));

			if (k < 0 || i >= ient) {
				/* Just copy contents -> t.contents */
				/*EMPTY*/
			} else if (k == 0) {

				writes++;
				if (putcfile_wrapper(&(ppcfextra[i]->cf_ent),
				    fpo)) {
					progerr(SPECIAL_ACCESS,
						strerror(errno));
					result = 1;
					goto add_done;
				}

				i++; /* We've used this rule, move on. */

				free(pcpath);
				(void) memset(line, 0, LINESZ);
				continue;
			} else while (i < ient) {

				/*
				 * This is a complex case:  The content entry
				 * is further along alphabetically than the
				 * rule.  Insert all rules which apply until
				 * the contents file entry is greater than
				 * the current content entry, or the rules are
				 * used up.
				 */

				if (piIndex[i] == 0) {
					i++;
					continue;
				} else if ((k = pathcmp(pcpath,
					&(ppcfextra[i]->cf_ent))) >= 0) {

					writes++;
					if (putcfile_wrapper(
					    &(ppcfextra[i]->cf_ent), fpo)) {
						progerr(SPECIAL_ACCESS,
							strerror(errno));
						result = 1;
						goto add_done;
					}

					i++; /* move on to the next rule. */

					if (k == 0) {
						free(pcpath);
						(void) memset(line, 0, LINESZ);
						break;
					}
				} else {
					break;
				}
			}
			/*
			 * In this case we exit the loop with a match.
			 * We must avoid copying the old content value.
			 */
			if (k == 0)
				continue;
		}

		free(pcpath);
		pcpath = NULL;

		if (fprintf(fpo, "%s", line) < 0) {
			progerr(SPECIAL_ACCESS, strerror(errno));
			result = 1;
			break;
		}

		(void) memset(line, 0, LINESZ);

	}

	/*
	 * There may be more entries to add.  The package installed may have
	 * entries which occur after all the entries in the contents file
	 * alphabetically.  In this case, write out all matching entries.
	 */
	if (no_contents == 0 && i < ient) {
		for (i = i; i < ient; i++) {
			if (piIndex[i] == 1) {
				writes++;
				if (putcfile_wrapper(&(ppcfextra[i]->cf_ent),
				    fpo)) {
					progerr(SPECIAL_ACCESS,
						strerror(errno));
						result = 1;
						goto add_done;
				}
			}
		}
	}

	t = time(NULL);
	(void) fprintf(fpo, "# Last modified by pkginstall\n");
	(void) fprintf(fpo, "# %s", ctime(&t));

add_done:
	free_special_contents(&ppcSC, max);

	if (piIndex != NULL)
		free(piIndex);

	if (fpi != NULL)
		(void) fclose(fpi);

	if (fpo != NULL)
		(void) fclose(fpo);

	if (result == 0 && writes > 0) {
		if (tcpath[0] != '\0' && cpath[0] != '\0' &&
		    rename(tcpath, cpath) != 0) {
			progerr(SPECIAL_ACCESS, strerror(errno));
			result = 1;
		}
	} else {
		if (tcpath[0] != '\0' && remove(tcpath) != 0) {
			/* No error message, this only leaves crud. */
#ifndef NDEBUG
			(void) printf("DEBUG WARNING: "
			    "could not remove temporary contents file\n");
#endif /* NDEBUG */

		}
	}

	return (result);
}

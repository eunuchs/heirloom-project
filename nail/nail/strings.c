/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)strings.c	2.6 (gritter) 3/4/06";
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * String allocation routines.
 * Strings handed out here are reclaimed at the top of the command
 * loop each time, so they need not be freed.
 */

#include "rcv.h"
#include "extern.h"

/*
 * Allocate size more bytes of space and return the address of the
 * first byte to the caller.  An even number of bytes are always
 * allocated so that the space will always be on a word boundary.
 * The string spaces are of exponentially increasing size, to satisfy
 * the occasional user with enormous string size requests.
 */

void *
salloc(size_t size)
{
	char *t;
	int s;
	struct strings *sp;
	int string_index;

	s = size;
	s += (sizeof (char *) - 1);
	s &= ~(sizeof (char *) - 1);
	string_index = 0;
	for (sp = &stringdope[0]; sp < &stringdope[NSPACE]; sp++) {
		if (sp->s_topFree == NULL && (STRINGSIZE << string_index) >= s)
			break;
		if (sp->s_nleft >= s)
			break;
		string_index++;
	}
	if (sp >= &stringdope[NSPACE])
		panic(catgets(catd, CATSET, 195, "String too large"));
	if (sp->s_topFree == NULL) {
		string_index = sp - &stringdope[0];
		sp->s_topFree = smalloc(STRINGSIZE << string_index);
		sp->s_nextFree = sp->s_topFree;
		sp->s_nleft = STRINGSIZE << string_index;
	}
	sp->s_nleft -= s;
	t = sp->s_nextFree;
	sp->s_nextFree += s;
	return(t);
}

void *
csalloc(size_t nmemb, size_t size)
{
	void	*vp;

	vp = salloc(nmemb * size);
	memset(vp, 0, nmemb * size);
	return vp;
}

/*
 * Reset the string area to be empty.
 * Called to free all strings allocated
 * since last reset.
 */
void 
sreset(void)
{
	struct strings *sp;
	int string_index;

	if (noreset)
		return;
	string_index = 0;
	for (sp = &stringdope[0]; sp < &stringdope[NSPACE]; sp++) {
		if (sp->s_topFree == NULL)
			continue;
		sp->s_nextFree = sp->s_topFree;
		sp->s_nleft = STRINGSIZE << string_index;
		string_index++;
	}
}

/*
 * Make the string area permanent.
 * Meant to be called in main, after initialization.
 */
void 
spreserve(void)
{
	struct strings *sp;

	for (sp = &stringdope[0]; sp < &stringdope[NSPACE]; sp++)
		sp->s_topFree = NULL;
}

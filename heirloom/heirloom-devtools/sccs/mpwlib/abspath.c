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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * from abspath.c 1.4 06/12/12
 */

/*	from OpenSolaris "abspath.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)abspath.c	1.3 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/mpwlib/abspath.c"	*/
static void push(char **, char **);
static char pop(char **);

char *
abspath(char *p)
{
int state;
int slashes;
char *stktop;
char *slash="/";
char *inptr;
char c;

	state = 0;
	stktop = inptr = p;
	while ((c = *inptr) != '\0')
		{
		 switch (state)
			{
			 case 0: if (c=='/') state = 1;
				 push(&inptr,&stktop);
				 break;
			 case 1: if (c=='.') state = 2;
					else state = 0;
				 push(&inptr,&stktop);
				 break;
			 case 2:      if (c=='.') state = 3;
				 else if (c=='/') state = 5;
				 else             state = 0;
				 push(&inptr,&stktop);
				 break;
			 case 3: if (c=='/') state = 4;
					else state = 0;
				 push(&inptr,&stktop);
				 break;
			 case 4: for (slashes = 0; slashes < 3; )
					{
					 if(pop(&stktop)=='/') ++slashes;
					 if (stktop < p) return((char *) -1);
					}
				 push(&slash,&stktop);
				 slash--;
				 state = 1;
				 break;
			 case 5: pop(&stktop);
				 if (stktop < p) return((char *) -1);
				 pop(&stktop);
				 if (stktop < p) return((char *) -1);
				 state = 1;
				 break;
			}
		}
	*stktop='\0';
	return(p);
}

static void 
push(char **chrptr, char **stktop)

{
	**stktop = **chrptr;
	(*stktop)++;
	(*chrptr)++;
}

static char 
pop(char **stktop)

{
char chr;
	(*stktop)--;
	chr = **stktop;
	return(chr);
}	

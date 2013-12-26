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
 * from chkid.c 1.5 06/12/12
 */

/*	from OpenSolaris "chkid.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)chkid.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/chkid.c"	*/
# include	<ctype.h>
# include	<defines.h>


int
chkid(char *line,char *idstr)
{
	register char *lp;
	register char *p;
	extern int Did_id;

	if (!Did_id && (lp = strchr(line,'%') ) )
		if (!idstr || idstr[0]=='\0' ) 
			for( ; *lp != 0; lp++) {
				if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%')
					if (isupper(lp[1]))
						switch (lp[1]) {
						case 'J':
							break;
						case 'K':
							break;
						case 'N':
							break;
						case 'O':
							break;
						case 'V':
							break;
						case 'X':
							break;
						default:
							return(Did_id++);
						}
			}
		else {
			if ( (lp = strchr(idstr,'%')) == NULL ) return(Did_id);
			for( ; *lp != 0; lp++) {
				if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%')
					if (isupper(lp[1]))
						switch (lp[1]) {
						case 'J':
							break;
						case 'K':
							break;
						case 'N':
							break;
						case 'O':
							break;
						case 'V':
							break;
						case 'X':
							break;
						default:
							Did_id++;
						}
			}
			if (!Did_id) return(Did_id); /* There's no keyword in idstr */
			Did_id = 0;
			p=idstr;
			lp=line;
			while(lp=strchr(lp,*p))
				if(!(strncmp(lp,p,strlen(p))))
					return(Did_id++);
				else
					++lp;
		}

	return(Did_id);
}

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
 * from getline.c 1.10 06/12/12
 */

/*	from OpenSolaris "getline.c"	*/

/*
 * Portions Copyright (c) 2006 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)getline.c	1.4 (gritter) 12/20/06
 */
/*	from OpenSolaris "sccs:lib/comobj/getline.c"	*/
#include	<defines.h>

/*
	Routine to read a line into the packet.  The main reason for
	it is to make sure that pkt->p_wrttn gets turned off,
	and to increment pkt->p_slnno.
*/

char *
getline(register struct packet *pkt)
{
	char	buf[DEF_LINE_SIZE];
	int	eof;
	register size_t read = 0;
	register size_t used = 0;
	register signed char *p;
	register unsigned char *u_p;

	if(pkt->p_wrttn==0)
		putline(pkt,(char *) 0);

	/* read until EOF or newline encountered */
	do {
		if (!(eof = (fgets(buf, sizeof(buf), pkt->p_iop) == NULL))) {
			read = strlen(buf);

			if ((used + read) >=  pkt->p_line_size) {
				pkt->p_line_size += sizeof(buf);
				pkt->p_line = (char*) realloc(pkt->p_line, pkt->p_line_size);
				if (pkt->p_line == NULL) {
					fatal("OUT OF SPACE (ut9)");
				}
			}

			strcpy(pkt->p_line + used, buf);
			used += read;
		}
	} while (!eof && (pkt->p_line[used-1] != '\n'));

	/* check end of file condition */
	if (eof && (used == 0)) {
		if (!pkt->p_reopen) {
			fclose(pkt->p_iop);
			pkt->p_iop = 0;
		}
		if (!pkt->p_chkeof)
			fatal("premature eof (co5)");
		if ((pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash)&0xFFFF) ) 
		    if(pkt->do_chksum && (pkt->p_uchash ^ pkt->p_ihash)&0xFFFF) 
			fatal("Corrupted file (co6)");
		if (pkt->p_reopen) {
			fseek(pkt->p_iop,0L,0);
			pkt->p_reopen = 0;
			pkt->p_slnno = 0;
			pkt->p_ihash = 0;
			pkt->p_chash = 0;
			pkt->p_uchash = 0;
			pkt->p_nhash = 0;
			pkt->p_keep = 0;
			pkt->do_chksum = 0;
		}
		return NULL;
	}

	pkt->p_wrttn = 0;
	pkt->p_slnno++;

	/* update check sum */
	for (p = (signed char *)pkt->p_line,u_p = (unsigned char *)pkt->p_line; *p; ) {
		pkt->p_chash += *p++;
		pkt->p_uchash += *u_p++;
	}

	return pkt->p_line;
}

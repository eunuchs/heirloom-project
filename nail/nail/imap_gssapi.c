/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2004
 *	Gunnar Ritter.  All rights reserved.
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
 *	This product includes software developed by Gunnar Ritter
 *	and his contributors.
 * 4. Neither the name of Gunnar Ritter nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GUNNAR RITTER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL GUNNAR RITTER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Partially derived from sample code in:
 *
 * GSS-API Programming Guide
 * Part No: 816-1331-11
 * Sun Microsystems, Inc. 4150 Network Circle Santa Clara, CA 95054 U.S.A.
 *
 * (c) 2002 Sun Microsystems
 */
/*
 * Copyright 1994 by OpenVision Technologies, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * 
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)imap_gssapi.c	1.10 (gritter) 3/4/06";
#endif
#endif /* not lint */

/*
 * Implementation of IMAP GSSAPI authentication according to RFC 1731.
 */

#ifdef	USE_GSSAPI

#ifndef	GSSAPI_REG_INCLUDE
#include <gssapi/gssapi.h>
#ifdef	GSSAPI_OLD_STYLE
#include <gssapi/gssapi_generic.h>
#define	GSS_C_NT_HOSTBASED_SERVICE	gss_nt_service_name
#endif	/* GSSAPI_OLD_STYLE */
#else	/* GSSAPI_REG_INCLUDE */
#include <gssapi.h>
#endif	/* GSSAPI_REG_INCLUDE */

static void imap_gss_error1(const char *s, OM_uint32 code, int type);
static void imap_gss_error(const char *s, OM_uint32 maj_stat,
		OM_uint32 min_stat);
static void
imap_gss_error1(const char *s, OM_uint32 code, int type)
{
	OM_uint32	maj_stat, min_stat;
	gss_buffer_desc	msg = GSS_C_EMPTY_BUFFER;
	OM_uint32	msg_ctx = 0;

	do {
		maj_stat = gss_display_status(&min_stat, code, type,
				GSS_C_NO_OID, &msg_ctx, &msg);
		if (maj_stat == GSS_S_COMPLETE) {
			fprintf(stderr, "GSS error: %s / %s\n",
					s,
					(char *)msg.value);
			if (msg.length != 0)
				gss_release_buffer(&min_stat, &msg);
		} else {
			fprintf(stderr, "GSS error: %s / unknown\n", s);
			break;
		}
	} while (msg_ctx);
}

static void
imap_gss_error(const char *s, OM_uint32 maj_stat, OM_uint32 min_stat)
{
	imap_gss_error1(s, maj_stat, GSS_C_GSS_CODE);
	imap_gss_error1(s, min_stat, GSS_C_MECH_CODE);
}

static enum okay 
imap_gss(struct mailbox *mp, char *user)
{
	gss_buffer_desc	send_tok, recv_tok, *token_ptr;
	gss_name_t	target_name;
	gss_ctx_id_t	gss_context;
	OM_uint32	maj_stat, min_stat, ret_flags;
	int	conf_state;
	struct str	in, out;
	FILE	*queuefp = NULL;
	char	*server, *cp;
	char	o[LINESIZE];
	enum okay	ok = STOP;

	if (user == NULL && (user = getuser()) == NULL)
		return STOP;
	server = salloc(strlen(mp->mb_imap_account));
	strcpy(server, mp->mb_imap_account);
	if (strncmp(server, "imap://", 7) == 0)
		server += 7;
	else if (strncmp(server, "imaps://", 8) == 0)
		server += 8;
	if ((cp = last_at_before_slash(server)) != NULL)
		server = &cp[1];
	for (cp = server; *cp; cp++)
		*cp = lowerconv(*cp&0377);
	send_tok.value = salloc(send_tok.length = strlen(server) + 6);
	snprintf(send_tok.value, send_tok.length, "imap@%s", server);
	maj_stat = gss_import_name(&min_stat, &send_tok,
			GSS_C_NT_HOSTBASED_SERVICE, &target_name);
	if (maj_stat != GSS_S_COMPLETE) {
		imap_gss_error(send_tok.value, maj_stat, min_stat);
		return STOP;
	}
	token_ptr = GSS_C_NO_BUFFER;
	gss_context = GSS_C_NO_CONTEXT;
	maj_stat = gss_init_sec_context(&min_stat,
			GSS_C_NO_CREDENTIAL,
			&gss_context,
			target_name,
			GSS_C_NO_OID,
			GSS_C_MUTUAL_FLAG|GSS_C_SEQUENCE_FLAG,
			0,
			GSS_C_NO_CHANNEL_BINDINGS,
			token_ptr,
			NULL,
			&send_tok,
			&ret_flags,
			NULL);
	if (maj_stat != GSS_S_COMPLETE && maj_stat != GSS_S_CONTINUE_NEEDED) {
		imap_gss_error("initializing GSS context", maj_stat, min_stat);
		gss_release_name(&min_stat, &target_name);
		return STOP;
	}
	snprintf(o, sizeof o, "%s AUTHENTICATE GSSAPI\r\n", tag(1));
	IMAP_OUT(o, 0, return STOP);
	/*
	 * No response data expected.
	 */
	imap_answer(mp, 1);
	if (response_type != RESPONSE_CONT)
		return STOP;
	while (maj_stat == GSS_S_CONTINUE_NEEDED) {
		/*
		 * Pass token obtained from first gss_init_sec_context() call.
		 */
		cp = memtob64(send_tok.value, send_tok.length);
		gss_release_buffer(&min_stat, &send_tok);
		snprintf(o, sizeof o, "%s\r\n", cp);
		free(cp);
		IMAP_OUT(o, 0, return STOP);
		imap_answer(mp, 1);
		if (response_type != RESPONSE_CONT)
			return STOP;
		in.s = responded_text;
		in.l = strlen(responded_text);
		mime_fromb64(&in, &out, 0);
		recv_tok.value = out.s;
		recv_tok.length = out.l;
		token_ptr = &recv_tok;
		maj_stat = gss_init_sec_context(&min_stat,
				GSS_C_NO_CREDENTIAL,
				&gss_context,
				target_name,
				GSS_C_NO_OID,
				GSS_C_MUTUAL_FLAG|GSS_C_SEQUENCE_FLAG,
				0,
				GSS_C_NO_CHANNEL_BINDINGS,
				token_ptr,
				NULL,
				&send_tok,
				&ret_flags,
				NULL);
		if (maj_stat != GSS_S_COMPLETE &&
				maj_stat != GSS_S_CONTINUE_NEEDED) {
			imap_gss_error("initializing context",
					maj_stat, min_stat);
			gss_release_name(&min_stat, &target_name);
			return STOP;
		}
		free(out.s);
	}
	/*
	 * Pass token obtained from second gss_init_sec_context() call.
	 */
	gss_release_name(&min_stat, &target_name);
	cp = memtob64(send_tok.value, send_tok.length);
	gss_release_buffer(&min_stat, &send_tok);
	snprintf(o, sizeof o, "%s\r\n", cp);
	free(cp);
	IMAP_OUT(o, 0, return STOP);
	/*
	 * First octet: bit-mask with protection mechanisms.
	 * Second to fourth octet: maximum message size in network byte order.
	 *
	 * This code currently does not care about the values.
	 */
	imap_answer(mp, 1);
	if (response_type != RESPONSE_CONT)
		return STOP;
	in.s = responded_text;
	in.l = strlen(responded_text);
	mime_fromb64(&in, &out, 0);
	recv_tok.value = out.s;
	recv_tok.length = out.l;
	maj_stat = gss_unwrap(&min_stat, gss_context, &recv_tok,
			&send_tok, &conf_state, NULL);
	if (maj_stat != GSS_S_COMPLETE) {
		imap_gss_error("unwrapping data", maj_stat, min_stat);
		return STOP;
	}
	free(out.s);
	/*
	 * First octet: bit-mask with protection mechanisms (1 = no protection
	 * 	mechanism).
	 * Second to fourth octet: maximum message size in network byte order.
	 * Fifth and following octets: user name string.
	 */
	o[0] = 1;
	o[1] = 0;
	o[2] = o[3] = 0377;
	snprintf(&o[4], sizeof o - 4, "%s", user);
	send_tok.value = o;
	send_tok.length = strlen(&o[4]) + 5;
	maj_stat = gss_wrap(&min_stat, gss_context, 0, GSS_C_QOP_DEFAULT,
			&send_tok, &conf_state, &recv_tok);
	if (maj_stat != GSS_S_COMPLETE) {
		imap_gss_error("wrapping data", maj_stat, min_stat);
		return STOP;
	}
	cp = memtob64(recv_tok.value, recv_tok.length);
	snprintf(o, sizeof o, "%s\r\n", cp);
	free(cp);
	IMAP_OUT(o, MB_COMD, return STOP);
	while (mp->mb_active & MB_COMD)
		ok = imap_answer(mp, 1);
	gss_delete_sec_context(&min_stat, &gss_context, &recv_tok);
	gss_release_buffer(&min_stat, &recv_tok);
	return ok;
}

#endif	/* USE_GSSAPI */

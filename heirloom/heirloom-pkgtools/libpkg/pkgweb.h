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
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _PKGWEB_H
#define	_PKGWEB_H

/*	from OpenSolaris "pkgweb.h	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgweb.h	1.4 (gritter) 2/24/07
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	__sun
#include <netdb.h>
#include <boot_http.h>

/* shortest backoff delay possible (in seconds) */
#define	MIN_BACKOFF	1

/* how much to increase backoff time after each failure */
#define	BACKOFF_FACTOR	2

/* Maximum amount of backoff for a heavy network or flaky server */
#define	MAX_BACKOFF	128

typedef enum {
	HTTP_REQ_TYPE_HEAD,
	HTTP_REQ_TYPE_GET
} HTTPRequestType;

typedef enum {
	OCSPSuccess,
	OCSPMem,
	OCSPParse,
	OCSPConnect,
	OCSPRequest,
	OCSPResponder,
	OCSPUnsupported,
	OCSPVerify,
	OCSPInternal,
	OCSPNoURI
} OCSPStatus;

typedef enum {
	none,
	web_http,
	web_https,
	web_ftp
} WebScheme;

typedef enum {
    WEB_OK,
    WEB_TIMEOUT,
    WEB_CONNREFUSED,
    WEB_HOSTDOWN,
    WEB_VERIFY_SETUP,
    WEB_NOCONNECT,
    WEB_GET_FAIL,
} WebStatus;

typedef struct {
	unsigned long prev_cont_length;
	unsigned long content_length;
	unsigned long cur_pos;
} DwnldData;

typedef struct {
	keystore_handle_t keystore;
	char *certfile;
	char *uniqfile;
	char *link;
	char *errstr;
	char *dwnld_dir;
	boolean_t	spool;
	void *content;
	int timeout;
	url_hport_t proxy;
	url_t url;
	DwnldData data;
	http_respinfo_t *resp;
	boot_http_ver_t *http_vers;
	http_handle_t *hps;
} WEB_SESSION;

extern boolean_t web_session_control(PKG_ERR *, char *, char *,
    keystore_handle_t, char *, unsigned short, int, int, int, char **);
extern boolean_t get_signature(PKG_ERR *, char *, struct pkgdev *,
    PKCS7 **);
extern boolean_t validate_signature(PKG_ERR *, char *, BIO *, PKCS7 *,
    STACK_OF(X509) *, url_hport_t *, int);
extern boolean_t ds_validate_signature(PKG_ERR *, struct pkgdev *, char **,
    char *, PKCS7 *, STACK_OF(X509) *, url_hport_t *, int);
extern boolean_t get_proxy_port(PKG_ERR *, char **, unsigned short *);
#endif	/* __sun */
extern boolean_t path_valid(char *);
extern void web_cleanup(void);
#ifdef __sun
extern unsigned short strip_port(char *proxy);
extern void set_web_install(void);
extern int is_web_install(void);
extern void echo_out(int, char *, ...);
extern void backoff(void);
extern void reset_backoff(void);
extern char *get_endof_string(char *, char);
extern char *get_startof_string(char *, char);
#endif	/* __sun */

#ifdef __cplusplus
}
#endif

#endif /* _PKGWEB_H */

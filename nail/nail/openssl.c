/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Copyright (c) 2002
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

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)openssl.c	1.26 (gritter) 5/26/09";
#endif
#endif /* not lint */

#include "config.h"

#ifndef	USE_NSS
#ifdef	USE_OPENSSL

#include <setjmp.h>
#include <termios.h>
#include <stdio.h>

static int	verbose;
static int	reset_tio;
static struct termios	otio;
static sigjmp_buf	ssljmp;

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include "rcv.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef	HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif	/* HAVE_ARPA_INET_H */

#include <dirent.h>

#include "extern.h"

/*
 * Mail -- a mail program
 *
 * SSL functions
 */

/*
 * OpenSSL client implementation according to: John Viega, Matt Messier,
 * Pravir Chandra: Network Security with OpenSSL. Sebastopol, CA 2002.
 */

static int	initialized;
static int	rand_init;
static int	message_number;
static int	verify_error_found;

static void sslcatch(int s);
static int ssl_rand_init(void);
static void ssl_init(void);
static int ssl_verify_cb(int success, X509_STORE_CTX *store);
static const SSL_METHOD *ssl_select_method(const char *uhp);
static void ssl_load_verifications(struct sock *sp);
static void ssl_certificate(struct sock *sp, const char *uhp);
static enum okay ssl_check_host(const char *server, struct sock *sp);
#ifdef HAVE_STACK_OF
static int smime_verify(struct message *m, int n, STACK_OF(X509) *chain,
		X509_STORE *store);
#else
static int smime_verify(struct message *m, int n, STACK *chain,
		X509_STORE *store);
#endif
static EVP_CIPHER *smime_cipher(const char *name);
static int ssl_password_cb(char *buf, int size, int rwflag, void *userdata);
static FILE *smime_sign_cert(const char *xname, const char *xname2, int warn);
#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
static enum okay load_crl1(X509_STORE *store, const char *name);
#endif
static enum okay load_crls(X509_STORE *store, const char *vfile,
		const char *vdir);

static void 
sslcatch(int s)
{
	if (reset_tio)
		tcsetattr(0, TCSADRAIN, &otio);
	siglongjmp(ssljmp, s);
}

static int 
ssl_rand_init(void)
{
	char *cp;
	int state = 0;

	if ((cp = value("ssl-rand-egd")) != NULL) {
		cp = expand(cp);
		if (RAND_egd(cp) == -1) {
			fprintf(stderr, catgets(catd, CATSET, 245,
				"entropy daemon at \"%s\" not available\n"),
					cp);
		} else
			state = 1;
	} else if ((cp = value("ssl-rand-file")) != NULL) {
		cp = expand(cp);
		if (RAND_load_file(cp, 1024) == -1) {
			fprintf(stderr, catgets(catd, CATSET, 246,
				"entropy file at \"%s\" not available\n"), cp);
		} else {
			struct stat st;

			if (stat(cp, &st) == 0 && S_ISREG(st.st_mode) &&
					access(cp, W_OK) == 0) {
				if (RAND_write_file(cp) == -1) {
					fprintf(stderr, catgets(catd, CATSET,
								247,
				"writing entropy data to \"%s\" failed\n"), cp);
				}
			}
			state = 1;
		}
	}
	return state;
}

static void 
ssl_init(void)
{
	verbose = value("verbose") != NULL;
	if (initialized == 0) {
		SSL_library_init();
		initialized = 1;
	}
	if (rand_init == 0)
		rand_init = ssl_rand_init();
}

static int
ssl_verify_cb(int success, X509_STORE_CTX *store)
{
	if (success == 0) {
		char data[256];
		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		int depth = X509_STORE_CTX_get_error_depth(store);
		int err = X509_STORE_CTX_get_error(store);

		verify_error_found = 1;
		if (message_number)
			fprintf(stderr, "Message %d: ", message_number);
		fprintf(stderr, catgets(catd, CATSET, 229,
				"Error with certificate at depth: %i\n"),
				depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data,
				sizeof data);
		fprintf(stderr, catgets(catd, CATSET, 230, " issuer = %s\n"),
				data);
		X509_NAME_oneline(X509_get_subject_name(cert), data,
				sizeof data);
		fprintf(stderr, catgets(catd, CATSET, 231, " subject = %s\n"),
				data);
		fprintf(stderr, catgets(catd, CATSET, 232, " err %i: %s\n"),
				err, X509_verify_cert_error_string(err));
		if (ssl_vrfy_decide() != OKAY)
			return 0;
	}
	return 1;
}

static const SSL_METHOD *
ssl_select_method(const char *uhp)
{
	const SSL_METHOD *method;
	char	*cp;

	cp = ssl_method_string(uhp);
	if (cp != NULL) {
		if (equal(cp, "ssl2"))
			method = SSLv2_client_method();
		else if (equal(cp, "ssl3"))
			method = SSLv3_client_method();
		else if (equal(cp, "tls1"))
			method = TLSv1_client_method();
		else {
			fprintf(stderr, catgets(catd, CATSET, 244,
					"Invalid SSL method \"%s\"\n"), cp);
			method = SSLv23_client_method();
		}
	} else
		method = SSLv23_client_method();
	return method;
}

static void 
ssl_load_verifications(struct sock *sp)
{
	char *ca_dir, *ca_file;
	X509_STORE	*store;

	if (ssl_vrfy_level == VRFY_IGNORE)
		return;
	if ((ca_dir = value("ssl-ca-dir")) != NULL)
		ca_dir = expand(ca_dir);
	if ((ca_file = value("ssl-ca-file")) != NULL)
		ca_file = expand(ca_file);
	if (ca_dir || ca_file) {
		if (SSL_CTX_load_verify_locations(sp->s_ctx,
					ca_file, ca_dir) != 1) {
			fprintf(stderr, catgets(catd, CATSET, 233,
						"Error loading"));
			if (ca_dir) {
				fprintf(stderr, catgets(catd, CATSET, 234,
							" %s"), ca_dir);
				if (ca_file)
					fprintf(stderr, catgets(catd, CATSET,
							235, " or"));
			}
			if (ca_file)
				fprintf(stderr, catgets(catd, CATSET, 236,
						" %s"), ca_file);
			fprintf(stderr, catgets(catd, CATSET, 237, "\n"));
		}
	}
	if (value("ssl-no-default-ca") == NULL) {
		if (SSL_CTX_set_default_verify_paths(sp->s_ctx) != 1)
			fprintf(stderr, catgets(catd, CATSET, 243,
				"Error loading default CA locations\n"));
	}
	verify_error_found = 0;
	message_number = 0;
	SSL_CTX_set_verify(sp->s_ctx, SSL_VERIFY_PEER, ssl_verify_cb);
	store = SSL_CTX_get_cert_store(sp->s_ctx);
	load_crls(store, "ssl-crl-file", "ssl-crl-dir");
}

static void 
ssl_certificate(struct sock *sp, const char *uhp)
{
	char *certvar, *keyvar, *cert, *key;

	certvar = ac_alloc(strlen(uhp) + 10);
	strcpy(certvar, "ssl-cert-");
	strcpy(&certvar[9], uhp);
	if ((cert = value(certvar)) != NULL ||
			(cert = value("ssl-cert")) != NULL) {
		cert = expand(cert);
		if (SSL_CTX_use_certificate_chain_file(sp->s_ctx, cert) == 1) {
			keyvar = ac_alloc(strlen(uhp) + 9);
			strcpy(keyvar, "ssl-key-");
			if ((key = value(keyvar)) == NULL &&
					(key = value("ssl-key")) == NULL)
				key = cert;
			else
				key = expand(key);
			if (SSL_CTX_use_PrivateKey_file(sp->s_ctx, key,
						SSL_FILETYPE_PEM) != 1)
				fprintf(stderr, catgets(catd, CATSET, 238,
				"cannot load private key from file %s\n"),
						key);
			ac_free(keyvar);
		} else
			fprintf(stderr, catgets(catd, CATSET, 239,
				"cannot load certificate from file %s\n"),
					cert);
	}
	ac_free(certvar);
}

static enum okay 
ssl_check_host(const char *server, struct sock *sp)
{
	X509 *cert;
	X509_NAME *subj;
	char data[256];
#ifdef HAVE_STACK_OF
	STACK_OF(GENERAL_NAME)	*gens;
#else
	/*GENERAL_NAMES*/STACK	*gens;
#endif
	GENERAL_NAME	*gen;
	int	i;

	if ((cert = SSL_get_peer_certificate(sp->s_ssl)) == NULL) {
		fprintf(stderr, catgets(catd, CATSET, 248,
				"no certificate from \"%s\"\n"), server);
		return STOP;
	}
	gens = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
	if (gens != NULL) {
		for (i = 0; i < sk_GENERAL_NAME_num(gens); i++) {
			gen = sk_GENERAL_NAME_value(gens, i);
			if (gen->type == GEN_DNS) {
				if (verbose)
					fprintf(stderr,
						"Comparing DNS name: \"%s\"\n",
						gen->d.ia5->data);
				if (rfc2595_hostname_match(server,
						(char *)gen->d.ia5->data)
						== OKAY)
					goto found;
			}
		}
	}
	if ((subj = X509_get_subject_name(cert)) != NULL &&
			X509_NAME_get_text_by_NID(subj, NID_commonName,
				data, sizeof data) > 0) {
		data[sizeof data - 1] = 0;
		if (verbose)
			fprintf(stderr, "Comparing common name: \"%s\"\n",
					data);
		if (rfc2595_hostname_match(server, data) == OKAY)
			goto found;
	}
	X509_free(cert);
	return STOP;
found:	X509_free(cert);
	return OKAY;
}

enum okay 
ssl_open(const char *server, struct sock *sp, const char *uhp)
{
	char *cp;
	long options;

	ssl_init();
	ssl_set_vrfy_level(uhp);
	if ((sp->s_ctx =
	     SSL_CTX_new((SSL_METHOD *)ssl_select_method(uhp))) == NULL) {
		ssl_gen_err(catgets(catd, CATSET, 261, "SSL_CTX_new() failed"));
		return STOP;
	}
#ifdef	SSL_MODE_AUTO_RETRY
	/* available with OpenSSL 0.9.6 or later */
	SSL_CTX_set_mode(sp->s_ctx, SSL_MODE_AUTO_RETRY);
#endif	/* SSL_MODE_AUTO_RETRY */
	options = SSL_OP_ALL;
	if (value("ssl-v2-allow") == NULL)
		options |= SSL_OP_NO_SSLv2;
	SSL_CTX_set_options(sp->s_ctx, options);
	ssl_load_verifications(sp);
	ssl_certificate(sp, uhp);
	if ((cp = value("ssl-cipher-list")) != NULL) {
		if (SSL_CTX_set_cipher_list(sp->s_ctx, cp) != 1)
			fprintf(stderr, catgets(catd, CATSET, 240,
					"invalid ciphers: %s\n"), cp);
	}
	if ((sp->s_ssl = SSL_new(sp->s_ctx)) == NULL) {
		ssl_gen_err(catgets(catd, CATSET, 262, "SSL_new() failed"));
		return STOP;
	}
	SSL_set_fd(sp->s_ssl, sp->s_fd);
	if (SSL_connect(sp->s_ssl) < 0) {
		ssl_gen_err(catgets(catd, CATSET, 263,
				"could not initiate SSL/TLS connection"));
		return STOP;
	}
	if (ssl_vrfy_level != VRFY_IGNORE) {
		if (ssl_check_host(server, sp) != OKAY) {
			fprintf(stderr, catgets(catd, CATSET, 249,
				"host certificate does not match \"%s\"\n"),
				server);
			if (ssl_vrfy_decide() != OKAY)
				return STOP;
		}
	}
	sp->s_use_ssl = 1;
	return OKAY;
}

void
ssl_gen_err(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	SSL_load_error_strings();
	fprintf(stderr, ": %s\n",
			(ERR_error_string(ERR_get_error(), NULL)));
}

FILE *
smime_sign(FILE *ip, struct header *headp)
{
	FILE	*sp, *fp, *bp, *hp;
	char	*cp, *addr;
	X509	*cert;
	PKCS7	*pkcs7;
	EVP_PKEY	*pkey;
	BIO	*bb, *sb;

	ssl_init();
	if ((addr = myorigin(headp)) == NULL) {
		fprintf(stderr, "No \"from\" address for signing specified\n");
		return NULL;
	}
	if ((fp = smime_sign_cert(addr, NULL, 1)) == NULL)
		return NULL;
	if ((pkey = PEM_read_PrivateKey(fp, NULL, ssl_password_cb, NULL))
			== NULL) {
		ssl_gen_err("Error reading private key from");
		Fclose(fp);
		return NULL;
	}
	rewind(fp);
	if ((cert = PEM_read_X509(fp, NULL, ssl_password_cb, NULL)) == NULL) {
		ssl_gen_err("Error reading signer certificate from");
		Fclose(fp);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	Fclose(fp);
	if ((sp = Ftemp(&cp, "Rs", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	rewind(ip);
	if (smime_split(ip, &hp, &bp, -1, 0) == STOP) {
		Fclose(sp);
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	if ((bb = BIO_new_fp(bp, BIO_NOCLOSE)) == NULL ||
			(sb = BIO_new_fp(sp, BIO_NOCLOSE)) == NULL) {
		ssl_gen_err("Error creating BIO signing objects");
		Fclose(sp);
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	if ((pkcs7 = PKCS7_sign(cert, pkey, NULL, bb,
			PKCS7_DETACHED)) == NULL) {
		ssl_gen_err("Error creating the PKCS#7 signing object");
		BIO_free(bb);
		BIO_free(sb);
		Fclose(sp);
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	if (PEM_write_bio_PKCS7(sb, pkcs7) == 0) {
		ssl_gen_err("Error writing signed S/MIME data");
		BIO_free(bb);
		BIO_free(sb);
		Fclose(sp);
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	BIO_free(bb);
	BIO_free(sb);
	X509_free(cert);
	EVP_PKEY_free(pkey);
	rewind(bp);
	fflush(sp);
	rewind(sp);
	return smime_sign_assemble(hp, bp, sp);
}

static int
#ifdef HAVE_STACK_OF
smime_verify(struct message *m, int n, STACK_OF(X509) *chain, X509_STORE *store)
#else
smime_verify(struct message *m, int n, STACK *chain, X509_STORE *store)
#endif
{
	struct message	*x;
	char	*cp, *sender, *to, *cc, *cnttype;
	int	c, i, j;
	FILE	*fp, *ip;
	off_t	size;
	BIO	*fb, *pb;
	PKCS7	*pkcs7;
#ifdef HAVE_STACK_OF
	STACK_OF(X509)	*certs;
	STACK_OF(GENERAL_NAME)	*gens;
#else
	STACK	*certs, *gens;
#endif
	X509	*cert;
	X509_NAME	*subj;
	char	data[LINESIZE];
	GENERAL_NAME	*gen;

	verify_error_found = 0;
	message_number = n;
loop:	sender = getsender(m);
	to = hfield("to", m);
	cc = hfield("cc", m);
	cnttype = hfield("content-type", m);
	if ((ip = setinput(&mb, m, NEED_BODY)) == NULL)
		return 1;
	if (cnttype && strncmp(cnttype, "application/x-pkcs7-mime", 24) == 0) {
		if ((x = smime_decrypt(m, to, cc, 1)) == NULL)
			return 1;
		if (x != (struct message *)-1) {
			m = x;
			goto loop;
		}
	}
	size = m->m_size;
	if ((fp = Ftemp(&cp, "Rv", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return 1;
	}
	rm(cp);
	Ftfree(&cp);
	while (size-- > 0) {
		c = getc(ip);
		putc(c, fp);
	}
	fflush(fp);
	rewind(fp);
	if ((fb = BIO_new_fp(fp, BIO_NOCLOSE)) == NULL) {
		ssl_gen_err("Error creating BIO verification object "
				"for message %d", n);
		Fclose(fp);
		return 1;
	}
	if ((pkcs7 = SMIME_read_PKCS7(fb, &pb)) == NULL) {
		ssl_gen_err("Error reading PKCS#7 object for message %d", n);
		BIO_free(fb);
		Fclose(fp);
		return 1;
	}
	if (PKCS7_verify(pkcs7, chain, store, pb, NULL, 0) != 1) {
		ssl_gen_err("Error verifying message %d", n);
		BIO_free(fb);
		Fclose(fp);
		return 1;
	}
	BIO_free(fb);
	Fclose(fp);
	if (sender == NULL) {
		fprintf(stderr,
			"Warning: Message %d has no sender.\n", n);
		return 0;
	}
	certs = PKCS7_get0_signers(pkcs7, chain, 0);
	if (certs == NULL) {
		fprintf(stderr, "No certificates found in message %d.\n", n);
		return 1;
	}
	for (i = 0; i < sk_X509_num(certs); i++) {
		cert = sk_X509_value(certs, i);
		gens = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
		if (gens != NULL) {
			for (j = 0; j < sk_GENERAL_NAME_num(gens); j++) {
				gen = sk_GENERAL_NAME_value(gens, j);
				if (gen->type == GEN_EMAIL) {
					if (verbose)
						fprintf(stderr,
							"Comparing alt. "
							"address: %s\"\n",
							data);
					if (!asccasecmp((char *)
							gen->d.ia5->data,
							sender))
						goto found;
				}
			}
		}
		if ((subj = X509_get_subject_name(cert)) != NULL &&
				X509_NAME_get_text_by_NID(subj,
					NID_pkcs9_emailAddress,
					data, sizeof data) > 0) {
			data[sizeof data - 1] = 0;
			if (verbose)
				fprintf(stderr, "Comparing address: \"%s\"\n",
						data);
			if (asccasecmp(data, sender) == 0)
				goto found;
		}
	}
	fprintf(stderr, "Message %d: certificate does not match <%s>\n",
			n, sender);
	return 1;
found:	if (verify_error_found == 0)
		printf("Message %d was verified successfully.\n", n);
	return verify_error_found;
}

int 
cverify(void *vp)
{
	int	*msgvec = vp, *ip;
	int	ec = 0;
#ifdef HAVE_STACK_OF
	STACK_OF(X509)	*chain = NULL;
#else
	STACK	*chain = NULL;
#endif
	X509_STORE	*store;
	char	*ca_dir, *ca_file;

	ssl_init();
	ssl_vrfy_level = VRFY_STRICT;
	if ((store = X509_STORE_new()) == NULL) {
		ssl_gen_err("Error creating X509 store");
		return 1;
	}
	X509_STORE_set_verify_cb_func(store, ssl_verify_cb);
	if ((ca_dir = value("smime-ca-dir")) != NULL)
		ca_dir = expand(ca_dir);
	if ((ca_file = value("smime-ca-file")) != NULL)
		ca_file = expand(ca_file);
	if (ca_dir || ca_file) {
		if (X509_STORE_load_locations(store, ca_file, ca_dir) != 1) {
			ssl_gen_err("Error loading %s",
					ca_file ? ca_file : ca_dir);
			return 1;
		}
	}
	if (value("smime-no-default-ca") == NULL) {
		if (X509_STORE_set_default_paths(store) != 1) {
			ssl_gen_err("Error loading default CA locations");
			return 1;
		}
	}
	if (load_crls(store, "smime-crl-file", "smime-crl-dir") != OKAY)
		return 1;
	for (ip = msgvec; *ip; ip++) {
		setdot(&message[*ip-1]);
		ec |= smime_verify(&message[*ip-1], *ip, chain, store);
	}
	return ec;
}

static EVP_CIPHER *
smime_cipher(const char *name)
{
	const EVP_CIPHER	*cipher;
	char	*vn, *cp;
	int	vs;

	vn = ac_alloc(vs = strlen(name) + 30);
	snprintf(vn, vs, "smime-cipher-%s", name);
	if ((cp = value(vn)) != NULL) {
		if (strcmp(cp, "rc2-40") == 0)
			cipher = EVP_rc2_40_cbc();
		else if (strcmp(cp, "rc2-64") == 0)
			cipher = EVP_rc2_64_cbc();
		else if (strcmp(cp, "des") == 0)
			cipher = EVP_des_cbc();
		else if (strcmp(cp, "des-ede3") == 0)
			cipher = EVP_des_ede3_cbc();
		else {
			fprintf(stderr, "Invalid cipher \"%s\".\n", cp);
			cipher = NULL;
		}
	} else
		cipher = EVP_des_ede3_cbc();
	ac_free(vn);
	return (EVP_CIPHER *)cipher;
}

FILE *
smime_encrypt(FILE *ip, const char *certfile, const char *to)
{
	FILE	*yp, *fp, *bp, *hp;
	char	*cp;
	X509	*cert;
	PKCS7	*pkcs7;
	BIO	*bb, *yb;
#ifdef HAVE_STACK_OF
	STACK_OF(X509)	*certs;
#else
	STACK	*certs;
#endif
	EVP_CIPHER	*cipher;

	certfile = expand((char *)certfile);
	ssl_init();
	if ((cipher = smime_cipher(to)) == NULL)
		return NULL;
	if ((fp = Fopen(certfile, "r")) == NULL) {
		perror(certfile);
		return NULL;
	}
	if ((cert = PEM_read_X509(fp, NULL, ssl_password_cb, NULL)) == NULL) {
		ssl_gen_err("Error reading encryption certificate from \"%s\"",
				certfile);
		Fclose(fp);
		return NULL;
	}
	Fclose(fp);
	certs = sk_X509_new_null();
	sk_X509_push(certs, cert);
	if ((yp = Ftemp(&cp, "Ry", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	rewind(ip);
	if (smime_split(ip, &hp, &bp, -1, 0) == STOP) {
		Fclose(yp);
		return NULL;
	}
	if ((bb = BIO_new_fp(bp, BIO_NOCLOSE)) == NULL ||
			(yb = BIO_new_fp(yp, BIO_NOCLOSE)) == NULL) {
		ssl_gen_err("Error creating BIO encryption objects");
		Fclose(yp);
		return NULL;
	}
	if ((pkcs7 = PKCS7_encrypt(certs, bb, cipher, 0)) == NULL) {
		ssl_gen_err("Error creating the PKCS#7 encryption object");
		BIO_free(bb);
		BIO_free(yb);
		Fclose(yp);
		return NULL;
	}
	if (PEM_write_bio_PKCS7(yb, pkcs7) == 0) {
		ssl_gen_err("Error writing encrypted S/MIME data");
		BIO_free(bb);
		BIO_free(yb);
		Fclose(yp);
		return NULL;
	}
	BIO_free(bb);
	BIO_free(yb);
	Fclose(bp);
	fflush(yp);
	rewind(yp);
	return smime_encrypt_assemble(hp, yp);
}

struct message *
smime_decrypt(struct message *m, const char *to, const char *cc, int signcall)
{
	FILE	*fp, *bp, *hp, *op;
	char	*cp;
	X509	*cert = NULL;
	PKCS7	*pkcs7;
	EVP_PKEY	*pkey = NULL;
	BIO	*bb, *pb, *ob;
	long	size = m->m_size;
	FILE	*yp;

	if ((yp = setinput(&mb, m, NEED_BODY)) == NULL)
		return NULL;
	ssl_init();
	if ((fp = smime_sign_cert(to, cc, 0)) != NULL) {
		if ((pkey = PEM_read_PrivateKey(fp, NULL, ssl_password_cb,
						NULL)) == NULL) {
			ssl_gen_err("Error reading private key");
			Fclose(fp);
			return NULL;
		}
		rewind(fp);
		if ((cert = PEM_read_X509(fp, NULL, ssl_password_cb,
						NULL)) == NULL) {
			ssl_gen_err("Error reading decryption certificate");
			Fclose(fp);
			EVP_PKEY_free(pkey);
			return NULL;
		}
		Fclose(fp);
	}
	if ((op = Ftemp(&cp, "Rp", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	if (smime_split(yp, &hp, &bp, size, 1) == STOP) {
		Fclose(op);
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);
		return NULL;
	}
	if ((ob = BIO_new_fp(op, BIO_NOCLOSE)) == NULL ||
			(bb = BIO_new_fp(bp, BIO_NOCLOSE)) == NULL) {
		ssl_gen_err("Error creating BIO decryption objects");
		Fclose(op);
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);
		return NULL;
	}
	if ((pkcs7 = SMIME_read_PKCS7(bb, &pb)) == NULL) {
		ssl_gen_err("Error reading PKCS#7 object");
		Fclose(op);
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);
		return NULL;
	}
	if (PKCS7_type_is_signed(pkcs7)) {
		if (signcall) {
			BIO_free(bb);
			BIO_free(ob);
			if (cert)
				X509_free(cert);
			if (pkey)
				EVP_PKEY_free(pkey);
			Fclose(op);
			Fclose(bp);
			Fclose(hp);
			setinput(&mb, m, NEED_BODY);
			return (struct message *)-1;
		}
		if (PKCS7_verify(pkcs7, NULL, NULL, NULL, ob,
				PKCS7_NOVERIFY|PKCS7_NOSIGS) != 1)
			goto err;
		fseek(hp, 0L, SEEK_END);
		fprintf(hp, "X-Encryption-Cipher: none\n");
		fflush(hp);
		rewind(hp);
	} else if (pkey == NULL) {
		fprintf(stderr, "No appropriate private key found.\n");
		goto err2;
	} else if (cert == NULL) {
		fprintf(stderr, "No appropriate certificate found.\n");
		goto err2;
	} else if (PKCS7_decrypt(pkcs7, pkey, cert, ob, 0) != 1) {
	err:	ssl_gen_err("Error decrypting PKCS#7 object");
	err2:	BIO_free(bb);
		BIO_free(ob);
		Fclose(op);
		Fclose(bp);
		Fclose(hp);
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);
		return NULL;
	}
	BIO_free(bb);
	BIO_free(ob);
	if (cert)
		X509_free(cert);
	if (pkey)
		EVP_PKEY_free(pkey);
	fflush(op);
	rewind(op);
	Fclose(bp);
	return smime_decrypt_assemble(m, hp, op);
}

/*ARGSUSED4*/
static int 
ssl_password_cb(char *buf, int size, int rwflag, void *userdata)
{
	sighandler_type	saveint;
	char	*pass = NULL;
	int	len;

	(void)&saveint;
	(void)&pass;
	saveint = safe_signal(SIGINT, SIG_IGN);
	if (sigsetjmp(ssljmp, 1) == 0) {
		if (saveint != SIG_IGN)
			safe_signal(SIGINT, sslcatch);
		pass = getpassword(&otio, &reset_tio, "PEM pass phrase:");
	}
	safe_signal(SIGINT, saveint);
	if (pass == NULL)
		return 0;
	len = strlen(pass);
	if (len > size)
		len = size;
	memcpy(buf, pass, len);
	return len;
}

static FILE *
smime_sign_cert(const char *xname, const char *xname2, int warn)
{
	char	*vn, *cp;
	int	vs;
	FILE	*fp;
	struct name	*np;
	const char	*name = xname, *name2 = xname2;

loop:	if (name) {
		np = sextract(savestr(name), GTO|GSKIN);
		while (np) {
			/*
			 * This needs to be more intelligent since it will
			 * currently take the first name for which a private
			 * key is available regardless of whether it is the
			 * right one for the message.
			 */
			vn = ac_alloc(vs = strlen(np->n_name) + 30);
			snprintf(vn, vs, "smime-sign-cert-%s", np->n_name);
			if ((cp = value(vn)) != NULL)
				goto open;
			np = np->n_flink;
		}
		if (name2) {
			name = name2;
			name2 = NULL;
			goto loop;
		}
	}
	if ((cp = value("smime-sign-cert")) != NULL)
		goto open;
	if (warn) {
		fprintf(stderr, "Could not find a certificate for %s", xname);
		if (xname2)
			fprintf(stderr, "or %s", xname2);
		fputc('\n', stderr);
	}
	return NULL;
open:	cp = expand(cp);
	if ((fp = Fopen(cp, "r")) == NULL) {
		perror(cp);
		return NULL;
	}
	return fp;
}

enum okay
smime_certsave(struct message *m, int n, FILE *op)
{
	struct message	*x;
	char	*cp, *to, *cc, *cnttype;
	int	c, i;
	FILE	*fp, *ip;
	off_t	size;
	BIO	*fb, *pb;
	PKCS7	*pkcs7;
#ifdef HAVE_STACK_OF
	STACK_OF(X509)	*certs;
	STACK_OF(X509)	*chain = NULL;
#else
	STACK	*certs;
	STACK	*chain = NULL;
#endif
	X509	*cert;
	enum okay	ok = OKAY;

	message_number = n;
loop:	to = hfield("to", m);
	cc = hfield("cc", m);
	cnttype = hfield("content-type", m);
	if ((ip = setinput(&mb, m, NEED_BODY)) == NULL)
		return STOP;
	if (cnttype && strncmp(cnttype, "application/x-pkcs7-mime", 24) == 0) {
		if ((x = smime_decrypt(m, to, cc, 1)) == NULL)
			return STOP;
		if (x != (struct message *)-1) {
			m = x;
			goto loop;
		}
	}
	size = m->m_size;
	if ((fp = Ftemp(&cp, "Rv", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return STOP;
	}
	rm(cp);
	Ftfree(&cp);
	while (size-- > 0) {
		c = getc(ip);
		putc(c, fp);
	}
	fflush(fp);
	rewind(fp);
	if ((fb = BIO_new_fp(fp, BIO_NOCLOSE)) == NULL) {
		ssl_gen_err("Error creating BIO object for message %d", n);
		Fclose(fp);
		return STOP;
	}
	if ((pkcs7 = SMIME_read_PKCS7(fb, &pb)) == NULL) {
		ssl_gen_err("Error reading PKCS#7 object for message %d", n);
		BIO_free(fb);
		Fclose(fp);
		return STOP;
	}
	BIO_free(fb);
	Fclose(fp);
	certs = PKCS7_get0_signers(pkcs7, chain, 0);
	if (certs == NULL) {
		fprintf(stderr, "No certificates found in message %d.\n", n);
		return STOP;
	}
	for (i = 0; i < sk_X509_num(certs); i++) {
		cert = sk_X509_value(certs, i);
		if (X509_print_fp(op, cert) == 0 ||
				PEM_write_X509(op, cert) == 0) {
			ssl_gen_err("Error writing certificate %d from "
					"message %d", i, n);
			ok = STOP;
		}
	}
	return ok;
}

#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
static enum okay 
load_crl1(X509_STORE *store, const char *name)
{
	X509_LOOKUP	*lookup;

	if (verbose)
		printf("Loading CRL from \"%s\".\n", name);
	if ((lookup = X509_STORE_add_lookup(store,
					X509_LOOKUP_file())) == NULL) {
		ssl_gen_err("Error creating X509 lookup object");
		return STOP;
	}
	if (X509_load_crl_file(lookup, name, X509_FILETYPE_PEM) != 1) {
		ssl_gen_err("Error loading CRL from \"%s\"", name);
		return STOP;
	}
	return OKAY;
}
#endif	/* new OpenSSL */

static enum okay 
load_crls(X509_STORE *store, const char *vfile, const char *vdir)
{
	char	*crl_file, *crl_dir;
#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
	DIR	*dirfd;
	struct dirent	*dp;
	char	*fn = NULL;
	int	fs = 0, ds, es;
#endif	/* new OpenSSL */

	if ((crl_file = value(vfile)) != NULL) {
#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
		crl_file = expand(crl_file);
		if (load_crl1(store, crl_file) != OKAY)
			return STOP;
#else	/* old OpenSSL */
		fprintf(stderr,
			"This OpenSSL version is too old to use CRLs.\n");
		return STOP;
#endif	/* old OpenSSL */
	}
	if ((crl_dir = value(vdir)) != NULL) {
#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
		crl_dir = expand(crl_dir);
		ds = strlen(crl_dir);
		if ((dirfd = opendir(crl_dir)) == NULL) {
			perror(crl_dir);
			return STOP;
		}
		fn = smalloc(fs = ds + 20);
		strcpy(fn, crl_dir);
		fn[ds] = '/';
		while ((dp = readdir(dirfd)) != NULL) {
			if (dp->d_name[0] == '.' &&
					(dp->d_name[1] == '\0' ||
					 (dp->d_name[1] == '.' &&
					  dp->d_name[2] == '\0')))
				continue;
			if (dp->d_name[0] == '.')
				continue;
			if (ds + (es = strlen(dp->d_name)) + 2 < fs)
				fn = srealloc(fn, fs = ds + es + 20);
			strcpy(&fn[ds+1], dp->d_name);
			if (load_crl1(store, fn) != OKAY) {
				closedir(dirfd);
				free(fn);
				return STOP;
			}
		}
		closedir(dirfd);
		free(fn);
#else	/* old OpenSSL */
		fprintf(stderr,
			"This OpenSSL version is too old to use CRLs.\n");
		return STOP;
#endif	/* old OpenSSL */
	}
#if defined (X509_V_FLAG_CRL_CHECK) && defined (X509_V_FLAG_CRL_CHECK_ALL)
	if (crl_file || crl_dir)
		X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK |
				X509_V_FLAG_CRL_CHECK_ALL);
#endif	/* old OpenSSL */
	return OKAY;
}

#else	/* !USE_OPENSSL */

#include <stdio.h>

static void
nosmime(void)
{
	fprintf(stderr, "No S/MIME support compiled in.\n");
}

/*ARGSUSED*/
FILE *
smime_sign(FILE *fp)
{
	nosmime();
	return NULL;
}

/*ARGSUSED*/
int 
cverify(void *vp)
{
	nosmime();
	return 1;
}

/*ARGSUSED*/
FILE *
smime_encrypt(FILE *fp, const char *certfile, const char *to)
{
	nosmime();
	return NULL;
}

/*ARGSUSED*/
struct message *
smime_decrypt(struct message *m, const char *to, const char *cc, int signcall)
{
	nosmime();
	return NULL;
}

/*ARGSUSED*/
int 
ccertsave(void *v)
{
	nosmime();
	return 1;
}
#endif	/* !USE_OPENSSL */
#endif	/* !USE_NSS */

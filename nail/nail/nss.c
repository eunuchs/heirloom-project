/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Changes Copyright (c) 2004
 *	Gunnar Ritter.  All rights reserved.
 */
/*
 * Parts of this file are derived from the Mozilla NSS 3.9.2 source,
 * mozilla/security/nss/cmd/smimetools/cmsutil.c. Therefore:
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 */

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)nss.c	1.48 (gritter) 8/4/07";
#endif
#endif /* not lint */

#include "config.h"

#ifdef	USE_NSS

#include "rcv.h"

#include <setjmp.h>
#include <termios.h>
#include <stdio.h>

static int	verbose;
static int	reset_tio;
static struct termios	otio;
static sigjmp_buf	nssjmp;

#include <stdarg.h>

#include <nss.h>
#include <ssl.h>
#include <prinit.h>
#include <prmem.h>
#include <pk11func.h>
#include <prtypes.h>
#include <prerror.h>
#include <secerr.h>
#include <smime.h>
#include <ciferfam.h>
#ifdef HAVE_XCONST_H
#include <xconst.h>
#endif
#ifdef HAVE_GENNAME_H
#include <genname.h>
#endif
#include <private/pprio.h>

#include "extern.h"

#ifndef	HAVE_CERTAltNameEncodedContext
/*
 * NSS 3.11.5 neither installs genname.h nor provides this
 * structure otherwise, so define it here.
 */
typedef struct CERTAltNameEncodedContextStr {
	SECItem	**encodedGenName;
} CERTAltNameEncodedContext;
#endif	/* !HAVE_CERTAltNameEncodedContext */

#include "nsserr.c"

static char *password_cb(PK11SlotInfo *slot, PRBool retry, void *arg);
static SECStatus bad_cert_cb(void *arg, PRFileDesc *fd);
static enum okay nss_check_host(const char *server, struct sock *sp);
static const char *bad_cert_str(void);
static enum okay nss_init(void);
static void nss_select_method(const char *uhp);
static CERTCertificate *get_signer_cert(char *addr);
static FILE *encode(FILE *ip, FILE **hp, FILE **bp, NSSCMSMessage *msg,
		void (*cb)(void *, const char *, unsigned long));
static void decoder_cb(void *arg, const char *buf, unsigned long len);
static void base64_cb(void *arg, const char *buf, unsigned long len);
static int verify1(struct message *m, int n);
static struct message *getsig(struct message *m, int n, NSSCMSMessage **msg);
static enum okay getdig(struct message *m, int n, SECItem ***digests,
		PLArenaPool **poolp, SECAlgorithmID **algids);
static void nsscatch(int s);
static void dumpcert(CERTCertificate *cert, FILE *op);
static enum okay getcipher(const char *to, SECOidTag *alg, int *key);

static char *
password_cb(PK11SlotInfo *slot, PRBool retry, void *arg)
{
	sighandler_type	saveint;
	char	*pass = NULL;

	(void)&saveint;
	(void)&pass;
	saveint = safe_signal(SIGINT, SIG_IGN);
	if (sigsetjmp(nssjmp, 1) == 0) {
		if (saveint != SIG_IGN)
			safe_signal(SIGINT, nsscatch);
		pass = getpassword(&otio, &reset_tio, arg);
	}
	safe_signal(SIGINT, saveint);
	if (pass == NULL)
		return NULL;
	return PL_strdup(pass);
}

static SECStatus
bad_cert_cb(void *arg, PRFileDesc *fd)
{
	if (PORT_GetError() == SSL_ERROR_BAD_CERT_DOMAIN)
		/*
		 * We must not use this result. NSS verifies host names
		 * according to RFC 2818, but we must verify host names
		 * according to RFC 2595. The rules are different:
		 *
		 * - RFC 2818 says that if both a dNSName and a CN are
		 *   contained in the peer certificate, only the dNSName
		 *   is used. RFC 2595 encourages to use both.
		 *
		 * - RFC 2818 allows the wildcard '*' in any component
		 *   of the host name. RFC 2595 allows it only as the
		 *   "left-most name component".
		 *
		 * So ignore it and verify separately.
		 */
		return SECSuccess;
	fprintf(stderr, "Error in certificate: %s.\n", bad_cert_str());
	return ssl_vrfy_decide() == OKAY ? SECSuccess : SECFailure;
}

/*
 * Host name checking according to RFC 2595.
 */
static enum okay
nss_check_host(const char *server, struct sock *sp)
{
	CERTCertificate	*cert;
	char	*cn = NULL;
	enum okay	ok = STOP;
	PRArenaPool	*arena;
	CERTGeneralName	*gn;
	SECItem	altname;
	CERTAltNameEncodedContext	ec;
	int	i;
	const SEC_ASN1Template	gntempl[] = {
		{ SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
	};

	if ((cert = SSL_PeerCertificate(sp->s_prfd)) == NULL) {
		fprintf(stderr, "no certificate from \"%s\"\n", server);
		return STOP;
	}
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (CERT_FindCertExtension(cert, SEC_OID_X509_SUBJECT_ALT_NAME,
				&altname) == SECSuccess &&
			SEC_ASN1DecodeItem(arena, &ec, gntempl,
				&altname) == SECSuccess &&
			ec.encodedGenName != NULL) {
		for (i = 0; ec.encodedGenName[i] != NULL; i++) {
			gn = CERT_DecodeGeneralName(arena, ec.encodedGenName[i],
					NULL);
			if (gn->type == certDNSName) {
				char	*dn = ac_alloc(gn->name.other.len + 1);
				memcpy(dn, gn->name.other.data,
						gn->name.other.len);
				dn[gn->name.other.len] = '\0';
				if (verbose)
					fprintf(stderr,
						"Comparing DNS name: \"%s\"\n",
						dn);
				if (rfc2595_hostname_match(server, dn)
						== OKAY) {
					ac_free(dn);
					goto out;
				}
				ac_free(dn);
			}
		}
	}
	if ((cn = CERT_GetCommonName(&cert->subject)) != NULL) {
		if (verbose)
			fprintf(stderr, "Comparing common name: \"%s\"\n", cn);
		ok = rfc2595_hostname_match(server, cn);
	}
	if (ok == STOP)
		fprintf(stderr, "host certificate does not match \"%s\"\n",
				server);
out:	if (cn)
		PORT_Free(cn);
	PORT_FreeArena(arena, PR_FALSE);
	CERT_DestroyCertificate(cert);
	return ok;
}

static const char *
bad_cert_str(void)
{
	int	ec;

	ec = PORT_GetError();
	return nss_strerror(ec);
}

static enum okay
nss_init(void)
{
	static int	initialized;
	char	*cp;

	verbose = value("verbose") != NULL;
	if (initialized == 0) {
		if ((cp = value("nss-config-dir")) == NULL) {
			fputs("Missing \"nss-config-dir\" variable.\n", stderr);
			return STOP;
		}
		cp = expand(cp);
		PR_Init(0, 0, 0);
		PK11_SetPasswordFunc(password_cb);
		if (NSS_Init(cp) == SECSuccess) {
			NSS_SetDomesticPolicy();
			initialized = 1;
			return OKAY;
		}
		nss_gen_err("Error initializing NSS");
		return STOP;
	}
	return OKAY;
}

static void
nss_select_method(const char *uhp)
{
	char	*cp;
	enum {
		SSL2 = 01,
		SSL3 = 02,
		TLS1 = 03
	} methods;

	methods = SSL2|SSL3|TLS1;
	cp = ssl_method_string(uhp);
	if (cp != NULL) {
		if (equal(cp, "ssl2"))
			methods = SSL2;
		else if (equal(cp, "ssl3"))
			methods = SSL3;
		else if (equal(cp, "tls1"))
			methods = TLS1;
		else {
			fprintf(stderr, catgets(catd, CATSET, 244,
					"Invalid SSL method \"%s\"\n"), cp);
		}
	}
	if (value("ssl-v2-allow") == NULL)
		methods &= ~SSL2;
	SSL_OptionSetDefault(SSL_ENABLE_SSL2, methods&SSL2 ? PR_TRUE:PR_FALSE);
	SSL_OptionSetDefault(SSL_ENABLE_SSL3, methods&SSL3 ? PR_TRUE:PR_FALSE);
	SSL_OptionSetDefault(SSL_ENABLE_TLS, methods&TLS1 ? PR_TRUE:PR_FALSE);
}

enum okay
ssl_open(const char *server, struct sock *sp, const char *uhp)
{
	PRFileDesc	*fdp, *fdc;

	if (nss_init() == STOP)
		return STOP;
	ssl_set_vrfy_level(uhp);
	nss_select_method(uhp);
	if ((fdp = PR_ImportTCPSocket(sp->s_fd)) == NULL) {
		nss_gen_err("Error importing OS file descriptor");
		return STOP;
	}
	if ((fdc = SSL_ImportFD(NULL, fdp)) == NULL) {
		nss_gen_err("Error importing NSPR file descriptor");
		PR_Close(fdp);
		return STOP;
	}
	SSL_SetURL(fdc, server);
	SSL_SetPKCS11PinArg(fdc, NULL);
	SSL_BadCertHook(fdc, bad_cert_cb, NULL);
	if (SSL_ResetHandshake(fdc, PR_FALSE) != SECSuccess) {
		nss_gen_err("Cannot reset NSS handshake");
		PR_Close(fdc);
		return STOP;
	}
	if (SSL_ForceHandshake(fdc) != 0) {
		nss_gen_err("SSL/TLS handshake failed");
		PR_Close(fdc);
		return STOP;
	}
	sp->s_prfd = fdc;
	if (nss_check_host(server, sp) != OKAY && ssl_vrfy_decide() != OKAY) {
		PR_Close(fdc);
		sp->s_prfd = NULL;
		return STOP;
	}
	sp->s_use_ssl = 1;
	if (verbose) {
		char	*cipher, *issuer, *subject;
		int	keysize, secretkeysize;

		if (SSL_SecurityStatus(fdc, NULL, &cipher,
					&keysize, &secretkeysize,
					&issuer, &subject) == SECSuccess) {
			fprintf(stderr, "SSL parameters: cipher=%s, "
					"keysize=%d, secretkeysize=%d,\n"
					"issuer=%s\n"
					"subject=%s\n",
					cipher, keysize, secretkeysize,
					issuer, subject);
			PR_Free(cipher);
			PR_Free(issuer);
			PR_Free(subject);
		} else
			nss_gen_err("Could not read status information");
	}
	return OKAY;
}

void
nss_gen_err(const char *fmt, ...)
{
	va_list	ap;
	char	*text;
	int	len;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if ((len = PR_GetErrorTextLength()) > 0) {
		text = ac_alloc(len);
		if (PR_GetErrorText(text) > 0)
			fprintf(stderr, ": %s\n", text);
		ac_free(text);
	} else
		fprintf(stderr, ": %s.\n", nss_strerror(PR_GetError()));
}

FILE *
smime_sign(FILE *ip, struct header *headp)
{
	NSSCMSMessage	*msg;
	NSSCMSContentInfo	*content;
	NSSCMSSignedData	*data;
	NSSCMSSignerInfo	*info;
	CERTCertificate	*cert;
	CERTCertDBHandle	*handle;
	FILE	*hp, *bp, *sp;
	char	*addr;

	if (nss_init() != OKAY)
		return NULL;
	if ((addr = myorigin(headp)) == NULL) {
		fprintf(stderr, "No \"from\" address for signing specified\n");
		return NULL;
	}
	if ((cert = get_signer_cert(addr)) == NULL)
		return NULL;
	handle = CERT_GetDefaultCertDB();
	if ((msg = NSS_CMSMessage_Create(NULL)) == NULL) {
		fprintf(stderr, "Cannot create CMS message.\n");
		return NULL;
	}
	if ((data = NSS_CMSSignedData_Create(msg)) == NULL) {
		fprintf(stderr, "Cannot create CMS signed data.\n");
		return NULL;
	}
	content = NSS_CMSMessage_GetContentInfo(msg);
	if (NSS_CMSContentInfo_SetContent_SignedData(msg, content, data)
			!= SECSuccess) {
		fprintf(stderr, "Cannot attach CMS signed data.\n");
		return NULL;
	}
	content = NSS_CMSSignedData_GetContentInfo(data);
	if (NSS_CMSContentInfo_SetContent_Data(msg, content, NULL, PR_TRUE)
			!= SECSuccess) {
		fprintf(stderr, "Cannot attach CMS data.\n");
		return NULL;
	}
	if ((info = NSS_CMSSignerInfo_Create(msg, cert, SEC_OID_SHA1)) == 0) {
		fprintf(stderr, "Cannot create signed information.\n");
		return NULL;
	}
	if (NSS_CMSSignerInfo_IncludeCerts(info, NSSCMSCM_CertOnly,
				certUsageEmailSigner) != SECSuccess) {
		fprintf(stderr, "Cannot include certificate.\n");
		return NULL;
	}
	if (NSS_CMSSignerInfo_AddSigningTime(info, PR_Now()) != SECSuccess) {
		fprintf(stderr, "Cannot add signing time.\n");
		return NULL;
	}
	if (NSS_CMSSignerInfo_AddSMIMECaps(info) != SECSuccess) {
		fprintf(stderr, "Cannot add S/MIME capabilities.\n");
		return NULL;
	}
	NSS_CMSSignerInfo_AddSMIMEEncKeyPrefs(info, cert, handle);
	NSS_CMSSignerInfo_AddMSSMIMEEncKeyPrefs(info, cert, handle);
	if (NSS_CMSSignedData_AddCertificate(data, cert) != SECSuccess) {
		fprintf(stderr, "Cannot add encryption certificate.\n");
		return NULL;
	}
	if (NSS_CMSSignedData_AddSignerInfo(data, info) != SECSuccess) {
		fprintf(stderr, "Cannot add signer information.\n");
		return NULL;
	}
	CERT_DestroyCertificate(cert);
	if ((sp = encode(ip, &hp, &bp, msg, base64_cb)) == NULL) {
		NSS_CMSMessage_Destroy(msg);
		return NULL;
	}
	NSS_CMSMessage_Destroy(msg);
	return smime_sign_assemble(hp, bp, sp);
}

int
cverify(void *vp)
{
	int	*msgvec = vp, *ip;
	int	ec = 0;

	if (nss_init() != OKAY)
		return 1;
	ssl_vrfy_level = VRFY_STRICT;
	for (ip = msgvec; *ip; ip++) {
		setdot(&message[*ip-1]);
		ec |= verify1(&message[*ip-1], *ip);
	}
	return ec;
}

FILE *
smime_encrypt(FILE *ip, const char *ignored, const char *to)
{
	NSSCMSMessage	*msg;
	NSSCMSContentInfo	*content;
	NSSCMSEnvelopedData	*data;
	NSSCMSRecipientInfo	*info;
	CERTCertificate	*cert[2];
	CERTCertDBHandle	*handle;
	SECOidTag	tag;
	FILE	*hp, *pp, *yp;
	int	keysize;
	char	*nickname, *vn;
	int	vs;

	if (nss_init() != OKAY)
		return NULL;
	handle = CERT_GetDefaultCertDB();
	vn = ac_alloc(vs = strlen(to) + 30);
	snprintf(vn, vs, "smime-nickname-%s", to);
	nickname = value(vn);
	ac_free(vn);
	if ((cert[0] = CERT_FindCertByNicknameOrEmailAddr(handle,
			nickname ? nickname : (char *)to)) == NULL) {
		if (nickname)
			fprintf(stderr, "Cannot find certificate \"%s\".\n",
					nickname);
		else
			fprintf(stderr, "Cannot find certificate for <%s>.\n",
					to);
		return NULL;
	}
	cert[1] = NULL;
	if (getcipher(to, &tag, &keysize) != OKAY)
		return NULL;
	if ((msg = NSS_CMSMessage_Create(NULL)) == NULL) {
		fprintf(stderr, "Cannot create CMS message.\n");
		return NULL;
	}
	if ((data = NSS_CMSEnvelopedData_Create(msg, tag, keysize)) == NULL) {
		fprintf(stderr, "Cannot create enveloped data.\n");
		return NULL;
	}
	content = NSS_CMSMessage_GetContentInfo(msg);
	if (NSS_CMSContentInfo_SetContent_EnvelopedData(msg, content, data)
			!= SECSuccess) {
		fprintf(stderr, "Cannot attach enveloped data.\n");
		return NULL;
	}
	content = NSS_CMSEnvelopedData_GetContentInfo(data);
	if (NSS_CMSContentInfo_SetContent_Data(msg, content, NULL, PR_FALSE)
			!= SECSuccess) {
		fprintf(stderr, "Cannot attach CMS data.\n");
		return NULL;
	}
	if ((info = NSS_CMSRecipientInfo_Create(msg, cert[0])) == NULL) {
		fprintf(stderr, "Cannot create CMS recipient information.\n");
		return NULL;
	}
	if (NSS_CMSEnvelopedData_AddRecipient(data, info) != SECSuccess) {
		fprintf(stderr, "Cannot add CMS recipient information.\n");
		return NULL;
	}
	CERT_DestroyCertificate(cert[0]);
	if ((yp = encode(ip, &hp, &pp, msg, base64_cb)) == NULL)
		return NULL;
	NSS_CMSMessage_Destroy(msg);
	return smime_encrypt_assemble(hp, yp);
}

struct message *
smime_decrypt(struct message *m, const char *to, const char *cc, int signcall)
{
	NSSCMSDecoderContext	*ctx;
	NSSCMSMessage	*msg;
	FILE	*op, *hp, *bp;
	char	*buf = NULL;
	size_t	bufsize = 0, buflen, count;
	char	*cp;
	struct str	in, out;
	FILE	*yp;
	long	size;
	int	i, nlevels;
	int	binary = 0;

	if ((yp = setinput(&mb, m, NEED_BODY)) == NULL)
		return NULL;
	if (nss_init() != OKAY)
		return NULL;
	if ((op = Ftemp(&cp, "Rp", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	if ((ctx = NSS_CMSDecoder_Start(NULL,
					decoder_cb, op,
					password_cb, "Pass phrase:",
					NULL, NULL)) == NULL) {
		fprintf(stderr, "Cannot start decoder.\n");
		return NULL;
	}
	size = m->m_size;
	if ((smime_split(yp, &hp, &bp, size, 1)) == STOP)
		return NULL;
	count = fsize(bp);
	while (fgetline(&buf, &bufsize, &count, &buflen, bp, 0) != NULL) {
		if (buf[0] == '\n')
			break;
		if ((cp = thisfield(buf, "content-transfer-encoding")) != NULL)
			if (ascncasecmp(cp, "binary", 7) == 0)
				binary = 1;
	}
	while (fgetline(&buf, &bufsize, &count, &buflen, bp, 0) != NULL) {
		if (binary)
			NSS_CMSDecoder_Update(ctx, buf, buflen);
		else {
			in.s = buf;
			in.l = buflen;
			mime_fromb64_b(&in, &out, 0, bp);
			NSS_CMSDecoder_Update(ctx, out.s, out.l);
			free(out.s);
		}
	}
	free(buf);
	if ((msg = NSS_CMSDecoder_Finish(ctx)) == NULL) {
		fprintf(stderr, "Failed to decode message.\n");
		Fclose(hp);
		Fclose(bp);
		return NULL;
	}
	nlevels = NSS_CMSMessage_ContentLevelCount(msg);
	for (i = 0; i < nlevels; i++) {
		NSSCMSContentInfo	*content;
		SECOidTag	tag;

		content = NSS_CMSMessage_ContentLevel(msg, i);
		tag = NSS_CMSContentInfo_GetContentTypeTag(content);
		if (tag == SEC_OID_PKCS7_DATA) {
			const char	*fld = "X-Encryption-Cipher";
			SECOidTag	alg;
			int	keysize;

			alg = NSS_CMSContentInfo_GetContentEncAlgTag(content);
			keysize = NSS_CMSContentInfo_GetBulkKeySize(content);
			fseek(hp, 0L, SEEK_END);
			switch (alg) {
			case 0:
				if (signcall) {
					NSS_CMSMessage_Destroy(msg);
					Fclose(hp);
					Fclose(bp);
					setinput(&mb, m, NEED_BODY);
					return (struct message *)-1;
				}
				fprintf(hp, "%s: none\n", fld);
				break;
			case SEC_OID_RC2_CBC:
				fprintf(hp, "%s: RC2, %d bits\n", fld, keysize);
				break;
			case SEC_OID_DES_CBC:
				fprintf(hp, "%s: DES, 56 bits\n", fld);
				break;
			case SEC_OID_DES_EDE3_CBC:
				fprintf(hp, "%s: 3DES, 112/168 bits\n", fld);
				break;
			case SEC_OID_FORTEZZA_SKIPJACK:
				fprintf(hp, "%s: Fortezza\n", fld);
				break;
			default:
				fprintf(hp, "%s: unknown type %lu\n", fld,
						(unsigned long)alg);
			}
			fflush(hp);
			rewind(hp);
		}
	}
	NSS_CMSMessage_Destroy(msg);
	fflush(op);
	rewind(op);
	Fclose(bp);
	return smime_decrypt_assemble(m, hp, op);
}

static CERTCertificate *
get_signer_cert(char *addr)
{
	CERTCertDBHandle	*handle;
	CERTCertList	*list;
	CERTCertListNode	*node;
	CERTCertificate	*cert = NULL;
	const char	*cp;
	char	*nick;
	char	*vn;
	int	vs, found = 0;

	addr = skin(addr);
	vn = ac_alloc(vs = strlen(addr) + 30);
	snprintf(vn, vs, "smime-sign-nickname-%s", addr);
	if ((nick = value(vn)) == NULL)
		nick = value("smime-sign-nickname");
	ac_free(vn);
	handle = CERT_GetDefaultCertDB();
	if (nick) {
		cert = CERT_FindCertByNickname(handle, nick);
		if (cert == NULL)
			fprintf(stderr, "No certificate \"%s\" found.\n", nick);
		return cert;
	}
	if ((list = CERT_FindUserCertsByUsage(handle, certUsageEmailSigner,
					PR_TRUE, PR_TRUE, NULL)) == NULL) {
		fprintf(stderr, "Cannot find any certificates for signing.\n");
		return NULL;
	}
	for (node = CERT_LIST_HEAD(list); !CERT_LIST_END(node, list);
			node = CERT_LIST_NEXT(node)) {
		if ((cp = CERT_GetCertEmailAddress(&node->cert->subject))
				!= NULL && asccasecmp(cp, addr) == 0) {
			cert = node->cert;
			found++;
		}
	}
	if (cert == NULL) {
		for (node = CERT_LIST_HEAD(list);
				!CERT_LIST_END(node, list) && cert == NULL;
				node = CERT_LIST_NEXT(node)) {
			cp = CERT_GetFirstEmailAddress(node->cert);
			while (cp) {
				if (asccasecmp(cp, addr) == 0) {
					cert = node->cert;
					found++;
				}
				cp = CERT_GetNextEmailAddress(node->cert, cp);
			}
		}
	}
	if (found > 1) {
		fprintf(stderr,
			"More than one signing certificate found for <%s>.\n"
			"Use the smime-sign-nickname variable.\n", addr);
		return NULL;
	}
	if (cert == NULL)
		fprintf(stderr,
			"Cannot find a signing certificate for <%s>.\n",
			addr);
	return cert;
}

static FILE *
encode(FILE *ip, FILE **hp, FILE **bp, NSSCMSMessage *msg,
		void    (*cb)(void *, const char *, unsigned long))
{
	NSSCMSEncoderContext	*ctx;
	char	*buf = NULL, *cp;
	size_t	bufsize = 0, buflen, count;
	FILE	*op;

	if (smime_split(ip, hp, bp, -1, 0) == STOP)
		return NULL;
	if ((op = Ftemp(&cp, "Ry", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	if ((ctx = NSS_CMSEncoder_Start(msg,
			cb, op,
			NULL, NULL,
			password_cb, "Pass phrase:",
			NULL, NULL,
			NULL, NULL)) == NULL) {
		fprintf(stderr, "Cannot create encoder context.\n");
		Fclose(op);
		return NULL;
	}
	count = fsize(*bp);
	while (fgetline(&buf, &bufsize, &count, &buflen, *bp, 0) != NULL) {
		buf[buflen-1] = '\r';
		buf[buflen] = '\n';
		if (NSS_CMSEncoder_Update(ctx, buf, buflen+1) != 0) {
			fprintf(stderr, "Failed to add data to encoder.\n");
			Fclose(op);
			return NULL;
		}
	}
	free(buf);
	if (NSS_CMSEncoder_Finish(ctx) != 0) {
		fprintf(stderr, "Failed to encode data.\n");
		Fclose(op);
		return NULL;
	}
	rewind(*bp);
	cb(op, (void *)-1, 0);
	fflush(op);
	if (ferror(op)) {
		perror("tempfile");
		Fclose(op);
		return NULL;
	}
	rewind(op);
	return op;
}

static void
decoder_cb(void *arg, const char *buf, unsigned long len)
{
	if (arg && buf)
		fwrite(buf, 1, len, arg);
}

static void
base64_cb(void *arg, const char *buf, unsigned long len)
{
	static char	back[972];
	static int	fill;
	unsigned long	pos;

	if (arg && buf && buf != (void *)-1) {
		pos = 0;
		while (len - pos >= sizeof back - fill) {
			memcpy(&back[fill], &buf[pos], sizeof back - fill);
			mime_write(back, sizeof back, arg,
					CONV_TOB64, TD_NONE, NULL, 0,
					NULL, NULL);
			pos += sizeof back - fill;
			fill = 0;
		}
		memcpy(&back[fill], &buf[pos], len - pos);
		fill += len - pos;
	} else if (buf == (void *)-1) {
		mime_write(back, fill, arg,
				CONV_TOB64, TD_NONE, NULL, 0,
				NULL, NULL);
		fill = 0;
	}
}

static int
verify1(struct message *m, int n)
{
	SECItem	**digests;
	NSSCMSMessage	*msg;
	PLArenaPool	*poolp;
	SECAlgorithmID	**algids;
	CERTCertDBHandle	*handle;
	int	nlevels, i;
	int	status = 0;
	int	foundsender = 0;
	char	*sender;

	if ((m = getsig(m, n, &msg)) == NULL)
		return 1;
	sender = getsender(m);
	handle = CERT_GetDefaultCertDB();
	nlevels = NSS_CMSMessage_ContentLevelCount(msg);
	for (i = 0; i < nlevels; i++) {
		NSSCMSContentInfo	*content;
		SECOidTag	tag;

		content = NSS_CMSMessage_ContentLevel(msg, i);
		tag = NSS_CMSContentInfo_GetContentTypeTag(content);
		if (tag == SEC_OID_PKCS7_SIGNED_DATA) {
			NSSCMSSignedData	*data;
			int	nsigners, j;

			if ((data = NSS_CMSContentInfo_GetContent(content))
					== NULL) {
				fprintf(stderr, "Signed data missing for "
						"message %d.\n", n);
				status = -1;
				break;
			}
			if (!NSS_CMSSignedData_HasDigests(data)) {
				algids = NSS_CMSSignedData_GetDigestAlgs(data);
				if (getdig(m, n, &digests, &poolp, algids)
						!= OKAY) {
					status = -1;
					break;
				}
				if (NSS_CMSSignedData_SetDigests(data, algids,
							digests)
						!= SECSuccess) {
					fprintf(stderr, "Cannot set digests "
							"for message %d.\n", n);
					status = -1;
					break;
				}
				PORT_FreeArena(poolp, PR_FALSE);
			}
			if (NSS_CMSSignedData_ImportCerts(data, handle,
						certUsageEmailSigner,
						PR_FALSE) != SECSuccess) {
				fprintf(stderr, "Cannot temporarily import "
						"certificates for "
						"message %d.\n", n);
				status = -1;
				break;
			}
			nsigners = NSS_CMSSignedData_SignerInfoCount(data);
			if (nsigners == 0) {
				fprintf(stderr, "Message %d has no signers.\n",
						n);
				status = -1;
				break;
			}
			if (!NSS_CMSSignedData_HasDigests(data)) {
				fprintf(stderr, "Message %d has no digests.\n",
						n);
				status = -1;
				break;
			}
			for (j = 0; j < nsigners; j++) {
				const char	*svs;
				NSSCMSSignerInfo	*info;
				NSSCMSVerificationStatus	vs;
				SECStatus	bad;
				CERTCertificate	*cert;
				const char	*addr;
				int	passed = 0;

				info = NSS_CMSSignedData_GetSignerInfo(data, j);
				cert = NSS_CMSSignerInfo_GetSigningCertificate
					(info, handle);
				bad = NSS_CMSSignedData_VerifySignerInfo(data,
						j, handle,
						certUsageEmailSigner);
				vs = NSS_CMSSignerInfo_GetVerificationStatus
					(info);
				svs = NSS_CMSUtil_VerificationStatusToString
					(vs);
				addr = CERT_GetCertEmailAddress(&cert->subject);
				if (sender != NULL && addr != NULL &&
						asccasecmp(sender, addr) == 0)
					foundsender++;
				else {
					addr = CERT_GetFirstEmailAddress(cert);
					while (sender && addr) {
						if (!asccasecmp(sender, addr)) {
							foundsender++;
							break;
						}
						addr = CERT_GetNextEmailAddress
							(cert, addr);
					}
				}
				if (CERT_VerifyCertNow(handle,
						cert, PR_TRUE,
						certUsageEmailSigner,
						NULL) != SECSuccess)
					fprintf(stderr, "Bad certificate for "
							"signer <%s> of "
							"message %d: %s.\n",
							addr ? addr : "?", n,
							bad_cert_str());
				else
					passed++;
				if (bad)
					fprintf(stderr, "Bad status for "
							"signer <%s> of "
							"message %d: %s.\n",
							addr ? addr : "?",
							n, svs);
				else
					passed++;
				if (passed < 2)
					status = -1;
				else if (status == 0)
					status = 1;
			}
		}
	}
	if (foundsender == 0) {
		if (sender) {
			fprintf(stderr, "Signers of message "
					"%d do not include the sender <%s>\n",
				n, sender);
			status = -1;
		} else
			fprintf(stderr, "Warning: Message %d has no From: "
					"header field.\n", n);
	} else if (status == 1)
		printf("Message %d was verified successfully.\n", n);
	if (status == 0)
		fprintf(stderr, "No verification information found in "
				"message %d.\n", n);
	NSS_CMSMessage_Destroy(msg);
	return status != 1;
}

static struct message *
getsig(struct message *m, int n, NSSCMSMessage **msg)
{
	struct message	*x;
	char	*ct, *pt, *boundary = NULL, *cte;
	char	*buf = NULL;
	size_t	bufsize = 0, buflen, count, boundlen = -1;
	int	part;
	FILE	*fp;
	NSSCMSDecoderContext	*decctx;
	struct str	in, out;
	char	*to, *cc;
	int	inhdr, binary;
	int	detached = 1;

loop:	if ((ct = hfield("content-type", m)) == NULL)
		goto not;
	if (strncmp(ct, "application/x-pkcs7-mime", 24) == 0 ||
			strncmp(ct, "application/pkcs7-mime", 22) == 0) {
		to = hfield("to", m);
		cc = hfield("cc", m);
		if ((x = smime_decrypt(m, to, cc, 1)) == NULL)
			return NULL;
		if (x != (struct message *)-1) {
			m = x;
			goto loop;
		}
		detached = 0;
	} else if (strncmp(ct, "multipart/signed", 16) ||
			(pt = mime_getparam("protocol", ct)) == NULL ||
			strcmp(pt, "application/x-pkcs7-signature") &&
			 strcmp(pt, "application/pkcs7-signature") ||
			(boundary = mime_getboundary(ct)) == NULL) {
	not:	fprintf(stderr,
			"Message %d is not an S/MIME signed message.\n", n);
		return NULL;
	} else
		boundlen = strlen(boundary);
	if ((decctx = NSS_CMSDecoder_Start(NULL, NULL, NULL,
					password_cb, "Pass phrase:",
					NULL, NULL)) == NULL) {
		fprintf(stderr, "Cannot start decoder.\n");
		return NULL;
	}
	if ((fp = setinput(&mb, m, NEED_BODY)) == NULL) {
		return NULL;
	}
	count = m->m_size;
	part = 0;
	inhdr = 1;
	binary = 0;
	while (fgetline(&buf, &bufsize, &count, &buflen, fp, 0) != NULL) {
		if (detached && boundary && buflen >= boundlen + 1 &&
				strncmp(buf, boundary, boundlen) == 0) {
			if (buf[boundlen] == '\n') {
				part++;
				inhdr = 1;
				binary = 0;
				if (part >= 3) {
					fprintf(stderr, "Message %d has too "
							"many parts.\n", n);
					free(buf);
					return NULL;
				}
				continue;
			}
			if (buf[boundlen] == '-' && buf[boundlen+1] == '-' &&
					buf[boundlen+2] == '\n')
				break;
		} else if (buf[0] == '\n') {
			inhdr = 0;
			continue;
		}
		if ((!detached || part == 2) && inhdr == 0) {
			if (binary)
				NSS_CMSDecoder_Update(decctx, buf, buflen);
			else {
				in.s = buf;
				in.l = buflen;
				mime_fromb64_b(&in, &out, 0, fp);
				NSS_CMSDecoder_Update(decctx, out.s, out.l);
				free(out.s);
			}
		}
		if (buflen == 1 && buf[0] == '\n')
			inhdr = 0;
		if (inhdr && (cte = thisfield(buf, "content-transfer-encoding"))
				!= NULL && ascncasecmp(cte, "binary", 7) == 0)
			binary = 1;
	}
	free(buf);
	if ((*msg = NSS_CMSDecoder_Finish(decctx)) == NULL) {
		fprintf(stderr, "Failed to decode signature for message %d.\n",
				n);
		return NULL;
	}
	return m;
}

static enum okay
getdig(struct message *m, int n, SECItem ***digests,
		PLArenaPool **poolp, SECAlgorithmID **algids)
{
	char	*ct, *pt, *boundary;
	char	*buf = NULL;
	size_t	bufsize = 0, buflen, count, boundlen;
	int	part;
	int	nl;
	FILE	*fp;
	NSSCMSDigestContext	*digctx;

	*poolp = PORT_NewArena(1024);
	if ((ct = hfield("content-type", m)) == NULL ||
			strncmp(ct, "multipart/signed", 16) ||
			(pt = mime_getparam("protocol", ct)) == NULL ||
			strcmp(pt, "application/x-pkcs7-signature") &&
			 strcmp(pt, "application/pkcs7-signature") ||
			(boundary = mime_getboundary(ct)) == NULL) {
		fprintf(stderr,
			"Message %d is not an S/MIME signed message.\n", n);
		return STOP;
	}
	boundlen = strlen(boundary);
	if ((digctx = NSS_CMSDigestContext_StartMultiple(algids)) == NULL) {
		fprintf(stderr, "Cannot start digest computation.\n");
		return STOP;
	}
	if ((fp = setinput(&mb, m, NEED_BODY)) == NULL) {
		return STOP;
	}
	count = m->m_size;
	part = 0;
	nl = 0;
	while (fgetline(&buf, &bufsize, &count, &buflen, fp, 0) != NULL) {
		if (buflen >= boundlen + 1 &&
				strncmp(buf, boundary, boundlen) == 0) {
			if (buf[boundlen] == '\n') {
				if (++part >= 2)
					break;
				continue;
			}
			if (buf[boundlen] == '-' && buf[boundlen+1] == '-' &&
					buf[boundlen+2] == '\n')
				break;
		}
		if (part == 1) {
			if (nl) {
				NSS_CMSDigestContext_Update(digctx,
						(unsigned char *)"\r\n", 2);
				nl = 0;
			}
			if (buf[buflen-1] == '\n') {
				nl = 1;
				buflen--;
			}
			NSS_CMSDigestContext_Update(digctx,
					(unsigned char *)buf, buflen);
			continue;
		}
	}
	free(buf);
	if (NSS_CMSDigestContext_FinishMultiple(digctx,
				*poolp, digests) != SECSuccess) {
		fprintf(stderr, "Error creating digest for message %d\n", n);
		return STOP;
	}
	return OKAY;
}

static void
nsscatch(int s)
{
	if (reset_tio)
		tcsetattr(0, TCSADRAIN, &otio);
	siglongjmp(nssjmp, s);
}

enum okay
smime_certsave(struct message *m, int n, FILE *op)
{
	NSSCMSMessage	*msg;
	CERTCertDBHandle	*handle;
	int	nlevels, i, cnt = 0;
	enum okay	ok = OKAY;

	if (nss_init() == STOP)
		return STOP;
	if ((m = getsig(m, n, &msg)) == NULL)
		return 1;
	handle = CERT_GetDefaultCertDB();
	nlevels = NSS_CMSMessage_ContentLevelCount(msg);
	for (i = 0; i < nlevels; i++) {
		NSSCMSContentInfo	*content;
		SECOidTag	tag;

		content = NSS_CMSMessage_ContentLevel(msg, i);
		tag = NSS_CMSContentInfo_GetContentTypeTag(content);
		if (tag == SEC_OID_PKCS7_SIGNED_DATA) {
			NSSCMSSignedData	*data;
			int	nsigners, j;

			if ((data = NSS_CMSContentInfo_GetContent(content))
					== NULL) {
				fprintf(stderr, "Signed data missing for "
						"message %d.\n", n);
				ok = STOP;
				break;
			}
			if (NSS_CMSSignedData_ImportCerts(data, handle,
						certUsageEmailSigner,
						PR_FALSE) != SECSuccess) {
				fprintf(stderr, "Cannot temporarily import "
						"certificates for "
						"message %d.\n", n);
				ok = STOP;
				break;
			}
			nsigners = NSS_CMSSignedData_SignerInfoCount(data);
			if (nsigners == 0) {
				fprintf(stderr, "Message %d has no signers.\n",
						n);
				ok = STOP;
				break;
			}
			for (j = 0; j < nsigners; j++) {
				NSSCMSSignerInfo	*info;
				CERTCertificateList	*list;
				CERTCertificate	*cert;
				int	k;

				info = NSS_CMSSignedData_GetSignerInfo(data, j);
				list = NSS_CMSSignerInfo_GetCertList(info);
				if (list) {
					for (k = 0; k < list->len; k++) {
						cert = (CERTCertificate *)
							&list->certs[k];
						dumpcert(cert, op);
						cnt++;
					}
				}
				cert = NSS_CMSSignerInfo_GetSigningCertificate
					(info, handle);
				if (cert) {
					dumpcert(cert, op);
					cnt++;
				}
			}
		}
	}
	NSS_CMSMessage_Destroy(msg);
	if (cnt == 0) {
		fprintf(stderr, "No certificates found in message %d.\n", n);
		ok = STOP;
	}
	return ok;
}

static void
dumpcert(CERTCertificate *cert, FILE *op)
{
	fprintf(op, "subject=%s\n", cert->subjectName);
	fprintf(op, "issuer=%s\n", cert->issuerName);
	fputs("-----BEGIN CERTIFICATE-----\n", op);
	mime_write(cert->derCert.data,
		cert->derCert.len, op,
		CONV_TOB64, TD_NONE, NULL, 0,
		NULL, NULL);
	fputs("-----END CERTIFICATE-----\n", op);
}

static enum okay
getcipher(const char *to, SECOidTag *alg, int *key)
{
	char	*vn, *cp;
	int	vs;

	*key = 0;
	*alg = SEC_OID_DES_EDE3_CBC;
	vn = ac_alloc(vs = strlen(to) + 30);
	snprintf(vn, vs, "smime-cipher-%s", to);
	if ((cp = value(vn)) != NULL) {
		if (strcmp(cp, "rc2-40") == 0) {
			*alg = SEC_OID_RC2_CBC;
			*key = 40;
		} else if (strcmp(cp, "rc2-64") == 0) {
			*alg = SEC_OID_RC2_CBC;
			*key = 64;
		} else if (strcmp(cp, "rc2-128") == 0) {
			*alg = SEC_OID_RC2_CBC;
			*key = 128;
		} else if (strcmp(cp, "des") == 0)
			*alg = SEC_OID_DES_CBC;
		else if (strcmp(cp, "fortezza") == 0)
			*alg = SEC_OID_FORTEZZA_SKIPJACK;
		else if (strcmp(cp, "des-ede3") == 0)
			/*EMPTY*/;
		else {
			fprintf(stderr, "Invalid cipher \"%s\".\n", cp);
			return STOP;
		}
	}
	ac_free(vn);
	return OKAY;
}
#endif	/* USE_NSS */

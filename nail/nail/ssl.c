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
static char sccsid[] = "@(#)ssl.c	1.39 (gritter) 6/12/06";
#endif
#endif /* not lint */

#include "config.h"

#ifdef	USE_SSL

#include "rcv.h"
#include "extern.h"

void 
ssl_set_vrfy_level(const char *uhp)
{
	char *cp;
	char *vrvar;

	ssl_vrfy_level = VRFY_ASK;
	vrvar = ac_alloc(strlen(uhp) + 12);
	strcpy(vrvar, "ssl-verify-");
	strcpy(&vrvar[11], uhp);
	if ((cp = value(vrvar)) == NULL)
		cp = value("ssl-verify");
	ac_free(vrvar);
	if (cp != NULL) {
		if (equal(cp, "strict"))
			ssl_vrfy_level = VRFY_STRICT;
		else if (equal(cp, "ask"))
			ssl_vrfy_level = VRFY_ASK;
		else if (equal(cp, "warn"))
			ssl_vrfy_level = VRFY_WARN;
		else if (equal(cp, "ignore"))
			ssl_vrfy_level = VRFY_IGNORE;
		else
			fprintf(stderr, catgets(catd, CATSET, 265,
					"invalid value of ssl-verify: %s\n"),
					cp);
	}
}

enum okay 
ssl_vrfy_decide(void)
{
	enum okay ok = STOP;

	switch (ssl_vrfy_level) {
	case VRFY_STRICT:
		ok = STOP;
		break;
	case VRFY_ASK:
		{
			char *line = NULL;
			size_t linesize = 0;

			fprintf(stderr, catgets(catd, CATSET, 264,
					"Continue (y/n)? "));
			if (readline(stdin, &line, &linesize) > 0 &&
					*line == 'y')
				ok = OKAY;
			else
				ok = STOP;
			if (line)
				free(line);
		}
		break;
	case VRFY_WARN:
	case VRFY_IGNORE:
		ok = OKAY;
	}
	return ok;
}

char *
ssl_method_string(const char *uhp)
{
	char *cp, *mtvar;

	mtvar = ac_alloc(strlen(uhp) + 12);
	strcpy(mtvar, "ssl-method-");
	strcpy(&mtvar[11], uhp);
	if ((cp = value(mtvar)) == NULL)
		cp = value("ssl-method");
	ac_free(mtvar);
	return cp;
}

enum okay
smime_split(FILE *ip, FILE **hp, FILE **bp, long xcount, int keep)
{
	char	*buf, *hn, *bn;
	char	*savedfields = NULL;
	size_t	bufsize, buflen, count, savedsize = 0;
	int	c;

	if ((*hp = Ftemp(&hn, "Rh", "w+", 0600, 1)) == NULL ||
			(*bp = Ftemp(&bn, "Rb", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return STOP;
	}
	rm(hn);
	rm(bn);
	Ftfree(&hn);
	Ftfree(&bn);
	buf = smalloc(bufsize = LINESIZE);
	savedfields = smalloc(savedsize = 1);
	*savedfields = '\0';
	if (xcount < 0)
		count = fsize(ip);
	else
		count = xcount;
	while (fgetline(&buf, &bufsize, &count, &buflen, ip, 0) != NULL &&
			*buf != '\n') {
		if (ascncasecmp(buf, "content-", 8) == 0) {
			if (keep)
				fputs("X-Encoded-", *hp);
			for (;;) {
				savedsize += buflen;
				savedfields = srealloc(savedfields, savedsize);
				strcat(savedfields, buf);
				if (keep)
					fwrite(buf, sizeof *buf, buflen, *hp);
				c = getc(ip);
				ungetc(c, ip);
				if (!blankchar(c))
					break;
				fgetline(&buf, &bufsize, &count, &buflen,
						ip, 0);
			}
			continue;
		}
		fwrite(buf, sizeof *buf, buflen, *hp);
	}
	fflush(*hp);
	rewind(*hp);
	fputs(savedfields, *bp);
	putc('\n', *bp);
	while (fgetline(&buf, &bufsize, &count, &buflen, ip, 0) != NULL)
		fwrite(buf, sizeof *buf, buflen, *bp);
	fflush(*bp);
	rewind(*bp);
	free(buf);
	return OKAY;
}

FILE *
smime_sign_assemble(FILE *hp, FILE *bp, FILE *sp)
{
	char	*boundary, *cp;
	FILE	*op;
	int	c, lastc = EOF;

	if ((op = Ftemp(&cp, "Rs", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	boundary = makeboundary();
	while ((c = getc(hp)) != EOF) {
		if (c == '\n' && lastc == '\n')
			break;
		putc(c, op);
		lastc = c;
	}
	fprintf(op, "Content-Type: multipart/signed;\n"
		" protocol=\"application/x-pkcs7-signature\"; micalg=sha1;\n"
		" boundary=\"%s\"\n\n", boundary);
	fprintf(op, "This is an S/MIME signed message.\n\n--%s\n",
			boundary);
	while ((c = getc(bp)) != EOF)
		putc(c, op);
	fprintf(op, "\n--%s\n", boundary);
	fputs("Content-Type: application/x-pkcs7-signature; "
			"name=\"smime.p7s\"\n"
		"Content-Transfer-Encoding: base64\n"
		"Content-Disposition: attachment; filename=\"smime.p7s\"\n\n",
		op);
	while ((c = getc(sp)) != EOF) {
		if (c == '-') {
			while ((c = getc(sp)) != EOF && c != '\n');
			continue;
		}
		putc(c, op);
	}
	fprintf(op, "\n--%s--\n", boundary);
	Fclose(hp);
	Fclose(bp);
	Fclose(sp);
	fflush(op);
	if (ferror(op)) {
		perror("signed output data");
		Fclose(op);
		return NULL;
	}
	rewind(op);
	return op;
}

FILE *
smime_encrypt_assemble(FILE *hp, FILE *yp)
{
	char	*cp;
	FILE	*op;
	int	c, lastc = EOF;

	if ((op = Ftemp(&cp, "Rs", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return NULL;
	}
	rm(cp);
	Ftfree(&cp);
	while ((c = getc(hp)) != EOF) {
		if (c == '\n' && lastc == '\n')
			break;
		putc(c, op);
		lastc = c;
	}
	fprintf(op, "Content-Type: application/x-pkcs7-mime; "
			"name=\"smime.p7m\"\n"
		"Content-Transfer-Encoding: base64\n"
		"Content-Disposition: attachment; "
			"filename=\"smime.p7m\"\n\n");
	while ((c = getc(yp)) != EOF) {
		if (c == '-') {
			while ((c = getc(yp)) != EOF && c != '\n');
			continue;
		}
		putc(c, op);
	}
	Fclose(hp);
	Fclose(yp);
	fflush(op);
	if (ferror(op)) {
		perror("encrypted output data");
		Fclose(op);
		return NULL;
	}
	rewind(op);
	return op;
}

struct message *
smime_decrypt_assemble(struct message *m, FILE *hp, FILE *bp)
{
	int	binary = 0, lastnl = 0;
	char	*buf = NULL, *cp;
	size_t	bufsize = 0, buflen, count;
	long	lines = 0, octets = 0;
	struct message	*x;
	off_t	offset;

	x = salloc(sizeof *x);
	*x = *m;
	fflush(mb.mb_otf);
	fseek(mb.mb_otf, 0L, SEEK_END);
	offset = ftell(mb.mb_otf);
	count = fsize(hp);
	while (fgetline(&buf, &bufsize, &count, &buflen, hp, 0) != NULL) {
		if (buf[0] == '\n')
			break;
		if ((cp = thisfield(buf, "content-transfer-encoding")) != NULL)
			if (ascncasecmp(cp, "binary", 7) == 0)
				binary = 1;
		fwrite(buf, sizeof *buf, buflen, mb.mb_otf);
		octets += buflen;
		lines++;
	}
	octets += mkdate(mb.mb_otf, "X-Decoding-Date");
	lines++;
	count = fsize(bp);
	while (fgetline(&buf, &bufsize, &count, &buflen, bp, 0) != NULL) {
		lines++;
		if (!binary && buf[buflen-1] == '\n' && buf[buflen-2] == '\r')
			buf[--buflen-1] = '\n';
		fwrite(buf, sizeof *buf, buflen, mb.mb_otf);
		octets += buflen;
		if (buf[0] == '\n')
			lastnl++;
		else if (buf[buflen-1] == '\n')
			lastnl = 1;
		else
			lastnl = 0;
	}
	while (!binary && lastnl < 2) {
		putc('\n', mb.mb_otf);
		lines++;
		octets++;
		lastnl++;
	}
	Fclose(hp);
	Fclose(bp);
	free(buf);
	fflush(mb.mb_otf);
	if (ferror(mb.mb_otf)) {
		perror("decrypted output data");
		return NULL;
	}
	x->m_size = x->m_xsize = octets;
	x->m_lines = x->m_xlines = lines;
	x->m_block = mailx_blockof(offset);
	x->m_offset = mailx_offsetof(offset);
	return x;
}

int 
ccertsave(void *v)
{
	int	*ip;
	int	f, *msgvec;
	char	*file = NULL, *str = v;
	int	val = 0;
	FILE	*fp;

	msgvec = salloc((msgCount + 2) * sizeof *msgvec);
	if ((file = laststring(str, &f, 1)) == NULL) {
		fprintf(stderr, "No file to save certificate given.\n");
		return 1;
	}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			if (inhook)
				return 0;
			fprintf(stderr,
				"No messages to get certificates from.\n");
			return 1;
		}
		msgvec[1] = 0;
	} else if (getmsglist(str, msgvec, 0) < 0)
		return 1;
	if (*msgvec == 0) {
		if (inhook)
			return 0;
		fprintf(stderr, "No applicable messages.\n");
		return 1;
	}
	if ((fp = Fopen(file, "a")) == NULL) {
		perror(file);
		return 1;
	}
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++)
		if (smime_certsave(&message[*ip-1], *ip, fp) != OKAY)
			val = 1;
	Fclose(fp);
	if (val == 0)
		printf("Certificate(s) saved.\n");
	return val;
}
enum okay
rfc2595_hostname_match(const char *host, const char *pattern)
{
	if (pattern[0] == '*' && pattern[1] == '.') {
		pattern++;
		while (*host && *host != '.')
			host++;
	}
	return asccasecmp(host, pattern) == 0 ? OKAY : STOP;
}
#endif	/* USE_SSL */

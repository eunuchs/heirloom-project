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
static char sccsid[] = "@(#)sendout.c	2.100 (gritter) 3/1/09";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "md5.h"

/*
 * Mail -- a mail program
 *
 * Mail to others.
 */

static char	*send_boundary;

static char *getencoding(enum conversion convert);
static struct name *fixhead(struct header *hp, struct name *tolist);
static int put_signature(FILE *fo, int convert);
static int attach_file1(struct attachment *ap, FILE *fo, int dosign);
static int attach_file(struct attachment *ap, FILE *fo, int dosign);
static int attach_message(struct attachment *ap, FILE *fo, int dosign);
static int make_multipart(struct header *hp, int convert, FILE *fi, FILE *fo,
		const char *contenttype, const char *charset, int dosign);
static FILE *infix(struct header *hp, FILE *fi, int dosign);
static int savemail(char *name, FILE *fi);
static int sendmail_internal(void *v, int recipient_record);
static enum okay transfer(struct name *to, struct name *mailargs, FILE *input,
		struct header *hp);
static enum okay start_mta(struct name *to, struct name *mailargs, FILE *input,
		struct header *hp);
static void message_id(FILE *fo, struct header *hp);
static int fmt(char *str, struct name *np, FILE *fo, int comma,
		int dropinvalid, int domime);
static int infix_resend(FILE *fi, FILE *fo, struct message *mp,
		struct name *to, int add_resent);

/*
 * Generate a boundary for MIME multipart messages.
 */
char *
makeboundary(void)
{
	static char bound[70];
	time_t	now;

	time(&now);
	snprintf(bound, sizeof bound, "=_%lx.%s", (long)now, getrandstring(48));
	send_boundary = bound;
	return send_boundary;
}

/*
 * Get an encoding flag based on the given string.
 */
static char *
getencoding(enum conversion convert)
{
	switch (convert) {
	case CONV_7BIT:
		return "7bit";
	case CONV_8BIT:
		return "8bit";
	case CONV_TOQP:
		return "quoted-printable";
	case CONV_TOB64:
		return "base64";
	default:
		break;
	}
	/*NOTREACHED*/
	return NULL;
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 */
static struct name *
fixhead(struct header *hp, struct name *tolist)
{
	struct name *np;

	hp->h_to = NULL;
	hp->h_cc = NULL;
	hp->h_bcc = NULL;
	for (np = tolist; np != NULL; np = np->n_flink)
		if ((np->n_type & GMASK) == GTO)
			hp->h_to =
				cat(hp->h_to, nalloc(np->n_fullname,
							np->n_type|GFULL));
		else if ((np->n_type & GMASK) == GCC)
			hp->h_cc =
				cat(hp->h_cc, nalloc(np->n_fullname,
							np->n_type|GFULL));
		else if ((np->n_type & GMASK) == GBCC)
			hp->h_bcc =
				cat(hp->h_bcc, nalloc(np->n_fullname,
							np->n_type|GFULL));
	return tolist;
}


/*
 * Do not change, you get incorrect base64 encodings else!
 */
#define	INFIX_BUF	972

/*
 * Put the signature file at fo.
 */
static int
put_signature(FILE *fo, int convert)
{
	char *sig, buf[INFIX_BUF], c = '\n';
	FILE *fsig;
	size_t sz;

	sig = value("signature");
	if (sig == NULL || *sig == '\0')
		return 0;
	else
		sig = expand(sig);
	if ((fsig = Fopen(sig, "r")) == NULL) {
		perror(sig);
		return -1;
	}
	while ((sz = fread(buf, sizeof *buf, INFIX_BUF, fsig)) != 0) {
		c = buf[sz - 1];
		if (mime_write(buf, sz, fo, convert, TD_NONE,
					NULL, (size_t)0, NULL, NULL)
				== 0) {
			perror(sig);
			Fclose(fsig);
			return -1;
		}
	}
	if (ferror(fsig)) {
		perror(sig);
		Fclose(fsig);
		return -1;
	}
	Fclose(fsig);
	if (c != '\n')
		putc('\n', fo);
	return 0;
}

/*
 * Write an attachment to the file buffer, converting to MIME.
 */
static int
attach_file1(struct attachment *ap, FILE *fo, int dosign)
{
	FILE *fi;
	char *charset = NULL, *contenttype = NULL, *basename;
	enum conversion convert = CONV_TOB64;
	int err = 0;
	enum mimeclean isclean;
	size_t sz;
	char *buf;
	size_t bufsize, count;
	int	lastc = EOF;
#ifdef	HAVE_ICONV
	char	*tcs;
#endif

	if ((fi = Fopen(ap->a_name, "r")) == NULL) {
		perror(ap->a_name);
		return -1;
	}
	if ((basename = strrchr(ap->a_name, '/')) == NULL)
		basename = ap->a_name;
	else
		basename++;
	if (ap->a_content_type)
		contenttype = ap->a_content_type;
	else
		contenttype = mime_filecontent(basename);
	if (ap->a_charset)
		charset = ap->a_charset;
	convert = get_mime_convert(fi, &contenttype, &charset, &isclean,
			dosign);
	fprintf(fo,
		"\n--%s\n"
		"Content-Type: %s",
		send_boundary, contenttype);
	if (charset == NULL)
		putc('\n', fo);
	else
		fprintf(fo, ";\n charset=%s\n", charset);
	if (ap->a_content_disposition == NULL)
		ap->a_content_disposition = "attachment";
	fprintf(fo, "Content-Transfer-Encoding: %s\n"
		"Content-Disposition: %s;\n"
		" filename=\"",
		getencoding(convert),
		ap->a_content_disposition);
	mime_write(basename, strlen(basename), fo,
			CONV_TOHDR, TD_NONE, NULL, (size_t)0, NULL, NULL);
	fwrite("\"\n", sizeof (char), 2, fo);
	if (ap->a_content_id)
		fprintf(fo, "Content-ID: %s\n", ap->a_content_id);
	if (ap->a_content_description)
		fprintf(fo, "Content-Description: %s\n",
				ap->a_content_description);
	putc('\n', fo);
#ifdef	HAVE_ICONV
	if (iconvd != (iconv_t)-1) {
		iconv_close(iconvd);
		iconvd = (iconv_t)-1;
	}
	tcs = gettcharset();
	if ((isclean & (MIME_HASNUL|MIME_CTRLCHAR)) == 0 &&
			ascncasecmp(contenttype, "text/", 5) == 0 &&
			isclean & MIME_HIGHBIT &&
			charset != NULL) {
		if ((iconvd = iconv_open_ft(charset, tcs)) == (iconv_t)-1 &&
				errno != 0) {
			if (errno == EINVAL)
				fprintf(stderr, catgets(catd, CATSET, 179,
			"Cannot convert from %s to %s\n"), tcs, charset);
			else
				perror("iconv_open");
			Fclose(fi);
			return -1;
		}
	}
#endif	/* HAVE_ICONV */
	buf = smalloc(bufsize = INFIX_BUF);
	if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
			|| iconvd != (iconv_t)-1
#endif
			) {
		fflush(fi);
		count = fsize(fi);
	}
	for (;;) {
		if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
				|| iconvd != (iconv_t)-1
#endif
				) {
			if (fgetline(&buf, &bufsize, &count, &sz, fi, 0)
					== NULL)
				break;
		} else {
			if ((sz = fread(buf, sizeof *buf, bufsize, fi)) == 0)
				break;
		}
		lastc = buf[sz-1];
		if (mime_write(buf, sz, fo, convert, TD_ICONV,
					NULL, (size_t)0, NULL, NULL) == 0)
			err = -1;
	}
	if (convert == CONV_TOQP && lastc != '\n')
		fwrite("=\n", 1, 2, fo);
	if (ferror(fi))
		err = -1;
	Fclose(fi);
	free(buf);
	return err;
}

/*
 * Try out different character set conversions to attach a file.
 */
static int
attach_file(struct attachment *ap, FILE *fo, int dosign)
{
	char	*_wantcharset, *charsets, *ncs;
	size_t	offs = ftell(fo);

	if (ap->a_charset || (charsets = value("sendcharsets")) == NULL)
		return attach_file1(ap, fo, dosign);
	_wantcharset = wantcharset;
	wantcharset = savestr(charsets);
loop:	if ((ncs = strchr(wantcharset, ',')) != NULL)
		*ncs++ = '\0';
try:	if (attach_file1(ap, fo, dosign) != 0) {
		if (errno == EILSEQ || errno == EINVAL) {
			if (ncs && *ncs) {
				wantcharset = ncs;
				clearerr(fo);
				fseek(fo, offs, SEEK_SET);
				goto loop;
			}
			if (wantcharset) {
				if (wantcharset == (char *)-1)
					wantcharset = NULL;
				else {
					wantcharset = (char *)-1;
					clearerr(fo);
					fseek(fo, offs, SEEK_SET);
					goto try;
				}
			}
		}
	}
	wantcharset = _wantcharset;
	return 0;
}

/*
 * Attach a message to the file buffer.
 */
static int
attach_message(struct attachment *ap, FILE *fo, int dosign)
{
	struct message	*mp;

	fprintf(fo, "\n--%s\n"
		    "Content-Type: message/rfc822\n"
		    "Content-Disposition: inline\n\n", send_boundary);
	mp = &message[ap->a_msgno - 1];
	touch(mp);
	if (send(mp, fo, 0, NULL, SEND_RFC822, NULL) < 0)
		return -1;
	return 0;
}

/*
 * Generate the body of a MIME multipart message.
 */
static int
make_multipart(struct header *hp, int convert, FILE *fi, FILE *fo,
		const char *contenttype, const char *charset, int dosign)
{
	struct attachment *att;

	fputs("This is a multi-part message in MIME format.\n", fo);
	if (fsize(fi) != 0) {
		char *buf, c = '\n';
		size_t sz, bufsize, count;

		fprintf(fo, "\n--%s\n", send_boundary);
		fprintf(fo, "Content-Type: %s", contenttype);
		if (charset)
			fprintf(fo, "; charset=%s", charset);
		fprintf(fo, "\nContent-Transfer-Encoding: %s\n"
				"Content-Disposition: inline\n\n",
				getencoding(convert));
		buf = smalloc(bufsize = INFIX_BUF);
		if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
				|| iconvd != (iconv_t)-1
#endif	/* HAVE_ICONV */
				) {
			fflush(fi);
			count = fsize(fi);
		}
		for (;;) {
			if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
					|| iconvd != (iconv_t)-1
#endif	/* HAVE_ICONV */
					) {
				if (fgetline(&buf, &bufsize, &count, &sz, fi, 0)
						== NULL)
					break;
			} else {
				sz = fread(buf, sizeof *buf, bufsize, fi);
				if (sz == 0)
					break;
			}
			c = buf[sz - 1];
			if (mime_write(buf, sz, fo, convert,
					TD_ICONV, NULL, (size_t)0,
					NULL, NULL) == 0) {
				free(buf);
				return -1;
			}
		}
		free(buf);
		if (ferror(fi))
			return -1;
		if (c != '\n')
			putc('\n', fo);
		if (charset != NULL)
			put_signature(fo, convert);
	}
	for (att = hp->h_attach; att != NULL; att = att->a_flink) {
		if (att->a_msgno) {
			if (attach_message(att, fo, dosign) != 0)
				return -1;
		} else {
			if (attach_file(att, fo, dosign) != 0)
				return -1;
		}
	}
	/* the final boundary with two attached dashes */
	fprintf(fo, "\n--%s--\n", send_boundary);
	return 0;
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */
static FILE *
infix(struct header *hp, FILE *fi, int dosign)
{
	FILE *nfo, *nfi;
	char *tempMail;
#ifdef	HAVE_ICONV
	char *tcs, *convhdr = NULL;
#endif
	enum mimeclean isclean;
	enum conversion convert;
	char *charset = NULL, *contenttype = NULL;
	int	lastc = EOF;

	if ((nfo = Ftemp(&tempMail, "Rs", "w", 0600, 1)) == NULL) {
		perror(catgets(catd, CATSET, 178, "temporary mail file"));
		return(NULL);
	}
	if ((nfi = Fopen(tempMail, "r")) == NULL) {
		perror(tempMail);
		Fclose(nfo);
		return(NULL);
	}
	rm(tempMail);
	Ftfree(&tempMail);
	convert = get_mime_convert(fi, &contenttype, &charset,
			&isclean, dosign);
#ifdef	HAVE_ICONV
	tcs = gettcharset();
	if ((convhdr = need_hdrconv(hp, GTO|GSUBJECT|GCC|GBCC|GIDENT)) != 0) {
		if (iconvd != (iconv_t)-1)
			iconv_close(iconvd);
		if ((iconvd = iconv_open_ft(convhdr, tcs)) == (iconv_t)-1
				&& errno != 0) {
			if (errno == EINVAL)
				fprintf(stderr, catgets(catd, CATSET, 179,
			"Cannot convert from %s to %s\n"), tcs, convhdr);
			else
				perror("iconv_open");
			Fclose(nfo);
			return NULL;
		}
	}
#endif	/* HAVE_ICONV */
	if (puthead(hp, nfo,
		   GTO|GSUBJECT|GCC|GBCC|GNL|GCOMMA|GUA|GMIME
		   |GMSGID|GIDENT|GREF|GDATE,
		   SEND_MBOX, convert, contenttype, charset)) {
		Fclose(nfo);
		Fclose(nfi);
#ifdef	HAVE_ICONV
		if (iconvd != (iconv_t)-1) {
			iconv_close(iconvd);
			iconvd = (iconv_t)-1;
		}
#endif
		return NULL;
	}
#ifdef	HAVE_ICONV
	if (convhdr && iconvd != (iconv_t)-1) {
		iconv_close(iconvd);
		iconvd = (iconv_t)-1;
	}
	if ((isclean & (MIME_HASNUL|MIME_CTRLCHAR)) == 0 &&
			ascncasecmp(contenttype, "text/", 5) == 0 &&
			isclean & MIME_HIGHBIT &&
			charset != NULL) {
		if (iconvd != (iconv_t)-1)
			iconv_close(iconvd);
		if ((iconvd = iconv_open_ft(charset, tcs)) == (iconv_t)-1
				&& errno != 0) {
			if (errno == EINVAL)
				fprintf(stderr, catgets(catd, CATSET, 179,
			"Cannot convert from %s to %s\n"), tcs, charset);
			else
				perror("iconv_open");
			Fclose(nfo);
			return NULL;
		}
	}
#endif
	if (hp->h_attach != NULL) {
		if (make_multipart(hp, convert, fi, nfo,
					contenttype, charset, dosign) != 0) {
			Fclose(nfo);
			Fclose(nfi);
#ifdef	HAVE_ICONV
			if (iconvd != (iconv_t)-1) {
				iconv_close(iconvd);
				iconvd = (iconv_t)-1;
			}
#endif
			return NULL;
		}
	} else {
		size_t sz, bufsize, count;
		char *buf;

		if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
				|| iconvd != (iconv_t)-1
#endif	/* HAVE_ICONV */
				) {
			fflush(fi);
			count = fsize(fi);
		}
		buf = smalloc(bufsize = INFIX_BUF);
		for (;;) {
			if (convert == CONV_TOQP
#ifdef	HAVE_ICONV
					|| iconvd != (iconv_t)-1
#endif	/* HAVE_ICONV */
					) {
				if (fgetline(&buf, &bufsize, &count, &sz, fi, 0)
						== NULL)
					break;
			} else {
				sz = fread(buf, sizeof *buf, bufsize, fi);
				if (sz == 0)
					break;
			}
			lastc = buf[sz - 1];
			if (mime_write(buf, sz, nfo, convert,
					TD_ICONV, NULL, (size_t)0,
					NULL, NULL) == 0) {
				Fclose(nfo);
				Fclose(nfi);
#ifdef	HAVE_ICONV
				if (iconvd != (iconv_t)-1) {
					iconv_close(iconvd);
					iconvd = (iconv_t)-1;
				}
#endif
				free(buf);
				return NULL;
			}
		}
		if (convert == CONV_TOQP && lastc != '\n')
			fwrite("=\n", 1, 2, nfo);
		free(buf);
		if (ferror(fi)) {
			Fclose(nfo);
			Fclose(nfi);
#ifdef	HAVE_ICONV
			if (iconvd != (iconv_t)-1) {
				iconv_close(iconvd);
				iconvd = (iconv_t)-1;
			}
#endif
			return NULL;
		}
		if (charset != NULL)
			put_signature(nfo, convert);
	}
#ifdef	HAVE_ICONV
	if (iconvd != (iconv_t)-1) {
		iconv_close(iconvd);
		iconvd = (iconv_t)-1;
	}
#endif
	fflush(nfo);
	if (ferror(nfo)) {
		perror(catgets(catd, CATSET, 180, "temporary mail file"));
		Fclose(nfo);
		Fclose(nfi);
		return NULL;
	}
	Fclose(nfo);
	Fclose(fi);
	fflush(nfi);
	rewind(nfi);
	return(nfi);
}

/*
 * Save the outgoing mail on the passed file.
 */

/*ARGSUSED*/
static int
savemail(char *name, FILE *fi)
{
	FILE *fo;
	char *buf;
	size_t bufsize, buflen, count;
	char *p;
	time_t now;
	int prependnl = 0;
	int error = 0;

	buf = smalloc(bufsize = LINESIZE);
	time(&now);
	if ((fo = Zopen(name, "a+", NULL)) == NULL) {
		if ((fo = Zopen(name, "wx", NULL)) == NULL) {
			perror(name);
			free(buf);
			return (-1);
		}
	} else {
		if (fseek(fo, -2L, SEEK_END) == 0) {
			switch (fread(buf, sizeof *buf, 2, fo)) {
			case 2:
				if (buf[1] != '\n') {
					prependnl = 1;
					break;
				}
				/*FALLTHRU*/
			case 1:
				if (buf[0] != '\n')
					prependnl = 1;
				break;
			default:
				if (ferror(fo)) {
					perror(name);
					free(buf);
					return -1;
				}
			}
			fflush(fo);
			if (prependnl) {
				putc('\n', fo);
				fflush(fo);
			}
		}
	}
	fprintf(fo, "From %s %s", myname, ctime(&now));
	buflen = 0;
	fflush(fi);
	rewind(fi);
	count = fsize(fi);
	while (fgetline(&buf, &bufsize, &count, &buflen, fi, 0) != NULL) {
		if (*buf == '>') {
			p = buf + 1;
			while (*p == '>')
				p++;
			if (strncmp(p, "From ", 5) == 0)
				/* we got a masked From line */
				putc('>', fo);
		} else if (strncmp(buf, "From ", 5) == 0)
			putc('>', fo);
		fwrite(buf, sizeof *buf, buflen, fo);
	}
	if (buflen && *(buf + buflen - 1) != '\n')
		putc('\n', fo);
	putc('\n', fo);
	fflush(fo);
	if (ferror(fo)) {
		perror(name);
		error = -1;
	}
	if (Fclose(fo) != 0)
		error = -1;
	fflush(fi);
	rewind(fi);
	/*
	 * OpenBSD 3.2 and NetBSD 1.5.2 were reported not to 
	 * reset the kernel file offset after the calls above,
	 * a clear violation of IEEE Std 1003.1, 1996, 8.2.3.7.
	 * So do it 'manually'.
	 */
	lseek(fileno(fi), 0, SEEK_SET);
	free(buf);
	return error;
}

/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */
int 
mail(struct name *to, struct name *cc, struct name *bcc,
		struct name *smopts, char *subject, struct attachment *attach,
		char *quotefile, int recipient_record, int tflag, int Eflag)
{
	struct header head;
	struct str in, out;

	memset(&head, 0, sizeof head);
	/* The given subject may be in RFC1522 format. */
	if (subject != NULL) {
		in.s = subject;
		in.l = strlen(subject);
		mime_fromhdr(&in, &out, TD_ISPR | TD_ICONV);
		head.h_subject = out.s;
	}
	if (tflag == 0) {
		head.h_to = to;
		head.h_cc = cc;
		head.h_bcc = bcc;
	}
	head.h_attach = attach;
	head.h_smopts = smopts;
	mail1(&head, 0, NULL, quotefile, recipient_record, 0, tflag, Eflag);
	if (subject != NULL)
		free(out.s);
	return(0);
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail routine below.
 */
static int 
sendmail_internal(void *v, int recipient_record)
{
	int Eflag;
	char *str = v;
	struct header head;

	memset(&head, 0, sizeof head);
	head.h_to = extract(str, GTO|GFULL);
	Eflag = value("skipemptybody") != NULL;
	mail1(&head, 0, NULL, NULL, recipient_record, 0, 0, Eflag);
	return(0);
}

int 
sendmail(void *v)
{
	return sendmail_internal(v, 0);
}

int 
Sendmail(void *v)
{
	return sendmail_internal(v, 1);
}

static enum okay
transfer(struct name *to, struct name *mailargs, FILE *input, struct header *hp)
{
	char	o[LINESIZE], *cp;
	struct name	*np, *nt;
	int	cnt = 0;
	FILE	*ef;
	enum okay	ok = OKAY;

	np = to;
	while (np) {
		snprintf(o, sizeof o, "smime-encrypt-%s", np->n_name);
		if ((cp = value(o)) != NULL) {
			if ((ef = smime_encrypt(input, cp, np->n_name)) != 0) {
				nt = nalloc(np->n_name,
					np->n_type & ~(GFULL|GSKIN));
				if (start_mta(nt, mailargs, ef, hp) != OKAY)
					ok = STOP;
				Fclose(ef);
			} else {
				fprintf(stderr, "Message not sent to <%s>\n",
						np->n_name);
				senderr++;
			}
			rewind(input);
			if (np->n_flink)
				np->n_flink->n_blink = np->n_blink;
			if (np->n_blink)
				np->n_blink->n_flink = np->n_flink;
			if (np == to)
				to = np->n_flink;
			np = np->n_flink;
		} else {
			cnt++;
			np = np->n_flink;
		}
	}
	if (cnt) {
		if (value("smime-force-encryption") ||
				start_mta(to, mailargs, input, hp) != OKAY)
			ok = STOP;
	}
	return ok;
}

/*
 * Start the Mail Transfer Agent
 * mailing to namelist and stdin redirected to input.
 */
static enum okay
start_mta(struct name *to, struct name *mailargs, FILE *input,
		struct header *hp)
{
	char **args = NULL, **t;
	pid_t pid;
	sigset_t nset;
	char *cp, *smtp;
	char	*user = NULL, *password = NULL, *skinned = NULL;
	enum okay	ok = STOP;
#ifdef	HAVE_SOCKETS
	struct termios	otio;
	int	reset_tio;
#endif	/* HAVE_SOCKETS */

	if ((smtp = value("smtp")) == NULL) {
		args = unpack(cat(mailargs, to));
		if (debug || value("debug")) {
			printf(catgets(catd, CATSET, 181,
					"Sendmail arguments:"));
			for (t = args; *t != NULL; t++)
				printf(" \"%s\"", *t);
			printf("\n");
			return OKAY;
		}
	}
#ifdef	HAVE_SOCKETS
	if (smtp != NULL) {
		skinned = skin(myorigin(hp));
		if ((user = smtp_auth_var("-user", skinned)) != NULL &&
				(password = smtp_auth_var("-password",
					skinned)) == NULL)
			password = getpassword(&otio, &reset_tio, NULL);
	}
#endif	/* HAVE_SOCKETS */
	/*
	 * Fork, set up the temporary mail file as standard
	 * input for "mail", and exec with the user list we generated
	 * far above.
	 */
	if ((pid = fork()) == -1) {
		perror("fork");
		savedeadletter(input);
		senderr++;
		return STOP;
	}
	if (pid == 0) {
		sigemptyset(&nset);
		sigaddset(&nset, SIGHUP);
		sigaddset(&nset, SIGINT);
		sigaddset(&nset, SIGQUIT);
		sigaddset(&nset, SIGTSTP);
		sigaddset(&nset, SIGTTIN);
		sigaddset(&nset, SIGTTOU);
		freopen("/dev/null", "r", stdin);
		if (smtp != NULL) {
			prepare_child(&nset, 0, 1);
			if (smtp_mta(smtp, to, input, hp,
					user, password, skinned) == 0)
				_exit(0);
		} else {
			prepare_child(&nset, fileno(input), -1);
			if ((cp = value("sendmail")) != NULL)
				cp = expand(cp);
			else
				cp = SENDMAIL;
			execv(cp, args);
			perror(cp);
		}
		savedeadletter(input);
		fputs(catgets(catd, CATSET, 182,
				". . . message not sent.\n"), stderr);
		_exit(1);
	}
	if (value("verbose") != NULL || value("sendwait") || debug
			|| value("debug")) {
		if (wait_child(pid) == 0)
			ok = OKAY;
		else
			senderr++;
	} else {
		ok = OKAY;
		free_child(pid);
	}
	return ok;
}

/*
 * Record outgoing mail if instructed to do so.
 */
static enum okay
mightrecord(FILE *fp, struct name *to, int recipient_record)
{
	char	*cp, *cq, *ep;

	if (recipient_record) {
		cq = skin(to->n_name);
		cp = salloc(strlen(cq) + 1);
		strcpy(cp, cq);
		for (cq = cp; *cq && *cq != '@'; cq++);
		*cq = '\0';
	} else
		cp = value("record");
	if (cp != NULL) {
		ep = expand(cp);
		if (value("outfolder") && *ep != '/' && *ep != '+' &&
				which_protocol(ep) == PROTO_FILE) {
			cq = salloc(strlen(cp) + 2);
			cq[0] = '+';
			strcpy(&cq[1], cp);
			cp = cq;
			ep = expand(cp);
		}
		if (savemail(ep, fp) != 0) {
			fprintf(stderr,
				"Error while saving message to %s - "
				"message not sent\n", ep);
			rewind(fp);
			exit_status |= 1;
			savedeadletter(fp);
			return STOP;
		}
	}
	return OKAY;
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */
enum okay 
mail1(struct header *hp, int printheaders, struct message *quote,
		char *quotefile, int recipient_record, int doprefix, int tflag,
		int Eflag)
{
	struct name *to;
	FILE *mtf, *nmtf;
	enum okay	ok = STOP;
	int	dosign = -1;
	char	*charsets, *ncs = NULL, *cp;

#ifdef	notdef
	if ((hp->h_to = checkaddrs(hp->h_to)) == NULL) {
		senderr++;
		return STOP;
	}
#endif
	if ((cp = value("autocc")) != NULL && *cp)
		hp->h_cc = cat(hp->h_cc, checkaddrs(sextract(cp, GCC|GFULL)));
	if ((cp = value("autobcc")) != NULL && *cp)
		hp->h_bcc = cat(hp->h_bcc,
				checkaddrs(sextract(cp, GBCC|GFULL)));
	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */
	if ((mtf = collect(hp, printheaders, quote, quotefile, doprefix,
					tflag)) == NULL)
		return STOP;
	if (value("interactive") != NULL) {
		if (((value("bsdcompat") || value("askatend"))
					&& (value("askcc") != NULL ||
					value("askbcc") != NULL)) ||
				value("askattach") != NULL ||
				value("asksign") != NULL) {
			if (value("askcc") != NULL)
				grabh(hp, GCC, 1);
			if (value("askbcc") != NULL)
				grabh(hp, GBCC, 1);
			if (value("askattach") != NULL)
				hp->h_attach = edit_attachments(hp->h_attach);
			if (value("asksign") != NULL)
				dosign = yorn("Sign this message (y/n)? ");
		} else {
			printf(catgets(catd, CATSET, 183, "EOT\n"));
			fflush(stdout);
		}
	}
	if (fsize(mtf) == 0) {
		if (Eflag)
			goto out;
		if (hp->h_subject == NULL)
			printf(catgets(catd, CATSET, 184,
				"No message, no subject; hope that's ok\n"));
		else if (value("bsdcompat") || value("bsdmsgs"))
			printf(catgets(catd, CATSET, 185,
				"Null message body; hope that's ok\n"));
	}
	if (dosign < 0) {
		if (value("smime-sign") != NULL)
			dosign = 1;
		else
			dosign = 0;
	}
	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */
	senderr = 0;
	if ((to = usermap(cat(hp->h_bcc, cat(hp->h_to, hp->h_cc)))) == NULL) {
		printf(catgets(catd, CATSET, 186, "No recipients specified\n"));
		senderr++;
	}
	to = fixhead(hp, to);
	if (hp->h_charset) {
		wantcharset = hp->h_charset;
		goto try;
	}
hloop:	wantcharset = NULL;
	if ((charsets = value("sendcharsets")) != NULL) {
		wantcharset = savestr(charsets);
	loop:	if ((ncs = strchr(wantcharset, ',')) != NULL)
			*ncs++ = '\0';
	}
try:	if ((nmtf = infix(hp, mtf, dosign)) == NULL) {
		if (hp->h_charset && (errno == EILSEQ || errno == EINVAL)) {
			rewind(mtf);
			hp->h_charset = NULL;
			goto hloop;
		}
		if (errno == EILSEQ || errno == EINVAL) {
			if (ncs && *ncs) {
				rewind(mtf);
				wantcharset = ncs;
				goto loop;
			}
			if (wantcharset && value("interactive") == NULL) {
				if (wantcharset == (char *)-1)
					wantcharset = NULL;
				else {
					rewind(mtf);
					wantcharset = (char *)-1;
					goto try;
				}
			}
		}
		/* fprintf(stderr, ". . . message lost, sorry.\n"); */
		perror("");
	fail:	senderr++;
		rewind(mtf);
		savedeadletter(mtf);
		fputs(catgets(catd, CATSET, 187,
				". . . message not sent.\n"), stderr);
		return STOP;
	}
	mtf = nmtf;
	if (dosign) {
		if ((nmtf = smime_sign(mtf, hp)) == NULL)
			goto fail;
		Fclose(mtf);
		mtf = nmtf;
	}
	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */
	to = outof(to, mtf, hp);
	if (senderr)
		savedeadletter(mtf);
	to = elide(to);
	if (count(to) == 0) {
		if (senderr == 0)
			ok = OKAY;
		goto out;
	}
	if (mightrecord(mtf, to, recipient_record) != OKAY)
		goto out;
	ok = transfer(to, hp->h_smopts, mtf, hp);
out:
	Fclose(mtf);
	return ok;
}

/*
 * Create a Message-Id: header field.
 * Use either the host name or the from address.
 */
static void
message_id(FILE *fo, struct header *hp)
{
	char	*cp;
	time_t	now;

	time(&now);
	if ((cp = value("hostname")) != NULL)
		fprintf(fo, "Message-ID: <%lx.%s@%s>\n",
				(long)now, getrandstring(24), cp);
	else if ((cp = skin(myorigin(hp))) != NULL && strchr(cp, '@') != NULL)
		fprintf(fo, "Message-ID: <%lx.%s%%%s>\n",
				(long)now, getrandstring(16), cp);
}

static const char *weekday_names[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *month_names[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL
};

/*
 * Create a Date: header field.
 * We compare the localtime() and gmtime() results to get the timezone,
 * because numeric timezones are easier to read and because $TZ is
 * not set on most GNU systems.
 */
int
mkdate(FILE *fo, const char *field)
{
	time_t t;
	struct tm *tmptr;
	int tzdiff, tzdiff_hour, tzdiff_min;

	time(&t);
	tzdiff = t - mktime(gmtime(&t));
	tzdiff_hour = (int)(tzdiff / 60);
	tzdiff_min = tzdiff_hour % 60;
	tzdiff_hour /= 60;
	tmptr = localtime(&t);
	if (tmptr->tm_isdst > 0)
		tzdiff_hour++;
	return fprintf(fo, "%s: %s, %02d %s %04d %02d:%02d:%02d %+05d\n",
			field,
			weekday_names[tmptr->tm_wday],
			tmptr->tm_mday, month_names[tmptr->tm_mon],
			tmptr->tm_year + 1900, tmptr->tm_hour,
			tmptr->tm_min, tmptr->tm_sec,
			tzdiff_hour * 100 + tzdiff_min);
}

static enum okay
putname(char *line, enum gfield w, enum sendaction action, int *gotcha,
		char *prefix, FILE *fo, struct name **xp)
{
	struct name	*np;

	np = sextract(line, GEXTRA|GFULL);
	if (xp)
		*xp = np;
	if (np == NULL)
		return 0;
	if (fmt(prefix, np, fo, w&(GCOMMA|GFILES), 0, action != SEND_TODISP))
		return 1;
	if (gotcha)
		(*gotcha)++;
	return 0;
}

#define	FMT_CC_AND_BCC	{ \
				if (hp->h_cc != NULL && w & GCC) { \
					if (fmt("Cc:", hp->h_cc, fo, \
							w&(GCOMMA|GFILES), 0, \
							action!=SEND_TODISP)) \
						return 1; \
					gotcha++; \
				} \
				if (hp->h_bcc != NULL && w & GBCC) { \
					if (fmt("Bcc:", hp->h_bcc, fo, \
							w&(GCOMMA|GFILES), 0, \
							action!=SEND_TODISP)) \
						return 1; \
					gotcha++; \
				} \
			}
/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */
int
puthead(struct header *hp, FILE *fo, enum gfield w,
		enum sendaction action, enum conversion convert,
		char *contenttype, char *charset)
{
	int gotcha;
	char *addr/*, *cp*/;
	int stealthmua;
	struct name *np, *fromfield = NULL, *senderfield = NULL;


	if (value("stealthmua"))
		stealthmua = 1;
	else
		stealthmua = 0;
	gotcha = 0;
	if (w & GDATE) {
		mkdate(fo, "Date"), gotcha++;
	}
	if (w & GIDENT) {
		if (hp->h_from != NULL) {
			if (fmt("From:", hp->h_from, fo, w&(GCOMMA|GFILES), 0,
						action!=SEND_TODISP))
				return 1;
			gotcha++;
			fromfield = hp->h_from;
		} else if ((addr = myaddrs(hp)) != NULL)
			if (putname(addr, w, action, &gotcha, "From:", fo,
						&fromfield))
				return 1;
		if (((addr = hp->h_organization) != NULL ||
				(addr = value("ORGANIZATION")) != NULL)
				&& strlen(addr) > 0) {
			fwrite("Organization: ", sizeof (char), 14, fo);
			if (mime_write(addr, strlen(addr), fo,
					action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
			gotcha++;
			putc('\n', fo);
		}
		if (hp->h_replyto != NULL) {
			if (fmt("Reply-To:", hp->h_replyto, fo,
					w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
				return 1;
			gotcha++;
		} else if ((addr = value("replyto")) != NULL)
			if (putname(addr, w, action, &gotcha, "Reply-To:", fo,
						NULL))
				return 1;
		if (hp->h_sender != NULL) {
			if (fmt("Sender:", hp->h_sender, fo,
					w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
				return 1;
			gotcha++;
			senderfield = hp->h_sender;
		} else if ((addr = value("sender")) != NULL)
			if (putname(addr, w, action, &gotcha, "Sender:", fo,
						&senderfield))
				return 1;
		if (check_from_and_sender(fromfield, senderfield))
			return 1;
	}
	if (hp->h_to != NULL && w & GTO) {
		if (fmt("To:", hp->h_to, fo, w&(GCOMMA|GFILES), 0,
					action!=SEND_TODISP))
			return 1;
		gotcha++;
	}
	if (value("bsdcompat") == NULL && value("bsdorder") == NULL)
		FMT_CC_AND_BCC
	if (hp->h_subject != NULL && w & GSUBJECT) {
		fwrite("Subject: ", sizeof (char), 9, fo);
		if (ascncasecmp(hp->h_subject, "re: ", 4) == 0) {
			fwrite("Re: ", sizeof (char), 4, fo);
			if (strlen(hp->h_subject + 4) > 0 &&
				mime_write(hp->h_subject + 4,
					strlen(hp->h_subject + 4),
					fo, action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
		} else if (*hp->h_subject) {
			if (mime_write(hp->h_subject,
					strlen(hp->h_subject),
					fo, action == SEND_TODISP ?
					CONV_NONE:CONV_TOHDR,
					action == SEND_TODISP ?
					TD_ISPR|TD_ICONV:TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL) == 0)
				return 1;
		}
		gotcha++;
		fwrite("\n", sizeof (char), 1, fo);
	}
	if (value("bsdcompat") || value("bsdorder"))
		FMT_CC_AND_BCC
	if (w & GMSGID && stealthmua == 0)
		message_id(fo, hp), gotcha++;
	if (hp->h_ref != NULL && w & GREF) {
		fmt("References:", hp->h_ref, fo, 0, 1, 0);
		if ((np = hp->h_ref) != NULL && np->n_name) {
			while (np->n_flink)
				np = np->n_flink;
			if (mime_name_invalid(np->n_name, 0) == 0) {
				fprintf(fo, "In-Reply-To: %s\n", np->n_name);
				gotcha++;
			}
		}
	}
	if (w & GUA && stealthmua == 0)
		fprintf(fo, "User-Agent: Heirloom mailx %s\n",
				version), gotcha++;
	if (w & GMIME) {
		fputs("MIME-Version: 1.0\n", fo), gotcha++;
		if (hp->h_attach != NULL) {
			makeboundary();
			fprintf(fo, "Content-Type: multipart/mixed;\n"
				" boundary=\"%s\"\n", send_boundary);
		} else {
			fprintf(fo, "Content-Type: %s", contenttype);
			if (charset)
				fprintf(fo, "; charset=%s", charset);
			fprintf(fo, "\nContent-Transfer-Encoding: %s\n",
					getencoding(convert));
		}
	}
	if (gotcha && w & GNL)
		putc('\n', fo);
	return(0);
}

/*
 * Format the given header line to not exceed 72 characters.
 */
static int
fmt(char *str, struct name *np, FILE *fo, int flags, int dropinvalid,
		int domime)
{
	int col, len, count = 0;
	int is_to = 0, comma;

	comma = flags&GCOMMA ? 1 : 0;
	col = strlen(str);
	if (col) {
		fwrite(str, sizeof *str, strlen(str), fo);
		if ((flags&GFILES) == 0 &&
				col == 3 && asccasecmp(str, "to:") == 0 ||
				col == 3 && asccasecmp(str, "cc:") == 0 ||
				col == 4 && asccasecmp(str, "bcc:") == 0 ||
				col == 10 && asccasecmp(str, "Resent-To:") == 0)
			is_to = 1;
	}
	for (; np != NULL; np = np->n_flink) {
		if (is_to && is_fileaddr(np->n_name))
			continue;
		if (np->n_flink == NULL)
			comma = 0;
		if (mime_name_invalid(np->n_name, !dropinvalid)) {
			if (dropinvalid)
				continue;
			else
				return 1;
		}
		len = strlen(np->n_fullname);
		col++;		/* for the space */
		if (count && col + len + comma > 72 && col > 1) {
			fputs("\n ", fo);
			col = 1;
		} else
			putc(' ', fo);
		len = mime_write(np->n_fullname,
				len, fo,
				domime?CONV_TOHDR_A:CONV_NONE,
				TD_ICONV, NULL, (size_t)0,
				NULL, NULL);
		if (comma && !(is_to && is_fileaddr(np->n_flink->n_name)))
			putc(',', fo);
		col += len + comma;
		count++;
	}
	putc('\n', fo);
	return 0;
}

/*
 * Rewrite a message for resending, adding the Resent-Headers.
 */
static int
infix_resend(FILE *fi, FILE *fo, struct message *mp, struct name *to,
		int add_resent)
{
	size_t count;
	char *buf = NULL, *cp/*, *cp2*/;
	size_t c, bufsize = 0;
	struct name	*fromfield = NULL, *senderfield = NULL;

	count = mp->m_size;
	/*
	 * Write the Resent-Fields.
	 */
	if (add_resent) {
		fputs("Resent-", fo);
		mkdate(fo, "Date");
		if ((cp = myaddrs(NULL)) != NULL) {
			if (putname(cp, GCOMMA, SEND_MBOX, NULL,
					"Resent-From:", fo, &fromfield))
				return 1;
		}
		if ((cp = value("sender")) != NULL) {
			if (putname(cp, GCOMMA, SEND_MBOX, NULL,
					"Resent-Sender:", fo, &senderfield))
				return 1;
		}
#ifdef	notdef
		/*
		 * RFC 2822 disallows generation of this field.
		 */
		cp = value("replyto");
		if (cp != NULL) {
			if (mime_name_invalid(cp, 1)) {
				if (buf)
					free(buf);
				return 1;
			}
			fwrite("Resent-Reply-To: ", sizeof (char),
					17, fo);
			mime_write(cp, strlen(cp), fo,
					CONV_TOHDR_A, TD_ICONV,
					NULL, (size_t)0,
					NULL, NULL);
			putc('\n', fo);
		}
#endif	/* notdef */
		if (fmt("Resent-To:", to, fo, 1, 1, 0)) {
			if (buf)
				free(buf);
			return 1;
		}
		if (value("stealthmua") == NULL) {
			fputs("Resent-", fo);
			message_id(fo, NULL);
		}
	}
	if (check_from_and_sender(fromfield, senderfield))
		return 1;
	/*
	 * Write the original headers.
	 */
	while (count > 0) {
		if ((cp = foldergets(&buf, &bufsize, &count, &c, fi)) == NULL)
			break;
		if (ascncasecmp("status: ", buf, 8) != 0
				&& strncmp("From ", buf, 5) != 0) {
			fwrite(buf, sizeof *buf, c, fo);
		}
		if (count > 0 && *buf == '\n')
			break;
	}
	/*
	 * Write the message body.
	 */
	while (count > 0) {
		if (foldergets(&buf, &bufsize, &count, &c, fi) == NULL)
			break;
		if (count == 0 && *buf == '\n')
			break;
		fwrite(buf, sizeof *buf, c, fo);
	}
	if (buf)
		free(buf);
	if (ferror(fo)) {
		perror(catgets(catd, CATSET, 188, "temporary mail file"));
		return 1;
	}
	return 0;
}

enum okay 
resend_msg(struct message *mp, struct name *to, int add_resent)
{
	FILE *ibuf, *nfo, *nfi;
	char *tempMail;
	struct header head;
	enum okay	ok = STOP;

	memset(&head, 0, sizeof head);
	if ((to = checkaddrs(to)) == NULL) {
		senderr++;
		return STOP;
	}
	if ((nfo = Ftemp(&tempMail, "Rs", "w", 0600, 1)) == NULL) {
		senderr++;
		perror(catgets(catd, CATSET, 189, "temporary mail file"));
		return STOP;
	}
	if ((nfi = Fopen(tempMail, "r")) == NULL) {
		senderr++;
		perror(tempMail);
		return STOP;
	}
	rm(tempMail);
	Ftfree(&tempMail);
	if ((ibuf = setinput(&mb, mp, NEED_BODY)) == NULL)
		return STOP;
	head.h_to = to;
	to = fixhead(&head, to);
	if (infix_resend(ibuf, nfo, mp, head.h_to, add_resent) != 0) {
		senderr++;
		rewind(nfo);
		savedeadletter(nfi);
		fputs(catgets(catd, CATSET, 190,
				". . . message not sent.\n"), stderr);
		Fclose(nfo);
		Fclose(nfi);
		return STOP;
	}
	fflush(nfo);
	rewind(nfo);
	Fclose(nfo);
	to = outof(to, nfi, &head);
	if (senderr)
		savedeadletter(nfi);
	to = elide(to);
	if (count(to) != 0) {
		if (value("record-resent") == NULL ||
				mightrecord(nfi, to, 0) == OKAY)
			ok = transfer(to, head.h_smopts, nfi, NULL);
	} else if (senderr == 0)
		ok = OKAY;
	Fclose(nfi);
	return ok;
}

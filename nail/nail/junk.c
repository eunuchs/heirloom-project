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

#ifndef lint
#ifdef	DOSCCS
static char sccsid[] = "@(#)junk.c	1.75 (gritter) 9/14/08";
#endif
#endif /* not lint */

#include "config.h"
#include "rcv.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#ifdef	HAVE_MMAP
#include <sys/mman.h>
#else	/* !HAVE_MMAP */
#define	mmap(a, b, c, d, e, f)	MAP_FAILED
#define	munmap(a, b)
#endif	/* !HAVE_MMAP */
#ifndef	HAVE_MREMAP
#define	mremap(a, b, c, d)	MAP_FAILED
#endif	/* !HAVE_MREMAP */

#ifndef	MAP_FAILED
#define	MAP_FAILED	((void *)-1)
#endif	/* !MAP_FAILED */

#include "extern.h"
#include "md5.h"

/*
 * Mail -- a mail program
 *
 * Junk classification, mostly according to Paul Graham's "A Plan for Spam",
 * August 2002, <http://www.paulgraham.com/spam.html>, and his "Better
 * Bayesian Filtering", January 2003, <http://www.paulgraham.com/better.html>.
 *
 * Chained tokens according to Jonathan A. Zdziarski's "Advanced Language
 * Classification using Chained Tokens", February 2004,
 * <http://www.nuclearelephant.com/papers/chained.html>.
 */

#define	DFL	.40
#define	THR	.9
#define	MID	.5

#define	MAX2	0x0000ffff
#define	MAX3	0x00ffffffUL
#define	MAX4	0xffffffffUL

/*
 * The dictionary consists of two files forming a hash table. The hash
 * consists of the first 56 bits of the result of applying MD5 to the
 * input word. This scheme ensures that collisions are unlikely enough
 * to make junk detection work; according to the birthday paradox, a
 * 50 % probability for one single collision is reached at 2^28 entries.
 *
 * To make the chain structure independent from input, the MD5 input is
 * xor'ed with a random number. This makes it impossible that someone uses
 * a carefully crafted message for a denial-of-service attack against the
 * database.
 */
#define	SIZEOF_node	17
#define	OF_node_hash	0	/* first 32 bits of MD5 of word|mangle */
#define	OF_node_next	4	/* bit-negated table index of next node */
#define	OF_node_good	8	/* number of times this appeared in good msgs */
#define	OF_node_bad	11	/* number of times this appeared in bad msgs */
#define	OF_node_prob_O	14	/* table_version<1: precomputed probability */
#define	OF_node_hash2	14	/* upper 3 bytes of MD5 hash */
static char	*nodes;

#define	SIZEOF_super	262164
#define	OF_super_size	0	/* allocated nodes in the chain file */
#define	OF_super_used	4	/* used nodes in the chain file */
#define	OF_super_ngood	8	/* number of good messages scanned so far */
#define	OF_super_nbad	12	/* number of bad messages scanned so far */
#define	OF_super_mangle	16	/* used to mangle the MD5 input */
#define	OF_super_bucket	20	/* 65536 bit-negated node indices */
#define	SIZEOF_entry	4
static char	*super;

static size_t	super_mmapped;
static size_t	nodes_mmapped;
static int	rw_map;
static int	chained_tokens;

/*
 * Version history
 * ---------------
 * 0	Initial version
 * 1	Fixed the mangling; it was ineffective in version 0.
 *      Hash extended to 56 bits.
 */
static int	table_version;
#define	current_table_version	1

#define	get(e)	\
	((unsigned)(((char *)(e))[0]&0377) + \
	 ((unsigned)(((char *)(e))[1]&0377) << 8) + \
	 ((unsigned)(((char *)(e))[2]&0377) << 16))

#define	put(e, n) \
	(((char *)(e))[0] = (n) & 0x0000ff, \
	 ((char *)(e))[1] = ((n) & 0x00ff00) >> 8, \
	 ((char *)(e))[2] = ((n) & 0xff0000) >> 16)

#define	f2s(d)	(smin(((unsigned)((d) * MAX3)), MAX3))

#define	s2f(s)	((float)(s) / MAX3)

#define	getn(p)	\
	((unsigned long)(((char *)(p))[0]&0377) + \
	 ((unsigned long)(((char *)(p))[1]&0377) << 8) + \
	 ((unsigned long)(((char *)(p))[2]&0377) << 16) + \
	 ((unsigned long)(((char *)(p))[3]&0377) << 24))

#define	putn(p, n) \
	(((char *)(p))[0] = (n) & 0x000000ffUL, \
	 ((char *)(p))[1] = ((n) & 0x0000ff00UL) >> 8, \
	 ((char *)(p))[2] = ((n) & 0x00ff0000UL) >> 16, \
	 ((char *)(p))[3] = ((n) & 0xff000000UL) >> 24)

struct lexstat {
	char	*save;
	int	price;
	int	url;
	int	lastc;
	int	hadamp;
	enum loc {
		FROM_LINE	= 0,
		HEADER		= 1,
		BODY		= 2
	} loc;
	enum html {
		HTML_NONE	= 0,
		HTML_TEXT	= 1,
		HTML_TAG	= 2,
		HTML_SKIP	= 3
	} html;
	char	tag[8];
	char	*tagp;
	char	field[LINESIZE];
};

#define	constituent(c, b, i, price, hadamp) \
	((c) & 0200 || alnumchar(c) || (c) == '\'' || (c) == '"' || \
		(c) == '$' || (c) == '!' || (c) == '_' || \
		(c) == '#' || (c) == '%' || (c) == '&' || \
		((c) == ';' && hadamp) || \
		((c) == '-' && !(price)) || \
		(((c) == '.' || (c) == ',' || (c) == '/') && \
		 (i) > 0 && digitchar((b)[(i)-1]&0377)))

#define	url_xchar(c) \
	(((c)&0200) == 0 && ((c)&037) != (c) && (c) != 0177 && \
	 !spacechar(c) && (c) != '{' && (c) != '}' && (c) != '|' && \
	 (c) != '\\' && (c) != '^' && (c) != '~' && (c) != '[' && \
	 (c) != ']' && (c) != '`' && (c) != '<' && (c) != '>' && \
	 (c) != '#' && (c) != '"')

enum db {
	SUPER = 0,
	NODES = 1
};

enum entry {
	GOOD = 0,
	BAD  = 1
};

static const char	README1[] = "\
This is a junk mail database maintained by mailx(1). It does not contain any\n\
of the actual words found in your messages. Instead, parts of MD5 hashes are\n\
used for lookup. It is thus possible to tell if some given word was likely\n\
contained in your mail from examining this data, at best.\n";
static const char	README2[] = "\n\
The database files are stored in compress(1) format by default. This saves\n\
some space, but leads to higher processor usage when the database is read\n\
or updated. You can use uncompress(1) on these files if you prefer to store\n\
them in flat form.\n";

static int	verbose;
static int	_debug;
static FILE	*sfp, *nfp;
static char	*sname, *nname;

static enum okay getdb(int rw);
static void putdb(void);
static void relsedb(void);
static FILE *dbfp(enum db db, int rw, int *compressed, char **fn);
static char *lookup(unsigned long h1, unsigned long h2, int create);
static unsigned long grow(unsigned long size);
static char *nextword(char **buf, size_t *bufsize, size_t *count, FILE *fp,
		struct lexstat *sp, int *stop);
static void join(char **buf, size_t *bufsize, const char *s1, const char *s2);
static void add(const char *word, enum entry entry, struct lexstat *sp,
		int incr);
static enum okay scan(struct message *m, enum entry entry,
		void (*func)(const char *, enum entry, struct lexstat *, int),
		int arg);
static void recompute(void);
static float getprob(char *n);
static int insert(int *msgvec, enum entry entry, int incr);
static void clsf(struct message *m);
static void rate(const char *word, enum entry entry, struct lexstat *sp,
		int unused);
static void dbhash(const char *word, unsigned long *h1, unsigned long *h2);
static void mkmangle(void);

static enum okay 
getdb(int rw)
{
	void	*zp = NULL;
	long	n;
	int	compressed;

	chained_tokens = value("chained-junk-tokens") != NULL;
	if ((sfp = dbfp(SUPER, rw, &compressed, &sname)) == (FILE *)-1)
		return STOP;
	if (sfp && !compressed) {
		super = mmap(NULL, SIZEOF_super,
				rw!=O_RDONLY ? PROT_READ|PROT_WRITE : PROT_READ,
				MAP_SHARED, fileno(sfp), 0);
		if (super != MAP_FAILED) {
			super_mmapped = SIZEOF_super;
			goto skip;
		}
	}
	super_mmapped = 0;
	super = smalloc(SIZEOF_super);
	if (sfp) {
		if (compressed)
			zp = zalloc(sfp);
		if ((compressed ? zread(zp, super, SIZEOF_super)
					!= SIZEOF_super :
				fread(super, 1, SIZEOF_super, sfp)
					!= SIZEOF_super) ||
				ferror(sfp)) {
			fprintf(stderr, "Error reading junk mail database.\n");
			memset(super, 0, SIZEOF_super);
			mkmangle();
			if (compressed)
				zfree(zp);
			Fclose(sfp);
			sfp = NULL;
		} else if (compressed)
			zfree(zp);
	} else {
		memset(super, 0, SIZEOF_super);
		mkmangle();
	}
skip:	if ((n = getn(&super[OF_super_size])) == 0) {
		n = 1;
		putn(&super[OF_super_size], 1);
	}
	if (sfp && (nfp = dbfp(NODES, rw, &compressed, &nname)) != NULL) {
		if (nfp == (FILE *)-1) {
			relsedb();
			return STOP;
		}
	}
	rw_map = rw;
	if (sfp && nfp && !compressed) {
		nodes = mmap(NULL, n * SIZEOF_node,
				rw!=O_RDONLY ? PROT_READ|PROT_WRITE : PROT_READ,
				MAP_SHARED, fileno(nfp), 0);
		if (nodes != MAP_FAILED) {
			nodes_mmapped = n * SIZEOF_node;
			return OKAY;
		}
	}
	nodes_mmapped = 0;
	nodes = smalloc(n * SIZEOF_node);
	if (sfp && nfp) {
		if (compressed)
			zp = zalloc(nfp);
		if ((compressed ? zread(zp, nodes, n * SIZEOF_node)
				!= n * SIZEOF_node :
				fread(nodes, 1, n * SIZEOF_node, nfp)
				!= n * SIZEOF_node) ||
				ferror(nfp)) {
			fprintf(stderr, "Error reading junk mail database.\n");
			memset(nodes, 0, n * SIZEOF_node);
			memset(super, 0, SIZEOF_super);
			mkmangle();
			putn(&super[OF_super_size], n);
		}
		if (compressed)
			zfree(zp);
		Fclose(nfp);
		nfp = NULL;
	} else
		memset(nodes, 0, n * SIZEOF_node);
	if (sfp) {
		Fclose(sfp);
		sfp = NULL;
	}
	return OKAY;
}

static void 
putdb(void)
{
	void	*zp;
	int	scomp, ncomp;

	if (!super_mmapped && (sfp = dbfp(SUPER, O_WRONLY, &scomp, &sname))
			== NULL || sfp == (FILE *)-1)
		return;
	if (!nodes_mmapped && (nfp = dbfp(NODES, O_WRONLY, &ncomp, &nname))
			== NULL || nfp == (FILE *)-1)
		return;
	if (super_mmapped == 0 || nodes_mmapped == 0)
		holdint();
	/*
	 * Use utime() with mmap() since Linux does not update st_mtime
	 * reliably otherwise.
	 */
	if (super_mmapped)
		utime(sname, NULL);
	else if (scomp) {
		zp = zalloc(sfp);
		zwrite(zp, super, SIZEOF_super);
		zfree(zp);
		trunc(sfp);
	} else
		fwrite(super, 1, SIZEOF_super, sfp);
	if (nodes_mmapped)
		utime(nname, NULL);
	else if (ncomp) {
		zp = zalloc(nfp);
		zwrite(zp, nodes, getn(&super[OF_super_size]) * SIZEOF_node);
		zfree(zp);
		trunc(nfp);
	} else
		fwrite(nodes, 1,
			getn(&super[OF_super_size]) * SIZEOF_node, nfp);
	if (super_mmapped == 0 || nodes_mmapped == 0)
		relseint();
}

static void
relsedb(void)
{
	if (super_mmapped) {
		munmap(super, super_mmapped);
		super_mmapped = 0;
	} else
		free(super);
	if (nodes_mmapped) {
		munmap(nodes, nodes_mmapped);
		nodes_mmapped = 0;
	} else
		free(nodes);
	if (sfp && sfp != (FILE *)-1) {
		Fclose(sfp);
		sfp = NULL;
	}
	if (nfp && nfp != (FILE *)-1) {
		Fclose(nfp);
		nfp = NULL;
	}
}

static FILE *
dbfp(enum db db, int rw, int *compressed, char **fn)
{
	FILE	*fp, *rp;
	char	*dir;
	struct flock	flp;
	char *sfx[][2] = {
		{ "super",	"nodes" },
		{ "super1",	"nodes1" }
	};
	char **sf;
	char *zfx[][2] = {
		{ "super.Z",	"nodes.Z" },
		{ "super1.Z",	"nodes1.Z" }
	};
	char **zf;
	int	n;

	if ((dir = value("junkdb")) == NULL) {
		fprintf(stderr, "No junk mail database specified. "
				"Set the junkdb variable.\n");
		return (FILE *)-1;
	}
	dir = expand(dir);
	if (makedir(dir) == STOP) {
		fprintf(stderr, "Cannot create directory \"%s\"\n.", dir);
		return (FILE *)-1;
	}
	if (rw!=O_WRONLY)
		table_version = current_table_version;
loop:	sf = sfx[table_version];
	zf = zfx[table_version];
	*fn = salloc((n = strlen(dir)) + 40);
	strcpy(*fn, dir);
	(*fn)[n] = '/';
	*compressed = 0;
	strcpy(&(*fn)[n+1], sf[db]);
	if ((fp = Fopen(*fn, rw!=O_RDONLY ? "r+" : "r")) != NULL)
		goto okay;
	*compressed = 1;
	strcpy(&(*fn)[n+1], zf[db]);
	if ((fp = Fopen(*fn, rw ? "r+" : "r")) == NULL &&
			rw==O_WRONLY ? (fp = Fopen(*fn, "w+")) == NULL : 0) {
		fprintf(stderr, "Cannot open junk mail database \"%s\".\n",*fn);
		return NULL;
	}
	if (rw==O_WRONLY) {
		strcpy(&(*fn)[n+1], "README");
		if (access(*fn, F_OK) < 0 && (rp = Fopen(*fn, "w")) != NULL) {
			fputs(README1, rp);
			fputs(README2, rp);
			Fclose(rp);
		}
	} else if (fp == NULL) {
		if (table_version > 0) {
			table_version--;
			goto loop;
		} else
			table_version = current_table_version;
	}
okay:	if (fp) {
		flp.l_type = rw!=O_RDONLY ? F_WRLCK : F_RDLCK;
		flp.l_start = 0;
		flp.l_len = 0;
		flp.l_whence = SEEK_SET;
		fcntl(fileno(fp), F_SETLKW, &flp);
	}
	return fp;
}

static char *
lookup(unsigned long h1, unsigned long h2, int create)
{
	char	*n, *lastn = NULL;
	unsigned long	c, lastc = MAX4, used, size;

	used = getn(&super[OF_super_used]);
	size = getn(&super[OF_super_size]);
	c = ~getn(&super[OF_super_bucket + (h1&MAX2)*SIZEOF_entry]);
	n = &nodes[c*SIZEOF_node];
	while (c < used) {
		if (getn(&n[OF_node_hash]) == h1 &&
				(table_version < 1 ? 1 :
				get(&n[OF_node_hash2]) == h2))
			return n;
		lastc = c;
		lastn = n;
		c = ~getn(&n[OF_node_next]);
		n = &nodes[c*SIZEOF_node];
	}
	if (create) {
		if (used >= size) {
			if ((size = grow(size)) == 0)
				return NULL;
			lastn = &nodes[lastc*SIZEOF_node];
		}
		putn(&super[OF_super_used], used+1);
		n = &nodes[used*SIZEOF_node];
		putn(&n[OF_node_hash], h1);
		put(&n[OF_node_hash2], h2);
		if (lastc < used)
			putn(&lastn[OF_node_next], ~used);
		else
			putn(&super[OF_super_bucket + (h1&MAX2)*SIZEOF_entry],
					~used);
		return n;
	} else
		return NULL;
}

static unsigned long
grow(unsigned long size)
{
	unsigned long	incr, newsize;
	void	*onodes;

	incr = size > MAX2 ? MAX2 : size;
	newsize = size + incr;
	if (newsize > MAX4-MAX2) {
	oflo:	fprintf(stderr, "Junk mail database overflow.\n");
		return 0;
	}
	if (nodes_mmapped) {
		if (lseek(fileno(nfp), newsize*SIZEOF_node-1, SEEK_SET)
				== (off_t)-1 || write(fileno(nfp),"\0",1) != 1)
			goto oflo;
		onodes = nodes;
		if ((nodes = mremap(nodes, nodes_mmapped, newsize*SIZEOF_node,
				MREMAP_MAYMOVE)) == MAP_FAILED) {
			if ((nodes = mmap(NULL, newsize*SIZEOF_node,
						rw_map!=O_RDONLY ?
							PROT_READ|PROT_WRITE :
							PROT_READ,
						MAP_SHARED, fileno(nfp), 0))
					== MAP_FAILED) {
				nodes = onodes;
				goto oflo;
			}
			munmap(onodes, nodes_mmapped);
		}
		nodes_mmapped = newsize*SIZEOF_node;
	} else {
		nodes = srealloc(nodes, newsize*SIZEOF_node);
		memset(&nodes[size*SIZEOF_node], 0, incr*SIZEOF_node);
	}
	size = newsize;
	putn(&super[OF_super_size], size);
	return size;
}

#define	SAVE(c)	{ \
	if (i+j >= (long)*bufsize-4) \
		*buf = srealloc(*buf, *bufsize += 32); \
	(*buf)[j+i] = (c); \
	i += (*buf)[j+i] != '\0'; \
}

static char *
nextword(char **buf, size_t *bufsize, size_t *count, FILE *fp,
		struct lexstat *sp, int *stop)
{
	int	c, i, j, k;
	char	*cp, *cq;

loop:	*stop = 0;
	sp->hadamp = 0;
	if (sp->save) {
		i = j = 0;
		for (cp = sp->save; *cp; cp++) {
			SAVE(*cp&0377)
		}
		SAVE('\0')
		free(sp->save);
		sp->save = NULL;
		goto out;
	}
	if (sp->loc == FROM_LINE)
		while (*count > 0 && (c = getc(fp)) != EOF) {
			sp->lastc = c;
			if (c == '\n') {
				sp->loc = HEADER;
				break;
			}
		}
	i = 0;
	j = 0;
	if (sp->loc == HEADER && sp->field[0]) {
	field:	cp = sp->field;
		do {
			c = *cp&0377;
			SAVE(c)
			cp++;
		} while (*cp);
		j = i;
		i = 0;
	}
	if (sp->price) {
		sp->price = 0;
		SAVE('$')
	}
	while (*count > 0 && (c = getc(fp)) != EOF) {
		(*count)--;
		if (c == '\0' && table_version >= 1) {
			sp->loc = HEADER;
			sp->lastc = '\n';
			*stop = 1;
			continue;
		}
		if (c == '\b' && table_version >= 1) {
			sp->html = HTML_TEXT;
			continue;
		}
		if (c == '<' && sp->html == HTML_TEXT) {
			sp->html = HTML_TAG;
			sp->tagp = sp->tag;
			continue;
		}
		if (sp->html == HTML_TAG) {
			if (spacechar(c)) {
				*sp->tagp = '\0';
				if (!asccasecmp(sp->tag, "a") ||
						!asccasecmp(sp->tag, "img") ||
						!asccasecmp(sp->tag, "font") ||
						!asccasecmp(sp->tag, "span") ||
						!asccasecmp(sp->tag, "meta") ||
						!asccasecmp(sp->tag, "table") ||
						!asccasecmp(sp->tag, "tr") ||
						!asccasecmp(sp->tag, "td") ||
						!asccasecmp(sp->tag, "p"))
					sp->html = HTML_TEXT;
				else
					sp->html = HTML_SKIP;
			} else if (c == '>') {
				sp->html = HTML_TEXT;
				continue;
			} else {
				if (sp->tagp - sp->tag < sizeof sp->tag - 1)
					*sp->tagp++ = c;
				continue;
			}
		}
		if (sp->html == HTML_SKIP) {
			if (c == '>')
				sp->html = HTML_TEXT;
			continue;
		}
		if (c == '$' && i == 0)
			sp->price = 1;
		if (sp->loc == HEADER && sp->lastc == '\n') {
			if (!spacechar(c)) {
				k = 0;
				while (k < sizeof sp->field - 3) {
					sp->field[k++] = c;
					if (*count <= 0 ||
							(c = getc(fp)) == EOF)
						break;
					if (spacechar(c) || c == ':') {
						ungetc(c, fp);
						break;
					}
					sp->lastc = c;
					(*count)--;
				}
				sp->field[k++] = '*';
				sp->field[k] = '\0';
				j = 0;
				*stop = 1;
				goto field;
			} else if (c == '\n') {
				j = 0;
				sp->loc = BODY;
				sp->html = HTML_NONE;
				*stop = 1;
			}
		}
		if (sp->url) {
			if (!url_xchar(c)) {
				sp->url = 0;
				cp = sp->save = smalloc(i+6);
				for (cq = "HOST*"; *cq; cq++)
					*cp++ = *cq;
				for (cq = &(*buf)[j]; *cq != ':'; cq++);
				cq += 3;	/* skip "://" */
				while (cq < &(*buf)[i+j] &&
						(alnumchar(*cq&0377) ||
						 *cq == '.' || *cq == '-'))
					*cp++ = *cq++;
				*cp = '\0';
				*stop = 1;
				break;
			}
			SAVE(c)
		} else if (constituent(c, *buf, i+j, sp->price, sp->hadamp) ||
				sp->loc == HEADER && c == '.' &&
				asccasecmp(sp->field, "subject*")) {
			if (c == '&')
				sp->hadamp = 1;
			SAVE(c)
		} else if (i > 0 && c == ':' && *count > 2) {
			if ((c = getc(fp)) != '/') {
				ungetc(c, fp);
				break;
			}
			(*count)--;
			if ((c = getc(fp)) != '/') {
				ungetc(c, fp);
				break;
			}
			(*count)--;
			sp->url = 1;
			SAVE('\0')
			cp = savestr(*buf);
			j = i = 0;
			for (cq = "URL*"; *cq; cq++) {
				SAVE(*cq&0377)
			}
			j = i;
			i = 0;
			do {
				if (alnumchar(*cp&0377)) {
					SAVE(*cp&0377)
				} else
					i = 0;
			} while (*++cp);
			for (cq = "://"; *cq; cq++) {
				SAVE(*cq&0377)
			}
		} else if (i > 1 && ((*buf)[i+j-1] == ',' ||
				 (*buf)[i+j-1] == '.') && !digitchar(c)) {
			i--;
			ungetc(c, fp);
			(*count)++;
			break;
		} else if (i > 0) {
			sp->lastc = c;
			break;
		}
		sp->lastc = c;
	}
out:	if (i > 0) {
		SAVE('\0')
		c = 0;
		for (k = 0; k < i; k++)
			if (digitchar((*buf)[k+j]&0377))
				c++;
			else if (!alphachar((*buf)[k+j]&0377) &&
					(*buf)[k+j] != '$') {
				c = 0;
				break;
			}
		if (c == i)
			goto loop;
		/*
		 * Including the results of other filtering software (the
		 * 'X-Spam' fields) might seem tempting, but will also rate
		 * their false negatives good with this filter. Therefore
		 * these fields are ignored.
		 *
		 * Handling 'Received' fields is difficult since they include
		 * lots of both useless and interesting words for our purposes.
		 */
		if (sp->loc == HEADER &&
				(asccasecmp(sp->field, "message-id*") == 0 ||
				 asccasecmp(sp->field, "references*") == 0 ||
				 asccasecmp(sp->field, "in-reply-to*") == 0 ||
				 asccasecmp(sp->field, "status*") == 0 ||
				 asccasecmp(sp->field, "x-status*") == 0 ||
				 asccasecmp(sp->field, "date*") == 0 ||
				 asccasecmp(sp->field, "delivery-date*") == 0 ||
				 ascncasecmp(sp->field, "x-spam", 6) == 0 ||
				 ascncasecmp(sp->field, "x-pstn", 6) == 0 ||
				 ascncasecmp(sp->field, "x-scanned", 9) == 0 ||
				 asccasecmp(sp->field, "received*") == 0 &&
				 	((2*c > i) || i < 4 ||
					asccasestr(*buf, "localhost") != NULL)))
			goto loop;
		return *buf;
	}
	return NULL;
}

#define	JOINCHECK	if (i >= *bufsize) \
				*buf = srealloc(*buf, *bufsize += 32)
static void
join(char **buf, size_t *bufsize, const char *s1, const char *s2)
{
	int	i = 0;

	while (*s1) {
		JOINCHECK;
		(*buf)[i++] = *s1++;
	}
	JOINCHECK;
	(*buf)[i++] = ' ';
	do {
		JOINCHECK;
		(*buf)[i++] = *s2;
	} while (*s2++);
}

/*ARGSUSED3*/
static void
add(const char *word, enum entry entry, struct lexstat *sp, int incr)
{
	unsigned	c;
	unsigned long	h1, h2;
	char	*n;

	dbhash(word, &h1, &h2);
	if ((n = lookup(h1, h2, 1)) != NULL) {
		switch (entry) {
		case GOOD:
			c = get(&n[OF_node_good]);
			if (incr>0 && c<MAX3-incr || incr<0 && c>=-incr) {
				c += incr;
				put(&n[OF_node_good], c);
			}
			break;
		case BAD:
			c = get(&n[OF_node_bad]);
			if (incr>0 && c<MAX3-incr || incr<0 && c>=-incr) {
				c += incr;
				put(&n[OF_node_bad], c);
			}
			break;
		}
	}
}

static enum okay 
scan(struct message *m, enum entry entry,
		void (*func)(const char *, enum entry, struct lexstat *, int),
		int arg)
{
	FILE	*fp;
	char	*buf0 = NULL, *buf1 = NULL, *buf2 = NULL, **bp, *cp;
	size_t	bufsize0 = 0, bufsize1 = 0, bufsize2 = 0, *zp, count;
	struct lexstat	*sp;
	int	stop;

	if ((fp = Ftemp(&cp, "Ra", "w+", 0600, 1)) == NULL) {
		perror("tempfile");
		return STOP;
	}
	rm(cp);
	Ftfree(&cp);
	if (send(m, fp, NULL, NULL, SEND_TOFLTR, NULL) < 0) {
		Fclose(fp);
		return STOP;
	}
	fflush(fp);
	rewind(fp);
	sp = scalloc(1, sizeof *sp);
	count = fsize(fp);
	bp = &buf0;
	zp = &bufsize0;
	while (nextword(bp, zp, &count, fp, sp, &stop) != NULL) {
		(*func)(*bp, entry, sp, arg);
		if (chained_tokens && buf0 && *buf0 && buf1 && *buf1 && !stop) {
			join(&buf2, &bufsize2, bp == &buf1 ? buf0 : buf1, *bp);
			(*func)(buf2, entry, sp, arg);
		}
		bp = bp == &buf1 ? &buf0 : &buf1;
		zp = zp == &bufsize1 ? &bufsize0 : &bufsize1;
	}
	free(buf0);
	free(buf1);
	free(buf2);
	free(sp);
	Fclose(fp);
	return OKAY;
}

static void
recompute(void)
{
	unsigned long	used, i;
	unsigned	s;
	char	*n;
	float	p;

	used = getn(&super[OF_super_used]);
	for (i = 0; i < used; i++) {
		n = &nodes[i*SIZEOF_node];
		p = getprob(n);
		s = f2s(p);
		put(&n[OF_node_prob_O], s);
	}
}

static float
getprob(char *n)
{
	unsigned long	ngood, nbad;
	unsigned	g, b;
	float	p, BOT, TOP;

	ngood = getn(&super[OF_super_ngood]);
	nbad = getn(&super[OF_super_nbad]);
	if (ngood + nbad >= 18000) {
		BOT = .0001;
		TOP = .9999;
	} else if (ngood + nbad >= 9000) {
		BOT = .001;
		TOP = .999;
	} else {
		BOT = .01;
		TOP = .99;
	}
	g = get(&n[OF_node_good]) * 2;
	b = get(&n[OF_node_bad]);
	if (g + b >= 5) {
		p = smin(1.0, nbad ? (float)b/nbad : 0.0) /
			(smin(1.0, ngood ? (float)g/ngood : 0.0) +
			 smin(1.0, nbad ? (float)b/nbad : 0.0));
		p = smin(TOP, p);
		p = smax(BOT, p);
		if (p == TOP && b <= 10 && g == 0)
			p -= BOT;
		else if (p == BOT && g <= 10 && b == 0)
			p += BOT;
	} else if (g == 0 && b == 0)
		p = DFL;
	else
		p = 0;
	return p;
}

static int 
insert(int *msgvec, enum entry entry, int incr)
{
	int	*ip;
	unsigned long	u = 0;

	verbose = value("verbose") != NULL;
	if (getdb(O_RDWR) != OKAY)
		return 1;
	switch (entry) {
	case GOOD:
		u = getn(&super[OF_super_ngood]);
		break;
	case BAD:
		u = getn(&super[OF_super_nbad]);
		break;
	}
	for (ip = msgvec; *ip; ip++) {
		setdot(&message[*ip-1]);
		if (incr > 0 && u == MAX4-incr+1) {
			fprintf(stderr, "Junk mail database overflow.\n");
			break;
		} else if (incr < 0 && -incr > u) {
			fprintf(stderr, "Junk mail database underflow.\n");
			break;
		}
		u += incr;
		if (entry == GOOD && incr > 0 || entry == BAD && incr < 0)
			message[*ip-1].m_flag &= ~MJUNK;
		else
			message[*ip-1].m_flag |= MJUNK;
		scan(&message[*ip-1], entry, add, incr);
	}
	switch (entry) {
	case GOOD:
		putn(&super[OF_super_ngood], u);
		break;
	case BAD:
		putn(&super[OF_super_nbad], u);
		break;
	}
	if (table_version < 1)
		recompute();
	putdb();
	relsedb();
	return 0;
}

int
cgood(void *v)
{
	return insert(v, GOOD, 1);
}

int
cjunk(void *v)
{
	return insert(v, BAD, 1);
}

int
cungood(void *v)
{
	return insert(v, GOOD, -1);
}

int
cunjunk(void *v)
{
	return insert(v, BAD, -1);
}

int 
cclassify(void *v)
{
	int	*msgvec = v, *ip;

	verbose = value("verbose") != NULL;
	_debug = debug || value("debug") != NULL;
	if (getdb(O_RDONLY) != OKAY)
		return 1;
	for (ip = msgvec; *ip; ip++) {
		setdot(&message[*ip-1]);
		clsf(&message[*ip-1]);
	}
	relsedb();
	return 0;
}

#define	BEST	15
static struct {
	float	dist;
	float	prob;
	char	*word;
	unsigned long	hash1;
	unsigned long	hash2;
	enum loc	loc;
} best[BEST];

static void 
clsf(struct message *m)
{
	int	i;
	float	a = 1, b = 1, r;

	if (verbose)
		fprintf(stderr, "Examining message %d\n", m - &message[0] + 1);
	for (i = 0; i < BEST; i++) {
		best[i].dist = 0;
		best[i].prob = -1;
	}
	if (scan(m, -1, rate, 0) != OKAY)
		return;
	if (best[0].prob == -1) {
		if (verbose)
			fprintf(stderr, "No information found.\n");
		m->m_flag &= ~MJUNK;
		return;
	}
	for (i = 0; i < BEST; i++) {
		if (best[i].prob == -1)
			break;
		if (verbose)
			fprintf(stderr, "Probe %2d: \"%s\", hash=%lu:%lu "
				"prob=%.4g dist=%.4g\n",
				i+1, prstr(best[i].word),
				best[i].hash1, best[i].hash2,
				best[i].prob, best[i].dist);
		a *= best[i].prob;
		b *= 1 - best[i].prob;
	}
	r = a+b > 0 ? a / (a+b) : 0;
	if (verbose)
		fprintf(stderr, "Junk probability of message %d: %g\n",
				m - &message[0] + 1, r);
	if (r > THR)
		m->m_flag |= MJUNK;
	else
		m->m_flag &= ~MJUNK;
}

/*ARGSUSED4*/
static void
rate(const char *word, enum entry entry, struct lexstat *sp, int unused)
{
	char	*n;
	unsigned long	h1, h2;
	float	p, d;
	int	i, j;

	dbhash(word, &h1, &h2);
	if ((n = lookup(h1, h2, 0)) != NULL) {
		p = getprob(n);
	} else
		p = DFL;
	if (_debug)
		fprintf(stderr, "h=%lu:%lu g=%u b=%u p=%.4g %s\n", h1, h2,
				n ? get(&n[OF_node_good]) : 0,
				n ? get(&n[OF_node_bad]) : 0,
				p, prstr(word));
	if (p == 0)
		return;
	d = p >= MID ? p - MID : MID - p;
	if (d >= best[BEST-1].dist)
		for (i = 0; i < BEST; i++) {
			if (h1 == best[i].hash1 && h2 == best[i].hash2)
				break;
			/*
			 * For equal distance, this selection prefers
			 * words with a low probability, since a false
			 * negative is better than a false positive,
			 * and since experience has shown that false
			 * positives are more likely otherwise. Then,
			 * words from the end of the header and from
			 * the start of the body are preferred. This
			 * gives the most interesting verbose output.
			 */
			if (d > best[i].dist ||
					d == best[i].dist &&
						p < best[i].prob ||
					best[i].loc == HEADER &&
						d == best[i].dist) {
				for (j = BEST-2; j >= i; j--)
					best[j+1] = best[j];
				best[i].dist = d;
				best[i].prob = p;
				best[i].word = savestr(word);
				best[i].hash1 = h1;
				best[i].hash2 = h2;
				best[i].loc = sp->loc;
				break;
			}
		}
}

static void
dbhash(const char *word, unsigned long *h1, unsigned long *h2)
{
	unsigned char	digest[16];
	MD5_CTX	ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char *)word, strlen(word));
	if (table_version >= 1)
		MD5Update(&ctx, (unsigned char *)&super[OF_super_mangle], 4);
	MD5Final(digest, &ctx);
	*h1 = getn(digest);
	if (table_version < 1) {
		*h1 ^= getn(&super[OF_super_mangle]);
		*h2 = 0;
	} else
		*h2 = get(&digest[4]);
}

/*
 * The selection of the value for mangling is not critical. It is practically
 * impossible for any person to determine the exact time when the database
 * was created first (without looking at the database, which would reveal the
 * value anyway), so we just use this. The MD5 hash here ensures that each
 * single second gives a completely different mangling value (which is not
 * necessary anymore if table_version>=1, but does not hurt).
 */
static void 
mkmangle(void)
{
	union {
		time_t	t;
		char	c[16];
	} u;
	unsigned long	s;
	unsigned char	digest[16];
	MD5_CTX	ctx;

	memset(&u, 0, sizeof u);
	time(&u.t);
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char *)u.c, sizeof u.c);
	MD5Final(digest, &ctx);
	s = getn(digest);
	putn(&super[OF_super_mangle], s);
}

int
cprobability(void *v)
{
	char	**args = v;
	unsigned long	used, ngood, nbad;
	unsigned long	h1, h2;
	unsigned	g, b;
	float	p, d;
	char	*n;

	if (*args == NULL) {
		fprintf(stderr, "No words given.\n");
		return 1;
	}
	if (getdb(O_RDONLY) != OKAY)
		return 1;
	used = getn(&super[OF_super_used]);
	ngood = getn(&super[OF_super_ngood]);
	nbad = getn(&super[OF_super_nbad]);
	printf("Database statistics: tokens=%lu ngood=%lu nbad=%lu\n",
			used, ngood, nbad);
	do {
		dbhash(*args, &h1, &h2);
		printf("\"%s\", hash=%lu:%lu ", *args, h1, h2);
		if ((n = lookup(h1, h2, 0)) != NULL) {
			g = get(&n[OF_node_good]);
			b = get(&n[OF_node_bad]);
			printf("good=%u bad=%u ", g, b);
			p = getprob(n);
			if (p != 0) {
				d = p >= MID ? p - MID : MID - p;
				printf("prob=%.4g dist=%.4g", p, d);
			} else
				printf("too infrequent");
		} else
			printf("not in database");
		putchar('\n');
	} while (*++args);
	relsedb();
	return 0;
}

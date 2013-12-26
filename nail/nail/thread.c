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
static char sccsid[] = "@(#)thread.c	1.57 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "config.h"

#include "rcv.h"
#include "extern.h"
#include <time.h>

/*
 * Mail -- a mail program
 *
 * Message threading.
 */

/*
 * Open addressing is used for Message-IDs because the maximum number of
 * messages in the table is known in advance (== msgCount).
 */
struct	mitem {
	struct message	*mi_data;
	char	*mi_id;
};

struct msort {
	union {
		long	ms_long;
		char	*ms_char;
		float	ms_float;
	} ms_u;
	int	ms_n;
};

static unsigned mhash(const char *cp, int mprime);
static struct mitem *mlook(char *id, struct mitem *mt, struct message *mdata,
		int mprime);
static void adopt(struct message *parent, struct message *child, int dist);
static struct message *interlink(struct message *m, long count, int newmail);
static void finalize(struct message *mp);
static int mlonglt(const void *a, const void *b);
static int mfloatlt(const void *a, const void *b);
static int mcharlt(const void *a, const void *b);
static void lookup(struct message *m, struct mitem *mi, int mprime);
static void makethreads(struct message *m, long count, int newmail);
static char *skipre(const char *cp);
static int colpt(int *msgvec, int cl);
static void colps(struct message *b, int cl);
static void colpm(struct message *m, int cl, int *cc, int *uc);

/*
 * Return the hash value for a message id modulo mprime, or mprime
 * if the passed string does not look like a message-id.
 */
static unsigned 
mhash(const char *cp, int mprime)
{

	unsigned	h = 0, g, at = 0;

	cp--;
	while (*++cp) {
		/*
		 * Pay attention not to hash characters which are
		 * irrelevant for Message-ID semantics.
		 */
		if (*cp == '(') {
			cp = skip_comment(&cp[1]) - 1;
			continue;
		}
		if (*cp == '"' || *cp == '\\')
			continue;
		if (*cp == '@')
			at++;
		h = ((h << 4) & 0xffffffff) + lowerconv(*cp & 0377);
		if ((g = h & 0xf0000000) != 0) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return at ? h % mprime : mprime;
}

#define	NOT_AN_ID	((struct mitem *)-1)

/*
 * Look up a message id. Returns NOT_AN_ID if the passed string does
 * not look like a message-id.
 */
static struct mitem *
mlook(char *id, struct mitem *mt, struct message *mdata, int mprime)
{
	struct mitem	*mp;
	unsigned	h, c, n = 0;

	if (id == NULL && (id = hfield("message-id", mdata)) == NULL)
		return NULL;
	if (mdata && mdata->m_idhash)
		h = ~mdata->m_idhash;
	else {
		h = mhash(id, mprime);
		if (h == mprime)
			return NOT_AN_ID;
	}
	mp = &mt[c = h];
	while (mp->mi_id != NULL) {
		if (msgidcmp(mp->mi_id, id) == 0)
			break;
		c += n&1 ? -((n+1)/2) * ((n+1)/2) : ((n+1)/2) * ((n+1)/2);
		n++;
		while (c >= mprime)
			c -= mprime;
		mp = &mt[c];
	}
	if (mdata != NULL && mp->mi_id == NULL) {
		mp->mi_id = id;
		mp->mi_data = mdata;
		mdata->m_idhash = ~h;
	}
	return mp->mi_id ? mp : NULL;
}

/*
 * Child is to be adopted by parent. A thread tree is structured
 * as follows:
 *
 *  ------       m_child       ------        m_child
 *  |    |-------------------->|    |------------------------> . . .
 *  |    |<--------------------|    |<-----------------------  . . .
 *  ------      m_parent       ------       m_parent
 *     ^^                       |  ^
 *     | \____        m_younger |  |
 *     |      \                 |  |
 *     |       ----             |  |
 *     |           \            |  | m_elder
 *     |   m_parent ----        |  |
 *     |                \       |  |
 *     |                 ----   |  |
 *     |                     \  +  |
 *     |                       ------        m_child
 *     |                       |    |------------------------> . . .
 *     |                       |    |<-----------------------  . . .
 *     |                       ------       m_parent
 *     |                        |  ^
 *      \-----        m_younger |  |
 *            \                 |  |
 *             ----             |  |
 *                 \            |  | m_elder
 *         m_parent ----        |  |
 *                      \       |  |
 *                       ----   |  |
 *                           \  +  |
 *                             ------        m_child
 *                             |    |------------------------> . . .
 *                             |    |<-----------------------  . . .
 *                             ------       m_parent
 *                              |  ^
 *                              . . .
 *
 * The base message of a thread does not have a m_parent link. Elements
 * connected by m_younger/m_elder links are replies to the same message,
 * which is connected to them by m_parent links. The first reply to a
 * message gets the m_child link.
 */
static void 
adopt(struct message *parent, struct message *child, int dist)
{
	struct message	*mp, *mq;

	for (mp = parent; mp; mp = mp->m_parent)
		if (mp == child)
			return;
	child->m_level = dist;	/* temporarily store distance */
	child->m_parent = parent;
	if (parent->m_child != NULL) {
		mq = NULL;
		for (mp = parent->m_child; mp; mp = mp->m_younger) {
			if (mp->m_date >= child->m_date) {
				if (mp->m_elder)
					mp->m_elder->m_younger = child;
				child->m_elder = mp->m_elder;
				mp->m_elder = child;
				child->m_younger = mp;
				if (mp == parent->m_child)
					parent->m_child = child;
				return;
			}
			mq = mp;
		}
		mq->m_younger = child;
		child->m_elder = mq;
	} else
		parent->m_child = child;
}

/*
 * Connect all messages on the lowest thread level with m_younger/m_elder
 * links.
 */
static struct message *
interlink(struct message *m, long count, int newmail)
{
	int	i;
	long	n;
	struct msort	*ms;
	struct message	*root;
	int	autocollapse = !newmail && !(inhook&2) &&
			value("autocollapse") != NULL;

	ms = smalloc(sizeof *ms * count);
	for (n = 0, i = 0; i < count; i++) {
		if (m[i].m_parent == NULL) {
			if (autocollapse)
				colps(&m[i], 1);
			ms[n].ms_u.ms_long = m[i].m_date;
			ms[n].ms_n = i;
			n++;
		}
	}
	if (n > 0) {
		qsort(ms, n, sizeof *ms, mlonglt);
		root = &m[ms[0].ms_n];
		for (i = 1; i < n; i++) {
			m[ms[i-1].ms_n].m_younger = &m[ms[i].ms_n];
			m[ms[i].ms_n].m_elder = &m[ms[i-1].ms_n];
		}
	} else
		root = &m[0];
	free(ms);
	return root;
}

static void 
finalize(struct message *mp)
{
	long	n;

	for (n = 0; mp; mp = next_in_thread(mp)) {
		mp->m_threadpos = ++n;
		mp->m_level = mp->m_parent ?
			mp->m_level + mp->m_parent->m_level : 0;
	}
}

static int 
mlonglt(const void *a, const void *b)
{
	int	i;

	i = ((struct msort *)a)->ms_u.ms_long -
		((struct msort *)b)->ms_u.ms_long;
	if (i == 0)
		i = ((struct msort *)a)->ms_n - ((struct msort *)b)->ms_n;
	return i;
}

static int 
mfloatlt(const void *a, const void *b)
{
	float	i;

	i = ((struct msort *)a)->ms_u.ms_float -
		((struct msort *)b)->ms_u.ms_float;
	if (i == 0)
		i = ((struct msort *)a)->ms_n - ((struct msort *)b)->ms_n;
	return i > 0 ? 1 : i < 0 ? -1 : 0;
}

static int 
mcharlt(const void *a, const void *b)
{
	int	i;

	i = strcoll(((struct msort *)a)->ms_u.ms_char,
			((struct msort *)b)->ms_u.ms_char);
	if (i == 0)
		i = ((struct msort *)a)->ms_n - ((struct msort *)b)->ms_n;
	return i;
}


static void 
lookup(struct message *m, struct mitem *mi, int mprime)
{
	struct name	*np;
	struct mitem	*ip;
	char	*cp;
	long	dist;

	if (m->m_flag & MHIDDEN)
		return;
	dist = 1;
	if ((cp = hfield("in-reply-to", m)) != NULL) {
		if ((np = extract(cp, GREF)) != NULL)
			do {
				if ((ip = mlook(np->n_name, mi, NULL, mprime))
						!= NULL && ip != NOT_AN_ID) {
					adopt(ip->mi_data, m, 1);
					return;
				}
			} while ((np = np->n_flink) != NULL);
	}
	if ((cp = hfield("references", m)) != NULL) {
		if ((np = extract(cp, GREF)) != NULL) {
			while (np->n_flink != NULL)
				np = np->n_flink;
			do {
				if ((ip = mlook(np->n_name, mi, NULL, mprime))
						!= NULL) {
					if (ip == NOT_AN_ID)
						continue; /* skip dist++ */
					adopt(ip->mi_data, m, dist);
					return;
				}
				dist++;
			} while ((np = np->n_blink) != NULL);
		}
	}
}

static void 
makethreads(struct message *m, long count, int newmail)
{
	struct mitem	*mt;
	char	*cp;
	long	i, mprime;

	if (count == 0)
		return;
	mprime = nextprime(count);
	mt = scalloc(mprime, sizeof *mt);
	for (i = 0; i < count; i++) {
		if ((m[i].m_flag&MHIDDEN) == 0) {
			mlook(NULL, mt, &m[i], mprime);
			if (m[i].m_date == 0) {
				if ((cp = hfield("date", &m[i])) != NULL)
					m[i].m_date = rfctime(cp);
			}
		}
		m[i].m_child = m[i].m_younger = m[i].m_elder =
			m[i].m_parent = NULL;
		m[i].m_level = 0;
		if (!newmail && !(inhook&2))
			m[i].m_collapsed = 0;
	}
	/*
	 * Most folders contain the eldest messages first. Traversing
	 * them in descending order makes it more likely that younger
	 * brothers are found first, so elder ones can be prepended to
	 * the brother list, which is faster. The worst case is still
	 * in O(n^2) and occurs when all but one messages in a folder
	 * are replies to the one message, and are sorted such that
	 * youngest messages occur first.
	 */
	for (i = count-1; i >= 0; i--)
		lookup(&m[i], mt, mprime);
	threadroot = interlink(m, count, newmail);
	finalize(threadroot);
	free(mt);
	mb.mb_threaded = 1;
}

int 
thread(void *vp)
{
	if (mb.mb_threaded != 1 || vp == NULL || vp == (void *)-1) {
		if (mb.mb_type == MB_IMAP)
			imap_getheaders(1, msgCount);
		makethreads(message, msgCount, vp == (void *)-1);
		free(mb.mb_sorted);
		mb.mb_sorted = sstrdup("thread");
	}
	if (vp && vp != (void *)-1 && !inhook && value("header"))
		return headers(vp);
	return 0;
}

int 
unthread(void *vp)
{
	struct message	*m;

	mb.mb_threaded = 0;
	free(mb.mb_sorted);
	mb.mb_sorted = NULL;
	for (m = &message[0]; m < &message[msgCount]; m++)
		m->m_collapsed = 0;
	if (vp && !inhook && value("header"))
		return headers(vp);
	return 0;
}

struct message *
next_in_thread(struct message *mp)
{
	if (mp->m_child)
		return mp->m_child;
	if (mp->m_younger)
		return mp->m_younger;
	while (mp->m_parent) {
		if (mp->m_parent->m_younger)
			return mp->m_parent->m_younger;
		mp = mp->m_parent;
	}
	return NULL;
}

struct message *
prev_in_thread(struct message *mp)
{
	if (mp->m_elder) {
		mp = mp->m_elder;
		while (mp->m_child) {
			mp = mp->m_child;
			while (mp->m_younger)
				mp = mp->m_younger;
		}
		return mp;
	}
	return mp->m_parent;
}

struct message *
this_in_thread(struct message *mp, long n)
{
	struct message	*mq;

	if (n == -1) {	/* find end of thread */
		while (mp) {
			if (mp->m_younger) {
				mp = mp->m_younger;
				continue;
			}
			mq = next_in_thread(mp);
			if (mq == NULL || mq->m_threadpos < mp->m_threadpos)
				return mp;
			mp = mq;
		}
		return NULL;
	}
	while (mp && mp->m_threadpos < n) {
		if (mp->m_younger && mp->m_younger->m_threadpos <= n) {
			mp = mp->m_younger;
			continue;
		}
		mp = next_in_thread(mp);
	}
	return mp && mp->m_threadpos == n ? mp : NULL;
}

/*
 * Sorted mode is internally just a variant of threaded mode with all
 * m_parent and m_child links being NULL.
 */
int 
sort(void *vp)
{
	enum method {
		SORT_SUBJECT,
		SORT_DATE,
		SORT_STATUS,
		SORT_SIZE,
		SORT_FROM,
		SORT_TO,
		SORT_SCORE,
		SORT_THREAD
	} method;
	struct {
		const char	*me_name;
		enum method	me_method;
		int	(*me_func)(const void *, const void *);
	} methnames[] = {
		{ "date",	SORT_DATE,	mlonglt },
		{ "from",	SORT_FROM,	mcharlt },
		{ "to",		SORT_TO,	mcharlt },
		{ "subject",	SORT_SUBJECT,	mcharlt },
		{ "size",	SORT_SIZE,	mlonglt },
		{ "status",	SORT_STATUS,	mlonglt },
		{ "score",	SORT_SCORE,	mfloatlt },
		{ "thread",	SORT_THREAD,	NULL },
		{ NULL,		-1,		NULL }
	};
	char	**args = (char **)vp, *cp, *_args[2];
	int	(*func)(const void *, const void *);
	struct msort	*ms;
	struct str	in, out;
	int	i, n, msgvec[2];
	int	showname = value("showname") != NULL;
	struct message	*mp;

	msgvec[0] = dot - &message[0] + 1;
	msgvec[1] = 0;
	if (vp == NULL || vp == (void *)-1) {
		_args[0] = savestr(mb.mb_sorted);
		_args[1] = NULL;
		args = _args;
	} else if (args[0] == NULL) {
		printf("Current sorting criterion is: %s\n",
				mb.mb_sorted ? mb.mb_sorted : "unsorted");
		return 0;
	}
	for (i = 0; methnames[i].me_name; i++)
		if (*args[0] && is_prefix(args[0], methnames[i].me_name))
			break;
	if (methnames[i].me_name == NULL) {
		fprintf(stderr, "Unknown sorting method \"%s\"\n", args[0]);
		return 1;
	}
	method = methnames[i].me_method;
	func = methnames[i].me_func;
	free(mb.mb_sorted);
	mb.mb_sorted = sstrdup(args[0]);
	if (method == SORT_THREAD)
		return thread(vp && vp != (void *)-1 ? msgvec : vp);
	ms = ac_alloc(sizeof *ms * msgCount);
	switch (method) {
	case SORT_SUBJECT:
	case SORT_DATE:
	case SORT_FROM:
	case SORT_TO:
		if (mb.mb_type == MB_IMAP)
			imap_getheaders(1, msgCount);
		break;
	default:
		break;
	}
	for (n = 0, i = 0; i < msgCount; i++) {
		mp = &message[i];
		if ((mp->m_flag&MHIDDEN) == 0) {
			switch (method) {
			case SORT_DATE:
				if (mp->m_date == 0 &&
						(cp = hfield("date", mp)) != 0)
					mp->m_date = rfctime(cp);
				ms[n].ms_u.ms_long = mp->m_date;
				break;
			case SORT_STATUS:
				if (mp->m_flag & MDELETED)
					ms[n].ms_u.ms_long = 1;
				else if ((mp->m_flag&(MNEW|MREAD)) == MNEW)
					ms[n].ms_u.ms_long = 90;
				else if (mp->m_flag & MFLAGGED)
					ms[n].ms_u.ms_long = 85;
				else if ((mp->m_flag&(MNEW|MBOX)) == MBOX)
					ms[n].ms_u.ms_long = 70;
				else if (mp->m_flag & MNEW)
					ms[n].ms_u.ms_long = 80;
				else if (mp->m_flag & MREAD)
					ms[n].ms_u.ms_long = 40;
				else
					ms[n].ms_u.ms_long = 60;
				break;
			case SORT_SIZE:
				ms[n].ms_u.ms_long = mp->m_xsize;
				break;
			case SORT_SCORE:
				ms[n].ms_u.ms_float = mp->m_score;
				break;
			case SORT_FROM:
			case SORT_TO:
				if ((cp = hfield(method == SORT_FROM ?
						"from" : "to", mp)) != NULL) {
					ms[n].ms_u.ms_char = showname ?
						realname(cp) : skin(cp);
					makelow(ms[n].ms_u.ms_char);
				} else
					ms[n].ms_u.ms_char = "";
				break;
			default:
			case SORT_SUBJECT:
				if ((cp = hfield("subject", mp)) != NULL) {
					in.s = cp;
					in.l = strlen(in.s);
					mime_fromhdr(&in, &out, TD_ICONV);
					ms[n].ms_u.ms_char =
						savestr(skipre(out.s));
					free(out.s);
					makelow(ms[n].ms_u.ms_char);
				} else
					ms[n].ms_u.ms_char = "";
				break;
			}
			ms[n++].ms_n = i;
		}
		mp->m_child = mp->m_younger = mp->m_elder = mp->m_parent = NULL;
		mp->m_level = 0;
		mp->m_collapsed = 0;
	}
	if (n > 0) {
		qsort(ms, n, sizeof *ms, func);
		threadroot = &message[ms[0].ms_n];
		for (i = 1; i < n; i++) {
			message[ms[i-1].ms_n].m_younger = &message[ms[i].ms_n];
			message[ms[i].ms_n].m_elder = &message[ms[i-1].ms_n];
		}
	} else
		threadroot = &message[0];
	finalize(threadroot);
	mb.mb_threaded = 2;
	ac_free(ms);
	return vp && vp != (void *)-1 && !inhook &&
		value("header") ? headers(msgvec) : 0;
}

static char *
skipre(const char *cp)
{
	if (lowerconv(cp[0]&0377) == 'r' &&
			lowerconv(cp[1]&0377) == 'e' &&
			cp[2] == ':' &&
			spacechar(cp[3]&0377)) {
		cp = &cp[4];
		while (spacechar(*cp&0377))
			cp++;
	}
	return (char *)cp;
}

int 
ccollapse(void *v)
{
	return colpt(v, 1);
}

int 
cuncollapse(void *v)
{
	return colpt(v, 0);
}

static int 
colpt(int *msgvec, int cl)
{
	int	*ip;

	if (mb.mb_threaded != 1) {
		puts("Not in threaded mode.");
		return 1;
	}
	for (ip = msgvec; *ip != 0; ip++)
		colps(&message[*ip-1], cl);
	return 0;
}

static void 
colps(struct message *b, int cl)
{
	struct message	*m;
	int	cc = 0, uc = 0;

	if (cl && (b->m_collapsed > 0 || (b->m_flag & (MNEW|MREAD)) == MNEW))
		return;
	if (b->m_child) {
		m = b->m_child;
		colpm(m, cl, &cc, &uc);
		for (m = m->m_younger; m; m = m->m_younger)
			colpm(m, cl, &cc, &uc);
	}
	if (cl) {
		b->m_collapsed = -cc;
		for (m = b->m_parent; m; m = m->m_parent)
			if (m->m_collapsed <= -uc ) {
				m->m_collapsed += uc;
				break;
			}
	} else {
		if (b->m_collapsed > 0) {
			b->m_collapsed = 0;
			uc++;
		}
		for (m = b; m; m = m->m_parent)
			if (m->m_collapsed <= -uc) {
				m->m_collapsed += uc;
				break;
			}
	}
}

static void 
colpm(struct message *m, int cl, int *cc, int *uc)
{
	if (cl) {
		if (m->m_collapsed > 0)
			(*uc)++;
		if ((m->m_flag & (MNEW|MREAD)) != MNEW || m->m_collapsed < 0)
			m->m_collapsed = 1;
		if (m->m_collapsed > 0)
			(*cc)++;
	} else {
		if (m->m_collapsed > 0) {
			m->m_collapsed = 0;
			(*uc)++;
		}
	}
	if (m->m_child) {
		m = m->m_child;
		colpm(m, cl, cc, uc);
		for (m = m->m_younger; m; m = m->m_younger)
			colpm(m, cl, cc, uc);
	}
}

void 
uncollapse1(struct message *m, int always)
{
	if (mb.mb_threaded == 1 && (always || m->m_collapsed > 0))
		colps(m, 0);
}

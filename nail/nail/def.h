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
 *
 *	Sccsid @(#)def.h	2.104 (gritter) 3/4/06
 */

/*
 * Mail -- a mail program
 *
 * Author: Kurt Shoens (UCB) March 25, 1978
 */

#if !defined (NI_MAXHOST) || (NI_MAXHOST) < 1025
#undef	NI_MAXHOST
#define	NI_MAXHOST	1025
#endif

#define	APPEND				/* New mail goes to end of mailbox */

#define	ESCAPE		'~'		/* Default escape for sending */
#ifndef	MAXPATHLEN
#ifdef	PATH_MAX
#define	MAXPATHLEN	PATH_MAX
#else
#define	MAXPATHLEN	1024
#endif
#endif
#ifndef	PATHSIZE
#define	PATHSIZE	MAXPATHLEN	/* Size of pathnames throughout */
#endif
#define	HSHSIZE		59		/* Hash size for aliases and vars */
#define	LINESIZE	2560		/* max readable line width */
#define	STRINGSIZE	((unsigned) 128)/* Dynamic allocation units */
#define	MAXARGC		1024		/* Maximum list of raw strings */
#define	MAXEXP		25		/* Maximum expansion of aliases */

#define	equal(a, b)	(strcmp(a,b)==0)/* A nice function to string compare */

#ifdef	HAVE_CATGETS
#define	CATSET	1
#else	/* !HAVE_CATGETS */
#define	catgets(a, b, c, d)	(d)
#endif	/* !HAVE_CATGETS */

typedef void (*sighandler_type)(int);

enum okay {
	STOP = 0,
	OKAY = 1
};

enum mimeenc {
	MIME_NONE,			/* message is not in MIME format */
	MIME_BIN,			/* message is in binary encoding */
	MIME_8B,			/* message is in 8bit encoding */
	MIME_7B,			/* message is in 7bit encoding */
	MIME_QP,			/* message is quoted-printable */
	MIME_B64			/* message is in base64 encoding */
};

enum conversion {
	CONV_NONE,			/* no conversion */
	CONV_7BIT,			/* no conversion, is 7bit */
	CONV_FROMQP,			/* convert from quoted-printable */
	CONV_TOQP,			/* convert to quoted-printable */
	CONV_8BIT,			/* convert to 8bit (iconv) */
	CONV_FROMB64,			/* convert from base64 */
	CONV_FROMB64_T,			/* convert from base64/text */
	CONV_TOB64,			/* convert to base64 */
	CONV_FROMHDR,			/* convert from RFC1522 format */
	CONV_TOHDR,			/* convert to RFC1522 format */
	CONV_TOHDR_A			/* convert addresses for header */
};

enum sendaction {
	SEND_MBOX,			/* no conversion to perform */
	SEND_RFC822,			/* no conversion, no From_ line */
	SEND_TODISP,			/* convert to displayable form */
	SEND_TODISP_ALL,		/* same, include all MIME parts */
	SEND_SHOW,			/* convert to 'show' command form */
	SEND_TOSRCH,			/* convert for IMAP SEARCH */
	SEND_TOFLTR,			/* convert for junk mail filtering */
	SEND_TOFILE,			/* convert for saving body to a file */
	SEND_TOPIPE,			/* convert for pipe-content/subc. */
	SEND_QUOTE,			/* convert for quoting */
	SEND_QUOTE_ALL,			/* same, include all MIME parts */
	SEND_DECRYPT			/* decrypt */
};

enum mimecontent {
	MIME_UNKNOWN,			/* unknown content */
	MIME_SUBHDR,			/* inside a multipart subheader */
	MIME_822,			/* message/rfc822 content */
	MIME_MESSAGE,			/* other message/ content */
	MIME_TEXT_PLAIN,		/* text/plain content */
	MIME_TEXT_HTML,			/* text/html content */
	MIME_TEXT,			/* other text/ content */
	MIME_ALTERNATIVE,		/* multipart/alternative content */
	MIME_DIGEST,			/* multipart/digest content */
	MIME_MULTI,			/* other multipart/ content */
	MIME_PKCS7,			/* PKCS7 content */
	MIME_DISCARD			/* content is discarded */
};

enum mimeclean {
	MIME_CLEAN	= 000,		/* plain RFC 2822 message */
	MIME_HIGHBIT	= 001,		/* characters >= 0200 */
	MIME_LONGLINES	= 002,		/* has lines too long for RFC 2822 */
	MIME_CTRLCHAR	= 004,		/* contains control characters */
	MIME_HASNUL	= 010,		/* contains \0 characters */
	MIME_NOTERMNL	= 020		/* lacks a terminating newline */
};

enum tdflags {
	TD_NONE		= 0,	/* no display conversion */
	TD_ISPR		= 01,	/* use isprint() checks */
	TD_ICONV	= 02,	/* use iconv() */
	TD_DELCTRL	= 04	/* delete control characters */
};

struct str {
	char *s;			/* the string's content */
	size_t l;			/* the stings's length */
};

enum protocol {
	PROTO_FILE,			/* refers to a local file */
	PROTO_POP3,			/* is a pop3 server string */
	PROTO_IMAP,			/* is an imap server string */
	PROTO_MAILDIR,			/* refers to a maildir folder */
	PROTO_UNKNOWN			/* unknown protocol */
};

struct sock {				/* data associated with a socket */
	int	s_fd;			/* file descriptor */
#ifdef	USE_SSL
	int	s_use_ssl;		/* SSL is used */
#if defined (USE_NSS)
	void	*s_prfd;		/* NSPR file descriptor */
#elif defined (USE_OPENSSL)
	void	*s_ssl;			/* SSL object */
	void	*s_ctx;			/* SSL context object */
#endif	/* SSL library specific */
#endif	/* USE_SSL */
	char	*s_wbuf;		/* for buffered writes */
	int	s_wbufsize;		/* allocated size of s_buf */
	int	s_wbufpos;		/* position of first empty data byte */
	char	s_rbuf[LINESIZE+1];	/* for buffered reads */
	char	*s_rbufptr;		/* read pointer to s_rbuf */
	int	s_rsz;			/* size of last read in s_rbuf */
	char	*s_desc;		/* description of error messages */
	void	(*s_onclose)(void);	/* execute on close */
};

struct mailbox {
	struct sock	mb_sock;	/* socket structure */
	enum {
		MB_NONE		= 000,	/* no reply expected */
		MB_COMD		= 001,	/* command reply expected */
		MB_MULT		= 002,	/* multiline reply expected */
		MB_PREAUTH	= 004,	/* not in authenticated state */
		MB_BYE		= 010	/* may accept a BYE state */
	} mb_active;
	FILE *mb_itf;			/* temp file with messages, read open */
	FILE *mb_otf;			/* same, write open */
	char *mb_sorted;		/* sort method */
	enum {
		MB_VOID,		/* no type (e. g. connection failed) */
		MB_FILE,		/* local file */
		MB_POP3,		/* POP3 mailbox */
		MB_IMAP,		/* IMAP mailbox */
		MB_MAILDIR,		/* maildir folder */
		MB_CACHE		/* cached mailbox */
	} mb_type;			/* type of mailbox */
	enum {
		MB_DELE = 01,		/* may delete messages in mailbox */
		MB_EDIT = 02		/* may edit messages in mailbox */
	} mb_perm;
	int mb_compressed;		/* is a compressed mbox file */
	int mb_threaded;		/* mailbox has been threaded */
	enum mbflags {
		MB_NOFLAGS	= 000,
		MB_UIDPLUS	= 001	/* supports IMAP UIDPLUS */
	} mb_flags;
	unsigned long	mb_uidvalidity;	/* IMAP unique identifier validity */
	char	*mb_imap_account;	/* name of current IMAP account */
	char	*mb_imap_mailbox;	/* name of current IMAP mailbox */
	char	*mb_cache_directory;	/* name of cache directory */
};

enum needspec {
	NEED_UNSPEC,			/* unspecified need, don't fetch */
	NEED_HEADER,			/* need the header of a message */
	NEED_BODY			/* need header and body of a message */
};

enum havespec {
	HAVE_NOTHING = 0,		/* nothing downloaded yet */
	HAVE_HEADER = 01,		/* header is downloaded */
	HAVE_BODY = 02			/* entire message is downloaded */
};

/*
 * flag bits. Attention: Flags that are used in cache.c may not change.
 */
enum mflag {
	MUSED		= (1<<0),	/* entry is used, but this bit isn't */
	MDELETED	= (1<<1),	/* entry has been deleted */
	MSAVED		= (1<<2),	/* entry has been saved */
	MTOUCH		= (1<<3),	/* entry has been noticed */
	MPRESERVE	= (1<<4),	/* keep entry in sys mailbox */
	MMARK		= (1<<5),	/* message is marked! */
	MODIFY		= (1<<6),	/* message has been modified */
	MNEW		= (1<<7),	/* message has never been seen */
	MREAD		= (1<<8),	/* message has been read sometime. */
	MSTATUS		= (1<<9),	/* message status has changed */
	MBOX		= (1<<10),	/* Send this to mbox, regardless */
	MNOFROM		= (1<<11),	/* no From line */
	MHIDDEN		= (1<<12),	/* message is hidden to user */
	MFULLYCACHED	= (1<<13),	/* message is completely cached */
	MBOXED		= (1<<14),	/* message has been sent to mbox */
	MUNLINKED	= (1<<15),	/* message was unlinked from cache */
	MNEWEST		= (1<<16),	/* message is very new (newmail) */
	MFLAG		= (1<<17),	/* message has been flagged recently */
	MUNFLAG		= (1<<18),	/* message has been unflagged */
	MFLAGGED	= (1<<19),	/* message is `flagged' */
	MANSWER		= (1<<20),	/* message has been answered recently */
	MUNANSWER	= (1<<21),	/* message has been unanswered */
	MANSWERED	= (1<<22),	/* message is `answered' */
	MDRAFT		= (1<<23),	/* message has been drafted recently */
	MUNDRAFT	= (1<<24),	/* message has been undrafted */
	MDRAFTED	= (1<<25),	/* message is marked as `draft' */
	MKILL		= (1<<26),	/* message has been killed */
	MOLDMARK	= (1<<27),	/* messages was marked previously */
	MJUNK		= (1<<28)	/* message is classified as junk */
};

struct mimepart {
	enum mflag	m_flag;		/* flags */
	enum havespec	m_have;		/* downloaded parts of the part */
	int	m_block;		/* block number of this part */
	size_t	m_offset;		/* offset in block of part */
	size_t	m_size;			/* Bytes in the part */
	size_t	m_xsize;		/* Bytes in the full part */
	long	m_lines;		/* Lines in the message */
	long	m_xlines;		/* Lines in the full message */
	time_t	m_time;			/* time the message was sent */
	char	*m_from;		/* message sender */
	struct mimepart	*m_nextpart;	/* next part at same level */
	struct mimepart	*m_multipart;	/* parts of multipart */
	struct mimepart	*m_parent;	/* enclosing multipart part */
	char	*m_ct_type;		/* content-type */
	char	*m_ct_type_plain;	/* content-type without specs */
	enum mimecontent	m_mimecontent;	/* same in enum */
	char	*m_charset;		/* charset */
	char	*m_ct_transfer_enc;	/* content-transfer-encoding */
	enum mimeenc	m_mimeenc;	/* same in enum */
	char	*m_partstring;		/* part level string */
	char	*m_filename;		/* attachment filename */
};

struct message {
	enum mflag	m_flag;		/* flags */
	enum havespec	m_have;		/* downloaded parts of the message */
	int	m_block;		/* block number of this message */
	size_t	m_offset;		/* offset in block of message */
	size_t	m_size;			/* Bytes in the message */
	size_t	m_xsize;		/* Bytes in the full message */
	long	m_lines;		/* Lines in the message */
	long	m_xlines;		/* Lines in the full message */
	time_t	m_time;			/* time the message was sent */
	time_t	m_date;			/* time in the 'Date' field */
	unsigned	m_idhash;	/* hash on Message-ID for threads */
	unsigned long	m_uid;		/* IMAP unique identifier */
	struct message	*m_child;	/* first child of this message */
	struct message	*m_younger;	/* younger brother of this message */
	struct message	*m_elder;	/* elder brother of this message */
	struct message	*m_parent;	/* parent of this message */
	unsigned	m_level;	/* thread level of message */
	long		m_threadpos;	/* position in threaded display */
	float		m_score;	/* score of message */
	char	*m_maildir_file;	/* original maildir file of msg */
	unsigned	m_maildir_hash;	/* hash of file name in maildir sub */
	int	m_collapsed;		/* collapsed thread information */
};

/*
 * Given a file address, determine the block number it represents.
 */
#define mailx_blockof(off)		((int) ((off) / 4096))
#define mailx_offsetof(off)		((int) ((off) % 4096))
#define mailx_positionof(block, offset)	((off_t)(block) * 4096 + (offset))

/*
 * Argument types.
 */
enum argtype {
	MSGLIST	= 0,		/* Message list type */
	STRLIST	= 1,		/* A pure string */
	RAWLIST	= 2,		/* Shell string list */
	NOLIST	= 3,		/* Just plain 0 */
	NDMLIST	= 4,		/* Message list, no defaults */
	ECHOLIST= 5,		/* Like raw list, but keep quote chars */
	P	= 040,		/* Autoprint dot after command */
	I	= 0100,		/* Interactive command bit */
	M	= 0200,		/* Legal from send mode bit */
	W	= 0400,		/* Illegal when read only bit */
	F	= 01000,	/* Is a conditional command */
	T	= 02000,	/* Is a transparent command */
	R	= 04000,	/* Cannot be called from collect */
	A	= 010000	/* Needs an active mailbox */
};

/*
 * Oft-used mask values
 */

#define	MMNORM		(MDELETED|MSAVED|MHIDDEN)/* Look at both save and delete bits */
#define	MMNDEL		(MDELETED|MHIDDEN)	/* Look only at deleted bit */

/*
 * Format of the command description table.
 * The actual table is declared and initialized
 * in lex.c
 */
struct cmd {
	char	*c_name;		/* Name of command */
	int	(*c_func)(void *);	/* Implementor of the command */
	enum argtype	c_argtype;	/* Type of arglist (see below) */
	short	c_msgflag;		/* Required flags of messages */
	short	c_msgmask;		/* Relevant flags of messages */
};

/* Yechh, can't initialize unions */

#define	c_minargs c_msgflag		/* Minimum argcount for RAWLIST */
#define	c_maxargs c_msgmask		/* Max argcount for RAWLIST */

/*
 * Structure used to return a break down of a head
 * line (hats off to Bill Joy!)
 */

struct headline {
	char	*l_from;	/* The name of the sender */
	char	*l_tty;		/* His tty string (if any) */
	char	*l_date;	/* The entire date string */
};

enum gfield {
	GTO	= 1,		/* Grab To: line */
	GSUBJECT= 2,		/* Likewise, Subject: line */
	GCC	= 4,		/* And the Cc: line */
	GBCC	= 8,		/* And also the Bcc: line */

	GNL	= 16,		/* Print blank line after */
	GDEL	= 32,		/* Entity removed from list */
	GCOMMA	= 64,		/* detract puts in commas */
	GUA	= 128,		/* User-Agent field */
	GMIME	= 256,		/* MIME 1.0 fields */
	GMSGID	= 512,		/* a Message-ID */
	/*	  1024 */	/* unused */
	GIDENT	= 2048,		/* From:, Reply-To: and Organization: field */
	GREF	= 4096,		/* References: field */
	GDATE	= 8192,		/* Date: field */
	GFULL	= 16384,	/* include full names */
	GSKIN	= 32768,	/* skin names */
	GEXTRA	= 65536, 	/* extra fields */
	GFILES	= 131072	/* include filename addresses */
};

#define	GMASK	(GTO|GSUBJECT|GCC|GBCC)	/* Mask of places from whence */

#define	visible(mp)	(((mp)->m_flag&(MDELETED|MHIDDEN|MKILL))==0|| \
				dot==(mp) && (mp)->m_flag&MKILL)

/*
 * Structure used to pass about the current
 * state of the user-typed message header.
 */

struct header {
	struct name *h_to;		/* Dynamic "To:" string */
	char *h_subject;		/* Subject string */
	struct name *h_cc;		/* Carbon copies string */
	struct name *h_bcc;		/* Blind carbon copies */
	struct name *h_ref;		/* References */
	struct name *h_smopts;		/* Sendmail options */
	struct attachment *h_attach;	/* MIME attachments */
	char	*h_charset;		/* preferred charset */
	struct name *h_from;		/* overridden "From:" field */
	struct name *h_replyto;		/* overridden "Reply-To:" field */
	struct name *h_sender;		/* overridden "Sender:" field */
	char *h_organization;		/* overridden "Organization:" field */
};

/*
 * Structure of namelist nodes used in processing
 * the recipients of mail and aliases and all that
 * kind of stuff.
 */

struct name {
	struct	name *n_flink;		/* Forward link in list. */
	struct	name *n_blink;		/* Backward list link */
	enum gfield	n_type;		/* From which list it came */
	char	*n_name;		/* This fella's name */
	char	*n_fullname;		/* Sometimes, name including comment */
};

/*
 * Structure of a MIME attachment.
 */

struct attachment {
	struct attachment *a_flink;	/* Forward link in list. */
	struct attachment *a_blink;	/* Backward list link */
	char	*a_name;		/* file name */
	char	*a_content_type;	/* content type */
	char	*a_content_disposition;	/* content disposition */
	char	*a_content_id;		/* content id */
	char	*a_content_description;	/* content description */
	char	*a_charset;		/* character set */
	int	a_msgno;		/* message number */
};

/*
 * Structure of a variable node.  All variables are
 * kept on a singly-linked list of these, rooted by
 * "variables"
 */

struct var {
	struct	var *v_link;		/* Forward link to next variable */
	char	*v_name;		/* The variable's name */
	char	*v_value;		/* And it's current value */
};

struct group {
	struct	group *ge_link;		/* Next person in this group */
	char	*ge_name;		/* This person's user name */
};

struct grouphead {
	struct	grouphead *g_link;	/* Next grouphead in list */
	char	*g_name;		/* Name of this group */
	struct	group *g_list;		/* Users in group. */
};

/*
 * Structure of the hash table of ignored header fields
 */
struct ignoretab {
	int i_count;			/* Number of entries */
	struct ignore {
		struct ignore *i_link;	/* Next ignored field in bucket */
		char *i_field;		/* This ignored field */
	} *i_head[HSHSIZE];
};

/*
 * Token values returned by the scanner used for argument lists.
 * Also, sizes of scanner-related things.
 */
enum ltoken {
	TEOL	= 0,			/* End of the command line */
	TNUMBER	= 1,			/* A message number */
	TDASH	= 2,			/* A simple dash */
	TSTRING	= 3,			/* A string (possibly containing -) */
	TDOT	= 4,			/* A "." */
	TUP	= 5,			/* An "^" */
	TDOLLAR	= 6,			/* A "$" */
	TSTAR	= 7,			/* A "*" */
	TOPEN	= 8,			/* An '(' */
	TCLOSE	= 9,			/* A ')' */
	TPLUS	= 10,			/* A '+' */
	TERROR	= 11,			/* A lexical error */
	TCOMMA	= 12,			/* A ',' */
	TSEMI	= 13,			/* A ';' */
	TBACK	= 14			/* A '`' */
};

#define	REGDEP	2			/* Maximum regret depth. */

/*
 * Constants for conditional commands.  These describe whether
 * we should be executing stuff or not.
 */
enum condition {
	CANY	= 0,	/* Execute in send or receive mode */
	CRCV	= 1,	/* Execute in receive mode only */
	CSEND	= 2,	/* Execute in send mode only */
	CTERM	= 3,	/* Execute only if stdin is a tty */
	CNONTERM= 4	/* Execute only if stdin not tty */
};

/*
 * For the 'shortcut' and 'unshortcut' functionality.
 */
struct shortcut {
	struct shortcut	*sh_next;	/* next shortcut in list */
	char	*sh_short;		/* shortcut string */
	char	*sh_long;		/* expanded form */
};

/*
 * Kludges to handle the change from setexit / reset to setjmp / longjmp
 */

#define	setexit()	sigsetjmp(srbuf, 1)
#define	reset(x)	siglongjmp(srbuf, x)

/*
 * Locale-independent character classes.
 */
enum {
	C_CNTRL	= 0000,
	C_BLANK	= 0001,
	C_WHITE = 0002,
	C_SPACE	= 0004,
	C_PUNCT	= 0010,
	C_OCTAL	= 0020,
	C_DIGIT	= 0040,
	C_UPPER	= 0100,
	C_LOWER	= 0200
};

extern const unsigned char	class_char[];

#define	asciichar(c) ((unsigned)(c) <= 0177)
#define	alnumchar(c) (asciichar(c)&&(class_char[c]&\
			(C_DIGIT|C_OCTAL|C_UPPER|C_LOWER)))
#define	alphachar(c) (asciichar(c)&&(class_char[c]&(C_UPPER|C_LOWER)))
#define	blankchar(c) (asciichar(c)&&(class_char[c]&(C_BLANK)))
#define	cntrlchar(c) (asciichar(c)&&(class_char[c]==C_CNTRL))
#define	digitchar(c) (asciichar(c)&&(class_char[c]&(C_DIGIT|C_OCTAL)))
#define	lowerchar(c) (asciichar(c)&&(class_char[c]&(C_LOWER)))
#define	punctchar(c) (asciichar(c)&&(class_char[c]&(C_PUNCT)))
#define	spacechar(c) (asciichar(c)&&(class_char[c]&(C_BLANK|C_SPACE|C_WHITE)))
#define	upperchar(c) (asciichar(c)&&(class_char[c]&(C_UPPER)))
#define	whitechar(c) (asciichar(c)&&(class_char[c]&(C_BLANK|C_WHITE)))
#define	octalchar(c) (asciichar(c)&&(class_char[c]&(C_OCTAL)))

#define	upperconv(c) (lowerchar(c) ? (c)-'a'+'A' : (c))
#define	lowerconv(c) (upperchar(c) ? (c)-'A'+'a' : (c))
/*	RFC 822, 3.2.	*/
#define	fieldnamechar(c) (asciichar(c)&&(c)>040&&(c)!=0177&&(c)!=':')

/*
 * Truncate a file to the last character written. This is
 * useful just before closing an old file that was opened
 * for read/write.
 */
#define trunc(stream) {							\
	fflush(stream); 						\
	ftruncate(fileno(stream), (off_t)ftell(stream));		\
}

/*
 * Use either alloca() or smalloc()/free(). ac_alloc can be used to
 * allocate space within a function. ac_free must be called when the
 * space is no longer needed, but expands to nothing if using alloca().
 */
#ifdef	HAVE_ALLOCA
#define	ac_alloc(n)	alloca(n)
#define	ac_free(n)
#else	/* !HAVE_ALLOCA */
#define	ac_alloc(n)	smalloc(n)
#define	ac_free(n)	free(n)
#endif	/* !HAVE_ALLOCA */

/*
 * glibc uses the slow thread-safe getc() even if _REENTRANT is not
 * defined. Work around it.
 */
#ifdef	__GLIBC__
#undef	getc
#define	getc(c)		getc_unlocked(c)
#undef	putc
#define	putc(c, f)	putc_unlocked(c, f)
#undef	putchar
#define	putchar(c)	putc_unlocked((c), stdout)
#endif	/* __GLIBC__ */

#define	CBAD		(-15555)

#define	smin(a, b)	((a) < (b) ? (a) : (b))
#define	smax(a, b)	((a) < (b) ? (b) : (a))

/*
 * For saving the current directory and later returning.
 */
#ifdef	HAVE_FCHDIR
struct	cw {
	int	cw_fd;
};
#else	/* !HAVE_FCHDIR */
struct	cw {
	char	cw_wd[PATHSIZE];
};
#endif	/* !HAVE_FCHDIR */

#ifdef	USE_SSL
enum ssl_vrfy_level {
	VRFY_IGNORE,
	VRFY_WARN,
	VRFY_ASK,
	VRFY_STRICT
};
#endif	/* USE_SSL */

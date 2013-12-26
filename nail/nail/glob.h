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
 *	Sccsid @(#)glob.h	2.27 (gritter) 6/16/07
 */

/*
 * A bunch of global variable declarations lie herein.
 * def.h must be included first.
 */

#if defined(_MAIL_GLOBS_)
#  undef  _E
#  define _E
#else
#  define _E	extern
#endif

_E int	msgCount;			/* Count of messages read in */
_E int	rcvmode;			/* True if receiving mail */
_E int	sawcom;				/* Set after first command */
_E int	Iflag;				/* -I show Newsgroups: field */
_E char	*Tflag;				/* -T temp file for netnews */
_E int	senderr;			/* An error while checking */
_E int	edit;				/* Indicates editing a file */
_E int	noreset;			/* String resets suspended */
_E int	sourcing;			/* Currently reading variant file */
_E int	loading;			/* Loading user definitions */
_E enum condition	cond;		/* Current state of conditional exc. */
_E struct mailbox mb;			/* Current mailbox */
_E int	image;				/* File descriptor for image of msg */
_E FILE	*input;				/* Current command input file */
_E char	mailname[PATHSIZE];		/* Name of current file */
_E char	mboxname[PATHSIZE];		/* Name of mbox */
_E char	prevfile[PATHSIZE];		/* Name of previous file */
_E char	*homedir;			/* Path name of home directory */
_E char *progname;			/* our name */
_E char	*myname;			/* My login name */
extern const char *version;		/* version string */
_E off_t mailsize;			/* Size of system mailbox */
_E struct message *dot;			/* Pointer to current message */
_E struct message *prevdot;		/* Previous current message */
_E struct message *message;		/* The actual message structure */
_E struct message *threadroot;		/* first threaded message */
_E int msgspace;			/* Number of allocated struct m */
_E struct var *variables[HSHSIZE];	/* Pointer to active var list */
_E struct grouphead *groups[HSHSIZE];	/* Pointer to active groups */
_E struct ignoretab ignore[2];		/* ignored and retained fields
					   0 is ignore, 1 is retain */
_E struct ignoretab saveignore[2];	/* ignored and retained fields
					   on save to folder */
_E struct ignoretab allignore[2];	/* special, ignore all headers */
_E struct ignoretab fwdignore[2];	/* fields to ignore for forwarding */
_E char	**altnames;			/* List of alternate names for user */
_E int	debug;				/* Debug flag set */
_E int	scrnwidth;			/* Screen width, or best guess */
_E int	scrnheight;			/* Screen height, or best guess,
					   for "header" command */
_E int	realscreenheight;		/* the real screen height */
_E gid_t	effectivegid;		/* Saved from when we started up */
_E gid_t	realgid;		/* Saved from when we started up */
_E int	exit_status;			/* Exit status */
_E int	is_a_tty[2];			/* isatty(0), isatty(1) */
_E int	did_print_dot;			/* current message has been printed */
_E int	tildeflag;			/* enable tilde escapes */
_E char	*uflag;				/* name given with -u option */
_E struct shortcut	*shortcuts;	/* list of shortcuts */
_E int	mb_cur_max;			/* value of MB_CUR_MAX */
_E int	imap_created_mailbox;		/* hack to get feedback from imap */
_E int	unset_allow_undefined;		/* allow to unset undefined variables */
_E int	inhook;				/* currently executing a hook */
_E int	starting;			/* still in startup code */
_E char *wantcharset;			/* overrides the "charset" variable */
_E int	utf8;				/* UTF-8 encoding in use for locale */
_E int	Rflag;				/* open all folders read-only */

#ifdef	USE_SSL
_E enum ssl_vrfy_level	ssl_vrfy_level;	/* SSL verification level */
#endif

#ifdef	HAVE_ICONV
_E iconv_t iconvd;
#endif

#ifdef	HAVE_CATGETS
_E nl_catd	catd;
#endif

/*
 * These are initialized strings.
 */
extern char *us_ascii;			/* "us-ascii" */
extern const char *month_names[];

#include <setjmp.h>

_E sigjmp_buf	srbuf;
_E int		interrupts;
_E sighandler_type	handlerstacktop;
#define	handlerpush(f)	(savedtop = handlerstacktop, handlerstacktop = (f))
#define	handlerpop()	(handlerstacktop = savedtop)
extern sighandler_type	dflpipe;

/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */

#define	NSPACE	25			/* Total number of string spaces */
_E struct strings {
	char	*s_topFree;		/* Beginning of this area */
	char	*s_nextFree;		/* Next alloctable place here */
	unsigned s_nleft;		/* Number of bytes left here */
} stringdope[NSPACE];

#undef  _E

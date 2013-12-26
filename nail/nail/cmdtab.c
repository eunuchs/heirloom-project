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
static char sccsid[] = "@(#)cmdtab.c	2.51 (gritter) 3/4/06";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Define all of the command names and bindings.
 */

const struct cmd cmdtab[] = {
	{ "next",	next,		A|NDMLIST,	0,	MMNDEL },
	{ "alias",	group,		M|RAWLIST,	0,	1000 },
	{ "print",	type,		A|MSGLIST,	0,	MMNDEL },
	{ "type",	type,		A|MSGLIST,	0,	MMNDEL },
	{ "Type",	Type,		A|MSGLIST,	0,	MMNDEL },
	{ "Print",	Type,		A|MSGLIST,	0,	MMNDEL },
	{ "visual",	visual,		A|I|MSGLIST,	0,	MMNORM },
	{ "top",	top,		A|MSGLIST,	0,	MMNDEL },
	{ "touch",	stouch,		A|W|MSGLIST,	0,	MMNDEL },
	{ "preserve",	preserve,	A|W|MSGLIST,	0,	MMNDEL },
	{ "delete",	delete,		A|W|P|MSGLIST,	0,	MMNDEL },
	{ "dp",		deltype,	A|W|MSGLIST,	0,	MMNDEL },
	{ "dt",		deltype,	A|W|MSGLIST,	0,	MMNDEL },
	{ "undelete",	undeletecmd,	A|P|MSGLIST,	MDELETED,MMNDEL },
	{ "unset",	unset,		M|RAWLIST,	1,	1000 },
	{ "mail",	sendmail,	R|M|I|STRLIST,	0,	0 },
	{ "Mail",	Sendmail,	R|M|I|STRLIST,	0,	0 },
	{ "mbox",	mboxit,		A|W|MSGLIST,	0,	0 },
	{ "more",	more,		A|MSGLIST,	0,	MMNDEL },
	{ "page",	more,		A|MSGLIST,	0,	MMNDEL },
	{ "More",	More,		A|MSGLIST,	0,	MMNDEL },
	{ "Page",	More,		A|MSGLIST,	0,	MMNDEL },
	{ "unread",	unread,		A|MSGLIST,	0,	MMNDEL },
	{ "Unread",	unread,		A|MSGLIST,	0,	MMNDEL },
	{ "new",	unread,		A|MSGLIST,	0,	MMNDEL },
	{ "New",	unread,		A|MSGLIST,	0,	MMNDEL },
	{ "!",		shell,		I|STRLIST,	0,	0 },
	{ "copy",	copycmd,	A|M|STRLIST,	0,	0 },
	{ "Copy",	Copycmd,	A|M|STRLIST,	0,	0 },
	{ "chdir",	schdir,		M|RAWLIST,	0,	1 },
	{ "cd",		schdir,		M|RAWLIST,	0,	1 },
	{ "save",	save,		A|STRLIST,	0,	0 },
	{ "Save",	Save,		A|STRLIST,	0,	0 },
	{ "source",	source,		M|RAWLIST,	1,	1 },
	{ "set",	set,		M|RAWLIST,	0,	1000 },
	{ "shell",	dosh,		I|NOLIST,	0,	0 },
	{ "version",	pversion,	M|NOLIST,	0,	0 },
	{ "group",	group,		M|RAWLIST,	0,	1000 },
	{ "ungroup",	ungroup,	M|RAWLIST,	0,	1000 },
	{ "unalias",	ungroup,	M|RAWLIST,	0,	1000 },
	{ "write",	cwrite,		A|STRLIST,	0,	0 },
	{ "from",	from,		A|MSGLIST,	0,	MMNORM },
	{ "file",	cfile,		T|M|RAWLIST,	0,	1 },
	{ "followup",	followup,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "followupall", followupall,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "followupsender", followupsender, A|R|I|MSGLIST, 0,	MMNDEL },
	{ "folder",	cfile,		T|M|RAWLIST,	0,	1 },
	{ "folders",	folders,	T|M|RAWLIST,	0,	1 },
	{ "?",		help,		M|NOLIST,	0,	0 },
	{ "z",		scroll,		A|M|STRLIST,	0,	0 },
	{ "Z",		Scroll,		A|M|STRLIST,	0,	0 },
	{ "headers",	headers,	A|MSGLIST,	0,	MMNDEL },
	{ "help",	help,		M|NOLIST,	0,	0 },
	{ "=",		pdot,		A|NOLIST,	0,	0 },
	{ "Reply",	Respond,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "Respond",	Respond,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "Followup",	Followup,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "reply",	respond,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "replyall",	respondall,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "replysender", respondsender,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "respond",	respond,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "respondall",	respondall,	A|R|I|MSGLIST,	0,	MMNDEL },
	{ "respondsender", respondsender, A|R|I|MSGLIST,0,	MMNDEL },
	{ "Resend",	Resendcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "Redirect",	Resendcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "resend",	resendcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "redirect",	resendcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "Forward",	Forwardcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "Fwd",	Forwardcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "forward",	forwardcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "fwd",	forwardcmd,	A|R|STRLIST,	0,	MMNDEL },
	{ "edit",	editor,		A|I|MSGLIST,	0,	MMNORM },
	{ "echo",	echo,		M|ECHOLIST,	0,	1000 },
	{ "quit",	quitcmd,	NOLIST,		0,	0 },
	{ "list",	pcmdlist,	M|NOLIST,	0,	0 },
	{ "xit",	rexit,		M|NOLIST,	0,	0 },
	{ "exit",	rexit,		M|NOLIST,	0,	0 },
	{ "pipe",	pipecmd,	A|STRLIST,	0,	MMNDEL },
	{ "|",		pipecmd,	A|STRLIST,	0,	MMNDEL },
	{ "Pipe",	Pipecmd,	A|STRLIST,	0,	MMNDEL },
	{ "size",	messize,	A|MSGLIST,	0,	MMNDEL },
	{ "hold",	preserve,	A|W|MSGLIST,	0,	MMNDEL },
	{ "if",		ifcmd,		F|M|RAWLIST,	1,	1 },
	{ "else",	elsecmd,	F|M|RAWLIST,	0,	0 },
	{ "endif",	endifcmd,	F|M|RAWLIST,	0,	0 },
	{ "alternates",	alternates,	M|RAWLIST,	0,	1000 },
	{ "ignore",	igfield,	M|RAWLIST,	0,	1000 },
	{ "discard",	igfield,	M|RAWLIST,	0,	1000 },
	{ "retain",	retfield,	M|RAWLIST,	0,	1000 },
	{ "saveignore",	saveigfield,	M|RAWLIST,	0,	1000 },
	{ "savediscard",saveigfield,	M|RAWLIST,	0,	1000 },
	{ "saveretain",	saveretfield,	M|RAWLIST,	0,	1000 },
	{ "unignore",	unignore,	M|RAWLIST,	0,	1000 },
	{ "unretain",	unretain,	M|RAWLIST,	0,	1000 },
	{ "unsaveignore", unsaveignore,	M|RAWLIST,	0,	1000 },
	{ "unsaveretain", unsaveretain,	M|RAWLIST,	0,	1000 },
	{ "inc",	newmail,	A|T|NOLIST,	0,	0 },
	{ "newmail",	newmail,	A|T|NOLIST,	0,	0 },
	{ "shortcut",	shortcut,	M|RAWLIST,	0,	1000 },
	{ "unshortcut",	unshortcut,	M|RAWLIST,	0,	1000 },
	{ "imap",	imap_imap,	A|STRLIST,	0,	1000 },
	{ "account",	account,	M|RAWLIST,	0,	1000 },
	{ "thread",	thread,		A|MSGLIST,	0,	0 },
	{ "unthread",	unthread,	A|MSGLIST,	0,	0 },
	{ "online",	cconnect,	A|NOLIST,	0,	0 },
	{ "connect",	cconnect,	A|NOLIST,	0,	0 },
	{ "disconnect",	cdisconnect,	A|NDMLIST,	0,	0 },
	{ "sort",	sort,		A|RAWLIST,	0,	1 },
	{ "unsort",	unthread,	A|MSGLIST,	0,	0 },
	{ "cache",	ccache,		A|MSGLIST,	0,	0 },
	{ "flag",	cflag,		A|M|MSGLIST,	0,	0 },
	{ "unflag",	cunflag,	A|M|MSGLIST,	0,	0 },
	{ "answered",	canswered,	A|M|MSGLIST,	0,	0 },
	{ "unanswered",	cunanswered,	A|M|MSGLIST,	0,	0 },
	{ "draft",	cdraft,		A|M|MSGLIST,	0,	0 },
	{ "undraft",	cundraft,	A|M|MSGLIST,	0,	0 },
	{ "kill",	ckill,		A|M|MSGLIST,	0,	0 },
	{ "unkill",	cunkill,	A|M|MSGLIST,	0,	0 },
	{ "score",	cscore,		A|M|STRLIST,	0,	0 },
	{ "define",	cdefine,	M|RAWLIST,	0,	2 },
	{ "defines",	cdefines,	M|RAWLIST,	0,	0 },
	{ "undef",	cundef,		M|RAWLIST,	0,	1000 },
	{ "call",	ccall,		M|RAWLIST,	0,	1 },
	{ "move",	cmove,		A|M|STRLIST,	0,	0 },
	{ "mv",		cmove,		A|M|STRLIST,	0,	0 },
	{ "Move",	cMove,		A|M|STRLIST,	0,	0 },
	{ "Mv",		cMove,		A|M|STRLIST,	0,	0 },
	{ "noop",	cnoop,		A|M|RAWLIST,	0,	0 },
	{ "collapse",	ccollapse,	A|MSGLIST,	0,	0 },
	{ "uncollapse",	cuncollapse,	A|MSGLIST,	0,	0 },
	{ "verify",	cverify,	A|MSGLIST,	0,	0 },
	{ "decrypt",	cdecrypt,	A|M|STRLIST,	0,	0 },
	{ "Decrypt",	cDecrypt,	A|M|STRLIST,	0,	0 },
	{ "certsave",	ccertsave,	A|STRLIST,	0,	0 },
	{ "rename",	crename,	M|RAWLIST,	0,	2 },
	{ "remove",	cremove,	M|RAWLIST,	0,	1000 },
	{ "classify",	cclassify,	A|M|MSGLIST,	0,	0 },
	{ "junk",	cjunk,		A|M|MSGLIST,	0,	0 },
	{ "bad",	cjunk,		A|M|MSGLIST,	0,	0 },
	{ "good",	cgood,		A|M|MSGLIST,	0,	0 },
	{ "unjunk",	cunjunk,	A|M|MSGLIST,	0,	0 },
	{ "ungood",	cungood,	A|M|MSGLIST,	0,	0 },
	{ "probability",cprobability,	M|RAWLIST,	0,	1000 },
	{ "show",	show,		A|MSGLIST,	0,	MMNDEL },
	{ "Show",	show,		A|MSGLIST,	0,	MMNDEL },
	{ "seen",	seen,		A|M|MSGLIST,	0,	MMNDEL },
	{ "Seen",	seen,		A|M|MSGLIST,	0,	MMNDEL },
	{ "fwdignore",	fwdigfield,	M|RAWLIST,	0,	1000 },
	{ "fwddiscard",fwdigfield,	M|RAWLIST,	0,	1000 },
	{ "fwdretain",	fwdretfield,	M|RAWLIST,	0,	1000 },
	{ "unfwdignore", unfwdignore,	M|RAWLIST,	0,	1000 },
	{ "unfwdretain", unfwdretain,	M|RAWLIST,	0,	1000 },
/*	{ "Header",	Header,		STRLIST,	0,	1000 },	*/
#ifdef	DEBUG_COMMANDS
	{ "core",	core,		M|NOLIST,	0,	0 },
	{ "clobber",	clobber,	M|RAWLIST,	0,	1 },
#endif	/* DEBUG_COMMANDS */
	{ 0,		0,		0,		0,	0 }
};

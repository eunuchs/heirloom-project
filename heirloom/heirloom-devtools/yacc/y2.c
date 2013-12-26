/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "y2.c	6.35	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)y2.c	1.11 (gritter) 11/26/05
 */

#include "dextern"
#include "sgs.h"
#include <wchar.h>
#include <unistd.h>
#define	IDENTIFIER 257

#define	MARK 258
#define	TERM 259
#define	LEFT 260
#define	RIGHT 261
#define	BINARY 262
#define	PREC 263
#define	LCURLY 264
#define	C_IDENTIFIER 265  /* name followed by colon */
#define	NUMBER 266
#define	START 267
#define	TYPEDEF 268
#define	TYPENAME 269
#define	UNION 270
#define	ENDFILE 0
#define	LHS_TEXT_LEN		80	/* length of lhstext */
#define	RHS_TEXT_LEN		640	/* length of rhstext */
	/* communication variables between various I/O routines */

#define	v_FLAG	0x01
#define	d_FLAG	0x02
#define	DEFAULT_PREFIX	"y"

char *infile;				/* input file name 		*/
static int numbval;			/* value of an input number 	*/
static int toksize = NAMESIZE;
static wchar_t *tokname;	/* input token name		*/
char *parser = NULL;		/* location of common parser 	*/

static void finact(void);
static wchar_t *cstash(wchar_t *);
static void defout(void);
static void cpyunion(void);
static void cpycode(void);
static void cpyact(int);
static void lhsfill(wchar_t *);
static void rhsfill(wchar_t *);
static void lrprnt(void);
static void beg_debug(void);
static void end_toks(void);
static void end_debug(void);
static void exp_tokname(void);
static void exp_prod(void);
static void exp_ntok(void);
static void exp_nonterm(void);
static int defin(int, wchar_t *);
static int gettok(void);
static int chfind(int, wchar_t *);
static int skipcom(void);
static int findchtok(int);
static void put_prefix_define(char *);


/* storage of names */

/*
 * initial block to place token and
 * nonterminal names are stored
 * points to initial block - more space
 * is allocated as needed.
 */
static wchar_t cnamesblk0[CNAMSZ];
static wchar_t *cnames = cnamesblk0;

/* place where next name is to be put in */
static wchar_t *cnamp = cnamesblk0;

/* number of defined symbols output */
static int ndefout = 3;

	/* storage of types */
static int defunion = 0;	/* union of types defined? */
static int ntypes = 0;		/* number of types defined */
static wchar_t *typeset[NTYPES]; /* pointers to type tags */

	/* symbol tables for tokens and nonterminals */

int ntokens = 0;
int ntoksz = NTERMS;
TOKSYMB *tokset;
int *toklev;

int nnonter = -1;
NTSYMB *nontrst;
int nnontersz = NNONTERM;

static int start;	/* start symbol */

	/* assigned token type values */
static int extval = 0;

	/* input and output file descriptors */

FILE *finput;		/* yacc input file */
FILE *faction;		/* file for saving actions */
FILE *fdefine;		/* file for # defines */
FILE *ftable;		/* y.tab.c file */
FILE *ftemp;		/* tempfile to pass 2 */
FILE *fdebug;		/* where the strings for debugging are stored */
FILE *foutput;		/* y.output file */

	/* output string */

static wchar_t *lhstext;
static wchar_t *rhstext;

	/* storage for grammar rules */

int *mem0; /* production storage */
int *mem;
int *tracemem;
extern int *optimmem;
int new_memsize = MEMSIZE;
int nprod = 1;	/* number of productions */
int nprodsz = NPROD;

int **prdptr;
int *levprd;
wchar_t *had_act;

/* flag for generating the # line's default is yes */
int gen_lines = 1;
int act_lines = 0;

/* flag for whether to include runtime debugging */
static int gen_testing = 0;

/* flag for version stamping--default turned off */
static char *v_stmp = "n";

int nmbchars = 0;	/* number of mb literals in mbchars */
MBCLIT *mbchars = (MBCLIT *) 0; /* array of mb literals */
int nmbcharsz = 0; /* allocated space for mbchars */

void 
setup(int argc, char *argv[])
{	int ii, i, j, lev, t, ty;
		/* ty is the sequencial number of token name in tokset */
	int c;
	int *p;
	char *cp;
	wchar_t actname[8];
	unsigned int options = 0;
	char *file_prefix = DEFAULT_PREFIX;
	char *sym_prefix = "";
#define	F_NAME_LENGTH	4096
	char	fname[F_NAME_LENGTH+1];

	foutput = NULL;
	fdefine = NULL;
	i = 1;

	tokname = malloc(sizeof (wchar_t) * toksize);
	tokset = malloc(sizeof (TOKSYMB) * ntoksz);
	toklev = malloc(sizeof (int) * ntoksz);
	nontrst = malloc(sizeof (NTSYMB) * nnontersz);
	mem0 = malloc(sizeof (int) * new_memsize);
	prdptr = malloc(sizeof (int *) * (nprodsz+2));
	levprd = malloc(sizeof (int) * (nprodsz+2));
	had_act = calloc(nprodsz + 2, sizeof (wchar_t));
	lhstext = malloc(sizeof (wchar_t) * LHS_TEXT_LEN);
	rhstext = malloc(sizeof (wchar_t) * RHS_TEXT_LEN);
	aryfil(toklev, ntoksz, 0);
	aryfil(levprd, nprodsz, 0);
	for (ii = 0; ii < ntoksz; ++ii)
		tokset[ii].value = 0;
	for (ii = 0; ii < nnontersz; ++ii)
		nontrst[ii].tvalue = 0;
	aryfil(mem0, new_memsize, 0);
	mem = mem0;
	tracemem = mem0;

	while ((c = getopt(argc, argv, "vVdltp:Q:Y:P:b:")) != EOF)
		switch (c) {
		case 'v':
			options |= v_FLAG;
			break;
		case 'V':
			fprintf(stderr, "yacc: %s , %s\n", pkg, rel);
			break;
		case 'Q':
			v_stmp = optarg;
			if (*v_stmp != 'y' && *v_stmp != 'n')
				error("yacc: -Q should be followed by [y/n]");
			break;
		case 'd':
			options |= d_FLAG;
			break;
		case 'l':
			gen_lines = 0;	/* don't gen #lines */
			break;
		case 't':
			gen_testing = 1;	/* set YYDEBUG on */
			break;
		case 'Y':
			cp = malloc(strlen(optarg) + sizeof ("/yaccpar") + 1);
			cp = strcpy(cp, optarg);
			parser = strcat(cp, "/yaccpar");
			break;
		case 'P':
			parser = optarg;
			break;
		case 'p':
			if (strcmp(optarg, "yy") != 0)
				sym_prefix = optarg;
			else
				sym_prefix = "";
			break;
		case 'b':
			file_prefix = optarg;
			break;
		case '?':
		default:
			fprintf(stderr,
"Usage: yacc [-vVdlt] [-Q(y/n)] [-P driver_file] file\n");
			exit(1);
		}
	/*
	 * Open y.output if -v is specified
	 */
	if (options & v_FLAG) {
		strncpy(fname,
			file_prefix,
			F_NAME_LENGTH-strlen(".output"));
		strcat(fname, ".output");
		foutput = fopen(fname, "w");
		if (foutput == NULL)
			error("cannot open y.output");
	}

	/*
	 * Open y.tab.h if -d is specified
	 */
	if (options & d_FLAG) {
		strncpy(fname,
			file_prefix,
			F_NAME_LENGTH-strlen(".tab.h"));
		strcat(fname, ".tab.h");
		fdefine = fopen(fname, "w");
		if (fdefine == NULL)
			error("cannot open y.tab.h");
	}

	fdebug = fopen(DEBUGNAME, "w");
	if (fdebug == NULL)
		error("cannot open yacc.debug");
	/*
	 * Open y.tab.c
	 */
	strncpy(fname,
		file_prefix,
		F_NAME_LENGTH-strlen(".tab.c"));
	strcat(fname, ".tab.c");
	ftable = fopen(fname, "w");
	if (ftable == NULL)
		error("cannot open %s", fname);

	ftemp = fopen(TEMPNAME, "w");
	faction = fopen(ACTNAME, "w");
	if (ftemp == NULL || faction == NULL)
		error("cannot open temp file");

	if ((finput = fopen(infile = argv[optind], "r")) == NULL)
		error("cannot open input file");

	lineno = 1;
	cnamp = cnames;
	defin(0, L"$end");
	extval = 0400;
	defin(0, L"error");
	defin(1, L"$accept");
	mem = mem0;
	lev = 0;
	ty = 0;
	i = 0;
	beg_debug();	/* initialize fdebug file */

	/*
	 * sorry -- no yacc parser here.....
	 *	we must bootstrap somehow...
	 */

	t = gettok();
	if (*v_stmp == 'y')
		fprintf(ftable, "\
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4\n\
#define	YYUSED	__attribute__ ((used))\n\
#elif defined __GNUC__\n\
#define	YYUSED	__attribute__ ((unused))\n\
#else\n\
#define	YYUSED\n\
#endif\n\
static const char yyident[] USED = \"yacc: %s\"\n", rel);
	for (; t != MARK && t != ENDFILE; ) {
		int tok_in_line;
		switch (t) {

		case L';':
			t = gettok();
			break;

		case START:
			if ((t = gettok()) != IDENTIFIER) {
				error("bad %%start construction");
				}
			start = chfind(1, tokname);
			t = gettok();
			continue;

		case TYPEDEF:
			tok_in_line = 0;
			if ((t = gettok()) != TYPENAME)
				error("bad syntax in %%type");
			ty = numbval;
			for (;;) {
				t = gettok();
				switch (t) {

				case IDENTIFIER:
			/*
			 * The following lines are idented to left.
			 */
			tok_in_line = 1;
			if ((t = chfind(1, tokname)) < NTBASE) {
				j = TYPE(toklev[t]);
				if (j != 0 && j != ty) {
					error("type redeclaration of token %ls",
					tokset[t].name);
					}
				else
					SETTYPE(toklev[t], ty);
			} else {
				j = nontrst[t-NTBASE].tvalue;
				if (j != 0 && j != ty) {
					error(
					"type redeclaration of nonterminal %ls",
						nontrst[t-NTBASE].name);
					}
				else
					nontrst[t-NTBASE].tvalue = ty;
				}
			/* FALLTHRU */
			/*
			 * End Indentation
			 */
				case L',':
					continue;

				case L';':
					t = gettok();
					break;
				default:
					break;
					}
				if (!tok_in_line)
					error(
					"missing tokens or illegal tokens");
				break;
				}
			continue;

		case UNION:
			/* copy the union declaration to the output */
			cpyunion();
			defunion = 1;
			t = gettok();
			continue;

		case LEFT:
		case BINARY:
		case RIGHT:
			i++;
			/* FALLTHRU */
		case TERM:
			tok_in_line = 0;

			/* nonzero means new prec. and assoc. */
			lev = (t-TERM) | 04;
			ty = 0;

			/* get identifiers so defined */

			t = gettok();
			if (t == TYPENAME) { /* there is a type defined */
				ty = numbval;
				t = gettok();
				}

			for (;;) {
				switch (t) {

				case L',':
					t = gettok();
					continue;

				case L';':
					break;

				case IDENTIFIER:
					tok_in_line = 1;
					j = chfind(0, tokname);
					if (j > NTBASE) {
						error("%ls is not a token.",
						tokname);
					}
					if (lev & ~04) {
						if (ASSOC(toklev[j]) & ~04)
							error(
				"redeclaration of precedence of %ls",
						tokname);
						SETASC(toklev[j], lev);
						SETPLEV(toklev[j], i);
					} else {
						if (ASSOC(toklev[j]))
						warning(1,
				"redeclaration of precedence of %ls.",
							tokname);
						SETASC(toklev[j], lev);
						}
					if (ty) {
						if (TYPE(toklev[j]))
							error(
						"redeclaration of type of %ls",
							tokname);
						SETTYPE(toklev[j], ty);
						}
					if ((t = gettok()) == NUMBER) {
						tokset[j].value = numbval;
						if (j < ndefout && j > 2) {
							error(
				"type number of %ls should be defined earlier",
							tokset[j].name);
							}
						if (numbval >= -YYFLAG1) {
							error(
				"token numbers must be less than %d",
							-YYFLAG1);
							}
						t = gettok();
						}
					continue;

					}
				if (!tok_in_line)
					error(
					"missing tokens or illegal tokens");
				break;
				}
			continue;

		case LCURLY:
			defout();
			cpycode();
			t = gettok();
			continue;

		default:
			error("syntax error");

			}

		}

	if (t == ENDFILE) {
		error("unexpected EOF before %%%%");
		}

	/* t is MARK */

	defout();
	end_toks();	/* all tokens dumped - get ready for reductions */

	fprintf(ftable, "\n#ifdef __STDC__\n");
	fprintf(ftable, "#include <stdlib.h>\n");
	fprintf(ftable, "#include <string.h>\n");
	fprintf(ftable, "#define	YYCONST	const\n");
	fprintf(ftable, "#else\n");
	fprintf(ftable, "#include <malloc.h>\n");
	fprintf(ftable, "#include <memory.h>\n");
	fprintf(ftable, "#define	YYCONST\n");
	fprintf(ftable, "#endif\n");

	if (sym_prefix[0] != '\0')
		put_prefix_define(sym_prefix);

	fprintf(ftable, "\n#if defined(__cplusplus) || defined(__STDC__)\n");
	fprintf(ftable,
	"\n#if defined(__cplusplus) && defined(__EXTERN_C__)\n");
	fprintf(ftable, "extern \"C\" {\n");
	fprintf(ftable, "#endif\n");
	fprintf(ftable, "#ifndef yyerror\n");
	fprintf(ftable, "#if defined(__cplusplus)\n");
	fprintf(ftable, "	void yyerror(YYCONST char *);\n");
	fprintf(ftable, "#endif\n");
	fprintf(ftable, "#endif\n");
	fprintf(ftable, "#ifndef yylex\n");
	fprintf(ftable, "	int yylex(void);\n");
	fprintf(ftable, "#endif\n");
	fprintf(ftable, "	int yyparse(void);\n");
	fprintf(ftable, "#if defined(__cplusplus) && defined(__EXTERN_C__)\n");
	fprintf(ftable, "}\n");
	fprintf(ftable, "#endif\n");
	fprintf(ftable, "\n#endif\n\n");

	fprintf(ftable, "#define yyclearin yychar = -1\n");
	fprintf(ftable, "#define yyerrok yyerrflag = 0\n");
	fprintf(ftable, "extern int yychar;\nextern int yyerrflag;\n");
	if (!(defunion || ntypes))
		fprintf(ftable,
			"#ifndef YYSTYPE\n#define YYSTYPE int\n#endif\n");
	fprintf(ftable, "YYSTYPE yylval;\n");
	fprintf(ftable, "YYSTYPE yyval;\n");
	fprintf(ftable, "typedef int yytabelem;\n");
	fprintf(ftable,
		"#ifndef YYMAXDEPTH\n#define YYMAXDEPTH 150\n#endif\n");
	fprintf(ftable, "#if YYMAXDEPTH > 0\n");
	fprintf(ftable, "int yy_yys[YYMAXDEPTH], *yys = yy_yys;\n");
	fprintf(ftable, "YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;\n");
	fprintf(ftable, "#else	/* user does initial allocation */\n");
	fprintf(ftable, "int *yys;\nYYSTYPE *yyv;\n#endif\n");
	fprintf(ftable, "static int yymaxdepth = YYMAXDEPTH;\n");

	prdptr[0] = mem;
	/* added production */
	*mem++ = NTBASE;

	/* if start is 0, we will overwrite with the lhs of the first rule */
	*mem++ = start;
	*mem++ = 1;
	*mem++ = 0;
	prdptr[1] = mem;

	while ((t = gettok()) == LCURLY)
		cpycode();

	if (t != C_IDENTIFIER)
		error("bad syntax on first rule");

	if (!start)
		prdptr[0][1] = chfind(1, tokname);

	/* read rules */

	while (t != MARK && t != ENDFILE) {

		/* process a rule */

		if (t == L'|') {
			rhsfill((wchar_t *)0); /* restart fill of rhs */
			*mem = *prdptr[nprod-1];
			if (++mem >= &tracemem[new_memsize])
				exp_mem(1);
		} else if (t == C_IDENTIFIER) {
			*mem = chfind(1, tokname);
			if (*mem < NTBASE)
				error("illegal nonterminal in grammar rule");
			if (++mem >= &tracemem[new_memsize])
				exp_mem(1);
			lhsfill(tokname);	/* new rule: restart strings */
		} else
			error("illegal rule: missing semicolon or | ?");

		/* read rule body */


		t = gettok();
	more_rule:
		while (t == IDENTIFIER) {
			*mem = chfind(1, tokname);
			if (*mem < NTBASE)
				levprd[nprod] = toklev[*mem]& ~04;
			if (++mem >= &tracemem[new_memsize])
				exp_mem(1);
			rhsfill(tokname);	/* add to rhs string */
			t = gettok();
			}

		if (t == PREC) {
			if (gettok() != IDENTIFIER)
				error("illegal %%prec syntax");
			j = chfind(2, tokname);
			if (j >= NTBASE)
				error("nonterminal %ls illegal after %%prec",
				nontrst[j-NTBASE].name);
			levprd[nprod] = toklev[j] & ~04;
			t = gettok();
			}

		if (t == L'=') {
			had_act[nprod] = 1;
			levprd[nprod] |= ACTFLAG;
			fprintf(faction, "\ncase %d:", nprod);
			cpyact(mem-prdptr[nprod] - 1);
			fprintf(faction, " break;");
			if ((t = gettok()) == IDENTIFIER) {
				/* action within rule... */

				lrprnt();		/* dump lhs, rhs */
				swprintf(actname, sizeof actname,
						L"$$%d", nprod);
				/*
				 * make it nonterminal
				 */
				j = chfind(1, actname);

				/*
				 * the current rule will become rule
				 * number nprod+1 move the contents down,
				 * and make room for the null
				 */

				if (mem + 2 >= &tracemem[new_memsize])
					exp_mem(1);
				for (p = mem; p >= prdptr[nprod]; --p)
					p[2] = *p;
				mem += 2;

				/* enter null production for action */

				p = prdptr[nprod];

				*p++ = j;
				*p++ = -nprod;

				/* update the production information */

				levprd[nprod+1] = levprd[nprod] & ~ACTFLAG;
				levprd[nprod] = ACTFLAG;

				if (++nprod >= nprodsz)
					exp_prod();
				prdptr[nprod] = p;

				/*
				 * make the action appear in
				 * the original rule
				 */
				*mem++ = j;
				if (mem >= &tracemem[new_memsize])
					exp_mem(1);
				/* get some more of the rule */
				goto more_rule;
			}
		}
		while (t == L';')
			t = gettok();
		*mem++ = -nprod;
		if (mem >= &tracemem[new_memsize])
			exp_mem(1);

		/* check that default action is reasonable */

		if (ntypes && !(levprd[nprod] & ACTFLAG) &&
				nontrst[*prdptr[nprod]-NTBASE].tvalue) {
			/* no explicit action, LHS has value */
			register int tempty;
			tempty = prdptr[nprod][1];
			if (tempty < 0)
				error("must return a value, since LHS has a type");
			else if (tempty >= NTBASE)
				tempty = nontrst[tempty-NTBASE].tvalue;
			else
				tempty = TYPE(toklev[tempty]);
			if (tempty != nontrst[*prdptr[nprod]-NTBASE].tvalue) {
				error(
				"default action causes potential type clash");
			}
		}

		if (++nprod >= nprodsz)
			exp_prod();
		prdptr[nprod] = mem;
		levprd[nprod] = 0;
		}
	/* end of all rules */

	end_debug();		/* finish fdebug file's input */
	finact();
	if (t == MARK) {
		if (gen_lines)
			fprintf(ftable, "\n# line %d \"%s\"\n",
				lineno, infile);
		while ((c = getwc(finput)) != EOF)
			putwc(c, ftable);
		}
	fclose(finput);
}

static void 
finact(void)
{
	/* finish action routine */
	fclose(faction);
	fprintf(ftable, "# define YYERRCODE %d\n", tokset[2].value);
}

static wchar_t *
cstash(s)
register wchar_t *s;
{
	wchar_t *temp;
	static int used = 0;
	static int used_save = 0;
	static int exp_cname = CNAMSZ;
	int len = wcslen(s);

	/*
	 * 2/29/88 -
	 * Don't need to expand the table, just allocate new space.
	 */
	used_save = used;
	while (len >= (exp_cname - used_save)) {
		exp_cname += CNAMSZ;
		if (!used)
			free(cnames);
		if ((cnames = malloc(sizeof (wchar_t)*exp_cname)) == NULL)
			error("cannot expand string dump");
		cnamp = cnames;
		used = 0;
	}

	temp = cnamp;
	do {
		*cnamp++ = *s;
	} while (*s++);
	used += cnamp - temp;
	return (temp);
}

static int
defin(int t, register wchar_t *s)
{
	/* define s to be a terminal if t=0 or a nonterminal if t=1 */

	register int val = 0;

	if (t) {
		if (++nnonter >= nnontersz)
			exp_nonterm();
		nontrst[nnonter].name = cstash(s);
		return (NTBASE + nnonter);
		}
	/* must be a token */
	if (++ntokens >= ntoksz)
		exp_ntok();
	tokset[ntokens].name = cstash(s);

	/* establish value for token */

	if (s[0] == L' ' && s[2] == 0) { /* single character literal */
		val = findchtok(s[1]);
	} else if (s[0] == L' ' && s[1] == L'\\') { /* escape sequence */
		if (s[3] == 0) { /* single character escape sequence */
			switch (s[2]) {
				/* character which is escaped */
			case L'a':
				warning(1,
		"\\a is ANSI C \"alert\" character");
#if __STDC__ - 1 == 0
				val = L'\a';
				break;
#else
				val = L'\007';
				break;
#endif
			case L'v': val = L'\v'; break;
			case L'n': val = L'\n'; break;
			case L'r': val = L'\r'; break;
			case L'b': val = L'\b'; break;
			case L't': val = L'\t'; break;
			case L'f': val = L'\f'; break;
			case L'\'': val = L'\''; break;
			case L'"': val = L'"'; break;
			case L'?': val = L'?'; break;
			case L'\\': val = L'\\'; break;
			default: error("invalid escape");
			}
		} else if (s[2] <= L'7' && s[2] >= L'0') { /* \nnn sequence */
			int i = 3;
			val = s[2] - L'0';
			while (iswdigit(s[i]) && i <= 4) {
				if (s[i] >= L'0' && s[i] <= L'7')
					val = val * 8 + s[i] - L'0';
				else
					error("illegal octal number");
				i++;
			}
			if (s[i] != 0)
				error("illegal \\nnn construction");
			if (val > 255)
				error(
"\\nnn exceed \\377; use \\xnnnnnnnn for wchar_t value of multibyte char");
			if (val == 0 && i >= 4)
				error("'\\000' is illegal");
		} else if (s[2] == L'x') { /* hexadecimal \xnnn sequence */
			int i = 3;
			val = 0;
			warning(1, "\\x is ANSI C hex escape");
			if (iswxdigit(s[i]))
				while (iswxdigit(s[i])) {
					int tmpval;
					if (iswdigit(s[i]))
						tmpval = s[i] - L'0';
					else if (s[i] >= L'a')
						tmpval = s[i] - L'a' + 10;
					else
						tmpval = s[i] - L'A' + 10;
					val = 16 * val + tmpval;
					i++;
				}
			else
				error("illegal hexadecimal number");
			if (s[i] != 0)
				error("illegal \\xnn construction");
#define	LWCHAR_MAX	0x7fffffff
			if ((unsigned)val > LWCHAR_MAX)
			    error(" \\xnnnnnnnn exceed %#x", LWCHAR_MAX);
			if (val == 0)
				error("'\\x00' is illegal");
			val = findchtok(val);
		} else
			error("invalid escape");
	} else {
		val = extval++;
	}
	tokset[ntokens].value = val;
	toklev[ntokens] = 0;
	return (ntokens);
}

static void 
defout(void)
{
	/* write out the defines (at the end of the declaration section) */

	register int i, c;
	register wchar_t *cp;

	for (i = ndefout; i <= ntokens; ++i) {

		cp = tokset[i].name;
		if (*cp == L' ')	/* literals */
		{
			fprintf(fdebug, "\t\"%ls\",\t%d,\n",
				tokset[i].name + 1, tokset[i].value);
			continue;	/* was cp++ */
		}

		for (; (c = *cp) != 0; ++cp) {
			if (iswlower(c) || iswupper(c) ||
				iswdigit(c) || c == L'_') /* EMPTY */;
			else
				goto nodef;
			}

		fprintf(fdebug, "\t\"%ls\",\t%d,\n", tokset[i].name,
			tokset[i].value);
		fprintf(ftable, "# define %ls %d\n", tokset[i].name,
			tokset[i].value);
		if (fdefine != NULL)
			fprintf(fdefine, "# define %ls %d\n",
				tokset[i].name,
				tokset[i].value);

	nodef:;
	}
	ndefout = ntokens+1;
}

static int
gettok(void)
{
	register int i, base;
	static int peekline; /* number of '\n' seen in lookahead */
	register int c, match, reserve;
begin:
	reserve = 0;
	lineno += peekline;
	peekline = 0;
	c = getwc(finput);
	/*
	 * while (c == ' ' || c == '\n' || c == '\t' || c == '\f') {
	 */
	while (iswspace(c)) {
		if (c == L'\n')
			++lineno;
		c = getwc(finput);
	}
	if (c == L'/') { /* skip comment */
		lineno += skipcom();
		goto begin;
	}

	switch (c) {

	case EOF:
		return (ENDFILE);
	case L'{':
		ungetwc(c, finput);
		return (L'=');  /* action ... */
	case L'<':  /* get, and look up, a type name (union member name) */
		i = 0;
		while ((c = getwc(finput)) != L'>' &&
				c != EOF && c != L'\n') {
			tokname[i] = c;
			if (++i >= toksize)
				exp_tokname();
			}
		if (c != L'>')
			error("unterminated < ... > clause");
		tokname[i] = 0;
		if (i == 0)
			error("missing type name in < ... > clause");
		for (i = 1; i <= ntypes; ++i) {
			if (!wcscmp(typeset[i], tokname)) {
				numbval = i;
				return (TYPENAME);
				}
			}
		typeset[numbval = ++ntypes] = cstash(tokname);
		return (TYPENAME);

	case L'"':
	case L'\'':
		match = c;
		tokname[0] = L' ';
		i = 1;
		for (;;) {
			c = getwc(finput);
			if (c == L'\n' || c == EOF)
				error("illegal or missing ' or \"");
			if (c == L'\\') {
				c = getwc(finput);
				tokname[i] = L'\\';
				if (++i >= toksize)
					exp_tokname();
			} else if (c == match) break;
			tokname[i] = c;
			if (++i >= toksize)
				exp_tokname();
			}
		break;

	case L'%':
	case L'\\':

		switch (c = getwc(finput)) {

		case L'0':	return (TERM);
		case L'<':	return (LEFT);
		case L'2':	return (BINARY);
		case L'>':	return (RIGHT);
		case L'%':
		case L'\\':	return (MARK);
		case L'=':	return (PREC);
		case L'{':	return (LCURLY);
		default:	reserve = 1;
			}

	default:

		if (iswdigit(c)) { /* number */
			numbval = c - L'0';
			base = (c == L'0') ? 8 : 10;
			for (c = getwc(finput);
					iswdigit(c);
					c = getwc(finput)) {
				numbval = numbval*base + c - L'0';
				}
			ungetwc(c, finput);
			return (NUMBER);
		} else if (iswlower(c) || iswupper(c) ||
				c == L'_' || c == L'.' ||
				c == L'$') {
			i = 0;
			while (iswlower(c) || iswupper(c) ||
					iswdigit(c) || c == L'_' ||
					c == L'.' || c == L'$') {
				tokname[i] = c;
				if (reserve && iswupper(c))
					tokname[i] = towlower(c);
				if (++i >= toksize)
					exp_tokname();
				c = getwc(finput);
				}
			}
		else
			return (c);

		ungetwc(c, finput);
		}

	tokname[i] = 0;

	if (reserve) { /* find a reserved word */
		if (!wcscmp(tokname, L"term"))
			return (TERM);
		if (!wcscmp(tokname, L"token"))
			return (TERM);
		if (!wcscmp(tokname, L"left"))
			return (LEFT);
		if (!wcscmp(tokname, L"nonassoc"))
			return (BINARY);
		if (!wcscmp(tokname, L"binary"))
			return (BINARY);
		if (!wcscmp(tokname, L"right"))
			return (RIGHT);
		if (!wcscmp(tokname, L"prec"))
			return (PREC);
		if (!wcscmp(tokname, L"start"))
			return (START);
		if (!wcscmp(tokname, L"type"))
			return (TYPEDEF);
		if (!wcscmp(tokname, L"union"))
			return (UNION);
		error("invalid escape, or illegal reserved word: %ls", tokname);
		}

	/* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

	c = getwc(finput);
	/*
	 * while (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '/')
	 * {
	 */
	while (iswspace(c) || c == L'/') {
		if (c == L'\n') {
			++peekline;
		} else if (c == L'/') { /* look for comments */
			peekline += skipcom();
			}
		c = getwc(finput);
		}
	if (c == L':')
		return (C_IDENTIFIER);
	ungetwc(c, finput);
	return (IDENTIFIER);
}

static int
fdtype(int t)
{
	/* determine the type of a symbol */
	register int v;
	if (t >= NTBASE)
		v = nontrst[t-NTBASE].tvalue;
	else
		v = TYPE(toklev[t]);
	if (v <= 0)
		error("must specify type for %ls",
			(t >= NTBASE) ? nontrst[t-NTBASE].name:
			tokset[t].name);
	return (v);
}

static int
chfind(int t, register wchar_t *s)
{
	int i;

	if (s[0] == ' ')
		t = 0;
	TLOOP(i) {
		if (!wcscmp(s, tokset[i].name)) {
			return (i);
		}
	}
	NTLOOP(i) {
		if (!wcscmp(s, nontrst[i].name)) {
			return (i + NTBASE);
		}
	}
	/* cannot find name */
	if (t > 1)
		error("%ls should have been defined earlier", s);
	return (defin(t, s));
}

static void 
cpyunion(void)
{
	/*
	 * copy the union declaration to the output,
	 * and the define file if present
	 */
	int level, c;
	if (gen_lines)
		fprintf(ftable, "\n# line %d \"%s\"\n", lineno, infile);
	fprintf(ftable, "typedef union\n");
	if (fdefine)
		fprintf(fdefine, "\ntypedef union\n");
	fprintf(ftable, "#ifdef __cplusplus\n\tYYSTYPE\n#endif\n");
	if (fdefine)
		fprintf(fdefine, "#ifdef __cplusplus\n\tYYSTYPE\n#endif\n");

	level = 0;
	for (;;) {
		if ((c = getwc(finput)) == EOF)
			error("EOF encountered while processing %%union");
		putwc(c, ftable);
		if (fdefine)
			putwc(c, fdefine);

		switch (c) {

		case L'\n':
			++lineno;
			break;

		case L'{':
			++level;
			break;

		case L'}':
			--level;
			if (level == 0) { /* we are finished copying */
				fprintf(ftable, " YYSTYPE;\n");
				if (fdefine)
					fprintf(fdefine,
					" YYSTYPE;\nextern YYSTYPE yylval;\n");
				return;
				}
			}
		}
}

static void 
cpycode(void)
{
	/* copies code between \{ and \} */

	int c;
	c = getwc(finput);
	if (c == L'\n') {
		c = getwc(finput);
		lineno++;
		}
	if (gen_lines)
		fprintf(ftable, "\n# line %d \"%s\"\n", lineno, infile);
	while (c != EOF) {
		if (c == L'\\') {
			if ((c = getwc(finput)) == L'}')
				return;
			else
				putwc(L'\\', ftable);
		} else if (c == L'%') {
			if ((c = getwc(finput)) == L'}')
				return;
			else
				putwc(L'%', ftable);
		}
		putwc(c, ftable);
		if (c == L'\n')
			++lineno;
		c = getwc(finput);
		}
	error("eof before %%}");
}

static int
skipcom(void)
{
	/* skip over comments */
	register int c, i = 0;  /* i is the number of lines skipped */

	/* skipcom is called after reading a / */

	if (getwc(finput) != L'*')
		error("illegal comment");
	c = getwc(finput);
	while (c != EOF) {
		while (c == L'*') {
			if ((c = getwc(finput)) == L'/')
				return (i);
			}
		if (c == L'\n')
			++i;
		c = getwc(finput);
		}
	error("EOF inside comment");
	/* NOTREACHED */
	return 0;
}

static void 
cpyact(int offset)
{
	/* copy C action to the next ; or closing } */
	int brac, c, match, i, t, j, s, tok, argument, m;
	wchar_t id_name[NAMESIZE+1];
	int id_idx = 0;

	if (gen_lines) {
		fprintf(faction, "\n# line %d \"%s\"\n", lineno, infile);
		act_lines++;
	}
	brac = 0;
	id_name[0] = 0;
loop:
	c = getwc(finput);
swt:
	switch (c) {
	case L';':
		if (brac == 0) {
			putwc(c, faction);
			return;
		}
		goto lcopy;
	case L'{':
		brac++;
		goto lcopy;
	case L'$':
		s = 1;
		tok = -1;
		argument = 1;
		while ((c = getwc(finput)) == L' ' || c == L'\t') /* EMPTY */;
		if (c == L'<') { /* type description */
			ungetwc(c, finput);
			if (gettok() != TYPENAME)
				error("bad syntax on $<ident> clause");
			tok = numbval;
			c = getwc(finput);
		}
		if (c == L'$') {
			fprintf(faction, "yyval");
			if (ntypes) { /* put out the proper tag... */
				if (tok < 0)
					tok = fdtype(*prdptr[nprod]);
				fprintf(faction, ".%ls", typeset[tok]);
			}
			goto loop;
		}
		if (iswalpha(c)) {
			int same = 0;
			int id_sw = 0;
			ungetwc(c, finput);
			if (gettok() != IDENTIFIER)
				error("bad action format");
			/*
			 * Save the number of non-terminal
			 */
			id_sw = nnonter;
			t = chfind(1, tokname);
			/*
			 * Check if the identifier is added as a non-terminal
			 */
			if (id_sw != nnonter)
				id_sw = 1;
			else
				id_sw = 0;
			while ((c = getwc(finput)) == L' ' ||
				c == L'\t') /* EMPTY */;
			if (c == L'#') {
				while ((c = getwc(finput)) == L' ' ||
					c == L'\t') /* EMPTY */;
				if (iswdigit(c)) {
					m = 0;
					while (iswdigit(c)) {
						m = m*10+c-L'0';
						c = getwc(finput);
					}
					argument = m;
				} else
					error("illegal character \"#\"");
			}
			if (argument < 1)
				error("illegal action argument no.");
			for (i = 1; i <= offset; ++i)
				if (prdptr[nprod][i] == t)
					if (++same == argument) {
						fprintf(faction,
							"yypvt[-%d]", offset-i);
						if (ntypes) {
							if (tok < 0)
								tok =
								/* CSTYLED */
								fdtype(prdptr[nprod][i]);
							fprintf(faction,
							".%ls", typeset[tok]);
						}
						goto swt;
					}
			/*
			 * This used to be handled as error.
			 * Treat this as a valid C statement.
			 * (Likely id with $ in.)
			 * If non-terminal is added, remove it from the list.
			 */
			fprintf(faction, "$%ls", tokname);
			warning(1, 
	"Illegal character '$' in Ansi C symbol: %ls$%ls.",
				id_name, tokname);

			if (id_sw == 1)
				--nnonter;
			goto swt;
		}
		if (c == '-') {
			s = -s;
			c = getwc(finput);
		}
		if (iswdigit(c)) {
			j = 0;
			while (iswdigit(c)) {
				j = j*10 + c - L'0';
				c = getwc(finput);
			}
			j = j*s - offset;
			if (j > 0) {
				error("Illegal use of $%d", j + offset);
			}
			fprintf(faction, "yypvt[-%d]", -j);
			if (ntypes) { /* put out the proper tag */
				if (j + offset <= 0 && tok < 0)
					error("must specify type of $%d",
					j + offset);
				if (tok < 0)
					tok = fdtype(prdptr[nprod][j+offset]);
				fprintf(faction,
					".%ls", typeset[tok]);
			}
			goto swt;
		}
		putwc(L'$', faction);
		if (s < 0)
			putwc(L'-', faction);
		goto swt;
	case L'}':
		if (--brac)
			goto lcopy;
		putwc(c, faction);
		return;
	case L'/':	/* look for comments */
		putwc(c, faction);
		c = getwc(finput);
		if (c != L'*')
			goto swt;
		/* it really is a comment */
		putwc(c, faction);
		c = getwc(finput);
		while (c != EOF) {
			while (c == L'*') {
				putwc(c, faction);
				if ((c = getwc(finput)) == L'/')
					goto lcopy;
			}
			putwc(c, faction);
			if (c == L'\n')
				++lineno;
			c = getwc(finput);
		}
		error("EOF inside comment");
		/* FALLTHRU */
	case L'\'':	/* character constant */
	case L'"':	/* character string */
		match = c;
		putwc(c, faction);
		while ((c = getwc(finput)) != EOF) {
			if (c == L'\\') {
				putwc(c, faction);
				c = getwc(finput);
				if (c == L'\n')
					++lineno;
			} else if (c == match)
				goto lcopy;
			else if (c == L'\n')
				error("newline in string or char. const.");
			putwc(c, faction);
		}
		error("EOF in string or character constant");
		/* FALLTHRU */
	case EOF:
		error("action does not terminate");
		/* FALLTHRU */
	case L'\n':
		++lineno;
		goto lcopy;
	}
lcopy:
	putwc(c, faction);
	/*
	 * Save the possible identifier name.
	 * Used to print out a warning message.
	 */
	if (id_idx >= NAMESIZE) {
		/*
		 * Error. Silently ignore.
		 */
		;
	}
	/*
	 * If c has a possibility to be a
	 * part of identifier, save it.
	 */
	else if (iswalnum(c) || c == L'_') {
		id_name[id_idx++] = c;
		id_name[id_idx] = 0;
	} else {
		id_idx = 0;
		id_name[id_idx] = 0;
	}
	goto loop;
}

static void
lhsfill(s)	/* new rule, dump old (if exists), restart strings */
wchar_t *s;
{
	static int lhs_len = LHS_TEXT_LEN;
	int s_lhs = wcslen(s);
	if (s_lhs >= lhs_len) {
		lhs_len = s_lhs + 2;
		lhstext = realloc(lhstext, sizeof (wchar_t)*lhs_len);
		if (lhstext == NULL)
			error("couldn't expanded LHS length");
	}
	rhsfill(NULL);
	wcscpy(lhstext, s); /* don't worry about too long of a name */
}

static void
rhsfill(wchar_t *s)	/* either name or 0 */
{
	static wchar_t *loc;	/* next free location in rhstext */
	static int rhs_len = RHS_TEXT_LEN;
	static int used = 0;
	int s_rhs = (s == NULL ? 0 : wcslen(s));
	register wchar_t *p;

	if (!s)	/* print out and erase old text */
	{
		if (*lhstext)		/* there was an old rule - dump it */
			lrprnt();
		(loc = rhstext)[0] = 0;
		return;
	}
	/* add to stuff in rhstext */
	p = s;

	used = loc - rhstext;
	if ((s_rhs + 3) >= (rhs_len - used)) {
		static wchar_t *textbase;
		textbase = rhstext;
		rhs_len += s_rhs + RHS_TEXT_LEN;
		rhstext = realloc(rhstext, sizeof (wchar_t)*rhs_len);
		if (rhstext == NULL)
			error("couldn't expanded RHS length");
		loc = loc - textbase + rhstext;
	}

	*loc++ = L' ';
	if (*s == L' ') /* special quoted symbol */
	{
		*loc++ = L'\'';	/* add first quote */
		p++;
	}
	while (*loc = *p++)
		if (loc++ > &rhstext[ RHS_TEXT_LEN ] - 3)
			break;

	if (*s == L' ')
		*loc++ = L'\'';
	*loc = 0;		/* terminate the string */
}

static void 
lrprnt (void)	/* print out the left and right hand sides */
{
	wchar_t *rhs;
	wchar_t *m_rhs = NULL;

	if (!*rhstext)		/* empty rhs - print usual comment */
		rhs = L" /* empty */";
	else {
		int idx1; /* tmp idx used to find if there are d_quotes */
		int idx2; /* tmp idx used to generate escaped string */
		wchar_t *p;
		/*
		 * Check if there are any double quote in RHS.
		 */
		for (idx1 = 0; rhstext[idx1] != 0; idx1++) {
			if (rhstext[idx1] == L'"') {
				/*
				 * A double quote is found.
				 */
				idx2 = wcslen(rhstext)*2;
				p = m_rhs = malloc((idx2 + 1)*sizeof (wchar_t));
				if (m_rhs == NULL)
					error(
					"Couldn't allocate memory for RHS.");
				/*
				 * Copy string
				 */
				for (idx2 = 0; rhstext[idx2] != 0; idx2++) {
					/*
					 * Check if this quote is escaped or not
					 */
					if (rhstext[idx2] == L'"') {
						int tmp_l = idx2-1;
						int cnt = 0;
						while (tmp_l >= 0 &&
						rhstext[tmp_l] == '\\') {
							cnt++;
							tmp_l--;
						}
						/*
						 * If quote is not escaped,
						 * then escape it.
						 */
						if (cnt%2 == 0)
							*p++ = L'\\';
					}
					*p++ = rhstext[idx2];
				}
				*p = 0;
				/*
				 * Break from the loop
				 */
				break;
			}
		}
		if (m_rhs == NULL)
			rhs = rhstext;
		else
			rhs = m_rhs;
	}
	fprintf(fdebug, "\t\"%ls :%ls\",\n", lhstext, rhs);
	if (m_rhs)
		free(m_rhs);
}


static void 
beg_debug (void)	/* dump initial sequence for fdebug file */
{
	fprintf(fdebug, "typedef struct\n");
	fprintf(fdebug, "#ifdef __cplusplus\n\tyytoktype\n");
	fprintf(fdebug, "#endif\n{\n");
	fprintf(fdebug, "#ifdef __cplusplus\nconst\n#endif\n");
	fprintf(fdebug, "char *t_name; int t_val; } yytoktype;\n");
	fprintf(fdebug, "#ifndef YYDEBUG\n#\tdefine YYDEBUG\t%d", gen_testing);
	fprintf(fdebug, "\t/*%sallow debugging */\n#endif\n\n",
		gen_testing ? " " : " don't ");
	fprintf(fdebug, "#if YYDEBUG\n\nyytoktype yytoks[] =\n{\n");
}


static void 
end_toks (void)	/* finish yytoks array, get ready for yyred's strings */
{
	fprintf(fdebug, "\t\"-unknown-\",\t-1\t/* ends search */\n");
	fprintf(fdebug, "};\n\n");
	fprintf(fdebug,
		"#ifdef __cplusplus\nconst\n#endif\n");
	fprintf(fdebug, "char * yyreds[] =\n{\n");
	fprintf(fdebug, "\t\"-no such reduction-\",\n");
}


static void 
end_debug (void)	/* finish yyred array, close file */
{
	lrprnt();		/* dump last lhs, rhs */
	fprintf(fdebug, "};\n#endif /* YYDEBUG */\n");
	fclose(fdebug);
}


/*
 * 2/29/88 -
 * The normal length for token sizes is NAMESIZE - If a token is
 * seen that has a longer length, expand "tokname" by NAMESIZE.
 */
static void 
exp_tokname(void)
{
	toksize += NAMESIZE;
	tokname = realloc(tokname, sizeof (wchar_t) * toksize);
}


/*
 * 2/29/88 -
 *
 */
static void 
exp_prod(void)
{
	int i;
	nprodsz += NPROD;

	prdptr = realloc(prdptr, sizeof (int *) * (nprodsz+2));
	levprd  = realloc(levprd, sizeof (int) * (nprodsz+2));
	had_act = realloc(had_act, sizeof (wchar_t) * (nprodsz+2));
	for (i = nprodsz-NPROD; i < nprodsz+2; ++i)
		had_act[i] = 0;

	if ((*prdptr == NULL) || (levprd == NULL) || (had_act == NULL))
		error("couldn't expand productions");
}

/*
 * 2/29/88 -
 * Expand the number of terminals.  Initially there are NTERMS;
 * each time space runs out, the size is increased by NTERMS.
 * The total size, however, cannot exceed MAXTERMS because of
 * the way LOOKSETS(struct looksets) is set up.
 * Tables affected:
 *	tokset, toklev : increased to ntoksz
 *
 *	tables with initial dimensions of TEMPSIZE must be changed if
 *	(ntoksz + NNONTERM) >= TEMPSIZE : temp1[]
 */
static void 
exp_ntok(void)
{
	ntoksz += NTERMS;

	tokset = realloc(tokset, sizeof (TOKSYMB) * ntoksz);
	toklev = realloc(toklev, sizeof (int) * ntoksz);

	if ((tokset == NULL) || (toklev == NULL))
		error("couldn't expand NTERMS");
}


static void 
exp_nonterm(void)
{
	nnontersz += NNONTERM;

	nontrst = realloc(nontrst, sizeof (TOKSYMB) * nnontersz);
	if (nontrst == NULL)
		error("couldn't expand NNONTERM");
}

void 
exp_mem(int flag)
{
	int i;
	static int *membase;
	new_memsize += MEMSIZE;

	membase = tracemem;
	tracemem = realloc(tracemem, sizeof (int) * new_memsize);
	if (tracemem == NULL)
		error("couldn't expand mem table");
	if (flag) {
		for (i = 0; i <= nprod; ++i)
			prdptr[i] = prdptr[i] - membase + tracemem;
		mem = mem - membase + tracemem;
	} else {
		size += MEMSIZE;
		temp1 = realloc(temp1, sizeof (int)*size);
		optimmem = optimmem - membase + tracemem;
	}
}

static int 
findchtok(int chlit)
/*
 * findchtok(chlit) returns the token number for a character literal
 * chlit that is "bigger" than 255 -- the max char value that the
 * original yacc was build for.  This yacc treate them as though
 * an ordinary token.
 */
{
	int	i;

	if (chlit < 0xff)
		return (chlit); /* single-byte char */
	for (i = 0; i < nmbchars; ++i) {
		if (mbchars->character == chlit)
			return (mbchars->tvalue);
	}

	/* Not found.  Register it! */
	if (++nmbchars > nmbcharsz) { /* Make sure there's enough space */
		nmbcharsz += NMBCHARSZ;
		mbchars = realloc(mbchars, sizeof (MBCLIT)*nmbcharsz);
		if (mbchars == NULL)
			error("too many character literals");
	}
	mbchars[nmbchars-1].character = chlit;
	return (mbchars[nmbchars-1].tvalue = extval++);
	/* Return the newly assigned token. */
}

/*
 * When -p is specified, symbol prefix for
 *	yy{parse, lex, error}(),
 *	yy{lval, val, char, debug, errflag, nerrs}
 * are defined to the specified name.
 */
static void
put_prefix_define(char *pre)
{
	char *syms[] = {
		/* Functions */
		"parse",
		"lex",
		"error",
		/* Variables */
		"lval",
		"val",
		"char",
		"debug",
		"errflag",
		"nerrs",
		NULL};
	int i;

	for (i = 0; syms[i]; i++)
		fprintf(ftable, "#define\tyy%s\t%s%s\n",
			syms[i], pre, syms[i]);
}

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
 * Copyright 2005 Sun Microsystems, Inc.
 * All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	All Rights Reserved	*/

/*	from OpenSolaris "header.c	6.22	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)header.c	1.12 (gritter) 9/23/06
 */

#include "ldefs.c"

static void rhd1(void);
static void chd1(void);
static void chd2(void);
static void ctail(void);
static void rtail(void);

void
phead1(void)
{
	ratfor ? rhd1() : chd1();
}

static void
chd1(void)
{
	if (*v_stmp == 'y') {
		extern const char	rel[];
		fprintf(fout, "\
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4\n\
#define	YYUSED	__attribute__ ((used))\n\
#elif defined __GNUC__\n\
#define	YYUSED	__attribute__ ((unused))\n\
#else\n\
#define	YYUSED\n\
#endif\n\
static const char yylexid[] USED = \"lex: %s\"\n", rel);
	}
	if (handleeuc) {
		fprintf(fout, "#ifndef EUC\n");
		fprintf(fout, "#define EUC\n");
		fprintf(fout, "#endif\n");
		fprintf(fout, "#include <stdio.h>\n");
		fprintf(fout, "#include <stdlib.h>\n");
		fprintf(fout, "#ifdef	__sun\n");
		fprintf(fout, "#include <widec.h>\n");
		fprintf(fout, "#else	/* !__sun */\n");
		fprintf(fout, "#include <wchar.h>\n");
		fprintf(fout, "#endif	/* !__sun */\n");
		if (widecio) { /* -w option */
			fprintf(fout, "#define YYTEXT yytext\n");
			fprintf(fout, "#define YYLENG yyleng\n");
			fprintf(fout, "#ifndef __cplusplus\n");
			fprintf(fout, "#define YYINPUT input\n");
			fprintf(fout, "#define YYOUTPUT output\n");
			fprintf(fout, "#else\n");
			fprintf(fout, "#define YYINPUT lex_input\n");
			fprintf(fout, "#define YYOUTPUT lex_output\n");
			fprintf(fout, "#endif\n");
			fprintf(fout, "#define YYUNPUT unput\n");
		} else { /* -e option */
			fprintf(fout, "#include <limits.h>\n");
			fprintf(fout, "#ifdef	__sun\n");
			fprintf(fout, "#include <sys/euc.h>\n");
			fprintf(fout, "#endif	/* __sun */\n");
			fprintf(fout, "#define YYLEX_E 1\n");
			fprintf(fout, "#define YYTEXT yywtext\n");
			fprintf(fout, "#define YYLENG yywleng\n");
			fprintf(fout, "#define YYINPUT yywinput\n");
			fprintf(fout, "#define YYOUTPUT yywoutput\n");
			fprintf(fout, "#define YYUNPUT yywunput\n");
		}
	} else { /* ASCII compatibility mode. */
		fprintf(fout, "#include <stdio.h>\n");
		fprintf(fout, "#include <stdlib.h>\n");
	}
	if (ZCH > NCH)
		fprintf(fout, "# define U(x) ((x)&0377)\n");
	else
	fprintf(fout, "# define U(x) x\n");
	fprintf(fout, "# define NLSTATE yyprevious=YYNEWLINE\n");
	fprintf(fout, "# define BEGIN yybgin = yysvec + 1 +\n");
	fprintf(fout, "# define INITIAL 0\n");
	fprintf(fout, "# define YYLERR yysvec\n");
	fprintf(fout, "# define YYSTATE (yyestate-yysvec-1)\n");
	if (optim)
		fprintf(fout, "# define YYOPTIM 1\n");
#ifdef DEBUG
	fprintf(fout, "# define LEXDEBUG 1\n");
#endif
	fprintf(fout, "# ifndef YYLMAX \n");
	fprintf(fout, "# define YYLMAX BUFSIZ\n");
	fprintf(fout, "# endif \n");
	fprintf(fout, "#ifndef __cplusplus\n");
	if (widecio)
		fprintf(fout, "# define output(c) (void)putwc(c,yyout)\n");
	else
		fprintf(fout, "# define output(c) (void)putc(c,yyout)\n");
	fprintf(fout, "#else\n");
	if (widecio)
		fprintf(fout, "# define lex_output(c) (void)putwc(c,yyout)\n");
	else
		fprintf(fout, "# define lex_output(c) (void)putc(c,yyout)\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "\n#if defined(__cplusplus) || defined(__STDC__)\n");
	fprintf(fout, "\n#if defined(__cplusplus) && defined(__EXTERN_C__)\n");
	fprintf(fout, "extern \"C\" {\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "\tint yyback(int *, int);\n"); /* ? */
	fprintf(fout, "\tint yyinput(void);\n"); /* ? */
	fprintf(fout, "\tint yylook(void);\n"); /* ? */
	fprintf(fout, "\tvoid yyoutput(int);\n"); /* ? */
	fprintf(fout, "\tint yyracc(int);\n"); /* ? */
	fprintf(fout, "\tint yyreject(void);\n"); /* ? */
	fprintf(fout, "\tvoid yyunput(int);\n"); /* ? */
	fprintf(fout, "\tint yylex(void);\n");
	fprintf(fout, "#ifdef YYLEX_E\n");
	fprintf(fout, "\tvoid yywoutput(wchar_t);\n");
	fprintf(fout, "\twchar_t yywinput(void);\n");
	fprintf(fout, "\tvoid yywunput(wchar_t);\n");
	fprintf(fout, "#endif\n");

	/* XCU4: type of yyless is int */
	fprintf(fout, "#ifndef yyless\n");
	fprintf(fout, "\tint yyless(int);\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "#ifndef yywrap\n");
	fprintf(fout, "\tint yywrap(void);\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "#ifdef LEXDEBUG\n");
	fprintf(fout, "\tvoid allprint(char);\n");
	fprintf(fout, "\tvoid sprint(char *);\n");
	fprintf(fout, "#endif\n");
	fprintf(fout,
	"#if defined(__cplusplus) && defined(__EXTERN_C__)\n");
	fprintf(fout, "}\n");
	fprintf(fout, "#endif\n\n");
	fprintf(fout, "#ifdef __cplusplus\n");
	fprintf(fout, "extern \"C\" {\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "\tvoid exit(int);\n");
	fprintf(fout, "#ifdef __cplusplus\n");
	fprintf(fout, "}\n");
	fprintf(fout, "#endif\n\n");
	fprintf(fout, "#endif\n");
	fprintf(fout,
	"# define unput(c)"
	" {yytchar= (c);if(yytchar=='\\n')yylineno--;*yysptr++=yytchar;}\n");
	fprintf(fout, "# define yymore() (yymorfg=1)\n");
	if (widecio) {
		fprintf(fout, "#ifndef __cplusplus\n");
		fprintf(fout, "%s%d%s\n",
"# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):yygetwchar())==",
		ctable['\n'],
"?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
		fprintf(fout, "#else\n");
		fprintf(fout, "%s%d%s\n",
"# define lex_input() (((yytchar=yysptr>yysbuf?U(*--yysptr):yygetwchar())==",
		ctable['\n'],
"?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
		fprintf(fout, "#endif\n");
		fprintf(fout,
		"# define ECHO (void)fprintf(yyout, \"%%ls\",yytext)\n");
		fprintf(fout,
		"# define REJECT { nstr = yyreject_w(); goto yyfussy;}\n");
		fprintf(fout, "#define yyless yyless_w\n");
		fprintf(fout, "int yyreject_w(void);\n");
		fprintf(fout, "int yyleng;\n");

		/*
		 * XCU4:
		 * If %array, yytext[] contains the token.
		 * If %pointer, yytext is a pointer to yy_tbuf[].
		 */

		if (isArray) {
			fprintf(fout, "#define YYISARRAY\n");
			fprintf(fout, "wchar_t yytext[YYLMAX];\n");
		} else {
			fprintf(fout, "wchar_t yy_tbuf[YYLMAX];\n");
			fprintf(fout, "wchar_t * yytext = yy_tbuf;\n");
			fprintf(fout, "int yytextsz = YYLMAX;\n");
			fprintf(fout, "#ifndef YYTEXTSZINC\n");
			fprintf(fout, "#define YYTEXTSZINC 100\n");
			fprintf(fout, "#endif\n");
		}
	} else {
		fprintf(fout, "#ifndef __cplusplus\n");
		fprintf(fout, "%s%d%s\n",
"# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==",
		ctable['\n'],
"?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
		fprintf(fout, "#else\n");
		fprintf(fout, "%s%d%s\n",
"# define lex_input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==",
		ctable['\n'],
"?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
		fprintf(fout, "#endif\n");
		fprintf(fout, "#define ECHO fprintf(yyout, \"%%s\",yytext)\n");
		if (handleeuc) {
			fprintf(fout,
"# define REJECT { nstr = yyreject_e(); goto yyfussy;}\n");
			fprintf(fout, "int yyreject_e(void);\n");
			fprintf(fout, "int yyleng;\n");
			fprintf(fout, "size_t yywleng;\n");
			/*
			 * XCU4:
			 * If %array, yytext[] contains the token.
			 * If %pointer, yytext is a pointer to yy_tbuf[].
			 */
			if (isArray) {
				fprintf(fout, "#define YYISARRAY\n");
				fprintf(fout,
				"unsigned char yytext[YYLMAX*MB_LEN_MAX];\n");
				fprintf(fout,
				"wchar_t yywtext[YYLMAX];\n");
			} else {
				fprintf(fout,
				"wchar_t yy_twbuf[YYLMAX];\n");
				fprintf(fout,
				"wchar_t yy_tbuf[YYLMAX*MB_LEN_MAX];\n");
				fprintf(fout,
				"unsigned char * yytext ="
				"(unsigned char *)yy_tbuf;\n");
				fprintf(fout,
				"wchar_t * yywtext = yy_twbuf;\n");
				fprintf(fout,
						"int yytextsz = YYLMAX;\n");
				fprintf(fout, "#ifndef YYTEXTSZINC\n");
				fprintf(fout, "#define YYTEXTSZINC 100\n");
				fprintf(fout, "#endif\n");
			}
		} else {
			fprintf(fout,
"# define REJECT { nstr = yyreject(); goto yyfussy;}\n");
			fprintf(fout, "int yyleng;\n");

			/*
			 * XCU4:
			 * If %array, yytext[] contains the token.
			 * If %pointer, yytext is a pointer to yy_tbuf[].
			 */
			if (isArray) {
				fprintf(fout, "#define YYISARRAY\n");
				fprintf(fout, "char yytext[YYLMAX];\n");
			} else {
				fprintf(fout, "char yy_tbuf[YYLMAX];\n");
				fprintf(fout,
				"char * yytext = yy_tbuf;\n");
				fprintf(fout,
					"int yytextsz = YYLMAX;\n");
				fprintf(fout, "#ifndef YYTEXTSZINC\n");
				fprintf(fout,
					"#define YYTEXTSZINC 100\n");
				fprintf(fout, "#endif\n");
			}
		}
	}
	fprintf(fout, "int yymorfg;\n");
	if (handleeuc)
		fprintf(fout, "extern wchar_t *yysptr, yysbuf[];\n");
	else
		fprintf(fout, "extern char *yysptr, yysbuf[];\n");
	fprintf(fout, "int yytchar;\n");
	fprintf(fout, "FILE *yyin = (FILE *)-1, *yyout = (FILE *)-1;\n");
	fprintf(fout, "#if defined (__GNUC__)\n");
	fprintf(fout,
	    "static void _yyioinit(void) __attribute__ ((constructor));\n");
	fprintf(fout, "#elif defined (__SUNPRO_C)\n");
	fprintf(fout, "#pragma init (_yyioinit)\n");
	fprintf(fout, "#elif defined (__HP_aCC) || defined (__hpux)\n");
	fprintf(fout, "#pragma INIT \"_yyioinit\"\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "static void _yyioinit(void) {\n");
	fprintf(fout, "yyin = stdin; yyout = stdout; }\n");
	fprintf(fout, "extern int yylineno;\n");
	fprintf(fout, "struct yysvf { \n");
	fprintf(fout, "\tstruct yywork *yystoff;\n");
	fprintf(fout, "\tstruct yysvf *yyother;\n");
	fprintf(fout, "\tint *yystops;};\n");
	fprintf(fout, "struct yysvf *yyestate;\n");
	fprintf(fout, "extern struct yysvf yysvec[], *yybgin;\n");
}

static void
rhd1(void)
{
	fprintf(fout, "integer function yylex(dummy)\n");
	fprintf(fout, "define YYLMAX 200\n");
	fprintf(fout, "define ECHO call yyecho(yytext,yyleng)\n");
	fprintf(fout,
	"define REJECT nstr = yyrjct(yytext,yyleng);goto 30998\n");
	fprintf(fout, "integer nstr,yylook,yywrap\n");
	fprintf(fout, "integer yyleng, yytext(YYLMAX)\n");
	fprintf(fout, "common /yyxel/ yyleng, yytext\n");
	fprintf(fout,
	"common /yyldat/ yyfnd, yymorf, yyprev, yybgin, yylsp, yylsta\n");
	fprintf(fout,
	"integer yyfnd, yymorf, yyprev, yybgin, yylsp, yylsta(YYLMAX)\n");
	fprintf(fout, "for(;;){\n");
	fprintf(fout, "\t30999 nstr = yylook(dummy)\n");
	fprintf(fout, "\tgoto 30998\n");
	fprintf(fout, "\t30000 k = yywrap(dummy)\n");
	fprintf(fout, "\tif(k .ne. 0){\n");
	fprintf(fout, "\tyylex=0; return; }\n");
	fprintf(fout, "\t\telse goto 30998\n");
}

void
phead2(void)
{
	if (!ratfor)
		chd2();
}

static void
chd2(void)
{
	fprintf(fout, "if (yyin == (FILE *)-1) yyin = stdin;\n");
	fprintf(fout, "if (yyout == (FILE *)-1) yyout = stdout;\n");
	fprintf(fout, "#if defined (__cplusplus) || defined (__GNUC__)\n");
	fprintf(fout,
	"/* to avoid CC and lint complaining yyfussy not being used ...*/\n");
	fprintf(fout, "{static int __lex_hack = 0;\n");
	fprintf(fout, "if (__lex_hack) { yyprevious = 0; goto yyfussy; } }\n");
	fprintf(fout, "#endif\n");
	fprintf(fout, "while((nstr = yylook()) >= 0)\n");
	fprintf(fout, "yyfussy: switch(nstr){\n");
	fprintf(fout, "case 0:\n");
	fprintf(fout, "if(yywrap()) return(0); break;\n");
}

void
ptail(void)
{
	if (!pflag)
		ratfor ? rtail() : ctail();
	pflag = 1;
}

static void
ctail(void)
{
	fprintf(fout, "case -1:\nbreak;\n");		/* for reject */
	fprintf(fout, "default:\n");
	fprintf(fout,
	"(void)fprintf(yyout,\"bad switch yylook %%d\",nstr);\n");
	fprintf(fout, "} return(0); }\n");
	fprintf(fout, "/* end of yylex */\n");
}

static void
rtail(void)
{
	int i;
	fprintf(fout,
	"\n30998 if(nstr .lt. 0 .or. nstr .gt. %d)goto 30999\n", casecount);
	fprintf(fout, "nstr = nstr + 1\n");
	fprintf(fout, "goto(\n");
	for (i = 0; i < casecount; i++)
		fprintf(fout, "%d,\n", 30000+i);
	fprintf(fout, "30999),nstr\n");
	fprintf(fout, "30997 continue\n");
	fprintf(fout, "}\nend\n");
}

void
statistics(void)
{
	fprintf(errorf,
"%d/%d nodes(%%e), %d/%d positions(%%p), %d/%d (%%n), %ld transitions,\n",
	tptr, treesize, nxtpos-positions, maxpos, stnum + 1, nstates, rcount);
	fprintf(errorf,
	"%d/%d packed char classes(%%k), ", pcptr-pchar, pchlen);
	if (optim)
		fprintf(errorf,
		" %d/%d packed transitions(%%a), ", nptr, ntrans);
	fprintf(errorf, " %d/%d output slots(%%o)", yytop, outsize);
	putc('\n', errorf);
}

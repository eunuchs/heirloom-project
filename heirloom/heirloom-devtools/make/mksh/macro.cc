/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)macro.cc 1.22 06/12/12
 */

/*	from OpenSolaris "macro.cc	1.22	06/12/12"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)macro.cc	1.11 (gritter) 3/7/07
 */

/*
 *	macro.cc
 *
 *	Handle expansion of make macros
 */

/*
 * Included files
 */
#include <wordexp.h>
#include <mksh/dosys.h>		/* sh_command2string() */
#include <mksh/i18n.h>		/* get_char_semantics_value() */
#include <mksh/macro.h>
#include <mksh/misc.h>		/* retmem() */
#include <mksh/read.h>		/* get_next_block_fn() */
#include <mksdmsi18n/mksdmsi18n.h>	/* libmksdmsi18n_init() */

/*
 * File table of contents
 */
static void	add_macro_to_global_list(Name macro_to_add);
#ifdef NSE
static void	expand_value_with_daemon(Name name, register Property macro, register String destination, Boolean cmd);
#else
static void	expand_value_with_daemon(Name, register Property macro, register String destination, Boolean cmd);
#endif
static Boolean	expand_function(wchar_t *, wchar_t *, String);

static void	init_arch_macros(void);
static void	init_mach_macros(void);
static Boolean	init_arch_done = false;
static Boolean	init_mach_done = false;


long env_alloc_num = 0;
long env_alloc_bytes = 0;

/*
 *	getvar(name)
 *
 *	Return expanded value of macro.
 *
 *	Return value:
 *				The expanded value of the macro
 *
 *	Parameters:
 *		name		The name of the macro we want the value for
 *
 *	Global variables used:
 */
Name
getvar(register Name name)
{
	String_rec		destination;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	register Name		result;

	if ((name == host_arch) || (name == target_arch)) {
		if (!init_arch_done) {
			init_arch_done = true;
			init_arch_macros();
		}
	}
	if ((name == host_mach) || (name == target_mach)) {
		if (!init_mach_done) {
			init_mach_done = true;
			init_mach_macros();
		}
	}

	INIT_STRING_FROM_STACK(destination, buffer);
	expand_value(maybe_append_prop(name, macro_prop)->body.macro.value,
		     &destination,
		     false);
	result = GETNAME(destination.buffer.start, FIND_LENGTH);
	if (destination.free_after_use) {
		retmem(destination.buffer.start);
	}
	return result;
}

/*
 *	expand_value(value, destination, cmd)
 *
 *	Recursively expands all macros in the string value.
 *	destination is where the expanded value should be appended.
 *
 *	Parameters:
 *		value		The value we are expanding
 *		destination	Where to deposit the expansion
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 */
void
expand_value(Name value, register String destination, Boolean cmd)
{
	Source_rec		sourceb;
	register Source		source = &sourceb;
	register wchar_t	*source_p = NULL;
	register wchar_t	*source_end = NULL;
	wchar_t			*block_start = NULL;
	int			quote_seen = 0;

	if (value == NULL) {
		/*
		 * Make sure to get a string allocated even if it
		 * will be empty.
		 */
		MBSTOWCS(wcs_buffer, "");
		append_string(wcs_buffer, destination, FIND_LENGTH);
		destination->text.end = destination->text.p;
		return;
	}
	if (!value->dollar) {
		/*
		 * If the value we are expanding does not contain
		 * any $, we don't have to parse it.
		 */
		APPEND_NAME(value,
			destination,
			(int) value->hash.length
		);
		destination->text.end = destination->text.p;
		return;
	}

	if (value->being_expanded) {
		if (sun_style)
			fatal_reader_mksh("Loop detected when expanding macro value `%s'",
			     value->string_mb);
		else
			fatal_reader_mksh("infinitely recursive macro?.");
	}
	value->being_expanded = true;
	/* Setup the structure we read from */
	Wstring vals(value);
	sourceb.string.text.p = sourceb.string.buffer.start = wsdup(vals.get_string());
	sourceb.string.free_after_use = true;
	sourceb.string.text.end =
	  sourceb.string.buffer.end =
	    sourceb.string.text.p + value->hash.length;
	sourceb.previous = NULL;
	sourceb.fd = -1;
	sourceb.inp_buf =
	  sourceb.inp_buf_ptr =
	    sourceb.inp_buf_end = NULL;
	sourceb.error_converting = false;
	/* Lift some pointers from the struct to local register variables */
	CACHE_SOURCE(0);
/* We parse the string in segments */
/* We read chars until we find a $, then we append what we have read so far */
/* (since last $ processing) to the destination. When we find a $ we call */
/* expand_macro() and let it expand that particular $ reference into dest */
	block_start = source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case backslash_char:
			/* Quote $ in macro value */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/* Save the plain string we found since */
			/* start of string or previous $ */
			if (quote_seen) {
				append_string(block_start,
					      destination,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      destination,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			UNCACHE_SOURCE();
			/* Go expand the macro reference */
			expand_macro(source, destination, sourceb.string.buffer.start, cmd);
			CACHE_SOURCE(1);
			block_start = source_p + 1;
			break;
		case nul_char:
			/* The string ran out. Get some more */
			append_string(block_start,
				      destination,
				      source_p - block_start);
			GET_NEXT_BLOCK_NOCHK(source);
			if (source == NULL) {
				destination->text.end = destination->text.p;
				value->being_expanded = false;
				return;
			}
			if (source->error_converting) {
				fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_value()"));
			}
			block_start = source_p;
			source_p--;
			continue;
		}
		quote_seen = 0;
	}
	retmem(sourceb.string.buffer.start);
}

/*
 *	expand_macro(source, destination, current_string, cmd)
 *
 *	Should be called with source->string.text.p pointing to
 *	the first char after the $ that starts a macro reference.
 *	source->string.text.p is returned pointing to the first char after
 *	the macro name.
 *	It will read the macro name, expanding any macros in it,
 *	and get the value. The value is then expanded.
 *	destination is a String that is filled in with the expanded macro.
 *	It may be passed in referencing a buffer to expand the macro into.
 * 	Note that most expansions are done on demand, e.g. right 
 *	before the command is executed and not while the file is
 * 	being parsed.
 *
 *	Parameters:
 *		source		The source block that references the string
 *				to expand
 *		destination	Where to put the result
 *		current_string	The string we are expanding, for error msg
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 *		funny		Vector of semantic tags for characters
 *		is_conditional	Set if a conditional macro is refd
 *		make_word_mentioned Set if the word "MAKE" is mentioned
 *		makefile_type	We deliver extra msg when reading makefiles
 *		query		The Name "?", compared against
 *		query_mentioned	Set if the word "?" is mentioned
 */
void
expand_macro(register Source source, register String destination, wchar_t *current_string, Boolean cmd)
{
	static Name		make = (Name)NULL;
	static wchar_t		colon_sh[4];
	static wchar_t		colon_shell[7];
	String_rec		string;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	register wchar_t	*source_p = source->string.text.p;
	register wchar_t	*source_end = source->string.text.end;
	register int		closer = 0;
	wchar_t			*block_start = (wchar_t *)NULL;
	int			quote_seen = 0;
	register int		closer_level = 1;
	Name			name = (Name)NULL;
	wchar_t			*colon = (wchar_t *)NULL;
	wchar_t			*percent = (wchar_t *)NULL;
	wchar_t			*eq = (wchar_t *) NULL;
	Property		macro = NULL;
	wchar_t			*p = (wchar_t*)NULL;
	String_rec		extracted;
	wchar_t			extracted_string[MAXPATHLEN];
	wchar_t			*left_head = NULL;
	wchar_t			*left_tail = NULL;
	wchar_t			*right_tail = NULL;
	int			left_head_len = 0;
	int			left_tail_len = 0;
	int			tmp_len = 0;
	wchar_t			*right_hand[128];
	int			i = 0;
	enum {
		no_extract,
		dir_extract,
		file_extract
	}                       extraction = no_extract;
	enum {
		no_replace,
		suffix_replace,
		pattern_replace,
		sh_replace
	}			replacement = no_replace;

	if (make == NULL) {
		MBSTOWCS(wcs_buffer, NOCATGETS("MAKE"));
		make = GETNAME(wcs_buffer, FIND_LENGTH);

		MBSTOWCS(colon_sh, NOCATGETS(":sh"));
		MBSTOWCS(colon_shell, NOCATGETS(":shell"));
	}

	right_hand[0] = NULL;

	/* First copy the (macro-expanded) macro name into string. */
	INIT_STRING_FROM_STACK(string, buffer);
recheck_first_char:
	/* Check the first char of the macro name to figure out what to do. */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK_NOCHK(source);
		if (source == NULL) {
			WCSTOMBS(mbs_buffer, current_string);
			fatal_reader_mksh("'$' at end of string `%s'",
				     mbs_buffer);
		}
		if (source->error_converting) {
			fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_macro()"));
		}
		goto recheck_first_char;
	case parenleft_char:
		/* Multi char name. */
		closer = (int) parenright_char;
		break;
	case braceleft_char:
		/* Multi char name. */
		closer = (int) braceright_char;
		break;
	case newline_char:
		fatal_reader_mksh("'$' at end of line");
	default:
		/* Single char macro name. Just suck it up */
		append_char(*source_p, &string);
		source->string.text.p = source_p + 1;
		goto get_macro_value;
	}

	/* Handle multi-char macro names */
	block_start = ++source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			append_string(block_start,
				      &string,
				      source_p - block_start);
			GET_NEXT_BLOCK_NOCHK(source);
			if (source == NULL) {
				if (current_string != NULL) {
					WCSTOMBS(mbs_buffer, current_string);
					fatal_reader_mksh("Unmatched `%c' in string `%s'",
						     closer ==
						     (int) braceright_char ?
						     (int) braceleft_char :
						     (int) parenleft_char,
						     mbs_buffer);
				} else {
					fatal_reader_mksh("Premature EOF");
				}
			}
			if (source->error_converting) {
				fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_macro()"));
			}
			block_start = source_p;
			source_p--;
			continue;
		case newline_char:
			fatal_reader_mksh("Unmatched `%c' on line",
				     closer == (int) braceright_char ?
				     (int) braceleft_char :
				     (int) parenleft_char);
		case backslash_char:
			/* Quote dollar in macro value. */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/*
			 * Macro names may reference macros.
			 * This expands the value of such macros into the
			 * macro name string.
			 */
			if (quote_seen) {
				append_string(block_start,
					      &string,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      &string,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			UNCACHE_SOURCE();
			expand_macro(source, &string, current_string, cmd);
			CACHE_SOURCE(0);
			block_start = source_p;
			source_p--;
			break;
		case parenleft_char:
			/* Allow nested pairs of () in the macro name. */
			if (closer == (int) parenright_char) {
				closer_level++;
			}
			break;
		case braceleft_char:
			/* Allow nested pairs of {} in the macro name. */
			if (closer == (int) braceright_char) {
				closer_level++;
			}
			break;
		case parenright_char:
		case braceright_char:
			/*
			 * End of the name. Save the string in the macro
			 * name string.
			 */
			if ((*source_p == closer) && (--closer_level <= 0)) {
				source->string.text.p = source_p + 1;
				append_string(block_start,
					      &string,
					      source_p - block_start);
				goto get_macro_value;
			}
			break;
		}
		quote_seen = 0;
	}
	/*
	 * We got the macro name. We now inspect it to see if it
	 * specifies any translations of the value.
	 */
get_macro_value:
	name = NULL;
	/* First check if we have a $(@D) type translation. */
	if ((get_char_semantics_value(string.buffer.start[0]) &
	     (int) special_macro_sem) &&
	    (string.text.p - string.buffer.start >= 2) &&
	    ((string.buffer.start[1] == 'D') ||
	     (string.buffer.start[1] == 'F'))) {
		switch (string.buffer.start[1]) {
		case 'D':
			extraction = dir_extract;
			break;
		case 'F':
			extraction = file_extract;
			break;
		default:
			WCSTOMBS(mbs_buffer, string.buffer.start);
			fatal_reader_mksh("Illegal macro reference `%s'",
				     mbs_buffer);
		}
		/* Internalize the macro name using the first char only. */
		name = GETNAME(string.buffer.start, 1);
		wscpy(string.buffer.start, string.buffer.start + 2);
	}
	/* Check for other kinds of translations. */
	if ((colon = (wchar_t *) wschr(string.buffer.start,
				       (int) colon_char)) != NULL) {
		/*
		 * We have a $(FOO:.c=.o) type translation.
		 * Get the name of the macro proper.
		 */
		if (name == NULL) {
			name = GETNAME(string.buffer.start,
				       colon - string.buffer.start);
		}
		/* Pickup all the translations. */
		if (IS_WEQUAL(colon, colon_sh) || IS_WEQUAL(colon, colon_shell)) {
			replacement = sh_replace;
		} else if ((svr4) ||
		           ((percent = (wchar_t *) wschr(colon + 1,
							 (int) percent_char)) == NULL)) {
			while (colon != NULL) {
				if ((eq = (wchar_t *) wschr(colon + 1,
							    (int) equal_char)) == NULL) {
					fatal_reader_mksh("= missing from replacement macro reference");
				}
				left_tail_len = eq - colon - 1;
				if(left_tail) {
					retmem(left_tail);
				}
				left_tail = ALLOC_WC(left_tail_len + 1);
				wsncpy(left_tail,
					      colon + 1,
					      eq - colon - 1);
				left_tail[eq - colon - 1] = (int) nul_char;
				replacement = suffix_replace;
				if ((colon = (wchar_t *) wschr(eq + 1,
							       (int) colon_char)) != NULL) {
					tmp_len = colon - eq;
					if(right_tail) {
						retmem(right_tail);
					}
					right_tail = ALLOC_WC(tmp_len);
					wsncpy(right_tail,
						      eq + 1,
						      colon - eq - 1);
					right_tail[colon - eq - 1] =
					  (int) nul_char;
				} else {
					if(right_tail) {
						retmem(right_tail);
					}
					right_tail = ALLOC_WC(wslen(eq) + 1);
					wscpy(right_tail, eq + 1);
				}
			}
		} else {
			if ((eq = (wchar_t *) wschr(colon + 1,
						    (int) equal_char)) == NULL) {
				fatal_reader_mksh("= missing from replacement macro reference");
			}
			if ((percent = (wchar_t *) wschr(colon + 1,
							 (int) percent_char)) == NULL) {
				fatal_reader_mksh("%% missing from replacement macro reference");
			}
			if (eq < percent) {
				fatal_reader_mksh("%% missing from replacement macro reference");
			}

			if (percent > (colon + 1)) {
				tmp_len = percent - colon;
				if(left_head) {
					retmem(left_head);
				}
				left_head = ALLOC_WC(tmp_len);
				wsncpy(left_head,
					      colon + 1,
					      percent - colon - 1);
				left_head[percent-colon-1] = (int) nul_char;
				left_head_len = percent-colon-1;
			} else {
				left_head = NULL;
				left_head_len = 0;
			}

			if (eq > percent+1) {
				tmp_len = eq - percent;
				if(left_tail) {
					retmem(left_tail);
				}
				left_tail = ALLOC_WC(tmp_len);
				wsncpy(left_tail,
					      percent + 1,
					      eq - percent - 1);
				left_tail[eq-percent-1] = (int) nul_char;
				left_tail_len = eq-percent-1;
			} else {
				left_tail = NULL;
				left_tail_len = 0;
			}

			if ((percent = (wchar_t *) wschr(++eq,
							 (int) percent_char)) == NULL) {

				right_hand[0] = ALLOC_WC(wslen(eq) + 1);
				right_hand[1] = NULL;
				wscpy(right_hand[0], eq);
			} else {
				i = 0;
				do {
					right_hand[i] = ALLOC_WC(percent-eq+1);
					wsncpy(right_hand[i],
						      eq,
						      percent - eq);
					right_hand[i][percent-eq] =
					  (int) nul_char;
					if (i++ >= VSIZEOF(right_hand)) {
						fatal_mksh("Too many %% in pattern");
					}
					eq = percent + 1;
					if (eq[0] == (int) nul_char) {
						MBSTOWCS(wcs_buffer, "");
						right_hand[i] = (wchar_t *) wsdup(wcs_buffer);
						i++;
						break;
					}
				} while ((percent = (wchar_t *) wschr(eq, (int) percent_char)) != NULL);
				if (eq[0] != (int) nul_char) {
					right_hand[i] = ALLOC_WC(wslen(eq) + 1);
					wscpy(right_hand[i], eq);
					i++;
				}
				right_hand[i] = NULL;
			}
			replacement = pattern_replace;
		}
	}
	if (!sun_style && !svr4) {
		wchar_t	*args;
		if ((args = wschr(string.buffer.start, space_char)) != NULL) {
			*args++ = 0;
			while (*args == space_char)
				args++;
			if (expand_function(string.buffer.start, args,
				destination) == true)
				return;
		}
	}
	if (name == NULL) {
		/*
		 * No translations found.
		 * Use the whole string as the macro name.
		 */
		name = GETNAME(string.buffer.start,
			       string.text.p - string.buffer.start);
	}
	if (string.free_after_use) {
		retmem(string.buffer.start);
	}
	if (name == make) {
		make_word_mentioned = true;
	}
	if (name == query) {
		query_mentioned = true;
	}
	if ((name == host_arch) || (name == target_arch)) {
		if (!init_arch_done) {
			init_arch_done = true;
			init_arch_macros();
		}
	}
	if ((name == host_mach) || (name == target_mach)) {
		if (!init_mach_done) {
			init_mach_done = true;
			init_mach_macros();
		}
	}
	/* Get the macro value. */
	macro = get_prop(name->prop, macro_prop);
#ifdef NSE
        if (nse_watch_vars && nse && macro != NULL) {
                if (macro->body.macro.imported) {
                        nse_shell_var_used= name;
		}
                if (macro->body.macro.value != NULL){
	              if (nse_backquotes(macro->body.macro.value->string)) {
	                       nse_backquote_seen= name;
		      }
	       }
	}
#endif
	if ((macro != NULL) && macro->body.macro.is_conditional) {
		conditional_macro_used = true;
		/*
		 * Add this conditional macro to the beginning of the
		 * global list.
		 */
		add_macro_to_global_list(name);
		if (makefile_type == reading_makefile) {
			warning_mksh("Conditional macro `%s' referenced in file `%ls', line %d",
					name->string_mb, file_being_read, line_number);
		}
	}
	/* Macro name read and parsed. Expand the value. */
	if ((macro == NULL) || (macro->body.macro.value == NULL)) {
		/* If the value is empty, we just get out of here. */
		goto exit;
	}
	if (replacement == sh_replace) {
		/* If we should do a :sh transform, we expand the command
		 * and process it.
		 */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer and run cmd. */
		expand_value_with_daemon(name, macro, &string, cmd);
		sh_command2string(&string, destination);
	} else if ((replacement != no_replace) || (extraction != no_extract)) {
		/*
		 * If there were any transforms specified in the macro
		 * name, we deal with them here.
		 */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer. */
		expand_value_with_daemon(name, macro, &string, cmd);
		/* Scan the expanded string. */
		p = string.buffer.start;
		while (*p != (int) nul_char) {
			wchar_t		chr = 0;

			/*
			 * First skip over any white space and append
			 * that to the destination string.
			 */
			block_start = p;
			while ((*p != (int) nul_char) && iswspace(*p)) {
				p++;
			}
			append_string(block_start,
				      destination,
				      p - block_start);
			/* Then find the end of the next word. */
			block_start = p;
			while ((*p != (int) nul_char) && !iswspace(*p)) {
				p++;
			}
			/* If we cant find another word we are done */
			if (block_start == p) {
				break;
			}
			/* Then apply the transforms to the word */
			INIT_STRING_FROM_STACK(extracted, extracted_string);
			switch (extraction) {
			case dir_extract:
				/*
				 * $(@D) type transform. Extract the
				 * path from the word. Deliver "." if
				 * none is found.
				 */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = (wchar_t *) wsrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					MBSTOWCS(wcs_buffer, ".");
					append_string(wcs_buffer, &extracted, 1);
				} else {
					append_string(block_start,
						      &extracted,
						      eq - block_start);
				}
				break;
			case file_extract:
				/*
				 * $(@F) type transform. Remove the path
				 * from the word if any.
				 */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = (wchar_t *) wsrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					append_string(block_start,
						      &extracted,
						      p - block_start);
				} else {
					append_string(eq + 1,
						      &extracted,
						      p - eq - 1);
				}
				break;
			case no_extract:
				append_string(block_start,
					      &extracted,
					      p - block_start);
				break;
			}
			switch (replacement) {
			case suffix_replace:
				/*
				 * $(FOO:.o=.c) type transform.
				 * Maybe replace the tail of the word.
				 */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_tail_len) &&
				    IS_WEQUALN(extracted.text.p - left_tail_len,
					      left_tail,
					      left_tail_len)) {
					append_string(extracted.buffer.start,
						      destination,
						      (extracted.text.p -
						       extracted.buffer.start)
						      - left_tail_len);
					append_string(right_tail,
						      destination,
						      FIND_LENGTH);
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case pattern_replace:
				/* $(X:a%b=c%d) type transform. */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_head_len+left_tail_len) &&
				    IS_WEQUALN(left_head,
					      extracted.buffer.start,
					      left_head_len) &&
				    IS_WEQUALN(left_tail,
					      extracted.text.p - left_tail_len,
					      left_tail_len)) {
					i = 0;
					while (right_hand[i] != NULL) {
						append_string(right_hand[i],
							      destination,
							      FIND_LENGTH);
						i++;
						if (right_hand[i] != NULL) {
							append_string(extracted.buffer.
								      start +
								      left_head_len,
								      destination,
								      (extracted.text.p - extracted.buffer.start)-left_head_len-left_tail_len);
						}
					}
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case no_replace:
				append_string(extracted.buffer.start,
					      destination,
					      FIND_LENGTH);
				break;
			case sh_replace:
				break;
			    }
		}
		if (string.free_after_use) {
			retmem(string.buffer.start);
		}
	} else {
		/*
		 * This is for the case when the macro name did not
		 * specify transforms.
		 */
		if (!strncmp(name->string_mb, NOCATGETS("GET"), 3)) {
			dollarget_seen = true;
		}
		dollarless_flag = false;
		if (!strncmp(name->string_mb, "<", 1) &&
		    dollarget_seen) {
			dollarless_flag = true;
			dollarget_seen = false;
		}
		expand_value_with_daemon(name, macro, destination, cmd);
	}
exit:
	if(left_tail) {
		retmem(left_tail);
	}
	if(right_tail) {
		retmem(right_tail);
	}
	if(left_head) {
		retmem(left_head);
	}
	i = 0;
	while (right_hand[i] != NULL) {
		retmem(right_hand[i]);
		i++;
	}
	*destination->text.p = (int) nul_char;
	destination->text.end = destination->text.p;
}

static void
add_macro_to_global_list(Name macro_to_add)
{
	Macro_list	new_macro;
	Macro_list	macro_on_list;
	char		*name_on_list = (char*)NULL;
	char		*name_to_add = macro_to_add->string_mb;
	char		*value_on_list = (char*)NULL;
	char		*value_to_add = (char*)NULL;

	if (macro_to_add->prop->body.macro.value != NULL) {
		value_to_add = macro_to_add->prop->body.macro.value->string_mb;
	} else {
		value_to_add = "";
	}

	/* 
	 * Check if this macro is already on list, if so, do nothing
	 */
	for (macro_on_list = cond_macro_list;
	     macro_on_list != NULL;
	     macro_on_list = macro_on_list->next) {

		name_on_list = macro_on_list->macro_name;
		value_on_list = macro_on_list->value;

		if (IS_EQUAL(name_on_list, name_to_add)) {
			if (IS_EQUAL(value_on_list, value_to_add)) {
				return;
			}
		}
	}
	new_macro = (Macro_list) malloc(sizeof(Macro_list_rec));
	new_macro->macro_name = strdup(name_to_add);
	new_macro->value = strdup(value_to_add);
	new_macro->next = cond_macro_list;
	cond_macro_list = new_macro;
}

/*
 *	init_arch_macros(void)
 *
 *	Set the magic macros TARGET_ARCH, HOST_ARCH,
 *
 *	Parameters: 
 *
 *	Global variables used:
 * 	                        host_arch   Property for magic macro HOST_ARCH
 * 	                        target_arch Property for magic macro TARGET_ARCH
 *
 *	Return value:
 *				The function does not return a value, but can
 *				call fatal() in case of error.
 */
static void
init_arch_macros(void)
{
	String_rec	result_string;
	wchar_t		wc_buf[STRING_BUFFER_LENGTH];
	char		mb_buf[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		value;
	int		set_host, set_target;
#ifdef NSE
	Property	macro;
#endif
#ifndef __sun
	const char	*mach_command = NOCATGETS("/bin/uname -p");
#else
	const char	*mach_command = NOCATGETS("/bin/mach");
#endif

	set_host = (get_prop(host_arch->prop, macro_prop) == NULL);
	set_target = (get_prop(target_arch->prop, macro_prop) == NULL);

	if (set_host || set_target) {
		INIT_STRING_FROM_STACK(result_string, wc_buf);
		append_char((int) hyphen_char, &result_string);

		if ((pipe = popen(mach_command, "r")) == NULL) {
			fatal_mksh("Execute of %s failed", mach_command);
		}
		while (fgets(mb_buf, sizeof(mb_buf), pipe) != NULL) {
			MBSTOWCS(wcs_buffer, mb_buf);
			append_string(wcs_buffer, &result_string, wslen(wcs_buffer));
		}
		if (pclose(pipe) != 0) {
			fatal_mksh("Execute of %s failed", mach_command);
		}

		value = GETNAME(result_string.buffer.start, wslen(result_string.buffer.start));

#ifdef NSE
	        macro = setvar_daemon(host_arch, value, false, no_daemon, true, 0);
	        macro->body.macro.imported= true;
	        macro = setvar_daemon(target_arch, value, false, no_daemon, true, 0);
	        macro->body.macro.imported= true;
#else
		if (set_host) {
			setvar_daemon(host_arch, value, false, no_daemon, true, 0);
		}
		if (set_target) {
			setvar_daemon(target_arch, value, false, no_daemon, true, 0);
		}
#endif
	}
}

/*
 *	init_mach_macros(void)
 *
 *	Set the magic macros TARGET_MACH, HOST_MACH,
 *
 *	Parameters: 
 *
 *	Global variables used:
 * 	                        host_mach   Property for magic macro HOST_MACH
 * 	                        target_mach Property for magic macro TARGET_MACH
 *
 *	Return value:
 *				The function does not return a value, but can
 *				call fatal() in case of error.
 */
static void
init_mach_macros(void)
{
	String_rec	result_string;
	wchar_t		wc_buf[STRING_BUFFER_LENGTH];
	char		mb_buf[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		value;
	int		set_host, set_target;
	const char	*arch_command = NOCATGETS("/bin/arch");

	set_host = (get_prop(host_mach->prop, macro_prop) == NULL);
	set_target = (get_prop(target_mach->prop, macro_prop) == NULL);

	if (set_host || set_target) {
		INIT_STRING_FROM_STACK(result_string, wc_buf);
		append_char((int) hyphen_char, &result_string);

		if ((pipe = popen(arch_command, "r")) == NULL) {
			fatal_mksh("Execute of %s failed", arch_command);
		}
		while (fgets(mb_buf, sizeof(mb_buf), pipe) != NULL) {
			MBSTOWCS(wcs_buffer, mb_buf);
			append_string(wcs_buffer, &result_string, wslen(wcs_buffer));
		}
		if (pclose(pipe) != 0) {
			fatal_mksh("Execute of %s failed", arch_command);
		}

		value = GETNAME(result_string.buffer.start, wslen(result_string.buffer.start));

		if (set_host) {
			setvar_daemon(host_mach, value, false, no_daemon, true, 0);
		}
		if (set_target) {
			setvar_daemon(target_mach, value, false, no_daemon, true, 0);
		}
	}
}

/*
 *	expand_value_with_daemon(name, macro, destination, cmd)
 *
 *	Checks for daemons and then maybe calls expand_value().
 *
 *	Parameters:
 *              name            Name of the macro  (Added by the NSE)
 *		macro		The property block with the value to expand
 *		destination	Where the result should be deposited
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 */
static void
#ifdef NSE
expand_value_with_daemon(Name name, register Property macro, register String destination, Boolean cmd)
#else
expand_value_with_daemon(Name, register Property macro, register String destination, Boolean cmd)
#endif
{
	register Chain		chain;

#ifdef NSE
        if (reading_dependencies) {
                /*
                 * Processing the dependencies themselves
                 */
                depvar_dep_macro_used(name);
	} else {
                /*
	         * Processing the rules for the targets
	         * the nse_watch_vars flags chokes off most
	         * checks.  it is true only when processing
	         * the output from a recursive make run
	         * which is all we are interested in here.
	         */
	         if (nse_watch_vars) {
	                depvar_rule_macro_used(name);
		}
	 }
#endif

	switch (macro->body.macro.daemon) {
	case no_daemon:
		if (!svr4 && !posix) {
			expand_value(macro->body.macro.value, destination, cmd);
		} else {
			if (dollarless_flag && tilde_rule) {
				expand_value(dollarless_value, destination, cmd);
				dollarless_flag = false;
				tilde_rule = false;
			} else {
				expand_value(macro->body.macro.value, destination, cmd);
			}
		}
		return;
	case chain_daemon:
		/* If this is a $? value we call the daemon to translate the */
		/* list of names to a string */
		for (chain = (Chain) macro->body.macro.value;
		     chain != NULL;
		     chain = chain->next) {
			APPEND_NAME(chain->name,
				      destination,
				      (int) chain->name->hash.length);
			if (chain->next != NULL) {
				append_char((int) space_char, destination);
			}
		}
		return;
	}
}

/*
 * We use a permanent buffer to reset SUNPRO_DEPENDENCIES value.
 */
char	*sunpro_dependencies_buf = NULL;
char	*sunpro_dependencies_oldbuf = NULL;
int	sunpro_dependencies_buf_size = 0;

/*
 *	setvar_daemon(name, value, append, daemon, strip_trailing_spaces)
 *
 *	Set a macro value, possibly supplying a daemon to be used
 *	when referencing the value.
 *
 *	Return value:
 *				The property block with the new value
 *
 *	Parameters:
 *		name		Name of the macro to set
 *		value		The value to set
 *		append		Should we reset or append to the current value?
 *		daemon		Special treatment when reading the value
 *		strip_trailing_spaces from the end of value->string
 *		debug_level	Indicates how much tracing we should do
 *
 *	Global variables used:
 *		makefile_type	Used to check if we should enforce read only
 *		path_name	The Name "PATH", compared against
 *		virtual_root	The Name "VIRTUAL_ROOT", compared against
 *		vpath_defined	Set if the macro VPATH is set
 *		vpath_name	The Name "VPATH", compared against
 *		envvar		A list of environment vars with $ in value
 */
Property
setvar_daemon(register Name name, register Name value, Boolean append, Daemon daemon, Boolean strip_trailing_spaces, short debug_level)
{
	register Property	macro = maybe_append_prop(name, macro_prop);
	register Property	macro_apx = get_prop(name->prop, macro_append_prop);
	int			length = 0;
	String_rec		destination;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	register Chain		chain;
	Name			val;
	wchar_t			*val_string = (wchar_t*)NULL;
	Wstring			wcb;

#ifdef NSE
        macro->body.macro.imported = false;
#endif

	if ((makefile_type != reading_nothing) &&
	    macro->body.macro.read_only) {
		return macro;
	}
	/* Strip spaces from the end of the value */
	if (daemon == no_daemon) {
		if(value != NULL) {
			wcb.init(value);
			length = wcb.length();
			val_string = wcb.get_string();
		}
		if ((length > 0) && iswspace(val_string[length-1])) {
			INIT_STRING_FROM_STACK(destination, buffer);
			buffer[0] = 0;
			append_string(val_string, &destination, length);
			if (strip_trailing_spaces) {
				while ((length > 0) &&
				       iswspace(destination.buffer.start[length-1])) {
					destination.buffer.start[--length] = 0;
				}
			}
			value = GETNAME(destination.buffer.start, FIND_LENGTH);
		}
	}
		
	if(macro_apx != NULL) {
		val = macro_apx->body.macro_appendix.value;
	} else {
		val = macro->body.macro.value;
	}

	if (append) {
		/*
		 * If we are appending, we just tack the new value after
		 * the old one with a space in between.
		 */
		INIT_STRING_FROM_STACK(destination, buffer);
		buffer[0] = 0;
		if ((macro != NULL) && (val != NULL)) {
			APPEND_NAME(val,
				      &destination,
				      (int) val->hash.length);
			if (value != NULL) {
				wcb.init(value);
				if(wcb.length() > 0) {
					MBTOWC(wcs_buffer, " ");
					append_char(wcs_buffer[0], &destination);
				}
			}
		}
		if (value != NULL) {
			APPEND_NAME(value,
				      &destination,
				      (int) value->hash.length);
		}
		value = GETNAME(destination.buffer.start, FIND_LENGTH);
		wcb.init(value);
		if (destination.free_after_use) {
			retmem(destination.buffer.start);
		}
	}

	/* Debugging trace */
	if (debug_level > 1) {
		if (value != NULL) {
			switch (daemon) {
			case chain_daemon:
				printf("%s =", name->string_mb);
				for (chain = (Chain) value;
				     chain != NULL;
				     chain = chain->next) {
					printf(" %s", chain->name->string_mb);
				}
				printf("\n");
				break;
			case no_daemon:
				printf("%s= %s\n",
					      name->string_mb,
					      value->string_mb);
				break;
			}
		} else {
			printf("%s =\n", name->string_mb);
		}
	}
	/* Set the new values in the macro property block */
/**/
	if(macro_apx != NULL) {
		macro_apx->body.macro_appendix.value = value;
		INIT_STRING_FROM_STACK(destination, buffer);
		buffer[0] = 0;
		if (value != NULL) {
			APPEND_NAME(value,
				      &destination,
				      (int) value->hash.length);
			if (macro_apx->body.macro_appendix.value_to_append != NULL) {
				MBTOWC(wcs_buffer, " ");
				append_char(wcs_buffer[0], &destination);
			}
		}
		if (macro_apx->body.macro_appendix.value_to_append != NULL) {
			APPEND_NAME(macro_apx->body.macro_appendix.value_to_append,
				      &destination,
				      (int) macro_apx->body.macro_appendix.value_to_append->hash.length);
		}
		value = GETNAME(destination.buffer.start, FIND_LENGTH);
		if (destination.free_after_use) {
			retmem(destination.buffer.start);
		}
	}
/**/
	macro->body.macro.value = value;
	macro->body.macro.daemon = daemon;
	/*
	 * If the user changes the VIRTUAL_ROOT, we need to flush
	 * the vroot package cache.
	 */
	if (name == path_name) {
		flush_path_cache();
	}
	if (name == virtual_root) {
		flush_vroot_cache();
	}
	/* If this sets the VPATH we remember that */
	if ((name == vpath_name) &&
	    (value != NULL) &&
	    (value->hash.length > 0)) {
		vpath_defined = true;
	}
	/*
	 * For environment variables we also set the
	 * environment value each time.
	 */
	if (macro->body.macro.exported) {
		static char	*env;

#ifdef DISTRIBUTED
		if (!reading_environment && (value != NULL)) {
#else
		if (!reading_environment && (value != NULL) && value->dollar) {
#endif
			Envvar	p;

			for (p = envvar; p != NULL; p = p->next) {
				if (p->name == name) {
					p->value = value;
					p->already_put = false;
					goto found_it;
				}
			}
			p = ALLOC(Envvar);
			p->name = name;
			p->value = value;
			p->next = envvar;
			p->env_string = NULL;
			p->already_put = false;
			envvar = p;
found_it:;
#ifdef DISTRIBUTED
		}
		if (reading_environment || (value == NULL) || !value->dollar) {
#else
		} else {
#endif
			length = 2 + strlen(name->string_mb);
			if (value != NULL) {
				length += strlen(value->string_mb);
			}
			Property env_prop = maybe_append_prop(name, env_mem_prop);
			/*
			 * We use a permanent buffer to reset SUNPRO_DEPENDENCIES value.
			 */
			if (!strncmp(name->string_mb, NOCATGETS("SUNPRO_DEPENDENCIES"), 19)) {
				if (length >= sunpro_dependencies_buf_size) {
					sunpro_dependencies_buf_size=length*2;
					if (sunpro_dependencies_buf_size < 4096)
						sunpro_dependencies_buf_size = 4096; // Default minimum size
					if (sunpro_dependencies_buf)
						sunpro_dependencies_oldbuf = sunpro_dependencies_buf;
					sunpro_dependencies_buf=getmem(sunpro_dependencies_buf_size);
				}
				env = sunpro_dependencies_buf;
			} else {
				env = getmem(length);
			}
			env_alloc_num++;
			env_alloc_bytes += length;
			sprintf(env,
				       "%s=%s",
				       name->string_mb,
				       value == NULL ?
			                 "" : value->string_mb);
			putenv(env);
			env_prop->body.env_mem.value = env;
			if (sunpro_dependencies_oldbuf) {
				/* Return old buffer */
				retmem_mb(sunpro_dependencies_oldbuf);
				sunpro_dependencies_oldbuf = NULL;
			}
		}
	}
	if (name == target_arch) {
		Name		ha = getvar(host_arch);
		Name		ta = getvar(target_arch);
		Name		vr = getvar(virtual_root);
		int		length;
		wchar_t		*new_value;
		wchar_t		*old_vr;
		Boolean		new_value_allocated = false;

		Wstring		ha_str(ha);
		Wstring		ta_str(ta);
		Wstring		vr_str(vr);

		wchar_t * wcb_ha = ha_str.get_string();
		wchar_t * wcb_ta = ta_str.get_string();
		wchar_t * wcb_vr = vr_str.get_string();

		length = 32 +
		  wslen(wcb_ha) +
		    wslen(wcb_ta) +
		      wslen(wcb_vr);
		old_vr = wcb_vr;
		MBSTOWCS(wcs_buffer, NOCATGETS("/usr/arch/"));
		if (IS_WEQUALN(old_vr,
			       wcs_buffer,
			       wslen(wcs_buffer))) {
			old_vr = (wchar_t *) wschr(old_vr, (int) colon_char) + 1;
		}
		if ( (ha == ta) || (wslen(wcb_ta) == 0) ) {
			new_value = old_vr;
		} else {
			new_value = ALLOC_WC(length);
			new_value_allocated = true;
			WCSTOMBS(mbs_buffer, old_vr);
#ifdef __sun
			wsprintf(new_value,
				        NOCATGETS("/usr/arch/%s/%s:%s"),
				        ha->string_mb + 1,
				        ta->string_mb + 1,
				        mbs_buffer);
#else
			char * mbs_new_value = (char *)getmem(length);
			sprintf(mbs_new_value,
				        NOCATGETS("/usr/arch/%s/%s:%s"),
				        ha->string_mb + 1,
				        ta->string_mb + 1,
				        mbs_buffer);
			MBSTOWCS(new_value, mbs_new_value);
			retmem_mb(mbs_new_value);
#endif
		}
		if (new_value[0] != 0) {
			setvar_daemon(virtual_root,
					     GETNAME(new_value, FIND_LENGTH),
					     false,
					     no_daemon,
					     true,
					     debug_level);
		}
		if (new_value_allocated) {
			retmem(new_value);
		}
	}
	return macro;
}

/*
 * GNU-style function handling support: $(function arguments)
 */
static wchar_t *
skip_space(wchar_t *s)
{
	while (*s == space_char)
		s++;
	while (*s && *s != space_char)
		s++;
	if (*s != 0) {
		*s++ = 0;
		while (*s == space_char)
			s++;
	}
	return s;
}

static wchar_t *
skip_comma(wchar_t *s)
{
	while (*s && *s != comma_char)
		s++;
	if (*s != 0) {
		*s++ = 0;
		return s;
	} else
		return NULL;
}

static wchar_t *
h_dir(wchar_t *args)
{
	wchar_t	*ap, *sp;

	sp = NULL;
	for (ap = args; *ap; ap++)
		if (*ap == slash_char)
			sp = ap;
	if (sp != NULL)
		sp[1] = 0;
	else
		args = L"./";
	return args;
}

static wchar_t *
h_notdir(wchar_t *args)
{
	wchar_t	*ap;

	for (ap = args; *ap; ap++)
		if (*ap == slash_char)
			args = &ap[1];
	return args;
}

static wchar_t *
h_suffix(wchar_t *args)
{
	wchar_t	*ap;

	ap = args;
	args = L"";
	for ( ; *ap; ap++)
		if (*ap == period_char)
			args = ap;
	return args;
}

static wchar_t *
h_basename(wchar_t *args)
{
	wchar_t	*ap, *sp, *pp;

	sp = args;
	for (ap = args; *ap; ap++)
		if (*ap == slash_char)
			sp = ap;
	pp = NULL;
	for ( ; *sp; sp++)
		if (*sp == period_char)
			pp = sp;
	if (pp)
		*pp = 0;
	return args;
}

static Boolean
f_sep_space(wchar_t *args, String destination, wchar_t *(*handler)(wchar_t *))
{
	wchar_t	*np;
	int	c = 0;

	do {
		np = skip_space(args);
		args = handler(args);
		if (c++)
			append_char(space_char, destination);
		append_string(args, destination, FIND_LENGTH);
		args = np;
	} while (*args);
	return true;
}

static Boolean
f_addsuffix(wchar_t *args, String destination)
{
	wchar_t	*np, *suffix;
	int	c = 0;

	suffix = args;
	if ((np = skip_comma(args)) == NULL)
		fatal_reader_mksh("addsuffix: no suffix specified");
	args = np;
	do {
		np = skip_space(args);
		if (c++)
			append_char(space_char, destination);
		append_string(args, destination, FIND_LENGTH);
		append_string(suffix, destination, FIND_LENGTH);
		args = np;
	} while (*args);
	return true;
}

static Boolean
f_addprefix(wchar_t *args, String destination)
{
	wchar_t	*np, *prefix;
	int	c = 0;

	prefix = args;
	if ((np = skip_comma(args)) == NULL)
		fatal_reader_mksh("addprefix: no prefix specified");
	args = np;
	do {
		np = skip_space(args);
		if (c++)
			append_char(space_char, destination);
		append_string(prefix, destination, FIND_LENGTH);
		append_string(args, destination, FIND_LENGTH);
		args = np;
	} while (*args);
	return true;
}

static Boolean
f_join(wchar_t *args, String destination)
{
	wchar_t	*ap, *join, *jp;
	int	c = 0;

	if ((join = skip_comma(args)) == NULL)
		fatal_reader_mksh("join: nothing to join");
	do {
		ap = skip_space(args);
		jp = skip_space(join);
		if (c++)
			append_char(space_char, destination);
		append_string(args, destination, FIND_LENGTH);
		append_string(join, destination, FIND_LENGTH);
		if (*ap)
			args = ap;
		if (*jp)
			join = jp;
	} while (*ap || *jp);
	return true;
}

static Boolean
f_wildcard(wchar_t *args, String destination)
{
	wordexp_t	we;
	size_t	n;
	char	*cp;
	char	**rp;

	n = MB_CUR_MAX * wcslen(args) + 1;
	cp = (char *)alloca(n);
	if (wcstombs(cp, args, n) == (size_t)-1 ||
			wordexp(cp, &we, WRDE_NOCMD) != 0)
		fatal_reader_mksh("wildcard: syntax error");
	for (rp = we.we_wordv; *rp; rp++) {
		if (rp != we.we_wordv)
			append_char(space_char, destination);
		append_string(*rp, destination, FIND_LENGTH);
	}
	wordfree(&we);
	return true;
}

static Boolean
f_subst(wchar_t *args, String destination)
{
	wchar_t	*from, *to, *text = NULL, *tp, *pp;
	size_t	fn, tn;

	from = args;
	if ((to = skip_comma(from)) == NULL || (text = skip_comma(to)) == NULL)
		fatal_reader_mksh("subst: missing argument");
	fn = wcslen(from);
	tn = wcslen(to);
	tp = text;
	while ((pp = wcsstr(tp, from)) != NULL) {
		append_string(tp, destination, pp - tp);
		append_string(to, destination, tn);
		tp = &pp[fn];
	}
	append_string(tp, destination, FIND_LENGTH);
	return true;
}

static Boolean
patsplit(wchar_t *pp, wchar_t **left, wchar_t **right)
{
	wchar_t	*op;
	Boolean	found = false;

	*right = L"";
	*left = pp;
	op = pp;
	while (*pp) {
		if (*pp == percent_char) {
			found = true;
			*pp++ = 0;
			*op = 0;
			*right = pp;
			op = pp;
			continue;
		}
		if (pp[0] == backslash_char && pp[1])
			pp++;
		*op++ = *pp++;
	}
	return found;
}

static Boolean
f_patsubst(wchar_t *args, String destination)
{
	wchar_t	*pattern, *replacement, *text = NULL;
	wchar_t	*pleft, *pright;
	wchar_t	*rleft, *rright;
	wchar_t	*np, *ap, *pp;
	int	c = 0;
	Boolean	pperc, rperc;
	size_t	pn;

	pattern = args;
	if ((replacement = skip_comma(args)) == NULL ||
			(text = skip_comma(replacement)) == NULL)
		fatal_reader_mksh("patsubst: missing argument");
	pperc = patsplit(pattern, &pleft, &pright);
	rperc = patsplit(replacement, &rleft, &rright);
	pn = wcslen(pleft);
	do {
		np = skip_space(text);
		if (c++)
			append_char(space_char, destination);
		if (pperc) {
			if (wcsncmp(text, pleft, pn))
				append_string(text, destination, FIND_LENGTH);
			else {
				ap = &text[pn];
				pp = NULL;
				while (*ap) {
					if (wcscmp(ap, pright) == 0)
						pp = ap;
					ap++;
				}
				if (pp == NULL)
					append_string(text, destination,
						FIND_LENGTH);
				else {
					append_string(rleft, destination,
						FIND_LENGTH);
					if (rperc)
						append_string(&text[pn],
							destination,
							pp - &text[pn]);
					append_string(rright, destination,
						FIND_LENGTH);
				}
			}
		} else {
			if (wcscmp(text, pleft) == 0) {
				append_string(rleft, destination, FIND_LENGTH);
				if (rperc)
					append_char(percent_char, destination);
				append_string(rright, destination, FIND_LENGTH);
			} else
				append_string(text, destination, FIND_LENGTH);
		}
		text = np;
	} while (*text);
	return true;
}

static wchar_t *
h_strip(wchar_t *args)
{
	return args;
}

static Boolean
f_findstring(wchar_t *args, String destination)
{
	wchar_t	*find, *in;

	find = args;
	if ((in = skip_comma(args)) == NULL)
		fatal_reader_mksh("findstring: missing argument");
	if (wcsstr(in, find) != NULL)
		append_string(find, destination, FIND_LENGTH);
	return true;
}

static Boolean
f_filter(wchar_t *args, String destination, Boolean invert)
{
	wchar_t	*text, *tp, *pp, *ap;
	wchar_t	**pleft = NULL, **pright = NULL;
	Boolean	*pperc = NULL;
	size_t	i, n, pn;
	Boolean	found;
	int	c = 0;

	if ((text = skip_comma(args)) == NULL)
		fatal_reader_mksh("%s: missing argument",
				invert ? "filter-out" : "filter");
	n = 0;
	do {
		pp = skip_space(args);
		pleft = (wchar_t **)realloc(pleft,
				(n+1) * sizeof *pleft);
		pright = (wchar_t **)realloc(pright,
				(n+1) * sizeof *pright);
		pperc = (Boolean *)realloc(pperc,
				(n+1) * sizeof *pperc);
		pperc[n] = patsplit(args, &pleft[n], &pright[n]);
		n++;
		args = pp;
	} while (*args);
	if (n == 0)
		return true;
	do {
		tp = skip_space(text);
		found = false;
		for (i = 0; i < n; i++) {
			if (pperc[i]) {
				pn = wcslen(pleft[i]);
				if (wcsncmp(text, pleft[i], pn) == 0) {
					ap = &text[pn];
					while (*ap) {
						if (wcscmp(ap, pright[i]) == 0){
							found = true;
							break;
						}
						ap++;
					}
				}
			} else {
				if (wcscmp(text, pleft[i]) == 0)
					found = true;
			}
			if (found == true)
				break;
		}
		if (found != invert) {
			if (c++)
				append_char(space_char, destination);
			append_string(text, destination, FIND_LENGTH);
		}
		text = tp;
	} while (*text);
	return true;
}

static int
f_sort_cmp(const void *_p, const void *_q)
{
	wchar_t	*p, *q;

	p = (wchar_t *)*(const void **)_p;
	q = (wchar_t *)*(const void **)_q;
	return wcscmp(p, q);
}

static Boolean
f_sort(wchar_t *args, String destination)
{
	wchar_t	**list = NULL;
	wchar_t	*ap;
	size_t	i, n = 0;
	int	c = 0;

	do {
		ap = skip_space(args);
		list = (wchar_t **)realloc(list, (n+1) * sizeof *list);
		list[n++] = args;
		args = ap;
	} while (*args);
	qsort(list, n, sizeof *list, f_sort_cmp);
	for (i = 0; i < n; i++) {
		if (i == 0 || wcscmp(list[i], list[i-1])) {
			if (c++)
				append_char(space_char, destination);
			append_string(list[i], destination, FIND_LENGTH);
		}
	}
	return true;
}

static Boolean
s_word(size_t n, size_t m, wchar_t *text, String destination)
{
	wchar_t	*tp;
	size_t	i;
	int	c = 0;

	for (i = 1; i <= m; i++) {
		tp = skip_space(text);
		if (i >= n) {
			if (c++)
				append_char(space_char, destination);
			append_string(text, destination, FIND_LENGTH);
		}
		text = tp;
		if (*text == 0)
			break;
	}
	return true;
}

static Boolean
f_word(wchar_t *args, String destination)
{
	wchar_t	*text, *xp;
	unsigned long	n;

	if ((text = skip_comma(args)) == NULL)
		fatal_reader_mksh("word: missing argument");
	n = wcstoul(args, &xp, 10);
	if (*xp)
		fatal_reader_mksh("word: \"%s\" is not a number", args);
	return s_word(n, n, text, destination);
}

static Boolean
f_wordlist(wchar_t *args, String destination)
{
	wchar_t	*end, *text = NULL, *xp;
	unsigned long	s, e;

	if ((end = skip_comma(args)) == NULL ||
			(text = skip_comma(end)) == NULL)
		fatal_reader_mksh("word: missing argument");
	s = wcstoul(args, &xp, 10);
	if (*xp)
		fatal_reader_mksh("word: \"%s\" is not a number", args);
	e = wcstoul(end, &xp, 10);
	if (*xp)
		fatal_reader_mksh("word: \"%s\" is not a number", end);
	return s_word(s, e, text, destination);
}

static Boolean
f_words(wchar_t *args, String destination)
{
	wchar_t	*np;
	char	buf[40];
	unsigned long	n = 0;

	do {
		np = skip_space(args);
		if (*args)
			n++;
		args = np;
	} while (*args);
	snprintf(buf, sizeof buf, "%lu", n);
	append_string(buf, destination, FIND_LENGTH);
	return true;
}

static Boolean
f_shell(wchar_t *args, String destination)
{
	String_rec	string;

	INIT_STRING_FROM_STACK(string, args);
	sh_command2string(&string, destination);
	return true;
}

extern const char	*progname;

static Boolean
f_error(wchar_t *args, String destination)
{
	fprintf(stderr, "%s: *** %ls\n", progname, args);
#ifdef SUN5_0
	exit_status = 2;
#endif
	exit(2);
	/*NOTREACHED*/
	return true;
}

static Boolean
f_warning(wchar_t *args, String destination)
{
	fprintf(stderr, "%s: %ls\n", progname, args);
	return true;
}

static Boolean
expand_function(wchar_t *func, wchar_t *args, String destination)
{
	if (wcscmp(func, L"dir") == 0)
		return f_sep_space(args, destination, h_dir);
	if (wcscmp(func, L"notdir") == 0)
		return f_sep_space(args, destination, h_notdir);
	if (wcscmp(func, L"suffix") == 0)
		return f_sep_space(args, destination, h_suffix);
	if (wcscmp(func, L"basename") == 0)
		return f_sep_space(args, destination, h_basename);
	if (wcscmp(func, L"addsuffix") == 0)
		return f_addsuffix(args, destination);
	if (wcscmp(func, L"addprefix") == 0)
		return f_addprefix(args, destination);
	if (wcscmp(func, L"join") == 0)
		return f_join(args, destination);
	if (wcscmp(func, L"wildcard") == 0)
		return f_wildcard(args, destination);
	if (wcscmp(func, L"subst") == 0)
		return f_subst(args, destination);
	if (wcscmp(func, L"patsubst") == 0)
		return f_patsubst(args, destination);
	if (wcscmp(func, L"strip") == 0)
		return f_sep_space(args, destination, h_strip);
	if (wcscmp(func, L"findstring") == 0)
		return f_findstring(args, destination);
	if (wcscmp(func, L"filter") == 0)
		return f_filter(args, destination, false);
	if (wcscmp(func, L"filter-out") == 0)
		return f_filter(args, destination, true);
	if (wcscmp(func, L"sort") == 0)
		return f_sort(args, destination);
	if (wcscmp(func, L"word") == 0)
		return f_word(args, destination);
	if (wcscmp(func, L"wordlist") == 0)
		return f_wordlist(args, destination);
	if (wcscmp(func, L"words") == 0)
		return f_words(args, destination);
	if (wcscmp(func, L"firstword") == 0)
		return s_word(1, 1, args, destination);
	if (wcscmp(func, L"shell") == 0)
		return f_shell(args, destination);
	if (wcscmp(func, L"error") == 0)
		return f_error(args, destination);
	if (wcscmp(func, L"warning") == 0)
		return f_warning(args, destination);
	return false;
}

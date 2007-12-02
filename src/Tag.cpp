/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#include "Tag.h"

using namespace CG3;

Tag::Tag() {
	type = 0;
	in_grammar = false;
	is_special = false;
	comparison_key = 0;
	comparison_op = OP_NOP;
	comparison_val = 0;
	tag = 0;
	regexp = 0;
	dep_self = 0;
	dep_parent = 0;
}

Tag::~Tag() {
	if (tag) {
		delete[] tag;
	}
	if (comparison_key) {
		delete[] comparison_key;
	}
}

void Tag::parseTag(const UChar *to, UFILE *ux_stderr) {
	assert(to != 0);
	type = 0;
	if (u_strlen(to)) {
		const UChar *tmp = to;
		while (tmp[0] && (tmp[0] == '!' || tmp[0] == '^')) {
			if (tmp[0] == '!') {
				type |= T_NEGATIVE;
				tmp++;
			}
			if (tmp[0] == '^') {
				type |= T_FAILFAST;
				tmp++;
			}
		}

		uint32_t length = u_strlen(tmp);
		if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<')) {
			type |= T_TEXTUAL;
		}
		while (tmp[0] && (tmp[0] == '"' || tmp[0] == '<') && (tmp[length-1] == 'i' || tmp[length-1] == 'r')) {
			if (tmp[length-1] == 'r') {
				type |= T_REGEXP;
				length--;
			}
			if (tmp[length-1] == 'i') {
				type |= T_CASE_INSENSITIVE;
				length--;
			}
		}

		if (tmp[0] == '"' && tmp[length-1] == '"') {
			if (tmp[1] == '<' && tmp[length-2] == '>') {
				type |= T_WORDFORM;
			}
			else {
				type |= T_BASEFORM;
			}
		}

		// ToDo: Implement META and VAR
		if (tmp[0] == 'M' && tmp[1] == 'E' && tmp[2] == 'T' && tmp[3] == 'A' && tmp[4] == ':') {
			type |= T_META;
			tmp += 5;
			length -= 5;
		}
		else if (tmp[0] == 'V' && tmp[1] == 'A' && tmp[2] == 'R' && tmp[3] == ':') {
			type |= T_VARIABLE;
			tmp += 4;
			length -= 4;
		}
		
		tag = new UChar[length+8];
		u_memset(tag, 0, length+8);
		u_strncpy(tag, tmp, length);
		UChar *utag = gbuffers[0];
		ux_unEscape(utag, tag);
		u_strcpy(tag, utag);
		utag = 0;
		comparison_hash = hash_sdbm_uchar(tag, 0);

		if (tag && tag[0] == '<' && tag[length-1] == '>') {
			UChar tkey[256];
			UChar top[256];
			tkey[0] = 0;
			top[0] = 0;
			int tval = 0;
			if (u_sscanf(tag, "<%[^<>=:]%[<>=:]%i>", &tkey, &top, &tval) == 3 && tval != 0 && top[0] && u_strlen(top)) {
				if (top[0] == '<') {
					comparison_op = OP_LESSTHAN;
				}
				else if (top[0] == '>') {
					comparison_op = OP_GREATERTHAN;
				}
				else if (top[0] == '=' || top[0] == ':') {
					comparison_op = OP_EQUALS;
				}
				comparison_val = tval;
				uint32_t length = u_strlen(tkey);
				comparison_key = new UChar[length+1];
				u_strcpy(comparison_key, tkey);
				comparison_hash = hash_sdbm_uchar(comparison_key, 0);
				type |= T_NUMERICAL;
				type &= ~T_TEXTUAL;
			}
		}
		if (tag && tag[0] == '#') {
			if (u_sscanf(tag, "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				type |= T_DEPENDENCY;
			}
		}

		if (u_strcmp(tag, stringbits[S_ASTERIK]) == 0) {
			//u_fprintf(ux_stderr, "Info: Tag marked as T_ANY.\n");
			type |= T_ANY;
		}

		// ToDo: Add ICASE: REGEXP: and //r //ri //i to tags
		if (type & T_REGEXP) {
			UParseError *pe = new UParseError;
			UErrorCode status = U_ZERO_ERROR;

			memset(pe, 0, sizeof(UParseError));
			status = U_ZERO_ERROR;
			if (type & T_CASE_INSENSITIVE) {
				regexp = uregex_open(tag, u_strlen(tag), UREGEX_CASE_INSENSITIVE, pe, &status);
			}
			else {
				regexp = uregex_open(tag, u_strlen(tag), 0, pe, &status);
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), tag);
				exit(1);
			}
		}
	}
	is_special = false;
	if (type & (T_ANY|T_NUMERICAL|T_VARIABLE|T_META|T_NEGATIVE|T_FAILFAST|T_CASE_INSENSITIVE|T_REGEXP)) {
		is_special = true;
	}
}

void Tag::parseTagRaw(const UChar *to) {
	assert(to != 0);
	type = 0;
	if (u_strlen(to)) {
		const UChar *tmp = to;
		uint32_t length = u_strlen(tmp);

		if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<')) {
			type |= T_TEXTUAL;
		}

		if (tmp[0] == '"' && tmp[length-1] == '"') {
			if (tmp[1] == '<' && tmp[length-2] == '>') {
				type |= T_WORDFORM;
			}
			else {
				type |= T_BASEFORM;
			}
		}

		tag = new UChar[length+1];
		tag[length] = 0;
		u_strncpy(tag, tmp, length);

		if (tag && tag[0] == '<' && tag[length-1] == '>') {
			UChar tkey[256];
			UChar top[256];
			tkey[0] = 0;
			top[0] = 0;
			int tval = 0;
			if (u_sscanf(tag, "<%[^<>=:]%[<>=:]%i>", &tkey, &top, &tval) == 3 && tval != 0 && top[0] && u_strlen(top)) {
				if (top[0] == '<') {
					comparison_op = OP_LESSTHAN;
				}
				else if (top[0] == '>') {
					comparison_op = OP_GREATERTHAN;
				}
				else if (top[0] == '=' || top[0] == ':') {
					comparison_op = OP_EQUALS;
				}
				comparison_val = tval;
				uint32_t length = u_strlen(tkey);
				comparison_key = new UChar[length+1];
				u_strcpy(comparison_key, tkey);
				comparison_hash = hash_sdbm_uchar(comparison_key, 0);
				type |= T_NUMERICAL;
				type &= ~T_TEXTUAL;
			}
		}
		if (tag && tag[0] == '#') {
			if (u_sscanf(tag, "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				type |= T_DEPENDENCY;
			}
		}
	}
	is_special = false;
	if (type & (T_ANY|T_NUMERICAL|T_VARIABLE|T_META|T_NEGATIVE|T_FAILFAST|T_CASE_INSENSITIVE|T_REGEXP)) {
		is_special = true;
	}
}

uint32_t Tag::rehash() {
	hash = 0;

	if (type & T_NEGATIVE) {
		hash = hash_sdbm_char("!", hash);
	}
	if (type & T_FAILFAST) {
		hash = hash_sdbm_char("^", hash);
	}

	if (type & T_META) {
		hash = hash_sdbm_char("META:", hash);
	}
	if (type & T_VARIABLE) {
		hash = hash_sdbm_char("VAR:", hash);
	}
/*
	UChar *tmp = new UChar[u_strlen(tag)*2+3];
	ux_escape(tmp, tag);
	hash = hash_sdbm_uchar(tmp, hash);
	delete[] tmp;
/*/
	hash = hash_sdbm_uchar(tag, hash);
//*/

	if (type & T_CASE_INSENSITIVE) {
		hash = hash_sdbm_char("i", hash);
	}
	if (type & T_REGEXP) {
		hash = hash_sdbm_char("r", hash);
	}

	is_special = false;
	if (type & (T_ANY|T_NUMERICAL|T_VARIABLE|T_META|T_NEGATIVE|T_FAILFAST|T_CASE_INSENSITIVE|T_REGEXP)) {
		is_special = true;
	}

	return hash;
}

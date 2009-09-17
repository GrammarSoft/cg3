/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Tag.h"
#include "Strings.h"

using namespace CG3;

Tag::Tag() :
in_grammar(false),
is_special(false),
is_used(false),
comparison_op(OP_NOP),
comparison_val(0),
type(0),
comparison_hash(0),
dep_self(0),
dep_parent(0),
hash(0),
plain_hash(0),
number(0),
seed(0),
comparison_key(0),
tag(0),
regexp(0)
{
	// Nothing in the actual body...
}

Tag::~Tag() {
	if (tag) {
		delete[] tag;
		tag = 0;
	}
	if (comparison_key) {
		delete[] comparison_key;
		comparison_key = 0;
	}
	if (regexp) {
		uregex_close(regexp);
		regexp = 0;
	}
}

void Tag::parseTag(const UChar *to, UFILE *ux_stderr) {
	in_grammar = true;
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

		if (tmp[0] == 'T' && tmp[1] == ':') {
			u_fprintf(ux_stderr, "Warning: Tag name %S looks like a misattempt of template usage.\n", tmp);
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
		
		if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<')) {
			type |= T_TEXTUAL;
		}
		while (tmp[0] && (tmp[0] == '"' || tmp[0] == '<') && (tmp[length-1] == 'i' || tmp[length-1] == 'r' || tmp[length-1] == 'v')) {
			if (tmp[length-1] == 'v') {
				type |= T_VARSTRING;
				length--;
			}
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

		tag = new UChar[length+8];
		u_memset(tag, 0, length+8);
		u_strncpy(tag, tmp, length);

		UChar *utag = gbuffers[0];
		ux_unEscape(utag, tag);
		if (utag[0] == 0 || u_strlen(utag) == 0) {
			u_fprintf(ux_stderr, "Error: Parsing tag %S resulted in an empty tag - cannot continue!\n", tag);
			CG3Quit(1);
		}

		u_strcpy(tag, utag);
		utag = 0;
		comparison_hash = hash_sdbm_uchar(tag);

		if (tag && tag[0] == '<' && tag[length-1] == '>') {
			parseNumeric(this, tag);
		}
		if (tag && tag[0] == '#') {
			if (u_sscanf(tag, "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				type |= T_DEPENDENCY;
			}
		}

		if (u_strcmp(tag, stringbits[S_ASTERIK]) == 0) {
			type |= T_ANY;
		}
		else if (u_strcmp(tag, stringbits[S_UU_LEFT]) == 0) {
			type |= T_PAR_LEFT;
		}
		else if (u_strcmp(tag, stringbits[S_UU_RIGHT]) == 0) {
			type |= T_PAR_RIGHT;
		}

		// ToDo: Add ICASE: REGEXP: and //r //ri //i to tags
		if (type & T_REGEXP) {
			if (u_strcmp(tag, stringbits[S_RXTEXT_ANY]) == 0
			|| u_strcmp(tag, stringbits[S_RXBASE_ANY]) == 0
			|| u_strcmp(tag, stringbits[S_RXWORD_ANY]) == 0) {
				type |= T_REGEXP_ANY;
				type &= ~T_REGEXP;
			}
			else {
				UParseError pe;
				UErrorCode status = U_ZERO_ERROR;
				status = U_ZERO_ERROR;

				if (type & T_CASE_INSENSITIVE) {
					regexp = uregex_open(tag, u_strlen(tag), UREGEX_CASE_INSENSITIVE, &pe, &status);
				}
				else {
					regexp = uregex_open(tag, u_strlen(tag), 0, &pe, &status);
				}
				if (status != U_ZERO_ERROR) {
					u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), tag);
					CG3Quit(1);
				}
			}
		}
	}
	is_special = false;
	if (type & (T_ANY|T_PAR_LEFT|T_PAR_RIGHT|T_NUMERICAL|T_VARIABLE|T_META|T_NEGATIVE|T_FAILFAST|T_CASE_INSENSITIVE|T_REGEXP|T_REGEXP_ANY|T_VARSTRING)) {
		is_special = true;
	}

	if (type & T_VARSTRING && type & (T_REGEXP|T_REGEXP_ANY|T_CASE_INSENSITIVE|T_NUMERICAL|T_VARIABLE|T_META)) {
		u_fprintf(ux_stderr, "Error: Tag %S cannot mix varstring with any other special feature!\n", to);
		CG3Quit(1);
	}
}

void Tag::parseTagRaw(const UChar *to) {
	in_grammar = false;
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
			parseNumeric(this, tag);
		}
		if (tag && tag[0] == '#') {
			if (u_sscanf(tag, "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				type |= T_DEPENDENCY;
			}
		}
	}
	is_special = false;
	if (type & (T_NUMERICAL)) {
		is_special = true;
	}
}

void Tag::parseNumeric(Tag *tag, const UChar *txt) {
	UChar tkey[256];
	UChar top[256];
	UChar txval[256];
	tkey[0] = 0;
	top[0] = 0;
	txval[0] = 0;
	if (u_sscanf(txt, "<%[^<>=:!]%[<>=:!]%[-MAXIN0-9]>", &tkey, &top, &txval) == 3 && top[0] && u_strlen(top)) {
		int tval = 0;
		int32_t rv = u_sscanf(txval, "%d", &tval);
		if (txval[0] == 'M' && txval[1] == 'A' && txval[2] == 'X') {
			tval = INT_MAX;
		}
		else if (txval[0] == 'M' && txval[1] == 'I' && txval[2] == 'N') {
			tval = INT_MIN;
		}
		else if (rv != 1) {
			return;
		}
		if (top[0] == '<') {
			tag->comparison_op = OP_LESSTHAN;
		}
		else if (top[0] == '>') {
			tag->comparison_op = OP_GREATERTHAN;
		}
		else if (top[0] == '=' || top[0] == ':') {
			tag->comparison_op = OP_EQUALS;
		}
		else if (top[0] == '!') {
			tag->comparison_op = OP_NOTEQUALS;
		}
		if (top[1]) {
			if (top[1] == '=' || top[1] == ':') {
				if (tag->comparison_op == OP_GREATERTHAN) {
					tag->comparison_op = OP_GREATEREQUALS;
				}
				else if (tag->comparison_op == OP_LESSTHAN) {
					tag->comparison_op = OP_LESSEQUALS;
				}
				else if (tag->comparison_op == OP_NOTEQUALS) {
					tag->comparison_op = OP_NOTEQUALS;
				}
			}
			else if (top[1] == '>') {
				if (tag->comparison_op == OP_EQUALS) {
					tag->comparison_op = OP_GREATEREQUALS;
				}
				else if (tag->comparison_op == OP_LESSTHAN) {
					tag->comparison_op = OP_NOTEQUALS;
				}
			}
			else if (top[1] == '<') {
				if (tag->comparison_op == OP_EQUALS) {
					tag->comparison_op = OP_LESSEQUALS;
				}
				else if (tag->comparison_op == OP_GREATERTHAN) {
					tag->comparison_op = OP_NOTEQUALS;
				}
			}
		}
		tag->comparison_val = tval;
		uint32_t length = u_strlen(tkey);
		tag->comparison_key = tag->allocateUChars(length+1);
		u_strcpy(tag->comparison_key, tkey);
		tag->comparison_hash = hash_sdbm_uchar(tag->comparison_key);
		tag->type |= T_NUMERICAL;
		tag->type &= ~T_TEXTUAL;
	}
}

uint32_t Tag::rehash() {
	hash = 0;
	plain_hash = 0;

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

	plain_hash = hash_sdbm_uchar(tag);
	if (hash) {
		hash = hash_sdbm_uint32_t(plain_hash, hash);
	}
	else {
		hash = plain_hash;
	}

	if (type & T_CASE_INSENSITIVE) {
		hash = hash_sdbm_char("i", hash);
	}
	if (type & T_REGEXP) {
		hash = hash_sdbm_char("r", hash);
	}

	if (seed) {
		hash += seed;
	}

	is_special = false;
	if (type & (T_ANY|T_PAR_LEFT|T_PAR_RIGHT|T_NUMERICAL|T_VARIABLE|T_META|T_NEGATIVE|T_FAILFAST|T_CASE_INSENSITIVE|T_REGEXP|T_REGEXP_ANY|T_VARSTRING)) {
		is_special = true;
	}

	return hash;
}

void Tag::markUsed() {
	is_used = true;
}

UChar *Tag::allocateUChars(uint32_t n) {
	return new UChar[n];
}

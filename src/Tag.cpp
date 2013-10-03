/*
* Copyright (C) 2007-2013, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "Tag.hpp"
#include "Set.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"

namespace CG3 {

UFILE* Tag::dump_hashes_out = 0;

Tag::Tag() :
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
regexp(0)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

Tag::~Tag() {
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif

	if (regexp) {
		uregex_close(regexp);
		regexp = 0;
	}
}

void Tag::parseTag(const UChar *to, UFILE *ux_stderr, Grammar *grammar) {
	type = 0;

	if (to && to[0]) {
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

		size_t length = u_strlen(tmp);
		assert(length && "parseTag() will not work with empty strings.");

		if (tmp[0] == 'T' && tmp[1] == ':') {
			u_fprintf(ux_stderr, "Warning: Tag %S looks like a misattempt of template usage on line %u.\n", tmp, grammar->lines);
		}

		// ToDo: Implement META and VAR
		if (tmp[0] == 'M' && tmp[1] == 'E' && tmp[2] == 'T' && tmp[3] == 'A' && tmp[4] == ':') {
			type |= T_META;
			tmp += 5;
			length -= 5;
		}
		if (tmp[0] == 'V' && tmp[1] == 'A' && tmp[2] == 'R' && tmp[3] == ':') {
			type |= T_VARIABLE;
			tmp += 4;
			length -= 4;
		}
		if (tmp[0] == 'S' && tmp[1] == 'E' && tmp[2] == 'T' && tmp[3] == ':') {
			type |= T_SET;
			tmp += 4;
			length -= 4;
		}
		if (tmp[0] == 'V' && tmp[1] == 'S' && tmp[2] == 'T' && tmp[3] == 'R' && tmp[4] == ':') {
			type |= T_VARSTRING;
			type |= T_VSTR;
			tmp += 5;
			length -= 5;

			tag.assign(tmp);
			if (tag.empty()) {
				u_fprintf(ux_stderr, "Error: Parsing tag %S resulted in an empty tag on line %u - cannot continue!\n", tag.c_str(), grammar->lines);
				CG3Quit(1);
			}

			goto label_isVarstring;
		}
		
		if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<' || tmp[0] == '/')) {
			size_t oldlength = length;

			// Parse the suffixes r, i, v but max only one of each.
			while (tmp[length-1] == 'i' || tmp[length-1] == 'r' || tmp[length-1] == 'v') {
				if (!(type & T_VARSTRING) && tmp[length-1] == 'v') {
					type |= T_VARSTRING;
					length--;
					continue;
				}
				if (!(type & T_REGEXP) && tmp[length-1] == 'r') {
					type |= T_REGEXP;
					length--;
					continue;
				}
				if (!(type & T_CASE_INSENSITIVE) && tmp[length-1] == 'i') {
					type |= T_CASE_INSENSITIVE;
					length--;
					continue;
				}
				break;
			}

			if (tmp[0] == '"' && tmp[length-1] == '"') {
				if (tmp[1] == '<' && tmp[length-2] == '>') {
					type |= T_WORDFORM;
				}
				else {
					type |= T_BASEFORM;
				}
			}

			if ((tmp[0] == '"' && tmp[length-1] == '"') || (tmp[0] == '<' && tmp[length-1] == '>') || (tmp[0] == '/' && tmp[length-1] == '/')) {
				type |= T_TEXTUAL;
			}
			else {
				type &= ~T_VARSTRING;
				type &= ~T_REGEXP;
				type &= ~T_CASE_INSENSITIVE;
				type &= ~T_WORDFORM;
				type &= ~T_BASEFORM;
				length = oldlength;
			}
		}

		for (size_t i=0, oldlength = length ; tmp[i] != 0 && i < oldlength ; ++i) {
			if (tmp[i] == '\\') {
				++i;
				--length;
			}
			if (tmp[i] == 0) {
				break;
			}
			tag += tmp[i];
		}
		if (tag.empty()) {
			u_fprintf(ux_stderr, "Error: Parsing tag %S resulted in an empty tag on line %u - cannot continue!\n", tag.c_str(), grammar->lines);
			CG3Quit(1);
		}

		foreach (Grammar::regex_tags_t, grammar->regex_tags, iter, iter_end) {
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(*iter, tag.c_str(), tag.length(), &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText(parseTag) returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
			status = U_ZERO_ERROR;
			if (uregex_matches(*iter, 0, &status)) {
				type |= T_TEXTUAL;
			}
		}
		foreach (Grammar::icase_tags_t, grammar->icase_tags, iter, iter_end) {
			UErrorCode status = U_ZERO_ERROR;
			if (u_strCaseCompare(tag.c_str(), tag.length(), (*iter)->tag.c_str(), (*iter)->tag.length(), U_FOLD_CASE_DEFAULT, &status) == 0) {
				type |= T_TEXTUAL;
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: u_strCaseCompare(parseTag) returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
		}

		comparison_hash = hash_sdbm_uchar(tag);

		if (tag[0] == '<' && tag[length-1] == '>') {
			parseNumeric();
		}

		if (u_strcmp(tag.c_str(), stringbits[S_ASTERIK].getTerminatedBuffer()) == 0) {
			type |= T_ANY;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_LEFT].getTerminatedBuffer()) == 0) {
			type |= T_PAR_LEFT;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_RIGHT].getTerminatedBuffer()) == 0) {
			type |= T_PAR_RIGHT;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_ENCL].getTerminatedBuffer()) == 0) {
			type |= T_ENCL;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_TARGET].getTerminatedBuffer()) == 0) {
			type |= T_TARGET;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_MARK].getTerminatedBuffer()) == 0) {
			type |= T_MARK;
		}
		else if (u_strcmp(tag.c_str(), stringbits[S_UU_ATTACHTO].getTerminatedBuffer()) == 0) {
			type |= T_ATTACHTO;
		}

		if (type & T_REGEXP) {
			if (u_strcmp(tag.c_str(), stringbits[S_RXTEXT_ANY].getTerminatedBuffer()) == 0
			|| u_strcmp(tag.c_str(), stringbits[S_RXBASE_ANY].getTerminatedBuffer()) == 0
			|| u_strcmp(tag.c_str(), stringbits[S_RXWORD_ANY].getTerminatedBuffer()) == 0) {
				// ToDo: Add a case-insensitive version of T_REGEXP_ANY for unification
				type |= T_REGEXP_ANY;
				type &= ~T_REGEXP;
			}
			else {
				UParseError pe;
				UErrorCode status = U_ZERO_ERROR;

				UString rt;
				if (tag[0] == '/' && tag[length-1] == '/') {
					rt = tag.substr(1, length-2);
				}
				else {
					rt += '^';
					rt += tag;
					rt += '$';
				}

				if (type & T_CASE_INSENSITIVE) {
					regexp = uregex_open(rt.c_str(), rt.length(), UREGEX_CASE_INSENSITIVE, &pe, &status);
				}
				else {
					regexp = uregex_open(rt.c_str(), rt.length(), 0, &pe, &status);
				}
				if (status != U_ZERO_ERROR) {
					u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S on line %u - cannot continue!\n", u_errorName(status), tag.c_str(), grammar->lines);
					CG3Quit(1);
				}
			}
		}
		else if (type & T_CASE_INSENSITIVE) {
			if (tag[0] == '/' && tag[length-1] == '/') {
				tag.resize(tag.size()-1);
				tag.erase(tag.begin());
				length -= 2;
			}
		}

label_isVarstring:
		if (type & T_VARSTRING) {
			UChar *p = &tag[0];
			UChar *n = 0;
			do {
				SKIPTO(p, '{');
				if (*p) {
					n = p;
					SKIPTO(n, '}');
					if (*n) {
						allocateVsSets();
						allocateVsNames();
						++p;
						UString theSet(p, n);
						Set *tmp = grammar->parseSet(theSet.c_str());
						vs_sets->push_back(tmp);
						UString old;
						old += '{';
						old += tmp->name;
						old += '}';
						vs_names->push_back(old);
						p = n;
						++p;
					}
				}
			} while(*p);
		}
	}

	type &= ~T_SPECIAL;
	if (type & MASK_TAG_SPECIAL) {
		type |= T_SPECIAL;
	}

	if (type & T_VARSTRING && type & (T_REGEXP|T_REGEXP_ANY|T_VARIABLE|T_META)) {
		u_fprintf(ux_stderr, "Error: Tag %S cannot mix varstring with any other special feature on line %u!\n", to, grammar->lines);
		CG3Quit(1);
	}
}

void Tag::parseTagRaw(const UChar *to, Grammar *grammar) {
	type = 0;
	size_t length = u_strlen(to);
	assert(length && "parseTagRaw() will not work with empty strings.");

	const UChar *tmp = to;

	if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<')) {
		if ((tmp[0] == '"' && tmp[length-1] == '"') || (tmp[0] == '<' && tmp[length-1] == '>')) {
			type |= T_TEXTUAL;
			if (tmp[0] == '"' && tmp[length-1] == '"') {
				if (tmp[1] == '<' && tmp[length-2] == '>') {
					type |= T_WORDFORM;
				}
				else {
					type |= T_BASEFORM;
				}
			}
		}
	}

	tag.assign(tmp, length);

	foreach (Grammar::regex_tags_t, grammar->regex_tags, iter, iter_end) {
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(*iter, tag.c_str(), tag.length(), &status);
		if (status == U_ZERO_ERROR) {
			if (uregex_matches(*iter, 0, &status)) {
				type |= T_TEXTUAL;
			}
		}
	}
	foreach (Grammar::icase_tags_t, grammar->icase_tags, iter, iter_end) {
		UErrorCode status = U_ZERO_ERROR;
		if (u_strCaseCompare(tag.c_str(), tag.length(), (*iter)->tag.c_str(), (*iter)->tag.length(), U_FOLD_CASE_DEFAULT, &status) == 0) {
			type |= T_TEXTUAL;
		}
	}

	if (tag[0] == '<' && tag[length-1] == '>') {
		parseNumeric();
	}
	if (tag[0] == '#') {
		if (u_sscanf(tag.c_str(), "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
			type |= T_DEPENDENCY;
		}
		const UChar local_dep_unicode[] = {'#', '%', 'i', L'\u2192', '%', 'i', 0};
		if (u_sscanf_u(tag.c_str(), local_dep_unicode, &dep_self, &dep_parent) == 2 && dep_self != 0) {
			type |= T_DEPENDENCY;
		}
	}
	if (tag[0] == 'I' && tag[1] == 'D' && tag[2] == ':' && u_isdigit(tag[3])) {
		if (u_sscanf(tag.c_str(), "ID:%i", &dep_self) == 1 && dep_self != 0) {
			type |= T_RELATION;
		}
	}
	if (tag[0] == 'R' && tag[1] == ':') {
		UChar relname[256];
		if (u_sscanf(tag.c_str(), "R:%[^:]:%i", &relname, &dep_parent) == 2 && dep_parent != 0) {
			type |= T_RELATION;
			Tag *reltag = grammar->allocateTag(relname, true);
			comparison_hash = reltag->hash;
		}
	}

	type &= ~T_SPECIAL;
	if (type & (T_NUMERICAL)) {
		type |= T_SPECIAL;
	}
}

void Tag::parseNumeric() {
	UChar tkey[256];
	UChar top[256];
	UChar txval[256];
	tkey[0] = 0;
	top[0] = 0;
	txval[0] = 0;
	if (u_sscanf(tag.c_str(), "<%[^<>=:!]%[<>=:!]%[-MAXIN0-9]>", &tkey, &top, &txval) == 3 && top[0]) {
		int32_t tval = 0;
		int32_t rv = u_sscanf(txval, "%d", &tval);
		if (txval[0] == 'M' && txval[1] == 'A' && txval[2] == 'X') {
			tval = std::numeric_limits<int32_t>::max();
		}
		else if (txval[0] == 'M' && txval[1] == 'I' && txval[2] == 'N') {
			tval = std::numeric_limits<int32_t>::min();
		}
		else if (rv != 1) {
			return;
		}
		if (top[0] == '<') {
			comparison_op = OP_LESSTHAN;
		}
		else if (top[0] == '>') {
			comparison_op = OP_GREATERTHAN;
		}
		else if (top[0] == '=' || top[0] == ':') {
			comparison_op = OP_EQUALS;
		}
		else if (top[0] == '!') {
			comparison_op = OP_NOTEQUALS;
		}
		if (top[1]) {
			if (top[1] == '=' || top[1] == ':') {
				if (comparison_op == OP_GREATERTHAN) {
					comparison_op = OP_GREATEREQUALS;
				}
				else if (comparison_op == OP_LESSTHAN) {
					comparison_op = OP_LESSEQUALS;
				}
				else if (comparison_op == OP_NOTEQUALS) {
					comparison_op = OP_NOTEQUALS;
				}
			}
			else if (top[1] == '>') {
				if (comparison_op == OP_EQUALS) {
					comparison_op = OP_GREATEREQUALS;
				}
				else if (comparison_op == OP_LESSTHAN) {
					comparison_op = OP_NOTEQUALS;
				}
			}
			else if (top[1] == '<') {
				if (comparison_op == OP_EQUALS) {
					comparison_op = OP_LESSEQUALS;
				}
				else if (comparison_op == OP_GREATERTHAN) {
					comparison_op = OP_NOTEQUALS;
				}
			}
		}
		comparison_val = tval;
		comparison_hash = hash_sdbm_uchar(tkey);
		type |= T_NUMERICAL;
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
	if (type & T_SET) {
		hash = hash_sdbm_char("SET:", hash);
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
	if (type & T_VARSTRING) {
		hash = hash_sdbm_char("v", hash);
	}

	hash += seed;

	type &= ~T_SPECIAL;
	if (type & MASK_TAG_SPECIAL) {
		type |= T_SPECIAL;
	}

	if (dump_hashes_out) {
		u_fprintf(dump_hashes_out, "DEBUG: Hash %u with seed %u for tag %S\n", hash, seed, tag.c_str());
		u_fprintf(dump_hashes_out, "DEBUG: Plain hash %u with seed %u for tag %S\n", plain_hash, seed, tag.c_str());
	}

	return hash;
}

void Tag::markUsed() {
	type |= T_USED;
}

void Tag::allocateVsSets() {
	if (!vs_sets) {
		vs_sets.reset(new SetVector);
	}
}

void Tag::allocateVsNames() {
	if (!vs_names) {
		vs_names.reset(new UStringVector);
	}
}

UString Tag::toUString(bool escape) const {
	UString str;
	str.reserve(tag.length());

	if (type & T_NEGATIVE) {
		str += '!';
	}
	if (type & T_FAILFAST) {
		str += '^';
	}
	if (type & T_META) {
		str += 'M';
		str += 'E';
		str += 'T';
		str += 'A';
		str += ':';
	}
	if (type & T_VARIABLE) {
		str += 'V';
		str += 'A';
		str += 'R';
		str += ':';
	}
	if (type & T_SET) {
		str += 'S';
		str += 'E';
		str += 'T';
		str += ':';
	}
	if (type & T_VSTR) {
		str += 'V';
		str += 'S';
		str += 'T';
		str += 'R';
		str += ':';
	}

	if (escape) {
		for (size_t i=0 ; i<tag.length() ; ++i) {
			if (tag[i] == '\\' || tag[i] == '(' || tag[i] == ')' || tag[i] == ';' || tag[i] == '#') {
				str += '\\';
			}
			str += tag[i];
		}
	}
	else {
		str + tag;
	}

	if (type & T_CASE_INSENSITIVE) {
		str += 'i';
	}
	if (type & T_REGEXP) {
		str += 'r';
	}
	if ((type & T_VARSTRING) && !(type & T_VSTR)) {
		str += 'v';
	}
	return str;
}

}

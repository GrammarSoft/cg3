/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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

std::ostream* Tag::dump_hashes_out = 0;

Tag::Tag()
  : comparison_op(OP_NOP)
  , comparison_val(0)
  , type(0)
  , comparison_hash(0)
  , dep_self(0)
  , dep_parent(0)
  , hash(0)
  , plain_hash(0)
  , number(0)
  , seed(0)
  , regexp(0)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif
}

Tag::Tag(const Tag& o)
  : comparison_op(o.comparison_op)
  , comparison_val(o.comparison_val)
  , type(o.type)
  , comparison_hash(o.comparison_hash)
  , dep_self(o.dep_self)
  , dep_parent(o.dep_parent)
  , hash(o.hash)
  , plain_hash(o.plain_hash)
  , number(o.number)
  , seed(o.seed)
  , tag(o.tag)
  , regexp(0)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	#endif

	if (o.vs_names) {
		allocateVsNames();
		*vs_names.get() = *o.vs_names.get();
	}
	if (o.vs_sets) {
		allocateVsSets();
		*vs_sets.get() = *o.vs_sets.get();
	}
	if (o.regexp) {
		UErrorCode status = U_ZERO_ERROR;
		regexp = uregex_clone(o.regexp, &status);
	}
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

void Tag::parseTagRaw(const UChar* to, Grammar* grammar) {
	type = 0;
	size_t length = u_strlen(to);
	assert(length && "parseTagRaw() will not work with empty strings.");

	const UChar* tmp = to;

	if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<')) {
		if ((tmp[0] == '"' && tmp[length - 1] == '"') || (tmp[0] == '<' && tmp[length - 1] == '>')) {
			type |= T_TEXTUAL;
			if (tmp[0] == '"' && tmp[length - 1] == '"') {
				if (tmp[1] == '<' && tmp[length - 2] == '>') {
					type |= T_WORDFORM;
				}
				else {
					type |= T_BASEFORM;
				}
			}
		}
	}

	tag.assign(tmp, length);

	for (auto iter : grammar->regex_tags) {
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(iter, tag.c_str(), tag.size(), &status);
		if (status == U_ZERO_ERROR) {
			if (uregex_matches(iter, 0, &status)) {
				type |= T_TEXTUAL;
			}
		}
	}
	for (auto iter : grammar->icase_tags) {
		UErrorCode status = U_ZERO_ERROR;
		if (u_strCaseCompare(tag.c_str(), tag.size(), iter->tag.c_str(), iter->tag.size(), U_FOLD_CASE_DEFAULT, &status) == 0) {
			type |= T_TEXTUAL;
		}
	}

	if (tag[0] == '<' && tag[length - 1] == '>') {
		parseNumeric();
	}
	if (tag[0] == '#') {
		if (u_sscanf(tag.c_str(), "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
			type |= T_DEPENDENCY;
		}
		constexpr UChar local_dep_unicode[] = { '#', '%', 'i', L'\u2192', '%', 'i', 0 };
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
			Tag* reltag = grammar->allocateTag(relname);
			comparison_hash = reltag->hash;
		}
	}

	type &= ~T_SPECIAL;
	if (type & (T_NUMERICAL)) {
		type |= T_SPECIAL;
	}
}

void Tag::parseNumeric() {
	if (tag.size() >= 256) {
		return;
	}
	UChar tkey[256];
	UChar top[256];
	UChar txval[256];
	UChar spn[] = { '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0 };
	tkey[0] = 0;
	top[0] = 0;
	txval[0] = 0;
	if (u_sscanf(tag.c_str(), "%*[<]%[^<>=:!]%[<>=:!]%[-.MAXIN0-9]%*[>]", &tkey, &top, &txval) == 3 && top[0] && txval[0]) {
		double tval = 0;
		int32_t r = u_strspn(txval, spn);
		if (txval[0] == 'M' && txval[1] == 'A' && txval[2] == 'X' && txval[3] == 0) {
			tval = NUMERIC_MAX;
		}
		else if (txval[0] == 'M' && txval[1] == 'I' && txval[2] == 'N' && txval[3] == 0) {
			tval = NUMERIC_MIN;
		}
		else if (txval[r] || u_sscanf(txval, "%lf", &tval) != 1) {
			return;
		}
		if (tval < NUMERIC_MIN) {
			tval = NUMERIC_MIN;
		}
		if (tval > NUMERIC_MAX) {
			tval = NUMERIC_MAX;
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
		comparison_hash = hash_value(tkey);
		type |= T_NUMERICAL;
	}
}

uint32_t Tag::rehash() {
	hash = 0;
	plain_hash = 0;

	if (type & T_FAILFAST) {
		hash = hash_value("^", hash);
	}

	if (type & T_META) {
		hash = hash_value("META:", hash);
	}
	if (type & T_VARIABLE) {
		hash = hash_value("VAR:", hash);
	}
	if (type & T_SET) {
		hash = hash_value("SET:", hash);
	}

	plain_hash = hash_value(tag);
	if (hash) {
		hash = hash_value(plain_hash, hash);
	}
	else {
		hash = plain_hash;
	}

	if (type & T_CASE_INSENSITIVE) {
		hash = hash_value("i", hash);
	}
	if (type & T_REGEXP) {
		hash = hash_value("r", hash);
	}
	if (type & T_VARSTRING) {
		hash = hash_value("v", hash);
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
	str.reserve(tag.size());

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

	if (type & (T_CASE_INSENSITIVE | T_REGEXP) && tag[0] != '"') {
		str += '/';
	}

	if (escape) {
		for (size_t i = 0; i < tag.size(); ++i) {
			if (tag[i] == '\\' || tag[i] == '(' || tag[i] == ')' || tag[i] == ';' || tag[i] == '#') {
				str += '\\';
			}
			str += tag[i];
		}
	}
	else {
		str + tag;
	}

	if (type & (T_CASE_INSENSITIVE | T_REGEXP) && tag[0] != '"') {
		str += '/';
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

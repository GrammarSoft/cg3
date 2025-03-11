/*
* Copyright (C) 2007-2025, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Tag.hpp"
#include "Set.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"
#include "MathParser.hpp"

namespace CG3 {

std::ostream* Tag::dump_hashes_out = nullptr;

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
  , regexp(nullptr)
{
	#ifdef CG_TRACE_OBJECTS
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << std::endl;
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
	std::cerr << "OBJECT: " << VOIDP(this) << " " << __PRETTY_FUNCTION__ << std::endl;
	#endif

	if (regexp) {
		uregex_close(regexp);
		regexp = nullptr;
	}
}

void Tag::parseTagRaw(const UChar* to, Grammar* grammar) {
	type = 0;
	const size_t length = u_strlen(to);
	assert(length && "parseTagRaw() will not work with empty strings.");

	if (to[0] && (to[0] == '"' || to[0] == '<')) {
		if ((to[0] == '"' && to[length - 1] == '"') || (to[0] == '<' && to[length - 1] == '>')) {
			type |= T_TEXTUAL;
			if (to[0] == '"' && to[length - 1] == '"') {
				if (to[1] == '<' && to[length - 2] == '>' && length > 4) {
					type |= T_WORDFORM;
				}
				else {
					type |= T_BASEFORM;
				}
			}
		}
	}

	tag.assign(to, length);

	for (auto iter : grammar->regex_tags) {
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(iter, tag.data(), SI32(tag.size()), &status);
		if (status == U_ZERO_ERROR) {
			if (uregex_find(iter, -1, &status)) {
				type |= T_TEXTUAL;
			}
		}
	}
	for (auto iter : grammar->icase_tags) {
		if (ux_strCaseCompare(tag, iter->tag)) {
			type |= T_TEXTUAL;
		}
	}

	if (tag[0] == '<' && tag[length - 1] == '>') {
		parseNumeric(false);
	}
	if (tag[0] == '#') {
		if (u_sscanf(tag.data(), "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
			type |= T_DEPENDENCY;
		}
		constexpr UChar local_dep_unicode[] = { '#', '%', 'i', u'\u2192', '%', 'i', 0 };
		if (u_sscanf_u(tag.data(), local_dep_unicode, &dep_self, &dep_parent) == 2 && dep_self != 0) {
			type |= T_DEPENDENCY;
		}
	}
	if (tag[0] == 'I' && tag[1] == 'D' && tag[2] == ':' && u_isdigit(tag[3])) {
		if (u_sscanf(tag.data(), "ID:%i", &dep_self) == 1 && dep_self != 0) {
			type |= T_RELATION;
		}
	}
	if (tag[0] == 'R' && tag[1] == ':') {
		UChar relname[256];
		dep_parent = std::numeric_limits<uint32_t>::max();
		if (u_sscanf(tag.data(), "R:%[^:]:%i", &relname, &dep_parent) == 2 && dep_parent != std::numeric_limits<uint32_t>::max()) {
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

void Tag::parseNumeric(bool trusted) {
	if (tag.size() >= 256) {
		return;
	}
	UChar tkey[256];
	UChar top[256];
	tkey[0] = 0;
	top[0] = 0;
	if (u_sscanf(tag.data(), "%*[<]%[^<>=:!]%[<>=:!]", &tkey, &top) == 2 && top[0]) {
		auto tkz = u_strlen(tkey);
		auto toz = u_strlen(top);
		if (UIZ(tkz + toz + 1) >= tag.size()) {
			return;
		}

		UChar txval[256];
		std::copy(tag.begin() + tkz + toz + 1, tag.end() - 1, txval);
		txval[tag.size() - tkz - toz - 2] = 0;
		if (txval[0] == 0) {
			return;
		}

		double tval = 0;
		auto r = u_strspn(txval, u"-.0123456789");
		if (trusted && txval[r] && (UStringView(txval).find_first_of(u"-+*/^%()=") != UStringView::npos) && (UStringView(txval).find_first_of(u"\"\\<>[]{}!?&$¤#£@~`´';:,|_") == UStringView::npos)) {
			comparison_offset = u_strlen(tkey) + u_strlen(top) + 1;
			try {
				MathParser mp(NUMERIC_MIN, NUMERIC_MAX);
				UStringView exp(tag);
				exp.remove_prefix(comparison_offset);
				exp.remove_suffix(1);
				mp.eval(exp);
			}
			catch (...) {
				comparison_offset = 0;
				return;
			}
			type |= T_NUMERIC_MATH;
		}
		else if (txval[0] == 'M' && txval[1] == 'A' && txval[2] == 'X' && txval[3] == 0) {
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
	if (type & T_LOCAL_VARIABLE) {
		hash = hash_value("LVAR:", hash);
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
		u_fprintf(dump_hashes_out, "DEBUG: Hash %u with seed %u for tag %S\n", hash, seed, tag.data());
		u_fprintf(dump_hashes_out, "DEBUG: Plain hash %u with seed %u for tag %S\n", plain_hash, seed, tag.data());
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
	if (!tag_raw.empty()) {
		return tag_raw;
	}

	UString str;
	str.reserve(tag.size());

	if (type & T_FAILFAST) {
		str += '^';
	}
	if (type & T_META) {
		str.append(u"META:");
	}
	if (type & T_VARIABLE) {
		str.append(u"VAR:");
	}
	if (type & T_LOCAL_VARIABLE) {
		str.append(u"LVAR:");
	}
	if (type & T_SET) {
		str.append(u"SET:");
	}
	if (type & T_VSTR) {
		str.append(u"VSTR:");
	}

	if (type & (T_CASE_INSENSITIVE | T_REGEXP) && !is_textual(tag)) {
		str += '/';
	}

	if (escape && tag[0] != '"') {
		for (auto c : tag) {
			if (c == '\\' || c == '(' || c == ')' || c == ';' || c == '#' || c == ' ') {
				str += '\\';
			}
			str += c;
		}
	}
	else {
		str += tag;
	}

	if (type & (T_CASE_INSENSITIVE | T_REGEXP) && !is_textual(tag)) {
		str += '/';
	}
	if (type & T_REGEXP_LINE) {
		str += 'l';
	}
	else if (type & (T_REGEXP | T_REGEXP_ANY)) {
		str += 'r';
	}
	if (type & T_CASE_INSENSITIVE) {
		str += 'i';
	}
	if ((type & T_VARSTRING) && !(type & T_VSTR)) {
		str += 'v';
	}
	return str;
}
}

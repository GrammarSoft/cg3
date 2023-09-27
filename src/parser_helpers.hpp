/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_PARSER_HELPERS_H
#define c6d28b7452ec699b_PARSER_HELPERS_H

#include "Tag.hpp"
#include "Set.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"

namespace CG3 {

template<typename State>
Tag* parseTag(const UChar* to, const UChar* p, State& state, bool unescape=true) {
	if (to[0] == 0) {
		state.error("%s: Error: Empty tag on line %u near `%S`! Forgot to fill in a ()?\n", p);
	}
	if (to[0] == '(') {
		state.error("%s: Error: Tag '%S' cannot start with ( on line %u near `%S`! Possible extra opening ( or missing closing ) to the left. If you really meant it, escape it as \\(.\n", to, p);
	}
	if (ux_isSetOp(to) != S_IGNORE) {
		u_fprintf(state.ux_stderr, "%s: Warning: Tag '%S' on line %u looks like a set operator. Maybe you meant to do SET instead of LIST?\n", state.filebase, to, state.get_grammar()->lines);
		u_fflush(state.ux_stderr);
	}

	Taguint32HashMap::iterator it;
	uint32_t thash = hash_value(to);
	if ((it = state.get_grammar()->single_tags.find(thash)) != state.get_grammar()->single_tags.end() && !it->second->tag.empty() && it->second->tag == to) {
		return it->second;
	}

	Tag* tag = state.get_grammar()->allocateTag();
	tag->type = 0;

	if (to[0]) {
		const UChar* tmp = to;
		while (tmp[0] && tmp[0] == '^') {
			tag->type |= T_FAILFAST;
			tmp++;
		}

		size_t length = u_strlen(tmp);
		assert(length && "parseTag() will not work with empty strings.");

		if (tmp[0] == 'T' && tmp[1] == ':') {
			u_fprintf(state.ux_stderr, "%s: Warning: Tag %S looks like a misattempt of template usage on line %u.\n", state.filebase, tmp, state.get_grammar()->lines);
			u_fflush(state.ux_stderr);
		}

		if (tmp[0] == 'M' && tmp[1] == 'E' && tmp[2] == 'T' && tmp[3] == 'A' && tmp[4] == ':') {
			tag->type |= T_META;
			tmp += 5;
			length -= 5;
		}
		if (tmp[0] == 'V' && tmp[1] == 'A' && tmp[2] == 'R' && tmp[3] == ':') {
			tag->type |= T_VARIABLE;
			tag->variable_hash = 0;
			tmp += 4;
			length -= 4;
		}
		if (tmp[0] == 'L' && tmp[1] == 'V' && tmp[2] == 'A' && tmp[3] == 'R' && tmp[4] == ':') {
			tag->type |= T_LOCAL_VARIABLE;
			tmp += 5;
			length -= 5;
		}
		if (tmp[0] == 'S' && tmp[1] == 'E' && tmp[2] == 'T' && tmp[3] == ':') {
			tag->type |= T_SET;
			tmp += 4;
			length -= 4;
		}
		if (tmp[0] == 'V' && tmp[1] == 'S' && tmp[2] == 'T' && tmp[3] == 'R' && tmp[4] == ':') {
			tag->type |= T_VARSTRING;
			tag->type |= T_VSTR;
			tmp += 5;

			tag->tag.assign(tmp);
			if (tag->tag.empty()) {
				state.error("%s: Error: Parsing tag %S resulted in an empty tag on line %u near `%S` - cannot continue!\n", tag->tag.data(), p);
			}

			goto label_isVarstring;
		}

		if (tmp[0] && !(tag->type & (T_VARIABLE|T_LOCAL_VARIABLE)) && (tmp[0] == '"' || tmp[0] == '<' || tmp[0] == '/')) {
			size_t oldlength = length;

			// Parse the suffixes r, i, v but max only one of each.
			while (tmp[length - 1] == 'i' || tmp[length - 1] == 'r' || tmp[length - 1] == 'v' || tmp[length - 1] == 'l') {
				if (!(tag->type & T_VARSTRING) && tmp[length - 1] == 'v') {
					tag->type |= T_VARSTRING;
					length--;
					continue;
				}
				if (!(tag->type & T_REGEXP) && tmp[length - 1] == 'r') {
					tag->type |= T_REGEXP;
					length--;
					continue;
				}
				if (!(tag->type & T_CASE_INSENSITIVE) && tmp[length - 1] == 'i') {
					tag->type |= T_CASE_INSENSITIVE;
					length--;
					continue;
				}
				if (!(tag->type & T_REGEXP_LINE) && tmp[length - 1] == 'l') {
					tag->type |= T_REGEXP;
					tag->type |= T_REGEXP_LINE;
					length--;
					continue;
				}
				break;
			}

			if (tmp[0] == '"' && tmp[length - 1] == '"') {
				if (tmp[1] == '<' && tmp[length - 2] == '>') {
					tag->type |= T_WORDFORM;
				}
				else {
					tag->type |= T_BASEFORM;
				}
			}

			if ((tmp[0] == '"' && tmp[length - 1] == '"') || (tmp[0] == '<' && tmp[length - 1] == '>') || (tmp[0] == '/' && tmp[length - 1] == '/')) {
				tag->type |= T_TEXTUAL;
			}
			else {
				tag->type &= ~T_VARSTRING;
				tag->type &= ~T_REGEXP;
				tag->type &= ~T_REGEXP_LINE;
				tag->type &= ~T_CASE_INSENSITIVE;
				tag->type &= ~T_WORDFORM;
				tag->type &= ~T_BASEFORM;
				length = oldlength;
			}
		}

		for (size_t i = 0, oldlength = length; tmp[i] != 0 && i < oldlength; ++i) {
			if (unescape && tmp[i] == '\\') {
				++i;
				--length;
			}
			if (tmp[i] == 0) {
				break;
			}
			tag->tag += tmp[i];
		}
		if (tag->tag.empty()) {
			state.error("%s: Error: Parsing tag %S resulted in an empty tag on line %u near `%S` - cannot continue!\n", tag->tag.data(), p);
		}

		// ToDo: Remove for real ordered mode
		if (tag->type & T_REGEXP_LINE) {
			constexpr UChar uu[] = { '_', '_', 0 };
			constexpr UChar rx[] = { '(', '?', ':', '^', '|', '$', '|', ' ', '|', ' ', '.', '+', '?', ' ', ')', 0 }; // (^|$| | .+? )
			size_t pos;
			while ((pos = tag->tag.find(uu)) != UString::npos) {
				tag->tag.replace(pos, 2, rx);
				length += size(rx) - size(uu);
			}
		}

		for (auto iter : state.get_grammar()->regex_tags) {
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(iter, tag->tag.data(), SI32(tag->tag.size()), &status);
			if (status != U_ZERO_ERROR) {
				state.error("%s: Error: uregex_setText(parseTag) returned %s on line %u near `%S` - cannot continue!\n", u_errorName(status), p);
			}
			status = U_ZERO_ERROR;
			if (uregex_find(iter, -1, &status)) {
				tag->type |= T_TEXTUAL;
			}
		}
		for (auto iter : state.get_grammar()->icase_tags) {
			if (ux_strCaseCompare(tag->tag, iter->tag)) {
				tag->type |= T_TEXTUAL;
			}
		}

		if (tag->type & (T_VARIABLE|T_LOCAL_VARIABLE)) {
			size_t pos = tag->tag.find('=');
			if (pos != UString::npos) {
				tag->comparison_op = OP_EQUALS;
				tag->variable_hash = parseTag(&tag->tag[pos + 1], p, state, false)->hash;
				tag->tag[pos] = 0;
				tag->comparison_hash = parseTag(tag->tag.c_str(), p, state, false)->hash;
				tag->tag[pos] = '=';
			}
			else {
				tag->comparison_hash = parseTag(tag->tag.c_str(), p, state, false)->hash;
			}
		}
		else {
			tag->comparison_hash = hash_value(tag->tag);
		}

		if (tag->tag[0] == '<' && tag->tag[length - 1] == '>' && !(tag->type & (T_CASE_INSENSITIVE | T_REGEXP | T_REGEXP_LINE | T_VARSTRING))) {
			tag->parseNumeric(true);
		}
		/*
		if (tag->tag[0] == '#') {
			uint32_t dep_self = 0;
			uint32_t dep_parent = 0;
			if (u_sscanf(tag->tag.data(), "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				tag->type |= T_DEPENDENCY;
			}
			constexpr UChar local_dep_unicode[] = { '#', '%', 'i', u'\u2192', '%', 'i', 0 };
			if (u_sscanf_u(tag->tag.data(), local_dep_unicode, &dep_self, &dep_parent) == 2 && dep_self != 0) {
				tag->type |= T_DEPENDENCY;
			}
		}
		//*/

		if (tag->tag == STR_ASTERIK) {
			tag->type |= T_ANY;
		}
		else if (tag->tag == STR_UU_LEFT) {
			tag->type |= T_PAR_LEFT;
		}
		else if (tag->tag == STR_UU_RIGHT) {
			tag->type |= T_PAR_RIGHT;
		}
		else if (tag->tag == STR_UU_ENCL) {
			tag->type |= T_ENCL;
		}
		else if (tag->tag == STR_UU_TARGET) {
			tag->type |= T_TARGET;
		}
		else if (tag->tag == STR_UU_MARK) {
			tag->type |= T_MARK;
		}
		else if (tag->tag == STR_UU_ATTACHTO) {
			tag->type |= T_ATTACHTO;
		}
		else if (tag->tag == STR_UU_SAME_BASIC) {
			tag->type |= T_SAME_BASIC;
		}
		else if (tag->tag == STR_UU_C1) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 1;
		}
		else if (tag->tag == STR_UU_C2) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 2;
		}
		else if (tag->tag == STR_UU_C3) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 3;
		}
		else if (tag->tag == STR_UU_C4) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 4;
		}
		else if (tag->tag == STR_UU_C5) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 5;
		}
		else if (tag->tag == STR_UU_C6) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 6;
		}
		else if (tag->tag == STR_UU_C7) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 7;
		}
		else if (tag->tag == STR_UU_C8) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 8;
		}
		else if (tag->tag == STR_UU_C9) {
			tag->type |= T_CONTEXT;
			tag->context_ref_pos = 9;
		}

		if (tag->type & T_REGEXP) {
			if (tag->tag == STR_RXTEXT_ANY || tag->tag == STR_RXBASE_ANY || tag->tag == STR_RXWORD_ANY) {
				// ToDo: Add a case-insensitive version of T_REGEXP_ANY for unification
				tag->type |= T_REGEXP_ANY;
				tag->type &= ~T_REGEXP;
			}
			else {
				UParseError pe;
				UErrorCode status = U_ZERO_ERROR;

				UString rt;
				if (tag->tag[0] == '/' && tag->tag[length - 1] == '/') {
					rt = tag->tag.substr(1, length - 2);
				}
				else {
					rt += '^';
					rt += tag->tag;
					rt += '$';
				}

				if (tag->type & T_CASE_INSENSITIVE) {
					tag->regexp = uregex_open(rt.data(), SI32(rt.size()), UREGEX_CASE_INSENSITIVE, &pe, &status);
				}
				else {
					tag->regexp = uregex_open(rt.data(), SI32(rt.size()), 0, &pe, &status);
				}
				if (status != U_ZERO_ERROR) {
					state.error("%s: Error: uregex_open returned %s trying to parse tag %S on line %u near `%S` - cannot continue!\n", u_errorName(status), tag->tag.data(), p);
				}
			}
		}
		if (tag->type & (T_CASE_INSENSITIVE | T_REGEXP)) {
			if (tag->tag[0] == '/' && tag->tag[length - 1] == '/') {
				tag->tag.resize(tag->tag.size() - 1);
				tag->tag.erase(tag->tag.begin());
			}
		}

	label_isVarstring:;
	}

	tag->type &= ~T_SPECIAL;
	if (tag->type & MASK_TAG_SPECIAL) {
		tag->type |= T_SPECIAL;
	}

	if (tag->type & T_VARSTRING && tag->type & (T_REGEXP | T_REGEXP_ANY | T_VARIABLE | T_LOCAL_VARIABLE | T_META)) {
		state.error("%s: Error: Tag %S cannot mix varstring with any other special feature on line %u near `%S`!\n", to, p);
	}

	if (USV(tag->tag) != to) {
		tag->tag_raw = to;
	}

	return state.addTag(tag);
}

template<typename State>
Set* parseSet(const UChar* name, const UChar* p, State& state) {
	uint32_t sh = hash_value(name);

	if (ux_isSetOp(name) != S_IGNORE) {
		state.error("%s: Error: Found set operator '%S' where set name expected on line %u near `%S`!\n", name, p);
	}

	if ((
	      (name[0] == '$' && name[1] == '$') || (name[0] == '&' && name[1] == '&')) &&
	    name[2]) {
		const UChar* wname = &(name[2]);
		const UChar wname2[1024]{};
		if (u_sscanf(wname, "%*u:%S", &wname2) == 1) {
			wname = wname2;
		}
		uint32_t wrap = hash_value(wname);
		Set* wtmp = state.get_grammar()->getSet(wrap);
		if (!wtmp) {
			state.error("%s: Error: Attempted to reference undefined set '%S' on line %u near `%S`!\n", wname, p);
		}
		Set* tmp = state.get_grammar()->getSet(sh);
		if (!tmp) {
			Set* ns = state.get_grammar()->allocateSet();
			ns->line = state.get_grammar()->lines;
			ns->setName(name);
			ns->sets.push_back(wtmp->hash);
			if (name[0] == '$' && name[1] == '$') {
				ns->type |= ST_TAG_UNIFY;
			}
			else if (name[0] == '&' && name[1] == '&') {
				ns->type |= ST_SET_UNIFY;
			}
			state.get_grammar()->addSet(ns);
		}
	}
	if (state.get_grammar()->set_alias.find(sh) != state.get_grammar()->set_alias.end()) {
		sh = state.get_grammar()->set_alias[sh];
	}
	Set* tmp = state.get_grammar()->getSet(sh);
	if (!tmp) {
		if (!state.strict_tags.empty() || !state.list_tags.empty()) {
			Tag* tag = parseTag(name, p, state);
			if (state.strict_tags.count(tag->plain_hash) || state.list_tags.count(tag->plain_hash)) {
				Set* ns = state.get_grammar()->allocateSet();
				ns->line = state.get_grammar()->lines;
				ns->setName(name);
				state.get_grammar()->addTagToSet(tag, ns);
				state.get_grammar()->addSet(ns);
				return ns;
			}
		}
		state.error("%s: Error: Attempted to reference undefined set '%S' on line %u near `%S`!\n", name, p);
	}
	return tmp;
}
}

#endif

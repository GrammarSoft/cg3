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

#pragma once
#ifndef c6d28b7452ec699b_PARSER_HELPERS_H
#define c6d28b7452ec699b_PARSER_HELPERS_H

#include "Tag.hpp"
#include "Set.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"

namespace CG3 {

template<typename State>
Tag* parseTag(const UChar* to, const UChar* p, State& state) {
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
	if ((it = state.get_grammar()->single_tags.find(thash)) != state.get_grammar()->single_tags.end() && !it->second->tag.empty() && u_strcmp(it->second->tag.c_str(), to) == 0) {
		return it->second;
	}

	Tag* tag = state.get_grammar()->allocateTag();
	tag->type = 0;

	if (to[0]) {
		const UChar* tmp = to;
		while (tmp[0] && (tmp[0] == '!' || tmp[0] == '^')) {
			if (tmp[0] == '!' || tmp[0] == '^') {
				tag->type |= T_FAILFAST;
				tmp++;
			}
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
			tmp += 4;
			length -= 4;
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
				state.error("%s: Error: Parsing tag %S resulted in an empty tag on line %u near `%S` - cannot continue!\n", tag->tag.c_str(), p);
			}

			goto label_isVarstring;
		}

		if (tmp[0] && (tmp[0] == '"' || tmp[0] == '<' || tmp[0] == '/')) {
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
				tag->type &= ~T_CASE_INSENSITIVE;
				tag->type &= ~T_WORDFORM;
				tag->type &= ~T_BASEFORM;
				length = oldlength;
			}
		}

		for (size_t i = 0, oldlength = length; tmp[i] != 0 && i < oldlength; ++i) {
			if (tmp[i] == '\\') {
				++i;
				--length;
			}
			if (tmp[i] == 0) {
				break;
			}
			tag->tag += tmp[i];
		}
		if (tag->tag.empty()) {
			state.error("%s: Error: Parsing tag %S resulted in an empty tag on line %u near `%S` - cannot continue!\n", tag->tag.c_str(), p);
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
			uregex_setText(iter, tag->tag.c_str(), tag->tag.size(), &status);
			if (status != U_ZERO_ERROR) {
				state.error("%s: Error: uregex_setText(parseTag) returned %s on line %u near `%S` - cannot continue!\n", u_errorName(status), p);
			}
			status = U_ZERO_ERROR;
			if (uregex_matches(iter, 0, &status)) {
				tag->type |= T_TEXTUAL;
			}
		}
		for (auto iter : state.get_grammar()->icase_tags) {
			UErrorCode status = U_ZERO_ERROR;
			if (u_strCaseCompare(tag->tag.c_str(), tag->tag.size(), iter->tag.c_str(), iter->tag.size(), U_FOLD_CASE_DEFAULT, &status) == 0) {
				tag->type |= T_TEXTUAL;
			}
			if (status != U_ZERO_ERROR) {
				state.error("%s: Error: u_strCaseCompare(parseTag) returned %s on line %u near `%S` - cannot continue!\n", u_errorName(status), p);
			}
		}

		tag->comparison_hash = hash_value(tag->tag);

		if (tag->tag[0] == '<' && tag->tag[length - 1] == '>') {
			tag->parseNumeric();
		}
		/*
		if (tag->tag[0] == '#') {
			uint32_t dep_self = 0;
			uint32_t dep_parent = 0;
			if (u_sscanf(tag->tag.c_str(), "#%i->%i", &dep_self, &dep_parent) == 2 && dep_self != 0) {
				tag->type |= T_DEPENDENCY;
			}
			constexpr UChar local_dep_unicode[] = { '#', '%', 'i', L'\u2192', '%', 'i', 0 };
			if (u_sscanf_u(tag->tag.c_str(), local_dep_unicode, &dep_self, &dep_parent) == 2 && dep_self != 0) {
				tag->type |= T_DEPENDENCY;
			}
		}
		//*/

		if (u_strcmp(tag->tag.c_str(), stringbits[S_ASTERIK].getTerminatedBuffer()) == 0) {
			tag->type |= T_ANY;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_LEFT].getTerminatedBuffer()) == 0) {
			tag->type |= T_PAR_LEFT;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_RIGHT].getTerminatedBuffer()) == 0) {
			tag->type |= T_PAR_RIGHT;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_ENCL].getTerminatedBuffer()) == 0) {
			tag->type |= T_ENCL;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_TARGET].getTerminatedBuffer()) == 0) {
			tag->type |= T_TARGET;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_MARK].getTerminatedBuffer()) == 0) {
			tag->type |= T_MARK;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_ATTACHTO].getTerminatedBuffer()) == 0) {
			tag->type |= T_ATTACHTO;
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_UU_SAME_BASIC].getTerminatedBuffer()) == 0) {
			tag->type |= T_SAME_BASIC;
		}

		if (tag->type & T_REGEXP) {
			if (u_strcmp(tag->tag.c_str(), stringbits[S_RXTEXT_ANY].getTerminatedBuffer()) == 0 || u_strcmp(tag->tag.c_str(), stringbits[S_RXBASE_ANY].getTerminatedBuffer()) == 0 || u_strcmp(tag->tag.c_str(), stringbits[S_RXWORD_ANY].getTerminatedBuffer()) == 0) {
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
					tag->regexp = uregex_open(rt.c_str(), rt.length(), UREGEX_CASE_INSENSITIVE, &pe, &status);
				}
				else {
					tag->regexp = uregex_open(rt.c_str(), rt.length(), 0, &pe, &status);
				}
				if (status != U_ZERO_ERROR) {
					state.error("%s: Error: uregex_open returned %s trying to parse tag %S on line %u near `%S` - cannot continue!\n", u_errorName(status), tag->tag.c_str(), p);
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

	if (tag->type & T_VARSTRING && tag->type & (T_REGEXP | T_REGEXP_ANY | T_VARIABLE | T_META)) {
		state.error("%s: Error: Tag %S cannot mix varstring with any other special feature on line %u near `%S`!\n", to, p);
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

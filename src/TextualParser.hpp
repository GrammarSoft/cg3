/*
* Copyright (C) 2007-2021, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_TEXTUALPARSER_H
#define c6d28b7452ec699b_TEXTUALPARSER_H

#include "IGrammarParser.hpp"
#include "Strings.hpp"
#include "sorted_vector.hpp"

namespace CG3 {
class Rule;
class Set;
class Tag;
class ContextualTest;

class TextualParser : public IGrammarParser {
public:
	TextualParser(Grammar& result, std::ostream& ux_err, bool dump_ast = false);

	void setCompatible(bool compat);
	void setVerbosity(uint32_t level);
	void print_ast(std::ostream& out);

	int parse_grammar(const char* buffer, size_t length);
	int parse_grammar(const UChar* buffer, size_t length);
	int parse_grammar(const std::string& buffer);
	int parse_grammar(const char* filename);

	[[noreturn]] void error(const char* str);
	[[noreturn]] void error(const char* str, UChar c);
	[[noreturn]] void error(const char* str, const UChar* p);
	[[noreturn]] void error(const char* str, const UChar* p, const UString& msg);
	[[noreturn]] void error(const char* str, UChar c, const UChar* p);
	[[noreturn]] void error(const char* str, const char* s, const UChar* p);
	[[noreturn]] void error(const char* str, const UChar* s, const UChar* p);
	[[noreturn]] void error(const char* str, const char* s, const UChar* S, const UChar* p);
	Tag* addTag(Tag* tag);
	Grammar* get_grammar() { return result; }
	const char* filebase = nullptr;
	uint32SortedVector strict_tags;
	uint32SortedVector list_tags;

private:
	UChar nearbuf[32]{};
	uint32_t verbosity_level = 0;
	uint32_t sets_counter = 100;
	uint32_t seen_mapping_prefix = 0;
	flags_t section_flags;
	bool option_vislcg_compat = false;
	bool in_section = false, in_before_sections = true, in_after_sections = false, in_null_section = false, in_nested_rule = false;
	bool no_isets = false, no_itmpls = false, strict_wforms = false, strict_bforms = false, strict_second = false, strict_regex = false, strict_icase = false;
	bool self_no_barrier = false;
	bool only_sets = false;
	Rule* nested_rule = nullptr;
	const char* filename = nullptr;

	typedef std::unordered_map<ContextualTest*, std::pair<size_t, UString>> deferred_t;
	deferred_t deferred_tmpls;
	std::vector<std::unique_ptr<UString>> grammarbufs;

	int parse_grammar(UString& buffer);
	void parseFromUChar(UChar* input, const char* fname = nullptr);
	void addRuleToGrammar(Rule* rule);

	Tag* parseTag(const UChar* to, const UChar* p = nullptr);
	Tag* parseTag(const UString& to, const UChar* p = nullptr);
	Tag* parseTag(const UStringView& to, const UChar* p = nullptr) {
		return parseTag(to.data(), p);
	}
	void parseTagList(UChar*& p, Set* s);
	Set* parseSet(const UChar* name, const UChar* p = nullptr);
	Set* parseSetInline(UChar*& p, Set* s = nullptr);
	Set* parseSetInlineWrapper(UChar*& p);
	void parseContextualTestPosition(UChar*& p, ContextualTest& t);
	ContextualTest* parseContextualTestList(UChar*& p, Rule* rule = nullptr, bool in_tmpl = false);
	void parseContextualTests(UChar*& p, Rule* rule);
	void parseContextualDependencyTests(UChar*& p, Rule* rule);
	flags_t parseRuleFlags(UChar*& p);
	void parseRule(UChar*& p, KEYWORDS key);
	void parseAnchorish(UChar*& p, bool rule_flags = true);

	int error_counter = 0;
	[[noreturn]] void incErrorCount();
};
}

#endif

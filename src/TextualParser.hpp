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

	void error(const char* str);
	void error(const char* str, UChar c);
	void error(const char* str, const UChar* p);
	void error(const char* str, UChar c, const UChar* p);
	void error(const char* str, const char* s, const UChar* p);
	void error(const char* str, const UChar* s, const UChar* p);
	void error(const char* str, const char* s, const UChar* S, const UChar* p);
	Tag* addTag(Tag* tag);
	Grammar* get_grammar() { return result; }
	const char* filebase;
	uint32SortedVector strict_tags;
	uint32SortedVector list_tags;

private:
	UChar nearbuf[32];
	uint32_t verbosity_level;
	uint32_t sets_counter;
	uint32_t seen_mapping_prefix;
	bool option_vislcg_compat;
	bool in_section, in_before_sections, in_after_sections, in_null_section;
	bool no_isets, no_itmpls, strict_wforms, strict_bforms, strict_second, strict_regex = false, strict_icase = false;
	const char* filename;

	typedef std::unordered_map<ContextualTest*, std::pair<size_t, UString>> deferred_t;
	deferred_t deferred_tmpls;
	std::vector<std::unique_ptr<UString>> grammarbufs;

	int parse_grammar(UString& buffer);
	void parseFromUChar(UChar* input, const char* fname = 0);
	void addRuleToGrammar(Rule* rule);

	Tag* parseTag(const UChar* to, const UChar* p = 0);
	void parseTagList(UChar*& p, Set* s);
	Set* parseSet(const UChar* name, const UChar* p = 0);
	Set* parseSetInline(UChar*& p, Set* s = 0);
	Set* parseSetInlineWrapper(UChar*& p);
	void parseContextualTestPosition(UChar*& p, ContextualTest& t);
	ContextualTest* parseContextualTestList(UChar*& p, Rule* rule = 0);
	void parseContextualTests(UChar*& p, Rule* rule);
	void parseContextualDependencyTests(UChar*& p, Rule* rule);
	void parseRule(UChar*& p, KEYWORDS key);
	void parseAnchorish(UChar*& p);

	int error_counter;
	void incErrorCount();
};
}

#endif

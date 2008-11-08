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

#ifndef __TEXTUALPARSER_H
#define __TEXTUALPARSER_H

#include "IGrammarParser.h"

namespace CG3 {
	class TextualParser : public IGrammarParser {
	public:
		TextualParser(UFILE *ux_err);
		~TextualParser();

		void setCompatible(bool compat);
		void setVerbosity(uint32_t level);
		void setResult(CG3::Grammar *result);

		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

	private:
		UFILE *ux_stderr;
		uint32_t verbosity_level;
		uint32_t sets_counter;
		bool option_vislcg_compat;
		bool in_section, in_before_sections, in_after_sections, in_null_section;
		const char *filename;
		const char *locale;
		const char *codepage;
		CG3::Grammar *result;

		int parseFromUChar(UChar *input);
		void addRuleToGrammar(Rule *rule);

		int parseTagList(Set *s, UChar **p, const bool isinline);
		int parseSetInline(Set *s, UChar **p);
		int parseContextualTestList(Rule *rule = 0, ContextualTest **head = 0, CG3::ContextualTest *parentTest = 0, UChar **p = 0, CG3::ContextualTest *self = 0);
		int parseContextualTests(Rule *rule, UChar **p);
		int parseContextualDependencyTests(Rule *rule, UChar **p);
		int parseRule(KEYWORDS key, UChar **p);
	};
}

#endif

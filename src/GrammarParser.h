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
#ifndef __GRAMMARPARSER_H
#define __GRAMMARPARSER_H

#include "stdafx.h"
#include <unicode/uchar.h>
#include <unicode/ustdio.h>
#include "Grammar.h"
 
namespace CG3 {
	class GrammarParser {
	public:
		bool option_vislcg_compat;
		const char *filename;
		const char *locale;
		const char *codepage;
		CG3::Grammar *result;
	
		GrammarParser();
		~GrammarParser();

		void setResult(CG3::Grammar *result);

		int parse_grammar_from_ufile(UFILE *input);
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

		int parseSingleLine(const int key, const UChar *line);

		int parseDelimiters(const UChar *line);
		int parsePreferredTargets(const UChar *line);

		int parseList(const UChar *line);

		int readSetOperator(UChar **paren);
		uint32_t readSingleSet(UChar **paren);
		uint32_t readTagList(UChar **paren, std::list<Tag*> *taglist);
		int parseSet(const UChar *line);

		uint32_t parseTarget(UChar **space);

		int parseContextualTest(UChar **paren, CG3::ContextualTest *test);
		int parseContextualTests(UChar **space, CG3::Rule *rule);
		int parseSelectRemoveIffDelimitMatch(const UChar *line, uint32_t key);
		int parseAddMapReplaceAppend(const UChar *line, uint32_t key);
	};
}

#endif

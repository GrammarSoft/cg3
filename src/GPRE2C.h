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
#ifndef __GPH
#define __GPH

// re2c defines
#define YYCTYPE         UChar

#include "IGrammarParser.h"
 
namespace CG3 {
	class GPRE2C : public IGrammarParser {
	public:
		bool option_vislcg_compat;
		bool in_section, in_before_sections, in_after_sections;
		const char *filename;
		const char *locale;
		const char *codepage;
		Grammar *result;
	
		YYCTYPE *marker;
		YYCTYPE *last_entity;

		GPRE2C();
		~GPRE2C();

		void setCompatible(bool compat);
		void setResult(Grammar *result);
		void addRuleToGrammar(Rule *rule);

		int grammar_from_ufile(UFILE *input);
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

		YYCTYPE *skipline(YYCTYPE *input);
		YYCTYPE *parseCompositeTag(YYCTYPE *input, Set *set);
		YYCTYPE *parseInlineSet(YYCTYPE *input, Set **ret_set);
		YYCTYPE *parseSetList(YYCTYPE *input, Set *set);
		YYCTYPE *parseTagList(YYCTYPE *input, Set *set);
		YYCTYPE *parseSet(YYCTYPE *input);
		YYCTYPE *parseList(YYCTYPE *input);
		YYCTYPE *parseMappingPrefix(YYCTYPE *input);
		YYCTYPE *parseDelimiters(YYCTYPE *input, STRINGS which);
		YYCTYPE *parsePreferredTargets(YYCTYPE *input);

		YYCTYPE *parseMapAddReplaceAppend(YYCTYPE *input, KEYWORDS which);

		KEYWORDS scan(YYCTYPE *input);
	};
}

#endif

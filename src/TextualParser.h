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
#ifndef __TEXTUALPARSER_H
#define __TEXTUALPARSER_H

#include "IGrammarParser.h"

namespace CG3 {
	class TextualParser : public IGrammarParser {
	public:
		TextualParser(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err);
		~TextualParser();

		void setCompatible(bool compat);
		void setResult(CG3::Grammar *result);

		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

	private:
		UFILE *ux_stdin;
		UFILE *ux_stdout;
		UFILE *ux_stderr;
		bool option_vislcg_compat;
		bool in_section, in_before_sections, in_after_sections;
		const char *filename;
		const char *locale;
		const char *codepage;
		CG3::Grammar *result;

		int parseFromUChar(UChar *input);
		void addRuleToGrammar(Rule *rule);
	};
}

#endif

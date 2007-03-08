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
#ifndef __GPRE2C_H
#define __GPRE2C_H

#include "IGrammarParser.h"
 
namespace CG3 {
	class GPRE2C : public IGrammarParser {
	public:
		bool option_vislcg_compat;
		bool in_section, in_before_sections, in_after_sections;
		const char *filename;
		const char *locale;
		const char *codepage;
		CG3::Grammar *result;
	
		GPRE2C();
		~GPRE2C();

		void setCompatible(bool compat);
		void setResult(CG3::Grammar *result);

		int re2c_grammar_from_ufile(UFILE *input);
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

		YYCTYPE *marker;
		YYCTYPE *re2c_skipline(YYCTYPE *input);
		YYCTYPE *re2c_parseCompositeTag(YYCTYPE *input, Set *set);
		YYCTYPE *re2c_parseInlineSet(YYCTYPE *input, Set **ret_set);
		YYCTYPE *re2c_parseSetList(YYCTYPE *input, Set *set);
		YYCTYPE *re2c_parseTagList(YYCTYPE *input, Set *set);
		YYCTYPE *re2c_parseSet(YYCTYPE *input);
		YYCTYPE *re2c_parseList(YYCTYPE *input);
		YYCTYPE *re2c_parseDelimiters(YYCTYPE *input, STRINGS which);
		YYCTYPE *re2c_parsePreferredTargets(YYCTYPE *input);
		KEYWORDS re2c_scan(YYCTYPE *input);
	};
}

#endif

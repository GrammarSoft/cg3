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

#ifndef __STRINGS_H
#define __STRINGS_H

#include <unicode/uregex.h>

namespace CG3 {
	namespace Strings {
		enum KEYWORDS {
			K_IGNORE,
			K_SETS,
			K_LIST,
			K_SET,
			K_DELIMITERS,
			K_PREFERRED_TARGETS,
			K_MAPPINGS,
			K_CONSTRAINTS,
			K_CORRECTIONS,
			K_SECTION,
			K_ADD,
			K_MAP,
			K_REPLACE,
			K_SELECT,
			K_REMOVE,
			K_IFF,
			K_APPEND,
			K_SUBSTITUTE,
			K_END,
			K_ANCHOR,
			K_EXECUTE,
			K_JUMP,
			K_DEFINEGLOBAL,
			K_REMGLOBAL,
			K_SETGLOBAL,
			K_MINVALUE,
			K_MAXVALUE,
			K_DELIMIT,
			K_MATCH,
			KEYWORD_COUNT
		};

		enum STRINGS {
			S_IGNORE,
			S_ADD,
			S_OR,
			S_PLUS,
			S_MINUS,
			S_MULTIPLY,
			S_FAILFAST,
			S_BACKSLASH,
			S_HASH,
			S_NOT,
			S_TEXTNOT,
			S_SPACE,
			S_LINK,
			S_LINKZ,
			S_BARRIER,
			S_DELIMITSET,
			S_ASTERIK,
			S_ASTERIKTWO,
			S_BEGINTAG,
			S_ENDTAG,
			STRINGS_COUNT
		};

		enum REGEXPS {
			R_ANDLINK,
			R_CLEANSTRING,
			REGEXP_COUNT
		};

		extern UChar *keywords[KEYWORD_COUNT];
		extern uint32_t keyword_pow[KEYWORD_COUNT];
		extern UChar *stringbits[STRINGS_COUNT];
		extern uint32_t string_hashes[STRINGS_COUNT];
		extern URegularExpression *regexps[REGEXP_COUNT];

		int init_keywords();
		int free_keywords();

		int init_strings();
		int free_strings();

		int init_regexps();
		int free_regexps();
	}
}

using namespace CG3::Strings;

#endif

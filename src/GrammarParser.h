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
	namespace GrammarParser {
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage, CG3::Grammar *result);

		int parseList(const UChar *line, CG3::Grammar *result);

		int readSetOperator(UChar **paren, CG3::Grammar *result);
		uint32_t readSingleSet(UChar **paren, CG3::Grammar *result);
		int parseSet(const UChar *line, CG3::Grammar *result);

		int parseSelectRemoveIffDelimit(const UChar *line, uint32_t key, CG3::Grammar *result);
	}
}

#endif

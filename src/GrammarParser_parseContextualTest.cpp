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

#include "stdafx.h"
#include "Strings.h"
#include <unicode/uregex.h>
#include "GrammarParser.h"
#include "Rule.h"
#include "Grammar.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

// (*1C N BARRIER NON-ATTR LINK 1 KOMMA)
int GrammarParser::parseContextualTests(UChar **paren, CG3::Rule *rule) {
	while (*paren && (*paren)[0] && (*paren)[0] == '(') {
		int matching = 0;
		if (!ux_findMatchingParenthesis(*paren, 0, &matching)) {
			u_fprintf(ux_stderr, "Error: Unmatched parentheses on or after line %u!\n", result->curline);
		} else {
			(*paren)[matching] = 0;
			UChar *test = (*paren)+1;
			ux_trim(test);

			UChar *space = u_strchr(test, ' ');
			if (!space) {
				u_fprintf(ux_stderr, "Error: Missing whitespace in test \"%S\" on line %u - skipping!\n", test, result->curline);
				*paren += matching+1;
				ux_trim(*paren);
				continue;
			}
			space[0] = 0;

			bool negative = false;
			UChar *position = test;
			test = space+1;
			if (u_strcmp(position, stringbits[S_TEXTNOT]) == 0) {
				negative = true;
				space = u_strchr(test, ' ');
				space[0] = 0;
				position = test;
				test = space+1;
			}

			ContextualTest *context = new ContextualTest;
			context->parsePosition(position);
			delete context;

			*paren += matching+1;
			ux_trim(*paren);
		}
	}
	return 0;
}

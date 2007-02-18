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

int GrammarParser::parseContextualTest(UChar **paren, CG3::ContextualTest *parentTest) {
	UChar *test = *paren;
	ux_trim(test);

	UChar *space = u_strchr(test, ' ');
	if (!space) {
		u_fprintf(ux_stderr, "Error: Missing whitespace in test \"%S\" on line %u - skipping!\n", test, result->curline);
		return 0;
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

	ContextualTest *context = parentTest->allocateContextualTest();
	context->parsePosition(position);
	context->negative = negative;
	parentTest->linked = context;

	context->line = result->curline;
	context->target = parseTarget(&test);

	if (test && test[0]) {
		space = u_strstr(test, stringbits[S_BARRIER]);
		if (space == test) {
			test += 8;
			context->barrier = parseTarget(&test);
		}
		space = u_strstr(test, stringbits[S_LINK]);
		if (space == test) {
			test += 5;
			parseContextualTest(&test, context);
		}
	}

	return 0;
}

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

			ContextualTest *context = rule->allocateContextualTest();
			context->parsePosition(position);
			context->negative = negative;
			rule->addContextualTest(context);

			context->line = result->curline;
			context->target = parseTarget(&test);

			if (test && test[0]) {
				space = u_strstr(test, stringbits[S_BARRIER]);
				if (space == test) {
					test += 8;
					context->barrier = parseTarget(&test);
				}
				space = u_strstr(test, stringbits[S_LINK]);
				if (space == test) {
					test += 5;
					parseContextualTest(&test, context);
				}
			}
			context->rehash();

			*paren += matching+1;
			ux_trim(*paren);
		}
	}
	if (*paren && (*paren)[0] && (*paren)[0] != '(') {
		u_fprintf(ux_stderr, "Warning: Remnant text \"%S\" on line %u - skipping it!\n", *paren, result->curline);
	}
	return 0;
}

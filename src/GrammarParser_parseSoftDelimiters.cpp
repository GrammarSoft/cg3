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

#include "GrammarParser.h"

using namespace CG3;
using namespace CG3::Strings;

int GrammarParser::parseSoftDelimiters(const UChar *line) {
	if (!line) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	int length = u_strlen(line);
	if (!length) {
		u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
		return -1;
	}
	UChar *local = gbuffers[1];
	u_strcpy(local, line+u_strlen(keywords[K_SOFT_DELIMITERS])+1);

	// Allocate temp vars and skips over "SOFT-DELIMITERS = "
	UChar *space = u_strchr(local, ' ');
	space[0] = 0;
	space++;

	CG3::Set *curset = result->allocateSet();
	curset->setName(stringbits[S_SOFTDELIMITSET]);
	curset->line = result->curline;

	UChar *paren = space;
	while(paren && paren[0]) {
		space = u_strchr(paren, ' ');
		if (!space || space[0] == 0) {
			if (u_strlen(paren)) {
				CG3::CompositeTag *ctag = result->allocateCompositeTag();
				CG3::Tag *tag = result->allocateTag(paren);
				tag = result->addTag(tag);
				result->addTagToCompositeTag(tag, ctag);
				result->addCompositeTagToSet(curset, ctag);
			}
			paren = space;
		}
		else if (paren[0] == '(' && paren[-1] != '\\') {
			space = paren;
			int matching = 0;
			if (!ux_findMatchingParenthesis(space, 0, &matching)) {
				u_fprintf(ux_stderr, "Error: Unmatched parentheses on or after line %u!\n", curset->line);
			} else {
				space[matching] = 0;
				UChar *composite = space+1;

				CG3::CompositeTag *ctag = result->allocateCompositeTag();
				UChar *temp = composite;
				while((temp = u_strchr(temp, ' ')) != 0) {
					if (temp[-1] == '\\') {
						temp++;
						continue;
					}
					temp[0] = 0;
					if (composite[0]) {
						CG3::Tag *tag = result->allocateTag(composite);
						tag = result->addTag(tag);
						result->addTagToCompositeTag(tag, ctag);
					}

					temp++;
					composite = temp;
				}
				if (composite[0]) {
					CG3::Tag *tag = result->allocateTag(composite);
					tag = result->addTag(tag);
					result->addTagToCompositeTag(tag, ctag);
				}

				result->addCompositeTagToSet(curset, ctag);

				paren = space+matching+1;
				space = space+matching;
				if (u_isWhitespace(paren[0])) {
					paren++;
				}
			}
		}
		else if (space[0] == ' ' && space[-1] != '\\') {
			space[0] = 0;
			if (u_strlen(paren)) {
				CG3::CompositeTag *ctag = result->allocateCompositeTag();
				CG3::Tag *tag = result->allocateTag(paren);
				tag = result->addTag(tag);
				result->addTagToCompositeTag(tag, ctag);
				result->addCompositeTagToSet(curset, ctag);
			}
			paren = space+1;
		}
	}

	result->addSet(curset);
	result->soft_delimiters = curset;

	return 0;
}

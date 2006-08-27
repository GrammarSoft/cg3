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
#include "Grammar.h"
#include "uextras.h"

using namespace CG3::Strings;

namespace CG3 {
	namespace GrammarParser {
		int parseList(const UChar *line, CG3::Grammar *result) {
			if (!line) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->lines);
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->lines);
				return -1;
			}
			UChar *local = new UChar[length+1];
			//memset(local, 0, length+1);
			u_strcpy(local, line+u_strlen(keywords[K_LIST])+1);

			// Allocate temp vars and skips over "LIST X = "
			UChar *space = u_strchr(local, ' ');
			space[0] = 0;
			space+=3;

			CG3::Set *curset = result->allocateSet();
			curset->setName(local);
			curset->setLine(result->lines);

			UChar *paren = space;
			while(paren[0]) {
				if (space[0] == 0) {
					if (u_strlen(paren)) {
						CG3::CompositeTag *ctag = result->allocateCompositeTag();
						CG3::Tag *tag = ctag->allocateTag(paren);
						tag->parseTag(paren);
						ctag->addTag(tag);
						result->addCompositeTagToSet(curset, ctag);
					}
					paren = space;
				}
				else if (space[0] == ' ') {
					if (space[-1] != '\\') {
						space[0] = 0;
						if (u_strlen(paren)) {
							CG3::CompositeTag *ctag = result->allocateCompositeTag();
							CG3::Tag *tag = ctag->allocateTag(paren);
							tag->parseTag(paren);
							ctag->addTag(tag);
							result->addCompositeTagToSet(curset, ctag);
						}
						paren = space+1;
					}
				}
				else if (space[0] == '(') {
					if (space[-1] != '\\') {
						int matching = 0;
						if (!ux_findMatchingParenthesis(space, 0, &matching)) {
							u_fprintf(ux_stderr, "Error: Unmatched parentheses on or after line %u!\n", curset->getLine());
						} else {
							space[matching] = 0;
							UChar *composite = space+1;
							ux_trimUChar(composite);

							CG3::CompositeTag *ctag = result->allocateCompositeTag();
							UChar *temp = composite;
							while(temp = u_strchr(temp, ' ')) {
								if (temp[-1] == '\\') {
									temp++;
									continue;
								}
								temp[0] = 0;
								CG3::Tag *tag = ctag->allocateTag(composite);
								tag->parseTag(composite);
								ctag->addTag(tag);

								temp++;
								composite = temp;
							}
							CG3::Tag *tag = ctag->allocateTag(composite);
							tag->parseTag(composite);
							ctag->addTag(tag);

							result->addCompositeTagToSet(curset, ctag);

							paren = space+matching+1;
							space = space+matching;
							ux_trimUChar(paren);
						}
					}
				}
				space++;
			}

			result->addSet(curset);

			delete local;
			return 0;
		}
	}
}

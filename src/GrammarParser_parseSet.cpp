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
		int readSetOperator(UChar **paren, CG3::Grammar *result) {
			UChar *space = 0;
			int set_op = 0;
			ux_trimUChar(*paren);
			space = u_strchr(*paren, ' ');
			if (space) {
				space[0] = 0;
				set_op = ux_isSetOp(*paren);
				if (!set_op) {
					space[0] = ' ';
					return 0;
				}
			} else {
				set_op = ux_isSetOp(*paren);
				if (!set_op) {
					return 0;
				}
			}
			*paren = *paren+u_strlen(*paren)+1;
			return set_op;
		}

		uint32_t readSingleSet(UChar **paren, CG3::Grammar *result) {
			ux_trimUChar(*paren);
			UChar *space = u_strchr(*paren, ' ');
			uint32_t retval = 0;

			if ((*paren)[0] == '(') {
				space = (*paren);
				int matching = 0;
				if (!ux_findMatchingParenthesis(space, 0, &matching)) {
					std::cerr << "Error: Unmatched parentheses on or after line " << result->lines << std::endl;
				} else {
					space[matching] = 0;
					UChar *composite = space+1;
					ux_trimUChar(composite);

					CG3::Set *set_c = result->allocateSet();
					set_c->setName(hash_sdbm_uchar(composite));
					retval = hash_sdbm_uchar(set_c->getName());

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

					result->addCompositeTagToSet(set_c, ctag);
					set_c->setLine(result->lines);
					result->addSet(set_c);

					*paren = space+matching+1;
					space = space+matching;
					ux_trimUChar(*paren);
				}
			}
			else if (space && space[0] == ' ') {
				space[0] = 0;
				if (u_strlen(*paren)) {
					retval = hash_sdbm_uchar(*paren);
				}
				*paren = space+1;
			} else if (u_strlen(*paren)) {
				retval = hash_sdbm_uchar(*paren);
				*paren = *paren+u_strlen(*paren);
			}
			return retval;
		}

		int parseSet(const UChar *line, const uint32_t which, CG3::Grammar *result) {
			if (!which) {
				std::cerr << "Error: No line number provided - cannot continue!" << std::endl;
				return -1;
			}
			if (!line) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				std::cerr << "Error: No string provided at line " << which << " - cannot continue!" << std::endl;
				return -1;
			}
			UChar *local = new UChar[length+1];
			//memset(local, 0, length+1);
			u_strcpy(local, line+u_strlen(keywords[K_SET])+1);

			// Allocate temp vars and skips over "SET X = "
			UChar *space = u_strchr(local, ' ');
			space[0] = 0;
			space+=3;

			CG3::Set *curset = result->allocateSet();
			curset->setName(local);
			curset->setLine(which);
			result->addSet(curset);

			uint32_t set_a = 0;
			uint32_t set_b = 0;
			uint32_t res = hash_sdbm_uchar(curset->getName());
			int set_op = S_IGNORE;
			while(space[0]) {
				if (!set_a) {
					set_a = readSingleSet(&space, result);
					if (!set_a) {
						std::cerr << "Error: Could not read in left hand set on line " << which << " - cannot continue!" << std::endl;
						break;
					}
				}
				if (!set_op) {
					set_op = readSetOperator(&space, result);
					if (!set_op) {
						std::cerr << "Warning: Could not read in set operator on line " << which << " - assuming set alias." << std::endl;
						result->manipulateSet(res, S_OR, set_a, res);
						break;
					}
				}
				if (!set_b) {
					set_b = readSingleSet(&space, result);
					if (!set_b) {
						std::cerr << "Error: Could not read in right hand set on line " << which << " - cannot continue!" << std::endl;
						break;
					}
				}
				if (set_a && set_b && set_op) {
					result->manipulateSet(set_a, set_op, set_b, res);
					set_op = 0;
					set_b = 0;
					set_a = res;
				}
			}

			delete local;
			return 0;
		}
	}
}

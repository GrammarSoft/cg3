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
		int parseSelectRemoveIffDelimit(const UChar *line, uint32_t key, CG3::Grammar *result) {
			if (!line) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
				return -1;
			}
			int length = u_strlen(line);
			if (!length) {
				u_fprintf(ux_stderr, "Error: No string provided at line %u - cannot continue!\n", result->curline);
				return -1;
			}
			if (key != K_SELECT && key != K_REMOVE && key != K_IFF && key != K_DELIMIT) {
				u_fprintf(ux_stderr, "Error: Invalid keyword %u for line %u - cannot continue!\n", key, result->curline);
				return -1;
			}

			UChar *local = new UChar[length+1];
			u_strcpy(local, line);
			UChar *space = u_strchr(local, ' ');
			space[0] = 0;
			UChar *wordform = 0;

			// ToDo: Add parsing for RULE:name
			if (u_strcmp(local, keywords[key]) != 0) {
				wordform = local;
				space++;
				space = u_strchr(space, ' ');
				space[0] = 0;
				space++;
			}

			CG3::Set *curset = result->allocateSet();
			curset->setLine(result->curline);
			curset->setName(result->curline);
			result->addSet(curset);

			uint32_t set_a = 0;
			uint32_t set_b = 0;
			uint32_t res = hash_sdbm_uchar(curset->getName());
			int set_op = S_IGNORE;
			while(space[0]) {
				if (!set_a) {
					set_a = readSingleSet(&space, result);
					if (!set_a) {
//						u_fprintf(ux_stderr, "Error: Could not read in left hand set on line %u for set %S - cannot continue!\n", result->curline, local);
						break;
					}
				}
				if (!set_op) {
					set_op = readSetOperator(&space, result);
					if (!set_op) {
//						u_fprintf(ux_stderr, "Warning: Could not read in operator on line %u for set %S - assuming set alias.\n", result->curline, local);
						result->manipulateSet(res, S_OR, set_a, res);
						break;
					}
				}
				if (!set_b) {
					set_b = readSingleSet(&space, result);
					if (!set_b) {
//						u_fprintf(ux_stderr, "Error: Could not read in right hand set on line %u for set %S - cannot continue!\n", result->curline, local);
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

			CG3::Rule *rule = result->allocateRule();
			rule->line = result->curline;
			rule->target = res;
			result->addRule(rule);

			delete local;
			return 0;
		}
	}
}

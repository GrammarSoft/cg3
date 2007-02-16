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

using namespace CG3;
using namespace CG3::Strings;

uint32_t GrammarParser::parseTarget(UChar **space) {
	CG3::Set *curset = result->allocateSet();
	curset->line = result->curline;
	curset->setName(hash_sdbm_uchar(*space, 0));

	bool only_or = true;
	uint32_t set_a = 0;
	uint32_t set_b = 0;
	uint32_t res = hash_sdbm_uchar(curset->name, 0);
	int set_op = S_IGNORE;
	while ((*space)[0]) {
		if (!set_a) {
			set_a = readSingleSet(space);
			if (!set_a) {
				u_fprintf(ux_stderr, "Error: Could not read in left hand set on line %u for set %S - cannot continue!\n", result->curline, curset->name);
				break;
			}
			curset->sets.push_back(set_a);
		}
		if (!set_op) {
			set_op = readSetOperator(space);
			if (!set_op) {
				break;
			}
			if (option_vislcg_compat && set_op == S_MINUS) {
//				u_fprintf(ux_stderr, "Warning: Set %S on line %u - difference operator converted to fail-fast as per --vislcg-compat.\n", curset->name, result->curline);
				set_op = S_FAILFAST;
			}
			curset->set_ops.push_back(set_op);
			if (set_op != S_OR) {
				only_or = false;
			}
			set_op = 0;
		}
		if (!set_b) {
			set_b = readSingleSet(space);
			if (!set_b) {
				u_fprintf(ux_stderr, "Error: Could not read in right hand set on line %u for set %S - cannot continue!\n", result->curline, curset->name);
				break;
			}
			curset->sets.push_back(set_b);
			set_b = 0;
		}
	}

	if (curset->sets.size() == 1) {
		res = curset->sets.at(0);
		result->destroySet(curset);
		curset = result->getSet(res);
	}
/*
	else if (only_or) {
		bool only_simple = true;
		for (uint32_t i=0;i<curset->sets.size();i++) {
			Set *set = result->getSet(curset->sets[i]);
			if (!set->sets.empty()) {
				only_simple = false;
			}
		}
		if (only_simple) {
			for (uint32_t i=0;i<curset->sets.size();i++) {
				Set *set = result->getSet(curset->sets[i]);
				curset->tags.insert(set->tags.begin(), set->tags.end());
				curset->tags_map.insert(set->tags.begin(), set->tags.end());
				curset->single_tags.insert(set->single_tags.begin(), set->single_tags.end());
				curset->tags_map.insert(set->single_tags.begin(), set->single_tags.end());
			}
			curset->sets.clear();
			curset->set_ops.clear();
		}
	}
//*/


	result->addSet(curset);

	return curset->hash;
}

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

// ToDo: Only precalc complex operations, or none at all.
uint32_t GrammarParser::parseTarget(UChar **space) {
	CG3::Set *curset = result->allocateSet();
	curset->setLine(result->curline);
	curset->setName(hash_sdbm_uchar(*space, 0));
	result->addSet(curset);

	std::vector<uint32_t> sets;
	uint32_t set_a = 0;
	uint32_t set_b = 0;
	uint32_t res = hash_sdbm_uchar(curset->getName(), 0);
	int set_op = S_IGNORE;
	while ((*space)[0]) {
		if (!set_a) {
			set_a = readSingleSet(space);
			if (!set_a) {
				u_fprintf(ux_stderr, "Error: Could not read in left hand set on line %u for set %S - cannot continue!\n", result->curline, curset->getName());
				break;
			}
			sets.push_back(set_a);
//			set_a = 0;
		}
		if (!set_op) {
			set_op = readSetOperator(space);
			if (!set_op) {
				sets.push_back(set_a);
				break;
			}
			if (option_vislcg_compat && set_op == S_MINUS) {
				u_fprintf(ux_stderr, "Warning: Set %S on line %u - difference operator converted to fail-fast as per --vislcg-compat.\n", curset->name, result->curline);
				set_op = S_FAILFAST;
			}
		}
		if (!set_b) {
			set_b = readSingleSet(space);
			if (!set_b) {
				u_fprintf(ux_stderr, "Error: Could not read in right hand set on line %u for set %S - cannot continue!\n", result->curline, curset->getName());
				break;
			}
			sets.push_back(set_b);
			set_b = 0;
		}

		set_a = sets.at(sets.size()-2);
		set_b = sets.at(sets.size()-1);
		if (set_a && set_b && set_op) {
			if (set_op != S_OR) {
				sets.pop_back();
				sets.pop_back();
				CG3::Set *curset = result->allocateSet();
				curset->setLine(result->curline);
				curset->setName(clock()+rand()+hash_sdbm_uchar(*space, 0));
				result->addSet(curset);

				uint32_t res = hash_sdbm_uchar(curset->getName(), 0);
				result->manipulateSet(set_a, set_op, set_b, res);
				sets.push_back(res);
				set_op = 0;
				set_b = 0;
			}
			else {
				set_a = set_b;
				set_b = 0;
			}
		}
		set_op = 0;
	}

	for (uint32_t i=0;i<sets.size();i++) {
		result->manipulateSet(res, S_OR, sets.at(i), res);
	}

	curset = result->getSet(res);
	result->addUniqSet(curset);
	if (curset->hash) {
		result->uniqsets[curset->hash]->used = true;
	}

	return curset->hash;
}

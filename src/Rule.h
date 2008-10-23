/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __RULE_H
#define __RULE_H

#include "stdafx.h"
#include "Strings.h"
#include "ContextualTest.h"
#include "Tag.h"
#include "ContextualTest.h"

namespace CG3 {

	class Grammar;

	class Rule {
	public:
		UChar *name;
		uint32_t wordform;
		uint32_t target;
		uint32_t line;
		uint32_t varname, varvalue;
		uint32_t jumpstart, jumpend;
		int32_t section;
		// ToDo: Add proper "quality" quantifier based on num_fail, num_match, total_time
		double weight, quality;
		KEYWORDS type;
		TagList maplist;
		uint32List sublist;
		
		mutable std::list<ContextualTest*> tests;
		mutable uint32_t num_fail, num_match;
		mutable clock_t total_time;
		mutable ContextualTest *dep_target;
		mutable std::list<ContextualTest*> dep_tests;

		Rule();
		~Rule();
		void setName(const UChar *to);
		
		void resetStatistics();

		ContextualTest *allocateContextualTest();
		void addContextualTest(ContextualTest *to, std::list<ContextualTest*> *thelist);

		static bool cmp_quality(Rule *a, Rule *b);
	};

	struct compare_Rule {
		inline bool operator() (const Rule* a, const Rule* b) const {
			return a->line < b->line;
		}
	};

}

#endif

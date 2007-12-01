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
		int32_t section;
		// ToDo: Add proper "quality" quantifier based on num_fail, num_match, total_time
		double weight, quality;
		KEYWORDS type;
		uint32List maplist;
		uint32List sublist;
		
		mutable std::list<ContextualTest*> tests;
		mutable uint32_t num_fail, num_match;
		mutable PACC_TimeStamp total_time;
		mutable ContextualTest *dep_target;
		mutable std::list<ContextualTest*> dep_tests;

		Rule();
		~Rule();
		void setName(uint32_t to);
		void setName(const UChar *to);
		
		void reset();

		ContextualTest *allocateContextualTest();
		void addContextualTest(ContextualTest *to, std::list<ContextualTest*> *thelist);

		static bool cmp_quality(Rule *a, Rule *b);
	};

}

#endif

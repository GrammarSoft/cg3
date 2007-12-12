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
#ifndef __CONTEXTUALTEST_H
#define __CONTEXTUALTEST_H

#include "stdafx.h"
#include "Strings.h"

namespace CG3 {

	enum CT_POS {
		POS_CAREFUL = 1,
		POS_NEGATED = 2,
		POS_NEGATIVE = 4,
		POS_SCANFIRST = 8,
		POS_SCANALL = 16,
		POS_ABSOLUTE = 32,
		POS_SPAN_RIGHT = 64,
		POS_SPAN_LEFT = 128,
		POS_SPAN_BOTH = 256,
		POS_DEP_PARENT = 512,
		POS_DEP_SIBLING = 1024,
		POS_DEP_CHILD = 2048
	};

	class ContextualTest {
	public:
		uint32_t line;
		uint32_t hash;
		uint32_t pos;
		int32_t offset;

		uint32_t target;
		uint32_t barrier;
		uint32_t cbarrier;

		mutable uint32_t num_fail, num_match;
		mutable clock_t total_time;

		ContextualTest *linked;

		ContextualTest();
		~ContextualTest();

		void parsePosition(const UChar *input, UFILE *ux_stderr);

		ContextualTest *allocateContextualTest();
		
		uint32_t rehash();
		void resetStatistics();

		static bool cmp_quality(ContextualTest *a, ContextualTest *b);
	};

}

#endif

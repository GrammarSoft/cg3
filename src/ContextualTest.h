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

#pragma once
#ifndef __CONTEXTUALTEST_H
#define __CONTEXTUALTEST_H

#include "stdafx.h"

namespace CG3 {

	enum CT_POS {
		POS_CAREFUL        =        1U,
		POS_NEGATED        =        2U,
		POS_NEGATIVE       =        4U,
		POS_SCANFIRST      =        8U,
		POS_SCANALL        =       16U,
		POS_ABSOLUTE       =       32U,
		POS_SPAN_RIGHT     =       64U,
		POS_SPAN_LEFT      =      128U,
		POS_SPAN_BOTH      =      256U,
		POS_DEP_PARENT     =      512U,
		POS_DEP_SIBLING    =     1024U,
		POS_DEP_CHILD      =     2048U,
		POS_PASS_ORIGIN    =     4096U,
		POS_NO_PASS_ORIGIN =     8192U,
		POS_LEFT_PAR       =    16384U,
		POS_RIGHT_PAR      =    32768U,
		POS_DEP_SELF       =    65536U,
		POS_DEP_NONE       =   131072U,
		POS_DEP_ALL        =   262144U,
		POS_DEP_DEEP       =   524288U,
		POS_MARK_SET       =  1048576U,
		POS_MARK_JUMP      =  2097152U,
		POS_LOOK_DELETED   =  4194304U,
		POS_LOOK_DELAYED   =  8388608U,
		POS_TMPL_OVERRIDE  = 16777216U,
		POS_NONE           = 33554432U
	};

	class ContextualTest {
	public:
		bool is_used;
		int32_t offset;
		uint32_t line;
		uint32_t name;
		uint32_t hash;
		uint32_t pos;
		uint32_t target;
		uint32_t barrier;
		uint32_t cbarrier;
		mutable uint32_t num_fail, num_match;
		mutable double total_time;
		ContextualTest *tmpl;
		ContextualTest *linked;
		ContextualTest *prev, *next;

		std::list<ContextualTest*> ors;
		void detach();

		ContextualTest();
		~ContextualTest();

		void parsePosition(const UChar *input, UFILE *ux_stderr);

		ContextualTest *allocateContextualTest();
		
		uint32_t rehash();
		uint32_t getHash();
		void resetStatistics();
		void markUsed(Grammar &grammar);
	};

}

#endif

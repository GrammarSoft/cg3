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
	class Grammar;

	enum CT_POS {
		POS_CAREFUL        =        0x1,
		POS_NEGATED        =        0x2,
		POS_NEGATIVE       =        0x4,
		POS_SCANFIRST      =        0x8,
		POS_SCANALL        =       0x10,
		POS_ABSOLUTE       =       0x20,
		POS_SPAN_RIGHT     =       0x40,
		POS_SPAN_LEFT      =       0x80,
		POS_SPAN_BOTH      =      0x100,
		POS_DEP_PARENT     =      0x200,
		POS_DEP_SIBLING    =      0x400,
		POS_DEP_CHILD      =      0x800,
		POS_PASS_ORIGIN    =     0x1000,
		POS_NO_PASS_ORIGIN =     0x2000,
		POS_LEFT_PAR       =     0x4000,
		POS_RIGHT_PAR      =     0x8000,
		POS_DEP_SELF       =    0x10000,
		POS_DEP_NONE       =    0x20000,
		POS_DEP_ALL        =    0x40000,
		POS_DEP_DEEP       =    0x80000,
		POS_MARK_SET       =   0x100000,
		POS_MARK_JUMP      =   0x200000,
		POS_LOOK_DELETED   =   0x400000,
		POS_LOOK_DELAYED   =   0x800000,
		POS_TMPL_OVERRIDE  =  0x1000000,
		POS_NONE           =  0x2000000,
		POS_RELATION       =  0x4000000
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
		uint32_t relation;
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

		ContextualTest *allocateContextualTest();
		
		uint32_t rehash();
		uint32_t getHash();
		void resetStatistics();
		void markUsed(Grammar &grammar);
	};

}

#endif

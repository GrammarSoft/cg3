/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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
#ifndef c6d28b7452ec699b_CONTEXTUALTEST_H
#define c6d28b7452ec699b_CONTEXTUALTEST_H

#include "stdafx.hpp"
#include <vector>
#include <list>
#include <cstdint>

namespace CG3 {
class Grammar;
class ContextualTest;
typedef std::vector<ContextualTest*> ContextVector;
typedef std::list<ContextualTest*> ContextList;

#ifdef _MSC_VER
enum : uint64_t {
#else
enum {
#endif
	POS_CAREFUL        = (1 <<  0),
	POS_NEGATE         = (1 <<  1),
	POS_NOT            = (1 <<  2),
	POS_SCANFIRST      = (1 <<  3),
	POS_SCANALL        = (1 <<  4),
	POS_ABSOLUTE       = (1 <<  5),
	POS_SPAN_RIGHT     = (1 <<  6),
	POS_SPAN_LEFT      = (1 <<  7),
	POS_SPAN_BOTH      = (1 <<  8),
	POS_DEP_PARENT     = (1 <<  9),
	POS_DEP_SIBLING    = (1 << 10),
	POS_DEP_CHILD      = (1 << 11),
	POS_PASS_ORIGIN    = (1 << 12),
	POS_NO_PASS_ORIGIN = (1 << 13),
	POS_LEFT_PAR       = (1 << 14),
	POS_RIGHT_PAR      = (1 << 15),
	POS_SELF           = (1 << 16),
	POS_NONE           = (1 << 17),
	POS_ALL            = (1 << 18),
	POS_DEP_DEEP       = (1 << 19),
	POS_MARK_SET       = (1 << 20),
	POS_MARK_JUMP      = (1 << 21),
	POS_LOOK_DELETED   = (1 << 22),
	POS_LOOK_DELAYED   = (1 << 23),
	POS_TMPL_OVERRIDE  = (1 << 24),
	POS_UNKNOWN        = (1 << 25),
	POS_RELATION       = (1 << 26),
	POS_ATTACH_TO      = (1 << 27),
	POS_NUMERIC_BRANCH = (1 << 28),
	POS_BAG_OF_TAGS    = (1 << 29),
	POS_DEP_GLOB       = (1 << 30),
	POS_64BIT          = (1ull << 31),
	POS_LEFT           = (1ull << 32),
	POS_RIGHT          = (1ull << 33),
	POS_LEFTMOST       = (1ull << 34),
	POS_RIGHTMOST      = (1ull << 35),

	MASK_POS_DEP       = POS_DEP_PARENT | POS_DEP_SIBLING | POS_DEP_CHILD | POS_DEP_GLOB,
	MASK_POS_DEPREL    = MASK_POS_DEP | POS_RELATION,
	MASK_POS_CDEPREL   = MASK_POS_DEPREL | POS_CAREFUL,
	MASK_POS_LORR      = POS_LEFT | POS_RIGHT | POS_LEFTMOST | POS_RIGHTMOST,
	MASK_POS_SCAN      = POS_SCANFIRST | POS_SCANALL | POS_DEP_DEEP | POS_DEP_GLOB,
};

enum GSR_SPECIALS {
	GSR_ANY = 32767,
};

class ContextualTest {
public:
	bool is_used;
	int32_t offset;
	int32_t offset_sub;
	uint32_t line;
	uint32_t hash;
	uint32_t seed;
	uint64_t pos;
	uint32_t target;
	uint32_t relation;
	uint32_t barrier;
	uint32_t cbarrier;
	mutable uint32_t num_fail, num_match;
	mutable double total_time;
	ContextualTest* tmpl;
	ContextualTest* linked;

	ContextVector ors;

	ContextualTest();

	bool operator==(const ContextualTest&) const;
	bool operator!=(const ContextualTest& o) const { return !(*this == o); }
	uint32_t rehash();
	void resetStatistics();
	void markUsed(Grammar& grammar);
};

inline void copy_cntx(const ContextualTest* src, ContextualTest* trg) {
	trg->offset = src->offset;
	trg->offset_sub = src->offset_sub;
	trg->line = src->line;
	trg->hash = src->hash;
	trg->seed = src->seed;
	trg->pos = src->pos;
	trg->target = src->target;
	trg->relation = src->relation;
	trg->barrier = src->barrier;
	trg->cbarrier = src->cbarrier;
	trg->tmpl = src->tmpl;
	trg->linked = src->linked;
}
}

#endif

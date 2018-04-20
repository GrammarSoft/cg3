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

#include "ContextualTest.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"

namespace CG3 {

ContextualTest::ContextualTest()
  : is_used(false)
  , offset(0)
  , offset_sub(0)
  , line(0)
  , hash(0)
  , seed(0)
  , pos(0)
  , target(0)
  , relation(0)
  , barrier(0)
  , cbarrier(0)
  , num_fail(0)
  , num_match(0)
  , total_time(0)
  , tmpl(0)
  , linked(0)
{
	// Nothing in the actual body...
}

bool ContextualTest::operator==(const ContextualTest& other) const {
	if (hash != other.hash) {
		return false;
	}
	if (pos != other.pos) {
		return false;
	}
	if (target != other.target) {
		return false;
	}
	if (barrier != other.barrier) {
		return false;
	}
	if (cbarrier != other.cbarrier) {
		return false;
	}
	if (relation != other.relation) {
		return false;
	}
	if (offset != other.offset) {
		return false;
	}
	if (offset_sub != other.offset_sub) {
		return false;
	}
	if (linked != other.linked) {
		if (!(linked && other.linked && linked->hash == other.linked->hash)) {
			return false;
		}
	}
	if (tmpl != other.tmpl) {
		return false;
	}
	if (ors != other.ors) {
		return false;
	}
	return true;
}

uint32_t ContextualTest::rehash() {
	if (hash) {
		return hash;
	}

	hash = hash_value(pos);
	hash = hash_value(hash, target);
	hash = hash_value(hash, barrier);
	hash = hash_value(hash, cbarrier);
	hash = hash_value(hash, relation);
	hash = hash_value(hash, abs(offset));
	if (offset < 0) {
		hash = hash_value(hash, 5000);
	}
	hash = hash_value(hash, abs(offset_sub));
	if (offset_sub < 0) {
		hash = hash_value(hash, 5000);
	}
	if (linked) {
		hash = hash_value(hash, linked->rehash());
	}
	if (tmpl) {
		hash = hash_value(hash, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tmpl)));
	}
	for (auto iter : ors) {
		hash = hash_value(hash, iter->rehash());
	}

	hash += seed;

	return hash;
}

void ContextualTest::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	if (tmpl) {
		tmpl->resetStatistics();
	}
	for (auto idts : ors) {
		idts->resetStatistics();
	}
	if (linked) {
		linked->resetStatistics();
	}
}

void ContextualTest::markUsed(Grammar& grammar) {
	if (is_used) {
		return;
	}
	is_used = true;

	Set* s = 0;
	if (target) {
		s = grammar.getSet(target);
		s->markUsed(grammar);
	}
	if (barrier) {
		s = grammar.getSet(barrier);
		s->markUsed(grammar);
	}
	if (cbarrier) {
		s = grammar.getSet(cbarrier);
		s->markUsed(grammar);
	}
	if (tmpl) {
		tmpl->markUsed(grammar);
	}
	for (auto idts : ors) {
		idts->markUsed(grammar);
	}
	if (linked) {
		linked->markUsed(grammar);
	}
}
}

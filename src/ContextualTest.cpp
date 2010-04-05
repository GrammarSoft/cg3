/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#include "ContextualTest.h"
#include "Grammar.h"
#include "Strings.h"

namespace CG3 {

ContextualTest::ContextualTest() :
is_used(false),
offset(0),
line(0),
name(0),
hash(0),
pos(0),
target(0),
relation(0),
barrier(0),
cbarrier(0),
num_fail(0),
num_match(0),
total_time(0),
tmpl(0),
linked(0),
prev(0),
next(0)
{
	// Nothing in the actual body...
}

ContextualTest::~ContextualTest() {
	foreach (std::list<ContextualTest*>, ors, iter, iter_end) {
		delete *iter;
		*iter = 0;
	}
	detach();
	ors.clear();
	tmpl = 0;
	delete linked;
}

void ContextualTest::detach() {
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}
	prev = next = 0;
}

ContextualTest *ContextualTest::allocateContextualTest() {
	return new ContextualTest;
}

uint32_t ContextualTest::rehash() {
	hash = 0;
	hash = hash_sdbm_uint32_t(hash, pos);
	hash = hash_sdbm_uint32_t(hash, target);
	hash = hash_sdbm_uint32_t(hash, barrier);
	hash = hash_sdbm_uint32_t(hash, cbarrier);
	hash = hash_sdbm_uint32_t(hash, relation);
	hash = hash_sdbm_uint32_t(hash, abs(offset));
	if (offset < 0) {
		hash = hash_sdbm_uint32_t(hash, 5000);
	}
	if (linked) {
		hash = hash_sdbm_uint32_t(hash, linked->rehash());
	}
	if (tmpl) {
		hash = hash_sdbm_uint32_t(hash, tmpl->rehash());
	}
	foreach (std::list<ContextualTest*>, ors, iter, iter_end) {
		hash = hash_sdbm_uint32_t(hash, (*iter)->rehash());
	}
	return hash;
}

void ContextualTest::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	if (tmpl) {
		tmpl->resetStatistics();
	}
	foreach (std::list<ContextualTest*>, ors, idts, idts_end) {
		(*idts)->resetStatistics();
	}
	if (linked) {
		linked->resetStatistics();
	}
}

void ContextualTest::markUsed(Grammar &grammar) {
	is_used = true;

	Set *s = 0;
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
	foreach (std::list<ContextualTest*>, ors, idts, idts_end) {
		(*idts)->markUsed(grammar);
	}
	if (linked) {
		linked->markUsed(grammar);
	}
}

}

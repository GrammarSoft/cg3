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

#include "Set.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"

namespace CG3 {

std::ostream* Set::dump_hashes_out = 0;

Set::Set()
  : type(0)
  , line(0)
  , hash(0)
  , number(0)
  , num_fail(0)
  , num_match(0)
  , total_time(0)
{
	// Nothing in the actual body...
}

bool Set::empty() const {
	return (ff_tags.empty() && trie.empty() && trie_special.empty() && sets.empty());
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = static_cast<uint32_t>(rand());
	}
	size_t n = sprintf(&cbuffers[0][0], "_G_%u_%u_", line, to);
	name.reserve(n);
	name.assign(&cbuffers[0][0], &cbuffers[0][0] + n);
}

void Set::setName(const UChar* to) {
	if (to) {
		name = to;
	}
	else {
		setName();
	}
}

void Set::setName(const UString& to) {
	if (!to.empty()) {
		name = to;
	}
	else {
		setName();
	}
}

uint32_t Set::rehash() {
	uint32_t retval = 0;
	if (sets.empty()) {
		retval = hash_value(3499, retval); // Combat hash-collisions
		if (!trie.empty()) {
			retval = hash_value(trie_rehash(trie), retval);
		}
		if (!trie_special.empty()) {
			retval = hash_value(trie_rehash(trie_special), retval);
		}
	}
	else {
		retval = hash_value(2683, retval); // Combat hash-collisions
		for (uint32_t i = 0; i < sets.size(); ++i) {
			retval = hash_value(sets[i], retval);
		}
		for (uint32_t i = 0; i < set_ops.size(); ++i) {
			retval = hash_value(set_ops[i], retval);
		}
	}
	hash = retval;

	if (dump_hashes_out) {
		if (sets.empty()) {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (LIST)\n", hash, name.c_str());
		}
		else {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (SET)\n", hash, name.c_str());
		}
	}

	return retval;
}

void Set::reindex(Grammar& grammar) {
	type &= ~ST_SPECIAL;
	type &= ~ST_CHILD_UNIFY;

	type |= trie_reindex(trie);
	type |= trie_reindex(trie_special);

	for (uint32_t i = 0; i < sets.size(); ++i) {
		Set* set = grammar.sets_by_contents.find(sets[i])->second;
		set->reindex(grammar);
		if (set->type & ST_SPECIAL) {
			type |= ST_SPECIAL;
		}
		if (set->type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY)) {
			type |= ST_CHILD_UNIFY;
		}
		if (set->type & ST_MAPPING) {
			type |= ST_MAPPING;
		}
	}

	if (type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY)) {
		type |= ST_SPECIAL;
		type |= ST_CHILD_UNIFY;
	}
}

void Set::markUsed(Grammar& grammar) {
	type |= ST_USED;

	trie_markused(trie);
	trie_markused(trie_special);

	for (auto tag : ff_tags) {
		tag->markUsed();
	}

	for (uint32_t i = 0; i < sets.size(); ++i) {
		Set* set = grammar.sets_by_contents.find(sets[i])->second;
		set->markUsed(grammar);
	}
}

void Set::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}
}

/*
* Copyright (C) 2007-2025, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Set.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"

namespace CG3 {

std::ostream* Set::dump_hashes_out = nullptr;

bool Set::empty() const {
	return (ff_tags.empty() && trie.empty() && trie_special.empty() && sets.empty());
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = UI32(rand());
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

	if (type & (ST_TAG_UNIFY | ST_SET_UNIFY)) {
		if (type & ST_TAG_UNIFY) {
			retval = hash_value(5153, retval);
		}
		if (type & ST_SET_UNIFY) {
			retval = hash_value(5171, retval);
		}

		// Parse and incorporate multi-use identifier, if any
		uint32_t u = 0;
		if (name[0] == '&' && u_sscanf(name.data(), "&&%u:%*S", &u) == 1 && u != 0) {
			retval = hash_value(u, retval);
		}
		else if (name[0] == '$' && u_sscanf(name.data(), "$$%u:%*S", &u) == 1 && u != 0) {
			retval = hash_value(u, retval);
		}
	}

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
		for (auto i : sets) {
			retval = hash_value(i, retval);
		}
		for (auto i : set_ops) {
			retval = hash_value(i, retval);
		}
	}
	hash = retval;

	if (dump_hashes_out) {
		if (sets.empty()) {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (LIST)\n", hash, name.data());
		}
		else {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (SET)\n", hash, name.data());
		}
	}

	return retval;
}

void Set::reindex(Grammar& grammar) {
	type &= ~ST_SPECIAL;
	type &= ~ST_CHILD_UNIFY;

	type |= trie_reindex(trie);
	type |= trie_reindex(trie_special);

	for (auto s : sets) {
		Set* set = grammar.sets_by_contents.find(s)->second;
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

	for (auto s : sets) {
		Set* set = grammar.sets_by_contents.find(s)->second;
		set->markUsed(grammar);
	}
}

}

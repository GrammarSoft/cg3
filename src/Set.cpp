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

#include "Set.h"
#include "CompositeTag.h"
#include "Strings.h"
#include "Grammar.h"

namespace CG3 {

bool Set::dump_hashes = false;
UFILE* Set::dump_hashes_out = 0;

Set::Set() :
match_any(false),
is_special(false),
is_tag_unified(false),
is_set_unified(false),
is_child_unified(false),
is_used(false),
line(0),
hash(0),
number(0),
num_fail(0),
num_match(0),
total_time(0)
{
	// Nothing in the actual body...
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = static_cast<uint32_t>(rand());
	}
	size_t n = sprintf(&cbuffers[0][0], "_G_%u_%u_", line, to);
	name.clear();
	name.reserve(n);
	for (size_t i=0 ; i<n ; ++i) {
		name.push_back(cbuffers[0][i]);
	}
}
void Set::setName(const UChar *to) {
	if (to) {
		name = to;
	}
	else {
		setName((uint32_t)rand());
	}
}

uint32_t Set::rehash() {
	uint32_t retval = 0;
	if (sets.empty()) {
		retval = hash_sdbm_uint32_t(3499, retval); // Combat hash-collisions
		foreach (uint32Set, tags_set, iter, iter_end) {
			retval = hash_sdbm_uint32_t(*iter, retval);
		}
	}
	else {
		retval = hash_sdbm_uint32_t(2683, retval); // Combat hash-collisions
		for (uint32_t i=0;i<sets.size();i++) {
			retval = hash_sdbm_uint32_t(sets.at(i), retval);
		}
		for (uint32_t i=0;i<set_ops.size();i++) {
			retval = hash_sdbm_uint32_t(set_ops.at(i), retval);
		}
	}
	hash = retval;

	if (dump_hashes && dump_hashes_out) {
		if (sets.empty()) {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (LIST)\n", hash, name.c_str());
		}
		else {
			u_fprintf(dump_hashes_out, "DEBUG: Hash %u for set %S (SET)\n", hash, name.c_str());
		}
	}

	return retval;
}

void Set::reindex(Grammar &grammar) {
	if (is_tag_unified || is_set_unified || is_child_unified) {
		is_special = true;
		is_child_unified = true;
	}

	if (sets.empty()) {
		TagHashSet::const_iterator tomp_iter;
		for (tomp_iter = single_tags.begin() ; tomp_iter != single_tags.end() ; tomp_iter++) {
			Tag *tag = *tomp_iter;
			if (tag->is_special) {
				is_special = true;
			}
		}
		CompositeTagHashSet::const_iterator comp_iter;
		for (comp_iter = tags.begin() ; comp_iter != tags.end() ; comp_iter++) {
			CompositeTag *curcomptag = *comp_iter;
			if (curcomptag->tags.size() == 1) {
				Tag *tag = *(curcomptag->tags.begin());
				if (tag->is_special) {
					is_special = true;
				}
			}
			else {
				TagSet::const_iterator tag_iter;
				for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
					Tag *tag = *tag_iter;
					if (tag->is_special) {
						is_special = true;
					}
				}
			}
		}
	}
	else {
		for (uint32_t i=0;i<sets.size();i++) {
			Set *set = grammar.sets_by_contents.find(sets.at(i))->second;
			set->reindex(grammar);
			if (set->is_special) {
				is_special = true;
			}
			if (set->is_tag_unified || set->is_set_unified || set->is_child_unified) {
				is_child_unified = true;
			}
		}
	}
}

void Set::markUsed(Grammar &grammar) {
	is_used = true;

	if (sets.empty()) {
		TagHashSet::iterator tomp_iter;
		for (tomp_iter = single_tags.begin() ; tomp_iter != single_tags.end() ; tomp_iter++) {
			Tag *tag = *tomp_iter;
			tag->markUsed();
		}
		CompositeTagHashSet::iterator comp_iter;
		for (comp_iter = tags.begin() ; comp_iter != tags.end() ; comp_iter++) {
			CompositeTag *curcomptag = *comp_iter;
			curcomptag->markUsed();
		}
	}
	else {
		for (uint32_t i=0;i<sets.size();i++) {
			Set *set = grammar.sets_by_contents.find(sets.at(i))->second;
			set->markUsed(grammar);
		}
	}
}

void Set::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

}

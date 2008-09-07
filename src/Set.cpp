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

#include "Set.h"

using namespace CG3;

Set::Set() {
	match_any = false;
	is_special = false;
	is_unified = false;
	is_child_unified = false;
	name = 0;
	line = 0;
	hash = 0;
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	number = 0;
}

Set::~Set() {
	if (name) {
		delete[] name;
	}
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = (uint32_t)rand();
	}
	name = new UChar[32];
	memset(name, 0, sizeof(UChar)*32);
	u_sprintf(name, "_G_%u_%u_", line, to);
}
void Set::setName(const UChar *to) {
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	} else {
		setName((uint32_t)rand());
	}
}

uint32_t Set::rehash() {
	uint32_t retval = 0;
	if (sets.empty()) {
		foreach (uint32Set, tags_set, iter, iter_end) {
			retval = hash_sdbm_uint32_t(*iter, retval);
		}
	}
	else {
		for (uint32_t i=0;i<sets.size();i++) {
			retval = hash_sdbm_uint32_t(sets.at(i), retval);
		}
		for (uint32_t i=0;i<set_ops.size();i++) {
			retval = hash_sdbm_uint32_t(set_ops.at(i), retval);
		}
	}
	hash = retval;
	return retval;
}

void Set::reindex(Grammar *grammar) {
	if (is_unified || is_child_unified) {
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
			} else {
				TagSet::const_iterator tag_iter;
				for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
					Tag *tag = *tag_iter;
					if (tag->is_special) {
						is_special = true;
					}
				}
			}
		}
	} else if (!sets.empty()) {
		for (uint32_t i=0;i<sets.size();i++) {
			Set *set = grammar->sets_by_contents.find(sets.at(i))->second;
			set->reindex(grammar);
			if (set->is_special) {
				is_special = true;
			}
			if (set->is_unified || set->is_child_unified) {
				is_child_unified = true;
			}
		}
	}
}

void Set::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

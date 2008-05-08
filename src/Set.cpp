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
	has_mappings = false;
	is_special = false;
	is_unified = false;
	is_child_unified = false;
	name = 0;
	line = 0;
	hash = 0;
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

Set::~Set() {
	if (name) {
		delete[] name;
	}
}

void Set::setName(uint32_t to) {
	assert(to != 0);
	name = new UChar[32];
	memset(name, 0, sizeof(UChar)*32);
	u_sprintf(name, "_A_%u_%u_", line, to);
}
void Set::setName(const UChar *to) {
	assert(to);
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	}
}

uint32_t Set::rehash() {
	uint32_t retval = 0;
	assert(tags_set.empty() || sets.empty());
	if (sets.empty()) {
		retval = hash_sdbm_uint32_t((uint32_t)tags_set.size(), retval);
		uint32Set::iterator iter;
		for (iter = tags_set.begin() ; iter != tags_set.end() ; iter++) {
			retval = hash_sdbm_uint32_t(*iter, retval);
		}
	}
	else {
		retval = hash_sdbm_uint32_t((uint32_t)sets.size(), retval);
		for (uint32_t i=0;i<sets.size();i++) {
			retval = hash_sdbm_uint32_t(sets.at(i)->hash, retval);
		}
		retval = hash_sdbm_uint32_t((uint32_t)set_ops.size(), retval);
		for (uint32_t i=0;i<set_ops.size();i++) {
			retval = hash_sdbm_uint32_t(set_ops.at(i), retval);
		}
	}
	assert(retval != 0);
	hash = retval;
	return retval;
}

void Set::reindex(Grammar *grammar) {
	has_mappings = false;
	if (is_unified || is_child_unified) {
		is_special = true;
		is_child_unified = true;
	}

	if (sets.empty()) {
		uint32HashSet::const_iterator comp_iter;
		for (comp_iter = single_tags.begin() ; comp_iter != single_tags.end() ; comp_iter++) {
			Tag *tag = grammar->single_tags.find(*comp_iter)->second;
			if (tag->is_special) {
				is_special = true;
			}
			if (tag->type & T_MAPPING || tag->type & T_VARIABLE || tag->tag[0] == grammar->mapping_prefix) {
				has_mappings = true;
				return;
			}
		}
		for (comp_iter = tags.begin() ; comp_iter != tags.end() ; comp_iter++) {
			if (grammar->tags.find(*comp_iter) != grammar->tags.end()) {
				CompositeTag *curcomptag = grammar->tags.find(*comp_iter)->second;
				if (curcomptag->tags.size() == 1) {
					Tag *tag = grammar->single_tags.find(*(curcomptag->tags.begin()))->second;
					if (tag->is_special) {
						is_special = true;
					}
					if (tag->type & T_MAPPING || tag->type & T_VARIABLE || tag->tag[0] == grammar->mapping_prefix) {
						has_mappings = true;
						return;
					}
				} else {
					uint32Set::const_iterator tag_iter;
					for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
						Tag *tag = grammar->single_tags.find(*tag_iter)->second;
						if (tag->is_special) {
							is_special = true;
						}
						if (tag->type & T_MAPPING || tag->type & T_VARIABLE || tag->tag[0] == grammar->mapping_prefix) {
							has_mappings = true;
							return;
						}
					}
				}
			}
		}
	} else if (!sets.empty()) {
		for (uint32_t i=0;i<sets.size();i++) {
			Set *set = sets.at(i);
			set->reindex(grammar);
			if (set->is_special) {
				is_special = true;
			}
			if (set->is_unified || set->is_child_unified) {
				is_child_unified = true;
			}
			if (set->has_mappings) {
				has_mappings = true;
			}
		}
	}
}

void Set::resetStatistics() {
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

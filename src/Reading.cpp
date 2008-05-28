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

#include "Reading.h"

using namespace CG3;

Reading::Reading(Cohort *p) {
	wordform = 0;
	baseform = 0;
	hash = 0;
	parent = p;
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	current_mapping_tag = 0;
	text = 0;
}

void Reading::clear(Cohort *p) {
	wordform = 0;
	baseform = 0;
	hash = 0;
	parent = p;
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	current_mapping_tag = 0;
	if (text) {
		delete[] text;
	}
	text = 0;
	hit_by.clear();
	tags_list.clear();
	tags.clear();
	tags_plain.clear();
	tags_mapped.clear();
	tags_textual.clear();
	tags_numerical.clear();
	possible_sets.clear();
}

Reading::~Reading() {
	if (text) {
		delete[] text;
	}
}

uint32_t Reading::rehash() {
	hash = 0;
	uint32Set::const_iterator iter;
	for (iter = tags.begin() ; iter != tags.end() ; iter++) {
		hash = hash_sdbm_uint32_t(*iter, hash);
	}
	assert(hash != 0);
	return hash;
}

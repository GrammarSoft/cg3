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
	hash_plain = 0;
	parent = p;
	number = 0;
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	text = 0;
	mapping = 0;
}

void Reading::clear(Cohort *p) {
	wordform = 0;
	baseform = 0;
	hash = 0;
	hash_plain = 0;
	parent = p;
	number = 0;
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	if (text) {
		delete[] text;
	}
	text = 0;
	hit_by.clear();
	tags_list.clear();
	tags.clear();
	tags_plain.clear();
	tags_textual.clear();
	tags_numerical.clear();
	possible_sets.clear();
	mapping = 0;
}

Reading::~Reading() {
	if (text) {
		delete[] text;
	}
}

uint32_t Reading::rehash() {
	hash = 0;
	hash_plain = 0;
	uint32Set::const_iterator iter;
	for (iter = tags.begin() ; iter != tags.end() ; iter++) {
		if (!mapping || mapping->hash != *iter) {
			hash = hash_sdbm_uint32_t(*iter, hash);
		}
	}
	hash_plain = hash;
	assert(hash_plain != 0);
	if (mapping) {
		hash = hash_sdbm_uint32_t(mapping->hash, hash);
	}
	assert(hash != 0);
	return hash;
}

void Reading::duplicateFrom(Reading *r) {
	wordform = r->wordform;
	baseform = r->baseform;
	hash = r->hash;
	hash_plain = r->hash_plain;
	parent = r->parent;
	mapped = r->mapped;
	deleted = r->deleted;
	noprint = r->noprint;
	mapping = r->mapping;
	/*
	matched_target = r->matched_target;
	matched_tests = r->matched_tests;
	//*/

	hit_by.clear();
	tags_list.clear();
	tags.clear();
	tags_plain.clear();
	tags_textual.clear();
	tags_numerical.clear();
	possible_sets.clear();

	hit_by.insert(hit_by.begin(), r->hit_by.begin(), r->hit_by.end());
	tags_list.insert(tags_list.begin(), r->tags_list.begin(), r->tags_list.end());
	tags.insert(r->tags.begin(), r->tags.end());
	tags_plain.insert(r->tags_plain.begin(), r->tags_plain.end());
	tags_textual.insert(r->tags_textual.begin(), r->tags_textual.end());
	tags_numerical.insert(r->tags_numerical.begin(), r->tags_numerical.end());
	possible_sets.insert(r->possible_sets.begin(), r->possible_sets.end());
}

bool Reading::cmp_number(Reading *a, Reading *b) {
	return a->number < b->number;
}

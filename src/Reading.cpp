/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
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
	tags_plain = new uint32HashSet;
	tags_mapped = new uint32HashSet;
	tags_textual = new uint32HashSet;
	tags_numerical = new uint32HashSet;
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
	tags_plain->clear();
	tags_mapped->clear();
	tags_textual->clear();
	tags_numerical->clear();
	possible_sets.clear();
}

Reading::~Reading() {
	if (text) {
		delete[] text;
	}
	delete tags_plain;
	delete tags_mapped;
	delete tags_textual;
	delete tags_numerical;
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

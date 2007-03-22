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
#include "stdafx.h"
#include <unicode/ustring.h>
#include "Reading.h"

using namespace CG3;

Reading::Reading() {
	wordform = 0;
	baseform = 0;
	hash = 0;
	parent = 0;
	hash_tags = 0;
	hash_mapped = 0;
	hash_plain = 0;
	hash_textual = 0;
	mapped = false;
	deleted = false;
	noprint = false;
	matched_target = false;
	matched_tests = false;
	current_mapping_tag = 0;
	text = 0;
	dep_self = 0;
}

Reading::~Reading() {
	wordform = 0;
	baseform = 0;
	if (text) {
		delete text;
	}
	text = 0;
	tags.clear();
	tags_plain.clear();
	tags_mapped.clear();
	tags_textual.clear();
	tags_numerical.clear();
	tags_list.clear();
	hit_by.clear();
	dep_parents.clear();
	dep_children.clear();
	dep_siblings.clear();
}

uint32_t Reading::rehash() {
	hash = 0;
	hash_tags = 0;
	std::list<uint32_t>::const_iterator iter;
	for (iter = tags_list.begin() ; iter != tags_list.end() ; iter++) {
		if (*iter == wordform || *iter == baseform) {
			continue;
		}
		hash = hash_sdbm_uint32_t(*iter, hash);
	}
	hash_tags = hash;

	if (hash_tags == 0) {
		hash_tags = 1;
	}

	hash = hash_sdbm_uint32_t(wordform, hash);
	hash = hash_sdbm_uint32_t(baseform, hash);

	assert(hash != 0);

	std::map<uint32_t, uint32_t>::const_iterator mter;
	hash_mapped = 1;
	for (mter = tags_mapped.begin() ; mter != tags_mapped.end() ; mter++) {
		hash_mapped = hash_sdbm_uint32_t(mter->second, hash_mapped);
	}
	hash_plain = 1;
	for (mter = tags_plain.begin() ; mter != tags_plain.end() ; mter++) {
		hash_plain = hash_sdbm_uint32_t(mter->second, hash_plain);
	}
	hash_textual = 1;
	for (mter = tags_textual.begin() ; mter != tags_textual.end() ; mter++) {
		hash_textual = hash_sdbm_uint32_t(mter->second, hash_textual);
	}

	return hash;
}

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
	hash_tags = 0;
	mapped = false;
	deleted = false;
	selected = false;
	hit_by = 0;
	noprint = false;
	text = 0;
}

Reading::~Reading() {
	wordform = 0;
	baseform = 0;
	if (text) {
		delete text;
	}
	text = 0;
	tags.clear();
	tags_list.clear();
}

uint32_t Reading::rehash() {
	hash = 0;
	hash_tags = 0;
	std::list<uint32_t>::iterator iter;
	for (iter = tags_list.begin() ; iter != tags_list.end() ; iter++) {
		if (*iter == wordform || *iter == baseform) {
			continue;
		}
		hash = hash_sdbm_uint32_t(*iter, hash);
	}
	hash_tags = hash;

	hash = hash_sdbm_uint32_t(wordform, hash);
	hash = hash_sdbm_uint32_t(baseform, hash);

	assert(hash != 0);
	return hash;
}

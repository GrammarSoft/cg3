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
#include "Grammar.h"
#include "Set.h"
#include "CompositeTag.h"
#include "Strings.h"

using namespace CG3;

Set::Set() {
	name = 0;
	line = 0;
	hash = 0;
	used = false;
}

Set::~Set() {
	if (name) {
		delete name;
	}
	tags_map.clear();
	tags.clear();
	single_tags.clear();
	sets.clear();
	set_ops.clear();
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = (uint32_t)rand();
	}
	name = new UChar[32];
	memset(name, 0, 32);
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

void Set::addCompositeTag(uint32_t tag) {
	tags_map[tag] = tag;
	tags[tag] = tag;
}

void Set::addTag(uint32_t tag) {
	tags_map[tag] = tag;
	single_tags[tag] = tag;
}

uint32_t Set::rehash() {
	uint32_t retval = 0;
	assert(tags_map.empty() || sets.empty());
	if (sets.empty()) {
		std::map<uint32_t, uint32_t>::iterator iter;
		for (iter = tags_map.begin() ; iter != tags_map.end() ; iter++) {
			retval = hash_sdbm_uint32_t(iter->second, retval);
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
	assert(retval != 0);
	hash = retval;
	return retval;
}

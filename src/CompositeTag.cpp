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
#include "CompositeTag.h"

using namespace CG3;

CompositeTag::CompositeTag() {
	hash = 0;
	tags_set.clear();
	tags.clear();
}

CompositeTag::~CompositeTag() {
	tags_set.clear();
	tags.clear();
}

void CompositeTag::addTag(uint32_t tag) {
	tags.insert(tag);
	tags_set.insert(tag);
}

uint32_t CompositeTag::rehash() {
	uint32_t retval = 0;
	uint32Set::iterator iter;
	for (iter = tags_set.begin() ; iter != tags_set.end() ; iter++) {
		retval = hash_sdbm_uint32_t(*iter, retval);
	}
	hash = retval;
	return retval;
}

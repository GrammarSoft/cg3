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
#include "CompositeTag.h"

namespace CG3 {

	CompositeTag::CompositeTag() {
		hash = 0;
	}
	
	CompositeTag::~CompositeTag() {
		std::map<uint32_t, Tag*>::iterator iter_map;
		for (iter_map = tags_map.begin() ; iter_map != tags_map.end() ; iter_map++) {
			if (iter_map->second) {
				delete iter_map->second;
			}
		}
		tags_map.clear();

		tags.clear();
	}

	void CompositeTag::addTag(Tag *tag) {
		tags[hash_sdbm_uchar(tag->raw)] = tag;
		tags_map[hash_sdbm_uchar(tag->raw)] = tag;
	}
	void CompositeTag::removeTag(Tag *tag) {
		tags.erase(hash_sdbm_uchar(tag->raw));
		tags_map.erase(hash_sdbm_uchar(tag->raw));
		destroyTag(tags[hash_sdbm_uchar(tag->raw)]);
	}

	Tag *CompositeTag::allocateTag(const UChar *tag) {
		Tag *fresh = new Tag;
		fresh->parseTag(tag);
		return fresh;
	}
	Tag *CompositeTag::duplicateTag(Tag *tag) {
		Tag *fresh = new Tag;
		fresh->parseTag(tag->raw);
		fresh->denied = tag->denied;
		fresh->negative = tag->negative;
		return fresh;
	}
	void CompositeTag::destroyTag(Tag *tag) {
		delete tag;
	}

	uint32_t CompositeTag::rehash() {
		uint32_t retval = 0;
		std::map<uint32_t, Tag*>::iterator iter;
		for (iter = tags_map.begin() ; iter != tags_map.end() ; iter++) {
			retval = hash_sdbm_uchar(iter->second->raw, retval);
		}
		hash = retval;
		return retval;
	}
	const uint32_t CompositeTag::getHash() {
		return hash;
	}
}

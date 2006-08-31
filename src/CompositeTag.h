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
#ifndef __COMPOSITETAG_H
#define __COMPOSITETAG_H

#include <unicode/ustring.h>
#include "Tag.h"

namespace CG3 {

	class CompositeTag {
	public:
		uint32_t hash;
		std::map<uint32_t, Tag*> tags_map;
		stdext::hash_map<uint32_t, Tag*> tags;

		CompositeTag() {
			hash = 0;
		}
		
		~CompositeTag() {
			std::map<uint32_t, Tag*>::iterator iter_map;
			for (iter_map = tags_map.begin() ; iter_map != tags_map.end() ; iter_map++) {
				delete (*iter_map).second;
			}
			tags_map.clear();

			tags.clear();
		}

		void addTag(Tag *tag) {
			tags[hash_sdbm_uchar(tag->raw)] = tag;
			tags_map[hash_sdbm_uchar(tag->raw)] = tag;
		}
		void removeTag(Tag *tag) {
			tags.erase(hash_sdbm_uchar(tag->raw));
			tags_map.erase(hash_sdbm_uchar(tag->raw));
			destroyTag(tags[hash_sdbm_uchar(tag->raw)]);
		}

		Tag *allocateTag(const UChar *tag) {
			Tag *fresh = new Tag;
			fresh->parseTag(tag);
			return fresh;
		}
		void destroyTag(Tag *tag) {
			delete tag;
		}

		uint32_t rehash() {
			uint32_t retval = 0;
			std::map<uint32_t, Tag*>::iterator iter;
			for (iter = tags_map.begin() ; iter != tags_map.end() ; iter++) {
				retval = hash_sdbm_uchar(iter->second->raw, retval);
			}
			hash = retval;
			return retval;
		}
		const uint32_t getHash() {
			return hash;
		}
	};

}

#endif

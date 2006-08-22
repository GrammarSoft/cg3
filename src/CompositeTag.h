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
		unsigned int num_tags;
		unsigned long hash;
		std::map<unsigned long, Tag*> _tags_map;
		stdext::hash_map<unsigned long, Tag*> tags;

		CompositeTag() {
			hash = 0;
			num_tags = 0;
		}

		void addTag(Tag *tag) {
			tags[hash_sdbm_uchar(tag->raw)] = tag;
			_tags_map[hash_sdbm_uchar(tag->raw)] = tag;
		}
		void removeTag(Tag *tag) {
			tags.erase(hash_sdbm_uchar(tag->raw));
			_tags_map.erase(hash_sdbm_uchar(tag->raw));
			destroyTag(tags[hash_sdbm_uchar(tag->raw)]);
		}

		Tag *allocateTag(const UChar *tag) {
			Tag *fresh = new Tag;
			fresh->parseTag(tag);
			num_tags++;
			return fresh;
		}
		void destroyTag(Tag *tag) {
			delete tag;
		}

		unsigned long rehash() {
			unsigned long retval = 0;
			std::map<unsigned long, Tag*>::iterator iter;
			for (iter = _tags_map.begin() ; iter != _tags_map.end() ; iter++) {
				retval = hash_sdbm_uchar(iter->second->raw, retval);
			}
			hash = retval;
			return retval;
		}
	};

}

#endif

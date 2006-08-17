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
		std::vector<Tag*> tags;

		CompositeTag() {
			num_tags = 0;
		}

		void addTag(Tag *tag) {
			tags.push_back(tag);
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
	};

}

#endif

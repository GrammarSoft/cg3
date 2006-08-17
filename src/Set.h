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
#ifndef __SET_H
#define __SET_H

#include <unicode/ustring.h>
#include "CompositeTag.h"

namespace CG3 {

	class Set {
	public:
		UChar *name;
		unsigned int num_tags;
		unsigned int line;
		stdext::hash_map<UChar*, unsigned long> index_certain;
		stdext::hash_map<UChar*, unsigned long> index_possible;
		stdext::hash_map<UChar*, unsigned long> index_impossible;
		std::vector<CompositeTag*> tags;

		Set() {
			name = 0;
			num_tags = 0;
			line = 0;
		}

		void setName(const UChar *to) {
			name = new UChar[u_strlen(to)+1];
			u_strcpy(name, to);
		}
		void setLine(unsigned int to) {
			line = to;
		}

		void addCompositeTag(CompositeTag *tag) {
			tags.push_back(tag);
			num_tags++;
		}

		CompositeTag *allocateCompositeTag() {
			return new CompositeTag;
		}
		void destroyCompositeTag(CompositeTag *tag) {
			delete tag;
		}
	};

}

#endif

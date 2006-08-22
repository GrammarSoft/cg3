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

#include <set>
#include <unicode/ustring.h>
#include "CompositeTag.h"
#include "Strings.h"

namespace CG3 {

	class Set {
	public:
		UChar *name;
		unsigned int num_tags;
		unsigned int line;
		stdext::hash_map<UChar*, unsigned long> index_certain;
		stdext::hash_map<UChar*, unsigned long> index_possible;
		stdext::hash_map<UChar*, unsigned long> index_impossible;
		stdext::hash_map<unsigned long, CompositeTag*> tags;

		Set() {
			name = 0;
			num_tags = 0;
			line = 0;
		}

		void setName(unsigned long to) {
			if (!to) {
				to = (unsigned long)rand();
			}
			name = new UChar[24];
			memset(name, 0, 24);
			u_sprintf(name, "_G_%u_", to);
		}
		void setName(const UChar *to) {
			if (to) {
				name = new UChar[u_strlen(to)+1];
				u_strcpy(name, to);
			} else {
				setName((unsigned long)rand());
			}
		}
		const UChar *getName() {
			return name;
		}
		void setLine(unsigned int to) {
			line = to;
		}
		unsigned int getLine() {
			return line;
		}

		void addCompositeTag(CompositeTag *tag) {
			tags[tag->rehash()] = tag;
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

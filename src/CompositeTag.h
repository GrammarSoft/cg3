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
		std::map<uint32_t, uint32_t> tags_map;
		stdext::hash_map<uint32_t, uint32_t> tags;

		CompositeTag();
		~CompositeTag();

		void addTag(uint32_t tag);
		void removeTag(uint32_t tag);

		uint32_t rehash();
	};

}

#endif

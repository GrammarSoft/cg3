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
#include "Grammar.h"
#include "CompositeTag.h"
#include "Strings.h"

namespace CG3 {

	class Set {
	public:
		UChar *name;
		uint32_t line;
		uint32_t hash;
		bool used;
		bool composite;

		std::map<uint32_t, uint32_t> tags_map;
		stdext::hash_map<uint32_t, uint32_t> tags;

		std::vector<uint32_t> set_ops;
		std::vector<uint32_t> sets;

		Set();
		~Set();

		void setName(uint32_t to);
		void setName(const UChar *to);
		const UChar *getName();

		void setLine(uint32_t to);
		uint32_t getLine();

		void addCompositeTag(uint32_t tag);

		uint32_t rehash();
	};

}

#endif

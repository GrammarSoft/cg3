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
#ifndef __TAG_H
#define __TAG_H

#include <unicode/uregex.h>
#include <unicode/ustdio.h>
#include <unicode/ustring.h>

namespace CG3 {

	enum C_OPS {
		OP_NOP,
		OP_EQUALS,
		OP_LESSTHAN,
		OP_GREATERTHAN,
		NUM_OPS
	};

	enum TAG_FEATURES {
		F_NEGATIVE =  1,
		F_FAILFAST =  2,
		F_CASE_INSENSITIVE = 4,
		F_REGEXP   =  8
	};

	enum TAG_TYPE {
		T_ANY       =  1,
		T_NUMERICAL =  2,
		T_MAPPING   =  4,
		T_VARIABLE  =  8,
		T_META      = 16,
		T_WORDFORM = 32,
		T_BASEFORM = 64,
		T_TEXTUAL = 128
	};

	class Tag {
	public:
		uint8_t features;
		uint8_t type;
		mutable URegularExpression *regexp;

		uint32_t comparison_hash;
		UChar *comparison_key;
		C_OPS comparison_op;
		int comparison_val;
		UChar *tag;
		uint32_t hash;

		Tag();
		~Tag();
		void parseTag(const UChar *to);
		void parseTagRaw(const UChar *to);
		void duplicateTag(const Tag *from);

		uint32_t rehash();
	};

}

#endif

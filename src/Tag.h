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

	class Tag {
	public:
		bool negative;
		bool failfast;
		bool case_insensitive;
		bool regexp;
		bool wildcard;
		bool wordform;
		bool baseform;
		bool numerical;
		bool any;
		bool mapping;
		UChar *comparison_key;
		C_OPS comparison_op;
		int comparison_val;
		UChar *tag;
		UChar *raw;

		Tag();
		~Tag();
		void parseTag(const UChar *to);

		void print(UFILE *out);
	};

}

#endif

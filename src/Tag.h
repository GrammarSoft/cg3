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

#include "stdafx.h"
#include "Strings.h"

namespace CG3 {

	enum C_OPS {
		OP_NOP,
		OP_EQUALS,
		OP_LESSTHAN,
		OP_GREATERTHAN,
		NUM_OPS
	};

	enum TAG_TYPE {
		T_ANY       =  1,
		T_NUMERICAL =  2,
		T_MAPPING   =  4,
		T_VARIABLE  =  8,
		T_META      = 16,
		T_WORDFORM = 32,
		T_BASEFORM = 64,
		T_TEXTUAL = 128,
		T_DEPENDENCY = 256,
		T_NEGATIVE =  512,
		T_FAILFAST =  1024,
		T_CASE_INSENSITIVE = 2048,
		T_REGEXP   =  4096
	};

	class Tag {
	public:
		uint16_t type;
		mutable URegularExpression *regexp;

		bool in_grammar;
		uint32_t comparison_hash;
		UChar *comparison_key;
		C_OPS comparison_op;
		int comparison_val;
		uint32_t dep_self, dep_parent;
		UChar *tag;
		uint32_t hash;

		Tag();
		~Tag();
		void parseTag(const UChar *to, UFILE *ux_stderr);
		void parseTagRaw(const UChar *to);

		uint32_t rehash();
	};

}

#endif

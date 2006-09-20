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
#ifndef __GRAMMARAPPLICATOR_H
#define __GRAMMARAPPLICATOR_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
 
namespace CG3 {
	class GrammarApplicator {
	public:
		uint32_t num_windows;

		const Grammar *grammar;

		stdext::hash_map<uint32_t, Tag*> single_tags;

		stdext::hash_map<uint32_t, uint32_t> index_reading_tags_yes;
		stdext::hash_map<uint32_t, uint32_t> index_reading_yes;
		stdext::hash_map<uint32_t, uint32_t> index_reading_tags_no;
		stdext::hash_map<uint32_t, uint32_t> index_reading_no;
	
		GrammarApplicator();
		~GrammarApplicator();

		void setGrammar(const Grammar *res);
		uint32_t addTag(const UChar *tag);

		int runGrammarOnText(UFILE *input, UFILE *output);

		bool doesTagMatchSet(uint32_t tag, uint32_t set);
	};
}

#endif

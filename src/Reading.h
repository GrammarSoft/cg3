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
#ifndef __READING_H
#define __READING_H

#include "stdafx.h"
#include "Recycler.h"

namespace CG3 {

	class Reading {
	public:
		Cohort *parent;

		uint32_t wordform;
		uint32_t baseform;
		uint32_t hash, hash_tags, hash_mapped, hash_plain, hash_textual;
		bool mapped;
		bool deleted;
		uint32Vector hit_by;
		bool noprint;
		uint32List tags_list;
		uint32HashSet tags;
		uint32Set *tags_plain;
		uint32Set *tags_mapped;
		uint32Set *tags_textual;
		uint32Set *tags_numerical;

		UChar *text;

		bool matched_target;
		bool matched_tests;
		uint32_t current_mapping_tag;

		Reading(Cohort *p);
		~Reading();

		uint32_t rehash();
	};

}

#endif

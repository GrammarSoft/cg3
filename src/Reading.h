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

namespace CG3 {

	class Reading {
	public:
		Cohort *parent;

		uint32_t wordform;
		uint32_t baseform;
		uint32_t hash;
		bool mapped;
		bool deleted;
		uint32Vector hit_by;
		bool noprint;
		uint32List tags_list;
		uint32Set tags;
		// ToDo: Get these back to normal ones; no need to recycle elements of a recycled object
		uint32HashSet *tags_plain;
		uint32HashSet *tags_mapped;
		uint32HashSet *tags_textual;
		uint32HashSet *tags_numerical;

		uint32HashSet possible_sets;

		UChar *text;

		bool matched_target;
		bool matched_tests;
		uint32_t current_mapping_tag;

		Reading(Cohort *p);
		~Reading();
		void clear(Cohort *p);

		uint32_t rehash();
	};

}

#endif

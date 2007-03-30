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
#ifndef __COHORT_H
#define __COHORT_H

#include "stdafx.h"
#include <unicode/ustdio.h>
#include <unicode/ustring.h>
#include "Window.h"
#include "Reading.h"

namespace CG3 {

	class Cohort {
	public:
		uint32_t global_number;
		uint32_t local_number;
		uint32_t wordform;
		SingleWindow *parent;
		std::list<Reading*> readings;
		std::list<Reading*> deleted;
		UChar *text;

		bool dep_done;
		uint32_t dep_self;
		uint32_t dep_parent;
		std::set<uint32_t> dep_children;
		std::set<uint32_t> dep_siblings;

		bool is_related;
		std::multimap<uint32_t, uint32_t> relations;

		Cohort(SingleWindow *p);
		~Cohort();

		void addSibling(uint32_t sibling);
		void remSibling(uint32_t sibling);
		void addChild(uint32_t child);
		void remChild(uint32_t child);
		void appendReading(Reading *read);
		Reading *allocateAppendReading();
	};

}

#endif

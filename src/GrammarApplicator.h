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
#include "Window.h"
#include "SingleWindow.h"
#include "Index.h"

namespace CG3 {
	class GrammarApplicator {
	public:
		static const uint32_t RV_NOTHING = 1;
		static const uint32_t RV_SOMETHING = 2;
		static const uint32_t RV_DELIMITED = 4;

		bool fast;
		bool apply_mappings;
		bool apply_corrections;
		bool trace;
		bool single_run;
		bool statistics;

		uint32_t num_windows;
		uint32_t cache_hits, cache_miss, match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t last_mapping_tag;

		const Grammar *grammar;

		stdext::hash_map<uint32_t, uint32_t> variables;
		stdext::hash_map<uint32_t, uint32_t> metas;

		stdext::hash_map<uint32_t, Tag*> single_tags;
		stdext::hash_map<uint32_t, Index*> index_tags_regexp;

		stdext::hash_map<uint32_t, Index*> index_reading_yes;
		stdext::hash_map<uint32_t, Index*> index_reading_no;
		stdext::hash_map<uint32_t, Index*> index_reading_tags_yes;
		stdext::hash_map<uint32_t, Index*> index_reading_tags_no;
		stdext::hash_map<uint32_t, Index*> index_reading_plain_yes;
		stdext::hash_map<uint32_t, Index*> index_reading_plain_no;
	
		GrammarApplicator();
		~GrammarApplicator();

		void setGrammar(const Grammar *res);
		uint32_t addTag(const UChar *tag);

		int runGrammarOnText(UFILE *input, UFILE *output);
		int runGrammarOnWindow(Window *window);
		uint32_t runRulesOnWindow(Window *window, const std::vector<Rule*> *rules, const uint32_t start, const uint32_t end);

		bool runContextualTest(const Window *window, const SingleWindow *sWindow, const uint32_t position, const ContextualTest *test);

		bool doesTagMatchSet(const uint32_t tag, const uint32_t set);
		bool doesSetMatchReading(const Reading *reading, const uint32_t set, bool bypass_index = false);
		bool doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set);
		bool doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set);

		void printSingleWindow(SingleWindow *window, UFILE *output);

	private:
		inline bool __index_matches(const stdext::hash_map<uint32_t, Index*> *me, const uint32_t value, const uint32_t set);
		inline void reflowReading(Reading *reading);
		inline void reflowSingleWindow(SingleWindow *swindow);
	};
}

#endif

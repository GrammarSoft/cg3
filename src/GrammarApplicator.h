/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef __GRAMMARAPPLICATOR_H
#define __GRAMMARAPPLICATOR_H

#include "stdafx.h"

namespace CG3 {
	class GrammarApplicator {
	public:
		bool always_span;
		bool apply_mappings;
		bool apply_corrections;
		bool no_before_sections;
		bool no_sections;
		bool no_after_sections;
		bool trace;
		bool trace_name_only;
		bool trace_no_removed;
		bool single_run;
		bool allow_magic_readings;
		bool no_pass_origin;
		bool unsafe;

		bool dep_has_spanned;
		bool dep_delimit;
		bool dep_humanize;
		bool dep_original;
		bool dep_block_loops;

		uint32_t num_windows;
		uint32_t soft_limit;
		uint32_t hard_limit;
		std::vector<uint32_t> sections;
		uint32_t verbosity_level;

		GrammarApplicator(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err);
		virtual ~GrammarApplicator();

		void enableStatistics();
		void disableStatistics();

		void setGrammar(const Grammar *res);
		void index();

		virtual int runGrammarOnText(UFILE *input, UFILE *output);

		bool has_dep;
		uint32_t dep_highest_seen;
		Window *gWindow;
		void reflowDependencyWindow(uint32_t max = 0);

	protected:
		void printReading(Reading *reading, UFILE *output);
		void printSingleWindow(SingleWindow *window, UFILE *output);

		UFILE *ux_stdin;
		UFILE *ux_stdout;
		UFILE *ux_stderr;

		uint32_t numLines;
		uint32_t numWindows;
		uint32_t numCohorts;
		uint32_t numReadings;

		bool did_index;

		uint32_t numsections;
		typedef std::map<int32_t,uint32Set*> RSType;
		RSType runsections;

		static const uint32_t RV_NOTHING = 1;
		static const uint32_t RV_SOMETHING = 2;
		static const uint32_t RV_DELIMITED = 4;

		uint32_t match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t par_left_tag, par_right_tag;
		uint32_t par_left_pos, par_right_pos;
		bool has_enclosures;
		bool did_final_enclosure;

		const Grammar *grammar;

		std::map<uint32_t,UnicodeString> regexgrps;
		uint32HashMap variables;
		uint32HashMap metas;
		Cohort *mark;

		bool unif_mode;
		uint32HashMap unif_tags;
		uint32_t unif_last_wordform;
		uint32_t unif_last_baseform;
		uint32_t unif_last_textual;

		stdext::hash_map<uint32_t, Tag*> single_tags;

		uint32HashSet index_regexp_yes;
		uint32HashSet index_regexp_no;
		uint32HashSet index_readingSet_yes;
		uint32HashSet index_readingSet_no;
		void resetIndexes();
	
		Tag *addTag(const UChar *tag);

		void updateRuleToCohorts(Cohort *c, uint32_t rsit);
		void indexSingleWindow(SingleWindow *current);
		int runGrammarOnWindow();
		int runGrammarOnSingleWindow(SingleWindow *current);
		void updateValidRules(uint32Set *rules, uint32Set *intersects, uint32_t hash, Reading *reading);
		uint32_t runRulesOnWindow(SingleWindow *current, uint32Set *rules);

		Cohort *runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runContextualTest(SingleWindow *sWindow, size_t position, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runDependencyTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0, const Cohort *self = 0);
		Cohort *runParenthesisTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);

		bool doesTagMatchSet(const uint32_t tag, const Set *set);
		bool doesTagMatchReading(const Reading *reading, const Tag *tag);
		bool doesSetMatchReading_tags(const Reading *reading, const Set *theset);
		bool doesSetMatchReading(Reading *reading, const uint32_t set, bool bypass_index = false);
		bool doesSetMatchCohortNormal(Cohort *cohort, const uint32_t set, uint32_t options = 0);
		bool doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set, uint32_t options = 0);

		bool statistics;
		ticks gtimer;

		void reflowReading(Reading *reading);
		void addTagToReading(Reading *reading, uint32_t tag, bool rehash = true);
		void delTagFromReading(Reading *reading, uint32_t tag);
		void splitMappings(TagList mappings, Cohort *cohort, Reading *reading, bool mapped = false);
		void mergeMappings(Cohort *cohort);
		bool wouldParentChildLoop(Cohort *parent, Cohort *child);
		bool attachParentChild(Cohort *parent, Cohort *child, bool allowloop = false);

		Reading *initEmptyCohort(Cohort *cohort);
	};
}

#endif

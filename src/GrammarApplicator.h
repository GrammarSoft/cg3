/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATOR_H
#define c6d28b7452ec699b_GRAMMARAPPLICATOR_H

#include "stdafx.h"
#include "Tag.h"
#include "CohortIterator.h"
#include "sorted_vector.hpp"

namespace CG3 {
	class Window;
	class Grammar;
	class Reading;
	class SingleWindow;
	class Cohort;
	class ContextualTest;
	class Set;

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
		bool trace_encl;
		bool allow_magic_readings;
		bool no_pass_origin;
		bool unsafe;
		bool ordered;
		bool show_end_tags;
		bool unicode_tags;

		bool dep_has_spanned;
		bool dep_delimit;
		bool dep_humanize;
		bool dep_original;
		bool dep_block_loops;
		bool dep_block_crossing;

		uint32_t num_windows;
		uint32_t soft_limit;
		uint32_t hard_limit;
		uint32Vector sections;
		uint32SortedVector valid_rules;
		uint32_t verbosity_level;
		uint32_t debug_level;
		uint32_t section_max_count;

		GrammarApplicator(UFILE *ux_err);
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

		const Grammar *grammar;

	protected:
		void printReading(const Reading *reading, UFILE *output);
		void printCohort(Cohort *cohort, UFILE *output);
		virtual void printSingleWindow(SingleWindow *window, UFILE *output);

		UFILE *ux_stderr;

		uint32_t numLines;
		uint32_t numWindows;
		uint32_t numCohorts;
		uint32_t numReadings;

		bool did_index;
		uint32Set dep_deep_seen;

		uint32_t numsections;
		typedef std::map<int32_t,uint32SortedVector> RSType;
		RSType runsections;

		uint32Vector ci_depths;
		stdext::hash_map<uint32_t,CohortIterator> cohortIterators;
		stdext::hash_map<uint32_t,TopologyLeftIter> topologyLeftIters;
		stdext::hash_map<uint32_t,TopologyRightIter> topologyRightIters;
		stdext::hash_map<uint32_t,DepParentIter> depParentIters;

		static const uint32_t RV_NOTHING = 1;
		static const uint32_t RV_SOMETHING = 2;
		static const uint32_t RV_DELIMITED = 4;

		uint32_t match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t par_left_tag, par_right_tag;
		uint32_t par_left_pos, par_right_pos;
		bool has_enclosures;
		bool did_final_enclosure;

		std::vector<UnicodeString> regexgrps;
		uint32HashMap variables;
		uint32HashMap metas;
		Cohort *target;
		Cohort *mark;
		Cohort *attach_to;

		uint32HashMap unif_tags;
		uint32_t unif_last_wordform;
		uint32_t unif_last_baseform;
		uint32_t unif_last_textual;
		uint32Set unif_sets;
		bool unif_sets_firstrun;

		Taguint32HashMap single_tags;

		uint32HashSet index_regexp_yes;
		uint32HashSet index_regexp_no;
		uint32HashSet index_icase_yes;
		uint32HashSet index_icase_no;
		uint32HashSet index_readingSet_yes;
		uint32HashSet index_readingSet_no;
		uint32HashSet index_ruleCohort_no;
		void resetIndexes();
	
		Tag *addTag(const UChar *tag);
		Tag *addTag(const UString& txt);
		Tag *makeBaseFromWord(uint32_t tag);
		Tag *makeBaseFromWord(Tag *tag);

		bool updateRuleToCohorts(Cohort& c, const uint32_t& rsit);
		void indexSingleWindow(SingleWindow& current);
		int runGrammarOnWindow();
		int runGrammarOnSingleWindow(SingleWindow& current);
		void updateValidRules(const uint32SortedVector& rules, uint32Vector& intersects, const uint32_t& hash, Reading& reading);
		uint32_t runRulesOnSingleWindow(SingleWindow& current, uint32SortedVector& rules);

		Cohort *runSingleTest(Cohort *cohort, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runContextualTest(SingleWindow *sWindow, size_t position, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runDependencyTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0, const Cohort *self = 0);
		Cohort *runParenthesisTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runRelationTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);

		bool doesTagMatchReading(const Reading& reading, const Tag& tag, bool unif_mode = false);
		bool doesSetMatchReading_tags(const Reading& reading, const Set& theset, bool unif_mode = false);
		bool doesSetMatchReading(Reading& reading, const uint32_t set, bool bypass_index = false, bool unif_mode = false);
		bool doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, uint32_t options = 0);
		bool doesSetMatchCohortCareful(const Cohort& cohort, const uint32_t set, uint32_t options = 0);

		bool statistics;
		ticks gtimer;

		Cohort *delimitAt(SingleWindow& current, Cohort *cohort);
		void reflowReading(Reading& reading);
		uint32_t addTagToReading(Reading& reading, uint32_t tag, bool rehash = true);
		void delTagFromReading(Reading& reading, uint32_t tag);
		TagList getTagList(const Set& theSet, bool unif_mode = false) const;
		void splitMappings(TagList mappings, Cohort& cohort, Reading& reading, bool mapped = false);
		void mergeMappings(Cohort& cohort);
		bool isChildOf(const Cohort *child, const Cohort *parent);
		bool wouldParentChildLoop(const Cohort *parent, const Cohort *child);
		bool wouldParentChildCross(const Cohort *parent, const Cohort *child);
		bool attachParentChild(Cohort& parent, Cohort& child, bool allowloop = false, bool allowcrossing = false);

		Reading *initEmptyCohort(Cohort& cohort);
	};
}

#endif

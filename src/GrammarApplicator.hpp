/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "stdafx.hpp"
#include "Tag.hpp"
#include "TagTrie.hpp"
#include "CohortIterator.hpp"
#include "Rule.hpp"
#include "interval_vector.hpp"
#include "flat_unordered_set.hpp"
#include "scoped_stack.hpp"
#include <deque>

class Process;

namespace CG3 {
class Window;
class Grammar;
class Reading;
class SingleWindow;
class Cohort;
class ContextualTest;
class Set;
class Rule;

typedef std::vector<UnicodeString> regexgrps_t;

struct tmpl_context_t {
	Cohort* min = 0;
	Cohort* max = 0;
	std::vector<const ContextualTest*> linked;
	bool in_template = false;

	void clear() {
		min = 0;
		max = 0;
		linked.clear();
		in_template = false;
	}
};

struct dSMC_Context {
	const ContextualTest* test;
	Cohort** deep;
	Cohort* origin;
	uint64_t options;
	bool did_test;
	bool matched_target;
	bool matched_tests;
	bool in_barrier;
};

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
	bool unique_tags;
	bool dry_run;
	bool owns_grammar;
	bool input_eof;
	bool seen_barrier;
	bool is_conv;
	bool split_mappings;

	bool dep_has_spanned;
	uint32_t dep_delimit;
	bool dep_original;
	bool dep_block_loops;
	bool dep_block_crossing;

	uint32_t num_windows;
	uint32_t soft_limit;
	uint32_t hard_limit;
	uint32Vector sections;
	uint32IntervalVector valid_rules;
	uint32IntervalVector trace_rules;
	uint32FlatHashMap variables;
	uint32_t verbosity_level;
	uint32_t debug_level;
	uint32_t section_max_count;

	GrammarApplicator(std::ostream& ux_err);
	virtual ~GrammarApplicator();

	void enableStatistics();
	void disableStatistics();

	void setGrammar(Grammar* res);
	void index();

	virtual void runGrammarOnText(std::istream& input, std::ostream& output);

	bool has_dep;
	uint32_t dep_highest_seen;
	std::unique_ptr<Window> gWindow;
	void reflowDependencyWindow(uint32_t max = 0);

	bool has_relations;
	void reflowRelationWindow(uint32_t max = 0);

	Grammar* grammar;

	// Moved these public to help the library API
	Tag* addTag(Tag* tag);
	Tag* addTag(const UChar* tag, bool vstr = false);
	Tag* addTag(const UString& txt, bool vstr = false);
	void initEmptySingleWindow(SingleWindow* cSWindow);
	uint32_t addTagToReading(Reading& reading, uint32_t tag, bool rehash = true);
	uint32_t addTagToReading(Reading& reading, Tag* tag, bool rehash = true);
	void runGrammarOnWindow();

	typedef std::map<Reading*, TagList> all_mappings_t;
	void splitMappings(TagList& mappings, Cohort& cohort, Reading& reading, bool mapped = false);
	void splitAllMappings(all_mappings_t& all_mappings, Cohort& cohort, bool mapped = false);
	Taguint32HashMap single_tags;

	std::ostream* ux_stderr;
	UChar* filebase;
	void error(const char* str, const UChar* p);
	void error(const char* str, const char* s, const UChar* p);
	void error(const char* str, const UChar* s, const UChar* p);
	void error(const char* str, const char* s, const UChar* S, const UChar* p);
	Grammar* get_grammar() { return grammar; }

protected:
	void printTrace(std::ostream& output, uint32_t hit_by);
	void printReading(const Reading* reading, std::ostream& output, size_t sub = 1);
	void printCohort(Cohort* cohort, std::ostream& output);
	virtual void printSingleWindow(SingleWindow* window, std::ostream& output);

	void pipeOutReading(const Reading* reading, std::ostream& output);
	void pipeOutCohort(const Cohort* cohort, std::ostream& output);
	void pipeOutSingleWindow(const SingleWindow& window, Process& output);

	void pipeInReading(Reading* reading, Process& input, bool force = false);
	void pipeInCohort(Cohort* cohort, Process& input);
	void pipeInSingleWindow(SingleWindow& window, Process& input);

	UString span_pattern_latin;
	UString span_pattern_utf;

	uint32_t numLines;
	uint32_t numWindows;
	uint32_t numCohorts;
	uint32_t numReadings;

	bool did_index;
	sorted_vector<std::pair<uint32_t, uint32_t>> dep_deep_seen;

	uint32_t numsections;
	typedef std::map<int32_t, uint32IntervalVector> RSType;
	RSType runsections;

	typedef std::map<uint32_t, Process> externals_t;
	externals_t externals;

	uint32Vector ci_depths;
	std::map<uint32_t, CohortIterator> cohortIterators;
	std::map<uint32_t, TopologyLeftIter> topologyLeftIters;
	std::map<uint32_t, TopologyRightIter> topologyRightIters;
	std::map<uint32_t, DepParentIter> depParentIters;
	std::map<uint32_t, DepDescendentIter> depDescendentIters;
	std::map<uint32_t, DepAncestorIter> depAncestorIters;

	uint32_t match_single, match_comp, match_sub;
	uint32_t begintag, endtag, substtag;
	Tag *tag_begin, *tag_end, *tag_subst;
	uint32_t par_left_tag, par_right_tag;
	uint32_t par_left_pos, par_right_pos;
	bool did_final_enclosure;

	tmpl_context_t tmpl_cntx;

	std::vector<regexgrps_t> regexgrps_store;
	std::pair<uint8_t, regexgrps_t*> regexgrps;
	bc::flat_map<uint32_t, uint8_t> regexgrps_z;
	bc::flat_map<uint32_t, regexgrps_t*> regexgrps_c;
	uint32_t same_basic;
	Cohort* target;
	Cohort* mark;
	Cohort* attach_to;
	Rule* current_rule;

	typedef bc::flat_map<uint32_t, Reading*> readings_plain_t;
	readings_plain_t readings_plain;

	typedef bc::flat_map<uint32_t, const void*> unif_tags_t;
	bc::flat_map<uint32_t, unif_tags_t*> unif_tags_rs;
	std::vector<unif_tags_t> unif_tags_store;
	bc::flat_map<uint32_t, uint32SortedVector*> unif_sets_rs;
	std::vector<uint32SortedVector> unif_sets_store;
	unif_tags_t* unif_tags;
	uint32_t unif_last_wordform;
	uint32_t unif_last_baseform;
	uint32_t unif_last_textual;
	uint32SortedVector* unif_sets;
	bool unif_sets_firstrun;

	scoped_stack<TagList> ss_taglist;
	scoped_stack<unif_tags_t> ss_utags;
	scoped_stack<uint32SortedVector> ss_u32sv;

	uint32FlatHashSet index_regexp_yes;
	uint32FlatHashSet index_regexp_no;
	uint32FlatHashSet index_icase_yes;
	uint32FlatHashSet index_icase_no;
	std::vector<uint32FlatHashSet> index_readingSet_yes;
	std::vector<uint32FlatHashSet> index_readingSet_no;
	uint32FlatHashSet index_ruleCohort_no;
	void resetIndexes();

	Tag* makeBaseFromWord(uint32_t tag);
	Tag* makeBaseFromWord(Tag* tag);

	bool updateRuleToCohorts(Cohort& c, const uint32_t& rsit);
	void indexSingleWindow(SingleWindow& current);
	uint32_t runGrammarOnSingleWindow(SingleWindow& current);
	bool updateValidRules(const uint32IntervalVector& rules, uint32IntervalVector& intersects, const uint32_t& hash, Reading& reading);
	uint32_t runRulesOnSingleWindow(SingleWindow& current, const uint32IntervalVector& rules);

	enum ST_RETVALS {
		TRV_BREAK         = (1 <<  0),
		TRV_BARRIER       = (1 <<  1),
		TRV_BREAK_DEFAULT = (1 <<  2),
	};
	Cohort* runSingleTest(Cohort* cohort, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep = 0, Cohort* origin = 0);
	Cohort* runSingleTest(SingleWindow* sWindow, size_t i, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep = 0, Cohort* origin = 0);
	bool posOutputHelper(const SingleWindow* sWindow, uint32_t position, const ContextualTest* test, const Cohort* cohort, const Cohort* cdeep);
	Cohort* runContextualTest_tmpl(SingleWindow* sWindow, size_t position, const ContextualTest* test, ContextualTest* tmpl, Cohort*& cdeep, Cohort* origin);
	Cohort* runContextualTest(SingleWindow* sWindow, size_t position, const ContextualTest* test, Cohort** deep = 0, Cohort* origin = 0);
	Cohort* runDependencyTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep = 0, Cohort* origin = 0, const Cohort* self = 0);
	Cohort* runParenthesisTest(SingleWindow* sWindow, const Cohort* current, const ContextualTest* test, Cohort** deep = 0, Cohort* origin = 0);
	Cohort* runRelationTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep = 0, Cohort* origin = 0);

	bool doesWordformsMatch(const Tag* cword, const Tag* rword);
	uint32_t doesTagMatchRegexp(uint32_t test, const Tag& tag, bool bypass_index = false);
	uint32_t doesTagMatchIcase(uint32_t test, const Tag& tag, bool bypass_index = false);
	uint32_t doesRegexpMatchLine(const Reading& reading, const Tag& tag, bool bypass_index = false);
	uint32_t doesRegexpMatchReading(const Reading& reading, const Tag& tag, bool bypass_index = false);
	uint32_t doesTagMatchReading(const Reading& reading, const Tag& tag, bool unif_mode = false, bool bypass_index = false);
	bool doesSetMatchReading_trie(const Reading& reading, const Set& theset, const trie_t& trie, bool unif_mode = false);
	bool doesSetMatchReading_tags(const Reading& reading, const Set& theset, bool unif_mode = false);
	bool doesSetMatchReading(const Reading& reading, const uint32_t set, bool bypass_index = false, bool unif_mode = false);

	inline bool doesSetMatchCohort_testLinked(Cohort& cohort, const Set& theset, dSMC_Context* context = 0);
	inline bool doesSetMatchCohort_helper(Cohort& cohort, Reading& reading, const Set& theset, dSMC_Context* context = 0);
	bool doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, dSMC_Context* context = 0);
	bool doesSetMatchCohortCareful(Cohort& cohort, const uint32_t set, dSMC_Context* context = 0);

	bool statistics;
	ticks gtimer;

	Cohort* delimitAt(SingleWindow& current, Cohort* cohort);
	void reflowReading(Reading& reading);
	Tag* generateVarstringTag(const Tag* tag);
	void delTagFromReading(Reading& reading, uint32_t tag);
	void delTagFromReading(Reading& reading, Tag* tag);
	bool unmapReading(Reading& reading, const uint32_t rule);
	TagList getTagList(const Set& theSet, bool unif_mode = false) const;
	void getTagList(const Set& theSet, TagList& theTags, bool unif_mode = false) const;
	void mergeReadings(ReadingList& readings);
	void mergeMappings(Cohort& cohort);
	bool isChildOf(const Cohort* child, const Cohort* parent);
	bool wouldParentChildLoop(const Cohort* parent, const Cohort* child);
	bool wouldParentChildCross(const Cohort* parent, const Cohort* child);
	bool attachParentChild(Cohort& parent, Cohort& child, bool allowloop = false, bool allowcrossing = false);

	void reflowTextuals_Reading(Reading& r);
	void reflowTextuals_Cohort(Cohort& c);
	void reflowTextuals_SingleWindow(SingleWindow& sw);
	void reflowTextuals();

	Reading* initEmptyCohort(Cohort& cohort);

	std::deque<Reading> subs_any;
	Reading* get_sub_reading(Reading* tr, int sub_reading);
};
}

#endif

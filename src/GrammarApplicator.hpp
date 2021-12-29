/*
* Copyright (C) 2007-2021, GrammarSoft ApS
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
	Cohort* min = nullptr;
	Cohort* max = nullptr;
	std::vector<const ContextualTest*> linked;
	bool in_template = false;

	void clear() {
		min = nullptr;
		max = nullptr;
		linked.clear();
		in_template = false;
	}
};

struct dSMC_Context {
	const ContextualTest* test = nullptr;
	Cohort** deep = nullptr;
	Cohort* origin = nullptr;
	uint64_t options = 0;
	bool did_test = false;
	bool matched_target = false;
	bool matched_tests = false;
	bool in_barrier = false;
};

class GrammarApplicator {
public:
	bool always_span = false;
	bool apply_mappings = true;
	bool apply_corrections = true;
	bool no_before_sections = false;
	bool no_sections = false;
	bool no_after_sections = false;
	bool trace = false;
	bool trace_name_only = false;
	bool trace_no_removed = false;
	bool trace_encl = false;
	bool allow_magic_readings = true;
	bool no_pass_origin = false;
	bool unsafe = false;
	bool ordered = false;
	bool show_end_tags = false;
	bool unicode_tags = false;
	bool unique_tags = false;
	bool dry_run = false;
	bool owns_grammar = false;
	bool input_eof = false;
	bool seen_barrier = false;
	bool is_conv = false;
	bool split_mappings = false;
	bool pipe_deleted = false;
	bool add_spacing = true;

	bool dep_has_spanned = false;
	uint32_t dep_delimit = 0;
	bool dep_absolute = false;
	bool dep_original = false;
	bool dep_block_loops = true;
	bool dep_block_crossing = false;

	uint32_t num_windows = 2;
	uint32_t soft_limit = 300;
	uint32_t hard_limit = 500;
	uint32Vector sections;
	uint32IntervalVector valid_rules;
	uint32IntervalVector trace_rules;
	uint32FlatHashMap variables;
	uint32_t verbosity_level = 0;
	uint32_t debug_level = 0;
	uint32_t section_max_count = 0;

	GrammarApplicator(std::ostream& ux_err);
	virtual ~GrammarApplicator();

	void enableStatistics();
	void disableStatistics();

	void setGrammar(Grammar* res);
	void setTextDelimiter(UString rx);
	void index();

	virtual void runGrammarOnText(std::istream& input, std::ostream& output);

	bool has_dep = false;
	bool parse_dep = false;
	uint32_t dep_highest_seen = 0;
	std::unique_ptr<Window> gWindow;
	void reflowDependencyWindow(uint32_t max = 0);

	bool has_relations = false;
	void reflowRelationWindow(uint32_t max = 0);

	Grammar* grammar = nullptr;

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

	std::istream* ux_stdin = nullptr;
	std::ostream* ux_stdout = nullptr;
	std::ostream* ux_stderr = nullptr;
	UChar* filebase = nullptr;
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
	UChar ws[4]{ ' ', '\t', 0, 0 };

	uint32_t numLines = 0;
	uint32_t numWindows = 0;
	uint32_t numCohorts = 0;
	uint32_t numReadings = 0;

	bool did_index = false;
	sorted_vector<std::pair<uint32_t, uint32_t>> dep_deep_seen;

	uint32_t numsections = 0;
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

	uint32_t match_single = 0, match_comp = 0, match_sub = 0;
	uint32_t begintag = 0, endtag = 0, substtag = 0;
	Tag *tag_begin = nullptr, *tag_end = nullptr, *tag_subst = nullptr;
	uint32_t par_left_tag = 0, par_right_tag = 0;
	uint32_t par_left_pos = 0, par_right_pos = 0;
	bool did_final_enclosure = false;
	uint32_t mprefix_key = 0, mprefix_value = 0;

	tmpl_context_t tmpl_cntx;

	std::vector<regexgrps_t> regexgrps_store;
	std::pair<uint8_t, regexgrps_t*> regexgrps;
	bc::flat_map<uint32_t, uint8_t> regexgrps_z;
	bc::flat_map<uint32_t, regexgrps_t*> regexgrps_c;
	uint32_t same_basic = 0;
	Cohort* target = nullptr;
	Cohort* mark = nullptr;
	Cohort* attach_to = nullptr;
	Cohort* merge_with = nullptr;
	Rule* current_rule = nullptr;

	typedef bc::flat_map<uint32_t, Reading*> readings_plain_t;
	readings_plain_t readings_plain;
	std::vector<URegularExpression*> text_delimiters;

	typedef bc::flat_map<uint32_t, const void*> unif_tags_t;
	bc::flat_map<uint32_t, unif_tags_t*> unif_tags_rs;
	std::vector<unif_tags_t> unif_tags_store;
	typedef bc::flat_map<uint32_t, uint32SortedVector> unif_sets_t;
	bc::flat_map<uint32_t, unif_sets_t*> unif_sets_rs;
	std::vector<unif_sets_t> unif_sets_store;
	unif_tags_t* unif_tags = nullptr;
	uint32_t unif_last_wordform = 0;
	uint32_t unif_last_baseform = 0;
	uint32_t unif_last_textual = 0;
	unif_sets_t* unif_sets = nullptr;
	bc::flat_map<uint32_t, uint32_t> rule_hits;

	scoped_stack<TagList> ss_taglist;
	scoped_stack<unif_tags_t> ss_utags;
	scoped_stack<unif_sets_t> ss_usets;
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
	Cohort* runSingleTest(Cohort* cohort, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep = nullptr, Cohort* origin = nullptr);
	Cohort* runSingleTest(SingleWindow* sWindow, size_t i, const ContextualTest* test, uint8_t& rvs, bool* retval, Cohort** deep = nullptr, Cohort* origin = nullptr);
	bool posOutputHelper(const SingleWindow* sWindow, size_t position, const ContextualTest* test, const Cohort* cohort, const Cohort* cdeep);
	Cohort* runContextualTest_tmpl(SingleWindow* sWindow, size_t position, const ContextualTest* test, ContextualTest* tmpl, Cohort*& cdeep, Cohort* origin);
	Cohort* runContextualTest(SingleWindow* sWindow, size_t position, const ContextualTest* test, Cohort** deep = nullptr, Cohort* origin = nullptr);
	Cohort* runDependencyTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep = nullptr, Cohort* origin = nullptr, const Cohort* self = nullptr);
	Cohort* runParenthesisTest(SingleWindow* sWindow, const Cohort* current, const ContextualTest* test, Cohort** deep = nullptr, Cohort* origin = nullptr);
	Cohort* runRelationTest(SingleWindow* sWindow, Cohort* current, const ContextualTest* test, Cohort** deep = nullptr, Cohort* origin = nullptr);

	bool doesWordformsMatch(const Tag* cword, const Tag* rword);
	uint32_t doesTagMatchRegexp(uint32_t test, const Tag& tag, bool bypass_index = false);
	uint32_t doesTagMatchIcase(uint32_t test, const Tag& tag, bool bypass_index = false);
	uint32_t doesRegexpMatchLine(const Reading& reading, const Tag& tag, bool bypass_index = false);
	uint32_t doesRegexpMatchReading(const Reading& reading, const Tag& tag, bool bypass_index = false);
	uint32_t doesTagMatchReading(const Reading& reading, const Tag& tag, bool unif_mode = false, bool bypass_index = false);
	bool doesSetMatchReading_trie(const Reading& reading, const Set& theset, const trie_t& trie, bool unif_mode = false);
	bool doesSetMatchReading_tags(const Reading& reading, const Set& theset, bool unif_mode = false);
	bool doesSetMatchReading(const Reading& reading, const uint32_t set, bool bypass_index = false, bool unif_mode = false);

	inline bool doesSetMatchCohort_testLinked(Cohort& cohort, const Set& theset, dSMC_Context* context = nullptr);
	inline bool doesSetMatchCohort_helper(Cohort& cohort, Reading& reading, const Set& theset, dSMC_Context* context = nullptr);
	bool doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, dSMC_Context* context = nullptr);
	bool doesSetMatchCohortCareful(Cohort& cohort, const uint32_t set, dSMC_Context* context = nullptr);

	bool statistics = false;
	ticks gtimer{};

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

/*
* Copyright (C) 2007-2025, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "ContextualTest.hpp"
#include "version.hpp"
#include "process.hpp"

namespace CG3 {

enum {
	RV_NOTHING   = 1,
	RV_SOMETHING = 2,
	RV_DELIMITED = 4,
	RV_TRACERULE = 8,
};

bool GrammarApplicator::doesWordformsMatch(const Tag* cword, const Tag* rword) {
	if (rword && rword != cword) {
		if (rword->type & T_REGEXP) {
			if (!doesTagMatchRegexp(cword->hash, *rword)) {
				return false;
			}
		}
		else if (rword->type & T_CASE_INSENSITIVE) {
			if (!doesTagMatchIcase(cword->hash, *rword)) {
				return false;
			}
		}
		else {
			return false;
		}
	}
	return true;
}

bool GrammarApplicator::updateRuleToCohorts(Cohort& c, const uint32_t& rsit) {
	// Check whether this rule is in the allowed rule list from cmdline flag --rule(s)
	if (!valid_rules.empty() && !valid_rules.contains(rsit)) {
		return false;
	}
	SingleWindow* current = c.parent;
	const Rule* r = grammar->rule_by_number[rsit];
	if (!doesWordformsMatch(c.wordform, r->wordform)) {
		return false;
	}
	if (current->rule_to_cohorts.size() < rsit+1) {
		indexSingleWindow(*current);
	}
	CohortSet& cohortset = current->rule_to_cohorts[rsit];
	std::vector<size_t> csi;
	for (size_t i = 0; i < cohortsets.size(); ++i) {
		if (cohortsets[i] != &cohortset) {
			continue;
		}
		csi.push_back(i);
	}
	if (!csi.empty()) {
		auto cap = cohortset.capacity();
		std::vector<CohortSet::const_iterator*> ends;
		std::vector<std::pair<CohortSet::const_iterator*,Cohort*>> chs;
		for (size_t i = 0; i < csi.size(); ++i) {
			if (*rocits[csi[i]] == cohortset.end()) {
				ends.push_back(rocits[csi[i]]);
			}
			else {
				chs.push_back(std::pair(rocits[csi[i]], **rocits[csi[i]]));
			}
		}
		cohortset.insert(&c);
		for (auto it : ends) {
			*it = cohortset.end();
		}
		if (cap != cohortset.capacity()) {
			for (auto& it : chs) {
				*it.first = cohortset.find(it.second);
			}
		}
	}
	else {
		cohortset.insert(&c);
	}
	return current->valid_rules.insert(rsit);
}

bool GrammarApplicator::updateValidRules(const uint32IntervalVector& rules, uint32IntervalVector& intersects, const uint32_t& hash, Reading& reading) {
	size_t os = intersects.size();
	auto it = grammar->rules_by_tag.find(hash);
	if (it != grammar->rules_by_tag.end()) {
		Cohort& c = *(reading.parent);
		for (auto rsit : (it->second)) {
			if (updateRuleToCohorts(c, rsit) && rules.contains(rsit)) {
				intersects.insert(rsit);
			}
		}
	}
	return (os != intersects.size());
}

void GrammarApplicator::indexSingleWindow(SingleWindow& current) {
	current.valid_rules.clear();
	current.rule_to_cohorts.resize(grammar->rule_by_number.size());
	for (auto& cs : current.rule_to_cohorts) {
		cs.clear();
	}

	for (auto c : current.cohorts) {
		for (uint32_t psit = 0; psit < c->possible_sets.size(); ++psit) {
			if (c->possible_sets.test(psit) == false) {
				continue;
			}
			auto rules_it = grammar->rules_by_set.find(psit);
			if (rules_it == grammar->rules_by_set.end()) {
				continue;
			}
			for (auto rsit : rules_it->second) {
				updateRuleToCohorts(*c, rsit);
			}
		}
	}
}

TagList GrammarApplicator::getTagList(const Set& theSet, bool unif_mode) const {
	TagList theTags;
	getTagList(theSet, theTags, unif_mode);
	return theTags;
}

void GrammarApplicator::getTagList(const Set& theSet, TagList& theTags, bool unif_mode) const {
	if (theSet.type & ST_SET_UNIFY) {
		const auto& usets = (*context_stack.back().unif_sets)[theSet.number];
		const Set& pSet = *(grammar->sets_list[theSet.sets[0]]);
		for (auto iter : pSet.sets) {
			if (usets.count(iter)) {
				getTagList(*(grammar->sets_list[iter]), theTags);
			}
		}
	}
	else if (theSet.type & ST_TAG_UNIFY) {
		for (auto iter : theSet.sets) {
			getTagList(*(grammar->sets_list[iter]), theTags, true);
		}
	}
	else if (!theSet.sets.empty()) {
		for (auto iter : theSet.sets) {
			getTagList(*(grammar->sets_list[iter]), theTags, unif_mode);
		}
	}
	else if (unif_mode) {
		auto unif_tags = context_stack.back().unif_tags;
		auto iter = unif_tags->find(theSet.number);
		if (iter != unif_tags->end()) {
			trie_getTagList(theSet.trie, theTags, iter->second);
			trie_getTagList(theSet.trie_special, theTags, iter->second);
		}
	}
	else {
		trie_getTagList(theSet.trie, theTags);
		trie_getTagList(theSet.trie_special, theTags);
	}
	// Eliminate consecutive duplicates. Not all duplicates, since AddCohort and Append may have multiple readings with repeated tags
	for (auto ot = theTags.begin(); theTags.size() > 1 && ot != theTags.end(); ++ot) {
		auto it = ot;
		++it;
		for (; it != theTags.end() && std::distance(ot, it) == 1;) {
			if (*ot == *it) {
				it = theTags.erase(it);
			}
			else {
				++it;
			}
		}
	}
}

Reading* GrammarApplicator::get_sub_reading(Reading* tr, int sub_reading) {
	if (sub_reading == 0) {
		return tr;
	}

	if (sub_reading == GSR_ANY) {
		// If there aren't any sub-readings, the primary reading is the same as the amalgamation of all readings
		if (tr->next == nullptr) {
			return tr;
		}

		subs_any.emplace_back(Reading());
		Reading* reading = &subs_any.back();
		*reading = *tr;
		reading->next = nullptr;
		while (tr->next) {
			tr = tr->next;
			reading->tags_list.push_back(0);
			reading->tags_list.insert(reading->tags_list.end(), tr->tags_list.begin(), tr->tags_list.end());
			for (auto tag : tr->tags) {
				reading->tags.insert(tag);
				reading->tags_bloom.insert(tag);
			}
			for (auto tag : tr->tags_plain) {
				reading->tags_plain.insert(tag);
				reading->tags_plain_bloom.insert(tag);
			}
			for (auto tag : tr->tags_textual) {
				reading->tags_textual.insert(tag);
				reading->tags_textual_bloom.insert(tag);
			}
			reading->tags_numerical.insert(tr->tags_numerical.begin(), tr->tags_numerical.end());
			if (tr->mapped) {
				reading->mapped = true;
			}
			if (tr->mapping) {
				reading->mapping = tr->mapping;
			}
			if (tr->matched_target) {
				reading->matched_target = true;
			}
			if (tr->matched_tests) {
				reading->matched_tests = true;
			}
		}
		reading->rehash();
		return reading;
	}

	if (sub_reading > 0) {
		for (int i = 0; i < sub_reading && tr; ++i) {
			tr = tr->next;
		}
	}
	else if (sub_reading < 0) {
		int ntr = 0;
		Reading* ttr = tr;
		while (ttr) {
			ttr = ttr->next;
			--ntr;
		}
		if (!tr->next) {
			tr = nullptr;
		}
		for (auto i = ntr; i < sub_reading && tr; ++i) {
			tr = tr->next;
		}
	}
	return tr;
}

#define TRACE                                                       \
	do {                                                            \
		get_apply_to().subreading->hit_by.push_back(rule->number);  \
		if (rule->sub_reading == 32767) {                           \
			get_apply_to().reading->hit_by.push_back(rule->number); \
		}                                                           \
	} while (0)

#define FILL_TAG_LIST(taglist)                                                      \
	do {                                                                            \
		Reading& reading = *get_apply_to().subreading;								\
		for (auto it = (taglist)->begin(); it != (taglist)->end();) {               \
			if (reading.tags.find((*it)->hash) == reading.tags.end()) {             \
				auto tt = *it;                                                      \
				it = (taglist)->erase(it);                                          \
				if (tt->type & T_SPECIAL) {                                         \
					if (context_stack.back().regexgrps == nullptr) { \
						context_stack.back().regexgrps = &regexgrps_store[used_regex]; \
					}                                                               \
					auto stag = doesTagMatchReading(reading, *tt, false, true);     \
					if (stag) {                                                     \
						(taglist)->insert(it, grammar->single_tags.find(stag)->second); \
					}                                                               \
				}                                                                   \
				continue;                                                           \
			}                                                                       \
			++it;                                                                   \
		}                                                                           \
	} while (0)

#define FILL_TAG_LIST_RAW(taglist)                                              	\
	do {                                                                        	\
		Reading& reading = *get_apply_to().subreading;								\
		for (auto& tt : *(taglist)) {                                           	\
			if (tt->type & T_SPECIAL) {                                         	\
				if (context_stack.back().regexgrps == nullptr) {					\
					context_stack.back().regexgrps = &regexgrps_store[used_regex]; 	\
				}                                                               	\
				auto stag = doesTagMatchReading(reading, *tt, false, true);     	\
				if (stag) {                                                     	\
					tt = grammar->single_tags.find(stag)->second;               	\
				}                                                               	\
			}                                                                   	\
		}                                                                       	\
	} while (0)

#define APPEND_TAGLIST_TO_READING(taglist, reading)                                  \
	do {                                                                             \
		for (auto tter : (taglist)) {                                                \
			while (tter->type & T_VARSTRING) {                                       \
				tter = generateVarstringTag(tter);                                   \
			}                                                                        \
			auto hash = tter->hash;                                                  \
			if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) { \
				mappings->push_back(tter);                                           \
			}                                                                        \
			else {                                                                   \
				hash = addTagToReading((reading), tter);							 \
			}                                                                        \
			if (updateValidRules(rules, intersects, hash, reading)) {                \
				iter_rules = intersects.find(rule->number);                          \
				iter_rules_end = intersects.end();                                   \
			}                                                                        \
		}                                                                            \
	} while (0)

#define VARSTRINGIFY(tag)                                      \
	do {                                                       \
		while ((tag)->type & T_VARSTRING) {                    \
			(tag) = generateVarstringTag((tag));               \
		}                                                      \
    }                                                          \
	while (0)


bool GrammarApplicator::runSingleRule(SingleWindow& current, const Rule& rule, RuleCallback reading_cb, RuleCallback cohort_cb) {
	finish_cohort_loop = true;
	bool anything_changed = false;
	KEYWORDS type = rule.type;
	const Set& set = *(grammar->sets_list[rule.target]);
	CohortSet* cohortset = &current.rule_to_cohorts[rule.number];

	auto override_cohortset = [&]() {
		if (in_nested) {
			if (!current.nested_rule_to_cohorts) {
				current.nested_rule_to_cohorts.reset(new CohortSet());
			}
			cohortset = current.nested_rule_to_cohorts.get();
			cohortset->clear();
			cohortset->insert(get_apply_to().cohort);
			for (auto& t : set.trie_special) {
				if (t.first->type & T_CONTEXT && t.first->context_ref_pos <= context_stack.back().context.size()) {
					cohortset->insert(context_stack.back().context[t.first->context_ref_pos - 1]);
				}
			}
		}
	};
	override_cohortset();
	cohortsets.push_back(cohortset);
	rocits.push_back(nullptr);

	scope_guard popper([&]() {
		cohortsets.pop_back();
		rocits.pop_back();
		});

	if (debug_level > 1) {
		std::cerr << "DEBUG: " << cohortset->size() << "/" << current.cohorts.size() << " = " << double(cohortset->size()) / double(current.cohorts.size()) << std::endl;
	}
	for (auto rocit = cohortset->cbegin(); (!cohortset->empty()) && (rocit != cohortset->cend());) {
		rocits.back() = &rocit;
		Cohort* cohort = *rocit;
		++rocit;

		finish_reading_loop = true;

		if (debug_level > 1) {
			std::cerr << "DEBUG: Trying cohort " << cohort->global_number << ":" << cohort->local_number << std::endl;
		}

		// If the current cohort is the initial >>> one, skip it.
		if (cohort->local_number == 0) {
			continue;
		}
		// If the cohort is removed, skip it...
		// Removed cohorts are still in the precalculated rule_to_cohorts map,
		// and it would take time to go through the whole map searching for the cohort.
		// Haven't tested whether it is worth it...
		if (cohort->type & (CT_REMOVED | CT_IGNORED)) {
			continue;
		}

		uint32_t c = cohort->local_number;
		// If the cohort is temporarily unavailable due to parentheses, skip it.
		if ((cohort->type & CT_ENCLOSED) || cohort->parent != &current) {
			continue;
		}
		// If there are no readings, skip it.
		// This is unlikely to happen as all cohorts will get a magic reading during input,
		// and not many use the unsafe Remove rules.
		if (cohort->readings.empty()) {
			continue;
		}
		// If there's no reason to even attempt to restore, just skip it.
		if (rule.type == K_RESTORE) {
			if ((rule.flags & RF_DELAYED) && cohort->delayed.empty()) {
				continue;
			}
			else if ((rule.flags & RF_IGNORED) && cohort->ignored.empty()) {
				continue;
			}
			else if (!(rule.flags & (RF_DELAYED|RF_IGNORED)) && cohort->deleted.empty()) {
				continue;
			}
		}
		// If there is not even a remote chance the target set might match this cohort, skip it.
		if (rule.sub_reading == 0 && (rule.target >= cohort->possible_sets.size() || !cohort->possible_sets.test(rule.target))) {
			continue;
		}

		// If there is only 1 reading left and it is a Select or safe Remove rule, skip it.
		if (cohort->readings.size() == 1) {
			if (type == K_SELECT) {
				continue;
			}
			if (type == K_REMOVE || type == K_IFF) {
				if (cohort->readings.front()->noprint) {
					continue;
				}
				if ((!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
					continue;
				}
			}
		}
		else if (type == K_UNMAP && rule.flags & RF_SAFE) {
			continue;
		}
		// If it's a Delimit rule and we're at the final cohort, skip it.
		if (type == K_DELIMIT && c == current.cohorts.size() - 1) {
				continue;
		}

		// If the rule is only supposed to run inside a parentheses, check if cohort is.
		if (rule.flags & RF_ENCL_INNER) {
			if (!par_left_pos) {
				continue;
			}
			if (cohort->local_number < par_left_pos || cohort->local_number > par_right_pos) {
				continue;
			}
		}
		// ...and if the rule should only run outside parentheses, check if cohort is.
		else if (rule.flags & RF_ENCL_OUTER) {
			if (par_left_pos && cohort->local_number >= par_left_pos && cohort->local_number <= par_right_pos) {
				continue;
			}
		}

		// If this is SETPARENT SAFE and there's already a parent, skip it.
		if (type == K_SETPARENT && (rule.flags & RF_SAFE) && cohort->dep_parent != DEP_NO_PARENT) {
			continue;
		}
		if ((rule.flags & RF_NOPARENT) && cohort->dep_parent != DEP_NO_PARENT) {
			continue;
		}

		// If this is REMPARENT and there's no parent, skip it.
		if ((type == K_REMPARENT || type == K_SWITCHPARENT) && cohort->dep_parent == DEP_NO_PARENT) {
			continue;
		}

		// Check if on previous runs the rule did not match this cohort, and skip if that is the case.
		// This cache is cleared if any rule causes any state change in the window.
		uint32_t ih = hash_value(rule.number, cohort->global_number);
		if (index_ruleCohort_no.contains(ih)) {
			continue;
		}
		index_ruleCohort_no.insert(ih);

		size_t num_active = 0;
		size_t num_iff = 0;

		std::vector<Rule_Context> reading_contexts;
		reading_contexts.reserve(cohort->readings.size());

		// Assume that Iff rules are really Remove rules, until proven otherwise.
		if (rule.type == K_IFF) {
			type = K_REMOVE;
		}

		bool did_test = false;
		bool test_good = false;
		bool matched_target = false;

		clear(readings_plain);
		clear(subs_any);

		// Varstring capture groups exist on a per-cohort basis, since we may need them for mapping later.
		clear(regexgrps_z);
		clear(regexgrps_c);
		clear(unif_tags_rs);
		clear(unif_sets_rs);

		used_regex = 0;
		regexgrps_store.resize(std::max(regexgrps_store.size(), cohort->readings.size()));
		regexgrps_z.reserve(std::max(regexgrps_z.size(), cohort->readings.size()));
		regexgrps_c.reserve(std::max(regexgrps_c.size(), cohort->readings.size()));

		size_t used_unif = 0;
		unif_tags_store.resize(std::max(unif_tags_store.size(), cohort->readings.size() + 1));
		unif_sets_store.resize(std::max(unif_sets_store.size(), cohort->readings.size() + 1));

		{
			Rule_Context context;
			context.target.cohort = cohort;
			context.is_with = (rule.type == K_WITH);
			context_stack.push_back(std::move(context));
		}

		auto reset_cohorts = [&]() {
			cohortset = &current.rule_to_cohorts[rule.number];
			override_cohortset();
			cohortsets.back() = cohortset;
			if (get_apply_to().cohort->type & (CT_REMOVED | CT_IGNORED)) {
				rocit = cohortset->lower_bound(current.cohorts[get_apply_to().cohort->local_number]);
			}
			else {
				rocit = cohortset->find(current.cohorts[get_apply_to().cohort->local_number]);
				if (rocit != cohortset->end()) {
					++rocit;
				}
			}
		};

	    // Remember the current state so we can compare later to see if anything has changed
		const size_t state_num_readings = cohort->readings.size();
		const size_t state_num_removed = cohort->deleted.size();
		const size_t state_num_delayed = cohort->delayed.size();
		const size_t state_num_ignored = cohort->ignored.size();

	    // This loop figures out which readings, if any, that are valid targets for the current rule
		// Criteria for valid is that the reading must match both target and all contextual tests
		for (size_t i = 0; i < cohort->readings.size(); ++i) {
			// ToDo: Switch sub-readings so that they build up a passed in vector<Reading*>
			Reading* reading = get_sub_reading(cohort->readings[i], rule.sub_reading);
			if (!reading) {
				cohort->readings[i]->matched_target = false;
				cohort->readings[i]->matched_tests = false;
				continue;
			}
			context_stack.back().target.reading = cohort->readings[i];
			context_stack.back().target.subreading = reading;

			// The state is stored in the readings themselves, so clear the old states
			reading->matched_target = false;
			reading->matched_tests = false;

			if (reading->mapped && (rule.type == K_MAP || rule.type == K_ADD || rule.type == K_REPLACE)) {
				continue;
			}
			if (reading->mapped && (rule.flags & RF_NOMAPPED)) {
				continue;
			}
			if (reading->noprint && !allow_magic_readings) {
				continue;
			}
			if (reading->immutable && rule.type != K_UNPROTECT) {
				if (type == K_SELECT) {
					reading->matched_target = true;
					reading->matched_tests = true;
					reading_contexts.push_back(context_stack.back());
				}
				++num_active;
				++num_iff;
				continue;
			}

			// Check if any previous reading of this cohort had the same plain signature, and if so just copy their results
			// This cache is cleared on a per-cohort basis
			did_test = false;
			if (!(set.type & (ST_SPECIAL | ST_MAPPING | ST_CHILD_UNIFY)) && !readings_plain.empty()) {
				auto rpit = readings_plain.find(reading->hash_plain);
				if (rpit != readings_plain.end()) {
					reading->matched_target = rpit->second->matched_target;
					reading->matched_tests = rpit->second->matched_tests;
					if (reading->matched_tests) {
						++num_active;
					}
					if (regexgrps_c.count(rpit->second->number)) {
						regexgrps_c[reading->number];
						regexgrps_c[reading->number] = regexgrps_c[rpit->second->number];
						regexgrps_z[reading->number];
						regexgrps_z[reading->number] = regexgrps_z[rpit->second->number];

						context_stack.back().regexgrp_ct = regexgrps_z[reading->number];
						context_stack.back().regexgrps = regexgrps_c[reading->number];
					}
					context_stack.back().unif_tags = unif_tags_rs[reading->hash_plain];
					context_stack.back().unif_sets = unif_sets_rs[reading->hash_plain];
					did_test = true;
					test_good = rpit->second->matched_tests;
					reading_contexts.push_back(context_stack.back());
					continue;
				}
			}

			// Regex capture is done on a per-reading basis, so clear all captured state.
			context_stack.back().regexgrp_ct = 0;
			context_stack.back().regexgrps = &regexgrps_store[used_regex];

			// Unification is done on a per-reading basis, so clear all unification state.
			context_stack.back().unif_tags = &unif_tags_store[used_unif];
			context_stack.back().unif_sets = &unif_sets_store[used_unif];
			unif_tags_rs[reading->hash_plain] = context_stack.back().unif_tags;
			unif_sets_rs[reading->hash_plain] = context_stack.back().unif_sets;
			unif_tags_rs[reading->hash] = context_stack.back().unif_tags;
			unif_sets_rs[reading->hash] = context_stack.back().unif_sets;
			++used_unif;

			context_stack.back().unif_tags->clear();
			context_stack.back().unif_sets->clear();

			unif_last_wordform = 0;
			unif_last_baseform = 0;
			unif_last_textual = 0;

			same_basic = reading->hash_plain;
			rule_target = context_target = nullptr;
			if (context_stack.size() > 1) {
				Cohort* m = context_stack[context_stack.size()-2].mark;
				if (m) set_mark(m);
				else set_mark(cohort);
			}
			else {
				set_mark(cohort);
			}
			uint8_t orz = context_stack.back().regexgrp_ct;
			for (auto r = cohort->readings[i]; r; r = r->next) {
				r->active = true;
			}
			rule_target = cohort;
			// Actually check if the reading is a valid target. First check if rule target matches...
			if (rule.target && doesSetMatchReading(*reading, rule.target, (set.type & (ST_CHILD_UNIFY | ST_SPECIAL)) != 0)) {
				bool regex_prop = true;
				if (orz != context_stack.back().regexgrp_ct) {
					did_test = false;
					regex_prop = false;
				}
				rule_target = context_target = cohort;
				reading->matched_target = true;
				matched_target = true;
				bool good = true;
				// If we didn't already run the contextual tests, run them now.
				if (!did_test) {
					context_stack.back().context.clear();
					foreach (it, rule.tests) {
						ContextualTest* test = *it;
						if (rule.flags & RF_RESETX || !(rule.flags & RF_REMEMBERX)) {
							set_mark(cohort);
						}
						seen_barrier = false;
						// Keeps track of where we have been, to prevent infinite recursion in trees with loops
						dep_deep_seen.clear();
						// Reset the counters for which types of CohortIterator we have in play
						std::fill(ci_depths.begin(), ci_depths.end(), UI32(0));
						tmpl_cntx.clear();
						// Run the contextual test...
						Cohort* next_test = nullptr;
						Cohort* result = nullptr;
						Cohort** deep = nullptr;
						if (rule.type == K_WITH) {
							deep = &result;
							merge_with = nullptr;
						}
						if (!(test->pos & POS_PASS_ORIGIN) && (no_pass_origin || (test->pos & POS_NO_PASS_ORIGIN))) {
							next_test = runContextualTest(&current, c, test, deep, cohort);
						}
						else {
							next_test = runContextualTest(&current, c, test, deep);
						}
						context_stack.back().context.push_back(merge_with ? merge_with : result);
						test_good = (next_test != nullptr);

						profileRuleContext(test_good, &rule, test);

						if (!test_good) {
							good = test_good;
							if (it != rule.tests.begin() && !(rule.flags & RF_KEEPORDER)) {
								rule.tests.erase(it);
								rule.tests.push_front(test);
							}
							break;
						}
						did_test = ((set.type & (ST_CHILD_UNIFY | ST_SPECIAL)) == 0 && context_stack.back().unif_tags->empty() && context_stack.back().unif_sets->empty());
					}
				}
				else {
					good = test_good;
				}
				if (good) {
					// We've found a match, so Iff should be treated as Select instead of Remove
					if (rule.type == K_IFF && type != K_SELECT) {
						type = K_SELECT;
						if (grammar->has_protect) {
							for (size_t j = 0; j < i; ++j) {
								Reading* reading = get_sub_reading(cohort->readings[j], rule.sub_reading);
								if (reading && reading->immutable) {
									reading->matched_target = true;
									reading->matched_tests = true;
									++num_active;
									++num_iff;
								}
							}
						}
					}
					reading->matched_tests = true;
					++num_active;
					if (profiler) {
						Profiler::Key k{ET_RULE, rule.number + 1 };
						auto& r = profiler->entries[k];
						++r.num_match;
						if (!r.example_window) {
							addProfilingExample(r);
						}
					}
					if (!debug_rules.empty() && debug_rules.contains(rule.line)) {
						printDebugRule(rule);
					}

					if (regex_prop && i && !regexgrps_c.empty()) {
						for (auto z = i; z > 0; --z) {
							auto it = regexgrps_c.find(cohort->readings[z - 1]->number);
							if (it != regexgrps_c.end()) {
								regexgrps_c.insert(std::make_pair(reading->number, it->second));
								regexgrps_z.insert(std::make_pair(reading->number, regexgrps_z.find(cohort->readings[z - 1]->number)->second));
								break;
							}
						}
					}
				}
				else {
					context_stack.back().regexgrp_ct = orz;
					if (!debug_rules.empty() && debug_rules.contains(rule.line)) {
						printDebugRule(rule, true, false);
					}
				}
				++num_iff;
			}
			else {
				context_stack.back().regexgrp_ct = orz;
				if (profiler) {
					Profiler::Key k{ ET_RULE, rule.number + 1 };
					++profiler->entries[k].num_fail;
				}
				if (!debug_rules.empty() && debug_rules.contains(rule.line)) {
					printDebugRule(rule, false, false);
				}
			}
			readings_plain.insert(std::make_pair(reading->hash_plain, reading));
			for (auto r = cohort->readings[i]; r; r = r->next) {
				r->active = false;
			}

			if (reading != cohort->readings[i]) {
				cohort->readings[i]->matched_target = reading->matched_target;
				cohort->readings[i]->matched_tests = reading->matched_tests;
			}
			if (context_stack.back().regexgrp_ct) {
				regexgrps_c[reading->number] = context_stack.back().regexgrps;
				regexgrps_z[reading->number] = context_stack.back().regexgrp_ct;
				++used_regex;
			}
			reading_contexts.push_back(context_stack.back());
		}

		if (state_num_readings != cohort->readings.size() || state_num_removed != cohort->deleted.size() || state_num_delayed != cohort->delayed.size() || state_num_ignored != cohort->ignored.size()) {
			anything_changed = true;
			cohort->type &= ~CT_NUM_CURRENT;
		}

		// If none of the readings were valid targets, remove this cohort from the rule's possible cohorts.
		if (num_active == 0 && (num_iff == 0 || rule.type != K_IFF)) {
			if (!matched_target) {
				--rocit;                         // We have already incremented rocit earlier, so take one step back...
				rocit = cohortset->erase(rocit); // ...and one step forward again
			}
			context_stack.pop_back();
			continue;
		}

		// All readings were valid targets, which means there is nothing to do for Select or safe Remove rules.
		if (num_active == cohort->readings.size()) {
			if (type == K_SELECT) {
				context_stack.pop_back();
				continue;
			}
			if (type == K_REMOVE && (!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
				context_stack.pop_back();
				continue;
			}
		}

		for (auto& ctx : reading_contexts) {
			if (!ctx.target.subreading->matched_target) {
				continue;
			}
			if (!ctx.target.subreading->matched_tests && rule.type != K_IFF) {
				continue;
			}
			context_stack.back() = ctx;
			reset_cohorts_for_loop = false;
			reading_cb();
			if (!finish_cohort_loop) {
				context_stack.pop_back();
				return anything_changed;
			}
			if (reset_cohorts_for_loop) {
				reset_cohorts();
				break;
			}
			if (!finish_reading_loop) {
				break;
			}
		}

		reset_cohorts_for_loop = false;
		cohort_cb();
		if (!finish_cohort_loop) {
			context_stack.pop_back();
			return anything_changed;
		}
		if (reset_cohorts_for_loop) {
			reset_cohorts();
		}
		context_stack.pop_back();
	}
	return anything_changed;
}

/**
 * Applies the passed rules to the passed SingleWindow.
 *
 * This function is called at least N*M times where N is number of sections in the grammar and M is the number of windows in the input.
 * Possibly many more times, since if a section changes the state of the window the section is run again.
 * Only when no further changes are caused at a level does it progress to next level.
 *
 * The loops in this function are increasingly explosive, despite efforts to contain them.
 * In the https://edu.visl.dk/cg3_performance.html test data, this function is called 1015 times.
 * The first loop (rules) is executed 3101728 times.
 * The second loop (cohorts) is executed 11087278 times.
 * The third loop (finding readings) is executed 11738927 times; of these, 1164585 (10%) match the rule target.
 * The fourth loop (contextual test) is executed 1184009 times; of those, 1156322 (97%) fail their contexts.
 * The fifth loop (acting on readings) is executed 41540 times.
 *
 * @param[in,out] current The window to apply rules on
 * @param[in] rules The rules to apply
 */
uint32_t GrammarApplicator::runRulesOnSingleWindow(SingleWindow& current, const uint32IntervalVector& rules) {
	uint32_t retval = RV_NOTHING;
	bool section_did_something = false;
	bool delimited = false;

	// ToDo: Now that numbering is used, can't this be made a normal max? Hm, maybe not since --sections can still force another order...but if we're smart, then we re-enumerate rules based on --sections
	uint32IntervalVector intersects = current.valid_rules.intersect(rules);
	ReadingList removed;
	ReadingList selected;

	if (debug_level > 1) {
		std::cerr << "DEBUG: Trying window " << current.number << std::endl;
	}

	current.parent->cohort_map[0] = current.cohorts.front();

	foreach (iter_rules, intersects) {
		// Conditionally re-sort the rule-to-cohort mapping when the current rule is finished, regardless of how it finishes
		struct Sorter {
			SingleWindow& current;
			bool do_sort = false;

			Sorter(SingleWindow& current)
			  : current(current)
			{}

			~Sorter() {
				if (do_sort) {
					for (auto& cs : current.rule_to_cohorts) {
						cs.sort();
					}
				}
			}
		} sorter(current);

	repeat_rule:
		bool rule_did_something = false;
		uint32_t j = (*iter_rules);

		// Check whether this rule is in the allowed rule list from cmdline flag --rule(s)
		if (!valid_rules.empty() && !valid_rules.contains(j)) {
			continue;
		}

		current_rule = grammar->rule_by_number[j];
		Rule* rule = grammar->rule_by_number[j];
		if (rule->type == K_IGNORE) {
			continue;
		}
		if (debug_level > 1) {
			std::cerr << "DEBUG: Trying rule " << rule->line << std::endl;
		}

		if (!apply_mappings && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
			continue;
		}
		if (!apply_corrections && (rule->type == K_SUBSTITUTE || rule->type == K_APPEND)) {
			continue;
		}
		// If there are parentheses and the rule is marked as only run on the final pass, skip if this is not it.
		if (current.has_enclosures) {
			if ((rule->flags & RF_ENCL_FINAL) && !did_final_enclosure) {
				continue;
			}
			if (did_final_enclosure && !(rule->flags & RF_ENCL_FINAL)) {
				continue;
			}
		}

		bool readings_changed = false;
		bool should_repeat = false;
		bool should_bail = false;

		auto reindex = [&](SingleWindow* which = nullptr) {
			if (!which) {
				which = &current;
			}
			foreach (iter, which->cohorts) {
				(*iter)->local_number = UI32(std::distance(which->cohorts.begin(), iter));
			}
			gWindow->rebuildCohortLinks();
		};

		auto collect_subtree = [&](CohortSet& cs, Cohort* head, uint32_t cset) {
			if (cset) {
				for (auto iter : current.cohorts) {
					// Always consider the initial cohort a match
					if (iter->global_number == head->global_number) {
						cs.insert(iter);
					}
					else if (iter->dep_parent == head->global_number && doesSetMatchCohortNormal(*iter, cset)) {
						cs.insert(iter);
					}
				}
				CohortSet more;
				for (auto iter : current.cohorts) {
					for (auto cht : cs) {
						// Do not grab the whole tree from the root, in case WithChild is not (*)
						if (cht->global_number == head->global_number) {
							continue;
						}
						if (isChildOf(iter, cht)) {
							more.insert(iter);
						}
					}
				}
				cs.insert(more.begin(), more.end());
			}
			else {
				cs.insert(head);
			}
		};

		auto add_cohort = [&](Cohort* cohort, size_t& spacesInAddedWf, CohortSet* withs = nullptr) {
			Cohort* cCohort = alloc_cohort(&current);
			cCohort->global_number = gWindow->cohort_counter++;

			Tag* wf = nullptr;
			std::vector<TagList> readings;
			auto theTags = ss_taglist.get();
			getTagList(*rule->maplist, theTags);

			for (auto& tter : *theTags) {
				if (tter->type & T_VSTR) {
					VARSTRINGIFY(tter);
				}
			}

			for (auto tter : *theTags) {
				if(tter->type & T_WORDFORM) {
					spacesInAddedWf = std::count_if(tter->tag.begin(), tter->tag.end(), [](UChar c){ return c == ' '; });
				}
				VARSTRINGIFY(tter);
				if (tter->type & T_WORDFORM) {
					cCohort->wordform = tter;
					wf = tter;
					continue;
				}
				if (!wf) {
					u_fprintf(ux_stderr, "Error: There must be a wordform before any other tags in ADDCOHORT/MERGECOHORTS on line %u before input line %u.\n", rule->line, numLines);
					CG3Quit(1);
				}
				if (tter->type & T_BASEFORM) {
					readings.resize(readings.size() + 1);
					readings.back().push_back(wf);
				}
				if (readings.empty()) {
					u_fprintf(ux_stderr, "Error: There must be a baseform after the wordform in ADDCOHORT/MERGECOHORTS on line %u before input line %u.\n", rule->line, numLines);
					CG3Quit(1);
				}
				readings.back().push_back(tter);
			}

			for (auto& tags : readings) {
				for (size_t i = 0; i < tags.size(); ++i) {
					if (tags[i]->hash == grammar->tag_any) {
						auto& nt = cohort->readings.front()->tags_list;
						if (nt.size() <= 2) {
							continue;
						}
						tags.reserve(tags.size() + nt.size() - 2);
						tags[i] = grammar->single_tags[nt[2]];
						for (size_t j = 3, k = 1; j < nt.size(); ++j) {
							if (grammar->single_tags[nt[j]]->type & T_DEPENDENCY) {
								continue;
							}
							tags.insert(tags.begin() + i + k, grammar->single_tags[nt[j]]);
							++k;
						}
					}
				}
			}

			for (auto& rit : readings) {
				Reading* cReading = alloc_reading(cCohort);
				++numReadings;
				insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
				cReading->hit_by.push_back(rule->number);
				cReading->noprint = false;
				TagList mappings;
				for (auto tter : rit) {
					uint32_t hash = tter->hash;
					VARSTRINGIFY(tter);
					if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
						mappings.push_back(tter);
					}
					else {
						hash = addTagToReading(*cReading, hash);
					}
					if (updateValidRules(rules, intersects, hash, *cReading)) {
						iter_rules = intersects.find(rule->number);
						iter_rules_end = intersects.end();
					}
				}
				if (!mappings.empty()) {
					splitMappings(mappings, *cCohort, *cReading);
				}
				cCohort->appendReading(cReading);
			}

			current.parent->cohort_map[cCohort->global_number] = cCohort;
			current.parent->dep_window[cCohort->global_number] = cCohort;
			if (grammar->addcohort_attach && (rule->type == K_ADDCOHORT_BEFORE || rule->type == K_ADDCOHORT_AFTER)) {
				attachParentChild(*cohort, *cCohort);
			}
			else if (rule->type == K_MERGECOHORTS && !(rule->flags & RF_DETACH)) {
				auto c = context_stack.back().target.cohort;
				if (c->dep_parent == DEP_NO_PARENT || !current.parent->cohort_map.count(c->dep_parent)) {
					// Don't introduce dependencies
					if (has_dep) {
						if (!withs->count(cohort)) {
							// Insertion target isn't being merged, so attach to that
							attachParentChild(*cohort, *cCohort);
						}
						else {
							// Attach to nearest un-merged token
							auto next = cohort->next;
							auto prev = cohort->prev;
							for (; next || prev ;) {
								if (next && next->parent != cohort->parent) {
									next = nullptr;
								}
								if (next && !withs->count(next)) {
									attachParentChild(*next, *cCohort);
									break;
								}
								if (next) {
									next = next->next;
								}

								if (prev && prev->parent != cohort->parent) {
									prev = nullptr;
								}
								if (prev && !withs->count(prev)) {
									attachParentChild(*prev, *cCohort);
									break;
								}
								if (prev) {
									prev = prev->prev;
								}
							}
						}
					}
				}
				else {
					attachParentChild(*current.parent->cohort_map[c->dep_parent], *cCohort);
				}

				std::set<uint32_t> ps;
				for (auto c : *withs) {
					ps.insert(c->global_number);
					if (c->type & CT_RELATED) {
						for (auto& riter : c->relations) {
							cCohort->relations[riter.first].insert(riter.second.begin(), riter.second.end());
						}
						cCohort->type |= CT_RELATED;
					}
				}
				for (auto c : current.all_cohorts) {
					if (ps.count(c->dep_parent)) {
						attachParentChild(*cCohort, *c);
					}
					for (auto& riter : c->relations) {
						for (auto r : ps) {
							if (riter.second.count(r)) {
								riter.second.erase(r);
								riter.second.insert(cCohort->global_number);
								cCohort->type |= CT_RELATED;
							}
						}
					}
				}
			}

			if (cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
				if (trace) {
					auto r = cCohort->readings.front();
					r->hit_by.push_back(rule->number);
					r->noprint = false;
				}
			}

			CohortSet cohorts;
			collect_subtree(cohorts, cohort, rule->childset1);

			if (rule->type == K_ADDCOHORT_BEFORE) {
				current.cohorts.insert(current.cohorts.begin() + cohorts.front()->local_number, cCohort);
				current.all_cohorts.insert(std::find(current.all_cohorts.begin() + cohorts.front()->local_number, current.all_cohorts.end(), cohorts.front()), cCohort);
			}
			else {
				current.cohorts.insert(current.cohorts.begin() + cohorts.back()->local_number + 1, cCohort);
				current.all_cohorts.insert(std::find(current.all_cohorts.begin() + cohorts.back()->local_number, current.all_cohorts.end(), cohorts.back()) + 1, cCohort);
			}

			foreach (iter, current.cohorts) {
				(*iter)->local_number = UI32(std::distance(current.cohorts.begin(), iter));
			}
			gWindow->rebuildCohortLinks();

			return cCohort;
		};

		auto rem_cohort = [&](Cohort* cohort) {
			auto& current = *cohort->parent;
			for (auto iter : cohort->readings) {
				iter->hit_by.push_back(rule->number);
				iter->deleted = true;
				if (trace) {
					iter->noprint = false;
				}
			}
			// Remove the cohort from all rules
			for (auto& cs : current.rule_to_cohorts) {
				cs.erase(cohort);
			}
			// Forward all children of this cohort to the parent of this cohort
			// ToDo: Named relations must be erased
			while (!cohort->dep_children.empty()) {
				uint32_t ch = cohort->dep_children.back();
				if (cohort->dep_parent == DEP_NO_PARENT) {
					attachParentChild(*gWindow->cohort_map[0], *gWindow->cohort_map[ch], true, true);
				}
				else {
					attachParentChild(*gWindow->cohort_map[cohort->dep_parent], *gWindow->cohort_map[ch], true, true);
				}
				cohort->dep_children.erase(ch);
			}
			cohort->type |= CT_REMOVED;
			cohort->detach();
			for (auto& cm : gWindow->cohort_map) {
				cm.second->dep_children.erase(cohort->dep_self);
			}
			gWindow->cohort_map.erase(cohort->global_number);
			current.cohorts.erase(current.cohorts.begin() + cohort->local_number);
			foreach (iter, current.cohorts) {
				(*iter)->local_number = UI32(std::distance(current.cohorts.begin(), iter));
			}

			if (current.cohorts.size() == 1 && &current != gWindow->current) {
				// This window is now empty, so remove it entirely from consideration so rules can look past it
				cohort = current.cohorts[0];

				// Remove the cohort from all rules
				for (auto& cs : current.rule_to_cohorts) {
					cs.erase(cohort);
				}
				cohort->detach();
				for (auto& cm : gWindow->cohort_map) {
					cm.second->dep_children.erase(cohort->dep_self);
				}
				gWindow->cohort_map.erase(cohort->global_number);
				free_cohort(cohort);

				if (current.previous) {
					current.previous->text += current.text + current.text_post;
					current.previous->all_cohorts.insert(current.previous->all_cohorts.end(), current.all_cohorts.begin() + 1, current.all_cohorts.end());
				}
				else if (current.next) {
					current.next->text = current.text_post + current.next->text;
					current.next->all_cohorts.insert(current.previous->all_cohorts.begin() + 1, current.all_cohorts.begin() + 1, current.all_cohorts.end());
				}
				current.all_cohorts.clear();

				for (size_t i = 0; i < gWindow->previous.size(); ++i) {
					if (gWindow->previous[i] == &current) {
						free_swindow(gWindow->previous[i]);
						gWindow->previous.erase(gWindow->previous.begin() + i);
						break;
					}
				}
				for (size_t i = 0; i < gWindow->next.size(); ++i) {
					if (gWindow->next[i] == &current) {
						free_swindow(gWindow->next[i]);
						gWindow->next.erase(gWindow->next.begin() + i);
						break;
					}
				}

				gWindow->rebuildSingleWindowLinks();
			}

			gWindow->rebuildCohortLinks();
		};

		auto ignore_cohort = [&](Cohort* cohort) {
			auto& current = *cohort->parent;
			for (auto iter : cohort->readings) {
				iter->hit_by.push_back(rule->number);
			}
			for (auto& cs : current.rule_to_cohorts) {
				cs.erase(cohort);
			}
			cohort->type |= CT_IGNORED;
			cohort->detach();
			gWindow->cohort_map.erase(cohort->global_number);
			current.cohorts.erase(current.cohorts.begin() + cohort->local_number);
		};

		auto make_relation_rtag = [&](Tag* tag, uint32_t id) {
			UChar tmp[256] = { 0 };
			u_sprintf(tmp, "R:%S:%u", tag->tag.data(), id);
			auto nt = addTag(tmp);
			return nt;
		};

		auto add_relation_rtag = [&](Cohort* cohort, Tag* tag, uint32_t id) {
			auto nt = make_relation_rtag(tag, id);
			for (auto& r : cohort->readings) {
				addTagToReading(*r, nt);
			}
		};

		auto set_relation_rtag = [&](Cohort* cohort, Tag* tag, uint32_t id) {
			auto nt = make_relation_rtag(tag, id);
			for (auto& r : cohort->readings) {
				for (auto it = r->tags_list.begin(); it != r->tags_list.end();) {
					const auto& utag = grammar->single_tags[*it]->tag;
					if (utag[0] == 'R' && utag[1] == ':' && utag.size() > 2 + tag->tag.size() && utag[2 + tag->tag.size()] == ':' && utag.compare(2, tag->tag.size(), tag->tag) == 0) {
						r->tags.erase(*it);
						r->tags_textual.erase(*it);
						r->tags_numerical.erase(*it);
						r->tags_plain.erase(*it);
						it = r->tags_list.erase(it);
					}
					else {
						++it;
					}
				}
				addTagToReading(*r, nt);
			}
		};

		auto rem_relation_rtag = [&](Cohort* cohort, Tag* tag, uint32_t id) {
			auto nt = make_relation_rtag(tag, id);
			for (auto& r : cohort->readings) {
				delTagFromReading(*r, nt);
			}
		};

		auto insert_taglist_to_reading = [&](auto& iter, auto& taglist, auto& reading, auto& mappings) {
			for (auto tag : taglist) {
				if (tag->type & T_VARSTRING) {
					tag = generateVarstringTag(tag);
				}
				if (tag->hash == grammar->tag_any) {
					break;
				}
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					mappings->push_back(tag);
				}
				else {
					iter = reading.tags_list.insert(iter, tag->hash);
					++iter;
				}
				if (updateValidRules(rules, intersects, tag->hash, reading)) {
					iter_rules = intersects.find(rule->number);
					iter_rules_end = intersects.end();
				}
			}
			reflowReading(reading);
		};

		auto cohort_cb = [&]() {
			if (rule->type == K_SELECT || (rule->type == K_IFF && !selected.empty())) {
				Cohort* target = get_apply_to().cohort;
				if (selected.size() < target->readings.size() && !selected.empty()) {
					ReadingList drop;
					size_t si = 0;
					for (size_t ri = 0; ri < target->readings.size(); ri++) {
						// Manually trace, since reading_cb doesn't get called on non-matching readings
						Reading* rd = target->readings[ri];
						if (rule->sub_reading != 32767) {
							rd = get_sub_reading(rd, rule->sub_reading);
						}
						if (rd) {
							rd->hit_by.push_back(rule->number);
						}
						if (si < selected.size() && target->readings[ri] == selected[si]) {
							si++;
						}
						else {
							target->readings[ri]->deleted = true;
							drop.push_back(target->readings[ri]);
						}
					}
					target->readings.swap(selected);
					if (rule->flags & RF_DELAYED) {
						target->delayed.insert(target->delayed.end(), drop.begin(), drop.end());
					}
					else if (rule->flags & RF_IGNORED) {
						target->ignored.insert(target->ignored.end(), drop.begin(), drop.end());
					}
					else {
						target->deleted.insert(target->deleted.end(), drop.begin(), drop.end());
					}
					readings_changed = true;
				}
				selected.clear();
			}
			else if (rule->type == K_REMOVE || rule->type == K_IFF) {
				if (!removed.empty() && (removed.size() < get_apply_to().cohort->readings.size() || (unsafe && !(rule->flags & RF_SAFE)) || (rule->flags & RF_UNSAFE))) {
					if (rule->flags & RF_DELAYED) {
						get_apply_to().cohort->delayed.insert(get_apply_to().cohort->delayed.end(), removed.begin(), removed.end());
					}
					else if (rule->flags & RF_IGNORED) {
						get_apply_to().cohort->ignored.insert(get_apply_to().cohort->ignored.end(), removed.begin(), removed.end());
					}
					else {
						get_apply_to().cohort->deleted.insert(get_apply_to().cohort->deleted.end(), removed.begin(), removed.end());
					}
					size_t oz = get_apply_to().cohort->readings.size();
					while (!removed.empty()) {
						removed.back()->deleted = true;
						for (size_t i = 0; i < oz; ++i) {
							if (get_apply_to().cohort->readings[i] == removed.back()) {
								--oz;
								std::swap(get_apply_to().cohort->readings[i], get_apply_to().cohort->readings[oz]);
							}
						}
						removed.pop_back();
					}
					get_apply_to().cohort->readings.resize(oz);
					if (debug_level > 0) {
						std::cerr << "DEBUG: Rule " << rule->line << " hit cohort " << get_apply_to().cohort->local_number << std::endl;
					}
					readings_changed = true;
				}
				if (get_apply_to().cohort->readings.empty()) {
					initEmptyCohort(*get_apply_to().cohort);
				}
				selected.clear();
			}
			else if (rule->type == K_JUMP) {
				auto to = getTagList(*rule->maplist).front();
				VARSTRINGIFY(to);
				auto it = grammar->anchors.find(to->hash);
				if (it == grammar->anchors.end()) {
					u_fprintf(ux_stderr, "Warning: JUMP on line %u could not find anchor '%S'.\n", rule->line, to->tag.data());
				}
				else {
					iter_rules = intersects.lower_bound(it->second);
					finish_cohort_loop = false;
					should_repeat = true;
				}
			}
			else if (rule->type == K_REMVARIABLE) {
				auto names = getTagList(*rule->maplist);
				for (auto tag : names) {
					VARSTRINGIFY(tag);
					auto it = variables.begin();
					if (tag->type & T_REGEXP) {
						it = std::find_if(it, variables.end(), [&](auto& kv) { return doesTagMatchRegexp(kv.first, *tag); });
					}
					else if (tag->type & T_CASE_INSENSITIVE) {
						it = std::find_if(it, variables.end(), [&](auto& kv) { return doesTagMatchIcase(kv.first, *tag); });
					}
					else {
						it = variables.find(tag->hash);
					}
					if (it != variables.end()) {
						if (rule->flags & RF_OUTPUT) {
							current.variables_output.insert(it->first);
						}
						variables.erase(it);
						//u_fprintf(ux_stderr, "Info: RemVariable fired for %S.\n", tag->tag.data());
					}
				}
			}
			else if (rule->type == K_SETVARIABLE) {
				auto names = getTagList(*rule->maplist);
				auto values = getTagList(*rule->sublist);
				VARSTRINGIFY(names.front());
				VARSTRINGIFY(values.front());
				variables[names.front()->hash] = values.front()->hash;
				if (rule->flags & RF_OUTPUT) {
					current.variables_output.insert(names.front()->hash);
				}
				//u_fprintf(ux_stderr, "Info: SetVariable fired for %S.\n", names.front()->tag.data());
			}
			else if (rule->type == K_DELIMIT) {
				auto cohort = get_apply_to().cohort;
				if (cohort->parent->cohorts.size() > cohort->local_number + 1) {
					delimitAt(current, cohort);
					delimited = true;
					readings_changed = true;
				}
			}
			else if (rule->type == K_EXTERNAL_ONCE || rule->type == K_EXTERNAL_ALWAYS) {
				if (rule->type == K_EXTERNAL_ONCE && !current.hit_external.insert(rule->line).second) {
					return;
				}

				auto ei = externals.find(rule->varname);
				if (ei == externals.end()) {
					Tag* ext = grammar->single_tags.find(rule->varname)->second;
					UErrorCode err = U_ZERO_ERROR;
					u_strToUTF8(&cbuffers[0][0], SI32(CG3_BUFFER_SIZE - 1), nullptr, ext->tag.data(), SI32(ext->tag.size()), &err);

					Process& es = externals[rule->varname];
					try {
						es.start(&cbuffers[0][0]);
						writeRaw(es, CG3_EXTERNAL_PROTOCOL);
					}
					catch (std::exception& e) {
						u_fprintf(ux_stderr, "Error: External on line %u resulted in error: %s\n", rule->line, e.what());
						CG3Quit(1);
					}
					ei = externals.find(rule->varname);
				}

				pipeOutSingleWindow(current, ei->second);
				pipeInSingleWindow(current, ei->second);

				indexSingleWindow(current);
				readings_changed = true;
				index_ruleCohort_no.clear();
				intersects = current.valid_rules.intersect(rules);
				iter_rules = intersects.find(rule->number);
				iter_rules_end = intersects.end();
				reset_cohorts_for_loop = true;
			}
			else if (rule->type == K_REMCOHORT) {
				// REMCOHORT-IGNORED
				if (rule->flags & RF_IGNORED) {
					CohortSet cohorts;
					collect_subtree(cohorts, get_apply_to().cohort, rule->childset1);
					for (auto c : reversed(cohorts)) {
						ignore_cohort(c);
					}
					reindex();
					reflowDependencyWindow();
				}
				else {
					rem_cohort(get_apply_to().cohort);
				}

				// If we just removed the last cohort, add <<< to the new last cohort
				if (get_apply_to().cohort->readings.front()->tags.count(endtag)) {
					for (auto r : current.cohorts.back()->readings) {
						addTagToReading(*r, endtag);
						if (updateValidRules(rules, intersects, endtag, *r)) {
							iter_rules = intersects.find(rule->number);
							iter_rules_end = intersects.end();
						}
					}
					index_ruleCohort_no.clear();
				}
				readings_changed = true;
				reset_cohorts_for_loop = true;
			}
		};

		RuleCallback reading_cb = [&]() {
			if (rule->type == K_SELECT || (rule->type == K_IFF && get_apply_to().subreading->matched_tests)) {
				selected.push_back(get_apply_to().reading);
				index_ruleCohort_no.clear();
			}
			else if (rule->type == K_REMOVE || rule->type == K_IFF) {
				if (rule->type == K_REMOVE && (rule->flags & RF_UNMAPLAST) && removed.size() == get_apply_to().cohort->readings.size() - 1) {
					if (unmapReading(*get_apply_to().subreading, rule->number)) {
						readings_changed = true;
					}
				}
				else {
					TRACE;
					removed.push_back(get_apply_to().reading);
				}
				index_ruleCohort_no.clear();
			}
			else if (rule->type == K_PROTECT) {
				TRACE;
				get_apply_to().subreading->immutable = true;
			}
			else if (rule->type == K_UNPROTECT) {
				TRACE;
				get_apply_to().subreading->immutable = false;
			}
			else if (rule->type == K_UNMAP) {
				if (unmapReading(*get_apply_to().subreading, rule->number)) {
					index_ruleCohort_no.clear();
					readings_changed = true;
				}
			}
			else if (rule->type == K_ADDCOHORT_AFTER || rule->type == K_ADDCOHORT_BEFORE) {
				index_ruleCohort_no.clear();
				TRACE;

				size_t spacesInAddedWf = 0; // not used here
				auto cCohort = add_cohort(get_apply_to().cohort, spacesInAddedWf);

				// If the new cohort is now the last cohort, add <<< to it and remove <<< from previous last cohort
				if (current.cohorts.back() == cCohort) {
					for (auto r : current.cohorts[current.cohorts.size() - 2]->readings) {
						delTagFromReading(*r, endtag);
					}
					for (auto r : current.cohorts.back()->readings) {
						addTagToReading(*r, endtag);
						if (updateValidRules(rules, intersects, endtag, *r)) {
							iter_rules = intersects.find(rule->number);
							iter_rules_end = intersects.end();
						}
					}
				}
				indexSingleWindow(current);
				readings_changed = true;

				reset_cohorts_for_loop = true;
			}
			else if (rule->type == K_SPLITCOHORT) {
				index_ruleCohort_no.clear();

				std::vector<std::pair<Cohort*, std::vector<TagList>>> cohorts;

				auto theTags = ss_taglist.get();
				getTagList(*rule->maplist, theTags);

				for (auto& tter : *theTags) {
					if (tter->type & T_VSTR) {
						VARSTRINGIFY(tter);
					}
				}

				Tag* wf = nullptr;
				for (auto tter : *theTags) {
					if (tter->type & T_WORDFORM) {
						cohorts.resize(cohorts.size() + 1);
						cohorts.back().first = alloc_cohort(&current);
						cohorts.back().first->global_number = gWindow->cohort_counter++;
						wf = tter;
						VARSTRINGIFY(wf);
						cohorts.back().first->wordform = wf;
						continue;
					}
					if (!wf) {
						u_fprintf(ux_stderr, "Error: There must be a wordform before any other tags in SPLITCOHORT on line %u before input line %u.\n", rule->line, numLines);
						CG3Quit(1);
					}
				}

				uint32_t rel_trg = DEP_NO_PARENT;
				std::vector<std::pair<uint32_t, uint32_t>> cohort_dep(cohorts.size());
				cohort_dep.front().second = DEP_NO_PARENT;
				cohort_dep.back().first = DEP_NO_PARENT;
				cohort_dep.back().second = UI32(cohort_dep.size() - 1);
				for (size_t i = 1; i < cohort_dep.size() - 1; ++i) {
					cohort_dep[i].second = UI32(i);
				}

				size_t i = 0;
				std::vector<TagList>* readings = &cohorts.front().second;
				Tag* bf = nullptr;
				for (auto tter : *theTags) {
					if (tter->type & T_WORDFORM) {
						++i;
						bf = nullptr;
						continue;
					}
					if (tter->type & T_BASEFORM) {
						readings = &cohorts[i - 1].second;
						readings->resize(readings->size() + 1);
						readings->back().push_back(cohorts[i - 1].first->wordform);
						bf = tter;
					}
					if (!bf) {
						u_fprintf(ux_stderr, "Error: There must be a baseform after the wordform in SPLITCOHORT on line %u before input line %u.\n", rule->line, numLines);
						CG3Quit(1);
					}

					UChar dep_self[12] = {};
					UChar dep_parent[12] = {};
					if (u_sscanf(tter->tag.data(), "%[0-9cd]->%[0-9pm]", &dep_self, &dep_parent) == 2) {
						if (dep_self[0] == 'c' || dep_self[0] == 'd') {
							cohort_dep[i - 1].first = DEP_NO_PARENT;
							if (rel_trg == DEP_NO_PARENT) {
								rel_trg = UI32(i - 1);
							}
						}
						else if (u_sscanf(dep_self, "%i", &cohort_dep[i - 1].first) != 1) {
							u_fprintf(ux_stderr, "Error: SPLITCOHORT dependency mapping dep_self was not valid on line %u before input line %u.\n", rule->line, numLines);
							CG3Quit(1);
						}
						if (dep_parent[0] == 'p' || dep_parent[0] == 'm') {
							cohort_dep[i - 1].second = DEP_NO_PARENT;
						}
						else if (u_sscanf(dep_parent, "%i", &cohort_dep[i - 1].second) != 1) {
							u_fprintf(ux_stderr, "Error: SPLITCOHORT dependency mapping dep_parent was not valid on line %u before input line %u.\n", rule->line, numLines);
							CG3Quit(1);
						}
						continue;
					}
					if (tter->tag.size() == 3 && tter->tag[0] == 'R' && tter->tag[1] == ':' && tter->tag[2] == '*') {
						rel_trg = UI32(i - 1);
						continue;
					}
					readings->back().push_back(tter);
				}

				if (rel_trg == DEP_NO_PARENT) {
					rel_trg = UI32(cohorts.size() - 1);
				}

				for (size_t i = 0; i < cohorts.size(); ++i) {
					Cohort* cCohort = cohorts[i].first;
					readings = &cohorts[i].second;

					for (auto tags : *readings) {
						Reading* cReading = alloc_reading(cCohort);
						++numReadings;
						insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
						cReading->hit_by.push_back(rule->number);
						cReading->noprint = false;
						TagList mappings;

						for (size_t i = 0; i < tags.size(); ++i) {
							if (tags[i]->hash == grammar->tag_any) {
								uint32Vector& nt = get_apply_to().cohort->readings.front()->tags_list;
								if (nt.size() <= 2) {
									continue;
								}
								tags.reserve(tags.size() + nt.size() - 2);
								tags[i] = grammar->single_tags[nt[2]];
								for (size_t j = 3, k = 1; j < nt.size(); ++j) {
									if (grammar->single_tags[nt[j]]->type & T_DEPENDENCY) {
										continue;
									}
									tags.insert(tags.begin() + i + k, grammar->single_tags[nt[j]]);
									++k;
								}
							}
						}

						for (auto tter : tags) {
							uint32_t hash = tter->hash;
							VARSTRINGIFY(tter);
							if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(tter);
							}
							else {
								hash = addTagToReading(*cReading, hash);
							}
							if (updateValidRules(rules, intersects, hash, *cReading)) {
								iter_rules = intersects.find(rule->number);
								iter_rules_end = intersects.end();
							}
						}
						if (!mappings.empty()) {
							splitMappings(mappings, *cCohort, *cReading);
						}
						cCohort->appendReading(cReading);
					}

					if (cCohort->readings.empty()) {
						initEmptyCohort(*cCohort);
					}

					current.parent->dep_window[cCohort->global_number] = cCohort;
					current.parent->cohort_map[cCohort->global_number] = cCohort;

					current.cohorts.insert(current.cohorts.begin() + get_apply_to().cohort->local_number + i + 1, cCohort);
					current.all_cohorts.insert(std::find(current.all_cohorts.begin() + get_apply_to().cohort->local_number, current.all_cohorts.end(), get_apply_to().cohort) + i + 1, cCohort);
				}

				// Move text from the to-be-deleted cohort to the last new cohort
				std::swap(cohorts.back().first->text, get_apply_to().cohort->text);

				for (size_t i = 0; i < cohorts.size(); ++i) {
					Cohort* cCohort = cohorts[i].first;

					if (cohort_dep[i].first == DEP_NO_PARENT) {
						while (!get_apply_to().cohort->dep_children.empty()) {
							uint32_t ch = get_apply_to().cohort->dep_children.back();
							attachParentChild(*cCohort, *current.parent->cohort_map[ch], true, true);
							get_apply_to().cohort->dep_children.erase(ch); // Just in case the attachment can't be made for some reason
						}
					}

					if (cohort_dep[i].second == DEP_NO_PARENT) {
						if (current.parent->cohort_map.count(get_apply_to().cohort->dep_parent)) {
							attachParentChild(*current.parent->cohort_map[get_apply_to().cohort->dep_parent], *cCohort, true, true);
						}
					}
					else {
						attachParentChild(*current.parent->cohort_map[cohorts.front().first->global_number + cohort_dep[i].second - 1], *cCohort, true, true);
					}

					// Re-attach all named relations to the dependency tail or R:* cohort
					if (rel_trg == i && (get_apply_to().cohort->type & CT_RELATED)) {
						cCohort->setRelated();
						cCohort->relations.swap(get_apply_to().cohort->relations);

						std::pair<SingleWindow**, size_t> swss[3] = {
							std::make_pair(&gWindow->previous[0], gWindow->previous.size()),
							std::make_pair(&gWindow->current, static_cast<size_t>(1)),
							std::make_pair(&gWindow->next[0], gWindow->next.size()),
						};
						for (auto sws : swss) {
							for (size_t sw = 0; sw < sws.second; ++sw) {
								for (auto ch : sws.first[sw]->cohorts) {
									for (auto& rel : ch->relations) {
										if (rel.second.count(get_apply_to().cohort->global_number)) {
											rel.second.erase(get_apply_to().cohort->global_number);
											rel.second.insert(cCohort->global_number);
										}
									}
								}
							}
						}
					}
				}

				// Remove the source cohort
				for (auto iter : get_apply_to().cohort->readings) {
					iter->hit_by.push_back(rule->number);
					iter->deleted = true;
				}
				get_apply_to().cohort->type |= CT_REMOVED;
				get_apply_to().cohort->detach();
				for (auto& cm : current.parent->cohort_map) {
					cm.second->dep_children.erase(get_apply_to().cohort->dep_self);
				}
				current.parent->cohort_map.erase(get_apply_to().cohort->global_number);
				current.cohorts.erase(current.cohorts.begin() + get_apply_to().cohort->local_number);

				reindex();
				indexSingleWindow(current);
				readings_changed = true;

				reset_cohorts_for_loop = true;
			}
			else if (rule->type == K_ADD || rule->type == K_MAP) {
				TRACE;
				auto state_hash = get_apply_to().subreading->hash;
				index_ruleCohort_no.clear();
				auto& reading = *(get_apply_to().subreading);
				reading.noprint = false;
				auto mappings = ss_taglist.get();
				auto theTags = ss_taglist.get();
				getTagList(*rule->maplist, theTags);

				bool did_insert = false;
				if (rule->childset1) {
					bool found_spot = false;
					auto spot_tags = ss_taglist.get();
					getTagList(*grammar->sets_list[rule->childset1], spot_tags);
					FILL_TAG_LIST(spot_tags);
					auto it = reading.tags_list.begin();
					for (; it != reading.tags_list.end(); ++it) {
						bool found = true;
						auto tmp = it;
						for (auto tag : *spot_tags) {
							if (*tmp != tag->hash) {
								found = false;
								break;
							}
							++tmp;
						}
						if (found) {
							found_spot = true;
							break;
						}
					}
					if (found_spot) {
						if (rule->flags & RF_AFTER) {
							std::advance(it, spot_tags->size());
						}
						if (it != reading.tags_list.end()) {
							insert_taglist_to_reading(it, *theTags, reading, mappings);
							did_insert = true;
						}
					}
				}

				if (!did_insert) {
					APPEND_TAGLIST_TO_READING(*theTags, reading);
				}
				if (!mappings->empty()) {
					splitMappings(mappings, *get_apply_to().cohort, reading, rule->type == K_MAP);
				}
				if (rule->type == K_MAP) {
					reading.mapped = true;
				}
				if (reading.hash != state_hash) {
					readings_changed = true;
				}
			}
			else if (rule->type == K_RESTORE) {
				bool did_restore = false;
				auto move_rs = [&](ReadingList& rl) {
					for (size_t i = 0; i < rl.size();) {
						if (doesSetMatchReading(*rl[i], rule->maplist->number)) {
							rl[i]->deleted = false;
							rl[i]->hit_by.push_back(rule->number);
							get_apply_to().cohort->readings.push_back(rl[i]);
							rl.erase(rl.begin() + i);
							did_restore = true;
						}
						else {
							++i;
						}
					}
				};

				if (rule->flags & RF_DELAYED) {
					move_rs(get_apply_to().cohort->delayed);
				}
				else if (rule->flags & RF_IGNORED) {
					move_rs(get_apply_to().cohort->ignored);
				}
				else {
					move_rs(get_apply_to().cohort->deleted);
				}

				if (did_restore) {
					TRACE;
				}
				finish_reading_loop = false;
			}
			else if (rule->type == K_REPLACE) {
				auto& reading = *get_apply_to().subreading;
				auto state_hash = reading.hash;
				index_ruleCohort_no.clear();
				TRACE;
				reading.noprint = false;

				auto excepts = ss_taglist.get();
				if (rule->sublist) {
					auto tags = ss_taglist.get();
					getTagList(*rule->sublist, tags);
					getTagsMatching(reading, tags, excepts);
				}

				reading.tags_list.clear();
				reading.tags_list.push_back(get_apply_to().cohort->wordform->hash);
				auto bform = reading.baseform;
				reading.baseform = 0;
				reflowReading(reading);
				auto mappings = ss_taglist.get();
				auto theTags = ss_taglist.get();
				getTagList(*rule->maplist, theTags);

				APPEND_TAGLIST_TO_READING(*theTags, reading);

				for (auto tter : *excepts) {
					addTagToReading(reading, tter);
				}
				if (!reading.baseform) {
					addTagToReading(reading, bform);
				}

				if (!mappings->empty()) {
					splitMappings(mappings, *get_apply_to().cohort, reading, true);
				}
				if (reading.hash != state_hash) {
					readings_changed = true;
				}
			}
			else if (rule->type == K_SUBSTITUTE) {
				// ToDo: Check whether this substitution will do nothing at all to the end result
				// ToDo: Not actually...instead, test whether any reading in the cohort already is the end result

				auto state_hash = get_apply_to().subreading->hash;
				auto theTags = ss_taglist.get();
				getTagList(*rule->sublist, theTags);
				bool appending = (theTags->size() == 1 && (*theTags)[0]->comparison_hash == grammar->tag_any);

				// Modify the list of tags to remove to be the actual list of tags present, including matching regex and icase tags
				FILL_TAG_LIST(theTags);

				// Perform the tag removal, remembering the position of the final removed tag for use as insertion spot
				size_t tpos = std::numeric_limits<size_t>::max();
				bool plain = true;
				for (size_t i = 0; i < get_apply_to().subreading->tags_list.size();) {
					auto& remter = get_apply_to().subreading->tags_list[i];

					if (plain && remter == (*theTags->begin())->hash) {
						if (get_apply_to().subreading->baseform == remter) {
							get_apply_to().subreading->baseform = 0;
						}
						remter = substtag;
						tpos = i;
						for (size_t j = 1; j < theTags->size() && i < get_apply_to().subreading->tags_list.size(); ++j, ++i) {
							auto& remter = get_apply_to().subreading->tags_list[i];
							auto tter = (*theTags)[j]->hash;
							if (remter != tter) {
								plain = false;
								break;
							}
							get_apply_to().subreading->tags_list.erase(get_apply_to().subreading->tags_list.begin() + i);
							get_apply_to().subreading->tags.erase(tter);
							if (get_apply_to().subreading->baseform == tter) {
								get_apply_to().subreading->baseform = 0;
							}
						}
						continue;
					}

					for (auto tter : *theTags) {
						if (remter != tter->hash) {
							continue;
						}
						tpos = i;
						remter = substtag;
						get_apply_to().subreading->tags.erase(tter->hash);
						if (get_apply_to().subreading->baseform == tter->hash) {
							get_apply_to().subreading->baseform = 0;
						}
					}

					++i;
				}

				if (appending) {
					get_apply_to().subreading->tags_list.push_back(substtag);
					tpos = get_apply_to().subreading->tags_list.size();
					reflowReading(*(get_apply_to().subreading));
				}

				// Should Substitute really do nothing if no tags were removed? 2013-10-21, Eckhard says this is expected behavior.
				if (tpos != std::numeric_limits<size_t>::max()) {
					if (!plain) {
						for (size_t i = 0; i < get_apply_to().subreading->tags_list.size() && i < tpos;) {
							if (get_apply_to().subreading->tags_list[i] == substtag) {
								get_apply_to().subreading->tags_list.erase(get_apply_to().subreading->tags_list.begin() + i);
								--tpos;
							}
							else {
								++i;
							}
						}
					}

					Tag* wf = nullptr;
					index_ruleCohort_no.clear();
					TRACE;
					get_apply_to().subreading->noprint = false;
					if (tpos >= get_apply_to().subreading->tags_list.size()) {
						tpos = get_apply_to().subreading->tags_list.size() - 1;
					}
					++tpos;
					auto mappings = ss_taglist.get();
					auto theTags = ss_taglist.get();
					getTagList(*rule->maplist, theTags);

					for (size_t i = 0; i < get_apply_to().subreading->tags_list.size();) {
						if (get_apply_to().subreading->tags_list[i] == substtag) {
							get_apply_to().subreading->tags_list.erase(get_apply_to().subreading->tags_list.begin() + i);
							tpos = i;

							for (auto tag : *theTags) {
								if (tag->type & T_VARSTRING) {
									tag = generateVarstringTag(tag);
								}
								if (tag->hash == grammar->tag_any) {
									break;
								}
								if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
									mappings->push_back(tag);
								}
								else {
									if (tag->type & T_WORDFORM) {
										wf = tag;
									}
									get_apply_to().subreading->tags_list.insert(get_apply_to().subreading->tags_list.begin() + tpos, tag->hash);
									++tpos;
								}
								if (updateValidRules(rules, intersects, tag->hash, *get_apply_to().subreading)) {
									iter_rules = intersects.find(rule->number);
									iter_rules_end = intersects.end();
								}
							}
						}
						else {
							++i;
						}
					}
					reflowReading(*get_apply_to().subreading);

					if (!mappings->empty()) {
						splitMappings(mappings, *get_apply_to().cohort, *get_apply_to().subreading, true);
					}
					if (wf && wf != get_apply_to().subreading->parent->wordform) {
						for (auto r : get_apply_to().subreading->parent->readings) {
							delTagFromReading(*r, get_apply_to().subreading->parent->wordform);
							addTagToReading(*r, wf);
						}
						for (auto r : get_apply_to().subreading->parent->deleted) {
							delTagFromReading(*r, get_apply_to().subreading->parent->wordform);
							addTagToReading(*r, wf);
						}
						for (auto r : get_apply_to().subreading->parent->delayed) {
							delTagFromReading(*r, get_apply_to().subreading->parent->wordform);
							addTagToReading(*r, wf);
						}
						get_apply_to().subreading->parent->wordform = wf;
						for (auto r : grammar->wf_rules) {
							if (doesWordformsMatch(wf, r->wordform)) {
								current.rule_to_cohorts[r->number].insert(get_apply_to().cohort);
								intersects.insert(r->number);
							}
							else {
								current.rule_to_cohorts[r->number].erase(get_apply_to().cohort);
							}
						}
						updateValidRules(rules, intersects, wf->hash, *get_apply_to().subreading);
						iter_rules = intersects.find(rule->number);
						iter_rules_end = intersects.end();
					}
				}
				if (get_apply_to().subreading->hash != state_hash) {
					readings_changed = true;
				}
			}
			else if (rule->type == K_APPEND) {
				index_ruleCohort_no.clear();
				TRACE;

				Tag* bf = nullptr;
				std::vector<TagList> readings;
				auto theTags = ss_taglist.get();
				getTagList(*rule->maplist, theTags);

				for (auto& tter : *theTags) {
					if (tter->type & T_VSTR) {
						VARSTRINGIFY(tter);
					}
				}

				for (auto tter : *theTags) {
					VARSTRINGIFY(tter);
					if (tter->type & T_BASEFORM) {
						bf = tter;
						readings.resize(readings.size() + 1);
					}
					if (bf == nullptr) {
						u_fprintf(ux_stderr, "Error: There must be a baseform before any other tags in APPEND on line %u.\n", rule->line);
						CG3Quit(1);
					}
					readings.back().push_back(tter);
				}

				for (const auto& rit : readings) {
					Reading* cReading = alloc_reading(get_apply_to().cohort);
					++numReadings;
					insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
					addTagToReading(*cReading, get_apply_to().cohort->wordform);
					cReading->hit_by.push_back(rule->number);
					cReading->noprint = false;
					TagList mappings;
					for (auto tter : rit) {
						uint32_t hash = tter->hash;
						VARSTRINGIFY(tter);
						if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
							mappings.push_back(tter);
						}
						else {
							hash = addTagToReading(*cReading, tter);
						}
						if (updateValidRules(rules, intersects, hash, *cReading)) {
							iter_rules = intersects.find(rule->number);
							iter_rules_end = intersects.end();
						}
					}
					if (!mappings.empty()) {
						splitMappings(mappings, *get_apply_to().cohort, *cReading);
					}
					get_apply_to().cohort->appendReading(cReading);
				}

				if (get_apply_to().cohort->readings.size() > 1) {
					foreach (rit, get_apply_to().cohort->readings) {
						if ((*rit)->noprint) {
							free_reading(*rit);
							rit = get_apply_to().cohort->readings.erase(rit);
							rit_end = get_apply_to().cohort->readings.end();
						}
					}
				}

				readings_changed = true;
				finish_reading_loop = false;
			}
			else if (rule->type == K_COPY) {
				// ToDo: Maybe just goto Substitute directly?
				Reading* cReading = get_apply_to().cohort->allocateAppendReading(*get_apply_to().reading);
				++numReadings;
				index_ruleCohort_no.clear();
				TRACE;
				cReading->hit_by.push_back(rule->number);
				cReading->noprint = false;

				if (rule->sublist) {
					auto tags = ss_taglist.get();
					getTagList(*rule->sublist, tags);
					auto excepts = ss_taglist.get();
					getTagsMatching(*cReading, tags, excepts);
					excepts->insert(excepts->end(), tags->begin(), tags->end());
					for (auto r = cReading; r; r = r->next) {
						for (auto tter : *excepts) {
							delTagFromReading(*r, tter);
						}
					}
				}

				auto mappings = ss_taglist.get();
				auto theTags = ss_taglist.get();
				getTagList(*rule->maplist, theTags);

				bool did_insert = false;
				if (rule->childset1) {
					auto spot_tags = ss_taglist.get();
					getTagList(*grammar->sets_list[rule->childset1], spot_tags);
					FILL_TAG_LIST(spot_tags);
					auto it = cReading->tags_list.begin();
					for (; it != cReading->tags_list.end(); ++it) {
						bool found = true;
						auto tmp = it;
						for (auto tag : *spot_tags) {
							if (*tmp != tag->hash) {
								found = false;
								break;
							}
							++tmp;
						}
						if (found) {
							break;
						}
					}
					if (rule->flags & RF_AFTER) {
						std::advance(it, spot_tags->size());
					}
					if (it != cReading->tags_list.end()) {
						insert_taglist_to_reading(it, *theTags, *cReading, mappings);
						did_insert = true;
					}
				}

				if (!did_insert) {
					APPEND_TAGLIST_TO_READING(*theTags, *cReading);
				}
				if (!mappings->empty()) {
					splitMappings(mappings, *get_apply_to().cohort, *cReading, true);
				}
				readings_changed = true;
				reflowReading(*cReading);
			}
			else if (rule->type == K_MERGECOHORTS) {
				index_ruleCohort_no.clear();

				CohortSet withs;
				Cohort* target = get_apply_to().cohort;
				withs.insert(target);
				Cohort* merge_at = target;
				for (auto it : rule->dep_tests) {
					auto& at = context_stack.back().attach_to;
					at.cohort = nullptr;
					at.reading = nullptr;
					at.subreading = nullptr;
					merge_with = nullptr;
					set_mark(target);
					dep_deep_seen.clear();
					tmpl_cntx.clear();
					Cohort* attach = nullptr;
					bool test_good = (runContextualTest(target->parent, target->local_number, it, &attach) && attach);

					profileRuleContext(test_good, rule, it);

					if (!test_good) {
						finish_reading_loop = false;
						return;
					}
					if (get_attach_to().cohort) {
						merge_at = get_attach_to().cohort;
						if (merge_with) {
							withs.insert(merge_with);
						}
					}
					else if (merge_with) {
						withs.insert(merge_with);
					}
					else {
						withs.insert(attach);
					}
				}

				size_t spacesInAddedWf = 0;
				context_stack.back().target.cohort = add_cohort(merge_at, spacesInAddedWf, &withs);

				for (auto c : withs) {
					size_t foundSpace = c->text.find_first_of(' ');
					while(spacesInAddedWf && foundSpace != std::string::npos) {
						c->text.erase(foundSpace, 1);
						foundSpace = c->text.find_first_of(' ');
						spacesInAddedWf--;
					}
					rem_cohort(c);
				}

				// If the last cohort was removed or inserted after, add <<< to the new end
				if (current.cohorts.back()->readings.front()->tags.count(endtag) == 0) {
					for (auto r : current.cohorts[current.cohorts.size() - 2]->readings) {
						delTagFromReading(*r, endtag);
					}
					for (auto r : current.cohorts.back()->readings) {
						addTagToReading(*r, endtag);
						if (updateValidRules(rules, intersects, endtag, *r)) {
							iter_rules = intersects.find(rule->number);
							iter_rules_end = intersects.end();
						}
					}
				}
				indexSingleWindow(current);
				readings_changed = true;

				reset_cohorts_for_loop = true;
			}
			else if (rule->type == K_COPYCOHORT) {
				Cohort* attach = nullptr;
				Cohort* cohort = context_stack.back().target.cohort;
				uint32_t c = cohort->local_number;
				dep_deep_seen.clear();
				tmpl_cntx.clear();
				context_stack.back().attach_to.cohort = nullptr;
				context_stack.back().attach_to.reading = nullptr;
				context_stack.back().attach_to.subreading = nullptr;
				if (runContextualTest(&current, c, rule->dep_target, &attach) && attach) {
					profileRuleContext(true, rule, rule->dep_target);

					if (get_attach_to().cohort) {
						attach = get_attach_to().cohort;
					}
					context_target = attach;
					bool good = true;
					for (auto it : rule->dep_tests) {
						context_stack.back().mark = attach;
						dep_deep_seen.clear();
						tmpl_cntx.clear();
						bool test_good = (runContextualTest(attach->parent, attach->local_number, it) != nullptr);

						profileRuleContext(test_good, rule, it);

						if (!test_good) {
							good = test_good;
							break;
						}
					}

					if (!good || cohort == attach || cohort->local_number == 0) {
						return;
					}

					auto childset = rule->childset2;
					if (rule->flags & RF_REVERSE) {
						std::swap(cohort, attach);
						childset = rule->childset1;
					}

					Cohort* cCohort = alloc_cohort(attach->parent);
					cCohort->global_number = gWindow->cohort_counter++;
					cCohort->wordform = cohort->wordform;
					insert_if_exists(cCohort->possible_sets, grammar->sets_any);

					auto theTags = ss_taglist.get();
					getTagList(*rule->maplist, theTags);

					for (auto& tter : *theTags) {
						if (tter->type & T_VSTR) {
							VARSTRINGIFY(tter);
						}
					}

					auto excepts = ss_taglist.get();
					if (rule->sublist) {
						auto tags = ss_taglist.get();
						getTagList(*rule->sublist, tags);
						getTagsMatching(*get_apply_to().subreading, tags, excepts);
						excepts->insert(excepts->end(), tags->begin(), tags->end());
					}

					std::vector<Reading*> rs;
					for (auto r : cohort->readings) {
						rs.clear();
						for (; r; r = r->next) {
							auto cReading = alloc_reading(cCohort);
							++numReadings;
							cReading->hit_by.push_back(rule->number);
							cReading->noprint = false;
							TagList mappings;
							for (auto hash : r->tags_list) {
								auto tter = grammar->single_tags[hash];
								if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(tter);
								}
								else {
									hash = addTagToReading(*cReading, hash);
								}
								if (updateValidRules(rules, intersects, hash, *cReading)) {
									iter_rules = intersects.find(rule->number);
									iter_rules_end = intersects.end();
								}
							}
							for (auto tter : *theTags) {
								auto hash = tter->hash;
								if (hash == grammar->tag_any) {
									continue;
								}
								if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(tter);
								}
								else {
									hash = addTagToReading(*cReading, hash);
								}
								if (updateValidRules(rules, intersects, hash, *cReading)) {
									iter_rules = intersects.find(rule->number);
									iter_rules_end = intersects.end();
								}
							}
							if (!mappings.empty()) {
								splitMappings(mappings, *cCohort, *cReading);
							}
							rs.push_back(cReading);
						}
						auto rn = rs.front();
						for (size_t j = 1; j < rs.size(); ++j) {
							rn->next = rs[j];
							rn = rn->next;
						}
						cCohort->appendReading(rs.front());
					}

					if (cCohort->readings.empty()) {
						initEmptyCohort(*cCohort);
						if (trace) {
							auto r = cCohort->readings.front();
							r->hit_by.push_back(rule->number);
							r->noprint = false;
						}
					}

					for (auto r : cCohort->readings) {
						for (; r; r = r->next) {
							for (auto tter : *excepts) {
								delTagFromReading(*r, tter);
							}
						}
					}

					if (cohort->wread) {
						cCohort->wread = alloc_reading(cCohort);
						for (auto hash : cohort->wread->tags_list) {
							hash = addTagToReading(*cCohort->wread, hash);
							if (updateValidRules(rules, intersects, hash, *cCohort->wread)) {
								iter_rules = intersects.find(rule->number);
								iter_rules_end = intersects.end();
							}
						}
					}

					current.parent->cohort_map[cCohort->global_number] = cCohort;
					current.parent->dep_window[cCohort->global_number] = cCohort;

					CohortSet edges;
					collect_subtree(edges, attach, childset);

					if (rule->flags & RF_BEFORE) {
						attach->parent->cohorts.insert(attach->parent->cohorts.begin() + edges.front()->local_number, cCohort);
						attach->parent->all_cohorts.insert(std::find(attach->parent->all_cohorts.begin() + edges.front()->local_number, attach->parent->all_cohorts.end(), edges.front()), cCohort);
						attachParentChild(*edges.front(), *cCohort);
					}
					else {
						attach->parent->cohorts.insert(attach->parent->cohorts.begin() + edges.back()->local_number + 1, cCohort);
						attach->parent->all_cohorts.insert(std::find(attach->parent->all_cohorts.begin() + edges.back()->local_number, attach->parent->all_cohorts.end(), edges.back()) + 1, cCohort);
						attachParentChild(*edges.back(), *cCohort);
					}

					reindex(attach->parent);
					indexSingleWindow(*attach->parent);
					readings_changed = true;
					reset_cohorts_for_loop = true;
				}
			}
			else if (rule->type == K_SETPARENT || rule->type == K_SETCHILD || rule->type == K_ADDRELATION || rule->type == K_SETRELATION || rule->type == K_REMRELATION || rule->type == K_ADDRELATIONS || rule->type == K_SETRELATIONS || rule->type == K_REMRELATIONS) {
				auto dep_target_cb = [&]() -> bool {
					Cohort* target = context_stack.back().target.cohort;
					Cohort* attach = context_stack.back().attach_to.cohort;
					swapper<Cohort*> sw((rule->flags & RF_REVERSE) != 0, target, attach);
					if (rule->type == K_SETPARENT || rule->type == K_SETCHILD) {
						has_dep = true;
						bool attached = false;
						if (rule->type == K_SETPARENT) {
							attached = attachParentChild(*attach, *target, (rule->flags & RF_ALLOWLOOP) != 0, (rule->flags & RF_ALLOWCROSS) != 0);
						}
						else {
							attached = attachParentChild(*target, *attach, (rule->flags & RF_ALLOWLOOP) != 0, (rule->flags & RF_ALLOWCROSS) != 0);
						}
						if (attached) {
							index_ruleCohort_no.clear();
							// force TRACE to use target
							Cohort* at_was = context_stack.back().attach_to.cohort;
							context_stack.back().attach_to.cohort = nullptr;
							TRACE;
							context_stack.back().attach_to.cohort = at_was;
							context_stack.back().target.subreading->noprint = false;
							has_dep = true;
							readings_changed = true;
						}
						return attached;
					}
					else if (rule->type == K_ADDRELATION || rule->type == K_SETRELATION || rule->type == K_REMRELATION) {
						has_relations = true;
						bool rel_did_anything = false;
						auto theTags = ss_taglist.get();
						getTagList(*rule->maplist, theTags);
						for (auto tter : *theTags) {
							VARSTRINGIFY(tter);
							if (rule->type == K_ADDRELATION) {
								attach->setRelated();
								target->setRelated();
								rel_did_anything |= target->addRelation(tter->hash, attach->global_number);
								add_relation_rtag(target, tter, attach->global_number);
							}
							else if (rule->type == K_SETRELATION) {
								attach->setRelated();
								target->setRelated();
								rel_did_anything |= target->setRelation(tter->hash, attach->global_number);
								set_relation_rtag(target, tter, attach->global_number);
							}
							else {
								rel_did_anything |= target->remRelation(tter->hash, attach->global_number);
								rem_relation_rtag(target, tter, attach->global_number);
							}
						}
						if (rel_did_anything) {
							index_ruleCohort_no.clear();
							// force TRACE to use target
							Cohort* at_was = context_stack.back().attach_to.cohort;
							context_stack.back().attach_to.cohort = nullptr;
							TRACE;
							context_stack.back().attach_to.cohort = at_was;
							context_stack.back().target.subreading->noprint = false;
							readings_changed = true;
						}
						// don't scan onwards if failed
						return true;
					}
					else if (rule->type == K_ADDRELATIONS || rule->type == K_SETRELATIONS || rule->type == K_REMRELATIONS) {
						has_relations = true;
						bool rel_did_anything = false;

						auto sublist = ss_taglist.get();
						getTagList(*rule->sublist, sublist);

						auto maplist = ss_taglist.get();
						getTagList(*rule->maplist, maplist);

						for (auto tter : *maplist) {
							VARSTRINGIFY(tter);
							if (rule->type == K_ADDRELATIONS) {
								target->setRelated();
								rel_did_anything |= target->addRelation(tter->hash, attach->global_number);
								add_relation_rtag(target, tter, attach->global_number);
							}
							else if (rule->type == K_SETRELATIONS) {
								target->setRelated();
								rel_did_anything |= target->setRelation(tter->hash, attach->global_number);
								set_relation_rtag(target, tter, attach->global_number);
							}
							else {
								rel_did_anything |= target->remRelation(tter->hash, attach->global_number);
								rem_relation_rtag(target, tter, attach->global_number);
							}
						}
						for (auto tter : *sublist) {
							VARSTRINGIFY(tter);
							if (rule->type == K_ADDRELATIONS) {
								attach->setRelated();
								rel_did_anything |= attach->addRelation(tter->hash, target->global_number);
								add_relation_rtag(attach, tter, target->global_number);
							}
							else if (rule->type == K_SETRELATIONS) {
								attach->setRelated();
								rel_did_anything |= attach->setRelation(tter->hash, target->global_number);
								set_relation_rtag(attach, tter, target->global_number);
							}
							else {
								rel_did_anything |= attach->remRelation(tter->hash, target->global_number);
								rem_relation_rtag(attach, tter, target->global_number);
							}
						}
						if (rel_did_anything) {
							index_ruleCohort_no.clear();
							// force TRACE to use target
							Cohort* at_was = context_stack.back().attach_to.cohort;
							context_stack.back().attach_to.cohort = nullptr;
							TRACE;
							context_stack.back().attach_to.cohort = at_was;
							context_stack.back().target.subreading->noprint = false;
							readings_changed = true;
						}
						// don't scan onwards if failed
						return true;
					}
					return true;
				};
				int32_t orgoffset = rule->dep_target->offset;
				auto seen_targets = ss_u32sv.get();

				ReadingSpec orgtarget = context_stack.back().target;
				while (true) {
					auto utags = ss_utags.get();
					auto usets = ss_usets.get();
					*utags = *context_stack.back().unif_tags;
					*usets = *context_stack.back().unif_sets;

					Cohort* attach = nullptr;
					Cohort* target = context_stack.back().target.cohort;
					seen_targets->insert(target->global_number);
					dep_deep_seen.clear();
					tmpl_cntx.clear();
					context_stack.back().attach_to.cohort = nullptr;
					context_stack.back().attach_to.reading = nullptr;
					context_stack.back().attach_to.subreading = nullptr;
					seen_barrier = false;
					if (runContextualTest(target->parent, target->local_number, rule->dep_target, &attach) && attach) {
						profileRuleContext(true, rule, rule->dep_target);

						bool break_after = seen_barrier || (rule->flags & RF_NEAREST);
						if (get_attach_to().cohort) {
							attach = get_attach_to().cohort;
						}
						context_target = attach;
						bool good = true;
						for (auto it : rule->dep_tests) {
							context_stack.back().mark = attach;
							dep_deep_seen.clear();
							tmpl_cntx.clear();
							bool test_good = (runContextualTest(attach->parent, attach->local_number, it) != nullptr);

							profileRuleContext(test_good, rule, it);

							if (!test_good) {
								good = test_good;
								break;
							}
						}
						if (!get_attach_to().cohort) {
							context_stack.back().attach_to.cohort = attach;
						}
						if (good) {
							ReadingSpec temp = context_stack.back().target;
							context_stack.back().target = orgtarget;
							bool attached = dep_target_cb();
							if (attached) {
								break;
							}
							else {
								context_stack.back().target = temp;
							}
						}
						if (break_after) {
							break;
						}
						if (seen_targets->count(attach->global_number)) {
							// We've found a cohort we have seen before...
							// We assume running the test again would result in the same, so don't bother.
							break;
						}
						seen_targets->insert(attach->global_number);
						// Did not successfully attach due to loop restrictions; look onwards from here
						context_stack.back().target = context_stack.back().attach_to;
						context_stack.back().unif_tags->swap(utags);
						context_stack.back().unif_sets->swap(usets);
						if (rule->dep_target->offset != 0) {
							// Temporarily set offset to +/- 1
							rule->dep_target->offset = ((rule->dep_target->offset < 0) ? -1 : 1);
						}
					}
					else {
						break;
					}
				}
				rule->dep_target->offset = orgoffset;
				finish_reading_loop = false;
			}
			else if (rule->type == K_REMPARENT) {
				// this is a per-cohort rule
				finish_reading_loop = false;
				TRACE;
				get_apply_to().cohort->dep_parent = DEP_NO_PARENT;
			}
			else if (rule->type == K_SWITCHPARENT) {
				// this is a per-cohort rule
				finish_reading_loop = false;
				TRACE;

				// collect cohorts
				Cohort* child = get_apply_to().cohort;
				Cohort* parent = current.parent->cohort_map[child->dep_parent];
				auto grandparent_number = parent->dep_parent;
				CohortSet siblings;
				for (auto iter : current.cohorts) {
					if (iter->dep_parent == parent->global_number && doesSetMatchCohortNormal(*iter, rule->childset1)) {
						siblings.insert(iter);
					}
				}

				// clear dependencies
				child->dep_parent = DEP_NO_PARENT;
				parent->dep_parent = DEP_NO_PARENT;
				for (auto s : siblings) {
					s->dep_parent = DEP_NO_PARENT;
				}

				// reattach
				auto it = current.parent->cohort_map.find(grandparent_number);
				if (it != current.parent->cohort_map.end()) {
					attachParentChild(*(it->second), *child);
				}
				attachParentChild(*child, *parent);
				for (auto s : siblings) {
					attachParentChild(*child, *s);
				}
			}
			else if (rule->type == K_MOVE_AFTER || rule->type == K_MOVE_BEFORE || rule->type == K_SWITCH) {
				// this is a per-cohort rule
				finish_reading_loop = false;
				// Calculate hash of current state to later compare whether this move/switch actually did anything
				uint32_t phash = 0;
				uint32_t chash = 0;
				for (const auto& c : current.cohorts) {
					phash = hash_value(c->global_number, phash);
					chash = hash_value(c->readings[0]->hash, chash);
				}

				// ToDo: ** tests will not correctly work for MOVE/SWITCH; cannot move cohorts between windows
				Cohort* attach = nullptr;
				Cohort* cohort = context_stack.back().target.cohort;
				uint32_t c = cohort->local_number;
				dep_deep_seen.clear();
				tmpl_cntx.clear();
				context_stack.back().attach_to.cohort = nullptr;
				context_stack.back().attach_to.reading = nullptr;
				context_stack.back().attach_to.subreading = nullptr;
				if (runContextualTest(&current, c, rule->dep_target, &attach) && attach && cohort->parent == attach->parent) {
					profileRuleContext(true, rule, rule->dep_target);

					if (get_attach_to().cohort) {
						attach = get_attach_to().cohort;
					}
					context_target = attach;
					bool good = true;
					for (auto it : rule->dep_tests) {
						context_stack.back().mark = attach;
						dep_deep_seen.clear();
						tmpl_cntx.clear();
						bool test_good = (runContextualTest(attach->parent, attach->local_number, it) != nullptr);

						profileRuleContext(test_good, rule, it);

						if (!test_good) {
							good = test_good;
							break;
						}
					}

					if (!good || cohort == attach || cohort->local_number == 0) {
						return;
					}

					swapper<Cohort*> sw((rule->flags & RF_REVERSE) != 0, attach, cohort);
					CohortSet cohorts;

					if (rule->type == K_SWITCH) {
						if (attach->local_number == 0) {
							return;
						}
						current.cohorts[cohort->local_number] = attach;
						current.cohorts[attach->local_number] = cohort;
						cohorts.insert(attach);
						cohorts.insert(cohort);
						auto ac_c = std::find(current.all_cohorts.begin() + cohort->local_number, current.all_cohorts.end(), cohort);
						auto ac_a = std::find(current.all_cohorts.begin() + attach->local_number, current.all_cohorts.end(), attach);
						*ac_c = attach;
						*ac_a = cohort;
					}
					else {
						CohortSet edges;
						collect_subtree(edges, attach, rule->childset2);
						collect_subtree(cohorts, cohort, rule->childset1);

						bool need_clean = false;
						for (auto iter : cohorts) {
							if (edges.count(iter)) {
								need_clean = true;
								break;
							}
						}

						if (need_clean) {
							if (isChildOf(cohort, attach)) {
								edges.erase(cohorts.rbegin(), cohorts.rend());
							}
							else /* if (isChildOf(attach, cohort)) */ {
								cohorts.erase(edges.rbegin(), edges.rend());
							}
						}
						if (cohorts.empty() || edges.empty()) {
							finish_reading_loop = false;
							return;
						}

						for (auto c : reversed(cohorts)) {
							current.cohorts.erase(current.cohorts.begin() + c->local_number);
							current.all_cohorts.erase(std::find(current.all_cohorts.begin() + c->local_number, current.all_cohorts.end(), c));
						}

						foreach (iter, current.cohorts) {
							(*iter)->local_number = UI32(std::distance(current.cohorts.begin(), iter));
						}

						for (auto iter : edges) {
							if (iter->parent != get_apply_to().cohort->parent) {
								u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move across window boundaries.\n", rule->line);
								CG3Quit(1);
							}
							for (auto cohort : cohorts) {
								if (iter == cohort) {
									u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move to a removed position.\n", rule->line);
									CG3Quit(1);
								}
							}
						}

						uint32_t spot = 0;
						auto ac_spot = current.all_cohorts.begin();
						if (rule->type == K_MOVE_BEFORE) {
							spot = edges.front()->local_number;
							if (spot == 0) {
								spot = 1;
							}
							ac_spot = std::find(current.all_cohorts.begin() + edges.front()->local_number, current.all_cohorts.end(), edges.front());
							if ((*ac_spot)->local_number == 0) {
								++ac_spot;
							}
						}
						else if (rule->type == K_MOVE_AFTER) {
							spot = edges.back()->local_number + 1;
							ac_spot = std::find(current.all_cohorts.begin() + edges.front()->local_number, current.all_cohorts.end(), edges.back());
							++ac_spot;
						}

						if (spot > current.cohorts.size()) {
							u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move out of bounds.\n", rule->line);
							CG3Quit(1);
						}

						for (auto c : reversed(cohorts)) {
							current.cohorts.insert(current.cohorts.begin() + spot, c);
							current.all_cohorts.insert(ac_spot, c);
						}
					}
					reindex();

					// Compare whether this move/switch actually did anything
					uint32_t phash_n = 0;
					uint32_t chash_n = 0;
					for (const auto& c : current.cohorts) {
						phash_n = hash_value(c->global_number, phash_n);
						chash_n = hash_value(c->readings[0]->hash, chash_n);
					}

					if (phash != phash_n || chash != chash_n) {
						if (++rule_hits[rule->number] > current.cohorts.size() * 100) {
							u_fprintf(ux_stderr, "Warning: Move/Switch endless loop detected for rule on line %u around input line %u - bailing out!\n", rule->line, get_apply_to().cohort->line_number);
							should_bail = true;
							finish_cohort_loop = false;
							return;
						}

						for (auto c : cohorts) {
							for (auto iter : c->readings) {
								iter->hit_by.push_back(rule->number);
							}
						}
						readings_changed = true;
						sorter.do_sort = true;
					}
				}
			}
			else if (rule->type == K_WITH) {
				TRACE;
				bool any_readings_changed = false;
				readings_changed = false;
				in_nested = true;
				for (auto& sr : rule->sub_rules) {
					Rule* cur_was = current_rule;
					Rule* rule_was = rule;
					current_rule = sr;
					rule = sr;
					bool result = false;
					do {
						readings_changed = false;
						result = runSingleRule(current, *rule, reading_cb, cohort_cb);
						any_readings_changed = any_readings_changed || result || readings_changed;
					} while ((result || readings_changed) && (rule->flags & RF_REPEAT) != 0) ;
					current_rule = cur_was;
					rule = rule_was;
				}
				in_nested = false;
				readings_changed = any_readings_changed;
				finish_reading_loop = false;
			}
			else if (rule->type != K_REMCOHORT) {
				TRACE;
			}
		};

		removed.resize(0);
		selected.resize(0);
		bool rv = runSingleRule(current, *rule, reading_cb, cohort_cb);
		if (rv || readings_changed) {
			if (!(rule->flags & RF_NOITERATE) && section_max_count != 1) {
				section_did_something = true;
			}
			rule_did_something = true;
		}
		if (should_bail) {
			goto bailout;
		}
		if (should_repeat) {
			goto repeat_rule;
		}

		if (rule_did_something) {
			iter_rules = intersects.find(rule->number);
			iter_rules_end = intersects.end();
			if (trace_rules.contains(rule->line)) {
				retval |= RV_TRACERULE;
			}
		}
		if (delimited) {
			break;
		}
		if (rule_did_something && (rule->flags & RF_REPEAT)) {
			index_ruleCohort_no.clear();
			goto repeat_rule;
		}

		if (false) {
		bailout:
			rule_hits[rule->number] = 0;
			index_ruleCohort_no.clear();
		}

		if (retval & RV_TRACERULE) {
			break;
		}
	}

	if (section_did_something) {
		retval |= RV_SOMETHING;
	}
	if (delimited) {
		retval |= RV_DELIMITED;
	}
	return retval;
}

uint32_t GrammarApplicator::runGrammarOnSingleWindow(SingleWindow& current) {
	if (!grammar->before_sections.empty() && !no_before_sections) {
		uint32_t rv = runRulesOnSingleWindow(current, runsections[-1]);
		if (rv & (RV_DELIMITED | RV_TRACERULE)) {
			return rv;
		}
	}

	if (!grammar->rules.empty() && !no_sections) {
		std::map<uint32_t, uint32_t> counter;
		// Caveat: This may look as if it is not recursing previous sections, but those rules are preprocessed into the successive sections so they are actually run.
		auto iter = runsections.begin();
		auto iter_end = runsections.end();
		for (size_t pass = 0; iter != iter_end; ++pass) {
			if (iter->first < 0 || (section_max_count && counter[iter->first] >= section_max_count)) {
				++iter;
				continue;
			}
			uint32_t rv = 0;
			if (debug_level > 0) {
				std::cerr << "Running section " << iter->first << " (rules " << *(iter->second.begin()) << " through " << *(--(iter->second.end())) << ") on window " << current.number << std::endl;
			}
			rv = runRulesOnSingleWindow(current, iter->second);
			++counter[iter->first];
			if (rv & (RV_DELIMITED | RV_TRACERULE)) {
				return rv;
			}
			if (!(rv & RV_SOMETHING)) {
				++iter;
				pass = 0;
			}
			if (pass >= 1000) {
				u_fprintf(ux_stderr, "Warning: Endless loop detected before input line %u. Window contents was:", numLines);
				UString tag;
				for (size_t i = 1; i < current.cohorts.size(); ++i) {
					Tag* t = current.cohorts[i]->wordform;
					tag.assign(t->tag.begin() + 2, t->tag.begin() + t->tag.size() - 2);
					u_fprintf(ux_stderr, " %S", tag.data());
				}
				u_fprintf(ux_stderr, "\n");
				u_fflush(ux_stderr);
				break;
			}
		}
	}

	if (!grammar->after_sections.empty() && !no_after_sections) {
		uint32_t rv = runRulesOnSingleWindow(current, runsections[-2]);
		if (rv & (RV_DELIMITED | RV_TRACERULE)) {
			return rv;
		}
	}

	return 0;
}

void GrammarApplicator::runGrammarOnWindow() {
	SingleWindow* current = gWindow->current;
	did_final_enclosure = false;

	for (const auto& vit : current->variables_set) {
		variables[vit.first] = vit.second;
	}
	for (auto vit : current->variables_rem) {
		variables.erase(vit);
	}
	variables[mprefix_key] = mprefix_value;

	if (has_dep) {
		reflowDependencyWindow();
		if (!input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
			for (auto cohort : gWindow->next.back()->cohorts) {
				gWindow->dep_window[cohort->global_number] = cohort;
			}
		}
	}
	if (has_relations) {
		reflowRelationWindow();
	}

	if (!grammar->parentheses.empty()) {
	label_scanParentheses:
		reverse_foreach (iter, current->cohorts) {
			Cohort* c = *iter;
			if (c->is_pleft == 0) {
				continue;
			}
			auto p = grammar->parentheses.find(c->is_pleft);
			if (p != grammar->parentheses.end()) {
				auto right = iter.base();
				--right;
				bool found = false;
				CohortVector encs;
				for (; right != current->cohorts.end(); ++right) {
					Cohort* s = *right;
					encs.push_back(s);
					if (s->is_pright == p->second) {
						found = true;
						break;
					}
				}
				if (found) {
					auto left = iter.base();
					--left;
					uint32_t lc = (*left)->local_number;
					++right;
					for (; right != current->cohorts.end(); ++right) {
						*left = *right;
						(*left)->local_number = lc;
						++lc;
						++left;
					}
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Info: Wrapped enclosure between lines %u to %u\n", encs.front()->line_number, encs.back()->line_number);
					}
					current->cohorts.resize(current->cohorts.size() - encs.size());
					auto ec = std::find(current->all_cohorts.begin() + encs.front()->local_number, current->all_cohorts.end(), encs.front());
					--ec;
					do {
						++ec;
						(*ec)->type |= CT_ENCLOSED;
						++((*ec)->enclosed);
					} while (*ec != encs.back());
					current->has_enclosures = true;
					goto label_scanParentheses;
				}
			}
		}
	}

	par_left_tag = 0;
	par_right_tag = 0;
	par_left_pos = 0;
	par_right_pos = 0;
	uint32_t pass = 0;

label_runGrammarOnWindow_begin:
	while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
		SingleWindow* tmp = gWindow->previous.front();
		printSingleWindow(tmp, *ux_stdout);
		free_swindow(tmp);
		gWindow->previous.erase(gWindow->previous.begin());
	}

	rule_hits.clear();
	index_ruleCohort_no.clear();
	current = gWindow->current;
	indexSingleWindow(*current);
	current->hit_external.clear();
	gWindow->rebuildCohortLinks(); // ToDo: Hack. This can be done better...

	++pass;
	if (pass > 1000) {
		u_fprintf(ux_stderr, "Warning: Endless loop detected before input line %u. Window contents was:", numLines);
		UString tag;
		for (size_t i = 1; i < current->cohorts.size(); ++i) {
			Tag* t = current->cohorts[i]->wordform;
			tag.assign(t->tag.begin() + 2, t->tag.begin() + t->tag.size() - 2);
			u_fprintf(ux_stderr, " %S", tag.data());
		}
		u_fprintf(ux_stderr, "\n");
		u_fflush(ux_stderr);
		return;
	}

	if (trace_encl) {
		uint32_t hitpass = std::numeric_limits<uint32_t>::max() - pass;
		for (auto& c : current->cohorts) {
			for (auto rit : c->readings) {
				rit->hit_by.push_back(hitpass);
			}
		}
	}

	uint32_t rv = runGrammarOnSingleWindow(*current);
	if (rv & RV_DELIMITED) {
		goto label_runGrammarOnWindow_begin;
	}

label_unpackEnclosures:
	if (current->has_enclosures) {
		size_t nc = current->all_cohorts.size();
		for (size_t i = 0; i < nc; ++i) {
			Cohort* c = current->all_cohorts[i];
			if (c->enclosed == 1) {
				size_t la = i;
				for (; la > 0; --la) {
					if (!(current->all_cohorts[la - 1]->type & (CT_ENCLOSED | CT_REMOVED | CT_IGNORED))) {
						--la;
						break;
					}
				}
				size_t ni = current->all_cohorts[la]->local_number;

				size_t ra = i;
				size_t ne = 0;
				for (; ra < nc; ++ra) {
					if (!(current->all_cohorts[ra]->type & (CT_ENCLOSED | CT_REMOVED | CT_IGNORED))) {
						break;
					}
					--(current->all_cohorts[ra]->enclosed);
					if (current->all_cohorts[ra]->enclosed == 0) {
						current->all_cohorts[ra]->type &= ~CT_ENCLOSED;
						++ne;
					}
				}

				current->cohorts.resize(current->cohorts.size() + ne, nullptr);
				for (size_t j = current->cohorts.size() - 1; j > ni + ne; --j) {
					current->cohorts[j] = current->cohorts[j - ne];
					current->cohorts[j]->local_number = UI32(j);
					current->cohorts[j - ne] = nullptr;
				}
				for (size_t j = 0; i < ra; ++i) {
					if (current->all_cohorts[i]->enclosed == 0) {
						current->cohorts[ni + j + 1] = current->all_cohorts[i];
						current->cohorts[ni + j + 1]->local_number = UI32(ni + j + 1);
						current->cohorts[ni + j + 1]->parent = current;
						++j;
					}
				}
				par_left_tag = current->all_cohorts[la + 1]->is_pleft;
				par_right_tag = current->all_cohorts[ra - 1]->is_pright;
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Info: Unwrapped enclosure between lines %u to %u\n", current->all_cohorts[la + 1]->line_number, current->all_cohorts[ra - 1]->line_number);
				}
				par_left_pos = UI32(ni + 1);
				par_right_pos = UI32(ni + ne);
				if (rv & RV_TRACERULE) {
					goto label_unpackEnclosures;
				}
				goto label_runGrammarOnWindow_begin;
			}
		}
		if (!did_final_enclosure) {
			par_left_tag = 0;
			par_right_tag = 0;
			par_left_pos = 0;
			par_right_pos = 0;
			did_final_enclosure = true;
			if (rv & RV_TRACERULE) {
				goto label_unpackEnclosures;
			}
			goto label_runGrammarOnWindow_begin;
		}
	}

	bool should_reflow = false;
	for (size_t i = current->all_cohorts.size(); i > 0; --i) {
		auto cohort = current->all_cohorts[i - 1];
		if (cohort->type & CT_IGNORED) {
			for (auto ins = i; ins > 0; --ins) {
				if (!(current->all_cohorts[ins - 1]->type & (CT_REMOVED | CT_ENCLOSED | CT_IGNORED))) {
					current->cohorts.insert(current->cohorts.begin() + current->all_cohorts[ins - 1]->local_number + 1, cohort);
					cohort->type &= ~CT_IGNORED;
					current->parent->cohort_map.insert(std::make_pair(cohort->global_number, cohort));
					should_reflow = true;
					break;
				}
			}
		}
	}
	if (should_reflow) {
		for (size_t i = 0; i < current->cohorts.size(); ++i) {
			current->cohorts[i]->local_number = UI32(i);
		}
		reflowDependencyWindow();
	}
}
}

// This helps the all_vislcg3.cpp profiling builds
#undef TRACE
